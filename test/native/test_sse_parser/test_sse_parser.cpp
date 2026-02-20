#ifdef NATIVE_TEST

#include <unity.h>
#include "http/SSEParser.h"
#include <vector>

using namespace ESPAI;

static SSEParser* parser = nullptr;
static String lastContent;
static bool lastDone;
static int callbackCount;

void contentCallback(const String& content, bool done) {
    lastContent = content;
    lastDone = done;
    callbackCount++;
}

void setUp() {
    parser = new SSEParser();
    parser->setContentCallback(contentCallback);
    lastContent = "";
    lastDone = false;
    callbackCount = 0;
}

void tearDown() {
    delete parser;
    parser = nullptr;
}

// Single chunk tests

void test_single_chunk_openai() {
    parser->setFormat(SSEFormat::OpenAI);
    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"}}]}\n\n");

    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("Hello", lastContent.c_str());
    TEST_ASSERT_FALSE(lastDone);
}

void test_single_chunk_openai_with_spaces() {
    parser->setFormat(SSEFormat::OpenAI);
    parser->feed("data:   {\"choices\":[{\"delta\":{\"content\":\"World\"}}]}\n\n");

    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("World", lastContent.c_str());
}

void test_single_chunk_anthropic() {
    parser->setFormat(SSEFormat::Anthropic);
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Hi\"}}\n\n");

    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("Hi", lastContent.c_str());
    TEST_ASSERT_FALSE(lastDone);
}

// Multiple chunks tests

void test_multiple_chunks_openai() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Hello\"}}]}\n\n");
    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("Hello", lastContent.c_str());

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\" World\"}}]}\n\n");
    TEST_ASSERT_EQUAL(2, callbackCount);
    TEST_ASSERT_EQUAL_STRING(" World", lastContent.c_str());

    TEST_ASSERT_EQUAL_STRING("Hello World", parser->getAccumulatedContent().c_str());
}

void test_multiple_chunks_anthropic() {
    parser->setFormat(SSEFormat::Anthropic);

    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"A\"}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"B\"}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"C\"}}\n\n");

    TEST_ASSERT_EQUAL(3, callbackCount);
    TEST_ASSERT_EQUAL_STRING("ABC", parser->getAccumulatedContent().c_str());
}

void test_multiple_lines_in_one_feed() {
    parser->setFormat(SSEFormat::OpenAI);

    String data = "data: {\"choices\":[{\"delta\":{\"content\":\"First\"}}]}\n\n"
                  "data: {\"choices\":[{\"delta\":{\"content\":\"Second\"}}]}\n\n";
    parser->feed(data);

    TEST_ASSERT_EQUAL(2, callbackCount);
    TEST_ASSERT_EQUAL_STRING("FirstSecond", parser->getAccumulatedContent().c_str());
}

// Partial data tests

void test_partial_data_split_line() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Par");
    TEST_ASSERT_EQUAL(0, callbackCount);

    parser->feed("tial\"}}]}\n\n");
    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("Partial", lastContent.c_str());
}

void test_partial_data_split_at_newline() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Test\"}}]}\n");
    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("Test", lastContent.c_str());

    parser->feed("\n");
    TEST_ASSERT_EQUAL(1, callbackCount);
}

void test_partial_data_byte_by_byte() {
    parser->setFormat(SSEFormat::OpenAI);

    const char* chunk = "data: {\"choices\":[{\"delta\":{\"content\":\"X\"}}]}\n\n";
    size_t len = strlen(chunk);

    for (size_t i = 0; i < len; i++) {
        parser->feed(chunk + i, 1);
    }

    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("X", lastContent.c_str());
}

void test_partial_anthropic_event() {
    parser->setFormat(SSEFormat::Anthropic);

    parser->feed("event: content_block_delta\n");
    TEST_ASSERT_EQUAL(0, callbackCount);

    parser->feed("data: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"OK\"}}\n");
    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("OK", lastContent.c_str());

    parser->feed("\n");
    TEST_ASSERT_EQUAL(1, callbackCount);
}

// [DONE] handling tests

void test_done_openai() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Final\"}}]}\n\n");
    TEST_ASSERT_FALSE(parser->isDone());

    parser->feed("data: [DONE]\n\n");
    TEST_ASSERT_TRUE(parser->isDone());
    TEST_ASSERT_TRUE(lastDone);
}

void test_done_openai_with_spaces() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data:   [DONE]\n\n");
    TEST_ASSERT_TRUE(parser->isDone());
}

void test_done_stops_processing() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: [DONE]\n\n");
    int countAfterDone = callbackCount;

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Ignored\"}}]}\n\n");
    TEST_ASSERT_EQUAL(countAfterDone, callbackCount);
}

// Anthropic event format tests

void test_anthropic_message_stop() {
    parser->setFormat(SSEFormat::Anthropic);

    parser->feed("event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n");
    TEST_ASSERT_TRUE(parser->isDone());
    TEST_ASSERT_TRUE(lastDone);
}

void test_anthropic_message_start_ignored() {
    parser->setFormat(SSEFormat::Anthropic);

    parser->feed("event: message_start\ndata: {\"type\":\"message_start\"}\n\n");
    TEST_ASSERT_FALSE(parser->isDone());
    TEST_ASSERT_EQUAL(0, callbackCount);
}

void test_anthropic_content_block_start() {
    parser->setFormat(SSEFormat::Anthropic);

    parser->feed("event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0}\n\n");
    TEST_ASSERT_EQUAL(0, callbackCount);
}

void test_anthropic_ping() {
    parser->setFormat(SSEFormat::Anthropic);

    parser->feed("event: ping\ndata: {\"type\":\"ping\"}\n\n");
    TEST_ASSERT_FALSE(parser->isDone());
    TEST_ASSERT_EQUAL(0, callbackCount);
}

void test_anthropic_full_sequence() {
    parser->setFormat(SSEFormat::Anthropic);

    parser->feed("event: message_start\ndata: {\"type\":\"message_start\"}\n\n");
    parser->feed("event: content_block_start\ndata: {\"type\":\"content_block_start\"}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Hello\"}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\" there\"}}\n\n");
    parser->feed("event: content_block_stop\ndata: {\"type\":\"content_block_stop\"}\n\n");
    parser->feed("event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n");

    TEST_ASSERT_TRUE(parser->isDone());
    TEST_ASSERT_EQUAL_STRING("Hello there", parser->getAccumulatedContent().c_str());
}

// Cancellation tests

void test_cancel() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Before\"}}]}\n\n");
    TEST_ASSERT_EQUAL(1, callbackCount);

    parser->cancel();
    TEST_ASSERT_TRUE(parser->isCancelled());

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"After\"}}]}\n\n");
    TEST_ASSERT_EQUAL(1, callbackCount);
}

// Reset tests

void test_reset() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Test\"}}]}\n\n");
    parser->feed("data: [DONE]\n\n");

    TEST_ASSERT_TRUE(parser->isDone());
    TEST_ASSERT_FALSE(parser->getAccumulatedContent().empty());

    parser->reset();

    TEST_ASSERT_FALSE(parser->isDone());
    TEST_ASSERT_FALSE(parser->isCancelled());
    TEST_ASSERT_TRUE(parser->getAccumulatedContent().empty());
}

// Accumulation control tests

void test_disable_accumulation() {
    parser->setFormat(SSEFormat::OpenAI);
    parser->setAccumulateContent(false);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"A\"}}]}\n\n");
    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"B\"}}]}\n\n");

    TEST_ASSERT_TRUE(parser->getAccumulatedContent().empty());
    TEST_ASSERT_EQUAL(2, callbackCount);
}

// Empty delta tests

void test_empty_delta_openai() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{}}]}\n\n");
    TEST_ASSERT_EQUAL(0, callbackCount);
}

void test_role_only_delta() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"role\":\"assistant\"}}]}\n\n");
    TEST_ASSERT_EQUAL(0, callbackCount);
}

// CRLF handling tests

void test_crlf_line_endings() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"CRLF\"}}]}\r\n\r\n");
    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("CRLF", lastContent.c_str());
}

// Event callback tests

void test_event_callback() {
    String lastEventType;
    String lastEventData;
    bool lastEventDone = false;

    parser->setEventCallback([&](const SSEEvent& event) {
        lastEventType = event.eventType;
        lastEventData = event.data;
        lastEventDone = event.isDone;
    });

    parser->setFormat(SSEFormat::Anthropic);
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Test\"}}\n\n");

    TEST_ASSERT_EQUAL_STRING("content_block_delta", lastEventType.c_str());
    TEST_ASSERT_FALSE(lastEventDone);
}

// Content with special characters

void test_content_with_newlines() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"Line1\\nLine2\"}}]}\n\n");
    TEST_ASSERT_EQUAL_STRING("Line1\nLine2", lastContent.c_str());
}

void test_content_with_quotes() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"He said \\\"hi\\\"\"}}]}\n\n");
    TEST_ASSERT_EQUAL_STRING("He said \"hi\"", lastContent.c_str());
}

// Gemini single chunk tests

void test_single_chunk_gemini() {
    parser->setFormat(SSEFormat::Gemini);
    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Hi\"}]}}]}\n\n");

    TEST_ASSERT_EQUAL(1, callbackCount);
    TEST_ASSERT_EQUAL_STRING("Hi", lastContent.c_str());
    TEST_ASSERT_FALSE(lastDone);
}

void test_gemini_done_with_finish_reason() {
    parser->setFormat(SSEFormat::Gemini);

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Done\"}]},\"finishReason\":\"STOP\"}]}\n\n");

    TEST_ASSERT_TRUE(parser->isDone());
    TEST_ASSERT_TRUE(lastDone);
    TEST_ASSERT_EQUAL_STRING("Done", parser->getAccumulatedContent().c_str());
}

void test_gemini_multiple_chunks() {
    parser->setFormat(SSEFormat::Gemini);

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"A\"}]}}]}\n\n");
    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"B\"}]}}]}\n\n");
    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"C\"}]},\"finishReason\":\"STOP\"}]}\n\n");

    // 3 content callbacks + 1 done callback = 4
    TEST_ASSERT_EQUAL(4, callbackCount);
    TEST_ASSERT_EQUAL_STRING("ABC", parser->getAccumulatedContent().c_str());
    TEST_ASSERT_TRUE(parser->isDone());
}

void test_gemini_done_stops_processing() {
    parser->setFormat(SSEFormat::Gemini);

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Final\"}]},\"finishReason\":\"STOP\"}]}\n\n");
    int countAfterDone = callbackCount;

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Ignored\"}]}}]}\n\n");
    TEST_ASSERT_EQUAL(countAfterDone, callbackCount);
}

void test_gemini_no_candidates_ignored() {
    parser->setFormat(SSEFormat::Gemini);

    parser->feed("data: {\"usageMetadata\":{\"promptTokenCount\":5}}\n\n");
    TEST_ASSERT_EQUAL(0, callbackCount);
}

void test_gemini_content_with_newlines() {
    parser->setFormat(SSEFormat::Gemini);

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Line1\\nLine2\"}]}}]}\n\n");
    TEST_ASSERT_EQUAL_STRING("Line1\nLine2", lastContent.c_str());
}

void test_gemini_full_sequence() {
    parser->setFormat(SSEFormat::Gemini);

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Hello\"}]}}]}\n\n");
    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\" there\"}]}}]}\n\n");
    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"!\"}]},\"finishReason\":\"STOP\"}],\"usageMetadata\":{\"promptTokenCount\":10,\"candidatesTokenCount\":5}}\n\n");

    TEST_ASSERT_TRUE(parser->isDone());
    TEST_ASSERT_EQUAL_STRING("Hello there!", parser->getAccumulatedContent().c_str());
}

void test_gemini_error_in_stream() {
    parser->setFormat(SSEFormat::Gemini);

    parser->feed("data: {\"error\":{\"code\":429,\"message\":\"Resource exhausted\"}}\n\n");

    TEST_ASSERT_TRUE(parser->hasError());
    TEST_ASSERT_EQUAL(ErrorCode::ServerError, parser->getError());
    TEST_ASSERT_TRUE(parser->getErrorMessage().find("Resource exhausted") != std::string::npos);
}

// Format switching

void test_format_switching() {
    parser->setFormat(SSEFormat::OpenAI);
    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"OpenAI\"}}]}\n\n");
    TEST_ASSERT_EQUAL_STRING("OpenAI", parser->getAccumulatedContent().c_str());

    parser->reset();
    parser->setFormat(SSEFormat::Anthropic);
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Anthropic\"}}\n\n");
    TEST_ASSERT_EQUAL_STRING("Anthropic", parser->getAccumulatedContent().c_str());

    parser->reset();
    parser->setFormat(SSEFormat::Gemini);
    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Gemini\"}]}}]}\n\n");
    TEST_ASSERT_EQUAL_STRING("Gemini", parser->getAccumulatedContent().c_str());
}

// Constructor tests

void test_constructor_default() {
    SSEParser p;
    TEST_ASSERT_EQUAL(SSEFormat::OpenAI, p.getFormat());
    TEST_ASSERT_FALSE(p.isDone());
    TEST_ASSERT_FALSE(p.isCancelled());
    TEST_ASSERT_FALSE(p.hasError());
}

void test_constructor_with_format() {
    SSEParser p(SSEFormat::Anthropic);
    TEST_ASSERT_EQUAL(SSEFormat::Anthropic, p.getFormat());
}

// Timeout tests

void test_timeout_configuration() {
    parser->setTimeout(5000);
    TEST_ASSERT_EQUAL(5000, parser->getTimeout());
}

#if ESPAI_ENABLE_TOOLS

static std::vector<ToolCall> toolCallResults;

void resetToolCallResults() {
    toolCallResults.clear();
}

void toolCallCallback(const String& id, const String& name, const String& arguments) {
    toolCallResults.push_back(ToolCall(id, name, arguments));
}

void test_openai_stream_single_tool_call() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::OpenAI);
    parser->setToolCallCallback(toolCallCallback);

    // First chunk: role + tool_calls with id and name
    parser->feed("data: {\"choices\":[{\"delta\":{\"role\":\"assistant\",\"tool_calls\":[{\"index\":0,\"id\":\"call_abc123\",\"type\":\"function\",\"function\":{\"name\":\"get_weather\",\"arguments\":\"\"}}]}}]}\n\n");

    // Argument fragments
    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\"{\\\"lo\"}}]}}]}\n\n");
    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\"cation\\\"\"}}]}}]}\n\n");
    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\": \\\"Tokyo\\\"}\"}}]}}]}\n\n");

    // [DONE] triggers finalization
    parser->feed("data: [DONE]\n\n");

    TEST_ASSERT_EQUAL(1, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("call_abc123", toolCallResults[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"location\": \"Tokyo\"}", toolCallResults[0].arguments.c_str());
}

void test_openai_stream_multiple_tool_calls() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::OpenAI);
    parser->setToolCallCallback(toolCallCallback);

    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_1\",\"type\":\"function\",\"function\":{\"name\":\"get_weather\",\"arguments\":\"\"}}]}}]}\n\n");
    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"function\":{\"arguments\":\"{\\\"location\\\":\\\"Tokyo\\\"}\"}}]}}]}\n\n");

    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":1,\"id\":\"call_2\",\"type\":\"function\",\"function\":{\"name\":\"get_time\",\"arguments\":\"\"}}]}}]}\n\n");
    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":1,\"function\":{\"arguments\":\"{\\\"timezone\\\":\\\"JST\\\"}\"}}]}}]}\n\n");

    parser->feed("data: [DONE]\n\n");

    TEST_ASSERT_EQUAL(2, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("call_1", toolCallResults[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"location\":\"Tokyo\"}", toolCallResults[0].arguments.c_str());
    TEST_ASSERT_EQUAL_STRING("call_2", toolCallResults[1].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_time", toolCallResults[1].name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"timezone\":\"JST\"}", toolCallResults[1].arguments.c_str());
}

void test_openai_stream_tool_calls_available_via_getter() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_x\",\"type\":\"function\",\"function\":{\"name\":\"func1\",\"arguments\":\"{}\"}}]}}]}\n\n");
    parser->feed("data: [DONE]\n\n");

    const auto& calls = parser->getToolCalls();
    TEST_ASSERT_EQUAL(1, calls.size());
    TEST_ASSERT_EQUAL_STRING("call_x", calls[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("func1", calls[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("{}", calls[0].arguments.c_str());
}

void test_anthropic_stream_single_tool_call() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::Anthropic);
    parser->setToolCallCallback(toolCallCallback);

    parser->feed("event: message_start\ndata: {\"type\":\"message_start\"}\n\n");

    // Tool use content block
    parser->feed("event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"tool_use\",\"id\":\"toolu_abc123\",\"name\":\"get_weather\",\"input\":{}}}\n\n");

    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"\"}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"location\\\"\"}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\": \\\"San\"}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\" Francisco, CA\\\"\"}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"}\"}}\n\n");

    // content_block_stop triggers callback for Anthropic
    parser->feed("event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":0}\n\n");

    TEST_ASSERT_EQUAL(1, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("toolu_abc123", toolCallResults[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"location\": \"San Francisco, CA\"}", toolCallResults[0].arguments.c_str());
}

void test_anthropic_stream_multiple_tool_calls() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::Anthropic);
    parser->setToolCallCallback(toolCallCallback);

    parser->feed("event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"tool_use\",\"id\":\"toolu_1\",\"name\":\"get_weather\",\"input\":{}}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"location\\\":\\\"Tokyo\\\"}\"}}\n\n");
    parser->feed("event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":0}\n\n");

    TEST_ASSERT_EQUAL(1, toolCallResults.size());

    parser->feed("event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":1,\"content_block\":{\"type\":\"tool_use\",\"id\":\"toolu_2\",\"name\":\"get_time\",\"input\":{}}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"timezone\\\":\\\"JST\\\"}\"}}\n\n");
    parser->feed("event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":1}\n\n");

    parser->feed("event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n");

    TEST_ASSERT_EQUAL(2, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("toolu_1", toolCallResults[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("toolu_2", toolCallResults[1].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_time", toolCallResults[1].name.c_str());
}

void test_openai_stream_mixed_text_and_tool_calls() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::OpenAI);
    parser->setToolCallCallback(toolCallCallback);

    // Text content first
    parser->feed("data: {\"choices\":[{\"delta\":{\"role\":\"assistant\",\"content\":\"Let me check\"}}]}\n\n");
    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\" the weather.\"}}]}\n\n");

    TEST_ASSERT_EQUAL(2, callbackCount);

    // Then tool call
    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_mix\",\"type\":\"function\",\"function\":{\"name\":\"get_weather\",\"arguments\":\"{\\\"location\\\":\\\"Paris\\\"}\"}}]}}]}\n\n");
    parser->feed("data: [DONE]\n\n");

    TEST_ASSERT_EQUAL(1, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"location\":\"Paris\"}", toolCallResults[0].arguments.c_str());
}

void test_anthropic_stream_mixed_text_and_tool_call() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::Anthropic);
    parser->setToolCallCallback(toolCallCallback);

    // Text block first
    parser->feed("event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"text\",\"text\":\"\"}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"Let me check.\"}}\n\n");
    parser->feed("event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":0}\n\n");

    TEST_ASSERT_EQUAL_STRING("Let me check.", parser->getAccumulatedContent().c_str());
    TEST_ASSERT_EQUAL(0, toolCallResults.size());

    // Tool use block
    parser->feed("event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":1,\"content_block\":{\"type\":\"tool_use\",\"id\":\"toolu_mix\",\"name\":\"get_weather\",\"input\":{}}}\n\n");
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":1,\"delta\":{\"type\":\"input_json_delta\",\"partial_json\":\"{\\\"location\\\":\\\"Paris\\\"}\"}}\n\n");
    parser->feed("event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":1}\n\n");

    parser->feed("event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n");

    TEST_ASSERT_EQUAL(1, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("toolu_mix", toolCallResults[0].id.c_str());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("{\"location\":\"Paris\"}", toolCallResults[0].arguments.c_str());
}

void test_gemini_stream_single_tool_call() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::Gemini);
    parser->setToolCallCallback(toolCallCallback);

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"functionCall\":{\"name\":\"get_weather\",\"args\":{\"location\":\"Tokyo\"}}}]},\"finishReason\":\"STOP\"}]}\n\n");

    TEST_ASSERT_EQUAL(1, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_TRUE(toolCallResults[0].arguments.find("Tokyo") != std::string::npos);
    TEST_ASSERT_TRUE(toolCallResults[0].id.find("gemini_tc_") != std::string::npos);
}

void test_gemini_stream_multiple_tool_calls() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::Gemini);
    parser->setToolCallCallback(toolCallCallback);

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"functionCall\":{\"name\":\"get_weather\",\"args\":{\"location\":\"Tokyo\"}}},{\"functionCall\":{\"name\":\"get_time\",\"args\":{\"timezone\":\"JST\"}}}]},\"finishReason\":\"STOP\"}]}\n\n");

    TEST_ASSERT_EQUAL(2, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("get_time", toolCallResults[1].name.c_str());
}

void test_gemini_stream_mixed_text_and_tool_call() {
    resetToolCallResults();
    parser->setFormat(SSEFormat::Gemini);
    parser->setToolCallCallback(toolCallCallback);

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"text\":\"Checking...\"}]}}]}\n\n");
    TEST_ASSERT_EQUAL(0, toolCallResults.size());

    parser->feed("data: {\"candidates\":[{\"content\":{\"parts\":[{\"functionCall\":{\"name\":\"get_weather\",\"args\":{\"location\":\"Paris\"}}}]},\"finishReason\":\"STOP\"}]}\n\n");

    TEST_ASSERT_EQUAL(1, toolCallResults.size());
    TEST_ASSERT_EQUAL_STRING("get_weather", toolCallResults[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("Checking...", parser->getAccumulatedContent().c_str());
}

void test_tool_calls_cleared_on_reset() {
    parser->setFormat(SSEFormat::OpenAI);

    parser->feed("data: {\"choices\":[{\"delta\":{\"tool_calls\":[{\"index\":0,\"id\":\"call_r\",\"type\":\"function\",\"function\":{\"name\":\"func\",\"arguments\":\"{}\"}}]}}]}\n\n");
    parser->feed("data: [DONE]\n\n");

    TEST_ASSERT_EQUAL(1, parser->getToolCalls().size());

    parser->reset();
    TEST_ASSERT_EQUAL(0, parser->getToolCalls().size());
}

#endif // ESPAI_ENABLE_TOOLS

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    // Single chunk
    RUN_TEST(test_single_chunk_openai);
    RUN_TEST(test_single_chunk_openai_with_spaces);
    RUN_TEST(test_single_chunk_anthropic);

    // Multiple chunks
    RUN_TEST(test_multiple_chunks_openai);
    RUN_TEST(test_multiple_chunks_anthropic);
    RUN_TEST(test_multiple_lines_in_one_feed);

    // Partial data
    RUN_TEST(test_partial_data_split_line);
    RUN_TEST(test_partial_data_split_at_newline);
    RUN_TEST(test_partial_data_byte_by_byte);
    RUN_TEST(test_partial_anthropic_event);

    // [DONE] handling
    RUN_TEST(test_done_openai);
    RUN_TEST(test_done_openai_with_spaces);
    RUN_TEST(test_done_stops_processing);

    // Anthropic event format
    RUN_TEST(test_anthropic_message_stop);
    RUN_TEST(test_anthropic_message_start_ignored);
    RUN_TEST(test_anthropic_content_block_start);
    RUN_TEST(test_anthropic_ping);
    RUN_TEST(test_anthropic_full_sequence);

    // Gemini format
    RUN_TEST(test_single_chunk_gemini);
    RUN_TEST(test_gemini_done_with_finish_reason);
    RUN_TEST(test_gemini_multiple_chunks);
    RUN_TEST(test_gemini_done_stops_processing);
    RUN_TEST(test_gemini_no_candidates_ignored);
    RUN_TEST(test_gemini_content_with_newlines);
    RUN_TEST(test_gemini_full_sequence);
    RUN_TEST(test_gemini_error_in_stream);

    // Cancellation
    RUN_TEST(test_cancel);

    // Reset
    RUN_TEST(test_reset);

    // Accumulation
    RUN_TEST(test_disable_accumulation);

    // Empty delta
    RUN_TEST(test_empty_delta_openai);
    RUN_TEST(test_role_only_delta);

    // CRLF
    RUN_TEST(test_crlf_line_endings);

    // Event callback
    RUN_TEST(test_event_callback);

    // Special characters
    RUN_TEST(test_content_with_newlines);
    RUN_TEST(test_content_with_quotes);

    // Format switching
    RUN_TEST(test_format_switching);

    // Constructors
    RUN_TEST(test_constructor_default);
    RUN_TEST(test_constructor_with_format);

    // Timeout
    RUN_TEST(test_timeout_configuration);

#if ESPAI_ENABLE_TOOLS
    // Streaming tool calls - OpenAI
    RUN_TEST(test_openai_stream_single_tool_call);
    RUN_TEST(test_openai_stream_multiple_tool_calls);
    RUN_TEST(test_openai_stream_tool_calls_available_via_getter);
    RUN_TEST(test_openai_stream_mixed_text_and_tool_calls);

    // Streaming tool calls - Anthropic
    RUN_TEST(test_anthropic_stream_single_tool_call);
    RUN_TEST(test_anthropic_stream_multiple_tool_calls);
    RUN_TEST(test_anthropic_stream_mixed_text_and_tool_call);

    // Streaming tool calls - Gemini
    RUN_TEST(test_gemini_stream_single_tool_call);
    RUN_TEST(test_gemini_stream_multiple_tool_calls);
    RUN_TEST(test_gemini_stream_mixed_text_and_tool_call);

    // Tool call reset
    RUN_TEST(test_tool_calls_cleared_on_reset);
#endif

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
