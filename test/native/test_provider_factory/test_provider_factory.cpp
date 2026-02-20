#ifdef NATIVE_TEST

#include <unity.h>
#include "providers/AIProvider.h"
#include "providers/ProviderFactory.h"

class MockProvider : public ESPAI::AIProvider {
public:
    MockProvider(const String& apiKey, const String& model) {
        _apiKey = apiKey;
        _model = model;
        _baseUrl = "https://api.openai.com/v1/chat/completions";
    }

    const char* getName() const override { return "MockProvider"; }
    ESPAI::Provider getType() const override { return ESPAI::Provider::OpenAI; }

#if ESPAI_ENABLE_TOOLS
    ESPAI::Message getAssistantMessageWithToolCalls(const String& content = "") const override {
        return ESPAI::Message(ESPAI::Role::Assistant, content);
    }
#endif

protected:
    String buildRequestBody(
        const std::vector<ESPAI::Message>& messages,
        const ESPAI::ChatOptions& options
    ) override {
        (void)messages;
        (void)options;
        return "{}";
    }

    ESPAI::Response parseResponse(const String& json) override {
        (void)json;
        return ESPAI::Response::ok("parsed");
    }

#if ESPAI_ENABLE_STREAMING
    bool parseStreamChunk(const String& chunk, String& content, bool& done) override {
        (void)chunk;
        content = "";
        done = false;
        return true;
    }
#endif
};

std::unique_ptr<ESPAI::AIProvider> createMockProvider(const String& apiKey, const String& model) {
    return std::make_unique<MockProvider>(apiKey, model);
}

void setUp() {
    ESPAI::ProviderFactory::unregisterProvider(ESPAI::Provider::OpenAI);
    ESPAI::ProviderFactory::unregisterProvider(ESPAI::Provider::Anthropic);
    ESPAI::ProviderFactory::unregisterProvider(ESPAI::Provider::Gemini);
    ESPAI::ProviderFactory::unregisterProvider(ESPAI::Provider::Ollama);
}

void tearDown() {
}

void test_register_provider() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    TEST_ASSERT_TRUE(ESPAI::ProviderFactory::isRegistered(ESPAI::Provider::OpenAI));
}

void test_unregister_provider() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    ESPAI::ProviderFactory::unregisterProvider(ESPAI::Provider::OpenAI);
    TEST_ASSERT_FALSE(ESPAI::ProviderFactory::isRegistered(ESPAI::Provider::OpenAI));
}

void test_create_registered_provider() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "test-api-key", "gpt-4");

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING("test-api-key", provider->getApiKey().c_str());
    TEST_ASSERT_EQUAL_STRING("gpt-4", provider->getModel().c_str());
}

void test_create_unregistered_provider_returns_null() {
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::Anthropic, "key", "model");
    TEST_ASSERT_NULL(provider.get());
}

void test_create_with_default_model() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "api-key");

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING(ESPAI_DEFAULT_MODEL_OPENAI, provider->getModel().c_str());
}

void test_create_with_custom_base_url() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(
        ESPAI::Provider::OpenAI, "api-key", "gpt-4", "https://custom.api.com/v1"
    );

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING("https://custom.api.com/v1", provider->getBaseUrl().c_str());
}

void test_create_sets_default_base_url() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "api-key", "gpt-4");

    TEST_ASSERT_NOT_NULL(provider.get());
    TEST_ASSERT_EQUAL_STRING(
        "https://api.openai.com/v1/chat/completions",
        provider->getBaseUrl().c_str()
    );
}

void test_get_default_model_openai() {
    TEST_ASSERT_EQUAL_STRING(
        ESPAI_DEFAULT_MODEL_OPENAI,
        ESPAI::ProviderFactory::getDefaultModel(ESPAI::Provider::OpenAI)
    );
}

void test_get_default_model_anthropic() {
    TEST_ASSERT_EQUAL_STRING(
        ESPAI_DEFAULT_MODEL_ANTHROPIC,
        ESPAI::ProviderFactory::getDefaultModel(ESPAI::Provider::Anthropic)
    );
}

void test_get_default_model_gemini() {
    TEST_ASSERT_EQUAL_STRING(
        ESPAI_DEFAULT_MODEL_GEMINI,
        ESPAI::ProviderFactory::getDefaultModel(ESPAI::Provider::Gemini)
    );
}

void test_get_default_model_ollama() {
    TEST_ASSERT_EQUAL_STRING(
        ESPAI_DEFAULT_MODEL_OLLAMA,
        ESPAI::ProviderFactory::getDefaultModel(ESPAI::Provider::Ollama)
    );
}

void test_get_default_base_url_openai() {
    TEST_ASSERT_EQUAL_STRING(
        "https://api.openai.com/v1/chat/completions",
        ESPAI::ProviderFactory::getDefaultBaseUrl(ESPAI::Provider::OpenAI)
    );
}

void test_get_default_base_url_anthropic() {
    TEST_ASSERT_EQUAL_STRING(
        "https://api.anthropic.com/v1/messages",
        ESPAI::ProviderFactory::getDefaultBaseUrl(ESPAI::Provider::Anthropic)
    );
}

void test_get_default_base_url_ollama() {
    TEST_ASSERT_EQUAL_STRING(
        "http://localhost:11434/api/chat",
        ESPAI::ProviderFactory::getDefaultBaseUrl(ESPAI::Provider::Ollama)
    );
}

void test_is_registered_false_initially() {
    TEST_ASSERT_FALSE(ESPAI::ProviderFactory::isRegistered(ESPAI::Provider::Gemini));
}

void test_multiple_providers() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::Anthropic, createMockProvider);

    TEST_ASSERT_TRUE(ESPAI::ProviderFactory::isRegistered(ESPAI::Provider::OpenAI));
    TEST_ASSERT_TRUE(ESPAI::ProviderFactory::isRegistered(ESPAI::Provider::Anthropic));
    TEST_ASSERT_FALSE(ESPAI::ProviderFactory::isRegistered(ESPAI::Provider::Gemini));
}

void test_provider_is_configured() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "api-key", "gpt-4");

    TEST_ASSERT_TRUE(provider->isConfigured());
}

void test_provider_not_configured_without_api_key() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "", "gpt-4");

    TEST_ASSERT_FALSE(provider->isConfigured());
}

void test_provider_name() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "key", "model");

    TEST_ASSERT_EQUAL_STRING("MockProvider", provider->getName());
}

void test_provider_type() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "key", "model");

    TEST_ASSERT_EQUAL(ESPAI::Provider::OpenAI, provider->getType());
}

void test_provider_set_timeout() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "key", "model");

    provider->setTimeout(60000);
    TEST_ASSERT_EQUAL(60000, provider->getTimeout());
}

void test_provider_supports_streaming() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "key", "model");

    TEST_ASSERT_TRUE(provider->supportsStreaming());
}

void test_provider_default_supports_tools_false() {
    ESPAI::ProviderFactory::registerProvider(ESPAI::Provider::OpenAI, createMockProvider);
    auto provider = ESPAI::ProviderFactory::create(ESPAI::Provider::OpenAI, "key", "model");

    TEST_ASSERT_FALSE(provider->supportsTools());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_register_provider);
    RUN_TEST(test_unregister_provider);
    RUN_TEST(test_create_registered_provider);
    RUN_TEST(test_create_unregistered_provider_returns_null);
    RUN_TEST(test_create_with_default_model);
    RUN_TEST(test_create_with_custom_base_url);
    RUN_TEST(test_create_sets_default_base_url);

    RUN_TEST(test_get_default_model_openai);
    RUN_TEST(test_get_default_model_anthropic);
    RUN_TEST(test_get_default_model_gemini);
    RUN_TEST(test_get_default_model_ollama);

    RUN_TEST(test_get_default_base_url_openai);
    RUN_TEST(test_get_default_base_url_anthropic);
    RUN_TEST(test_get_default_base_url_ollama);

    RUN_TEST(test_is_registered_false_initially);
    RUN_TEST(test_multiple_providers);

    RUN_TEST(test_provider_is_configured);
    RUN_TEST(test_provider_not_configured_without_api_key);
    RUN_TEST(test_provider_name);
    RUN_TEST(test_provider_type);
    RUN_TEST(test_provider_set_timeout);
    RUN_TEST(test_provider_supports_streaming);
    RUN_TEST(test_provider_default_supports_tools_false);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
