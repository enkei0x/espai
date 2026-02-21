#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/OpenAICompatibleProvider.h"

using namespace ESPAI;

class TestProviderWithCustomHeaders : public OpenAICompatibleProvider {
public:
    using OpenAICompatibleProvider::OpenAICompatibleProvider;

protected:
    void addProviderHeaders(HttpRequest& req) override {
        addHeader(req, "X-Provider-Custom", "injected-by-subclass");
    }
};

static OpenAICompatibleProvider* provider = nullptr;

void setUp() {
    OpenAICompatibleConfig config;
    config.name = "TestProvider";
    config.baseUrl = "https://api.example.com/v1/chat/completions";
    config.apiKey = "test-api-key";
    config.model = "test-model";
    config.providerType = Provider::Custom;
    provider = new OpenAICompatibleProvider(config);
}

void tearDown() {
    delete provider;
    provider = nullptr;
}


void test_config_defaults() {
    OpenAICompatibleConfig config;
    TEST_ASSERT_EQUAL_STRING("Authorization", config.authHeaderName.c_str());
    TEST_ASSERT_EQUAL_STRING("Bearer ", config.authHeaderValuePrefix.c_str());
    TEST_ASSERT_TRUE(config.requiresApiKey);
    TEST_ASSERT_TRUE(config.toolCallingSupported);
    TEST_ASSERT_EQUAL(Provider::Custom, config.providerType);
    TEST_ASSERT_EQUAL_STRING("Custom", config.name.c_str());
}

void test_custom_config() {
    OpenAICompatibleConfig config;
    config.name = "MyProvider";
    config.baseUrl = "https://my.api.com/chat";
    config.apiKey = "my-key";
    config.model = "my-model";
    config.authHeaderName = "X-API-Key";
    config.authHeaderValuePrefix = "";
    config.requiresApiKey = true;
    config.toolCallingSupported = false;
    config.providerType = Provider::Custom;

    OpenAICompatibleProvider p(config);
    TEST_ASSERT_EQUAL_STRING("MyProvider", p.getName());
    TEST_ASSERT_EQUAL(Provider::Custom, p.getType());
    TEST_ASSERT_EQUAL_STRING("my-key", p.getApiKey().c_str());
    TEST_ASSERT_EQUAL_STRING("my-model", p.getModel().c_str());
    TEST_ASSERT_EQUAL_STRING("https://my.api.com/chat", p.getBaseUrl().c_str());
    TEST_ASSERT_FALSE(p.supportsTools());
}


void test_is_configured_with_api_key_and_model() {
    TEST_ASSERT_TRUE(provider->isConfigured());
}

void test_is_configured_requires_api_key_when_set() {
    OpenAICompatibleConfig config;
    config.apiKey = "";
    config.model = "test-model";
    config.requiresApiKey = true;

    OpenAICompatibleProvider p(config);
    TEST_ASSERT_FALSE(p.isConfigured());
}

void test_is_configured_no_api_key_required() {
    OpenAICompatibleConfig config;
    config.apiKey = "";
    config.model = "test-model";
    config.requiresApiKey = false;

    OpenAICompatibleProvider p(config);
    TEST_ASSERT_TRUE(p.isConfigured());
}

void test_is_configured_requires_model() {
    OpenAICompatibleConfig config;
    config.apiKey = "key";
    config.model = "";
    config.requiresApiKey = true;

    OpenAICompatibleProvider p(config);
    TEST_ASSERT_FALSE(p.isConfigured());
}


void test_custom_headers_in_http_request() {
    provider->addCustomHeader("X-Custom-Header", "custom-value");
    provider->addCustomHeader("X-Another", "another-value");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = provider->buildHttpRequest(messages, options);

    bool hasCustom = false;
    bool hasAnother = false;
    for (const auto& header : req.headers) {
        if (header.first == "X-Custom-Header" && header.second == "custom-value") {
            hasCustom = true;
        }
        if (header.first == "X-Another" && header.second == "another-value") {
            hasAnother = true;
        }
    }
    TEST_ASSERT_TRUE(hasCustom);
    TEST_ASSERT_TRUE(hasAnother);
}


void test_auth_header_present() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = provider->buildHttpRequest(messages, options);

    bool hasAuth = false;
    for (const auto& header : req.headers) {
        if (header.first == "Authorization") {
            TEST_ASSERT_EQUAL_STRING("Bearer test-api-key", header.second.c_str());
            hasAuth = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasAuth);
}

void test_no_auth_header_when_auth_header_name_empty() {
    OpenAICompatibleConfig config;
    config.apiKey = "some-key";
    config.model = "test-model";
    config.authHeaderName = "";

    OpenAICompatibleProvider p(config);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = p.buildHttpRequest(messages, options);

    for (const auto& header : req.headers) {
        TEST_ASSERT_TRUE(header.first != "Authorization");
    }
}

void test_no_auth_header_when_api_key_empty() {
    OpenAICompatibleConfig config;
    config.apiKey = "";
    config.model = "test-model";
    config.requiresApiKey = false;

    OpenAICompatibleProvider p(config);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = p.buildHttpRequest(messages, options);

    for (const auto& header : req.headers) {
        TEST_ASSERT_TRUE(header.first != "Authorization");
    }
}

void test_custom_auth_header_name() {
    OpenAICompatibleConfig config;
    config.apiKey = "my-secret";
    config.model = "test-model";
    config.authHeaderName = "X-API-Key";
    config.authHeaderValuePrefix = "";

    OpenAICompatibleProvider p(config);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = p.buildHttpRequest(messages, options);

    bool hasCustomAuth = false;
    for (const auto& header : req.headers) {
        if (header.first == "X-API-Key") {
            TEST_ASSERT_EQUAL_STRING("my-secret", header.second.c_str());
            hasCustomAuth = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasCustomAuth);
}


void test_build_request_body_openai_format() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.temperature = 0.7f;
    options.maxTokens = 100;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"model\":\"test-model\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"messages\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"Hello\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"temperature\":0.7") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"max_tokens\":100") != std::string::npos);
}

void test_build_request_body_multiple_messages() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are helpful"));
    messages.push_back(Message(Role::User, "Hi"));
    messages.push_back(Message(Role::Assistant, "Hello!"));
    messages.push_back(Message(Role::User, "How are you?"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"role\":\"system\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"assistant\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"You are helpful\"") != std::string::npos);
}


void test_parse_response_openai_format() {
    String json = R"({
        "choices": [{
            "message": {
                "role": "assistant",
                "content": "Hello! How can I help?"
            }
        }],
        "usage": {
            "prompt_tokens": 10,
            "completion_tokens": 8
        }
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Hello! How can I help?", resp.content.c_str());
    TEST_ASSERT_EQUAL(10, resp.promptTokens);
    TEST_ASSERT_EQUAL(8, resp.completionTokens);
}

void test_parse_response_error() {
    String json = R"({
        "error": {
            "message": "Invalid API key"
        }
    })";

    Response resp = provider->parseResponse(json);
    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ServerError, resp.error);
    TEST_ASSERT_EQUAL_STRING("Invalid API key", resp.errorMessage.c_str());
}


#if ESPAI_ENABLE_TOOLS
void test_tool_calling_supported_true_includes_tools() {
    OpenAICompatibleConfig config;
    config.apiKey = "key";
    config.model = "model";
    config.toolCallingSupported = true;

    OpenAICompatibleProvider p(config);

    Tool tool;
    tool.name = "get_weather";
    tool.description = "Get weather";
    tool.parametersJson = "{}";
    p.addTool(tool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Weather?"));
    ChatOptions options;

    String body = p.buildRequestBody(messages, options);
    TEST_ASSERT_TRUE(body.find("\"tools\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_weather\"") != std::string::npos);
}

void test_tool_calling_supported_false_skips_tools() {
    OpenAICompatibleConfig config;
    config.apiKey = "key";
    config.model = "model";
    config.toolCallingSupported = false;

    OpenAICompatibleProvider p(config);

    Tool tool;
    tool.name = "get_weather";
    tool.description = "Get weather";
    tool.parametersJson = "{}";
    p.addTool(tool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Weather?"));
    ChatOptions options;

    String body = p.buildRequestBody(messages, options);
    TEST_ASSERT_TRUE(body.find("\"tools\"") == std::string::npos);
}
#endif


void test_legacy_constructor() {
    OpenAICompatibleProvider p("api-key", "model-v1", "https://api.test.com/chat", "LegacyProv", Provider::Custom);

    TEST_ASSERT_EQUAL_STRING("LegacyProv", p.getName());
    TEST_ASSERT_EQUAL(Provider::Custom, p.getType());
    TEST_ASSERT_EQUAL_STRING("api-key", p.getApiKey().c_str());
    TEST_ASSERT_EQUAL_STRING("model-v1", p.getModel().c_str());
    TEST_ASSERT_EQUAL_STRING("https://api.test.com/chat", p.getBaseUrl().c_str());
    TEST_ASSERT_TRUE(p.isConfigured());
}


void test_http_request_url_and_method() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("https://api.example.com/v1/chat/completions", req.url.c_str());
    TEST_ASSERT_EQUAL_STRING("POST", req.method.c_str());
    TEST_ASSERT_FALSE(req.body.isEmpty());
}


void test_add_provider_headers_override() {
    OpenAICompatibleConfig config;
    config.name = "CustomHdrProv";
    config.baseUrl = "https://api.example.com/v1/chat/completions";
    config.apiKey = "key";
    config.model = "model";

    TestProviderWithCustomHeaders p(config);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = p.buildHttpRequest(messages, options);

    bool hasProviderHeader = false;
    for (const auto& header : req.headers) {
        if (header.first == "X-Provider-Custom" && header.second == "injected-by-subclass") {
            hasProviderHeader = true;
            break;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(hasProviderHeader,
        "addProviderHeaders() override should inject X-Provider-Custom header");
}


void test_set_base_url_post_construction() {
    OpenAICompatibleConfig config;
    config.name = "TestProv";
    config.baseUrl = "https://original.example.com/v1/chat/completions";
    config.apiKey = "key";
    config.model = "model";

    OpenAICompatibleProvider p(config);

    p.setBaseUrl("https://changed.example.com/v1/chat/completions");

    TEST_ASSERT_EQUAL_STRING("https://changed.example.com/v1/chat/completions",
        p.getBaseUrl().c_str());

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = p.buildHttpRequest(messages, options);
    TEST_ASSERT_EQUAL_STRING("https://changed.example.com/v1/chat/completions",
        req.url.c_str());
}

void test_set_api_key_post_construction() {
    OpenAICompatibleConfig config;
    config.name = "TestProv";
    config.baseUrl = "https://api.example.com/v1/chat/completions";
    config.apiKey = "original-key";
    config.model = "model";

    OpenAICompatibleProvider p(config);

    p.setApiKey("new-api-key");

    TEST_ASSERT_EQUAL_STRING("new-api-key", p.getApiKey().c_str());

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = p.buildHttpRequest(messages, options);

    bool foundCorrectAuth = false;
    for (const auto& header : req.headers) {
        if (header.first == "Authorization") {
            TEST_ASSERT_EQUAL_STRING("Bearer new-api-key", header.second.c_str());
            foundCorrectAuth = true;
            break;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(foundCorrectAuth,
        "Auth header should use the updated API key");
}

#if ESPAI_ENABLE_STREAMING
void test_sse_format_is_openai() {
    TEST_ASSERT_EQUAL(SSEFormat::OpenAI, provider->getSSEFormat());
}
#endif

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_config_defaults);
    RUN_TEST(test_custom_config);

    RUN_TEST(test_is_configured_with_api_key_and_model);
    RUN_TEST(test_is_configured_requires_api_key_when_set);
    RUN_TEST(test_is_configured_no_api_key_required);
    RUN_TEST(test_is_configured_requires_model);

    RUN_TEST(test_custom_headers_in_http_request);

    RUN_TEST(test_auth_header_present);
    RUN_TEST(test_no_auth_header_when_auth_header_name_empty);
    RUN_TEST(test_no_auth_header_when_api_key_empty);
    RUN_TEST(test_custom_auth_header_name);

    RUN_TEST(test_build_request_body_openai_format);
    RUN_TEST(test_build_request_body_multiple_messages);

    RUN_TEST(test_parse_response_openai_format);
    RUN_TEST(test_parse_response_error);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_tool_calling_supported_true_includes_tools);
    RUN_TEST(test_tool_calling_supported_false_skips_tools);
#endif

    RUN_TEST(test_legacy_constructor);
    RUN_TEST(test_http_request_url_and_method);

    RUN_TEST(test_add_provider_headers_override);
    RUN_TEST(test_set_base_url_post_construction);
    RUN_TEST(test_set_api_key_post_construction);

#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_sse_format_is_openai);
#endif

    return UNITY_END();
}

#else
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
