#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>
#include <memory>
#include "core/interfaces/ievent_bus.h"

// 事件总线 - 实现 IEventBus 接口，支持依赖注入
// 保留单例方法以支持向后兼容，但推荐使用依赖注入
class EventBus : public IEventBus {
public:
    // 获取单例实例（向后兼容）
    static EventBus& GetInstance();
    
    // IEventBus 接口实现
    void Subscribe(EventType type, EventHandler handler) override;
    size_t SubscribeWithId(EventType type, EventHandler handler) override;
    void Unsubscribe(EventType type, size_t id) override;
    void Publish(const Event& event) override;
    void Publish(std::shared_ptr<Event> event) override;
    void Clear() override;

private:
    EventBus() = default;
    ~EventBus() = default;
    EventBus(const EventBus&) = delete;
    EventBus& operator=(const EventBus&) = delete;
    
    struct HandlerInfo {
        size_t id;
        EventHandler handler;
    };
    
    std::unordered_map<EventType, std::vector<HandlerInfo>> m_handlers;
    std::mutex m_mutex;
    size_t m_nextId = 1;
};

