#pragma once

#include <windows.h>      // 2. 系统头文件

/**
 * FPS监控器 - 独立管理帧率计算和性能统计
 * 
 * 职责：提供帧率监控和时间统计功能，支持固定时间步和可变渲染插值
 * 设计：使用Windows高精度计时器，独立于渲染系统，支持依赖注入
 */
class FPSMonitor {
public:
    FPSMonitor();
    ~FPSMonitor();
    
    /**
     * 初始化FPS监控器
     * 初始化高精度计时器和内部状态
     */
    void Initialize();
    
    /**
     * 清理资源
     * 重置所有状态，准备销毁
     */
    void Cleanup();
    
    /**
     * 更新帧率（每帧调用）
     * 计算deltaTime和FPS，更新内部统计
     */
    void Update();
    
    /**
     * 获取当前FPS
     * 返回值：当前帧率（每秒帧数）
     */
    float GetFPS() const { return m_fps; }
    
    /**
     * 获取上一帧的deltaTime
     * 返回值：上一帧的时间间隔（秒）
     */
    float GetDeltaTime() const { return m_deltaTime; }
    
    /**
     * 获取总时间
     * 返回值：从初始化开始的总运行时间（秒）
     */
    float GetTotalTime() const { return m_totalTime; }
    
    /**
     * 重置统计信息
     * 重置FPS、deltaTime和总时间，但保持初始化状态
     */
    void Reset();

private:
    LARGE_INTEGER m_frequency;
    LARGE_INTEGER m_lastTime;
    LARGE_INTEGER m_currentTime;
    
    float m_fps = 0.0f;
    float m_deltaTime = 0.0f;
    float m_totalTime = 0.0f;
    
    float m_fpsUpdateInterval = 0.1f;  // 每0.1秒更新一次FPS
    float m_fpsUpdateTimer = 0.0f;
    int m_fpsFrameCount = 0;
    
    bool m_initialized = false;
};

