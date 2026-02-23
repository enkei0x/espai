#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/GeminiProvider.h"

using namespace ESPAI;

static GeminiProvider* provider = nullptr;

void setUp() {
    provider = new GeminiProvider("test-api-key", "gemini-2.5-flash");
}

void tearDown() {
    delete provider;
    provider = nullptr;
}

void test_build_request_basic() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.temperature = 0.7f;
    options.maxTokens = 100;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"contents\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"text\":\"Hello\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"generationConfig\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"temperature\":0.7") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"maxOutputTokens\":100") != std::string::npos);
}

void test_build_request_system_as_instruction() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are helpful"));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"system_instruction\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"text\":\"You are helpful\"") != std::string::npos);

    // System message should not appear in contents array
    TEST_ASSERT_TRUE(body.find("\"role\":\"system\"") == std::string::npos);
}

void test_build_request_system_from_options() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.systemPrompt = "Be concise";

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"system_instruction\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"text\":\"Be concise\"") != std::string::npos);
}

void test_build_request_options_system_overrides_message() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "From messages"));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.systemPrompt = "From options";

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"text\":\"From options\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"text\":\"From messages\"") == std::string::npos);
}

void test_build_request_no_system_instruction_when_absent() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"system_instruction\"") == std::string::npos);
}

void test_build_request_role_mapping_assistant_to_model() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "First"));
    messages.push_back(Message(Role::Assistant, "Response"));
    messages.push_back(Message(Role::User, "Second"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"role\":\"model\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"text\":\"First\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"text\":\"Response\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"text\":\"Second\"") != std::string::npos);
}

void test_build_request_escapes_special_chars() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Say \"hello\"\nNew line"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\\\"hello\\\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\\n") != std::string::npos);
}

void test_build_request_generation_config_temperature() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.temperature = 0.5f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"temperature\":0.5") != std::string::npos);
}

void test_build_request_generation_config_with_top_p() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.topP = 0.9f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"topP\":0.9") != std::string::npos);
}

void test_build_request_without_top_p_when_default() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"topP\"") == std::string::npos);
}

void test_build_request_no_max_tokens_when_zero() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.maxTokens = 0;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"maxOutputTokens\"") == std::string::npos);
}

void test_build_request_frequency_penalty() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.frequencyPenalty = 0.5f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"frequencyPenalty\":0.5") != std::string::npos);
}

void test_build_request_presence_penalty() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.presencePenalty = 0.3f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"presencePenalty\":0.3") != std::string::npos);
}

void test_build_request_no_penalties_when_zero() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.frequencyPenalty = 0.0f;
    options.presencePenalty = 0.0f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"frequencyPenalty\"") == std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"presencePenalty\"") == std::string::npos);
}

void test_build_request_empty_content() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, ""));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"text\":\"\"") != std::string::npos);
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
    TEST_ASSERT_TRUE(body.find("\"functionDeclarations\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_weather\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"description\":\"Get current weather\"") != std::string::npos);

    provider->clearTools();
}

void test_build_request_without_tools() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"tools\"") == std::string::npos);
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

void test_build_request_tool_result_message() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::Tool, "{\"temp\": 25}", "get_weather"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"functionResponse\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_weather\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"response\"") != std::string::npos);
}

void test_build_request_assistant_message_with_tool_calls() {
    Message assistantMsg(Role::Assistant, "");
    assistantMsg.toolCallsJson = R"([{"name":"get_weather","arguments":"{\"location\":\"NYC\"}"}])";

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather?"));
    messages.push_back(assistantMsg);

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"functionCall\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_weather\"") != std::string::npos);
}

void test_get_assistant_message_with_tool_calls() {
    String mockResponse = R"({
        "candidates": [{
            "content": {
                "role": "model",
                "parts": [{
                    "functionCall": {
                        "name": "get_temp",
                        "args": {}
                    }
                }]
            },
            "finishReason": "STOP"
        }],
        "usageMetadata": {"promptTokenCount": 10, "candidatesTokenCount": 5}
    })";

    Response resp = provider->parseResponse(mockResponse);
    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());

    Message msg = provider->getAssistantMessageWithToolCalls("");
    TEST_ASSERT_EQUAL(Role::Assistant, msg.role);
    TEST_ASSERT_TRUE(msg.hasToolCalls());
    TEST_ASSERT_TRUE(msg.toolCallsJson.find("get_temp") != std::string::npos);
}

void test_full_tool_calling_flow() {
    String mockResponse = R"({
        "candidates": [{
            "content": {
                "role": "model",
                "parts": [
                    {
                        "functionCall": {
                            "name": "get_temperature",
                            "args": {}
                        }
                    },
                    {
                        "functionCall": {
                            "name": "get_humidity",
                            "args": {}
                        }
                    }
                ]
            },
            "finishReason": "STOP"
        }],
        "usageMetadata": {"promptTokenCount": 10, "candidatesTokenCount": 5}
    })";

    Response resp = provider->parseResponse(mockResponse);
    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());
    TEST_ASSERT_EQUAL(2, provider->getLastToolCalls().size());

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the temperature and humidity?"));

    Message assistantMsg = provider->getAssistantMessageWithToolCalls(resp.content);
    messages.push_back(assistantMsg);
    TEST_ASSERT_TRUE(assistantMsg.hasToolCalls());

    const auto& toolCalls = provider->getLastToolCalls();
    for (const auto& call : toolCalls) {
        String result;
        if (call.name == "get_temperature") {
            result = "{\"temperature\": 23.5, \"unit\": \"celsius\"}";
        } else {
            result = "{\"humidity\": 65.0, \"unit\": \"percent\"}";
        }
        Message toolMsg(Role::Tool, result, call.id);
        messages.push_back(toolMsg);
    }

    provider->clearTools();

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"functionCall\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"functionResponse\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_temperature\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_humidity\"") != std::string::npos);

    size_t userPos = body.find("\"role\":\"user\"");
    size_t modelPos = body.find("\"role\":\"model\"");
    TEST_ASSERT_TRUE(userPos < modelPos);
}
#endif

void test_build_http_request_url() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_TRUE(req.url.find("generativelanguage.googleapis.com") != std::string::npos);
    TEST_ASSERT_TRUE(req.url.find("models/gemini-2.5-flash") != std::string::npos);
    TEST_ASSERT_TRUE(req.url.find("generateContent") != std::string::npos);
    TEST_ASSERT_TRUE(req.url.find("streamGenerateContent") == std::string::npos);
}

void test_build_http_request_method() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("POST", req.method.c_str());
}

void test_build_http_request_api_key_header() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    bool hasApiKey = false;
    for (const auto& header : req.headers) {
        if (header.first == "x-goog-api-key") {
            TEST_ASSERT_EQUAL_STRING("test-api-key", header.second.c_str());
            hasApiKey = true;
            break;
        }
    }
    TEST_ASSERT_TRUE(hasApiKey);
}

void test_build_http_request_has_body() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_FALSE(req.body.isEmpty());
    TEST_ASSERT_TRUE(req.body.find("\"contents\"") != std::string::npos);
}

void test_build_http_request_custom_model_in_options() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.model = "gemini-1.5-pro";

    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_TRUE(req.url.find("models/gemini-1.5-pro") != std::string::npos);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_build_request_basic);
    RUN_TEST(test_build_request_system_as_instruction);
    RUN_TEST(test_build_request_system_from_options);
    RUN_TEST(test_build_request_options_system_overrides_message);
    RUN_TEST(test_build_request_no_system_instruction_when_absent);
    RUN_TEST(test_build_request_role_mapping_assistant_to_model);
    RUN_TEST(test_build_request_escapes_special_chars);
    RUN_TEST(test_build_request_generation_config_temperature);
    RUN_TEST(test_build_request_generation_config_with_top_p);
    RUN_TEST(test_build_request_without_top_p_when_default);
    RUN_TEST(test_build_request_no_max_tokens_when_zero);
    RUN_TEST(test_build_request_frequency_penalty);
    RUN_TEST(test_build_request_presence_penalty);
    RUN_TEST(test_build_request_no_penalties_when_zero);
    RUN_TEST(test_build_request_empty_content);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_build_request_with_tools);
    RUN_TEST(test_build_request_without_tools);
    RUN_TEST(test_build_request_multiple_tools);
    RUN_TEST(test_build_request_tool_result_message);
    RUN_TEST(test_build_request_assistant_message_with_tool_calls);
    RUN_TEST(test_get_assistant_message_with_tool_calls);
    RUN_TEST(test_full_tool_calling_flow);
#endif

    RUN_TEST(test_build_http_request_url);
    RUN_TEST(test_build_http_request_method);
    RUN_TEST(test_build_http_request_api_key_header);
    RUN_TEST(test_build_http_request_has_body);
    RUN_TEST(test_build_http_request_custom_model_in_options);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
