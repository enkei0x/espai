#ifdef NATIVE_TEST

#include <unity.h>
#include "core/AIClient.h"
#include "providers/ProviderFactory.h"
#include "providers/OpenAIProvider.h"
#include "providers/AnthropicProvider.h"

using namespace ESPAI;

void setUp() {
    ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);
    ProviderFactory::registerProvider(Provider::Anthropic, createAnthropicProvider);
}

void tearDown() {
}

void test_default_constructor() {
    AIClient client;
    TEST_ASSERT_FALSE(client.isConfigured());
    TEST_ASSERT_EQUAL(Provider::OpenAI, client.getProvider());
}

void test_parameterized_constructor() {
    AIClient client(Provider::OpenAI, "sk-test-key");
    TEST_ASSERT_TRUE(client.isConfigured());
    TEST_ASSERT_EQUAL(Provider::OpenAI, client.getProvider());
}

void test_constructor_with_anthropic() {
    AIClient client(Provider::Anthropic, "sk-ant-test");
    TEST_ASSERT_TRUE(client.isConfigured());
    TEST_ASSERT_EQUAL(Provider::Anthropic, client.getProvider());
}

void test_set_provider_openai() {
    AIClient client;
    bool result = client.setProvider(Provider::OpenAI, "sk-test");
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_TRUE(client.isConfigured());
    TEST_ASSERT_EQUAL(Provider::OpenAI, client.getProvider());
}

void test_set_provider_requires_api_key() {
    AIClient client;
    bool result = client.setProvider(Provider::OpenAI, "");
    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_FALSE(client.isConfigured());
}

void test_set_model() {
    AIClient client(Provider::OpenAI, "sk-test");
    client.setModel("gpt-4");
    TEST_ASSERT_EQUAL_STRING("gpt-4", client.getModel().c_str());
}

void test_default_model_openai() {
    AIClient client(Provider::OpenAI, "sk-test");
    const String& model = client.getModel();
    TEST_ASSERT_EQUAL_STRING(ESPAI_DEFAULT_MODEL_OPENAI, model.c_str());
}

void test_default_model_anthropic() {
    AIClient client(Provider::Anthropic, "sk-ant-test");
    const String& model = client.getModel();
    TEST_ASSERT_EQUAL_STRING(ESPAI_DEFAULT_MODEL_ANTHROPIC, model.c_str());
}

void test_set_timeout() {
    AIClient client(Provider::OpenAI, "sk-test");
    client.setTimeout(60000);
    TEST_ASSERT_TRUE(client.isConfigured());
}

void test_chat_unconfigured_returns_error() {
    AIClient client;
    Response resp = client.chat("Hello");
    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::NotConfigured, resp.error);
}

void test_chat_empty_message_returns_error() {
    AIClient client(Provider::OpenAI, "sk-test");
    Response resp = client.chat("");
    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::InvalidRequest, resp.error);
}

void test_chat_with_options() {
    AIClient client(Provider::OpenAI, "sk-test");
    ChatOptions options;
    options.temperature = 0.5f;
    options.maxTokens = 500;
    // In native build, HTTP is not available so we expect failure
    Response resp = client.chat("Hello", options);
    TEST_ASSERT_FALSE(resp.success);
}

void test_chat_with_system_prompt() {
    AIClient client(Provider::OpenAI, "sk-test");
    // In native build, HTTP is not available so we expect failure
    Response resp = client.chat("You are helpful", "Hello");
    TEST_ASSERT_FALSE(resp.success);
}

#if ESPAI_ENABLE_STREAMING
void test_stream_unconfigured_returns_error() {
    AIClient client;
    bool callbackInvoked = false;
    Response resp = client.chatStream("Hello", [&](const String& chunk, bool done) {
        (void)chunk;
        (void)done;
        callbackInvoked = true;
    });
    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::NotConfigured, resp.error);
    TEST_ASSERT_FALSE(callbackInvoked);
}

void test_stream_null_callback_returns_error() {
    AIClient client(Provider::OpenAI, "sk-test");
    Response resp = client.chatStream("Hello", nullptr);
    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::InvalidRequest, resp.error);
}

void test_stream_empty_message_returns_error() {
    AIClient client(Provider::OpenAI, "sk-test");
    Response resp = client.chatStream("", [](const String& chunk, bool done) {
        (void)chunk;
        (void)done;
    });
    TEST_ASSERT_FALSE(resp.success);
    TEST_ASSERT_EQUAL(ErrorCode::InvalidRequest, resp.error);
}
#endif

void test_get_last_error() {
    AIClient client;
    client.chat("Hello");
    TEST_ASSERT_FALSE(client.getLastError().isEmpty());
}

void test_get_last_http_status_default() {
    AIClient client;
    TEST_ASSERT_EQUAL(0, client.getLastHttpStatus());
}

void test_reset_clears_state() {
    AIClient client;
    client.chat("Hello");
    client.reset();
    TEST_ASSERT_TRUE(client.getLastError().isEmpty());
    TEST_ASSERT_EQUAL(0, client.getLastHttpStatus());
}

void test_switch_provider() {
    AIClient client(Provider::OpenAI, "sk-openai");
    TEST_ASSERT_EQUAL(Provider::OpenAI, client.getProvider());
    client.setProvider(Provider::Anthropic, "sk-anthropic");
    TEST_ASSERT_EQUAL(Provider::Anthropic, client.getProvider());
    TEST_ASSERT_TRUE(client.isConfigured());
}

void test_switch_provider_resets_model() {
    AIClient client(Provider::OpenAI, "sk-openai");
    client.setModel("gpt-4");
    client.setProvider(Provider::Anthropic, "sk-anthropic");
    TEST_ASSERT_EQUAL_STRING(ESPAI_DEFAULT_MODEL_ANTHROPIC, client.getModel().c_str());
}

void test_get_provider_instance() {
    AIClient client(Provider::OpenAI, "sk-test");
    TEST_ASSERT_NOT_NULL(client.getProviderInstance());
    TEST_ASSERT_EQUAL_STRING("OpenAI", client.getProviderInstance()->getName());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_default_constructor);
    RUN_TEST(test_parameterized_constructor);
    RUN_TEST(test_constructor_with_anthropic);

    RUN_TEST(test_set_provider_openai);
    RUN_TEST(test_set_provider_requires_api_key);
    RUN_TEST(test_set_model);
    RUN_TEST(test_default_model_openai);
    RUN_TEST(test_default_model_anthropic);
    RUN_TEST(test_set_timeout);

    RUN_TEST(test_chat_unconfigured_returns_error);
    RUN_TEST(test_chat_empty_message_returns_error);
    RUN_TEST(test_chat_with_options);
    RUN_TEST(test_chat_with_system_prompt);

#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_stream_unconfigured_returns_error);
    RUN_TEST(test_stream_null_callback_returns_error);
    RUN_TEST(test_stream_empty_message_returns_error);
#endif

    RUN_TEST(test_get_last_error);
    RUN_TEST(test_get_last_http_status_default);
    RUN_TEST(test_reset_clears_state);

    RUN_TEST(test_switch_provider);
    RUN_TEST(test_switch_provider_resets_model);
    RUN_TEST(test_get_provider_instance);

    return UNITY_END();
}

#else
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
