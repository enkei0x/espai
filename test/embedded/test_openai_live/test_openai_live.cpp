#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ESPAI.h"
#include "../secrets.h"

using namespace ESPAI;

String buildToolSchema(const char* propName, const char* propType, const char* description = nullptr, bool required = false) {
    JsonDocument doc;
    doc["type"] = "object";

    auto props = doc["properties"].to<JsonObject>();
    auto prop = props[propName].to<JsonObject>();
    prop["type"] = propType;
    if (description) {
        prop["description"] = description;
    }

    if (required) {
        auto req = doc["required"].to<JsonArray>();
        req.add(propName);
    }

    String output;
    serializeJson(doc, output);
    return output;
}

static OpenAIProvider* provider = nullptr;

void printHeapInfo(const char* label) {
    Serial.printf("[HEAP] %s: free=%u\n", label, ESP.getFreeHeap());
}

void connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;

    WiFi.mode(WIFI_STA);
    WiFi.begin(TEST_WIFI_SSID, TEST_WIFI_PASS);

    uint32_t timeout = millis() + 15000;
    while (WiFi.status() != WL_CONNECTED && millis() < timeout) {
        delay(100);
    }
}

void setUp() {
    printHeapInfo("setUp");
}

void tearDown() {
    printHeapInfo("tearDown");
}

void test_wifi_connection() {
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
    TEST_ASSERT_TRUE(WiFi.localIP()[0] != 0);
    Serial.printf("[INFO] Connected to %s, IP: %s\n", TEST_WIFI_SSID, WiFi.localIP().toString().c_str());
}

void test_provider_configured() {
    TEST_ASSERT_NOT_NULL(provider);
    TEST_ASSERT_TRUE(provider->isConfigured());
    TEST_ASSERT_EQUAL_STRING("OpenAI", provider->getName());
    TEST_ASSERT_EQUAL(Provider::OpenAI, provider->getType());
}

void test_basic_chat_request() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is 2+2? Reply with just the number."));

    ChatOptions options;
    options.maxTokens = 10;
    options.temperature = 0.0f;

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] HTTP: %d, Success: %d\n", resp.httpStatus, resp.success);
    Serial.printf("[INFO] Content: %s\n", resp.content.c_str());
    Serial.printf("[INFO] Tokens: prompt=%u, completion=%u\n", resp.promptTokens, resp.completionTokens);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL(200, resp.httpStatus);
    TEST_ASSERT_FALSE(resp.content.isEmpty());
    TEST_ASSERT_TRUE(resp.content.indexOf("4") >= 0);
    TEST_ASSERT_GREATER_THAN(0, resp.promptTokens);
    TEST_ASSERT_GREATER_THAN(0, resp.completionTokens);
}

void test_chat_with_system_prompt() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are a pirate. Always respond in pirate speak."));
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.maxTokens = 50;

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] Content: %s\n", resp.content.c_str());

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_FALSE(resp.content.isEmpty());
}

#if ESPAI_ENABLE_STREAMING
void test_streaming_response() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Count from 1 to 5, one number per line."));

    ChatOptions options;
    options.maxTokens = 30;

    String accumulated;
    int chunkCount = 0;
    bool completed = false;

    bool success = provider->chatStream(messages, options, [&](const String& chunk, bool done) {
        if (!chunk.isEmpty()) {
            accumulated += chunk;
            chunkCount++;
            Serial.printf("[STREAM] Chunk %d: %s\n", chunkCount, chunk.c_str());
        }
        if (done) {
            completed = true;
        }
    });

    Serial.printf("[INFO] Streaming: success=%d, chunks=%d, completed=%d\n", success, chunkCount, completed);
    Serial.printf("[INFO] Accumulated: %s\n", accumulated.c_str());

    TEST_ASSERT_TRUE(success);
    TEST_ASSERT_TRUE(completed);
    TEST_ASSERT_GREATER_THAN(0, chunkCount);
    TEST_ASSERT_FALSE(accumulated.isEmpty());
}
#endif

#if ESPAI_ENABLE_TOOLS
void test_tool_calling() {
    Tool weatherTool;
    weatherTool.name = "get_weather";
    weatherTool.description = "Get the current weather for a location";
    weatherTool.parametersJson = buildToolSchema("location", "string", "City name", true);

    provider->addTool(weatherTool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather in Tokyo?"));

    ChatOptions options;
    options.maxTokens = 100;

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] Tool call response: success=%d\n", resp.success);
    Serial.printf("[INFO] Has tool calls: %d\n", provider->hasToolCalls());

    TEST_ASSERT_TRUE(resp.success);

    if (provider->hasToolCalls()) {
        const auto& calls = provider->getLastToolCalls();
        TEST_ASSERT_GREATER_THAN(0, calls.size());
        TEST_ASSERT_EQUAL_STRING("get_weather", calls[0].name.c_str());
        Serial.printf("[INFO] Tool: %s, Args: %s\n", calls[0].name.c_str(), calls[0].arguments.c_str());
    }

    provider->clearTools();
}
#endif

#if ESPAI_ENABLE_STREAMING && ESPAI_ENABLE_TOOLS
void test_streaming_tool_calling() {
    Tool weatherTool;
    weatherTool.name = "get_weather";
    weatherTool.description = "Get the current weather for a location";
    weatherTool.parametersJson = buildToolSchema("location", "string", "City name", true);

    provider->addTool(weatherTool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather in Tokyo?"));

    ChatOptions options;
    options.maxTokens = 100;

    String accumulated;
    int chunkCount = 0;
    bool completed = false;

    bool success = provider->chatStream(messages, options, [&](const String& chunk, bool done) {
        if (!chunk.isEmpty()) {
            accumulated += chunk;
            chunkCount++;
            Serial.printf("[STREAM] Chunk %d: %s\n", chunkCount, chunk.c_str());
        }
        if (done) {
            completed = true;
        }
    });

    Serial.printf("[INFO] Stream tool call: success=%d, chunks=%d, completed=%d\n", success, chunkCount, completed);
    Serial.printf("[INFO] Has tool calls: %d\n", provider->hasToolCalls());

    String debugMsg = "Stream tool call failed: success=" + String(success) + " completed=" + String(completed);
    TEST_ASSERT_TRUE_MESSAGE(success, debugMsg.c_str());
    TEST_ASSERT_TRUE_MESSAGE(completed, "Stream did not complete");
    TEST_ASSERT_TRUE_MESSAGE(provider->hasToolCalls(), "Expected tool calls after streaming");

    const auto& calls = provider->getLastToolCalls();
    TEST_ASSERT_GREATER_THAN(0, calls.size());
    TEST_ASSERT_EQUAL_STRING("get_weather", calls[0].name.c_str());
    TEST_ASSERT_TRUE_MESSAGE(!calls[0].arguments.isEmpty(), "Tool call arguments should not be empty");
    Serial.printf("[INFO] Tool: %s, Args: %s\n", calls[0].name.c_str(), calls[0].arguments.c_str());

    provider->clearTools();
}
#endif

void test_error_handling_empty_message() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, ""));

    ChatOptions options;
    options.maxTokens = 10;

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] Empty message: success=%d, error=%s\n", resp.success, resp.errorMessage.c_str());
}

void test_response_parsing_validation() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Reply with exactly: TEST_RESPONSE_OK"));

    ChatOptions options;
    options.maxTokens = 20;
    options.temperature = 0.0f;

    Response resp = provider->chat(messages, options);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_FALSE(resp.content.isEmpty());
    TEST_ASSERT_EQUAL(ErrorCode::None, resp.error);
    TEST_ASSERT_TRUE(resp.errorMessage.isEmpty());
}

void test_model_override() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Say hi"));

    ChatOptions options;
    options.maxTokens = 5;
    options.model = "gpt-4.1-mini";

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] Model override: success=%d\n", resp.success);
    TEST_ASSERT_TRUE(resp.success);
}

void test_timeout_handling() {
    provider->setTimeout(1);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello"));

    ChatOptions options;
    options.maxTokens = 5;

    Response resp = provider->chat(messages, options);

    provider->setTimeout(30000);

    Serial.printf("[INFO] Timeout test: success=%d, error=%d\n", resp.success, (int)resp.error);
}

void test_max_tokens_respected() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Write a very long story about a cat."));

    ChatOptions options;
    options.maxTokens = 10;

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] Max tokens: completion=%u\n", resp.completionTokens);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_LESS_OR_EQUAL(20, resp.completionTokens);
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    Serial.println("\n=== ESPAI OpenAI Live Tests ===");
    printHeapInfo("Initial");

    connectWiFi();

    provider = new OpenAIProvider(TEST_OPENAI_KEY, "gpt-4.1-mini");
    provider->setTimeout(30000);

    UNITY_BEGIN();

    RUN_TEST(test_wifi_connection);
    RUN_TEST(test_provider_configured);
    RUN_TEST(test_basic_chat_request);
    RUN_TEST(test_chat_with_system_prompt);
#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_streaming_response);
#endif
#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_tool_calling);
#endif
#if ESPAI_ENABLE_STREAMING && ESPAI_ENABLE_TOOLS
    RUN_TEST(test_streaming_tool_calling);
#endif
    RUN_TEST(test_error_handling_empty_message);
    RUN_TEST(test_response_parsing_validation);
    RUN_TEST(test_model_override);
    RUN_TEST(test_timeout_handling);
    RUN_TEST(test_max_tokens_respected);

    UNITY_END();

    delete provider;
    printHeapInfo("Final");
}

void loop() {
    delay(1000);
}
