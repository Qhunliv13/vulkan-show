#pragma once

#include <functional>      // 2. 系统头文件
#include <memory>          // 2. 系统头文件
#include <mutex>           // 2. 系统头文件
#include <string>          // 2. 系统头文件
#include <unordered_map>   // 2. 系统头文件
#include <vector>          // 2. 系统头文件

#include "core/interfaces/ievent_bus.h"  // 4. 项目头文件

// 事件总线 - 实现 IEventBus 接口，支持依赖注入
// 禁止使用单例，必须通过依赖注入使用
class EventBus : public IEventBus {
public:
    // 公共构造函数（支持依赖注入）
    EventBus() = default;
    ~EventBus() = default;
    
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
};

