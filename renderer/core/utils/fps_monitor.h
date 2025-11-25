#pragma once

#include <windows.h>

// FPS监控器 - 独立管理帧率计算
class FPSMonitor {
public:
    FPSMonitor();
    ~FPSMonitor();
    
    // 初始化
    void Initialize();
    
    // 更新帧率（每帧调用）
    void Update();
    
    // 获取当前FPS
    float GetFPS() const { return m_fps; }
    
    // 获取上一帧的deltaTime
    float GetDeltaTime() const { return m_deltaTime; }
    
    // 获取总时间
    float GetTotalTime() const { return m_totalTime; }
    
    // 重置
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

