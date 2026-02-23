#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/AnthropicProvider.h"

using namespace ESPAI;

static AnthropicProvider* provider = nullptr;

void setUp() {
    provider = new AnthropicProvider("test-api-key", "claude-sonnet-4-20250514");
}

void tearDown() {
    delete provider;
    provider = nullptr;
}

void test_build_request_basic() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.maxTokens = 100;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"model\":\"claude-sonnet-4-20250514\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"messages\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"Hello\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"max_tokens\":100") != std::string::npos);
}

void test_build_request_max_tokens_required() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.maxTokens = 0;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"max_tokens\":4096") != std::string::npos);
}

void test_build_request_system_as_separate_field() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are helpful"));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"system\":\"You are helpful\"") != std::string::npos);

    size_t systemInMessages = body.find("\"role\":\"system\"");
    TEST_ASSERT_TRUE(systemInMessages == std::string::npos);
}

void test_build_request_system_from_options() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.systemPrompt = "Be concise";

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"system\":\"Be concise\"") != std::string::npos);
}

void test_build_request_options_system_overrides_message() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "From messages"));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.systemPrompt = "From options";

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"system\":\"From options\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"system\":\"From messages\"") == std::string::npos);
}

void test_build_request_escapes_special_chars() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Say \"hello\"\nNew line"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\\\"hello\\\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\\n") != std::string::npos);
}

void test_build_request_custom_model_in_options() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.model = "claude-opus-4-20250514";

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"model\":\"claude-opus-4-20250514\"") != std::string::npos);
}

void test_build_request_multiple_messages() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "First"));
    messages.push_back(Message(Role::Assistant, "Response"));
    messages.push_back(Message(Role::User, "Second"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"assistant\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"First\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"Response\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"Second\"") != std::string::npos);
}

void test_build_request_with_temperature() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.temperature = 0.5f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"temperature\":0.5") != std::string::npos);
}

void test_build_request_without_temperature_when_default() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"temperature\"") == std::string::npos);
}

void test_build_request_with_top_p() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.topP = 0.9f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"top_p\":0.9") != std::string::npos);
}

void test_build_request_without_top_p_when_default() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"top_p\"") == std::string::npos);
}

#if ESPAI_ENABLE_TOOLS
void test_build_request_with_tools() {
    Tool tool;
    tool.name = "get_weather";
    tool.description = "Get current weather";
    tool.parametersJson = "{\"type\":\"object\",\"properties\":{\"location\":{\"type\":\"string\"}}}";

    provider->addTool(tool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather?"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"tools\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_weather\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"description\":\"Get current weather\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"input_schema\"") != std::string::npos);

    provider->clearTools();
}

void test_build_request_without_tools() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"tools\"") == std::string::npos);
}

void test_build_request_tool_result_message() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::Tool, "25 degrees", "toolu_abc123"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"type\":\"tool_result\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"tool_use_id\":\"toolu_abc123\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"25 degrees\"") != std::string::npos);
}

void test_build_request_multiple_tools() {
    Tool tool1;
    tool1.name = "get_weather";
    tool1.description = "Get weather";
    tool1.parametersJson = "{}";

    Tool tool2;
    tool2.name = "get_time";
    tool2.description = "Get time";
    tool2.parametersJson = "{}";

    provider->addTool(tool1);
    provider->addTool(tool2);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"name\":\"get_weather\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_time\"") != std::string::npos);

    provider->clearTools();
}
#endif

void test_build_http_request_url() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("https://api.anthropic.com/v1/messages", req.url.c_str());
}

void test_build_http_request_method() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("POST", req.method.c_str());
}

void test_build_http_request_x_api_key_header() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    bool hasApiKey = false;
    for (const auto& header : req.headers) {
        if (header.first == "x-api-key") {
            TEST_ASSERT_EQUAL_STRING("test-api-key", header.second.c_str());
            hasApiKey = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasApiKey);
}

void test_build_http_request_anthropic_version_header() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    bool hasVersion = false;
    for (const auto& header : req.headers) {
        if (header.first == "anthropic-version") {
            TEST_ASSERT_EQUAL_STRING("2023-06-01", header.second.c_str());
            hasVersion = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasVersion);
}

void test_build_http_request_has_body() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_FALSE(req.body.isEmpty());
    TEST_ASSERT_TRUE(req.body.find("\"model\"") != std::string::npos);
}

void test_build_request_empty_content() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, ""));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"content\":\"\"") != std::string::npos);
}

void test_extract_system_prompt_found() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "Be helpful"));
    messages.push_back(Message(Role::User, "Hello"));

    String system = provider->extractSystemPrompt(messages);

    TEST_ASSERT_EQUAL_STRING("Be helpful", system.c_str());
}

void test_extract_system_prompt_not_found() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    String system = provider->extractSystemPrompt(messages);

    TEST_ASSERT_TRUE(system.isEmpty());
}

void test_extract_system_prompt_first_match() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "First system"));
    messages.push_back(Message(Role::User, "Hello"));
    messages.push_back(Message(Role::System, "Second system"));

    String system = provider->extractSystemPrompt(messages);

    TEST_ASSERT_EQUAL_STRING("First system", system.c_str());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_build_request_basic);
    RUN_TEST(test_build_request_max_tokens_required);
    RUN_TEST(test_build_request_system_as_separate_field);
    RUN_TEST(test_build_request_system_from_options);
    RUN_TEST(test_build_request_options_system_overrides_message);
    RUN_TEST(test_build_request_escapes_special_chars);
    RUN_TEST(test_build_request_custom_model_in_options);
    RUN_TEST(test_build_request_multiple_messages);
    RUN_TEST(test_build_request_with_temperature);
    RUN_TEST(test_build_request_without_temperature_when_default);
    RUN_TEST(test_build_request_with_top_p);
    RUN_TEST(test_build_request_without_top_p_when_default);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_build_request_with_tools);
    RUN_TEST(test_build_request_without_tools);
    RUN_TEST(test_build_request_tool_result_message);
    RUN_TEST(test_build_request_multiple_tools);
#endif

    RUN_TEST(test_build_http_request_url);
    RUN_TEST(test_build_http_request_method);
    RUN_TEST(test_build_http_request_x_api_key_header);
    RUN_TEST(test_build_http_request_anthropic_version_header);
    RUN_TEST(test_build_http_request_has_body);
    RUN_TEST(test_build_request_empty_content);
    RUN_TEST(test_extract_system_prompt_found);
    RUN_TEST(test_extract_system_prompt_not_found);
    RUN_TEST(test_extract_system_prompt_first_match);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
