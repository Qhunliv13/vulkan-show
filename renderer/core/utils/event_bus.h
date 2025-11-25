#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <string>
#include <memory>

// 事件类型枚举
enum class EventType {
    SceneStateChanged,      // 场景状态变化
    ButtonClicked,          // 按钮点击
    ColorChanged,           // 颜色变化
    WindowResized,          // 窗口大小变化
    MouseClicked,           // 鼠标点击
    MouseMoved,             // 鼠标移动
    KeyPressed,             // 按键按下
    Custom                  // 自定义事件
};

// 事件基类
struct Event {
    EventType type;
    virtual ~Event() = default;
    Event(EventType t) : type(t) {}
};

// 具体事件类型
struct SceneStateChangedEvent : public Event {
    int oldState;
    int newState;
    SceneStateChangedEvent(int oldS, int newS) 
        : Event(EventType::SceneStateChanged), oldState(oldS), newState(newS) {}
};

struct ButtonClickedEvent : public Event {
    std::string buttonId;
    ButtonClickedEvent(const std::string& id) 
        : Event(EventType::ButtonClicked), buttonId(id) {}
};

struct ColorChangedEvent : public Event {
    float r, g, b, a;
    ColorChangedEvent(float r_, float g_, float b_, float a_) 
        : Event(EventType::ColorChanged), r(r_), g(g_), b(b_), a(a_) {}
};

// 事件总线 - 观察者模式实现，线程安全
class EventBus {
public:
    // 获取单例实例
    static EventBus& GetInstance();
    
    // 订阅事件
    using EventHandler = std::function<void(const Event&)>;
    void Subscribe(EventType type, EventHandler handler);
    
    // 取消订阅（需要保存返回的ID）
    size_t SubscribeWithId(EventType type, EventHandler handler);
    void Unsubscribe(EventType type, size_t id);
    
    // 发布事件
    void Publish(const Event& event);
    void Publish(std::shared_ptr<Event> event);
    
    // 清理所有订阅
    void Clear();

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

