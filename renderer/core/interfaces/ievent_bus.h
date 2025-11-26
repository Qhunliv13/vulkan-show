#pragma once

#include <functional>
#include <memory>
#include <string>
#include "core/config/constants.h"

// 前向声明
class IRenderer;

// 事件类型枚举
enum class EventType {
    SceneStateChanged,      // 场景状态变化
    ButtonClicked,          // 按钮点击
    ColorChanged,           // 颜色变化
    WindowResized,          // 窗口大小变化
    MouseClicked,           // 鼠标点击（窗口坐标）
    MouseMoved,             // 鼠标移动（用于相机控制）
    MouseMovedUI,           // 鼠标移动（UI坐标，用于UI交互）
    MouseUp,                // 鼠标释放
    WindowResizeRequest,    // 窗口大小变化请求
    KeyPressed,             // 按键按下
    UIClick,                // UI点击事件（已转换的UI坐标）
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

struct MouseMovedEvent : public Event {
    float deltaX;
    float deltaY;
    bool leftButtonDown;
    MouseMovedEvent(float dx, float dy, bool buttonDown) 
        : Event(EventType::MouseMoved), deltaX(dx), deltaY(dy), leftButtonDown(buttonDown) {}
};

struct KeyPressedEvent : public Event {
    int keyCode;
    bool isPressed;
    KeyPressedEvent(int code, bool pressed) 
        : Event(EventType::KeyPressed), keyCode(code), isPressed(pressed) {}
};

struct UIClickEvent : public Event {
    float uiX;
    float uiY;
    StretchMode stretchMode;
    UIClickEvent(float x, float y, StretchMode mode) 
        : Event(EventType::UIClick), uiX(x), uiY(y), stretchMode(mode) {}
};

struct MouseMovedUIEvent : public Event {
    float uiX;
    float uiY;
    MouseMovedUIEvent(float x, float y) 
        : Event(EventType::MouseMovedUI), uiX(x), uiY(y) {}
};

struct MouseUpEvent : public Event {
    MouseUpEvent() : Event(EventType::MouseUp) {}
};

struct WindowResizeRequestEvent : public Event {
    StretchMode stretchMode;
    IRenderer* renderer;
    WindowResizeRequestEvent(StretchMode mode, IRenderer* r) 
        : Event(EventType::WindowResizeRequest), stretchMode(mode), renderer(r) {}
};

// 事件总线接口 - 用于依赖注入，替代单例
class IEventBus {
public:
    virtual ~IEventBus() = default;
    
    // 订阅事件
    using EventHandler = std::function<void(const Event&)>;
    virtual void Subscribe(EventType type, EventHandler handler) = 0;
    
    // 取消订阅（需要保存返回的ID）
    virtual size_t SubscribeWithId(EventType type, EventHandler handler) = 0;
    virtual void Unsubscribe(EventType type, size_t id) = 0;
    
    // 发布事件
    virtual void Publish(const Event& event) = 0;
    virtual void Publish(std::shared_ptr<Event> event) = 0;
    
    // 清理所有订阅
    virtual void Clear() = 0;
};

