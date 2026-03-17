# ChatSDK 

`ChatSDK` 是一款自主研发的高性能基于 C++17 的大语言模型 (LLM) 接入库。旨在为 C++ 开发者提供一个简单、易用、可扩展的接口，以便将当下主流的云端或本地 AI 大模型无缝集成到自己的应用程序（如聊天服务器、终端工具等）中。

## ✨ 核心特性

* 🌐 **多模型支持**：已原生接入多种主流大模型：
  * **DeepSeek**: DeepSeek-V3 
  * **OpenAI**: GPT-4o-mini 等
  * **Google**: Gemini-2.5-flash
* 💻 **本地模型支持**：通过 Ollama 轻松接入本地运行的私有化模型（如 `deepseek-r1:1.5b`）。
* ⚡ **灵活的交互模式**：支持单轮/多轮对话，支持**流式响应 (Stream)** 带来打字机效果，也支持全量响应。
* 💾 **持久化会话管理**：内置 SQLite 数据库，自动保存和管理会话历史上下文。

---

## 📂 源码目录结构

```text
ChatSDK
├── ChatServer        # Web 聊天服务器应用示例
├── sdk               # ChatSDK 核心源码库 (本项目核心)
│   ├── CMakeLists.txt
│   ├── include       # SDK 暴露的头文件
│   └── src           # 各模型 Provider 及管理类的具体实现
├── test              # 单元测试与功能验证  
├── demo              # 快速上手 Demo (内置控制台聊天应用)
│   ├── CMakeLists.txt
│   └── cmdChatDemo.cpp
└── LICENSE
```

---

## 🧰 环境搭建与项目依赖

为了避免重复造轮子，本项目使用了一系列成熟的现代 C++ 第三方库（“脚手架”）来处理日志、网络通信、序列化等通用任务。在编译本项目之前，请确保你的系统（以 Ubuntu/Debian 为例）已安装以下依赖：

### 1. 基础工具链与“脚手架”

你可以通过以下命令一键安装基础 C++ 后端开发库：

```bash
# 1. 命令行参数解析库 (gflags)：用于处理启动参数
sudo apt-get install libgflags-dev -y

# 2. 格式化库 (fmt) 与 日志库 (spdlog)：提供高性能、分级的异步日志记录
sudo apt-get install libfmt-dev libspdlog-dev -y

# 3. JSON 处理库 (jsoncpp)：用于处理大模型 API 返回的 JSON 数据序列化与反序列化
sudo apt-get install libjsoncpp-dev -y

# 4. 单元测试框架 (gtest)：用于对 SDK 接口进行单元测试
sudo apt-get install libgtest-dev -y

# 5. SQLite3 开发包：用于本地会话的持久化存储
sudo apt-get install libsqlite3-dev -y
```

### 2. 网络通信与构建工具

各大模型的 API 接口均强制使用 **HTTPS** 协议，因此必须安装 OpenSSL。

```bash
# 1. OpenSSL 开发包 (强制依赖，用于 HTTPS 安全连接)
sudo apt-get install libssl-dev -y

# 2. CMake 与 pkg-config (跨平台构建与库查找工具)
sudo apt-get install cmake pkg-config -y

# 3. cURL (可选但推荐，用于在终端快速调试与验证 API Key 连通性)
sudo apt-get install curl -y
```

### 3. HTTP 客户端库：cpp-httplib

本项目网络层依赖于极其轻量且符合现代 C++ 风格的 `cpp-httplib` 库。这是一个 **Header-only（仅头文件）** 的库，无需编译，只需将头文件放入系统目录即可：

```bash
# 进入工作目录并克隆源码
cd ~/workspace
git clone https://github.com/yhirose/cpp-httplib.git

# 将核心头文件拷贝到系统 include 目录
sudo cp cpp-httplib/httplib.h /usr/include/
```

### 4. 获取与编译安装

在确立了系统架构并安装完所有依赖后，我们就可以拉取源码并编译 SDK 了。我们需要将其编译为静态库 `libai_chat_sdk.a` 并安装到系统目录。

**(1) 获取源码**

```bash
mkdir -p ~/workspace/AI_Project
cd ~/workspace/AI_Project
git clone https://github.com/Kutbas/ChatSDK.git
```

**(2) 编译 SDK**

项目使用 CMake 进行构建，推荐采用“外部构建”的方式保持源码整洁：

```bash
cd ChatSDK/sdk
mkdir build && cd build

# 生成 Makefile 并编译
cmake ..
make
```

编译成功后，终端将输出 `[100%] Built target ai_chat_sdk`。

**(3) 安装到系统目录**

为了方便其他项目引用，请将静态库和头文件安装到系统标准路径下（需要 root 权限）：

```bash
sudo make install
```

安装完成后，CMake 会根据 `install` 指令将文件复制到：

* **静态库**: `/usr/local/lib/libai_chat_sdk.a`
* **头文件**: `/usr/local/include/ai_chat_sdk/`

------

## 🐳 使用 Docker 开箱即用 

如果您希望在一个纯净的环境中快速体验本项目，我们提供了一键构建的 Dockerfile。该镜像不仅会自动配置所有依赖、安装 SDK，还会**自动编译内置的 Demo 程序**。

### 1. 创建 Dockerfile

在任意空目录下创建一个名为 Dockerfile 的文件，填入以下内容：

```dockerfile
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

# 1. 安装基础编译环境与网络工具
RUN apt-get update && apt-get install -y \
    build-essential git sudo cmake pkg-config curl nano vim

# 2. 安装项目底层依赖 ("脚手架")
RUN apt-get install -y \
    libgflags-dev libfmt-dev libspdlog-dev libjsoncpp-dev \
    libgtest-dev libsqlite3-dev libssl-dev

# 3. 安装 cpp-httplib (Header-only)
RUN mkdir -p /workspace && cd /workspace && \
    git clone https://github.com/yhirose/cpp-httplib.git && \
    cp cpp-httplib/httplib.h /usr/include/

# 4. 获取项目源码并完成 SDK 编译与安装
RUN cd /workspace && \
    git clone https://github.com/Kutbas/ChatSDK.git && \
    cd ChatSDK/sdk && \
    mkdir build && cd build && \
    cmake .. && make && make install

# 5. 自动编译自带的 Demo 程序
RUN cd /workspace/ChatSDK/demo && \
    mkdir build && cd build && \
    cmake .. && make

# 6. 设置工作目录到 Demo 运行目录
WORKDIR /workspace/ChatSDK/demo/build

# 7. 默认启动 Demo 程序
CMD ["./AIChatDemo"]
```

### 2. 构建并运行 AI 对话

```Bash
# 构建镜像 (耗时约几分钟)
docker build -t chatsdk-env .

# 启动容器并传入 API Key！直接开启 AI 聊天！
# (注意：因为程序需要接收键盘输入，必须加上 -it 参数)
docker run -it --rm -e deepseek_apikey="sk-您的真实API_KEY" chatsdk-env
```

执行完毕后，您将直接看到控制台闪烁着光标，此时已经可以丝滑体验大模型的流式打字机回复了！

---

## 🚀 快速上手：构建你的第一个 AI 聊天程序

如果你是通过宿主机手动安装的 SDK，可以直接进入仓库自带的 `demo` 目录运行测试程序。

### 1. CMakeLists.txt 配置

在你的新项目中，需要链接我们刚才编译的 `ai_chat_sdk` 以及它依赖的第三方库：

```cmake
project(AIChatDemo)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)

find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(/usr/local/lib)

add_executable(AIChatDemo cmdChatDemo.cpp)

# 链接 SDK 及其依赖的脚手架库
target_link_libraries(AIChatDemo PRIVATE 
    ai_chat_sdk 
    fmt jsoncpp OpenSSL::SSL OpenSSL::Crypto 
    gflags spdlog sqlite3
)
```

### 2. C++ 示例代码 (`cmdChatDemo.cpp`)

这段代码展示了 SDK 的核心调用流程：**日志初始化 -> 模型配置 -> 会话创建 -> 消息收发**。

```cpp
#include <iostream>
#include <memory>
#include <vector>
#include <string>
#include <ai_chat_sdk/ChatSDK.h>
#include <ai_chat_sdk/util/myLog.h>

void sendMessageStream(ai_chat_sdk::ChatSDK &chatSDK, const std::string &sessionId) {
    std::cout << "\n-------------- 发送消息 --------------\nUser > ";
    std::string message;
    std::getline(std::cin, message);
    if (message.empty()) return;

    std::cout << "--------------------------------------\nAssistant > ";
    // 使用 Lambda 表达式处理流式响应
    chatSDK.sendMessageStream(sessionId, message,[](const std::string &chunk, bool done) {
        std::cout << chunk << std::flush;
        if (done) std::cout << "\n-------------- 回复结束 --------------\n";
    });
}

int main() {
    // 1. 初始化日志系统 (必须步骤)
    mylog::Logger::initLogger("aiChatServer", "stdout", spdlog::level::info);
    
    // 2. 配置 DeepSeek 模型
    ai_chat_sdk::ChatSDK chatSDK;
    ai_chat_sdk::APIConfig deepseekConfig;
    
    // 建议从环境变量获取 API Key 保证安全
    const char* apiKey = std::getenv("deepseek_apikey");
    if (!apiKey) {
        std::cerr << "Error: 请设置环境变量 deepseek_apikey" << std::endl;
        return -1;
    }
    
    deepseekConfig._apiKey = apiKey;
    deepseekConfig._temperature = 0.7;
    deepseekConfig._maxTokens = 2048;
    deepseekConfig._modelName = "deepseek-chat";

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> configs;
    configs.push_back(std::make_shared<ai_chat_sdk::APIConfig>(deepseekConfig));

    // 3. 初始化 SDK 并创建会话
    chatSDK.initModels(configs);
    std::string sessionId = chatSDK.createSession("deepseek-chat");
    std::cout << "会话创建成功，Session ID: " << sessionId << std::endl;

    // 4. 开启交互循环
    int userOp = 1;
    while (true) {
        std::cout << "\n[1] 发送消息 [0] 退出程序: ";
        if (!(std::cin >> userOp) || userOp == 0) break;
        std::cin.ignore(); // 清理换行符
        sendMessageStream(chatSDK, sessionId);
    }
    std::cout << "程序退出..." << std::endl;
    return 0;
}
```

### 3. 编译与运行

```bash
# 编译 Demo
mkdir build && cd build
cmake ..
make

# 导出你的 API Key 并运行程序
export deepseek_apikey="sk-你的真实API_KEY"
./AIChatDemo
```

运行后，你将体验到丝滑的打字机输出效果，并且本地会自动生成 `chat.db` 数据库文件，用于自动管理和长久保存你们的多轮对话上下文！

---

## 📜 许可证

本项目遵循 [GPL-3.0] 许可证开源。