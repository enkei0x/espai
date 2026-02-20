#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/OpenAIProvider.h"
#include "http/SSEParser.h"

using namespace ESPAI;

static OpenAIProvider* provider = nullptr;

void setUp() {
    provider = new OpenAIProvider("test-api-key", "gpt-4.1-mini");
}

void tearDown() {
    delete provider;
    provider = nullptr;
}

void test_parse_response_basic() {
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

void test_parse_response_with_quotes() {
    String json = R"({
        "choices": [{
            "message": {
                "content": "He said \"hello\" to me"
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("He said \"hello\" to me", resp.content.c_str());
}

void test_parse_response_with_backslash() {
    String json = R"({
        "choices": [{
            "message": {
                "content": "path\\to\\file"
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("path\\to\\file", resp.content.c_str());
}

void test_parse_response_error() {
    String json = R"({
        "error": {
            "message": "Invalid API key provided",
            "type": "invalid_request_error",
            "code": "invalid_api_key"
        }
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ServerError, resp.error);
    TEST_ASSERT_EQUAL_STRING("Invalid API key provided", resp.errorMessage.c_str());
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

void test_parse_response_no_usage() {
    String json = R"({
        "choices": [{
            "message": {
                "content": "Hello"
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL(0, resp.promptTokens);
    TEST_ASSERT_EQUAL(0, resp.completionTokens);
}

void test_parse_response_long_content() {
    String content = "This is a very long response. ";
    for (int i = 0; i < 10; i++) {
        content += content;
    }

    String json = "{\"choices\":[{\"message\":{\"content\":\"";
    json += content;
    json += "\"}}]}";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(resp.content.length() > 1000);
}

void test_parse_response_with_tabs() {
    String json = R"({
        "choices": [{
            "message": {
                "content": "col1\tcol2\tcol3"
            }
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("col1\tcol2\tcol3", resp.content.c_str());
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
#endif

#if ESPAI_ENABLE_STREAMING
void test_parse_stream_chunk_basic() {
    TEST_ASSERT_EQUAL(SSEFormat::OpenAI, provider->getSSEFormat());

    SSEParser parser(SSEFormat::OpenAI);
    String result;
    bool isDone = false;

    parser.setContentCallback([&](const String& content, bool done) {
        if (!content.isEmpty()) result += content;
        if (done) isDone = true;
    });

    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"}}]}\n");

    TEST_ASSERT_EQUAL_STRING("Hello", result.c_str());
    TEST_ASSERT_FALSE(isDone);
}

void test_parse_stream_chunk_done() {
    SSEParser parser(SSEFormat::OpenAI);
    bool isDone = false;

    parser.setContentCallback([&](const String& content, bool done) {
        (void)content;
        if (done) isDone = true;
    });

    parser.feed("data: [DONE]\n");

    TEST_ASSERT_TRUE(isDone);
}

void test_parse_stream_chunk_with_newline() {
    SSEParser parser(SSEFormat::OpenAI);
    String result;

    parser.setContentCallback([&](const String& content, bool done) {
        (void)done;
        if (!content.isEmpty()) result += content;
    });

    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\"Line1\\nLine2\"}}]}\n");

    TEST_ASSERT_EQUAL_STRING("Line1\nLine2", result.c_str());
}

void test_parse_stream_chunk_empty_delta() {
    SSEParser parser(SSEFormat::OpenAI);
    String result;

    parser.setContentCallback([&](const String& content, bool done) {
        (void)done;
        if (!content.isEmpty()) result += content;
    });

    parser.feed("data: {\"choices\":[{\"delta\":{}}]}\n");

    TEST_ASSERT_TRUE(result.isEmpty());
}

void test_parse_stream_chunk_role_only() {
    SSEParser parser(SSEFormat::OpenAI);
    String result;

    parser.setContentCallback([&](const String& content, bool done) {
        (void)done;
        if (!content.isEmpty()) result += content;
    });

    parser.feed("data: {\"choices\":[{\"delta\":{\"role\":\"assistant\"}}]}\n");

    TEST_ASSERT_TRUE(result.isEmpty());
}

void test_parse_stream_chunk_multiple_spaces_after_data() {
    SSEParser parser(SSEFormat::OpenAI);
    String result;

    parser.setContentCallback([&](const String& content, bool done) {
        (void)done;
        if (!content.isEmpty()) result += content;
    });

    parser.feed("data:   {\"choices\":[{\"delta\":{\"content\":\"Spaced\"}}]}\n");

    TEST_ASSERT_EQUAL_STRING("Spaced", result.c_str());
}

void test_parse_stream_multiple_chunks() {
    SSEParser parser(SSEFormat::OpenAI);
    String result;
    bool isDone = false;

    parser.setContentCallback([&](const String& content, bool done) {
        if (!content.isEmpty()) result += content;
        if (done) isDone = true;
    });

    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"}}]}\n");
    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\" there\"}}]}\n");
    parser.feed("data: {\"choices\":[{\"delta\":{\"content\":\"!\"}}]}\n");
    parser.feed("data: [DONE]\n");

    TEST_ASSERT_EQUAL_STRING("Hello there!", result.c_str());
    TEST_ASSERT_TRUE(isDone);
}
#endif

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_parse_response_basic);
    RUN_TEST(test_parse_response_with_newlines);
    RUN_TEST(test_parse_response_with_quotes);
    RUN_TEST(test_parse_response_with_backslash);
    RUN_TEST(test_parse_response_error);
    RUN_TEST(test_parse_response_no_choices);
    RUN_TEST(test_parse_response_no_message);
    RUN_TEST(test_parse_response_null_content);
    RUN_TEST(test_parse_response_empty_content);
    RUN_TEST(test_parse_response_no_usage);
    RUN_TEST(test_parse_response_long_content);
    RUN_TEST(test_parse_response_with_tabs);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_parse_response_with_tool_calls);
    RUN_TEST(test_parse_response_with_multiple_tool_calls);
    RUN_TEST(test_parse_response_no_tool_calls);
#endif

#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_parse_stream_chunk_basic);
    RUN_TEST(test_parse_stream_chunk_done);
    RUN_TEST(test_parse_stream_chunk_with_newline);
    RUN_TEST(test_parse_stream_chunk_empty_delta);
    RUN_TEST(test_parse_stream_chunk_role_only);
    RUN_TEST(test_parse_stream_chunk_multiple_spaces_after_data);
    RUN_TEST(test_parse_stream_multiple_chunks);
#endif

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
