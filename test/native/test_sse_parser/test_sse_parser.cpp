#ifdef NATIVE_TEST

#include <unity.h>
#include "http/SSEParser.h"

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

// Format switching

void test_format_switching() {
    parser->setFormat(SSEFormat::OpenAI);
    parser->feed("data: {\"choices\":[{\"delta\":{\"content\":\"OpenAI\"}}]}\n\n");
    TEST_ASSERT_EQUAL_STRING("OpenAI", parser->getAccumulatedContent().c_str());

    parser->reset();
    parser->setFormat(SSEFormat::Anthropic);
    parser->feed("event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"delta\":{\"type\":\"text_delta\",\"text\":\"Anthropic\"}}\n\n");
    TEST_ASSERT_EQUAL_STRING("Anthropic", parser->getAccumulatedContent().c_str());
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

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
