# CMake 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 项目名称
project(AIChatServer)

# 1. 设置 C++ 标准
# cpp-httplib 和项目中使用的部分现代特性依赖 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 2. 设置构建类型为 Debug (便于开发阶段排错)
set(CMAKE_BUILD_TYPE Debug)

# 3. 添加可执行文件
# 编译 ChatServer 类的实现和启动入口
add_executable(AIChatServer ChatServer.cpp main.cpp)

# 4. 查找系统依赖包
# OpenSSL: 用于 httplib 支持 HTTPS 请求
find_package(OpenSSL REQUIRED)
# Threads: 用于 httplib 和 spdlog 的底层多线程支持 (即 pthread)
find_package(Threads REQUIRED)

# 5. 包含头文件路径
include_directories(${OPENSSL_INCLUDE_DIR})

# 6. 添加编译定义 (宏定义)
# 必须在编译目标上公开定义此宏，否则 httplib 无法开启 SSL 支持
target_compile_definitions(AIChatServer PUBLIC CPPHTTPLIB_OPENSSL_SUPPORT)

# 7. 设置静态库的查找目录
# 假设我们之前编译好的 ai_chat_sdk 已经 make install 到了系统标准路径
link_directories(/usr/local/lib)

# 8. 链接所有依赖库
# 注意：ai_chat_sdk 是我们自己封装的顶层业务库，其余为第三方基础库
target_link_libraries(AIChatServer PRIVATE
    ai_chat_sdk
    Threads::Threads
    fmt
    jsoncpp
    OpenSSL::SSL
    OpenSSL::Crypto
    gflags
    spdlog
    sqlite3
)
