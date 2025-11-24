#define VK_USE_PLATFORM_WIN32_KHR
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <cmath>
#include <string>
#include <cstring>
#include <algorithm>
#include <cctype>
#include "renderer/window/window.h"
#include "renderer/vulkan/vulkan_renderer.h"
#include "renderer/core/constants.h"
#include "renderer/loading/loading_animation.h"
#include "renderer/text/text_renderer.h"
#include "renderer/ui/button/button.h"
#include "renderer/ui/slider/slider.h"
#include "renderer/ui/color_controller/color_controller.h"

// 应用状态
enum class AppState {
    LoadingCubes,  // 显示loading_cubes shader（启动界面）
    Loading,       // 显示加载动画
    Shader         // 显示shader
};

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 分配控制台用于调试输出
    AllocConsole();
    FILE* pCout;
    FILE* pCin;
    FILE* pCerr;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    freopen_s(&pCin, "CONIN$", "r", stdin);
    freopen_s(&pCerr, "CONOUT$", "w", stderr);
    SetConsoleTitleA("Shader App Debug Console");
    
    // Debug console initialized (output removed as requested)
    
    // Parse command line arguments for aspect ratio mode, stretch mode, and background mode
    AspectRatioMode aspectMode = AspectRatioMode::Keep;
    StretchMode stretchMode = StretchMode::Fit;
    BackgroundStretchMode backgroundMode = BackgroundStretchMode::Fit;  // 默认Fit模式
    
    if (lpCmdLine && strlen(lpCmdLine) > 0) {
        std::string cmdLine(lpCmdLine);
        // Convert to lowercase for case-insensitive matching
        std::string cmdLineLower = cmdLine;
        std::transform(cmdLineLower.begin(), cmdLineLower.end(), cmdLineLower.begin(), ::tolower);
        
        // Parse aspect ratio mode (case-insensitive)
        if (cmdLineLower.find("--aspect=keep") != std::string::npos || cmdLineLower.find("-a keep") != std::string::npos) {
            aspectMode = AspectRatioMode::Keep;
        } else if (cmdLineLower.find("--aspect=expand") != std::string::npos || cmdLineLower.find("-a expand") != std::string::npos) {
            aspectMode = AspectRatioMode::Expand;
        } else if (cmdLineLower.find("--aspect=keepwidth") != std::string::npos || cmdLineLower.find("-a keepwidth") != std::string::npos) {
            aspectMode = AspectRatioMode::KeepWidth;
        } else if (cmdLineLower.find("--aspect=keepheight") != std::string::npos || cmdLineLower.find("-a keepheight") != std::string::npos) {
            aspectMode = AspectRatioMode::KeepHeight;
        } else if (cmdLineLower.find("--aspect=center") != std::string::npos || cmdLineLower.find("-a center") != std::string::npos) {
            aspectMode = AspectRatioMode::Center;
        }
        
        // Parse stretch mode (case-insensitive): disabled, scaled, fit
        if (cmdLineLower.find("--stretch=disabled") != std::string::npos || cmdLineLower.find("-s disabled") != std::string::npos) {
            stretchMode = StretchMode::Disabled;
        } else if (cmdLineLower.find("--stretch=scaled") != std::string::npos || cmdLineLower.find("-s scaled") != std::string::npos ||
                   cmdLineLower.find("--stretch=canvas_items") != std::string::npos || cmdLineLower.find("-s canvas_items") != std::string::npos ||
                   cmdLineLower.find("--stretch=2d") != std::string::npos || cmdLineLower.find("-s 2d") != std::string::npos) {
            // Support "scaled", "canvas_items" (backward compatibility), and "2d" (shorthand)
            stretchMode = StretchMode::Scaled;
        } else if (cmdLineLower.find("--stretch=fit") != std::string::npos || cmdLineLower.find("-s fit") != std::string::npos) {
            stretchMode = StretchMode::Fit;
        }
        
        // Parse background mode (case-insensitive): fit, scaled
        if (cmdLineLower.find("--background=fit") != std::string::npos || cmdLineLower.find("-b fit") != std::string::npos) {
            backgroundMode = BackgroundStretchMode::Fit;
        } else if (cmdLineLower.find("--background=scaled") != std::string::npos || cmdLineLower.find("-b scaled") != std::string::npos) {
            backgroundMode = BackgroundStretchMode::Scaled;
        }
    }
    
    // 创建最大化窗口（保留边框），同时设置图标
    // 注意：图标必须在窗口创建前设置，这样任务栏图标才会正确显示
    if (!Window::Create(hInstance, WINDOW_WIDTH, WINDOW_HEIGHT, "Vulkan Shader Gouyu", nullptr, false, "assets/test.png")) {
        return 1;
    }
    
    // 再次设置图标以确保窗口图标也更新（虽然已经在Create中设置了）
    Window::SetIcon("assets/test.png");
    
    // 创建Vulkan渲染器
    VulkanRenderer renderer;
    
    // 初始化Vulkan
    if (!renderer.Initialize(Window::GetHandle(), hInstance)) {
        Window::Destroy();
        return 1;
    }
    
    // 设置宽高比模式和拉伸模式
    renderer.SetAspectRatioMode(aspectMode);
    renderer.SetStretchMode(stretchMode);
    renderer.SetBackgroundStretchMode(backgroundMode);
    
    // 设置鼠标移动回调，用于控制loading_cubes shader的相机旋转（在GPU上计算）
    Window::SetMouseMoveCallback([&renderer](float deltaX, float deltaY, bool leftButtonDown) {
        renderer.SetMouseInput(deltaX, deltaY, leftButtonDown);
    });
    
    // 设置键盘回调（用于WASD控制相机位置）
    // 注意：这里只设置回调，实际按键状态通过Window::IsKeyPressed()查询
    Window::SetKeyCallback([](int keyCode, bool isPressed) {
        // 可以在这里处理按键事件，但相机位置更新在游戏循环中进行
    });
    
    // 加载背景纹理
    if (!renderer.LoadBackgroundTexture("assets/space_background.png")) {
        printf("[WARNING] Failed to load background texture, continuing without background\n");
    }
    
    // 如果支持硬件光线追踪，尝试创建ray tracing pipeline
    if (renderer.IsRayTracingSupported()) {
        printf("[INFO] Hardware ray tracing is supported, attempting to create pipeline...\n");
        if (renderer.CreateRayTracingPipeline()) {
            printf("[INFO] Hardware ray tracing pipeline created successfully!\n");
        } else {
            printf("[INFO] Hardware ray tracing pipeline creation failed, will use software ray casting\n");
        }
    } else {
        printf("[INFO] Hardware ray tracing not supported, using software ray casting\n");
    }
    
    // 创建加载动画
    LoadingAnimation loadingAnim;
    AppState appState = AppState::Loading;  // 启动时显示Loading界面
    bool shaderPipelineCreated = false;
    bool loadingCubesPipelineCreated = false;
    
    // 启动时间（用于10秒后自动跳转）
    float startTime = 0.0f;
    bool startTimeSet = false;
    
    // 创建文字渲染器
    TextRenderer textRenderer;
    bool textRendererInitialized = false;
    
    // 初始化加载动画
    RECT clientRect;
    GetClientRect(Window::GetHandle(), &clientRect);
    float screenWidth = (float)(clientRect.right - clientRect.left);
    float screenHeight = (float)(clientRect.bottom - clientRect.top);
    
    // UI基准使用背景纹理大小（唯一的耦合点）
    // Disabled模式：使用背景纹理大小作为坐标系，UI完全和窗口无关
    // FIT模式：使用背景纹理大小作为坐标系
    // Scaled模式：使用实际窗口大小（和example一致）
    VkExtent2D uiExtent = {};
    if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
        // 使用背景纹理大小作为UI基准（如果没有背景则使用800x800）
        uiExtent = renderer.GetUIBaseSize();
    } else {
        // Scaled模式：使用实际窗口大小
        uiExtent = renderer.GetSwapchainExtent();
    }
    
    if (loadingAnim.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass(),
            uiExtent)) {
        
        // 设置加载动画位置
        // banter-loader是72x72px，需要居中
        // Disabled/FIT模式：UI基于背景纹理大小坐标系，使用背景纹理大小
        // Scaled模式：使用实际窗口大小
        float baseWidth = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? (float)uiExtent.width : screenWidth;
        float baseHeight = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? (float)uiExtent.height : screenHeight;
        
        float centerX = baseWidth / 2.0f - 36.0f; // 72px宽度的中心
        
        // 计算位置：上方40%位置显示方块动画
        float picCenterY = baseHeight * 0.4f - 36.0f; // 上方40%位置
        
        loadingAnim.SetPosition(centerX, picCenterY);
    }
    
    // 初始化文字渲染器
    if (textRenderer.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass())) {
        textRendererInitialized = true;
        textRenderer.LoadFont("Microsoft YaHei", 24); // 使用微软雅黑字体，24号大小
    }
    
    // 创建按钮组件（一行代码即可使用，所有参数由使用方传入）
    Button enterButton;
    // 使用带文本的按钮配置
    ButtonConfig buttonConfig = ButtonConfig::CreateRelativeWithText(
        0.5f, 0.75f, 300.0f, 50.0f,  // 相对位置(0.5, 0.75)，大小(300, 50) - 增加宽度以显示完整文本"点击进入"
        1.0f, 0.0f, 0.0f, 1.0f,      // 按钮颜色：红色
        "点击进入",                    // 按钮文本
        1.0f, 1.0f, 1.0f, 1.0f);     // 文本颜色：白色
    buttonConfig.zIndex = 25;  // 设置最高层级，确保"点击进入"按钮始终在最上层，不被遮挡
    buttonConfig.enableHoverEffect = true;   // 启用悬停效果
    buttonConfig.hoverEffectType = 0;        // 变暗效果
    buttonConfig.hoverEffectStrength = 0.3f;  // 效果强度30%
    if (enterButton.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass(),
            uiExtent,  // Disabled/FIT模式时使用固定的800x800，Scaled模式时使用实际窗口大小
            buttonConfig,
            textRendererInitialized ? &textRenderer : nullptr)) {
        // Disabled/FIT模式时，设置固定screenSize，不响应窗口变化
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            enterButton.SetFixedScreenSize(true);
        }
        // Scaled模式：不需要特殊设置，直接使用UpdateForWindowResize更新
        // 设置点击回调
        enterButton.SetOnClickCallback([&]() {
            printf("[DEBUG] Button clicked! Switching to Shader mode\n");
            appState = AppState::Shader;
            if (!shaderPipelineCreated) {
                if (!renderer.CreateGraphicsPipeline("renderer/shader/shader.vert.spv", "renderer/shader/shader.frag.spv")) {
                    Window::ShowError("Failed to create shader pipeline!");
                } else {
                    shaderPipelineCreated = true;
                }
            }
        });
    }
    
    // 声明蓝色按钮的3x3按钮矩阵和颜色控制器相关的变量（需要在蓝色按钮回调之前声明）
    std::vector<Button> boxColorButtons(9);  // 蓝色按钮专用的3x3按钮矩阵
    std::vector<bool> boxColorButtonsInitialized(9, false);
    bool boxColorButtonsExpanded = false;  // 蓝色按钮的展开状态
    std::vector<ColorController> boxColorControllers(9);
    std::vector<bool> boxColorControllersInitialized(9, false);
    
    // 创建第二个按钮（蓝色小按钮，在右半边）
    Button colorButton;
    ButtonConfig colorButtonConfig = ButtonConfig::CreateRelative(0.75f, 0.5f, 80.0f, 40.0f, 0.0f, 0.0f, 1.0f, 1.0f); // 相对位置(0.75, 0.5)，大小(80, 40)，蓝色
    if (colorButton.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass(),
            uiExtent,  // Disabled/FIT模式时使用固定的800x800，Scaled模式时使用实际窗口大小
            colorButtonConfig,
            textRendererInitialized ? &textRenderer : nullptr)) {
        // Disabled/FIT模式时，设置固定screenSize，不响应窗口变化
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            colorButton.SetFixedScreenSize(true);
        }
        // Scaled模式：不需要特殊设置，直接使用UpdateForWindowResize更新
        // 设置点击回调：显示/隐藏3x3按钮矩阵
        colorButton.SetOnClickCallback([&]() {
            boxColorButtonsExpanded = !boxColorButtonsExpanded;
            printf("[DEBUG] Color button clicked! Box color buttons expanded: %s\n", 
                   boxColorButtonsExpanded ? "true" : "false");
            // 切换所有方块颜色按钮的可见性
            for (int i = 0; i < 9; i++) {
                if (boxColorButtonsInitialized[i]) {
                    boxColorButtons[i].SetVisible(boxColorButtonsExpanded);
                }
            }
            // 隐藏所有颜色控制器
            for (int i = 0; i < 9; i++) {
                if (boxColorControllersInitialized[i]) {
                    boxColorControllers[i].SetVisible(false);
                }
            }
        });
    }
    
    // 创建左下角按钮（用于进入3D场景）
    Button leftButton;
    // 使用传统渲染方式，支持纹理和文本
    ButtonConfig leftButtonConfig = ButtonConfig::CreateRelativeWithTexture(
        0.1f, 0.9f, 60.0f, 60.0f,  // 相对位置(0.1, 0.9) - 左下角，大小(60, 60)
        "assets/shell.png");           // 使用shell.png纹理
    leftButtonConfig.zIndex = 0;  // 设置低层级，确保纹理按钮在文本按钮之下
    // 添加文本（可选）
    leftButtonConfig.enableText = true;
    leftButtonConfig.text = "3D";
    leftButtonConfig.textColorR = 1.0f;
    leftButtonConfig.textColorG = 1.0f;
    leftButtonConfig.textColorB = 1.0f;  // 白色文本
    leftButtonConfig.textColorA = 1.0f;
    // 启用悬停变暗效果
    leftButtonConfig.enableHoverEffect = true;   // 启用悬停效果
    leftButtonConfig.hoverEffectType = 0;         // 变暗效果
    leftButtonConfig.hoverEffectStrength = 0.3f; // 效果强度30%
    
    if (leftButton.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass(),
            uiExtent,  // Disabled/FIT模式时使用固定的800x800，Scaled模式时使用实际窗口大小
            leftButtonConfig,
            textRendererInitialized ? &textRenderer : nullptr,
            false)) { // 使用传统渲染方式，支持纹理
        // Disabled/FIT模式时，设置固定screenSize，不响应窗口变化
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            leftButton.SetFixedScreenSize(true);
        }
        // Scaled模式：不需要特殊设置，直接使用UpdateForWindowResize更新
        printf("[DEBUG] Left button (3D scene) initialized successfully with texture\n");
        // 设置点击回调：进入3D场景（LoadingCubes）
        leftButton.SetOnClickCallback([&]() {
            printf("[DEBUG] Left button clicked! Entering 3D scene (LoadingCubes)\n");
            appState = AppState::LoadingCubes;
            // 确保pipeline已创建
            if (!loadingCubesPipelineCreated) {
                if (!renderer.CreateLoadingCubesPipeline("renderer/loading/loading_cubes.vert.spv", "renderer/loading/loading_cubes.frag.spv")) {
                    if (!renderer.CreateLoadingCubesPipeline("renderer/loading/loading_cubes.vert", "renderer/loading/loading_cubes.frag")) {
                        printf("[ERROR] Failed to create loading cubes pipeline!\n");
                        Window::ShowError("Failed to create loading cubes pipeline!");
                        appState = AppState::Loading;  // 失败时返回Loading状态
                    } else {
                        loadingCubesPipelineCreated = true;
                    }
                } else {
                    loadingCubesPipelineCreated = true;
                }
            }
        });
    } else {
        printf("[DEBUG] Failed to initialize left button! Falling back to color mode\n");
        // 如果纹理加载失败，回退到颜色模式
        ButtonConfig fallbackConfig = ButtonConfig::CreateRelativeWithText(
            0.1f, 0.9f, 120.0f, 120.0f,  // 左下角位置
            0.2f, 0.6f, 1.0f, 1.0f,  // 蓝色背景
            "3D",
            1.0f, 1.0f, 1.0f, 1.0f); // 白色文本
        if (leftButton.Initialize(
                renderer.GetDevice(),
                renderer.GetPhysicalDevice(),
                renderer.GetCommandPool(),
                renderer.GetGraphicsQueue(),
                renderer.GetRenderPass(),
                uiExtent,  // Disabled/FIT模式时使用固定的800x800，Scaled模式时使用实际窗口大小
                fallbackConfig,
                textRendererInitialized ? &textRenderer : nullptr,
                false)) {
            // Disabled/FIT模式时，设置固定screenSize，不响应窗口变化
            if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
                leftButton.SetFixedScreenSize(true);
            }
            // Scaled模式：不需要特殊设置，直接使用UpdateForWindowResize更新
            leftButton.SetOnClickCallback([&]() {
                printf("[DEBUG] Left button clicked! Entering 3D scene (LoadingCubes)\n");
                appState = AppState::LoadingCubes;
                // 确保pipeline已创建
                if (!loadingCubesPipelineCreated) {
                    if (!renderer.CreateLoadingCubesPipeline("renderer/loading/loading_cubes.vert.spv", "renderer/loading/loading_cubes.frag.spv")) {
                        if (!renderer.CreateLoadingCubesPipeline("renderer/loading/loading_cubes.vert", "renderer/loading/loading_cubes.frag")) {
                            printf("[ERROR] Failed to create loading cubes pipeline!\n");
                            Window::ShowError("Failed to create loading cubes pipeline!");
                            appState = AppState::Loading;
                        } else {
                            loadingCubesPipelineCreated = true;
                        }
                    } else {
                        loadingCubesPipelineCreated = true;
                    }
                }
            });
        }
    }
    
    // 创建橙色滑块（左上角）
    Slider orangeSlider;
    SliderConfig sliderConfig(20.0f, 20.0f, 300.0f, 6.0f, 0.0f, 100.0f, 50.0f);  // 位置(20, 20)，大小(300, 6)，值范围0-100，默认50（轨道变细）
    sliderConfig.trackColorR = 0.3f;  // 轨道颜色：深灰色
    sliderConfig.trackColorG = 0.3f;
    sliderConfig.trackColorB = 0.3f;
    sliderConfig.fillColorR = 1.0f;   // 填充颜色：橙色
    sliderConfig.fillColorG = 0.5f;
    sliderConfig.fillColorB = 0.0f;
    sliderConfig.thumbColorR = 0.5f;  // 拖动点颜色：淡蓝色
    sliderConfig.thumbColorG = 0.8f;
    sliderConfig.thumbColorB = 1.0f;
    sliderConfig.thumbWidth = 20.0f;  // 拖动点宽度
    sliderConfig.thumbHeight = 20.0f;  // 拖动点高度（与宽度相等，确保圆形显示为正圆）
    sliderConfig.zIndex = 10;  // 设置层级
    sliderConfig.useRelativePosition = false;  // 使用绝对位置
    
    bool sliderInitialized = false;
    if (orangeSlider.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass(),
            uiExtent,
            sliderConfig,
            false)) {  // 使用传统渲染方式
        sliderInitialized = true;
        printf("[DEBUG] Orange slider initialized successfully\n");
    }
    
    // 声明右上角纹理按钮的3x3按钮矩阵相关的变量
    std::vector<Button> colorButtons(9);  // 右上角纹理按钮的3x3按钮矩阵
    std::vector<bool> colorButtonsInitialized(9, false);
    bool colorButtonsExpanded = false;  // 右上角纹理按钮的展开状态
    
    // 创建主按钮（使用frozed_flower.png纹理）
    Button flowerButton;
    ButtonConfig flowerButtonConfig = ButtonConfig::CreateRelativeWithTexture(
        0.9f, 0.1f, 40.0f, 40.0f,  // 相对位置(0.9, 0.1)，大小(40, 40) - 右上角，缩小一倍
        "assets/frozed_flower.png"); // 使用frozed_flower.png纹理
    flowerButtonConfig.zIndex = 20;  // 设置高层级，确保在主按钮之上
    
    bool flowerButtonInitialized = false;
    if (flowerButton.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass(),
            uiExtent,
            flowerButtonConfig,
            textRendererInitialized ? &textRenderer : nullptr,
            false)) { // 使用传统渲染方式，支持纹理
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            flowerButton.SetFixedScreenSize(true);
        }
        flowerButtonInitialized = true;
        printf("[DEBUG] Flower button initialized successfully\n");
    }
    
    // 创建9个不同颜色的按钮（3x3矩阵排列）
    // 注意：colorButtons、colorButtonsInitialized和colorButtonsExpanded已在上面声明
    
    // 定义9种颜色（RGB，0.0-1.0）
    struct ColorInfo {
        float r, g, b;
        const char* name;
    };
    ColorInfo colors[9] = {
        {1.0f, 0.0f, 0.0f, "红"},  // 红色
        {0.0f, 1.0f, 0.0f, "绿"},  // 绿色
        {0.0f, 0.0f, 1.0f, "蓝"},  // 蓝色
        {1.0f, 1.0f, 0.0f, "黄"},  // 黄色
        {1.0f, 0.0f, 1.0f, "紫"},  // 紫色
        {0.0f, 1.0f, 1.0f, "青"},  // 青色
        {1.0f, 0.5f, 0.0f, "橙"},  // 橙色
        {1.0f, 1.0f, 1.0f, "白"},  // 白色
        {0.0f, 0.0f, 0.0f, "黑"}   // 黑色
    };
    
    // 初始化9个颜色按钮（默认隐藏）
    // 注意：colorButtonsExpanded已在上面声明
    float buttonSize = 50.0f;  // 每个颜色按钮的大小
    float spacing = 10.0f;     // 按钮之间的间距
    float centerX = 0.9f;  // 主按钮的相对X位置
    float centerY = 0.1f;  // 主按钮的相对Y位置
    
    // 计算相对位置（使用屏幕尺寸）
    // 注意：screenWidth和screenHeight已经在上面定义过了，这里使用不同的变量名
    float colorBtnScreenWidth = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                 (float)uiExtent.width : screenWidth;
    float colorBtnScreenHeight = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                  (float)uiExtent.height : screenHeight;
    
    // 按钮矩阵的位置（在主按钮下方，居中）
    float matrixCenterX = centerX;  // 与主按钮相同的X位置
    float matrixCenterY = centerY + (80.0f + spacing + 80.0f) / colorBtnScreenHeight;  // 主按钮下方，预留空间
    
    // 计算每个按钮的相对位置（相对于矩阵中心）
    float buttonSizeRel = buttonSize / colorBtnScreenWidth;
    float buttonSizeRelY = buttonSize / colorBtnScreenHeight;
    float spacingRelX = spacing / colorBtnScreenWidth;
    float spacingRelY = spacing / colorBtnScreenHeight;
    
    // 矩阵总尺寸（用于居中计算）
    float matrixWidth = 3.0f * buttonSizeRel + 2.0f * spacingRelX;
    float matrixHeight = 3.0f * buttonSizeRelY + 2.0f * spacingRelY;
    
    // 矩阵左上角的位置（用于计算每个按钮的位置）
    float matrixStartX = matrixCenterX - matrixWidth / 2.0f;
    float matrixStartY = matrixCenterY - matrixHeight / 2.0f;
    
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int index = row * 3 + col;
            // 计算每个按钮的相对位置（相对于屏幕）
            float relX = matrixStartX + col * (buttonSizeRel + spacingRelX);
            float relY = matrixStartY + row * (buttonSizeRelY + spacingRelY);
            
            ButtonConfig colorBtnConfig = ButtonConfig::CreateRelativeWithText(
                relX, relY, buttonSize, buttonSize,
                colors[index].r, colors[index].g, colors[index].b, 1.0f,
                colors[index].name,
                1.0f - colors[index].r, 1.0f - colors[index].g, 1.0f - colors[index].b, 1.0f);  // 文本颜色取反色
            colorBtnConfig.zIndex = 15;  // 设置层级，在主按钮之下但高于其他按钮
            colorBtnConfig.shapeType = 1;  // 设置为圆形按钮
            
            if (colorButtons[index].Initialize(
                    renderer.GetDevice(),
                    renderer.GetPhysicalDevice(),
                    renderer.GetCommandPool(),
                    renderer.GetGraphicsQueue(),
                    renderer.GetRenderPass(),
                    uiExtent,
                    colorBtnConfig,
                    textRendererInitialized ? &textRenderer : nullptr)) {
                if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
                    colorButtons[index].SetFixedScreenSize(true);
                }
                // 默认隐藏
                colorButtons[index].SetVisible(false);
                colorButtonsInitialized[index] = true;
                
                // 设置点击回调：显示对应的颜色控制器
                int boxIndex = index;  // 捕获index用于回调
                colorButtons[index].SetOnClickCallback([&, boxIndex]() {
                    printf("[DEBUG] Color button %d clicked! Showing color controller for box %d\n", 
                           boxIndex, boxIndex);
                    // 隐藏所有其他颜色控制器
                    for (int i = 0; i < 9; i++) {
                        if (boxColorControllersInitialized[i]) {
                            boxColorControllers[i].SetVisible(i == boxIndex);
                        }
                    }
                });
            }
        }
    }
    
    // 设置主按钮的点击回调：展开/收起颜色按钮
    if (flowerButtonInitialized) {
        flowerButton.SetOnClickCallback([&]() {
            colorButtonsExpanded = !colorButtonsExpanded;
            printf("[DEBUG] Flower button clicked! Color buttons expanded: %s\n", 
                   colorButtonsExpanded ? "true" : "false");
            // 切换所有颜色按钮的可见性
            for (int i = 0; i < 9; i++) {
                if (colorButtonsInitialized[i]) {
                    colorButtons[i].SetVisible(colorButtonsExpanded);
                }
            }
            // 隐藏所有颜色控制器
            for (int i = 0; i < 9; i++) {
                if (boxColorControllersInitialized[i]) {
                    boxColorControllers[i].SetVisible(false);
                }
            }
        });
    }
    
    // 为蓝色按钮创建独立的3x3按钮矩阵（用于控制方块颜色）
    // 计算按钮矩阵的位置（在蓝色按钮右侧，更靠右一些）
    float boxBtnMatrixCenterX = 0.85f;  // 蓝色按钮右侧
    float boxBtnMatrixCenterY = 0.5f;  // 与蓝色按钮相同的Y位置
    
    float boxBtnButtonSize = 40.0f;  // 按钮大小
    float boxBtnSpacing = 8.0f;     // 按钮之间的间距
    
    // 计算每个按钮的相对位置（相对于矩阵中心）
    float boxBtnButtonSizeRel = boxBtnButtonSize / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                     (float)uiExtent.width : screenWidth);
    float boxBtnButtonSizeRelY = boxBtnButtonSize / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                      (float)uiExtent.height : screenHeight);
    float boxBtnSpacingRelX = boxBtnSpacing / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                (float)uiExtent.width : screenWidth);
    float boxBtnSpacingRelY = boxBtnSpacing / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                (float)uiExtent.height : screenHeight);
    
    // 矩阵总尺寸（用于居中计算）
    float boxBtnMatrixWidth = 3.0f * boxBtnButtonSizeRel + 2.0f * boxBtnSpacingRelX;
    float boxBtnMatrixHeight = 3.0f * boxBtnButtonSizeRelY + 2.0f * boxBtnSpacingRelY;
    
    // 矩阵左上角的位置（用于计算每个按钮的位置）
    float boxBtnMatrixStartX = boxBtnMatrixCenterX - boxBtnMatrixWidth / 2.0f;
    float boxBtnMatrixStartY = boxBtnMatrixCenterY - boxBtnMatrixHeight / 2.0f;
    
    // 创建9个方块颜色按钮（3x3矩阵）
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            int index = row * 3 + col;
            // 计算每个按钮的相对位置（相对于屏幕）
            float relX = boxBtnMatrixStartX + col * (boxBtnButtonSizeRel + boxBtnSpacingRelX);
            float relY = boxBtnMatrixStartY + row * (boxBtnButtonSizeRelY + boxBtnSpacingRelY);
            
            // 按钮显示方块索引（0-8）
            char buttonText[4];
            sprintf_s(buttonText, "%d", index);
            
            ButtonConfig boxBtnConfig = ButtonConfig::CreateRelativeWithText(
                relX, relY, boxBtnButtonSize, boxBtnButtonSize,
                0.3f, 0.3f, 0.8f, 1.0f,  // 按钮颜色：淡蓝色
                buttonText,
                1.0f, 1.0f, 1.0f, 1.0f);  // 文本颜色：白色
            boxBtnConfig.zIndex = 15;  // 设置层级，低于滑块
            boxBtnConfig.shapeType = 0;  // 方形按钮
            
            if (boxColorButtons[index].Initialize(
                    renderer.GetDevice(),
                    renderer.GetPhysicalDevice(),
                    renderer.GetCommandPool(),
                    renderer.GetGraphicsQueue(),
                    renderer.GetRenderPass(),
                    uiExtent,
                    boxBtnConfig,
                    textRendererInitialized ? &textRenderer : nullptr)) {
                if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
                    boxColorButtons[index].SetFixedScreenSize(true);
                }
                // 默认隐藏
                boxColorButtons[index].SetVisible(false);
                boxColorButtonsInitialized[index] = true;
                
                // 设置点击回调：显示对应的颜色控制器
                int boxIndex = index;  // 捕获index用于回调
                boxColorButtons[index].SetOnClickCallback([&, boxIndex]() {
                    printf("[DEBUG] Box color button %d clicked! Showing color controller for box %d\n", 
                           boxIndex, boxIndex);
                    // 隐藏所有其他颜色控制器
                    for (int i = 0; i < 9; i++) {
                        if (boxColorControllersInitialized[i]) {
                            boxColorControllers[i].SetVisible(i == boxIndex);
                        }
                    }
                });
            }
        }
    }
    
    // 为每个方块创建颜色控制器（3x3矩阵，共9个）
    // 注意：boxColorControllers和boxColorControllersInitialized已在上面声明
    
    // 计算颜色控制器的位置（在按钮矩阵右侧，与按钮矩阵对齐）
    float controllerBaseX = boxBtnMatrixCenterX + boxBtnMatrixWidth / 2.0f + 20.0f / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                                                      (float)uiExtent.width : screenWidth);  // 按钮矩阵右侧20像素
    float controllerBaseY = boxBtnMatrixCenterY;  // 与按钮矩阵相同的Y位置（垂直居中）
    
    for (int i = 0; i < 9; i++) {
        ColorControllerConfig boxControllerConfig;
        boxControllerConfig.relativeX = controllerBaseX;  // 按钮矩阵右侧
        boxControllerConfig.relativeY = controllerBaseY;
        // 颜色控制器尺寸缩小到原来的40%
        boxControllerConfig.sliderWidth = 80.0f;      // 200 * 0.4
        boxControllerConfig.sliderHeight = 2.4f;      // 6 * 0.4
        boxControllerConfig.sliderSpacing = 20.0f;    // 50 * 0.4
        boxControllerConfig.displayWidth = 40.0f;      // 100 * 0.4
        boxControllerConfig.displayHeight = 20.0f;     // 50 * 0.4
        boxControllerConfig.displayOffsetY = 12.0f;    // 30 * 0.4
        boxControllerConfig.initialR = 1.0f;  // 默认白色
        boxControllerConfig.initialG = 1.0f;
        boxControllerConfig.initialB = 1.0f;
        boxControllerConfig.initialA = 1.0f;
        boxControllerConfig.zIndex = 30;  // 高于按钮的层级，确保滑块可以点击
        boxControllerConfig.visible = false;  // 默认隐藏
        boxControllerConfig.screenWidth = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                          (float)uiExtent.width : screenWidth;
        boxControllerConfig.screenHeight = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                           (float)uiExtent.height : screenHeight;
        
        if (boxColorControllers[i].Initialize(
                renderer.GetDevice(),
                renderer.GetPhysicalDevice(),
                renderer.GetCommandPool(),
                renderer.GetGraphicsQueue(),
                renderer.GetRenderPass(),
                uiExtent,
                boxControllerConfig,
                textRendererInitialized ? &textRenderer : nullptr)) {
            if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
                boxColorControllers[i].SetFixedScreenSize(true);
            }
            boxColorControllersInitialized[i] = true;
            
            // 设置颜色变化回调：更新对应方块的颜色
            int boxIndex = i;  // 捕获i用于回调
            boxColorControllers[i].SetOnColorChangedCallback([&, boxIndex](float r, float g, float b, float a) {
                printf("[DEBUG] Box %d color changed to (%.2f, %.2f, %.2f, %.2f)\n", 
                       boxIndex, r, g, b, a);
                loadingAnim.SetBoxColor(boxIndex, r, g, b, a);
            });
            
            printf("[DEBUG] Box color controller %d initialized successfully\n", i);
        }
    }
    
    // 创建左半边的颜色调整按钮（使用test.png纹理）
    Button colorAdjustButton;
    ButtonConfig colorAdjustButtonConfig = ButtonConfig::CreateRelativeWithTexture(
        0.1f, 0.3f, 60.0f, 60.0f,  // 相对位置(0.1, 0.3) - 左半边，大小(60, 60)
        "assets/test.png");           // 使用test.png纹理
    colorAdjustButtonConfig.zIndex = 18;  // 设置层级
    colorAdjustButtonConfig.enableText = false;
    
    bool colorAdjustButtonInitialized = false;
    float buttonColorR = 1.0f;  // 按钮当前颜色（RGBA，0.0-1.0）
    float buttonColorG = 1.0f;
    float buttonColorB = 1.0f;
    float buttonColorA = 1.0f;
    
    if (colorAdjustButton.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass(),
            uiExtent,
            colorAdjustButtonConfig,
            textRendererInitialized ? &textRenderer : nullptr,
            false)) { // 使用传统渲染方式，支持纹理
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            colorAdjustButton.SetFixedScreenSize(true);
        }
        colorAdjustButtonInitialized = true;
        printf("[DEBUG] Color adjust button initialized successfully\n");
    }
    
    // 创建颜色控制器组件（封装4个RGBA滑块和颜色显示区域）
    ColorController colorController;
    ColorControllerConfig colorControllerConfig;
    colorControllerConfig.relativeX = 0.1f;  // 与按钮相同的X位置
    colorControllerConfig.relativeY = 0.3f + 80.0f / ((stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                                      (float)uiExtent.height : screenHeight);  // 按钮下方
    colorControllerConfig.sliderWidth = 200.0f;
    colorControllerConfig.sliderHeight = 6.0f;
    colorControllerConfig.sliderSpacing = 50.0f;
    colorControllerConfig.displayWidth = 100.0f;
    colorControllerConfig.displayHeight = 50.0f;
    colorControllerConfig.displayOffsetY = 30.0f;
    colorControllerConfig.initialR = buttonColorR;
    colorControllerConfig.initialG = buttonColorG;
    colorControllerConfig.initialB = buttonColorB;
    colorControllerConfig.initialA = buttonColorA;
    colorControllerConfig.zIndex = 19;
    colorControllerConfig.visible = false;  // 默认隐藏
    colorControllerConfig.screenWidth = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                       (float)uiExtent.width : screenWidth;
    colorControllerConfig.screenHeight = (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) ? 
                                        (float)uiExtent.height : screenHeight;
    
    bool colorControllerInitialized = false;
    if (colorController.Initialize(
            renderer.GetDevice(),
            renderer.GetPhysicalDevice(),
            renderer.GetCommandPool(),
            renderer.GetGraphicsQueue(),
            renderer.GetRenderPass(),
            uiExtent,
            colorControllerConfig,
            textRendererInitialized ? &textRenderer : nullptr)) {
        if (stretchMode == StretchMode::Fit || stretchMode == StretchMode::Disabled) {
            colorController.SetFixedScreenSize(true);
        }
        colorControllerInitialized = true;
        printf("[DEBUG] Color controller initialized successfully\n");
    }
    
    // 设置颜色控制器的颜色变化回调：更新右边按钮（colorButton）的颜色
    colorController.SetOnColorChangedCallback([&](float r, float g, float b, float a) {
        buttonColorR = r;
        buttonColorG = g;
        buttonColorB = b;
        buttonColorA = a;
        
        // 更新右边按钮（colorButton）的颜色
        colorButton.SetColor(buttonColorR, buttonColorG, buttonColorB, buttonColorA);
        
        printf("[DEBUG] Color changed to (%.2f, %.2f, %.2f, %.2f)\n", 
               buttonColorR, buttonColorG, buttonColorB, buttonColorA);
    });
    
    // 设置颜色调整按钮的点击回调：显示/隐藏颜色控制器
    if (colorAdjustButtonInitialized) {
        colorAdjustButton.SetOnClickCallback([&]() {
            bool visible = !colorController.IsVisible();
            colorController.SetVisible(visible);
            printf("[DEBUG] Color adjust button clicked! Color controller visible: %s\n", 
                   visible ? "true" : "false");
        });
    }
    
    // 创建按钮向量用于传递给渲染函数
    std::vector<Button*> allButtons;
    if (flowerButtonInitialized) {
        allButtons.push_back(&flowerButton);
    }
    for (int i = 0; i < 9; i++) {
        if (colorButtonsInitialized[i]) {
            allButtons.push_back(&colorButtons[i]);
        }
    }
    // 添加蓝色按钮的3x3按钮矩阵
    for (int i = 0; i < 9; i++) {
        if (boxColorButtonsInitialized[i]) {
            allButtons.push_back(&boxColorButtons[i]);
        }
    }
    if (colorAdjustButtonInitialized) {
        allButtons.push_back(&colorAdjustButton);
    }
    // 添加颜色控制器的按钮和滑块
    if (colorControllerInitialized) {
        std::vector<Button*> colorControllerButtons = colorController.GetButtons();
        for (Button* btn : colorControllerButtons) {
            allButtons.push_back(btn);
        }
    }
    // 添加方块颜色控制器的按钮和滑块
    for (int i = 0; i < 9; i++) {
        if (boxColorControllersInitialized[i]) {
            std::vector<Button*> boxControllerButtons = boxColorControllers[i].GetButtons();
            for (Button* btn : boxControllerButtons) {
                allButtons.push_back(btn);
            }
        }
    }
    
    // 主循环
    float time = 0.0f;
    MSG msg = {};
    
    // 帧率计算相关变量
    LARGE_INTEGER frequency, lastTime, currentTime;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastTime);
    float fps = 0.0f;
    float fpsUpdateInterval = 0.1f;  // 每0.1秒更新一次帧率
    float fpsUpdateTimer = 0.0f;
    int fpsFrameCount = 0;
    
    while (Window::IsRunning()) {
        // 处理消息
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                Window::SetRunning(false);
                break;
            } else if (msg.message == WM_SIZE) {
                // Window size changed
                // FIT模式时，UI不响应窗口变化，只根据NDC坐标系自然变化
                if (stretchMode == StretchMode::Scaled) {
                    // Scaled模式：更新拉伸参数
                    enterButton.SetStretchParams(renderer.GetStretchParams());
                    colorButton.SetStretchParams(renderer.GetStretchParams());
                    leftButton.SetStretchParams(renderer.GetStretchParams());
                    if (sliderInitialized) {
                        orangeSlider.SetStretchParams(renderer.GetStretchParams());
                    }
                } else if (stretchMode != StretchMode::Fit) {
                    // 非FIT/Scaled模式：更新UI位置
                RECT clientRect;
                GetClientRect(Window::GetHandle(), &clientRect);
                float newScreenWidth = (float)(clientRect.right - clientRect.left);
                float newScreenHeight = (float)(clientRect.bottom - clientRect.top);
                
                // Update animation positions (maintain relative positions)
                float centerX = newScreenWidth / 2.0f - 36.0f;
                float picCenterY = newScreenHeight * 0.4f - 36.0f;
                loadingAnim.SetPosition(centerX, picCenterY);
                
                    // Update button positions (maintain relative positions)
                    enterButton.UpdateForWindowResize(newScreenWidth, newScreenHeight);
                    colorButton.UpdateForWindowResize(newScreenWidth, newScreenHeight);
                    leftButton.UpdateForWindowResize(newScreenWidth, newScreenHeight);
                    if (sliderInitialized) {
                        orangeSlider.UpdateForWindowResize(newScreenWidth, newScreenHeight);
                    }
                    // 更新主按钮和颜色按钮位置
                    if (flowerButtonInitialized) {
                        flowerButton.UpdateForWindowResize(newScreenWidth, newScreenHeight);
                    }
                    for (int i = 0; i < 9; i++) {
                        if (colorButtonsInitialized[i]) {
                            colorButtons[i].UpdateForWindowResize(newScreenWidth, newScreenHeight);
                        }
                        // 更新蓝色按钮的3x3按钮矩阵位置
                        if (boxColorButtonsInitialized[i]) {
                            boxColorButtons[i].UpdateForWindowResize(newScreenWidth, newScreenHeight);
                        }
                    }
                    // 更新颜色调整按钮和颜色显示按钮位置
                    if (colorAdjustButtonInitialized) {
                        colorAdjustButton.UpdateForWindowResize(newScreenWidth, newScreenHeight);
                    }
                    // 更新颜色控制器位置
                    if (colorControllerInitialized) {
                        colorController.UpdateScreenSize(newScreenWidth, newScreenHeight);
                    }
                    // 更新方块颜色控制器位置
                    for (int i = 0; i < 9; i++) {
                        if (boxColorControllersInitialized[i]) {
                            boxColorControllers[i].UpdateScreenSize(newScreenWidth, newScreenHeight);
                        }
                    }
                }
                // FIT模式时，UI会根据NDC坐标系自动变化，无需手动更新
                
                // Force redraw on next frame
                InvalidateRect(Window::GetHandle(), nullptr, FALSE);
            } else if (msg.message == WM_LBUTTONDOWN) {
                // 检测鼠标点击
                if (appState == AppState::Loading) {
                    // Get client coordinates (WM_LBUTTONDOWN lParam is already client coordinates)
                    POINT pt;
                    pt.x = LOWORD(msg.lParam);
                    pt.y = HIWORD(msg.lParam);
                    
                    // Also get current client rect to verify coordinate system
                    RECT clientRect;
                    GetClientRect(Window::GetHandle(), &clientRect);
                    
                    // Get current screen size for debug output
                    float currentScreenWidth = (float)(clientRect.right - clientRect.left);
                    float currentScreenHeight = (float)(clientRect.bottom - clientRect.top);
                    
                    // IMPORTANT: Recalculate button position based on current window size
                    // This ensures button position is always correct even if WM_SIZE was missed
                    // FIT模式时，UI不响应窗口变化，只根据NDC坐标系自然变化
                    if (stretchMode != StretchMode::Fit) {
                    enterButton.UpdateForWindowResize(currentScreenWidth, currentScreenHeight);
                    colorButton.UpdateForWindowResize(currentScreenWidth, currentScreenHeight);
                    leftButton.UpdateForWindowResize(currentScreenWidth, currentScreenHeight);
                    if (colorAdjustButtonInitialized) {
                        colorAdjustButton.UpdateForWindowResize(currentScreenWidth, currentScreenHeight);
                    }
                    // 更新颜色控制器位置
                    if (colorControllerInitialized) {
                        colorController.UpdateScreenSize(currentScreenWidth, currentScreenHeight);
                    }
                    // 更新方块颜色控制器位置
                    for (int i = 0; i < 9; i++) {
                        if (boxColorControllersInitialized[i]) {
                            boxColorControllers[i].UpdateScreenSize(currentScreenWidth, currentScreenHeight);
                        }
                    }
                    }
                    
                    // Debug output: show click position and button position
                    printf("[DEBUG] Click position: (%d, %d)\n", pt.x, pt.y);
                    printf("[DEBUG] Screen size: (%.0f, %.0f)\n", currentScreenWidth, currentScreenHeight);
                    printf("[DEBUG] Button position: (%.2f, %.2f)\n", enterButton.GetX(), enterButton.GetY());
                    printf("[DEBUG] Button size: (%.2f, %.2f)\n", enterButton.GetWidth(), enterButton.GetHeight());
                    printf("[DEBUG] Button bounds: X[%.2f - %.2f], Y[%.2f - %.2f]\n", 
                           enterButton.GetX(), enterButton.GetX() + enterButton.GetWidth(),
                           enterButton.GetY(), enterButton.GetY() + enterButton.GetHeight());
                    
                    // FIT模式时，需要将窗口坐标转换为UI坐标系坐标
                    // Scaled模式时，也需要进行坐标转换
                    float clickX = (float)pt.x;
                    float clickY = (float)pt.y;
                    
                    if (stretchMode == StretchMode::Scaled) {
                        // Scaled模式：将屏幕坐标转换为逻辑坐标
                        // 获取拉伸参数
                        StretchParams stretchParams = renderer.GetStretchParams();
                        if (stretchParams.stretchScaleX > 0.0f && stretchParams.stretchScaleY > 0.0f) {
                            clickX = (clickX - stretchParams.marginX) / stretchParams.stretchScaleX;
                            clickY = (clickY - stretchParams.marginY) / stretchParams.stretchScaleY;
                        }
                    } else if (stretchMode == StretchMode::Fit) {
                        // 获取UI基准尺寸（背景纹理大小，如果没有背景则使用800x800）
                        VkExtent2D uiBaseSize = renderer.GetUIBaseSize();
                        float uiBaseWidth = (float)uiBaseSize.width;
                        float uiBaseHeight = (float)uiBaseSize.height;
                        
                        // 计算视口大小和偏移（与vulkan_renderer.cpp中的逻辑一致）
                        const float targetAspect = uiBaseWidth / uiBaseHeight;  // 使用实际的UI基准宽高比
                        const float currentAspect = currentScreenWidth / currentScreenHeight;
                        
                        float viewportWidth, viewportHeight;
                        float offsetX = 0.0f, offsetY = 0.0f;
                        
                        if (currentAspect > targetAspect) {
                            // 窗口更宽 - 添加左右黑边（pillarbox）
                            viewportHeight = currentScreenHeight;
                            viewportWidth = viewportHeight * targetAspect;
                            offsetX = (currentScreenWidth - viewportWidth) * 0.5f;
                        } else {
                            // 窗口更高或相等 - 添加上下黑边（letterbox）
                            viewportWidth = currentScreenWidth;
                            viewportHeight = viewportWidth / targetAspect;
                            offsetY = (currentScreenHeight - viewportHeight) * 0.5f;
                        }
                        
                        // 窗口坐标 -> 视口坐标（减去视口偏移）
                        float viewportX = clickX - offsetX;
                        float viewportY = clickY - offsetY;
                        
                        // 检查点击是否在视口内
                        if (viewportX >= 0.0f && viewportX <= viewportWidth &&
                            viewportY >= 0.0f && viewportY <= viewportHeight) {
                            // 视口坐标 -> UI坐标系坐标（使用实际的UI基准尺寸）
                            float scaleX = uiBaseWidth / viewportWidth;
                            float scaleY = uiBaseHeight / viewportHeight;
                            clickX = viewportX * scaleX;
                            clickY = viewportY * scaleY;
                        } else {
                            // 点击在视口外（黑边区域），忽略
                            clickX = -1.0f;
                            clickY = -1.0f;
                        }
                    }
                    
                    // 使用按钮组件的点击处理（自动调用回调函数）
                    bool clicked = false;
                    if (clickX >= 0.0f && clickY >= 0.0f) {
                        // 首先检查主按钮和颜色按钮（因为它们在高层级）
                        if (flowerButtonInitialized) {
                            clicked = flowerButton.HandleClick(clickX, clickY);
                        }
                        if (!clicked) {
                            // 检查9个颜色按钮
                            for (int i = 0; i < 9; i++) {
                                if (colorButtonsInitialized[i] && colorButtons[i].IsVisible()) {
                                    clicked = colorButtons[i].HandleClick(clickX, clickY);
                                    if (clicked) break;
                                }
                            }
                            // 先检查颜色控制器（滑块应该在按钮之前检查，因为滑块层级更高）
                            if (!clicked && colorControllerInitialized && colorController.IsVisible()) {
                                clicked = colorController.HandleMouseDown(clickX, clickY);
                                if (clicked) {
                                    printf("[DEBUG] Color controller clicked at (%.2f, %.2f)\n", clickX, clickY);
                                }
                            }
                            // 检查方块颜色控制器（滑块应该在按钮之前检查）
                            if (!clicked) {
                                for (int i = 0; i < 9; i++) {
                                    if (boxColorControllersInitialized[i] && boxColorControllers[i].IsVisible()) {
                                        clicked = boxColorControllers[i].HandleMouseDown(clickX, clickY);
                                        if (clicked) {
                                            printf("[DEBUG] Box color controller %d clicked at (%.2f, %.2f)\n", i, clickX, clickY);
                                            break;
                                        }
                                    }
                                }
                            }
                            // 检查蓝色按钮的3x3按钮矩阵（在滑块之后检查）
                            if (!clicked) {
                                for (int i = 0; i < 9; i++) {
                                    if (boxColorButtonsInitialized[i] && boxColorButtons[i].IsVisible()) {
                                        clicked = boxColorButtons[i].HandleClick(clickX, clickY);
                                        if (clicked) break;
                                    }
                                }
                            }
                        }
                        if (!clicked) {
                            // 检查颜色调整按钮
                            if (colorAdjustButtonInitialized) {
                                clicked = colorAdjustButton.HandleClick(clickX, clickY);
                            }
                            if (!clicked) {
                                // 检查进入按钮
                                clicked = enterButton.HandleClick(clickX, clickY);
                                if (!clicked) {
                                    // 尝试点击颜色按钮
                                    clicked = colorButton.HandleClick(clickX, clickY);
                                    if (!clicked) {
                                        // 尝试点击左侧按钮
                                        clicked = leftButton.HandleClick(clickX, clickY);
                                        if (!clicked && sliderInitialized) {
                                            // 尝试点击滑块
                                            clicked = orangeSlider.HandleMouseDown(clickX, clickY);
                                            if (clicked) {
                                                printf("[DEBUG] Slider clicked at (%.2f, %.2f)\n", clickX, clickY);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                        if (!clicked) {
                            printf("[DEBUG] Click position is outside button bounds\n");
                        }
                    }
                    printf("\n");
                }
            } else if (msg.message == WM_MOUSEMOVE) {
                // 处理鼠标移动（用于拖动滑块）
                if (appState == AppState::Loading) {
                    POINT pt;
                    pt.x = LOWORD(msg.lParam);
                    pt.y = HIWORD(msg.lParam);
                    
                    RECT clientRect;
                    GetClientRect(Window::GetHandle(), &clientRect);
                    float currentScreenWidth = (float)(clientRect.right - clientRect.left);
                    float currentScreenHeight = (float)(clientRect.bottom - clientRect.top);
                    
                    float mouseX = (float)pt.x;
                    float mouseY = (float)pt.y;
                    
                    // 坐标转换（与点击处理一致）
                    if (stretchMode == StretchMode::Scaled) {
                        StretchParams stretchParams = renderer.GetStretchParams();
                        if (stretchParams.stretchScaleX > 0.0f && stretchParams.stretchScaleY > 0.0f) {
                            mouseX = (mouseX - stretchParams.marginX) / stretchParams.stretchScaleX;
                            mouseY = (mouseY - stretchParams.marginY) / stretchParams.stretchScaleY;
                        }
                    } else if (stretchMode == StretchMode::Fit) {
                        VkExtent2D uiBaseSize = renderer.GetUIBaseSize();
                        float uiBaseWidth = (float)uiBaseSize.width;
                        float uiBaseHeight = (float)uiBaseSize.height;
                        const float targetAspect = uiBaseWidth / uiBaseHeight;
                        const float currentAspect = currentScreenWidth / currentScreenHeight;
                        
                        float viewportWidth, viewportHeight;
                        float offsetX = 0.0f, offsetY = 0.0f;
                        
                        if (currentAspect > targetAspect) {
                            viewportHeight = currentScreenHeight;
                            viewportWidth = viewportHeight * targetAspect;
                            offsetX = (currentScreenWidth - viewportWidth) * 0.5f;
                        } else {
                            viewportWidth = currentScreenWidth;
                            viewportHeight = viewportWidth / targetAspect;
                            offsetY = (currentScreenHeight - viewportHeight) * 0.5f;
                        }
                        
                        float viewportX = mouseX - offsetX;
                        float viewportY = mouseY - offsetY;
                        
                        if (viewportX >= 0.0f && viewportX <= viewportWidth &&
                            viewportY >= 0.0f && viewportY <= viewportHeight) {
                            float scaleX = uiBaseWidth / viewportWidth;
                            float scaleY = uiBaseHeight / viewportHeight;
                            mouseX = viewportX * scaleX;
                            mouseY = viewportY * scaleY;
                        } else {
                            mouseX = -1.0f;
                            mouseY = -1.0f;
                        }
                    }
                    
                    if (mouseX >= 0.0f && mouseY >= 0.0f) {
                        // 处理按钮悬停效果（按层级从高到低处理）
                        if (flowerButtonInitialized) {
                            flowerButton.HandleMouseMove(mouseX, mouseY);
                        }
                        for (int i = 0; i < 9; i++) {
                            if (colorButtonsInitialized[i] && colorButtons[i].IsVisible()) {
                                colorButtons[i].HandleMouseMove(mouseX, mouseY);
                            }
                        }
                        for (int i = 0; i < 9; i++) {
                            if (boxColorButtonsInitialized[i] && boxColorButtons[i].IsVisible()) {
                                boxColorButtons[i].HandleMouseMove(mouseX, mouseY);
                            }
                        }
                        if (colorAdjustButtonInitialized) {
                            colorAdjustButton.HandleMouseMove(mouseX, mouseY);
                        }
                        enterButton.HandleMouseMove(mouseX, mouseY);
                        colorButton.HandleMouseMove(mouseX, mouseY);
                        leftButton.HandleMouseMove(mouseX, mouseY);
                        
                        // 处理橙色滑块
                        if (sliderInitialized) {
                            orangeSlider.HandleMouseMove(mouseX, mouseY);
                        }
                        // 处理颜色控制器
                        if (colorControllerInitialized && colorController.IsVisible()) {
                            colorController.HandleMouseMove(mouseX, mouseY);
                        }
                        // 处理方块颜色控制器的鼠标移动
                        for (int i = 0; i < 9; i++) {
                            if (boxColorControllersInitialized[i] && boxColorControllers[i].IsVisible()) {
                                boxColorControllers[i].HandleMouseMove(mouseX, mouseY);
                            }
                        }
                    } else {
                        // 鼠标在视口外，清除所有按钮的悬停状态
                        if (flowerButtonInitialized) {
                            flowerButton.HandleMouseMove(-1.0f, -1.0f);
                        }
                        for (int i = 0; i < 9; i++) {
                            if (colorButtonsInitialized[i]) {
                                colorButtons[i].HandleMouseMove(-1.0f, -1.0f);
                            }
                            if (boxColorButtonsInitialized[i]) {
                                boxColorButtons[i].HandleMouseMove(-1.0f, -1.0f);
                            }
                        }
                        if (colorAdjustButtonInitialized) {
                            colorAdjustButton.HandleMouseMove(-1.0f, -1.0f);
                        }
                        enterButton.HandleMouseMove(-1.0f, -1.0f);
                        colorButton.HandleMouseMove(-1.0f, -1.0f);
                        leftButton.HandleMouseMove(-1.0f, -1.0f);
                    }
                }
            } else if (msg.message == WM_LBUTTONUP) {
                // 处理鼠标释放（结束拖动）
                if (appState == AppState::Loading) {
                    if (sliderInitialized) {
                        orangeSlider.HandleMouseUp();
                    }
                    // 处理颜色控制器
                    if (colorControllerInitialized) {
                        colorController.HandleMouseUp();
                    }
                    // 处理方块颜色控制器的鼠标释放
                    for (int i = 0; i < 9; i++) {
                        if (boxColorControllersInitialized[i]) {
                            boxColorControllers[i].HandleMouseUp();
                        }
                    }
                }
            }
            
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (Window::IsRunning()) {
            // 如果窗口最小化，跳过渲染以避免错误
            if (Window::IsMinimized()) {
                Sleep(100);  // 最小化时降低CPU使用率
                continue;
            }
            
            // 计算帧率
            QueryPerformanceCounter(&currentTime);
            double deltaTime = (double)(currentTime.QuadPart - lastTime.QuadPart) / (double)frequency.QuadPart;
            lastTime = currentTime;
            
            time += (float)deltaTime;
            fpsUpdateTimer += (float)deltaTime;
            fpsFrameCount++;
            
            // 每0.1秒更新一次帧率显示
            if (fpsUpdateTimer >= fpsUpdateInterval) {
                fps = (float)fpsFrameCount / fpsUpdateTimer;
                fpsFrameCount = 0;
                fpsUpdateTimer = 0.0f;
            }
            
            // 设置启动时间（第一次进入循环时）
            if (!startTimeSet) {
                startTime = time;
                startTimeSet = true;
            }
            
            if (appState == AppState::LoadingCubes) {
                // 渲染3D场景（loading_cubes shader）
                if (loadingCubesPipelineCreated) {
                    // 设置键盘输入
                    bool wPressed = Window::IsKeyPressed('W') || Window::IsKeyPressed(VK_UP);
                    bool aPressed = Window::IsKeyPressed('A') || Window::IsKeyPressed(VK_LEFT);
                    bool sPressed = Window::IsKeyPressed('S') || Window::IsKeyPressed(VK_DOWN);
                    bool dPressed = Window::IsKeyPressed('D') || Window::IsKeyPressed(VK_RIGHT);
                    renderer.SetKeyInput(wPressed, aPressed, sPressed, dPressed);
                    
                    // 更新相机状态（在CPU端累积更新）
                    renderer.UpdateCamera((float)deltaTime);
                    
                    // 渲染loading_cubes shader
                    renderer.DrawFrame(time, true, textRendererInitialized ? &textRenderer : nullptr, fps);  // 使用loading_cubes pipeline
                    
                    // 按ESC键返回Loading状态
                    if (Window::IsKeyPressed(VK_ESCAPE)) {
                        printf("[DEBUG] ESC pressed, returning to Loading state\n");
                        appState = AppState::Loading;
                    }
                } else {
                    // 如果pipeline还没创建，先渲染黑色背景
                    renderer.DrawFrame(time, false, textRendererInitialized ? &textRenderer : nullptr, fps);
                }
            } else if (appState == AppState::Loading) {
                // Ensure button position is up-to-date before rendering
                RECT currentRect;
                GetClientRect(Window::GetHandle(), &currentRect);
                float currentWidth = (float)(currentRect.right - currentRect.left);
                float currentHeight = (float)(currentRect.bottom - currentRect.top);
                
                // Recalculate positions if window size changed (safety check)
                // Disabled/FIT模式：UI完全固定，不响应窗口变化
                if (stretchMode == StretchMode::Scaled) {
                    // Scaled模式：更新UI位置
                float centerX = currentWidth / 2.0f - 36.0f;
                float picCenterY = currentHeight * 0.4f - 36.0f;
                loadingAnim.SetPosition(centerX, picCenterY);
                
                // Update button positions
                enterButton.UpdateForWindowResize(currentWidth, currentHeight);
                colorButton.UpdateForWindowResize(currentWidth, currentHeight);
                leftButton.UpdateForWindowResize(currentWidth, currentHeight);
                if (sliderInitialized) {
                    orangeSlider.UpdateForWindowResize(currentWidth, currentHeight);
                }
                // 更新主按钮和颜色按钮位置
                if (flowerButtonInitialized) {
                    flowerButton.UpdateForWindowResize(currentWidth, currentHeight);
                }
                for (int i = 0; i < 9; i++) {
                    if (colorButtonsInitialized[i]) {
                        colorButtons[i].UpdateForWindowResize(currentWidth, currentHeight);
                    }
                    // 更新蓝色按钮的3x3按钮矩阵位置
                    if (boxColorButtonsInitialized[i]) {
                        boxColorButtons[i].UpdateForWindowResize(currentWidth, currentHeight);
                    }
                }
                // 更新颜色调整按钮位置
                if (colorAdjustButtonInitialized) {
                    colorAdjustButton.UpdateForWindowResize(currentWidth, currentHeight);
                }
                // 更新颜色控制器位置
                if (colorControllerInitialized) {
                    colorController.UpdateScreenSize(currentWidth, currentHeight);
                }
                // 更新方块颜色控制器位置
                for (int i = 0; i < 9; i++) {
                    if (boxColorControllersInitialized[i]) {
                        boxColorControllers[i].UpdateScreenSize(currentWidth, currentHeight);
                    }
                }
                }
                // Disabled/FIT模式时，加载动画和按钮位置已经基于固定的800x800坐标系初始化，无需更新
                
                // Debug output: print button render position every 60 frames (about once per second)
                static int frameCount = 0;
                if (frameCount % 60 == 0) {
                    printf("[DEBUG] Render button position: (%.2f, %.2f), size: (%.2f, %.2f), screen: (%.0f, %.0f)\n", 
                           enterButton.GetX(), enterButton.GetY(), enterButton.GetWidth(), enterButton.GetHeight(), 
                           currentWidth, currentHeight);
                }
                frameCount++;
                
                // 创建滑块向量用于传递给渲染函数
                std::vector<Slider*> allSliders;
                if (sliderInitialized) {
                    allSliders.push_back(&orangeSlider);
                }
                // 添加颜色控制器的滑块
                if (colorControllerInitialized) {
                    std::vector<Slider*> colorControllerSliders = colorController.GetSliders();
                    for (Slider* slider : colorControllerSliders) {
                        allSliders.push_back(slider);
                    }
                }
                // 添加方块颜色控制器的滑块
                for (int i = 0; i < 9; i++) {
                    if (boxColorControllersInitialized[i] && boxColorControllers[i].IsVisible()) {
                        std::vector<Slider*> boxControllerSliders = boxColorControllers[i].GetSliders();
                        for (Slider* slider : boxControllerSliders) {
                            allSliders.push_back(slider);
                        }
                    }
                }
                
                // 更新并渲染加载动画
                loadingAnim.Update(time);
                renderer.DrawFrameWithLoading(time, &loadingAnim, 
                                              &enterButton,
                                              textRendererInitialized ? &textRenderer : nullptr,
                                              &colorButton,
                                              &leftButton,
                                              &allButtons, // 传递所有额外按钮（包括主按钮和颜色按钮）
                                              sliderInitialized ? &orangeSlider : nullptr, // 传递主滑块
                                              &allSliders, // 传递所有额外滑块（包括颜色滑块）
                                              fps); // 传递帧率
            } else {
                // 渲染shader
                renderer.DrawFrame(time, false, textRendererInitialized ? &textRenderer : nullptr, fps);
            }
            
            // 控制帧率
            Sleep(1);
        }
    }
    
    // 清理资源
    if (textRendererInitialized) {
        textRenderer.Cleanup();
    }
    if (sliderInitialized) {
        orangeSlider.Cleanup();
    }
    // 清理颜色控制器
    if (colorControllerInitialized) {
        colorController.Cleanup();
    }
    // 清理方块颜色控制器
    for (int i = 0; i < 9; i++) {
        if (boxColorControllersInitialized[i]) {
            boxColorControllers[i].Cleanup();
        }
    }
    enterButton.Cleanup();
    colorButton.Cleanup();
    leftButton.Cleanup();
    // 清理主按钮和颜色按钮
    if (flowerButtonInitialized) {
        flowerButton.Cleanup();
    }
    for (int i = 0; i < 9; i++) {
        if (colorButtonsInitialized[i]) {
            colorButtons[i].Cleanup();
        }
        // 清理蓝色按钮的3x3按钮矩阵
        if (boxColorButtonsInitialized[i]) {
            boxColorButtons[i].Cleanup();
        }
    }
    // 清理颜色调整按钮
    if (colorAdjustButtonInitialized) {
        colorAdjustButton.Cleanup();
    }
    loadingAnim.Cleanup();
    renderer.Cleanup();
    Window::Destroy();
    
    // Program exiting (debug output removed)
    FreeConsole();
    
    return 0;
}

