#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstdlib> // for std::getenv

// 引入 SDK 头文件
#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/util/myLog.h"

// 测试用例：验证 DeepSeek 全量消息发送
TEST(DeepSeekProviderTest, sendMessage)
{
    // 1. 实例化 DeepSeekProvider
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider != nullptr);

    // 2. 初始化模型参数
    std::map<std::string, std::string> modelConfig;

    // 从环境变量获取 API Key
    const char *apiKey = std::getenv("deepseek_apikey");
    ASSERT_TRUE(apiKey != nullptr) << "Environment variable 'deepseek_apikey' not set!";

    modelConfig["api_key"] = apiKey;
    modelConfig["endpoint"] = "https://api.deepseek.com";

    // 执行初始化
    bool initRet = provider->initModel(modelConfig);
    ASSERT_TRUE(initRet) << "Provider initialization failed";
    ASSERT_TRUE(provider->isAvailable());

    // 3. 准备请求参数
    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}};

    // 4. 构造消息上下文
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "你好，请做一个简短的自我介绍。"});

    // 5. 发送全量消息
    std::string response = provider->sendMessage(messages, requestParam);

    // 6. 验证响应结果
    // 响应不应为空
    ASSERT_FALSE(response.empty());

    // 打印结果到日志 (方便人工确认)
    INFO("DeepSeek Response: {}", response);
}

// 测试用例：验证 DeepSeek 流式响应
TEST(DeepSeekProviderTest, sendMessageStream) {
    // 1. 实例化 DeepSeekProvider
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider != nullptr);

    // 2. 初始化模型参数 (从环境变量获取 Key)
    std::map<std::string, std::string> modelConfig;
    const char* apiKey = std::getenv("deepseek_apikey");
    ASSERT_TRUE(apiKey != nullptr) << "Environment variable 'deepseek_apikey' not set!";
    
    modelConfig["api_key"] = apiKey;
    modelConfig["endpoint"] = "https://api.deepseek.com";
    
    bool initRet = provider->initModel(modelConfig);
    ASSERT_TRUE(initRet);

    // 3. 准备请求参数
    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}
    };

    // 4. 构造消息上下文
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "请用 20 个字以内解释什么是流式响应。"});

    // 5. 定义流式回调函数 (Lambda)
    // 这里的逻辑模拟了用户如何处理接收到的数据
    auto writeChunk = [&](const std::string &chunk, bool last) {
        // 打印每一个接收到的数据块
        if (!chunk.empty()) {
            INFO("chunk : {}", chunk); 
        }
        
        // 如果检测到结束标记
        if (last) {
            INFO("[DONE] - Stream finished.");
        }
    };

    // 6. 调用流式接口
    // fullData 将包含拼接好的完整回复，用于最后的断言检查
    std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);

    // 7. 验证结果
    // 确保我们最终拿到了完整的响应数据
    ASSERT_FALSE(fullData.empty());
    INFO("Full Response : {}", fullData);
}


// 主函数：初始化环境并运行所有测试
int main(int argc, char **argv)
{
    // 1. 初始化日志系统 (输出到控制台，级别为 DEBUG)
    mylog::Logger::initLogger("testLLM", "stdout", spdlog::level::debug);

    // 2. 初始化 GTest
    testing::InitGoogleTest(&argc, argv);

    // 3. 运行所有测试用例
    return RUN_ALL_TESTS();
}