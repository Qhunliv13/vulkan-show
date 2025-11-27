#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "core/interfaces/ievent_bus.h"  // 4. 项目头文件

/**
 * 事件总线 - 实现 IEventBus 接口，支持依赖注入
 * 
 * 职责：提供发布-订阅模式的事件通信机制，解耦组件间通信
 * 设计：通过接口抽象，支持依赖注入，禁止使用单例
 * 
 * 使用方式：
 * 1. 通过依赖注入获取接口指针
 * 2. 使用 Subscribe() 订阅事件，使用 Publish() 发布事件
 * 3. 注意：必须在组件 Cleanup() 时取消所有订阅
 */
class EventBus : public IEventBus {
public:
    /**
     * 构造函数（支持依赖注入）
     */
    EventBus() = default;
    
    /**
     * 析构函数
     */
    ~EventBus() = default;
    
    /**
     * 初始化事件总线
     * 初始化内部状态，准备使用
     */
    void Initialize();
    
    /**
     * 清理资源
     * 清除所有订阅，准备销毁
     */
    void Cleanup();
    
    // IEventBus 接口实现
    void Subscribe(EventType type, EventHandler handler) override;
    size_t SubscribeWithId(EventType type, EventHandler handler) override;
    void Unsubscribe(EventType type, size_t id) override;
    void Publish(const Event& event) override;
    void Publish(std::shared_ptr<Event> event) override;
    void Clear() override;

private:
    // 禁止拷贝和赋值
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    struct HandlerInfo {
        size_t id;
        EventHandler handler;
    };
    
    std::unordered_map<EventType, std::vector<HandlerInfo>> m_handlers;
    std::mutex m_mutex;
    size_t m_nextId = 1;
    bool m_initialized = false;
};

