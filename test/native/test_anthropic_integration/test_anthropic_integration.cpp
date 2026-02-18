#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/AnthropicProvider.h"
#include "providers/ProviderFactory.h"
#include <ArduinoJson.h>

using namespace ESPAI;

// Helper function to build tool schema JSON using ArduinoJson
String buildToolSchema(const char* propName, const char* propType, const char* description = nullptr, bool required = false) {
    JsonDocument doc;
    doc["type"] = "object";

    auto props = doc["properties"].to<JsonObject>();
    auto prop = props[propName].to<JsonObject>();
    prop["type"] = propType;
    if (description) {
        prop["description"] = description;
    }

    if (required) {
        auto req = doc["required"].to<JsonArray>();
        req.add(propName);
    }

    String output;
    serializeJson(doc, output);
    return output;
}

void setUp() {
    ProviderFactory::unregisterProvider(Provider::Anthropic);
}

void tearDown() {
}

void test_factory_registration() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);
    TEST_ASSERT_TRUE(ProviderFactory::isRegistered(Provider::Anthropic));
}

void test_factory_creates_anthropic_provider() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key", "claude-sonnet-4-20250514");

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING("Anthropic", provider->getName());
    TEST_ASSERT_EQUAL(Provider::Anthropic, provider->getType());
}

void test_factory_uses_default_model() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING(ESPAI_DEFAULT_MODEL_ANTHROPIC, provider->getModel().c_str());
}

void test_factory_sets_custom_base_url() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(
        Provider::Anthropic,
        "test-key",
        "claude-sonnet-4-20250514",
        "https://custom.anthropic.proxy.com/v1/messages"
    );

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING(
        "https://custom.anthropic.proxy.com/v1/messages",
        provider->getBaseUrl().c_str()
    );
}

void test_provider_supports_tools() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");

    TEST_ASSERT_TRUE(provider->supportsTools());
}

void test_provider_supports_streaming() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");

    TEST_ASSERT_TRUE(provider->supportsStreaming());
}

void test_provider_is_configured_with_key() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "sk-ant-test123");

    TEST_ASSERT_TRUE(provider->isConfigured());
}

void test_provider_not_configured_without_key() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "");

    TEST_ASSERT_FALSE(provider->isConfigured());
}

void test_provider_timeout_setting() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");
    provider->setTimeout(60000);

    TEST_ASSERT_EQUAL(60000, provider->getTimeout());
}

void test_full_request_response_cycle() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key", "claude-sonnet-4-20250514");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are helpful"));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.temperature = 0.5f;
    options.maxTokens = 150;

    HttpRequest req = anthropicProvider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("POST", req.method.c_str());
    TEST_ASSERT_TRUE(req.body.find("\"model\":\"claude-sonnet-4-20250514\"") != std::string::npos);
    TEST_ASSERT_TRUE(req.body.find("\"system\":\"You are helpful\"") != std::string::npos);
    TEST_ASSERT_TRUE(req.body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(req.body.find("\"max_tokens\":150") != std::string::npos);

    bool hasXApiKey = false;
    bool hasVersion = false;
    for (const auto& header : req.headers) {
        if (header.first == "x-api-key") hasXApiKey = true;
        if (header.first == "anthropic-version") hasVersion = true;
    }
    TEST_ASSERT_TRUE(hasXApiKey);
    TEST_ASSERT_TRUE(hasVersion);

    String mockResponse = R"({
        "id": "msg_test",
        "content": [
            {"type": "text", "text": "Hello! How can I assist you today?"}
        ],
        "usage": {
            "input_tokens": 15,
            "output_tokens": 8
        }
    })";

    Response resp = anthropicProvider->parseResponse(mockResponse);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Hello! How can I assist you today?", resp.content.c_str());
    TEST_ASSERT_EQUAL(15, resp.promptTokens);
    TEST_ASSERT_EQUAL(8, resp.completionTokens);
}

#if ESPAI_ENABLE_TOOLS
void test_tool_calling_cycle() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    Tool tool;
    tool.name = "get_current_weather";
    tool.description = "Get the current weather in a location";
    tool.parametersJson = buildToolSchema("location", "string", nullptr, true);

    anthropicProvider->addTool(tool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather in Tokyo?"));

    ChatOptions options;
    HttpRequest req = anthropicProvider->buildHttpRequest(messages, options);

    TEST_ASSERT_TRUE(req.body.find("\"tools\"") != std::string::npos);
    TEST_ASSERT_TRUE(req.body.find("\"get_current_weather\"") != std::string::npos);
    TEST_ASSERT_TRUE(req.body.find("\"input_schema\"") != std::string::npos);

    String toolUseResponse = R"({
        "content": [
            {
                "type": "text",
                "text": "I'll check the weather for you."
            },
            {
                "type": "tool_use",
                "id": "toolu_xyz789",
                "name": "get_current_weather",
                "input": {"location": "Tokyo"}
            }
        ],
        "stop_reason": "tool_use"
    })";

    Response resp1 = anthropicProvider->parseResponse(toolUseResponse);

    TEST_ASSERT_TRUE(resp1.success);
    TEST_ASSERT_TRUE(anthropicProvider->hasToolCalls());

    const auto& calls = anthropicProvider->getLastToolCalls();
    TEST_ASSERT_EQUAL(1, calls.size());
    TEST_ASSERT_EQUAL_STRING("toolu_xyz789", calls[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_current_weather", calls[0].name.c_str());

    std::vector<Message> messages2;
    messages2.push_back(Message(Role::User, "What is the weather in Tokyo?"));
    messages2.push_back(Message(Role::Tool, "25 degrees Celsius, sunny", "toolu_xyz789"));

    HttpRequest req2 = anthropicProvider->buildHttpRequest(messages2, options);

    TEST_ASSERT_TRUE(req2.body.find("\"type\":\"tool_result\"") != std::string::npos);
    TEST_ASSERT_TRUE(req2.body.find("\"tool_use_id\":\"toolu_xyz789\"") != std::string::npos);

    anthropicProvider->clearTools();
}
#endif

#if ESPAI_ENABLE_STREAMING
void test_streaming_parsing_cycle() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    String accumulated;

    std::vector<String> chunks = {
        "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"content_block\":{\"type\":\"text\",\"text\":\"\"}}",
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Hello\"}}",
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\" there\"}}",
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"!\"}}",
        "event: message_stop\ndata: {\"type\":\"message_stop\"}"
    };

    for (const auto& chunk : chunks) {
        String content;
        bool done = false;

        bool result = anthropicProvider->parseStreamChunk(chunk, content, done);
        TEST_ASSERT_TRUE(result);

        if (done) {
            break;
        }

        accumulated += content;
    }

    TEST_ASSERT_EQUAL_STRING("Hello there!", accumulated.c_str());
}
#endif

void test_error_handling_auth_error() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "invalid-key");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    String errorResponse = R"({
        "error": {
            "type": "authentication_error",
            "message": "Invalid API key"
        }
    })";

    Response resp = anthropicProvider->parseResponse(errorResponse);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ServerError, resp.error);
    TEST_ASSERT_TRUE(resp.errorMessage.find("Invalid API key") != std::string::npos);
}

void test_error_handling_rate_limit() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    String errorResponse = R"({
        "error": {
            "type": "rate_limit_error",
            "message": "Rate limit exceeded"
        }
    })";

    Response resp = anthropicProvider->parseResponse(errorResponse);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Rate limit exceeded", resp.errorMessage.c_str());
}

void test_model_override_in_options() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key", "claude-sonnet-4-20250514");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.model = "claude-opus-4-20250514";

    String body = anthropicProvider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"model\":\"claude-opus-4-20250514\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("claude-sonnet-4-20250514") == std::string::npos);
}

void test_api_key_in_header() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "sk-ant-secret123");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = anthropicProvider->buildHttpRequest(messages, options);

    bool foundApiKey = false;
    for (const auto& header : req.headers) {
        if (header.first == "x-api-key") {
            TEST_ASSERT_EQUAL_STRING("sk-ant-secret123", header.second.c_str());
            foundApiKey = true;
        }
    }

    TEST_ASSERT_TRUE(foundApiKey);
}

void test_anthropic_version_header() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = anthropicProvider->buildHttpRequest(messages, options);

    bool foundVersion = false;
    for (const auto& header : req.headers) {
        if (header.first == "anthropic-version") {
            TEST_ASSERT_EQUAL_STRING("2023-06-01", header.second.c_str());
            foundVersion = true;
        }
    }

    TEST_ASSERT_TRUE(foundVersion);
}

void test_custom_api_version() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    anthropicProvider->setApiVersion("2024-01-01");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = anthropicProvider->buildHttpRequest(messages, options);

    bool foundVersion = false;
    for (const auto& header : req.headers) {
        if (header.first == "anthropic-version") {
            TEST_ASSERT_EQUAL_STRING("2024-01-01", header.second.c_str());
            foundVersion = true;
        }
    }

    TEST_ASSERT_TRUE(foundVersion);
}

void test_system_not_in_messages_array() {
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);

    auto provider = ProviderFactory::create(Provider::Anthropic, "test-key");
    auto* anthropicProvider = static_cast<AnthropicProvider*>(provider.get());

    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "Be helpful"));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    String body = anthropicProvider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"system\":\"Be helpful\"") != std::string::npos);

    size_t messagesStart = body.find("\"messages\":[");
    size_t messagesEnd = body.find("]", messagesStart);
    String messagesSection = body.substr(messagesStart, messagesEnd - messagesStart);

    TEST_ASSERT_TRUE(messagesSection.find("\"role\":\"system\"") == std::string::npos);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_factory_registration);
    RUN_TEST(test_factory_creates_anthropic_provider);
    RUN_TEST(test_factory_uses_default_model);
    RUN_TEST(test_factory_sets_custom_base_url);
    RUN_TEST(test_provider_supports_tools);
    RUN_TEST(test_provider_supports_streaming);
    RUN_TEST(test_provider_is_configured_with_key);
    RUN_TEST(test_provider_not_configured_without_key);
    RUN_TEST(test_provider_timeout_setting);
    RUN_TEST(test_full_request_response_cycle);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_tool_calling_cycle);
#endif

#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_streaming_parsing_cycle);
#endif

    RUN_TEST(test_error_handling_auth_error);
    RUN_TEST(test_error_handling_rate_limit);
    RUN_TEST(test_model_override_in_options);
    RUN_TEST(test_api_key_in_header);
    RUN_TEST(test_anthropic_version_header);
    RUN_TEST(test_custom_api_version);
    RUN_TEST(test_system_not_in_messages_array);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
