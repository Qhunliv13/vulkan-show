#include "ui/html_ui.h"  // 1. 对应头文件

#include <windows.h>     // 2. 系统头文件
#include <shlwapi.h>     // 2. 系统头文件
#include <shlobj.h>      // 2. 系统头文件
#include <comdef.h>      // 2. 系统头文件
#include <exdisp.h>      // 2. 系统头文件
#include <mshtml.h>      // 2. 系统头文件
#include <stdlib.h>      // 2. 系统头文件
#include <stdio.h>       // 2. 系统头文件
#include <fstream>       // 2. 系统头文件
#include <sstream>       // 2. 系统头文件

#include "window/window.h"  // 3. 项目头文件

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

HtmlUI::HtmlUI() 
    : m_parentHwnd(nullptr)
    , m_webViewHwnd(nullptr)
    , m_initialized(false)
    , m_pWebBrowser(nullptr)
    , m_enterMainCallback(nullptr)
{
}

HtmlUI::~HtmlUI() {
    Cleanup();
}

bool HtmlUI::Initialize(HWND parentHwnd) {
    if (m_initialized) {
        return true;
    }
    
    m_parentHwnd = parentHwnd;
    
    // 尝试创建WebView2，如果失败则使用备用方案
    if (!CreateWebView2()) {
        // 使用简单的浏览器控件作为备用
        if (!CreateFallbackBrowser()) {
            return false;
        }
    }
    
    m_initialized = true;
    return true;
}

bool HtmlUI::CreateWebView2() {
    // WebView2需要额外的运行时，这里先尝试使用备用方案
    // 如果需要完整的WebView2支持，需要添加WebView2Loader.dll和相关依赖
    return false;
}

bool HtmlUI::CreateFallbackBrowser() {
    // 使用Windows的WebBrowser控件（基于IE，但简单可用）
    // 注意：IE已过时，但作为简单HTML显示仍然可用
    
    // 初始化COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        return false;
    }
    
    // 获取父窗口的客户区大小
    RECT clientRect;
    GetClientRect(m_parentHwnd, &clientRect);
    
    // 创建WebBrowser控件的容器窗口
    // 移除WS_EX_CLIENTEDGE以避免灰边
    m_webViewHwnd = CreateWindowEx(
        0,
        "STATIC",
        "",
        WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        0, 0, 
        clientRect.right - clientRect.left,
        clientRect.bottom - clientRect.top,
        m_parentHwnd,
        (HMENU)1001,
        GetModuleHandle(NULL),
        NULL
    );
    
    if (m_webViewHwnd == NULL) {
        CoUninitialize();
        return false;
    }
    
    // 创建WebBrowser ActiveX控件
    IWebBrowser2* pWebBrowser = NULL;
    hr = CoCreateInstance(CLSID_WebBrowser, NULL, CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER,
                         IID_IWebBrowser2, (void**)&pWebBrowser);
    
    if (SUCCEEDED(hr) && pWebBrowser) {
        // 先设置基本属性
        pWebBrowser->put_Left(0);
        pWebBrowser->put_Top(0);
        pWebBrowser->put_Width(clientRect.right - clientRect.left);
        pWebBrowser->put_Height(clientRect.bottom - clientRect.top);
        
        // 隐藏滚动条和工具栏
        pWebBrowser->put_Silent(VARIANT_TRUE);
        pWebBrowser->put_Visible(VARIANT_TRUE);
        
        VARIANT_BOOL vbFalse = VARIANT_FALSE;
        pWebBrowser->put_MenuBar(vbFalse);
        pWebBrowser->put_ToolBar(0);
        pWebBrowser->put_StatusBar(vbFalse);
        pWebBrowser->put_AddressBar(vbFalse);
        
        // 使用IOleObject接口来正确嵌入控件
        IOleObject* pObject = NULL;
        hr = pWebBrowser->QueryInterface(IID_IOleObject, (void**)&pObject);
        if (SUCCEEDED(hr) && pObject) {
            // 设置客户端站点（使用NULL，但需要先设置）
            pObject->SetClientSite(NULL);
            pObject->SetHostNames(L"Shader App", NULL);
            
            // 获取容器窗口的RECT
            RECT rect;
            GetClientRect(m_webViewHwnd, &rect);
            
            // 先设置位置
            IOleInPlaceObject* pInPlace = NULL;
            if (SUCCEEDED(pObject->QueryInterface(IID_IOleInPlaceObject, (void**)&pInPlace))) {
                RECT containerRect;
                GetClientRect(m_webViewHwnd, &containerRect);
                pInPlace->SetObjectRects(&containerRect, &containerRect);
                pInPlace->Release();
            }
            
            // 激活控件（使用容器窗口作为父窗口）
            hr = pObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, NULL, 0, m_webViewHwnd, &rect);
            
            pObject->Release();
        }
        
        // 获取WebBrowser的窗口句柄并设置父窗口
        // 注意：需要在激活后才能获取窗口句柄
        IOleWindow* pOleWindow = NULL;
        HWND hwndBrowser = NULL;
        if (SUCCEEDED(pWebBrowser->QueryInterface(IID_IOleWindow, (void**)&pOleWindow))) {
            pOleWindow->GetWindow(&hwndBrowser);
            pOleWindow->Release();
            
            if (hwndBrowser && hwndBrowser != m_webViewHwnd) {
                // 设置父窗口为容器窗口
                SetParent(hwndBrowser, m_webViewHwnd);
                // 设置窗口位置和大小
                SetWindowPos(hwndBrowser, HWND_TOP, 0, 0, 
                            clientRect.right - clientRect.left,
                            clientRect.bottom - clientRect.top,
                            SWP_SHOWWINDOW);
                ShowWindow(hwndBrowser, SW_SHOW);
                UpdateWindow(hwndBrowser);
                InvalidateRect(hwndBrowser, NULL, TRUE);
            }
        }
        
        // 强制刷新
        InvalidateRect(m_webViewHwnd, NULL, TRUE);
        UpdateWindow(m_webViewHwnd);
        
        // 保存接口指针
        m_pWebBrowser = pWebBrowser;
        return true;
    }
    
    return false;
}

bool HtmlUI::LoadHtmlFile(const std::string& filePath) {
    if (!m_initialized) {
        return false;
    }
    
    // 转换为绝对路径
    char absPath[MAX_PATH];
    if (!_fullpath(absPath, filePath.c_str(), MAX_PATH)) {
        // 如果_fullpath失败，尝试使用当前工作目录
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        sprintf_s(absPath, MAX_PATH, "%s\\%s", currentDir, filePath.c_str());
    }
    
    // 自动查找对应的CSS文件
    std::string cssPath = GetCssPathFromHtmlPath(absPath);
    
    // 读取HTML文件
    std::ifstream htmlFile(absPath);
    if (!htmlFile.is_open()) {
        // 尝试相对路径
        htmlFile.open(filePath);
        if (!htmlFile.is_open()) {
            return false;
        }
    }
    
    std::stringstream htmlBuffer;
    htmlBuffer << htmlFile.rdbuf();
    std::string htmlContent = htmlBuffer.str();
    htmlFile.close();
    
    // 读取CSS文件（如果存在）
    std::string cssContent = "";
    if (!cssPath.empty()) {
        std::ifstream cssFile(cssPath);
        if (cssFile.is_open()) {
            std::stringstream cssBuffer;
            cssBuffer << cssFile.rdbuf();
            cssContent = cssBuffer.str();
            cssFile.close();
        }
    }
    
    return LoadHtmlStringWithCss(htmlContent, cssContent);
}

bool HtmlUI::LoadHtmlFileWithCss(const std::string& htmlPath, const std::string& cssPath) {
    if (!m_initialized) {
        return false;
    }
    
    // 读取HTML文件
    std::ifstream htmlFile(htmlPath);
    if (!htmlFile.is_open()) {
        return false;
    }
    
    std::stringstream htmlBuffer;
    htmlBuffer << htmlFile.rdbuf();
    std::string htmlContent = htmlBuffer.str();
    htmlFile.close();
    
    // 读取CSS文件
    std::string cssContent = "";
    if (!cssPath.empty()) {
        std::ifstream cssFile(cssPath);
        if (cssFile.is_open()) {
            std::stringstream cssBuffer;
            cssBuffer << cssFile.rdbuf();
            cssContent = cssBuffer.str();
            cssFile.close();
        }
    }
    
    return LoadHtmlStringWithCss(htmlContent, cssContent);
}

bool HtmlUI::LoadHtmlString(const std::string& htmlContent) {
    return LoadHtmlStringWithCss(htmlContent, "");
}

bool HtmlUI::LoadHtmlStringWithCss(const std::string& htmlContent, const std::string& cssContent) {
    if (!m_initialized) {
        return false;
    }
    
    // 构建完整的HTML文档
    std::string completeHtml = BuildCompleteHtml(htmlContent, cssContent);
    
    return NavigateToHtml(completeHtml);
}

bool HtmlUI::NavigateToHtml(const std::string& htmlContent) {
    if (!m_webViewHwnd || !m_pWebBrowser) {
        return false;
    }
    
    // 使用保存的WebBrowser接口
    IWebBrowser2* pWebBrowser = (IWebBrowser2*)m_pWebBrowser;
    
    // 先导航到about:blank
    VARIANT vEmpty;
    VariantInit(&vEmpty);
    BSTR bstrBlank = SysAllocString(L"about:blank");
    HRESULT hr = pWebBrowser->Navigate(bstrBlank, &vEmpty, &vEmpty, &vEmpty, &vEmpty);
    SysFreeString(bstrBlank);
    
    // 确保WebBrowser可见
    pWebBrowser->put_Visible(VARIANT_TRUE);
    
    // 等待导航完成，然后直接写入HTML内容
    if (SUCCEEDED(hr)) {
        READYSTATE readyState;
        int attempts = 0;
        while (attempts < 100) { // 最多等待10秒
            pWebBrowser->get_ReadyState(&readyState);
            if (readyState == READYSTATE_COMPLETE || readyState == READYSTATE_INTERACTIVE) {
                // 获取文档接口并直接写入HTML
                IDispatch* pDispatch = NULL;
                if (SUCCEEDED(pWebBrowser->get_Document(&pDispatch)) && pDispatch) {
                    IHTMLDocument2* pDoc = NULL;
                    if (SUCCEEDED(pDispatch->QueryInterface(IID_IHTMLDocument2, (void**)&pDoc)) && pDoc) {
                        // 将HTML内容转换为宽字符
                        int wlen = MultiByteToWideChar(CP_UTF8, 0, htmlContent.c_str(), -1, NULL, 0);
                        if (wlen > 0) {
                            BSTR bstrHtml = SysAllocStringLen(NULL, wlen);
                            if (bstrHtml) {
                                MultiByteToWideChar(CP_UTF8, 0, htmlContent.c_str(), -1, (LPWSTR)bstrHtml, wlen);
                                
                                // 写入HTML内容
                                SAFEARRAY* psa = SafeArrayCreateVector(VT_VARIANT, 0, 1);
                                if (psa) {
                                    VARIANT* pVar = NULL;
                                    SafeArrayAccessData(psa, (LPVOID*)&pVar);
                                    pVar->vt = VT_BSTR;
                                    pVar->bstrVal = bstrHtml;
                                    SafeArrayUnaccessData(psa);
                                    
                                    pDoc->write(psa);
                                    pDoc->close();
                                    SafeArrayDestroy(psa);
                                }
                                SysFreeString(bstrHtml);
                            }
                        }
                        pDoc->Release();
                    }
                    pDispatch->Release();
                }
                
                // 等待一小段时间确保内容完全渲染
                Sleep(300);
                
                // 刷新显示
                pWebBrowser->Refresh();
                
                // 获取WebBrowser窗口并刷新
                IOleWindow* pOleWindow = NULL;
                HWND hwndBrowser = NULL;
                if (SUCCEEDED(pWebBrowser->QueryInterface(IID_IOleWindow, (void**)&pOleWindow))) {
                    pOleWindow->GetWindow(&hwndBrowser);
                    pOleWindow->Release();
                    if (hwndBrowser) {
                        // 确保浏览器窗口可见且在最上层
                        ShowWindow(hwndBrowser, SW_SHOW);
                        SetWindowPos(hwndBrowser, HWND_TOP, 0, 0, 0, 0, 
                                    SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
                        InvalidateRect(hwndBrowser, NULL, TRUE);
                        UpdateWindow(hwndBrowser);
                    }
                }
                
                // 刷新容器窗口
                InvalidateRect(m_webViewHwnd, NULL, TRUE);
                UpdateWindow(m_webViewHwnd);
                break;
            }
            Sleep(100);
            attempts++;
        }
    }
    
    return SUCCEEDED(hr);
}

void HtmlUI::SetVisible(bool visible) {
    if (m_webViewHwnd) {
        ShowWindow(m_webViewHwnd, visible ? SW_SHOW : SW_HIDE);
        
        // 确保在最上层
        if (visible) {
            SetWindowPos(m_webViewHwnd, HWND_TOP, 0, 0, 0, 0, 
                        SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        }
    }
    
    // 同时设置WebBrowser控件的可见性
    if (m_pWebBrowser) {
        IWebBrowser2* pWebBrowser = (IWebBrowser2*)m_pWebBrowser;
        pWebBrowser->put_Visible(visible ? VARIANT_TRUE : VARIANT_FALSE);
    }
}

void HtmlUI::Resize(int x, int y, int width, int height) {
    if (m_webViewHwnd) {
        SetWindowPos(m_webViewHwnd, NULL, x, y, width, height, SWP_NOZORDER);
    }
    
    // 同时更新WebBrowser控件的大小
    if (m_pWebBrowser) {
        IWebBrowser2* pWebBrowser = (IWebBrowser2*)m_pWebBrowser;
        pWebBrowser->put_Left(x);
        pWebBrowser->put_Top(y);
        pWebBrowser->put_Width(width);
        pWebBrowser->put_Height(height);
    }
}

void HtmlUI::Cleanup() {
    if (m_pWebBrowser) {
        IWebBrowser2* pWebBrowser = (IWebBrowser2*)m_pWebBrowser;
        pWebBrowser->Release();
        m_pWebBrowser = nullptr;
    }
    
    if (m_webViewHwnd) {
        DestroyWindow(m_webViewHwnd);
        m_webViewHwnd = nullptr;
    }
    
    CoUninitialize();
    m_initialized = false;
}

bool HtmlUI::IsWebView2Available() {
    // 检查WebView2运行时是否可用
    // 这里简化实现，返回false表示使用备用方案
    return false;
}

std::string HtmlUI::BuildCompleteHtml(const std::string& htmlBody, const std::string& cssContent) {
    std::stringstream html;
    
    html << "<!DOCTYPE html>\n";
    html << "<html>\n";
    html << "<head>\n";
    html << "<meta charset=\"UTF-8\">\n";
    html << "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n";
    html << "<style>\n";
    
    // 添加默认样式（使用醒目的红色背景确保可见）
    html << "html, body {\n";
    html << "  margin: 0 !important;\n";
    html << "  padding: 0 !important;\n";
    html << "  width: 100% !important;\n";
    html << "  height: 100% !important;\n";
    html << "  overflow: hidden !important;\n";
    html << "  background-color: #FF0000 !important;\n"; // 大红色背景，非常醒目
    html << "  background: #FF0000 !important;\n"; // 同时设置background属性
    html << "  display: flex !important;\n";
    html << "  justify-content: center !important;\n";
    html << "  align-items: center !important;\n";
    html << "  font-family: Arial, sans-serif !important;\n";
    html << "}\n";
    html << "* {\n";
    html << "  box-sizing: border-box;\n";
    html << "}\n";
    
    // 添加醒目的测试标识
    html << ".test-indicator {\n";
    html << "  position: fixed !important;\n";
    html << "  top: 50% !important;\n";
    html << "  left: 50% !important;\n";
    html << "  transform: translate(-50%, -50%) !important;\n";
    html << "  background-color: #FFFF00 !important;\n"; // 黄色背景
    html << "  color: #000000 !important;\n"; // 黑色文字
    html << "  padding: 20px 40px !important;\n";
    html << "  font-size: 48px !important;\n";
    html << "  font-weight: bold !important;\n";
    html << "  border: 5px solid #000000 !important;\n";
    html << "  z-index: 9999 !important;\n";
    html << "  text-align: center !important;\n";
    html << "}\n";
    
    // 添加用户提供的CSS
    if (!cssContent.empty()) {
        html << cssContent << "\n";
    }
    
    // 在最后再次确保body背景色（覆盖CSS中的白色背景）
    html << "body {\n";
    html << "  background-color: #FF0000 !important;\n"; // 大红色
    html << "  background: #FF0000 !important;\n";
    html << "}\n";
    
    html << "</style>\n";
    html << "<script>\n";
    html << "// 创建window.external对象\n";
    html << "window.external = {\n";
    html << "  EnterMain: function() {\n";
    html << "    // 使用自定义URL协议触发回调\n";
    html << "    window.location.href = 'app://entermain';\n";
    html << "  }\n";
    html << "};\n";
    html << "</script>\n";
    html << "</head>\n";
    html << "<body>\n";
    // 添加醒目的测试标识
    html << "<div class=\"test-indicator\">HTML UI 已加载！</div>\n";
    html << htmlBody << "\n";
    html << "</body>\n";
    html << "</html>\n";
    
    return html.str();
}

std::string HtmlUI::GetCssPathFromHtmlPath(const std::string& htmlPath) {
    // 查找同名的CSS文件
    // 例如: HTML/loading.html -> HTML/loading.css
    
    std::string cssPath = htmlPath;
    
    // 查找最后一个点（文件扩展名）
    size_t lastDot = cssPath.find_last_of('.');
    if (lastDot != std::string::npos) {
        // 替换扩展名为.css
        cssPath = cssPath.substr(0, lastDot) + ".css";
        
        // 检查文件是否存在
        std::ifstream testFile(cssPath);
        if (testFile.good()) {
            testFile.close();
            return cssPath;
        }
    }
    
    // 如果找不到同名CSS，尝试查找同目录下的.css文件
    // 这里简化实现，返回空字符串表示未找到
    return "";
}

void HtmlUI::SetEnterMainCallback(std::function<void()> callback) {
    m_enterMainCallback = callback;
}

// 在窗口消息循环中调用此方法来处理URL导航
bool HtmlUI::HandleNavigation(const std::string& url) {
    if (url.find("app://entermain") != std::string::npos) {
        if (m_enterMainCallback) {
            m_enterMainCallback();
        }
        // 发送消息到主窗口
        if (m_parentHwnd) {
            PostMessage(m_parentHwnd, WM_HTML_ENTER_MAIN, 0, 0);
        }
        return true; // 阻止导航
    }
    return false; // 允许导航
}

