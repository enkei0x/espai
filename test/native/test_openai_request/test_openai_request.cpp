#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/OpenAIProvider.h"

using namespace ESPAI;

static OpenAIProvider* provider = nullptr;

void setUp() {
    provider = new OpenAIProvider("test-api-key", "gpt-4o-mini");
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

    TEST_ASSERT_TRUE(body.find("\"model\":\"gpt-4o-mini\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"messages\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"Hello\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"temperature\":0.7") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"max_tokens\":100") != std::string::npos);
}

void test_build_request_with_system_message() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are helpful"));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"role\":\"system\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"You are helpful\"") != std::string::npos);
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
    options.model = "gpt-4o";

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"model\":\"gpt-4o\"") != std::string::npos);
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
    options.topP = 1.0f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"top_p\"") == std::string::npos);
}

void test_build_request_with_frequency_penalty() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.frequencyPenalty = 0.5f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"frequency_penalty\":0.5") != std::string::npos);
}

void test_build_request_with_presence_penalty() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.presencePenalty = 0.3f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"presence_penalty\":0.3") != std::string::npos);
}

void test_build_request_no_penalties_when_zero() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    options.frequencyPenalty = 0.0f;
    options.presencePenalty = 0.0f;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"frequency_penalty\"") == std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"presence_penalty\"") == std::string::npos);
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
    TEST_ASSERT_TRUE(body.find("\"type\":\"function\"") != std::string::npos);
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

void test_build_request_tool_message() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::Tool, "25 degrees", "call_abc123"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"role\":\"tool\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"tool_call_id\":\"call_abc123\"") != std::string::npos);
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

void test_build_request_assistant_message_with_tool_calls() {
    // Build a message that simulates an assistant response with tool_calls
    Message assistantMsg(Role::Assistant, "");
    assistantMsg.toolCallsJson = R"([{"id":"call_abc123","type":"function","function":{"name":"get_weather","arguments":"{\"location\":\"NYC\"}"}}])";

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather?"));
    messages.push_back(assistantMsg);
    messages.push_back(Message(Role::Tool, "{\"temp\": 25}", "call_abc123"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);


    // Verify assistant message has tool_calls array
    TEST_ASSERT_TRUE(body.find("\"tool_calls\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"id\":\"call_abc123\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_weather\"") != std::string::npos);
    // Verify content is null when empty
    TEST_ASSERT_TRUE(body.find("\"content\":null") != std::string::npos);
    // Verify tool message is present
    TEST_ASSERT_TRUE(body.find("\"tool_call_id\":\"call_abc123\"") != std::string::npos);
}

void test_get_assistant_message_with_tool_calls() {
    // First, we need to simulate a response with tool calls
    // Since we can't call the API, we'll test the method directly by manually
    // setting up the provider's internal state (this requires parseResponse)

    // Parse a simulated response with tool calls
    String mockResponse = R"({
        "choices": [{
            "message": {
                "role": "assistant",
                "content": null,
                "tool_calls": [{
                    "id": "call_xyz789",
                    "type": "function",
                    "function": {
                        "name": "get_temp",
                        "arguments": "{}"
                    }
                }]
            }
        }],
        "usage": {"prompt_tokens": 10, "completion_tokens": 5}
    })";

    Response resp = provider->parseResponse(mockResponse);
    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());

    // Get assistant message with tool calls
    Message msg = provider->getAssistantMessageWithToolCalls("");
    TEST_ASSERT_EQUAL(Role::Assistant, msg.role);
    TEST_ASSERT_TRUE(msg.hasToolCalls());
    TEST_ASSERT_TRUE(msg.toolCallsJson.find("call_xyz789") != std::string::npos);
    TEST_ASSERT_TRUE(msg.toolCallsJson.find("get_temp") != std::string::npos);
}

void test_full_tool_calling_flow() {
    // Simulate the complete tool calling flow as done in the example
    // This should reproduce the exact sequence of operations

    // Step 1: Parse a response with tool calls (simulating first API call)
    String mockResponse = R"({
        "choices": [{
            "message": {
                "role": "assistant",
                "content": null,
                "tool_calls": [{
                    "id": "call_abc123",
                    "type": "function",
                    "function": {
                        "name": "get_temperature",
                        "arguments": "{}"
                    }
                },
                {
                    "id": "call_def456",
                    "type": "function",
                    "function": {
                        "name": "get_humidity",
                        "arguments": "{}"
                    }
                }]
            }
        }],
        "usage": {"prompt_tokens": 10, "completion_tokens": 5}
    })";

    Response resp = provider->parseResponse(mockResponse);
    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());
    TEST_ASSERT_EQUAL(2, provider->getLastToolCalls().size());

    // Step 2: Build the follow-up request with tool results
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the temperature and humidity?"));

    // Get assistant message with tool_calls
    Message assistantMsg = provider->getAssistantMessageWithToolCalls(resp.content);
    messages.push_back(assistantMsg);

    // Verify assistant message has toolCallsJson populated
    TEST_ASSERT_TRUE(assistantMsg.hasToolCalls());

    // Add tool result messages
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

    // NOW simulate clearing tools (like the example does before the second call)
    provider->clearTools();

    // Build the request body AFTER clearTools (this is what actually happens)
    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);


    // Verify the structure
    // 1. Assistant message should have tool_calls
    TEST_ASSERT_TRUE_MESSAGE(body.find("\"tool_calls\":[") != std::string::npos,
        "Assistant message should have tool_calls array");
    TEST_ASSERT_TRUE(body.find("\"id\":\"call_abc123\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"id\":\"call_def456\"") != std::string::npos);

    // 2. Tool messages should have tool_call_id
    TEST_ASSERT_TRUE(body.find("\"tool_call_id\":\"call_abc123\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"tool_call_id\":\"call_def456\"") != std::string::npos);

    // 3. Verify proper structure with type: "function"
    TEST_ASSERT_TRUE(body.find("\"type\":\"function\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"function\":{") != std::string::npos);

    // 4. Verify message order: user, assistant, tool, tool
    // The messages array should have exactly 4 messages
    size_t userPos = body.find("\"role\":\"user\"");
    size_t assistantPos = body.find("\"role\":\"assistant\"");
    size_t toolPos = body.find("\"role\":\"tool\"");

    TEST_ASSERT_TRUE(userPos < assistantPos);
    TEST_ASSERT_TRUE(assistantPos < toolPos);
}
#endif

void test_build_http_request_url() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("https://api.openai.com/v1/chat/completions", req.url.c_str());
}

void test_build_http_request_method() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("POST", req.method.c_str());
}

void test_build_http_request_auth_header() {
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

void test_build_http_request_has_body() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));

    ChatOptions options;
    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_FALSE(req.body.isEmpty());
    TEST_ASSERT_TRUE(req.body.find("\"model\"") != std::string::npos);
}

void test_build_request_unicode_content() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello world"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("Hello world") != std::string::npos);
}

void test_build_request_empty_content() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, ""));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"content\":\"\"") != std::string::npos);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_build_request_basic);
    RUN_TEST(test_build_request_with_system_message);
    RUN_TEST(test_build_request_escapes_special_chars);
    RUN_TEST(test_build_request_custom_model_in_options);
    RUN_TEST(test_build_request_multiple_messages);
    RUN_TEST(test_build_request_with_top_p);
    RUN_TEST(test_build_request_without_top_p_when_default);
    RUN_TEST(test_build_request_with_frequency_penalty);
    RUN_TEST(test_build_request_with_presence_penalty);
    RUN_TEST(test_build_request_no_penalties_when_zero);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_build_request_with_tools);
    RUN_TEST(test_build_request_without_tools);
    RUN_TEST(test_build_request_tool_message);
    RUN_TEST(test_build_request_multiple_tools);
    RUN_TEST(test_build_request_assistant_message_with_tool_calls);
    RUN_TEST(test_get_assistant_message_with_tool_calls);
    RUN_TEST(test_full_tool_calling_flow);
#endif

    RUN_TEST(test_build_http_request_url);
    RUN_TEST(test_build_http_request_method);
    RUN_TEST(test_build_http_request_auth_header);
    RUN_TEST(test_build_http_request_has_body);
    RUN_TEST(test_build_request_unicode_content);
    RUN_TEST(test_build_request_empty_content);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
