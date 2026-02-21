#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/OllamaProvider.h"

using namespace ESPAI;

static OllamaProvider* provider = nullptr;

void setUp() {
    provider = new OllamaProvider("", "llama3.2");
}

void tearDown() {
    delete provider;
    provider = nullptr;
}

void test_parse_basic_response() {
    String json = R"({
        "id": "chatcmpl-123",
        "choices": [{
            "index": 0,
            "message": {
                "role": "assistant",
                "content": "Hello! How can I help you?"
            },
            "finish_reason": "stop"
        }],
        "usage": {
            "prompt_tokens": 10,
            "completion_tokens": 8,
            "total_tokens": 18
        }
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Hello! How can I help you?", resp.content.c_str());
    TEST_ASSERT_EQUAL(10, resp.promptTokens);
    TEST_ASSERT_EQUAL(8, resp.completionTokens);
}

void test_parse_error_response() {
    String json = R"({
        "error": {
            "message": "model not found"
        }
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ServerError, resp.error);
    TEST_ASSERT_EQUAL_STRING("model not found", resp.errorMessage.c_str());
}

void test_parse_response_no_choices() {
    String json = R"({"id": "chatcmpl-123"})";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_no_message() {
    String json = R"({
        "choices": [{
            "index": 0
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_null_content() {
    String json = R"({
        "choices": [{
            "message": {
                "role": "assistant",
                "content": null
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(resp.content.isEmpty());
}

void test_parse_response_empty_content() {
    String json = R"({
        "choices": [{
            "message": {
                "content": ""
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(resp.content.isEmpty());
}

void test_parse_response_missing_usage() {
    String json = R"({
        "choices": [{
            "message": {
                "content": "Hello from Ollama"
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Hello from Ollama", resp.content.c_str());
    TEST_ASSERT_EQUAL(0, resp.promptTokens);
    TEST_ASSERT_EQUAL(0, resp.completionTokens);
}

void test_parse_response_invalid_json() {
    String json = "not json at all";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_with_newlines() {
    String json = R"({
        "choices": [{
            "message": {
                "content": "Line 1\nLine 2\nLine 3"
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Line 1\nLine 2\nLine 3", resp.content.c_str());
}

#if ESPAI_ENABLE_TOOLS
void test_parse_response_with_tool_calls() {
    String json = R"({
        "choices": [{
            "message": {
                "role": "assistant",
                "content": null,
                "tool_calls": [{
                    "id": "call_abc123",
                    "type": "function",
                    "function": {
                        "name": "get_weather",
                        "arguments": "{\"location\":\"Tokyo\"}"
                    }
                }]
            },
            "finish_reason": "tool_calls"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());

    const auto& calls = provider->getLastToolCalls();
    TEST_ASSERT_EQUAL(1, calls.size());
    TEST_ASSERT_EQUAL_STRING("call_abc123", calls[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_weather", calls[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"location\":\"Tokyo\"}", calls[0].arguments.c_str());
}

void test_parse_response_with_multiple_tool_calls() {
    String json = R"({
        "choices": [{
            "message": {
                "content": null,
                "tool_calls": [
                    {
                        "id": "call_1",
                        "function": {
                            "name": "get_weather",
                            "arguments": "{\"location\":\"Tokyo\"}"
                        }
                    },
                    {
                        "id": "call_2",
                        "function": {
                            "name": "get_time",
                            "arguments": "{\"timezone\":\"JST\"}"
                        }
                    }
                ]
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());

    const auto& calls = provider->getLastToolCalls();
    TEST_ASSERT_EQUAL(2, calls.size());
    TEST_ASSERT_EQUAL_STRING("get_weather", calls[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("get_time", calls[1].name.c_str());
}

void test_parse_response_no_tool_calls() {
    String json = R"({
        "choices": [{
            "message": {
                "content": "Hello"
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_FALSE(provider->hasToolCalls());
}

void test_get_assistant_message_with_tool_calls() {
    String json = R"({
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

    Response resp = provider->parseResponse(json);
    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());

    Message msg = provider->getAssistantMessageWithToolCalls("");
    TEST_ASSERT_EQUAL(Role::Assistant, msg.role);
    TEST_ASSERT_TRUE(msg.hasToolCalls());
    TEST_ASSERT_TRUE(msg.toolCallsJson.find("call_xyz789") != std::string::npos);
    TEST_ASSERT_TRUE(msg.toolCallsJson.find("get_temp") != std::string::npos);
}
#endif

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_parse_basic_response);
    RUN_TEST(test_parse_error_response);
    RUN_TEST(test_parse_response_no_choices);
    RUN_TEST(test_parse_response_no_message);
    RUN_TEST(test_parse_response_null_content);
    RUN_TEST(test_parse_response_empty_content);
    RUN_TEST(test_parse_response_missing_usage);
    RUN_TEST(test_parse_response_invalid_json);
    RUN_TEST(test_parse_response_with_newlines);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_parse_response_with_tool_calls);
    RUN_TEST(test_parse_response_with_multiple_tool_calls);
    RUN_TEST(test_parse_response_no_tool_calls);
    RUN_TEST(test_get_assistant_message_with_tool_calls);
#endif

    return UNITY_END();
}

#else
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
