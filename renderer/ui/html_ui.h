#pragma once

#include <windows.h>    // 2. 系统头文件
#include <string>       // 2. 系统头文件
#include <memory>       // 2. 系统头文件
#include <functional>   // 2. 系统头文件

// HTML UI管理器 - 使用WebView2或备用浏览器控件显示HTML内容
// 支持加载HTML文件和字符串，自动加载对应的CSS文件，提供JavaScript回调接口
class HtmlUI {
public:
    HtmlUI();
    ~HtmlUI();
    
    // 初始化HTML UI系统
    bool Initialize(HWND parentHwnd);
    
    // 加载HTML文件（自动加载同名的CSS文件）
    bool LoadHtmlFile(const std::string& filePath);
    
    // 加载HTML文件并指定CSS文件
    bool LoadHtmlFileWithCss(const std::string& htmlPath, const std::string& cssPath);
    
    // 加载HTML字符串
    bool LoadHtmlString(const std::string& htmlContent);
    
    // 加载HTML字符串并指定CSS内容
    bool LoadHtmlStringWithCss(const std::string& htmlContent, const std::string& cssContent);
    
    // 设置UI可见性
    void SetVisible(bool visible);
    
    // 调整UI大小和位置
    void Resize(int x, int y, int width, int height);
    
    // 清理资源
    void Cleanup();
    
    // 获取WebView2控件句柄
    HWND GetWebViewHandle() const { return m_webViewHwnd; }
    
    // 检查WebView2是否可用
    static bool IsWebView2Available();
    
    // 设置JavaScript回调函数（当HTML调用window.external.EnterMain()时触发）
    void SetEnterMainCallback(std::function<void()> callback);
    
    // 处理导航事件（在窗口消息循环中调用）
    bool HandleNavigation(const std::string& url);
    
private:
    HWND m_parentHwnd;
    HWND m_webViewHwnd;
    bool m_initialized;
    void* m_pWebBrowser; // IWebBrowser2* 指针（使用void*避免头文件包含COM接口）
    std::function<void()> m_enterMainCallback; // 进入主界面的回调函数
    
    // 创建WebView2控件（如果可用）
    bool CreateWebView2();
    
    // 创建备用HTML显示控件（使用简单的浏览器控件）
    bool CreateFallbackBrowser();
    
    // 加载HTML到控件
    bool NavigateToHtml(const std::string& htmlContent);
    
    // 构建完整的HTML文档（包含CSS）
    std::string BuildCompleteHtml(const std::string& htmlBody, const std::string& cssContent = "");
    
    // 从文件路径获取对应的CSS文件路径
    std::string GetCssPathFromHtmlPath(const std::string& htmlPath);
};

