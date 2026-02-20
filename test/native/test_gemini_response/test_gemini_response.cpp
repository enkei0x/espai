#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/GeminiProvider.h"
#include "http/SSEParser.h"

using namespace ESPAI;

static GeminiProvider* provider = nullptr;

void setUp() {
    provider = new GeminiProvider("test-api-key", "gemini-2.5-flash");
}

void tearDown() {
    delete provider;
    provider = nullptr;
}

void test_parse_response_basic() {
    String json = R"({
        "candidates": [{
            "content": {
                "role": "model",
                "parts": [{"text": "Hello! How can I help you?"}]
            },
            "finishReason": "STOP"
        }],
        "usageMetadata": {
            "promptTokenCount": 10,
            "candidatesTokenCount": 8
        }
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Hello! How can I help you?", resp.content.c_str());
    TEST_ASSERT_EQUAL(10, resp.promptTokens);
    TEST_ASSERT_EQUAL(8, resp.completionTokens);
}

void test_parse_response_multiple_text_parts() {
    String json = R"({
        "candidates": [{
            "content": {
                "parts": [
                    {"text": "First part."},
                    {"text": "Second part."}
                ]
            },
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("First part.\nSecond part.", resp.content.c_str());
}

void test_parse_response_with_newlines() {
    String json = R"({
        "candidates": [{
            "content": {
                "parts": [{"text": "Line 1\nLine 2\nLine 3"}]
            },
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Line 1\nLine 2\nLine 3", resp.content.c_str());
}

void test_parse_response_with_quotes() {
    String json = R"({
        "candidates": [{
            "content": {
                "parts": [{"text": "He said \"hello\" to me"}]
            },
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("He said \"hello\" to me", resp.content.c_str());
}

void test_parse_response_error() {
    String json = R"({
        "error": {
            "code": 400,
            "message": "API key not valid",
            "status": "INVALID_ARGUMENT"
        }
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ServerError, resp.error);
    TEST_ASSERT_EQUAL_STRING("API key not valid", resp.errorMessage.c_str());
    TEST_ASSERT_EQUAL(400, resp.httpStatus);
}

void test_parse_response_no_candidates() {
    String json = R"({"modelVersion": "gemini-2.5-flash"})";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_empty_candidates() {
    String json = R"({"candidates": []})";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_no_content_in_candidate() {
    String json = R"({
        "candidates": [{
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_safety_block() {
    String json = R"({
        "candidates": [{
            "finishReason": "SAFETY"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::InvalidRequest, resp.error);
    TEST_ASSERT_TRUE(resp.errorMessage.find("SAFETY") != std::string::npos);
}

void test_parse_response_recitation_block() {
    String json = R"({
        "candidates": [{
            "finishReason": "RECITATION"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::InvalidRequest, resp.error);
    TEST_ASSERT_TRUE(resp.errorMessage.find("RECITATION") != std::string::npos);
}

void test_parse_response_no_usage() {
    String json = R"({
        "candidates": [{
            "content": {
                "parts": [{"text": "Hello"}]
            },
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL(0, resp.promptTokens);
    TEST_ASSERT_EQUAL(0, resp.completionTokens);
}

void test_parse_response_with_tabs() {
    String json = R"({
        "candidates": [{
            "content": {
                "parts": [{"text": "col1\tcol2\tcol3"}]
            },
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("col1\tcol2\tcol3", resp.content.c_str());
}

void test_parse_response_invalid_json() {
    String json = "not valid json";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_long_content() {
    String textContent = "This is a very long response. ";
    for (int i = 0; i < 5; i++) {
        textContent += textContent;
    }

    String json = "{\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"";
    json += textContent;
    json += "\"}]},\"finishReason\":\"STOP\"}]}";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(resp.content.length() > 500);
}

void test_parse_response_sse_wrapped() {
    String sseBody = "data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Hello \"}]},\"finishReason\":\"STOP\"}]}\n\n";
    sseBody += "data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"World\"}]},\"finishReason\":\"STOP\"}],\"usageMetadata\":{\"promptTokenCount\":5,\"candidatesTokenCount\":3}}\n\n";

    Response resp = provider->parseResponse(sseBody);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Hello World", resp.content.c_str());
    TEST_ASSERT_EQUAL(5, resp.promptTokens);
    TEST_ASSERT_EQUAL(3, resp.completionTokens);
}

void test_parse_response_sse_wrapped_no_valid_data() {
    String sseBody = "data: {\"invalid\": true}\n\n";

    Response resp = provider->parseResponse(sseBody);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::ParseError, resp.error);
}

void test_parse_response_sse_safety_block() {
    String sseBody = "data: {\"candidates\":[{\"finishReason\":\"SAFETY\"}]}\n\n";

    Response resp = provider->parseResponse(sseBody);

    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::InvalidRequest, resp.error);
    TEST_ASSERT_TRUE(resp.errorMessage.find("SAFETY") != std::string::npos);
}

#if ESPAI_ENABLE_TOOLS
void test_parse_response_with_tool_call() {
    String json = R"({
        "candidates": [{
            "content": {
                "role": "model",
                "parts": [{
                    "functionCall": {
                        "name": "get_weather",
                        "args": {"location": "Tokyo"}
                    }
                }]
            },
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());

    const auto& calls = provider->getLastToolCalls();
    TEST_ASSERT_EQUAL(1, calls.size());
    TEST_ASSERT_EQUAL_STRING("get_weather", calls[0].name.c_str());
    TEST_ASSERT_TRUE(calls[0].arguments.find("Tokyo") != std::string::npos);
    TEST_ASSERT_TRUE(calls[0].id.find("gemini_tc_") != std::string::npos);
}

void test_parse_response_with_multiple_tool_calls() {
    String json = R"({
        "candidates": [{
            "content": {
                "parts": [
                    {
                        "functionCall": {
                            "name": "get_weather",
                            "args": {"location": "Tokyo"}
                        }
                    },
                    {
                        "functionCall": {
                            "name": "get_time",
                            "args": {"timezone": "JST"}
                        }
                    }
                ]
            },
            "finishReason": "STOP"
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

void test_parse_response_text_and_tool_call() {
    String json = R"({
        "candidates": [{
            "content": {
                "parts": [
                    {"text": "Let me check the weather."},
                    {
                        "functionCall": {
                            "name": "get_weather",
                            "args": {"location": "Paris"}
                        }
                    }
                ]
            },
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("Let me check the weather.", resp.content.c_str());
    TEST_ASSERT_TRUE(provider->hasToolCalls());
    TEST_ASSERT_EQUAL(1, provider->getLastToolCalls().size());
}

void test_parse_response_no_tool_calls() {
    String json = R"({
        "candidates": [{
            "content": {
                "parts": [{"text": "Hello"}]
            },
            "finishReason": "STOP"
        }]
    })";

    Response resp = provider->parseResponse(json);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_FALSE(provider->hasToolCalls());
}

void test_parse_response_clears_previous_tool_calls() {
    String json1 = R"({
        "candidates": [{
            "content": {
                "parts": [{"functionCall": {"name": "func1", "args": {}}}]
            },
            "finishReason": "STOP"
        }]
    })";
    provider->parseResponse(json1);
    TEST_ASSERT_EQUAL(1, provider->getLastToolCalls().size());

    String json2 = R"({
        "candidates": [{
            "content": {
                "parts": [{"text": "No tools"}]
            },
            "finishReason": "STOP"
        }]
    })";
    provider->parseResponse(json2);
    TEST_ASSERT_EQUAL(0, provider->getLastToolCalls().size());
}

void test_parse_response_sse_wrapped_with_tool_calls() {
    String sseBody = "data: {\"candidates\":[{\"content\":{\"parts\":[{\"functionCall\":{\"name\":\"get_weather\",\"args\":{\"location\":\"NYC\"}}}]},\"finishReason\":\"STOP\"}]}\n\n";

    Response resp = provider->parseResponse(sseBody);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_TRUE(provider->hasToolCalls());
    TEST_ASSERT_EQUAL(1, provider->getLastToolCalls().size());
    TEST_ASSERT_EQUAL_STRING("get_weather", provider->getLastToolCalls()[0].name.c_str());
}
#endif

#if ESPAI_ENABLE_STREAMING
void test_parse_stream_chunk_basic() {
    TEST_ASSERT_EQUAL(SSEFormat::Gemini, provider->getSSEFormat());

    SSEParser parser(SSEFormat::Gemini);
    String result;
    bool isDone = false;

    parser.setContentCallback([&](const String& content, bool done) {
        if (!content.isEmpty()) result += content;
        if (done) isDone = true;
    });

    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Hello\"}]}}]}\n");

    TEST_ASSERT_EQUAL_STRING("Hello", result.c_str());
    TEST_ASSERT_FALSE(isDone);
}

void test_parse_stream_chunk_done_with_finish_reason() {
    SSEParser parser(SSEFormat::Gemini);
    bool isDone = false;
    String result;

    parser.setContentCallback([&](const String& content, bool done) {
        if (!content.isEmpty()) result += content;
        if (done) isDone = true;
    });

    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Final\"}]},\"finishReason\":\"STOP\"}]}\n");

    TEST_ASSERT_TRUE(isDone);
    TEST_ASSERT_EQUAL_STRING("Final", result.c_str());
}

void test_parse_stream_multiple_chunks() {
    SSEParser parser(SSEFormat::Gemini);
    String result;
    bool isDone = false;

    parser.setContentCallback([&](const String& content, bool done) {
        if (!content.isEmpty()) result += content;
        if (done) isDone = true;
    });

    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Hello\"}]}}]}\n");
    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\" there\"}]}}]}\n");
    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"!\"}]},\"finishReason\":\"STOP\"}]}\n");

    TEST_ASSERT_EQUAL_STRING("Hello there!", result.c_str());
    TEST_ASSERT_TRUE(isDone);
}

void test_parse_stream_chunk_with_newline_content() {
    SSEParser parser(SSEFormat::Gemini);
    String result;

    parser.setContentCallback([&](const String& content, bool done) {
        (void)done;
        if (!content.isEmpty()) result += content;
    });

    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Line1\\nLine2\"}]}}]}\n");

    TEST_ASSERT_EQUAL_STRING("Line1\nLine2", result.c_str());
}

void test_parse_stream_accumulation() {
    SSEParser parser(SSEFormat::Gemini);

    parser.setContentCallback([&](const String& content, bool done) {
        (void)content;
        (void)done;
    });

    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"A\"}]}}]}\n");
    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"B\"}]}}]}\n");
    parser.feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"C\"}]},\"finishReason\":\"STOP\"}]}\n");

    TEST_ASSERT_EQUAL_STRING("ABC", parser.getAccumulatedContent().c_str());
}
#endif

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_parse_response_basic);
    RUN_TEST(test_parse_response_multiple_text_parts);
    RUN_TEST(test_parse_response_with_newlines);
    RUN_TEST(test_parse_response_with_quotes);
    RUN_TEST(test_parse_response_error);
    RUN_TEST(test_parse_response_no_candidates);
    RUN_TEST(test_parse_response_empty_candidates);
    RUN_TEST(test_parse_response_no_content_in_candidate);
    RUN_TEST(test_parse_response_safety_block);
    RUN_TEST(test_parse_response_recitation_block);
    RUN_TEST(test_parse_response_no_usage);
    RUN_TEST(test_parse_response_with_tabs);
    RUN_TEST(test_parse_response_invalid_json);
    RUN_TEST(test_parse_response_long_content);
    RUN_TEST(test_parse_response_sse_wrapped);
    RUN_TEST(test_parse_response_sse_wrapped_no_valid_data);
    RUN_TEST(test_parse_response_sse_safety_block);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_parse_response_with_tool_call);
    RUN_TEST(test_parse_response_with_multiple_tool_calls);
    RUN_TEST(test_parse_response_text_and_tool_call);
    RUN_TEST(test_parse_response_no_tool_calls);
    RUN_TEST(test_parse_response_clears_previous_tool_calls);
    RUN_TEST(test_parse_response_sse_wrapped_with_tool_calls);
#endif

#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_parse_stream_chunk_basic);
    RUN_TEST(test_parse_stream_chunk_done_with_finish_reason);
    RUN_TEST(test_parse_stream_multiple_chunks);
    RUN_TEST(test_parse_stream_chunk_with_newline_content);
    RUN_TEST(test_parse_stream_accumulation);
#endif

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
