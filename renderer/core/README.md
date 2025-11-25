# Core 模块组织结构

`renderer/core` 文件夹已按功能分类组织，便于维护和查找。

## 文件夹结构

### 📁 `interfaces/` - 接口定义
所有抽象接口和协议定义：
- `irenderer.h` - 渲染器接口
- `iconfig_provider.h` - 配置提供者接口
- `iscene_provider.h` - 场景提供者接口
- `irenderable.h` - 可渲染对象接口
- `irenderer_factory.h` - 渲染器工厂接口
- `iwindow_resize_handler.h` - 窗口大小变化处理接口

### 📁 `managers/` - 核心管理器
应用的核心管理组件：
- `application.*` - 应用程序主类
- `app_initializer.*` - 应用初始化器
- `config_manager.*` - 配置管理器
- `event_manager.*` - 事件管理器
- `scene_manager.*` - 场景管理器
- `window_manager.*` - 窗口管理器
- `render_scheduler.*` - 渲染调度器

### 📁 `ui/` - UI管理器
用户界面相关的管理器：
- `ui_manager.*` - UI管理器主类
- `ui_manager_getters.*` - UI管理器访问器
- `button_ui_manager.*` - 按钮UI管理器
- `color_ui_manager.*` - 颜色UI管理器
- `slider_ui_manager.*` - 滑块UI管理器

### 📁 `utils/` - 工具类
通用工具和辅助类：
- `logger.*` - 日志记录器
- `event_bus.*` - 事件总线
- `fps_monitor.*` - FPS监控器
- `input_handler.*` - 输入处理器

### 📁 `config/` - 配置和常量
配置定义和常量：
- `constants.h` - 常量定义（窗口大小、枚举等）
- `stretch_params.h` - 拉伸参数定义
- `render_context.h` - 渲染上下文定义

### 📁 `handlers/` - 处理器
特定功能的处理器：
- `window_message_handler.*` - 窗口消息处理器

## Include 路径示例

```cpp
// 接口
#include "core/interfaces/irenderer.h"
#include "core/interfaces/iconfig_provider.h"

// 管理器
#include "core/managers/application.h"
#include "core/managers/event_manager.h"

// UI
#include "core/ui/ui_manager.h"
#include "core/ui/button_ui_manager.h"

// 工具
#include "core/utils/logger.h"
#include "core/utils/fps_monitor.h"

// 配置
#include "core/config/constants.h"
#include "core/config/stretch_params.h"

// 处理器
#include "core/handlers/window_message_handler.h"
```

## 注意事项

- 所有 `.obj` 文件是编译生成的中间文件，可以忽略
- 如果添加新文件，请按照功能分类放入对应文件夹
- 更新 include 路径时，请使用新的路径格式

