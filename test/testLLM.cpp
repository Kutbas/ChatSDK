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
#include "../sdk/include/SessionManager.h"
#include "../sdk/include/util/myLog.h"
#include "../sdk/include/ChatSDK.h"

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
    modelParam["endpoint"] = "http://192.168.71.22:11434";

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

// 测试用例：验证 Ollama 流式响应
TEST(OllamaLLMProviderTest, sendMessageStream)
{
    auto provider = std::make_shared<ai_chat_sdk::OllamaLLMProvider>();
    ASSERT_TRUE(provider != nullptr);

    std::map<std::string, std::string> modelParam;
    modelParam["model_name"] = "deepseek-r1:1.5b";
    modelParam["model_desc"] = "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理";
    modelParam["endpoint"] = "http://192.168.71.22:11434";

    provider->initModel(modelParam);
    ASSERT_TRUE(provider->isAvailable());

    std::map<std::string, std::string> requestParam = {
        {"temperature", "0.7"},
        {"max_tokens", "2048"}};
    std::vector<ai_chat_sdk::Message> messages;
    messages.push_back({"user", "我现在正在进行 Ollama 流式响应测试，如果成功请回复：Ollama 流式响应测试成功！"});

    auto writeChunk = [&](const std::string &chunk, bool last)
    {
        INFO("chunk : {}", chunk);
        if (last)
        {
            INFO("[DONE]");
        }
    };
    std::string fullData = provider->sendMessageStream(messages, requestParam, writeChunk);
    ASSERT_FALSE(fullData.empty());
    INFO("response : {}", fullData);
}

// 测试用例：验证 SessionManager 数据持久化 (SQLite)
TEST(SessionManagerTest, PersistenceTest)
{
    const std::string dbName = "test_persistence.db";
    // 清理旧数据，确保环境纯净
    std::remove(dbName.c_str());

    std::string targetSessionId;

    // 阶段一：生产数据
    {
        ai_chat_sdk::SessionManager managerA(dbName);
        targetSessionId = managerA.createSession("deepseek-chat");

        ai_chat_sdk::Message msg("user", "Hello Persistence");
        managerA.addMessage(targetSessionId, msg);

        INFO("Manager A created session: {}", targetSessionId);
    } // managerA 在这里析构，模拟程序退出

    // 阶段二：恢复数据
    {
        ai_chat_sdk::SessionManager managerB(dbName);

        // 1. 验证会话是否存在
        auto session = managerB.getSession(targetSessionId);
        ASSERT_TRUE(session != nullptr);
        ASSERT_EQ(session->_modelName, "deepseek-chat");

        // 2. 验证历史消息是否加载 (懒加载测试)
        auto history = managerB.getHistoryMessages(targetSessionId);
        ASSERT_EQ(history.size(), 1);
        ASSERT_EQ(history[0]._content, "Hello Persistence");

        INFO("Manager B successfully restored session and messages.");
    }
}

// 测试ChatSDK
TEST(ChatSDKTest, sendMessage)
{
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    ASSERT_TRUE(sdk != nullptr);

    // 配置支持的模型参数：云模型-deepseek-chat gpt-4o-mini gemini-2.5-flash   Ollama本地接入deepseek-r1:1.5b
    // deepseek-chat
    auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    ASSERT_TRUE(deepseekConfig != nullptr);
    deepseekConfig->_modelName = "deepseek-chat";
    deepseekConfig->_apiKey = std::getenv("deepseek_apikey");
    ASSERT_FALSE(deepseekConfig->_apiKey.empty());
    deepseekConfig->_temperature = 0.7;
    deepseekConfig->_maxTokens = 2048;

    // gpt-4o-mini
    auto chatGPTConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    ASSERT_TRUE(chatGPTConfig != nullptr);
    chatGPTConfig->_modelName = "gpt-4o-mini";
    chatGPTConfig->_apiKey = std::getenv("chatgpt_apikey");
    ASSERT_FALSE(chatGPTConfig->_apiKey.empty());
    chatGPTConfig->_temperature = 0.7;
    chatGPTConfig->_maxTokens = 2048;

    // gemini-2.5-flash
    auto geminiConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    ASSERT_TRUE(geminiConfig != nullptr);
    geminiConfig->_modelName = "gemini-2.5-flash";
    geminiConfig->_apiKey = std::getenv("gemini_apikey");
    ASSERT_FALSE(geminiConfig->_apiKey.empty());
    geminiConfig->_temperature = 0.7;
    geminiConfig->_maxTokens = 2048;

    // Ollama本地接入deepseek-r1:1.5b
    auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
    ASSERT_TRUE(ollamaConfig != nullptr);
    ollamaConfig->_modelName = "deepseek-r1:1.5b";
    ollamaConfig->_modelDesc = "本地部署deepseek-r1:1.5b模型，采用专家混合架构，专注于深度理解与推理";
    ollamaConfig->_endpoint = "http://192.168.71.22:11434";
    ollamaConfig->_temperature = 0.7;
    ollamaConfig->_maxTokens = 2048;

    std::vector<std::shared_ptr<ai_chat_sdk::Config>> modelConfigs = {
        deepseekConfig, chatGPTConfig, geminiConfig, ollamaConfig};

    sdk->initModels(modelConfigs);

    // 创建会话
    auto sessionId = sdk->createSession(ollamaConfig->_modelName);
    ASSERT_FALSE(sessionId.empty());

    std::string message;
    std::cout << ">>> ";
    std::getline(std::cin, message);
    auto response = sdk->sendMessage(sessionId, message);
    ASSERT_FALSE(response.empty());

    std::cout << ">>> ";
    std::getline(std::cin, message);
    sdk->sendMessage(sessionId, message);
    ASSERT_FALSE(response.empty());

    // 获取会话历史消息
    auto messages = sdk->_sessionManager.getHistoryMessages(sessionId);
    for (const auto &msg : messages)
    {
        std::cout << msg._role << ": " << msg._content << std::endl;
    }
    ASSERT_FALSE(messages.empty());
}

TEST(ChatSDKTest, FullIntegrationTest)
{
    // 1. 实例化 ChatSDK
    auto sdk = std::make_shared<ai_chat_sdk::ChatSDK>();
    ASSERT_TRUE(sdk != nullptr);

    // 2. 准备配置列表
    std::vector<std::shared_ptr<ai_chat_sdk::Config>> configs;

    // (A) DeepSeek 配置
    auto deepseekConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    deepseekConfig->_modelName = "deepseek-chat";
    deepseekConfig->_apiKey = std::getenv("deepseek_apikey");
    ASSERT_TRUE(deepseekConfig->_apiKey.size() > 0) << "DeepSeek API Key not set";
    deepseekConfig->_temperature = 0.7;
    configs.push_back(deepseekConfig);

    // (B) ChatGPT 配置
    auto chatGPTConfig = std::make_shared<ai_chat_sdk::APIConfig>();
    chatGPTConfig->_modelName = "gpt-4o-mini";
    chatGPTConfig->_apiKey = std::getenv("chatgpt_apikey");
    ASSERT_TRUE(chatGPTConfig->_apiKey.size() > 0) << "ChatGPT API Key not set";
    configs.push_back(chatGPTConfig);

    // (C) Ollama 本地配置
    auto ollamaConfig = std::make_shared<ai_chat_sdk::OllamaConfig>();
    ollamaConfig->_modelName = "deepseek-r1:1.5b";
    ollamaConfig->_endpoint = "http://192.168.71.22:11434"; // 本地 Ollama 服务
    configs.push_back(ollamaConfig);

    // 3. 初始化 SDK
    // SDK 内部会自动注册并初始化所有 Provider
    bool initRet = sdk->initModels(configs);
    ASSERT_TRUE(initRet);

    // 4. 开始测试：创建一个 Ollama 会话
    std::string modelName = "deepseek-r1:1.5b";
    std::string sessionId = sdk->createSession(modelName);
    ASSERT_FALSE(sessionId.empty());
    INFO("Created session: {}", sessionId);

    // 5. 发送第一条消息
    std::string q1 = "你好，请做一个简短的自我介绍。";
    std::cout << ">>> User: " << q1 << std::endl;
    std::string a1 = sdk->sendMessage(sessionId, q1);
    ASSERT_FALSE(a1.empty());
    std::cout << ">>> AI: " << a1 << std::endl;

    // 6. 发送第二条消息 (测试上下文记忆)
    std::string q2 = "我刚才让你做什么了？";
    std::cout << ">>> User: " << q2 << std::endl;
    std::string a2 = sdk->sendMessage(sessionId, q2);
    ASSERT_FALSE(a2.empty());
    std::cout << ">>> AI: " << a2 << std::endl;

    // 7. 验证历史记录持久化
    // 注意：为了测试方便，我们将 _sessionManager 设为了 public，但在正式发布时应通过 getter 访问
    auto history = sdk->_sessionManager.getHistoryMessages(sessionId);

    // 应该包含 4 条消息：User1, AI1, User2, AI2
    ASSERT_EQ(history.size(), 4);

    std::cout << "\n=== History Dump ===" << std::endl;
    for (const auto &msg : history)
    {
        std::cout << "[" << msg._role << "] " << msg._content << std::endl;
    }
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