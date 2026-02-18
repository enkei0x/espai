/**
 * @file test_types.cpp
 * @brief Unit tests for AITypes.h
 */

#ifdef NATIVE_TEST

#include <unity.h>
#include "core/AITypes.h"

void test_message_default_constructor() {
    ESPAI::Message msg;
    TEST_ASSERT_EQUAL(ESPAI::Role::User, msg.role);
    TEST_ASSERT_TRUE(msg.content.isEmpty());
    TEST_ASSERT_TRUE(msg.name.isEmpty());
}

void test_message_two_param_constructor() {
    ESPAI::Message msg(ESPAI::Role::Assistant, "Hello world");
    TEST_ASSERT_EQUAL(ESPAI::Role::Assistant, msg.role);
    TEST_ASSERT_EQUAL_STRING("Hello world", msg.content.c_str());
    TEST_ASSERT_TRUE(msg.name.isEmpty());
}

void test_message_three_param_constructor() {
    ESPAI::Message msg(ESPAI::Role::Tool, "Result: 42", "get_temperature");
    TEST_ASSERT_EQUAL(ESPAI::Role::Tool, msg.role);
    TEST_ASSERT_EQUAL_STRING("Result: 42", msg.content.c_str());
    TEST_ASSERT_EQUAL_STRING("get_temperature", msg.name.c_str());
}

void test_message_role_system() {
    ESPAI::Message msg;
    msg.role = ESPAI::Role::System;
    msg.content = "You are a helpful assistant";
    TEST_ASSERT_EQUAL(ESPAI::Role::System, msg.role);
}

void test_response_default_values() {
    ESPAI::Response resp;
    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ESPAI::ErrorCode::None, resp.error);
    TEST_ASSERT_EQUAL(0, resp.httpStatus);
    TEST_ASSERT_EQUAL(0, resp.promptTokens);
    TEST_ASSERT_EQUAL(0, resp.completionTokens);
    TEST_ASSERT_TRUE(resp.content.isEmpty());
    TEST_ASSERT_TRUE(resp.errorMessage.isEmpty());
}

void test_response_ok_factory() {
    ESPAI::Response resp = ESPAI::Response::ok("AI response text");
    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL_STRING("AI response text", resp.content.c_str());
    TEST_ASSERT_EQUAL(ESPAI::ErrorCode::None, resp.error);
}

void test_response_fail_factory() {
    ESPAI::Response resp = ESPAI::Response::fail(
        ESPAI::ErrorCode::AuthError,
        "Invalid API key",
        401
    );
    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ESPAI::ErrorCode::AuthError, resp.error);
    TEST_ASSERT_EQUAL_STRING("Invalid API key", resp.errorMessage.c_str());
    TEST_ASSERT_EQUAL(401, resp.httpStatus);
}

void test_response_total_tokens() {
    ESPAI::Response resp;
    resp.promptTokens = 100;
    resp.completionTokens = 50;
    TEST_ASSERT_EQUAL(150, resp.totalTokens());
}

void test_response_total_tokens_zero() {
    ESPAI::Response resp;
    TEST_ASSERT_EQUAL(0, resp.totalTokens());
}

void test_chat_options_defaults() {
    ESPAI::ChatOptions opts;
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.7f, opts.temperature);
    TEST_ASSERT_EQUAL(1024, opts.maxTokens);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, opts.topP);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, opts.frequencyPenalty);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, opts.presencePenalty);
    TEST_ASSERT_TRUE(opts.model.isEmpty());
    TEST_ASSERT_TRUE(opts.systemPrompt.isEmpty());
}

void test_chat_options_custom_values() {
    ESPAI::ChatOptions opts;
    opts.temperature = 0.9f;
    opts.maxTokens = 2048;
    opts.model = "gpt-4";
    opts.systemPrompt = "You are helpful";
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.9f, opts.temperature);
    TEST_ASSERT_EQUAL(2048, opts.maxTokens);
    TEST_ASSERT_EQUAL_STRING("gpt-4", opts.model.c_str());
    TEST_ASSERT_EQUAL_STRING("You are helpful", opts.systemPrompt.c_str());
}

void test_provider_enum_values() {
    TEST_ASSERT_EQUAL(0, static_cast<int>(ESPAI::Provider::OpenAI));
    TEST_ASSERT_EQUAL(1, static_cast<int>(ESPAI::Provider::Anthropic));
    TEST_ASSERT_EQUAL(2, static_cast<int>(ESPAI::Provider::Gemini));
    TEST_ASSERT_EQUAL(3, static_cast<int>(ESPAI::Provider::Ollama));
    TEST_ASSERT_EQUAL(4, static_cast<int>(ESPAI::Provider::Custom));
}

void test_provider_to_string() {
    TEST_ASSERT_EQUAL_STRING("OpenAI", ESPAI::providerToString(ESPAI::Provider::OpenAI));
    TEST_ASSERT_EQUAL_STRING("Anthropic", ESPAI::providerToString(ESPAI::Provider::Anthropic));
    TEST_ASSERT_EQUAL_STRING("Gemini", ESPAI::providerToString(ESPAI::Provider::Gemini));
    TEST_ASSERT_EQUAL_STRING("Ollama", ESPAI::providerToString(ESPAI::Provider::Ollama));
    TEST_ASSERT_EQUAL_STRING("Custom", ESPAI::providerToString(ESPAI::Provider::Custom));
}

void test_role_enum_values() {
    TEST_ASSERT_EQUAL(0, static_cast<int>(ESPAI::Role::System));
    TEST_ASSERT_EQUAL(1, static_cast<int>(ESPAI::Role::User));
    TEST_ASSERT_EQUAL(2, static_cast<int>(ESPAI::Role::Assistant));
    TEST_ASSERT_EQUAL(3, static_cast<int>(ESPAI::Role::Tool));
}

void test_role_to_string() {
    TEST_ASSERT_EQUAL_STRING("system", ESPAI::roleToString(ESPAI::Role::System));
    TEST_ASSERT_EQUAL_STRING("user", ESPAI::roleToString(ESPAI::Role::User));
    TEST_ASSERT_EQUAL_STRING("assistant", ESPAI::roleToString(ESPAI::Role::Assistant));
    TEST_ASSERT_EQUAL_STRING("tool", ESPAI::roleToString(ESPAI::Role::Tool));
}

void test_error_code_enum_values() {
    TEST_ASSERT_EQUAL(0, static_cast<int>(ESPAI::ErrorCode::None));
    TEST_ASSERT_EQUAL(1, static_cast<int>(ESPAI::ErrorCode::NetworkError));
    TEST_ASSERT_EQUAL(2, static_cast<int>(ESPAI::ErrorCode::Timeout));
    TEST_ASSERT_EQUAL(3, static_cast<int>(ESPAI::ErrorCode::AuthError));
    TEST_ASSERT_EQUAL(4, static_cast<int>(ESPAI::ErrorCode::RateLimited));
}

void test_error_code_to_string() {
    TEST_ASSERT_EQUAL_STRING("None", ESPAI::errorCodeToString(ESPAI::ErrorCode::None));
    TEST_ASSERT_EQUAL_STRING("NetworkError", ESPAI::errorCodeToString(ESPAI::ErrorCode::NetworkError));
    TEST_ASSERT_EQUAL_STRING("AuthError", ESPAI::errorCodeToString(ESPAI::ErrorCode::AuthError));
    TEST_ASSERT_EQUAL_STRING("RateLimited", ESPAI::errorCodeToString(ESPAI::ErrorCode::RateLimited));
    TEST_ASSERT_EQUAL_STRING("ParseError", ESPAI::errorCodeToString(ESPAI::ErrorCode::ParseError));
    TEST_ASSERT_EQUAL_STRING("OutOfMemory", ESPAI::errorCodeToString(ESPAI::ErrorCode::OutOfMemory));
}

int main(int argc, char** argv) {
    UNITY_BEGIN();

    RUN_TEST(test_message_default_constructor);
    RUN_TEST(test_message_two_param_constructor);
    RUN_TEST(test_message_three_param_constructor);
    RUN_TEST(test_message_role_system);

    RUN_TEST(test_response_default_values);
    RUN_TEST(test_response_ok_factory);
    RUN_TEST(test_response_fail_factory);
    RUN_TEST(test_response_total_tokens);
    RUN_TEST(test_response_total_tokens_zero);

    RUN_TEST(test_chat_options_defaults);
    RUN_TEST(test_chat_options_custom_values);

    RUN_TEST(test_provider_enum_values);
    RUN_TEST(test_provider_to_string);

    RUN_TEST(test_role_enum_values);
    RUN_TEST(test_role_to_string);

    RUN_TEST(test_error_code_enum_values);
    RUN_TEST(test_error_code_to_string);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
