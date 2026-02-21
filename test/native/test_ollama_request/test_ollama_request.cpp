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

void test_basic_request_body() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.temperature = 0.7f;
    options.maxTokens = 100;

    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"model\":\"llama3.2\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"messages\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"Hello\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"temperature\":0.7") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"max_tokens\":100") != std::string::npos);
}

void test_no_authorization_header() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = provider->buildHttpRequest(messages, options);

    for (const auto& header : req.headers) {
        TEST_ASSERT_TRUE(header.first != "Authorization");
    }
}

void test_url_is_ollama_endpoint() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("http://localhost:11434/v1/chat/completions", req.url.c_str());
}

void test_is_configured_without_api_key() {
    TEST_ASSERT_TRUE(provider->isConfigured());
}

void test_is_configured_with_api_key() {
    OllamaProvider p("optional-key", "llama3.2");
    TEST_ASSERT_TRUE(p.isConfigured());
}

void test_empty_model_gets_default() {
    OllamaProvider p("", "");
    TEST_ASSERT_EQUAL_STRING(ESPAI_DEFAULT_MODEL_OLLAMA, p.getModel().c_str());
    TEST_ASSERT_TRUE(p.isConfigured());
}

void test_get_name() {
    TEST_ASSERT_EQUAL_STRING("Ollama", provider->getName());
}

void test_get_type() {
    TEST_ASSERT_EQUAL(Provider::Ollama, provider->getType());
}

void test_supports_tools() {
    TEST_ASSERT_TRUE(provider->supportsTools());
}

void test_default_model() {
    OllamaProvider p;
    TEST_ASSERT_EQUAL_STRING(ESPAI_DEFAULT_MODEL_OLLAMA, p.getModel().c_str());
}

void test_custom_model() {
    OllamaProvider p("", "mistral");
    TEST_ASSERT_EQUAL_STRING("mistral", p.getModel().c_str());
}

void test_request_method_is_post() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = provider->buildHttpRequest(messages, options);

    TEST_ASSERT_EQUAL_STRING("POST", req.method.c_str());
}

void test_multiple_messages() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are helpful"));
    messages.push_back(Message(Role::User, "Hi"));
    messages.push_back(Message(Role::Assistant, "Hello!"));
    messages.push_back(Message(Role::User, "How are you?"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"role\":\"system\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"user\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"role\":\"assistant\"") != std::string::npos);
}

#if ESPAI_ENABLE_TOOLS
void test_with_tools_same_as_openai() {
    Tool tool;
    tool.name = "get_weather";
    tool.description = "Get current weather";
    tool.parametersJson = "{\"type\":\"object\",\"properties\":{\"location\":{\"type\":\"string\"}}}";

    provider->addTool(tool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather?"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"tools\":[") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"type\":\"function\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"name\":\"get_weather\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"description\":\"Get current weather\"") != std::string::npos);

    provider->clearTools();
}

void test_tool_message_format() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::Tool, "25 degrees", "call_abc123"));

    ChatOptions options;
    String body = provider->buildRequestBody(messages, options);

    TEST_ASSERT_TRUE(body.find("\"role\":\"tool\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"tool_call_id\":\"call_abc123\"") != std::string::npos);
    TEST_ASSERT_TRUE(body.find("\"content\":\"25 degrees\"") != std::string::npos);
}
#endif

void test_custom_base_url() {
    OllamaProvider p("", "llama3.2");
    p.setBaseUrl("http://192.168.1.100:11434/v1/chat/completions");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Test"));
    ChatOptions options;

    HttpRequest req = p.buildHttpRequest(messages, options);
    TEST_ASSERT_EQUAL_STRING("http://192.168.1.100:11434/v1/chat/completions", req.url.c_str());
}

#if ESPAI_ENABLE_STREAMING
void test_sse_format_is_openai() {
    TEST_ASSERT_EQUAL(SSEFormat::OpenAI, provider->getSSEFormat());
}
#endif

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_basic_request_body);
    RUN_TEST(test_no_authorization_header);
    RUN_TEST(test_url_is_ollama_endpoint);
    RUN_TEST(test_is_configured_without_api_key);
    RUN_TEST(test_is_configured_with_api_key);
    RUN_TEST(test_empty_model_gets_default);
    RUN_TEST(test_get_name);
    RUN_TEST(test_get_type);
    RUN_TEST(test_supports_tools);
    RUN_TEST(test_default_model);
    RUN_TEST(test_custom_model);
    RUN_TEST(test_request_method_is_post);
    RUN_TEST(test_multiple_messages);

#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_with_tools_same_as_openai);
    RUN_TEST(test_tool_message_format);
#endif

    RUN_TEST(test_custom_base_url);

#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_sse_format_is_openai);
#endif

    return UNITY_END();
}

#else
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
