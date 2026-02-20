#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/OpenAIProvider.h"
#include "providers/ProviderFactory.h"
#include "http/SSEParser.h"
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
    ProviderFactory::unregisterProvider(Provider::OpenAI);
}

void tearDown() {
}

void test_factory_registration() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);
    TEST_ASSERT_TRUE(ProviderFactory::isRegistered(Provider::OpenAI));
}

void test_factory_creates_openai_provider() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key", "gpt-4o");

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING("OpenAI", provider->getName());
    TEST_ASSERT_EQUAL(Provider::OpenAI, provider->getType());
}

void test_factory_uses_default_model() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key");

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING(ESPAI_DEFAULT_MODEL_OPENAI, provider->getModel().c_str());
}

void test_factory_sets_custom_base_url() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(
        Provider::OpenAI,
        "test-key",
        "gpt-4o",
        "https://custom.openai.proxy.com/v1/chat/completions"
    );

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING(
        "https://custom.openai.proxy.com/v1/chat/completions",
        provider->getBaseUrl().c_str()
    );
}

void test_provider_supports_tools() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key");

    TEST_ASSERT_TRUE(provider->supportsTools());
}

void test_provider_supports_streaming() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key");

    TEST_ASSERT_TRUE(provider->supportsStreaming());
}

void test_provider_is_configured_with_key() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "sk-test123");

    TEST_ASSERT_TRUE(provider->isConfigured());
}

void test_provider_not_configured_without_key() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "");

    TEST_ASSERT_FALSE(provider->isConfigured());
}

void test_provider_timeout_setting() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key");
    provider->setTimeout(60000);

    TEST_ASSERT_EQUAL(60000, provider->getTimeout());
}

void test_full_request_response_cycle() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key", "gpt-4o-mini");
    auto* openaiProvider = static_cast<OpenAIProvider*>(provider.get());

    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are helpful"));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.temperature = 0.7f;
    options.maxTokens = 150;

    HttpRequest req = openaiProvider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("POST", req.method.c_str());
    TEST_ASSERT_TRUE(req.body.find("\"model\":\"gpt-4o-mini\"") != std::string::npos);
    TEST_ASSERT_TRUE(req.body.find("\"role\":\"system\"") != std::string::npos);
    TEST_ASSERT_TRUE(req.body.find("\"role\":\"user\"") != std::string::npos);

    String mockResponse = R"({
        "choices": [{
            "message": {
                "content": "Hello! How can I assist you today?"
            }
        }],
        "usage": {
            "prompt_tokens": 15,
            "completion_tokens": 8
        }
    })";

    Response resp = openaiProvider->parseResponse(mockResponse);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Hello! How can I assist you today?", resp.content.c_str());
    TEST_ASSERT_EQUAL(15, resp.promptTokens);
    TEST_ASSERT_EQUAL(8, resp.completionTokens);
}

#if ESPAI_ENABLE_TOOLS
void test_tool_calling_cycle() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key");
    auto* openaiProvider = static_cast<OpenAIProvider*>(provider.get());

    Tool tool;
    tool.name = "get_current_weather";
    tool.description = "Get the current weather in a location";
    tool.parametersJson = buildToolSchema("location", "string", nullptr, true);

    openaiProvider->addTool(tool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather in Tokyo?"));

    ChatOptions options;
    HttpRequest req = openaiProvider->buildHttpRequest(messages, options);

    TEST_ASSERT_TRUE(req.body.find("\"tools\"") != std::string::npos);
    TEST_ASSERT_TRUE(req.body.find("\"get_current_weather\"") != std::string::npos);

    String toolCallResponse = R"({
        "choices": [{
            "message": {
                "content": null,
                "tool_calls": [{
                    "id": "call_xyz789",
                    "type": "function",
                    "function": {
                        "name": "get_current_weather",
                        "arguments": "{\"location\":\"Tokyo\"}"
                    }
                }]
            },
            "finish_reason": "tool_calls"
        }]
    })";

    Response resp1 = openaiProvider->parseResponse(toolCallResponse);

    TEST_ASSERT_TRUE(resp1.success);
    TEST_ASSERT_TRUE(openaiProvider->hasToolCalls());

    const auto& calls = openaiProvider->getLastToolCalls();
    TEST_ASSERT_EQUAL(1, calls.size());
    TEST_ASSERT_EQUAL_STRING("call_xyz789", calls[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_current_weather", calls[0].name.c_str());

    std::vector<Message> messages2;
    messages2.push_back(Message(Role::User, "What is the weather in Tokyo?"));
    messages2.push_back(Message(Role::Tool, "25 degrees Celsius, sunny", "call_xyz789"));

    HttpRequest req2 = openaiProvider->buildHttpRequest(messages2, options);

    TEST_ASSERT_TRUE(req2.body.find("\"role\":\"tool\"") != std::string::npos);
    TEST_ASSERT_TRUE(req2.body.find("\"tool_call_id\":\"call_xyz789\"") != std::string::npos);

    openaiProvider->clearTools();
}
#endif

#if ESPAI_ENABLE_STREAMING
void test_streaming_parsing_cycle() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key");
    TEST_ASSERT_EQUAL(SSEFormat::OpenAI, provider->getSSEFormat());

    SSEParser parser(SSEFormat::OpenAI);
    String accumulated;
    bool isDone = false;

    parser.setContentCallback([&](const String& content, bool done) {
        if (!content.isEmpty()) accumulated += content;
        if (done) isDone = true;
    });

    parser.feed("data: {\"choices\":[{\"delta\":{\"role\":\"assistant\"}}]}\n");
    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"}}]}\n");
    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\" there\"}}]}\n");
    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\"!\"}}]}\n");
    parser.feed("data: [DONE]\n");

    TEST_ASSERT_TRUE(isDone);
    TEST_ASSERT_EQUAL_STRING("Hello there!", accumulated.c_str());
}
#endif

void test_error_handling_auth_error() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "invalid-key");
    auto* openaiProvider = static_cast<OpenAIProvider*>(provider.get());

    String errorResponse = R"({
        "error": {
            "message": "Incorrect API key provided",
            "type": "invalid_request_error",
            "code": "invalid_api_key"
        }
    })";

    Response resp = openaiProvider->parseResponse(errorResponse);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ServerError, resp.error);
    TEST_ASSERT_TRUE(resp.errorMessage.find("Incorrect API key") != std::string::npos);
}

void test_error_handling_rate_limit() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key");
    auto* openaiProvider = static_cast<OpenAIProvider*>(provider.get());

    String errorResponse = R"({
        "error": {
            "message": "Rate limit exceeded",
            "type": "rate_limit_error"
        }
    })";

    Response resp = openaiProvider->parseResponse(errorResponse);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Rate limit exceeded", resp.errorMessage.c_str());
}

void test_model_override_in_options() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "test-key", "gpt-4o-mini");
    auto* openaiProvider = static_cast<OpenAIProvider*>(provider.get());

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.model = "gpt-4o";

    String body = openaiProvider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"model\":\"gpt-4o\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("gpt-4o-mini") == std::string::npos);
}

void test_api_key_in_header() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

    auto provider = ProviderFactory::create(Provider::OpenAI, "sk-secretkey123");
    auto* openaiProvider = static_cast<OpenAIProvider*>(provider.get());

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = openaiProvider->buildHttpRequest(messages, options);

    bool foundAuth = false;
    for (const auto& header : req.headers) {
        if (header.first == "Authorization") {
            TEST_ASSERT_EQUAL_STRING("Bearer sk-secretkey123", header.second.c_str());
            foundAuth = true;
        }
    }

    TEST_ASSERT_TRUE(foundAuth);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_factory_registration);
    RUN_TEST(test_factory_creates_openai_provider);
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

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
