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
