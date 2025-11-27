#pragma once

#include <memory>  // 2. 系统头文件
#include <vector>  // 2. 系统头文件

#include "core/config/constants.h"  // 4. 项目头文件（配置）
#include "core/interfaces/iwindow_resize_handler.h"  // 4. 项目头文件（接口）
#include "ui/slider/slider.h"  // 4. 项目头文件（UI组件）
// 注意：必须包含完整定义，因为 std::unique_ptr<Slider> 作为成员变量需要完整类型（析构函数需要知道如何删除）

// 前向声明（ColorController 仅作为参数类型，不需要完整定义）
class ColorController;

// 前向声明
class IRenderer;
class IWindow;
class IRenderContext;

/**
 * 滑块UI管理器 - 负责管理所有滑块组件
 * 
 * 职责：管理所有滑块组件的创建、初始化和生命周期
 * 设计：实现 IWindowResizeHandler 接口，支持窗口大小变化时更新滑块位置
 * 
 * 使用方式：
 * 1. 通过依赖注入传入所有依赖（IRenderContext、IWindow）
 * 2. 调用 Initialize() 初始化所有滑块组件
 * 3. 使用 Get 方法获取滑块组件
 * 4. 调用 Cleanup() 清理所有资源
 */
class SliderUIManager : public IWindowResizeHandler {
public:
    SliderUIManager();
    ~SliderUIManager();
    
    /**
     * 初始化滑块组件
     * 
     * 通过依赖注入传入所有依赖，创建和初始化所有滑块组件
     * 
     * @param renderContext 渲染上下文（不拥有所有权，用于创建滑块）
     * @param window 窗口（不拥有所有权，用于获取窗口信息）
     * @param stretchMode 拉伸模式
     * @return true 如果初始化成功，false 如果失败
     */
    bool Initialize(const IRenderContext& renderContext, IWindow* window, StretchMode stretchMode);
    
    /**
     * 清理资源
     * 
     * 清理所有滑块组件和资源
     */
    void Cleanup();
    
    /**
     * IWindowResizeHandler 接口实现
     * 
     * 处理窗口大小变化，更新所有滑块的位置
     * 
     * @param stretchMode 拉伸模式
     * @param renderer 渲染器（不拥有所有权，用于获取拉伸参数）
     */
    void HandleWindowResize(StretchMode stretchMode, IRenderer* renderer) override;
    
    /**
     * 获取滑块组件
     * 
     * 所有权：[BORROW] 返回的指针不拥有所有权，由 SliderUIManager 管理生命周期
     */
    Slider* GetOrangeSlider() const { return m_orangeSlider.get(); }
    
    /**
     * 获取所有滑块（包括颜色控制器中的滑块）
     * 
     * 收集所有滑块指针到向量中，包括颜色控制器中的滑块
     * 
     * @param sliders 输出参数，存储所有滑块指针
     * @param colorController 颜色控制器（可选，用于获取其中的滑块）
     * @param boxColorControllers 盒子颜色控制器组（可选，用于获取其中的滑块）
     */
    void GetAllSliders(std::vector<Slider*>& sliders, 
                      ColorController* colorController = nullptr,
                      const std::vector<std::unique_ptr<ColorController>>* boxColorControllers = nullptr) const;
    
private:
    bool InitializeOrangeSlider(IRenderContext& renderContext, StretchMode stretchMode);
    
    // 滑块组件（拥有所有权）
    std::unique_ptr<Slider> m_orangeSlider;  // 橙色滑块
    
    // 初始化状态标志
    bool m_sliderInitialized = false;  // 滑块是否已初始化
    
    // 依赖对象（不拥有所有权，由外部管理生命周期）
    IWindow* m_window = nullptr;  // 窗口
};

