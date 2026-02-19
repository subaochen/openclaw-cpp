# OpenClaw-CPP

OpenClaw-CPP是OpenClaw的C++实现，提供事件驱动的架构设计、配置管理、日志系统和基础通信功能。

## 项目结构

```
openclaw-cpp/
├── include/              # 头文件目录
│   ├── events/          # 事件驱动系统头文件
│   │   └── EventDispatcher.h
│   ├── config/          # 配置管理系统头文件
│   │   └── ConfigManager.h
│   ├── logging/         # 日志系统头文件
│   │   └── Logger.h
│   └── communication/   # 通信系统头文件
│       └── Communicator.h
├── src/                 # 源文件目录
│   ├── events/          # 事件驱动系统实现
│   │   └── EventDispatcher.cpp
│   ├── config/          # 配置管理系统实现
│   │   └── ConfigManager.cpp
│   ├── logging/         # 日志系统实现
│   │   └── Logger.cpp
│   ├── communication/   # 通信系统实现
│   │   └── Communicator.cpp
│   └── main.cpp         # 主程序入口
├── tests/               # 测试文件目录
│   ├── EventDispatcherTest.cpp
│   ├── ConfigManagerTest.cpp
│   ├── LoggerTest.cpp
│   ├── CommunicatorTest.cpp
│   └── CMakeLists.txt
├── build/               # 构建输出目录
├── CMakeLists.txt       # CMake构建文件
├── config.ini           # 系统配置文件
└── README.md            # 项目说明文件
```

## 系统架构

OpenClaw-CPP采用事件驱动的架构设计，包含以下核心组件：

### 1. 事件驱动系统 (EventDispatcher)
- 支持多种事件类型
- 事件处理函数注册和调用
- 事件调度和分发
- 事件统计和队列监控

### 2. 配置管理系统 (ConfigManager)
- 支持多种配置文件格式 (INI、JSON、YAML、XML)
- 配置加载和访问
- 配置设置和删除
- 配置文件的重新加载和保存

### 3. 日志系统 (Logger)
- 支持多种日志级别 (DEBUG、INFO、WARNING、ERROR、CRITICAL)
- 控制台和文件输出
- 日志文件管理 (旋转、大小限制)
- 自定义日志格式

### 4. 通信系统 (Communicator)
- 点对点通信
- 广播通信
- 消息处理函数注册
- 连接管理和心跳检测
- 自动重新连接

## 编译和安装

### 编译要求

- C++17 编译器 (GCC/Clang/MSVC)
- CMake 3.15 或更高版本
- Google Test 框架 (用于测试)

### 编译步骤

1. 克隆项目到本地：
   ```bash
   git clone <项目地址>
   cd openclaw-cpp
   ```

2. 创建并进入构建目录：
   ```bash
   mkdir -p build && cd build
   ```

3. 配置项目并生成构建系统：
   ```bash
   cmake ..
   ```

4. 编译项目：
   ```bash
   cmake --build .
   ```

5. 运行测试：
   ```bash
   ctest
   ```

### 安装

编译完成后，可以使用以下命令安装项目：

```bash
sudo cmake --install .
```

## 使用说明

### 系统初始化

```cpp
#include <iostream>
#include "events/EventDispatcher.h"
#include "config/ConfigManager.h"
#include "logging/Logger.h"
#include "communication/Communicator.h"

using namespace openclaw;

int main() {
    // 初始化组件
    ConfigManager::getInstance().loadConfig("config.ini");
    Logger::getInstance().initialize("openclaw.log");
    Communicator::getInstance().initialize();
    EventDispatcher::getInstance().setEventDispatchEnabled(true);
    
    // 系统运行逻辑...
    
    return 0;
}
```

### 事件驱动编程

```cpp
// 注册事件处理函数
EventDispatcher::getInstance().registerEventHandler(EventType::INFO_MESSAGE, 
    [](const Event& event) {
        Logger::getInstance().info("Received event: " + event.data);
    });

// 发送事件
EventDispatcher::getInstance().dispatchEvent(EventType::INFO_MESSAGE, 
    "sender", "Test event data");
```

### 配置管理

```cpp
// 获取配置值
std::string logLevel = ConfigManager::getInstance().getString("log_level");
int maxLogSize = ConfigManager::getInstance().getInt("max_log_size");
bool debugMode = ConfigManager::getInstance().getBool("debug_mode");

// 设置配置值
ConfigManager::getInstance().setString("new_config", "value");
```

### 通信系统

```cpp
// 连接到服务器
Communicator::getInstance().connect("localhost", 8080);

// 发送消息
Communicator::getInstance().sendMessage("user1", "Hello!");

// 接收消息
std::string receivedMessage = Communicator::getInstance().receiveMessage("user1");

// 广播消息
Communicator::getInstance().sendBroadcast("Message to all users");
```

## 项目管理

项目使用OpenClaw任务管理系统，包含以下任务：

- phase1_architecture：系统架构设计
- phase1_development：系统架构实现
- phase1_testing：系统测试
- phase1_management：任务管理

## 许可证

OpenClaw-CPP采用MIT许可证，详情请参考LICENSE文件。

## 联系方式

如有问题或建议，请通过以下方式联系：

- 项目主页：<https://openclaw.org>
- 邮箱：support@openclaw.org
- 问题追踪：GitHub Issues