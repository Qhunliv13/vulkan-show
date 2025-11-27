#include "core/utils/fps_monitor.h"

#include <windows.h>

FPSMonitor::FPSMonitor() {
}

FPSMonitor::~FPSMonitor() {
}

void FPSMonitor::Initialize() {
    if (m_initialized) {
        return;
    }
    
    QueryPerformanceFrequency(&m_frequency);
    QueryPerformanceCounter(&m_lastTime);
    
    m_fps = 0.0f;
    m_deltaTime = 0.0f;
    m_totalTime = 0.0f;
    m_fpsUpdateTimer = 0.0f;
    m_fpsFrameCount = 0;
    
    m_initialized = true;
}

void FPSMonitor::Update() {
    if (!m_initialized) {
        Initialize();
    }
    
    QueryPerformanceCounter(&m_currentTime);
    double deltaTime = (double)(m_currentTime.QuadPart - m_lastTime.QuadPart) / (double)m_frequency.QuadPart;
    m_lastTime = m_currentTime;
    
    m_deltaTime = (float)deltaTime;
    m_totalTime += m_deltaTime;
    
    m_fpsUpdateTimer += m_deltaTime;
    m_fpsFrameCount++;
    
    // 每0.1秒更新一次帧率显示
    if (m_fpsUpdateTimer >= m_fpsUpdateInterval) {
        m_fps = (float)m_fpsFrameCount / m_fpsUpdateTimer;
        m_fpsFrameCount = 0;
        m_fpsUpdateTimer = 0.0f;
    }
}

void FPSMonitor::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    m_fps = 0.0f;
    m_deltaTime = 0.0f;
    m_totalTime = 0.0f;
    m_fpsUpdateTimer = 0.0f;
    m_fpsFrameCount = 0;
    m_initialized = false;
}

void FPSMonitor::Reset() {
    QueryPerformanceCounter(&m_lastTime);
    m_fps = 0.0f;
    m_deltaTime = 0.0f;
    m_totalTime = 0.0f;
    m_fpsUpdateTimer = 0.0f;
    m_fpsFrameCount = 0;
}

