#include "core/managers/initialization_phase.h"  // 1. 对应头文件

#include <stdio.h>

InitializationPhaseManager::InitializationPhaseManager() {
}

InitializationPhaseManager::~InitializationPhaseManager() {
    CleanupAll();
}

void InitializationPhaseManager::AddPhase(std::unique_ptr<IInitializationPhase> phase) {
    if (phase) {
        m_phases.push_back(std::move(phase));
    }
}

bool InitializationPhaseManager::InitializeAll() {
    m_initializedCount = 0;
    
    for (size_t i = 0; i < m_phases.size(); ++i) {
        auto& phase = m_phases[i];
        
        printf("[INFO] Initializing phase: %s\n", phase->GetName().c_str());
        
        PhaseResult result = phase->Initialize();
        if (!result.success) {
            printf("[ERROR] Phase '%s' failed: %s\n", phase->GetName().c_str(), result.errorMessage.c_str());
            
            // 自动回滚已初始化的阶段
            CleanupAll();
            return false;
        }
        
        m_initializedCount++;
        printf("[INFO] Phase '%s' initialized successfully\n", phase->GetName().c_str());
    }
    
    return true;
}

void InitializationPhaseManager::CleanupAll() {
    // 按逆序清理已初始化的阶段
    for (size_t i = m_initializedCount; i > 0; --i) {
        size_t idx = i - 1;
        if (idx < m_phases.size() && m_phases[idx]->IsInitialized()) {
            printf("[INFO] Cleaning up phase: %s\n", m_phases[idx]->GetName().c_str());
            m_phases[idx]->Cleanup();
        }
    }
    
    m_initializedCount = 0;
}

