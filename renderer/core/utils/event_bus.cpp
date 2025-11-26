#include "core/utils/event_bus.h"  // 1. 对应头文件

#include <algorithm>  // 2. 系统头文件

void EventBus::Subscribe(EventType type, EventHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers[type].push_back({m_nextId++, handler});
}

size_t EventBus::SubscribeWithId(EventType type, EventHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    size_t id = m_nextId++;
    m_handlers[type].push_back({id, handler});
    return id;
}

void EventBus::Unsubscribe(EventType type, size_t id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto& handlers = m_handlers[type];
    handlers.erase(
        std::remove_if(handlers.begin(), handlers.end(),
            [id](const HandlerInfo& info) { return info.id == id; }),
        handlers.end()
    );
}

void EventBus::Publish(const Event& event) {
    std::vector<EventHandler> handlersToCall;
    
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_handlers.find(event.type);
        if (it != m_handlers.end()) {
            for (const auto& info : it->second) {
                handlersToCall.push_back(info.handler);
            }
        }
    }
    
    // 在锁外调用处理器，避免死锁
    for (const auto& handler : handlersToCall) {
        handler(event);
    }
}

void EventBus::Publish(std::shared_ptr<Event> event) {
    if (event) {
        Publish(*event);
    }
}

void EventBus::Clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_handlers.clear();
}

