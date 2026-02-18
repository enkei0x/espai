/**
 * @file test_conversation.cpp
 * @brief Unit tests for Conversation class
 */

#ifdef NATIVE_TEST

#include <unity.h>
#include "conversation/Conversation.h"

// --- Add/Get Messages Tests ---

void test_add_user_message() {
    ESPAI::Conversation conv;
    conv.addUserMessage("Hello");

    TEST_ASSERT_EQUAL(1, conv.size());
    TEST_ASSERT_EQUAL(ESPAI::Role::User, conv.getMessages()[0].role);
    TEST_ASSERT_EQUAL_STRING("Hello", conv.getMessages()[0].content.c_str());
}

void test_add_assistant_message() {
    ESPAI::Conversation conv;
    conv.addAssistantMessage("Hi there!");

    TEST_ASSERT_EQUAL(1, conv.size());
    TEST_ASSERT_EQUAL(ESPAI::Role::Assistant, conv.getMessages()[0].role);
    TEST_ASSERT_EQUAL_STRING("Hi there!", conv.getMessages()[0].content.c_str());
}

void test_add_message_with_role() {
    ESPAI::Conversation conv;
    conv.addMessage(ESPAI::Role::Tool, "Result: 42");

    TEST_ASSERT_EQUAL(1, conv.size());
    TEST_ASSERT_EQUAL(ESPAI::Role::Tool, conv.getMessages()[0].role);
}

void test_get_messages_returns_all() {
    ESPAI::Conversation conv;
    conv.addUserMessage("First");
    conv.addAssistantMessage("Second");
    conv.addUserMessage("Third");

    const auto& messages = conv.getMessages();
    TEST_ASSERT_EQUAL(3, messages.size());
    TEST_ASSERT_EQUAL_STRING("First", messages[0].content.c_str());
    TEST_ASSERT_EQUAL_STRING("Second", messages[1].content.c_str());
    TEST_ASSERT_EQUAL_STRING("Third", messages[2].content.c_str());
}

void test_size_reflects_message_count() {
    ESPAI::Conversation conv;
    TEST_ASSERT_EQUAL(0, conv.size());

    conv.addUserMessage("One");
    TEST_ASSERT_EQUAL(1, conv.size());

    conv.addAssistantMessage("Two");
    TEST_ASSERT_EQUAL(2, conv.size());
}

// --- Max Messages Pruning Tests ---

void test_default_max_messages() {
    ESPAI::Conversation conv;
    TEST_ASSERT_EQUAL(20, conv.getMaxMessages());
}

void test_custom_max_messages_constructor() {
    ESPAI::Conversation conv(5);
    TEST_ASSERT_EQUAL(5, conv.getMaxMessages());
}

void test_set_max_messages() {
    ESPAI::Conversation conv;
    conv.setMaxMessages(10);
    TEST_ASSERT_EQUAL(10, conv.getMaxMessages());
}

void test_pruning_removes_oldest() {
    ESPAI::Conversation conv(3);

    conv.addUserMessage("First");
    conv.addAssistantMessage("Second");
    conv.addUserMessage("Third");
    conv.addAssistantMessage("Fourth");

    TEST_ASSERT_EQUAL(3, conv.size());
    TEST_ASSERT_EQUAL_STRING("Second", conv.getMessages()[0].content.c_str());
    TEST_ASSERT_EQUAL_STRING("Third", conv.getMessages()[1].content.c_str());
    TEST_ASSERT_EQUAL_STRING("Fourth", conv.getMessages()[2].content.c_str());
}

void test_pruning_multiple_messages() {
    ESPAI::Conversation conv(2);

    conv.addUserMessage("A");
    conv.addAssistantMessage("B");
    conv.addUserMessage("C");
    conv.addAssistantMessage("D");
    conv.addUserMessage("E");

    TEST_ASSERT_EQUAL(2, conv.size());
    TEST_ASSERT_EQUAL_STRING("D", conv.getMessages()[0].content.c_str());
    TEST_ASSERT_EQUAL_STRING("E", conv.getMessages()[1].content.c_str());
}

void test_set_max_messages_triggers_pruning() {
    ESPAI::Conversation conv(10);

    for (int i = 0; i < 10; i++) {
        conv.addUserMessage(String(i));
    }
    TEST_ASSERT_EQUAL(10, conv.size());

    conv.setMaxMessages(5);
    TEST_ASSERT_EQUAL(5, conv.size());
    TEST_ASSERT_EQUAL_STRING("5", conv.getMessages()[0].content.c_str());
}

// --- System Prompt Handling Tests ---

void test_set_system_prompt() {
    ESPAI::Conversation conv;
    conv.setSystemPrompt("You are a helpful assistant");

    TEST_ASSERT_EQUAL_STRING("You are a helpful assistant", conv.getSystemPrompt().c_str());
}

void test_system_prompt_default_empty() {
    ESPAI::Conversation conv;
    TEST_ASSERT_TRUE(conv.getSystemPrompt().isEmpty());
}

void test_system_prompt_not_in_messages() {
    ESPAI::Conversation conv;
    conv.setSystemPrompt("System instruction");
    conv.addUserMessage("Hello");

    TEST_ASSERT_EQUAL(1, conv.size());
    TEST_ASSERT_EQUAL(ESPAI::Role::User, conv.getMessages()[0].role);
}

void test_system_prompt_preserved_after_clear() {
    ESPAI::Conversation conv;
    conv.setSystemPrompt("Keep this");
    conv.addUserMessage("Hello");
    conv.clear();

    TEST_ASSERT_EQUAL_STRING("Keep this", conv.getSystemPrompt().c_str());
}

// --- Clear Tests ---

void test_clear_removes_all_messages() {
    ESPAI::Conversation conv;
    conv.addUserMessage("Hello");
    conv.addAssistantMessage("Hi");
    conv.addUserMessage("Bye");

    conv.clear();

    TEST_ASSERT_EQUAL(0, conv.size());
    TEST_ASSERT_TRUE(conv.getMessages().empty());
}

void test_clear_on_empty_conversation() {
    ESPAI::Conversation conv;
    conv.clear();
    TEST_ASSERT_EQUAL(0, conv.size());
}

void test_clear_preserves_max_messages() {
    ESPAI::Conversation conv(5);
    conv.addUserMessage("Hello");
    conv.clear();

    TEST_ASSERT_EQUAL(5, conv.getMaxMessages());
}

// --- EstimateTokens Tests ---

void test_estimate_tokens_empty() {
    ESPAI::Conversation conv;
    TEST_ASSERT_EQUAL(0, conv.estimateTokens());
}

void test_estimate_tokens_system_prompt_only() {
    ESPAI::Conversation conv;
    conv.setSystemPrompt("1234567890123456");

    TEST_ASSERT_EQUAL(4, conv.estimateTokens());
}

void test_estimate_tokens_messages_only() {
    ESPAI::Conversation conv;
    conv.addUserMessage("12345678");

    TEST_ASSERT_EQUAL(2, conv.estimateTokens());
}

void test_estimate_tokens_combined() {
    ESPAI::Conversation conv;
    conv.setSystemPrompt("12345678");
    conv.addUserMessage("12345678");
    conv.addAssistantMessage("12345678");

    TEST_ASSERT_EQUAL(6, conv.estimateTokens());
}

void test_estimate_tokens_rough_calculation() {
    ESPAI::Conversation conv;
    conv.addUserMessage("Hello, how are you?");

    size_t tokens = conv.estimateTokens();
    TEST_ASSERT_TRUE(tokens >= 4 && tokens <= 5);
}

// --- Serialization Tests ---

void test_to_json_empty_conversation() {
    ESPAI::Conversation conv;
    String json = conv.toJson();

    TEST_ASSERT_TRUE(json.length() > 0);
    TEST_ASSERT_TRUE(json.find("systemPrompt") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("messages") != std::string::npos);
}

void test_to_json_with_system_prompt() {
    ESPAI::Conversation conv;
    conv.setSystemPrompt("Test prompt");

    String json = conv.toJson();
    TEST_ASSERT_TRUE(json.find("Test prompt") != std::string::npos);
}

void test_to_json_with_messages() {
    ESPAI::Conversation conv;
    conv.addUserMessage("Hello");
    conv.addAssistantMessage("Hi");

    String json = conv.toJson();
    TEST_ASSERT_TRUE(json.find("Hello") != std::string::npos);
    TEST_ASSERT_TRUE(json.find("Hi") != std::string::npos);
}

void test_from_json_basic() {
    ESPAI::Conversation conv;
    String json = R"({"systemPrompt":"Test","maxMessages":10,"messages":[]})";

    bool result = conv.fromJson(json);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING("Test", conv.getSystemPrompt().c_str());
    TEST_ASSERT_EQUAL(10, conv.getMaxMessages());
    TEST_ASSERT_EQUAL(0, conv.size());
}

void test_from_json_with_messages() {
    ESPAI::Conversation conv;
    String json = R"({"systemPrompt":"","maxMessages":20,"messages":[{"role":1,"content":"Hello"},{"role":2,"content":"Hi"}]})";

    bool result = conv.fromJson(json);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(2, conv.size());
    TEST_ASSERT_EQUAL(ESPAI::Role::User, conv.getMessages()[0].role);
    TEST_ASSERT_EQUAL_STRING("Hello", conv.getMessages()[0].content.c_str());
    TEST_ASSERT_EQUAL(ESPAI::Role::Assistant, conv.getMessages()[1].role);
    TEST_ASSERT_EQUAL_STRING("Hi", conv.getMessages()[1].content.c_str());
}

void test_from_json_invalid() {
    ESPAI::Conversation conv;
    bool result = conv.fromJson("invalid json{");

    TEST_ASSERT_FALSE(result);
}

void test_roundtrip_serialization() {
    ESPAI::Conversation original(15);
    original.setSystemPrompt("Be helpful");
    original.addUserMessage("Question");
    original.addAssistantMessage("Answer");

    String json = original.toJson();

    ESPAI::Conversation restored;
    bool result = restored.fromJson(json);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_STRING(original.getSystemPrompt().c_str(), restored.getSystemPrompt().c_str());
    TEST_ASSERT_EQUAL(original.getMaxMessages(), restored.getMaxMessages());
    TEST_ASSERT_EQUAL(original.size(), restored.size());
    TEST_ASSERT_EQUAL_STRING("Question", restored.getMessages()[0].content.c_str());
    TEST_ASSERT_EQUAL_STRING("Answer", restored.getMessages()[1].content.c_str());
}

void test_from_json_default_max_messages() {
    ESPAI::Conversation conv;
    String json = R"({"systemPrompt":"","messages":[]})";

    conv.fromJson(json);

    TEST_ASSERT_EQUAL(20, conv.getMaxMessages());
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    // Add/Get Messages Tests
    RUN_TEST(test_add_user_message);
    RUN_TEST(test_add_assistant_message);
    RUN_TEST(test_add_message_with_role);
    RUN_TEST(test_get_messages_returns_all);
    RUN_TEST(test_size_reflects_message_count);

    // Max Messages Pruning Tests
    RUN_TEST(test_default_max_messages);
    RUN_TEST(test_custom_max_messages_constructor);
    RUN_TEST(test_set_max_messages);
    RUN_TEST(test_pruning_removes_oldest);
    RUN_TEST(test_pruning_multiple_messages);
    RUN_TEST(test_set_max_messages_triggers_pruning);

    // System Prompt Handling Tests
    RUN_TEST(test_set_system_prompt);
    RUN_TEST(test_system_prompt_default_empty);
    RUN_TEST(test_system_prompt_not_in_messages);
    RUN_TEST(test_system_prompt_preserved_after_clear);

    // Clear Tests
    RUN_TEST(test_clear_removes_all_messages);
    RUN_TEST(test_clear_on_empty_conversation);
    RUN_TEST(test_clear_preserves_max_messages);

    // EstimateTokens Tests
    RUN_TEST(test_estimate_tokens_empty);
    RUN_TEST(test_estimate_tokens_system_prompt_only);
    RUN_TEST(test_estimate_tokens_messages_only);
    RUN_TEST(test_estimate_tokens_combined);
    RUN_TEST(test_estimate_tokens_rough_calculation);

    // Serialization Tests
    RUN_TEST(test_to_json_empty_conversation);
    RUN_TEST(test_to_json_with_system_prompt);
    RUN_TEST(test_to_json_with_messages);
    RUN_TEST(test_from_json_basic);
    RUN_TEST(test_from_json_with_messages);
    RUN_TEST(test_from_json_invalid);
    RUN_TEST(test_roundtrip_serialization);
    RUN_TEST(test_from_json_default_max_messages);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
