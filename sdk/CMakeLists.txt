# 1. 最低版本要求
cmake_minimum_required(VERSION 3.10)

# 2. 定义 SDK 名称
set(SDK_NAME "ai_chat_sdk")

# 3. 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 4. 设置构建类型 (Release 模式以优化性能)
set(CMAKE_BUILD_TYPE Release)

# 5. 自动搜集源文件和头文件
# GLOB_RECURSE 会递归查找指定目录下的匹配文件
file(GLOB_RECURSE SDK_SOURCES "src/*.cpp")
file(GLOB_RECURSE SDK_HEADERS "include/*.h")

# 6. 生成静态库
add_library(${SDK_NAME} STATIC ${SDK_SOURCES} ${SDK_HEADERS})

# 7. 设置输出目录
# 将生成的 .a 文件放在 build/lib 目录下，而不是深埋在 CMake 临时目录中
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_SOURCE_DIR}/build)

# 8. 查找 OpenSSL
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})

# 9. 定义预处理宏
# 开启 httplib 的 SSL 支持
target_compile_definitions(${SDK_NAME} PRIVATE CPPHTTPLIB_OPENSSL_SUPPORT)

# 10. 设置头文件搜索路径
# PUBLIC 属性允许使用该库的用户也能自动继承这个 include 路径
target_include_directories(${SDK_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# 11. 链接依赖库 (可选)
# 注意：生成的静态库通常不包含依赖库的二进制代码。用户在使用我们的静态库时，
# 仍需要在他们的项目中链接 jsoncpp, sqlite3 等库。
# 这里的 target_link_libraries 主要是为了通过编译检查。
target_link_libraries(${SDK_NAME} 
    jsoncpp 
    fmt 
    spdlog 
    sqlite3 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# 12. 安装静态库
# DESTINATION lib -> /usr/local/lib (在 Linux 下)
install(TARGETS ${SDK_NAME} ARCHIVE DESTINATION lib)

# 13. 安装头文件
# 将 include 目录下的所有内容拷贝到 /usr/local/include/ai_chat_sdk
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ 
        DESTINATION include/ai_chat_sdk 
        FILES_MATCHING PATTERN "*.h")