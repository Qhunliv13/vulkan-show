#include "core/managers/initialization_chain.h"
#include <algorithm>
#include <queue>

InitializationChain::InitializationChain() {
}

InitializationChain::~InitializationChain() {
    Cleanup();
}

void InitializationChain::AddStep(std::unique_ptr<IInitializationStep> step) {
    if (!step) {
        return;
    }
    
    const char* name = step->GetName();
    int index = static_cast<int>(m_steps.size());
    m_steps.push_back(std::move(step));
    m_stepNameToIndex[name] = index;
}

InitializationResult InitializationChain::Execute() {
    // 拓扑排序确定初始化顺序
    std::vector<int> order = TopologicalSort();
    if (order.empty() && !m_steps.empty()) {
        return InitializationResult::Failure("Circular dependency detected in initialization steps");
    }
    
    m_initializedCount = 0;
    
    // 按顺序执行初始化
    for (int index : order) {
        auto& step = m_steps[index];
        InitializationResult result = step->Initialize();
        
        if (!result.success) {
            // 初始化失败，清理已初始化的步骤
            Cleanup(m_initializedCount);
            return InitializationResult::Failure(
                result.errorMessage.empty() ? 
                    std::string("Failed to initialize step: ") + step->GetName() : 
                    result.errorMessage,
                index
            );
        }
        
        m_initializedCount++;
    }
    
    return InitializationResult::Success();
}

void InitializationChain::Cleanup(int initializedCount) {
    if (initializedCount < 0) {
        initializedCount = m_initializedCount;
    }
    
    // 按逆序清理
    for (int i = initializedCount - 1; i >= 0; i--) {
        if (i < static_cast<int>(m_steps.size())) {
            m_steps[i]->Cleanup();
        }
    }
    
    m_initializedCount = 0;
}

std::vector<int> InitializationChain::TopologicalSort() {
    int n = static_cast<int>(m_steps.size());
    std::vector<int> inDegree(n, 0);
    std::vector<std::vector<int>> graph(n);
    
    // 构建依赖图
    for (int i = 0; i < n; i++) {
        auto dependencies = m_steps[i]->GetDependencies();
        for (const char* depName : dependencies) {
            int depIndex = FindStepIndex(depName);
            if (depIndex >= 0) {
                graph[depIndex].push_back(i);
                inDegree[i]++;
            }
        }
    }
    
    // 拓扑排序
    std::queue<int> q;
    for (int i = 0; i < n; i++) {
        if (inDegree[i] == 0) {
            q.push(i);
        }
    }
    
    std::vector<int> result;
    while (!q.empty()) {
        int u = q.front();
        q.pop();
        result.push_back(u);
        
        for (int v : graph[u]) {
            inDegree[v]--;
            if (inDegree[v] == 0) {
                q.push(v);
            }
        }
    }
    
    // 检查是否有循环依赖
    if (static_cast<int>(result.size()) != n) {
        return {};  // 返回空向量表示有循环依赖
    }
    
    return result;
}

int InitializationChain::FindStepIndex(const char* name) const {
    auto it = m_stepNameToIndex.find(name);
    if (it != m_stepNameToIndex.end()) {
        return it->second;
    }
    return -1;
}

