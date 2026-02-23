#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include "ESPAI.h"
#include "../secrets.h"

using namespace ESPAI;

// Ollama host — your Mac's local IP
#define OLLAMA_HOST "192.168.88.55"
#define OLLAMA_PORT 11434
#define OLLAMA_MODEL "qwen3:4b"

static OllamaProvider* provider = nullptr;

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
    TEST_ASSERT_EQUAL_STRING("Ollama", provider->getName());
    TEST_ASSERT_EQUAL(Provider::Ollama, provider->getType());
}

void test_basic_chat() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is 2+2? Reply with just the number."));

    ChatOptions options;
    options.maxTokens = 256;  // qwen3 needs headroom for reasoning + answer
    options.temperature = 0.0f;

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] HTTP: %d, Success: %d\n", resp.httpStatus, resp.success);
    Serial.printf("[INFO] Content: %s\n", resp.content.c_str());
    Serial.printf("[INFO] Tokens: prompt=%u, completion=%u\n", resp.promptTokens, resp.completionTokens);

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_EQUAL(200, resp.httpStatus);
    TEST_ASSERT_FALSE(resp.content.isEmpty());
    TEST_ASSERT_TRUE(resp.content.indexOf("4") >= 0);
}

void test_chat_with_system_prompt() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "You are a helpful assistant. Always reply in Ukrainian."));
    messages.push_back(Message(Role::User, "What is the capital of France?"));

    ChatOptions options;
    options.maxTokens = 256;  // qwen3 needs headroom for reasoning + answer

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] Content: %s\n", resp.content.c_str());

    TEST_ASSERT_TRUE(resp.success);
    TEST_ASSERT_FALSE(resp.content.isEmpty());
}

#if ESPAI_ENABLE_STREAMING
void test_streaming() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Count from 1 to 5, one number per line."));

    ChatOptions options;
    options.maxTokens = 64;

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

    JsonDocument doc;
    doc["type"] = "object";
    auto props = doc["properties"].to<JsonObject>();
    auto loc = props["location"].to<JsonObject>();
    loc["type"] = "string";
    loc["description"] = "City name";
    auto req = doc["required"].to<JsonArray>();
    req.add("location");

    String schema;
    serializeJson(doc, schema);
    weatherTool.parametersJson = schema;

    provider->addTool(weatherTool);

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the weather in Tokyo?"));

    ChatOptions options;
    options.maxTokens = 128;

    Response resp = provider->chat(messages, options);

    Serial.printf("[INFO] Tool call: success=%d, hasToolCalls=%d\n", resp.success, provider->hasToolCalls());

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

void setup() {
    delay(2000);
    Serial.begin(115200);

    Serial.println("\n=== ESPAI Ollama Live Tests (qwen3:4b) ===");
    printHeapInfo("Initial");

    connectWiFi();

    // Create Ollama provider pointing to local network host
    String ollamaUrl = "http://" OLLAMA_HOST ":" + String(OLLAMA_PORT) + "/v1/chat/completions";
    provider = new OllamaProvider("", OLLAMA_MODEL);
    provider->setBaseUrl(ollamaUrl);
    provider->setTimeout(60000);  // 60s — local models can be slow on first inference

    Serial.printf("[INFO] Ollama URL: %s\n", ollamaUrl.c_str());
    Serial.printf("[INFO] Model: %s\n", OLLAMA_MODEL);

    UNITY_BEGIN();

    RUN_TEST(test_wifi_connection);
    RUN_TEST(test_provider_configured);
    RUN_TEST(test_basic_chat);
    RUN_TEST(test_chat_with_system_prompt);
#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_streaming);
#endif
#if ESPAI_ENABLE_TOOLS
    RUN_TEST(test_tool_calling);
#endif

    UNITY_END();

    delete provider;
    printHeapInfo("Final");
}

void loop() {
    delay(1000);
}
