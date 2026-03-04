#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <cstdlib> // for std::getenv

// 引入 SDK 头文件
#include "../sdk/include/DeepSeekProvider.h"
#include "../sdk/include/ChatGPTProvider.h"
#include "../sdk/include/GeminiProvider.h"
#include "../sdk/include/OllamaLLMProvider.h"
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
    messages.push_back({"user", "我现在正在进行 DeepSeek 全量返回测试，如果成功请回复：DeepSeek 全量返回测试成功！"});

    // 5. 发送全量消息
    std::string response = provider->sendMessage(messages, requestParam);

    // 6. 验证响应结果
    // 响应不应为空
    ASSERT_FALSE(response.empty());

    // 打印结果到日志 (方便人工确认)
    INFO("DeepSeek Response: {}", response);
}

// 测试用例：验证 DeepSeek 流式响应
TEST(DeepSeekProviderTest, sendMessageStream)
{
    // 1. 实例化 DeepSeekProvider
    auto provider = std::make_shared<ai_chat_sdk::DeepSeekProvider>();
    ASSERT_TRUE(provider != nullptr);

    // 2. 初始化模型参数 (从环境变量获取 Key)
    std::map<std::string, std::string> modelConfig;
    const char *apiKey = std::getenv("deepseek_apikey");
    ASSERT_TRUE(apiKey != nullptr) << "Environment variable 'deepseek_apikey' not set!";

    modelConfig["api_key"] = apiKey;
    modelConfig["endpoint"] = "https://api.deepseek.com";

    bool initRet = provider->initModel(modelConfig);
    ASSERT_TRUE(initRet);

    // 3. 准备请求参数
    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}};

    // 4. 构造消息上下文
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "我现在正在进行 DeepSeek 流式响应测试，如果成功请回复：DeepSeek 流式响应测试成功！"});

    // 5. 定义流式回调函数 (Lambda)
    // 这里的逻辑模拟了用户如何处理接收到的数据
    auto writeChunk = [&](const std::string &chunk, bool last)
    {
        // 打印每一个接收到的数据块
        if (!chunk.empty())
        {
            INFO("chunk : {}", chunk);
        }

        // 如果检测到结束标记
        if (last)
        {
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

// 测试用例：验证 ChatGPT 全量返回
TEST(ChatGPTProviderTest, sendMessage)
{
    // 1. 实例化 Provider
    auto provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
    ASSERT_TRUE(provider != nullptr);

    // 2. 初始化配置
    std::map<std::string, std::string> modelParam;
    // 从环境变量获取 Key，避免硬编码
    const char *apiKey = std::getenv("chatgpt_apikey");
    ASSERT_TRUE(apiKey != nullptr) << "Environment variable 'chatgpt_apikey' not set!";

    modelParam["api_key"] = apiKey;
    modelParam["endpoint"] = "https://api.openai.com"; // OpenAI 官方端点

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    // 3. 构造请求参数
    // 注意：OpenAI 新版 API 推荐使用 max_output_tokens
    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_output_tokens", "2048"}};

    // 4. 构造消息
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "我现在正在进行 ChatGPT 全量返回测试，如果成功请回复：ChatGPT 全量返回测试成功！"});

    // 5. 发送请求并验证
    std::string fullData = provider->sendMessage(messages, requestParam);

    // 确保返回不为空
    ASSERT_FALSE(fullData.empty());

    // 打印模型回复
    INFO("ChatGPT Response: {}", fullData);
}

// 测试用例：验证 ChatGPT 流式响应
TEST(ChatGPTProviderTest, sendMessageStream)
{
    auto provider = std::make_shared<ai_chat_sdk::ChatGPTProvider>();
    ASSERT_TRUE(provider != nullptr);

    // 1. 初始化配置 (从环境变量获取 Key)
    std::map<std::string, std::string> modelParam;
    modelParam["api_key"] = std::getenv("chatgpt_apikey");
    modelParam["endpoint"] = "https://api.openai.com"; // OpenAI 官方端点

    ASSERT_TRUE(provider->initModel(modelParam));
    ASSERT_TRUE(provider->isAvailable());

    // 2. 构造请求参数
    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_output_tokens", "2048"}};

    // 3. 构造消息上下文
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "我现在正在进行 ChatGPT 流式响应测试，如果成功请回复：ChatGPT 流式响应测试成功！"});

    // 4. 定义回调函数 (Lambda)
    auto writeChunk = [&](const std::string &chunk, bool last)
    {
        // 打印每一个数据块
        INFO("chunk : {}", chunk);
        if (last)
        {
            INFO("[DONE] Stream finished.");
        }
    };

    // 5. 发送流式请求
    std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);

    // 6. 验证结果
    ASSERT_FALSE(fullData.empty());
    INFO("Full Response : {}", fullData);
}

// 测试用例：验证 Gemini 全量返回
TEST(GeminiProviderTest, sendMessage)
{
    // 1. 实例化 GeminiProvider
    auto provider = std::make_shared<ai_chat_sdk::GeminiProvider>();
    ASSERT_TRUE(provider != nullptr);

    // 2. 初始化配置
    std::map<std::string, std::string> modelParam;

    // 从环境变量获取 Key
    const char *apiKey = std::getenv("gemini_apikey");
    ASSERT_TRUE(apiKey != nullptr) << "Environment variable 'gemini_apikey' not set!";

    modelParam["api_key"] = apiKey;
    // Gemini 官方 API 端点
    modelParam["endpoint"] = "https://generativelanguage.googleapis.com";

    // 执行初始化
    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    // 3. 构造请求参数
    // 注意：Gemini 兼容接口使用标准的 max_tokens
    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}};

    // 4. 构造消息
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "我现在正在进行 Gemini 全量返回测试，如果成功请回复：Gemini 全量返回测试成功！"});

    // 5. 调用全量发送接口
    std::string fullData = provider->sendMessage(messages, requestParam);

    // 6. 验证结果
    ASSERT_FALSE(fullData.empty());
    INFO("Gemini Response: {}", fullData);
}

// 测试用例：验证 Gemini 流式响应
TEST(GeminiProviderTest, sendMessageStream)
{
    // 1. 实例化 GeminiProvider
    auto provider = std::make_shared<ai_chat_sdk::GeminiProvider>();
    ASSERT_TRUE(provider != nullptr);

    // 2. 初始化配置
    std::map<std::string, std::string> modelParam;

    // 从环境变量获取 Key，避免硬编码
    const char *apiKey = std::getenv("gemini_apikey");
    ASSERT_TRUE(apiKey != nullptr) << "Environment variable 'gemini_apikey' not set!";

    modelParam["api_key"] = apiKey;
    modelParam["endpoint"] = "https://generativelanguage.googleapis.com"; // Gemini 官方端点

    // 执行初始化
    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    // 3. 构造请求参数
    // Gemini 兼容接口使用标准的 max_tokens
    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}};

    // 4. 构造消息上下文
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "我现在正在进行 Gemini 流式响应测试，如果成功请回复：Gemini 流式响应测试成功！"});

    // 5. 定义回调函数 (Lambda 表达式)
    // 参数 chunk: 本次接收到的增量文本
    // 参数 last: 是否为最后一条数据
    auto writeChunk = [&](const std::string &chunk, bool last)
    {
        // 打印每一个数据块
        if (!chunk.empty())
        {
            INFO("chunk : {}", chunk);
        }

        // 当检测到流结束时，打印标记
        if (last)
        {
            INFO("[DONE] Stream finished.");
        }
    };

    // 6. 调用流式发送接口
    // fullData 将包含拼接好的完整回复，用于最后的断言检查
    std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);

    // 7. 验证结果
    ASSERT_FALSE(fullData.empty());
    INFO("Gemini Full Response: {}", fullData);
}

// 测试用例：验证 Ollama 全量返回
TEST(OllamaLLMProviderTest, sendMessage)
{
    // 1. 实例化 OllamaLLMProvider
    auto provider = std::make_shared<ai_chat_sdk::OllamaLLMProvider>();
    ASSERT_TRUE(provider != nullptr);

    // 2. 初始化配置
    std::map<std::string, std::string> modelParam;
    // 必填：指定本地已下载的模型名称
    modelParam["model_name"] = "deepseek-r1:1.5b";
    // 选填：模型描述
    modelParam["model_desc"] = "本地部署 deepseek-r1:1.5b 模型，采用专家混合架构，专注于深度理解与推理";
    // 必填：Ollama 服务地址 (本地通常为 http://127.0.0.1:11434)
    // 如果是远程服务器，请修改 IP，例如 http://192.168.71.99:11434
    modelParam["endpoint"] = "http://192.168.71.99:11434";

    // 执行初始化
    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    // 3. 构造请求参数
    // Ollama 兼容 OpenAI 格式，但底层使用的是 num_ctx，我们在 Provider 内部做了映射
    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}};

    // 4. 构造消息
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "我现在正在进行 Ollama 全量返回测试，如果成功请回复：Ollama 全量返回测试成功！"});

    // 5. 调用全量发送接口
    std::string fullData = provider->sendMessage(messages, requestParam);

    // 6. 验证结果
    ASSERT_FALSE(fullData.empty());
    INFO("Ollama Response: {}", fullData);
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