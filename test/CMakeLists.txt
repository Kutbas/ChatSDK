# 1. 设置 CMake 最小版本
cmake_minimum_required(VERSION 3.10)

# 2. 定义项目名称
project(testLLM)

# 3. 设置 C++ 标准 (C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 设置构建类型为 Debug (方便调试)
set(CMAKE_BUILD_TYPE Debug)

# 5. 添加可执行文件
# 注意：我们需要手动添加 SDK 的相关源文件，或者先将 SDK 编译为库再链接
# 这里为了简单直接，我们将 SDK 源码文件加入编译列表
add_executable(testLLM
    testLLM.cpp
    ../sdk/src/util/myLog.cpp
    ../sdk/src/DeepSeekProvider.cpp
)

# 6. 设置输出目录 (可选，将可执行文件统一放到 build 根目录)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

# 7. 添加头文件搜索路径
# 让编译器能找到 "LLMProvider.h", "util/myLog.h" 等
include_directories(${CMAKE_SOURCE_DIR}/../sdk/include)

# 8. 查找并配置 OpenSSL (HTTPS 必须)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# 9. 定义预处理宏：开启 cpp-httplib 的 SSL 支持
# 如果不加这个宏，httplib 默认只支持 HTTP，访问 https://api.deepseek.com 会报错
target_compile_definitions(testLLM PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# 10. 链接依赖库
target_link_libraries(testLLM
    jsoncpp # JSON 处理
    fmt # 格式化库 (spdlog 依赖)
    spdlog # 日志库
    gtest # Google Test 框架
    OpenSSL::SSL # SSL 加密
    OpenSSL::Crypto # 加密算法
    pthread # GTest 和 spdlog 需要线程支持
)
