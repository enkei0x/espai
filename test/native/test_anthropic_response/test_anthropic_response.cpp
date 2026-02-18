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

void test_parse_response_basic() {
    String json = R"({
        "id": "msg_123",
        "type": "message",
        "role": "assistant",
        "content": [
            {
                "type": "text",
                "text": "Hello! How can I help you?"
            }
        ],
        "stop_reason": "end_turn",
        "usage": {
            "input_tokens": 10,
            "output_tokens": 8
        }
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Hello! How can I help you?", resp.content.c_str());
    TEST_ASSERT_EQUAL(10, resp.promptTokens);
    TEST_ASSERT_EQUAL(8, resp.completionTokens);
}

void test_parse_response_multiple_text_blocks() {
    String json = R"({
        "content": [
            {"type": "text", "text": "First part."},
            {"type": "text", "text": "Second part."}
        ]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("First part.\nSecond part.", resp.content.c_str());
}

void test_parse_response_with_newlines() {
    String json = R"({
        "content": [
            {"type": "text", "text": "Line 1\nLine 2\nLine 3"}
        ]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Line 1\nLine 2\nLine 3", resp.content.c_str());
}

void test_parse_response_with_quotes() {
    String json = R"({
        "content": [
            {"type": "text", "text": "He said \"hello\" to me"}
        ]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("He said \"hello\" to me", resp.content.c_str());
}

void test_parse_response_error() {
    String json = R"({
        "error": {
            "type": "invalid_request_error",
            "message": "Invalid API key provided"
        }
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ServerError, resp.error);
    TEST_ASSERT_EQUAL_STRING("Invalid API key provided", resp.errorMessage.c_str());
}

void test_parse_response_no_content() {
    String json = R"({"id": "msg_123"})";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_empty_content_array() {
    String json = R"({
        "content": []
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_no_usage() {
    String json = R"({
        "content": [
            {"type": "text", "text": "Hello"}
        ]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL(0, resp.promptTokens);
    TEST_ASSERT_EQUAL(0, resp.completionTokens);
}

void test_parse_response_with_tabs() {
    String json = R"({
        "content": [
            {"type": "text", "text": "col1\tcol2\tcol3"}
        ]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("col1\tcol2\tcol3", resp.content.c_str());
}

#if ESPAI_ENABLE_TOOLS
void test_parse_response_with_tool_use() {
    String json = R"({
        "content": [
            {
                "type": "tool_use",
                "id": "toolu_abc123",
                "name": "get_weather",
                "input": {"location": "Tokyo"}
            }
        ],
        "stop_reason": "tool_use"
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());

    const auto& calls = provider->getLastToolCalls();
    TEST_ASSERT_EQUAL(1, calls.size());
    TEST_ASSERT_EQUAL_STRING("toolu_abc123", calls[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_weather", calls[0].name.c_str());
    TEST_ASSERT_TRUE(calls[0].arguments.find("Tokyo") != std::string::npos);
}

void test_parse_response_with_multiple_tool_uses() {
    String json = R"({
        "content": [
            {
                "type": "tool_use",
                "id": "toolu_1",
                "name": "get_weather",
                "input": {"location": "Tokyo"}
            },
            {
                "type": "tool_use",
                "id": "toolu_2",
                "name": "get_time",
                "input": {"timezone": "JST"}
            }
        ]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());

    const auto& calls = provider->getLastToolCalls();
    TEST_ASSERT_EQUAL(2, calls.size());
    TEST_ASSERT_EQUAL_STRING("get_weather", calls[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("get_time", calls[1].name.c_str());
}

void test_parse_response_text_and_tool_use() {
    String json = R"({
        "content": [
            {"type": "text", "text": "Let me check the weather."},
            {
                "type": "tool_use",
                "id": "toolu_xyz",
                "name": "get_weather",
                "input": {"location": "Paris"}
            }
        ]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Let me check the weather.", resp.content.c_str());
    TEST_ASSERT_TRUE(provider->hasToolCalls());
    TEST_ASSERT_EQUAL(1, provider->getLastToolCalls().size());
}

void test_parse_response_no_tool_uses() {
    String json = R"({
        "content": [
            {"type": "text", "text": "Hello"}
        ]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_FALSE(provider->hasToolCalls());
}

void test_parse_response_clears_previous_tool_uses() {
    String json1 = R"({
        "content": [
            {"type": "tool_use", "id": "toolu_1", "name": "func1", "input": {}}
        ]
    })";
    provider->parseResponse(json1);
    TEST_ASSERT_EQUAL(1, provider->getLastToolCalls().size());

    String json2 = R"({
        "content": [
            {"type": "text", "text": "No tools"}
        ]
    })";
    provider->parseResponse(json2);
    TEST_ASSERT_EQUAL(0, provider->getLastToolCalls().size());
}
#endif

#if ESPAI_ENABLE_STREAMING
void test_parse_stream_chunk_content_block_delta() {
    String chunk = "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"Hello\"}}";

    String content;
    bool done = false;

    bool result = provider->parseStreamChunk(chunk, content, done);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(done);
    TEST_ASSERT_EQUAL_STRING("Hello", content.c_str());
}

void test_parse_stream_chunk_message_stop() {
    String chunk = "event: message_stop\ndata: {\"type\":\"message_stop\"}";

    String content;
    bool done = false;

    bool result = provider->parseStreamChunk(chunk, content, done);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(done);
}

void test_parse_stream_chunk_content_block_start() {
    String chunk = "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"text\",\"text\":\"\"}}";

    String content;
    bool done = false;

    bool result = provider->parseStreamChunk(chunk, content, done);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(done);
    TEST_ASSERT_TRUE(content.isEmpty());
}

void test_parse_stream_chunk_multiple_deltas() {
    String accumulated;

    std::vector<String> chunks = {
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Hello\"}}",
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\" there\"}}",
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"!\"}}",
        "event: message_stop\ndata: {\"type\":\"message_stop\"}"
    };

    for (const auto& chunk : chunks) {
        String content;
        bool done = false;

        provider->parseStreamChunk(chunk, content, done);

        if (done) {
            break;
        }
        accumulated += content;
    }

    TEST_ASSERT_EQUAL_STRING("Hello there!", accumulated.c_str());
}

void test_parse_stream_chunk_data_only() {
    String chunk = "data: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Test\"}}";

    String content;
    bool done = false;

    bool result = provider->parseStreamChunk(chunk, content, done);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Test", content.c_str());
}

void test_parse_stream_chunk_message_delta() {
    String chunk = "event: message_delta\ndata: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"end_turn\"}}";

    String content;
    bool done = false;

    bool result = provider->parseStreamChunk(chunk, content, done);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_FALSE(done);
    TEST_ASSERT_TRUE(content.isEmpty());
}
#endif

void test_parse_response_long_content() {
    String textContent = "This is a very long response. ";
    for (int i = 0; i < 5; i++) {
        textContent += textContent;
    }

    String json = "{\"content\":[{\"type\":\"text\",\"text\":\"";
    json += textContent;
    json += "\"}]}";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(resp.content.length() > 500);
}

void test_parse_response_invalid_json() {
    String json = "not valid json";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_parse_response_basic);
    RUN_TEST(test_parse_response_multiple_text_blocks);
    RUN_TEST(test_parse_response_with_newlines);
    RUN_TEST(test_parse_response_with_quotes);
    RUN_TEST(test_parse_response_error);
    RUN_TEST(test_parse_response_no_content);
    RUN_TEST(test_parse_response_empty_content_array);
    RUN_TEST(test_parse_response_no_usage);
    RUN_TEST(test_parse_response_with_tabs);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_parse_response_with_tool_use);
    RUN_TEST(test_parse_response_with_multiple_tool_uses);
    RUN_TEST(test_parse_response_text_and_tool_use);
    RUN_TEST(test_parse_response_no_tool_uses);
    RUN_TEST(test_parse_response_clears_previous_tool_uses);
#endif

#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_parse_stream_chunk_content_block_delta);
    RUN_TEST(test_parse_stream_chunk_message_stop);
    RUN_TEST(test_parse_stream_chunk_content_block_start);
    RUN_TEST(test_parse_stream_chunk_multiple_deltas);
    RUN_TEST(test_parse_stream_chunk_data_only);
    RUN_TEST(test_parse_stream_chunk_message_delta);
#endif

    RUN_TEST(test_parse_response_long_content);
    RUN_TEST(test_parse_response_invalid_json);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
