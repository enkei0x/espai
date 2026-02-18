#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "ESPAI.h"
#include "../secrets.h"

using namespace ESPAI;

static OpenAIProvider* provider = nullptr;
static uint32_t initialHeap = 0;

void printHeapInfo(const char* label) {
    Serial.printf("[HEAP] %s: free=%u, largest=%u, min=%u\n",
        label,
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap(),
        ESP.getMinFreeHeap());
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
    initialHeap = ESP.getFreeHeap();
    printHeapInfo("setUp");
}

void tearDown() {
    printHeapInfo("tearDown");
    uint32_t currentHeap = ESP.getFreeHeap();
    int32_t diff = (int32_t)initialHeap - (int32_t)currentHeap;
    Serial.printf("[HEAP] Diff: %d bytes\n", diff);
}

void test_wifi_connected() {
    TEST_ASSERT_EQUAL(WL_CONNECTED, WiFi.status());
    Serial.printf("[INFO] IP: %s\n", WiFi.localIP().toString().c_str());
}

void test_heap_after_provider_creation() {
    uint32_t heapBefore = ESP.getFreeHeap();

    {
        OpenAIProvider tempProvider(TEST_OPENAI_KEY, "gpt-4o-mini");
    }

    uint32_t heapAfter = ESP.getFreeHeap();
    int32_t leak = (int32_t)heapBefore - (int32_t)heapAfter;

    Serial.printf("[HEAP] Provider creation leak: %d bytes\n", leak);
    TEST_ASSERT_LESS_OR_EQUAL(100, abs(leak));
}

void test_heap_after_single_request() {
    uint32_t heapBefore = ESP.getFreeHeap();
    printHeapInfo("Before request");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Say hello in one word"));

    ChatOptions options;
    options.maxTokens = 10;

    Response resp = provider->chat(messages, options);

    uint32_t heapAfter = ESP.getFreeHeap();
    printHeapInfo("After request");

    int32_t leak = (int32_t)heapBefore - (int32_t)heapAfter;
    Serial.printf("[HEAP] Single request leak: %d bytes\n", leak);
    Serial.printf("[INFO] Response: success=%d, content=%s\n", resp.success, resp.content.c_str());

    TEST_ASSERT_TRUE(resp.success);
    // First request allocates ~3-4KB for SSL session caching (expected)
    TEST_ASSERT_LESS_OR_EQUAL(5120, abs(leak));
}

void test_memory_leak_multiple_requests() {
    uint32_t heapBefore = ESP.getFreeHeap();
    printHeapInfo("Before 10 requests");

    const int iterations = 10;
    int successCount = 0;

    for (int i = 0; i < iterations; i++) {
        std::vector<Message> messages;
        messages.push_back(Message(Role::User, "Say OK"));

        ChatOptions options;
        options.maxTokens = 5;

        Response resp = provider->chat(messages, options);

        if (resp.success) {
            successCount++;
        } else {
            Serial.printf("[WARN] Request %d failed: %s\n", i, resp.errorMessage.c_str());
        }

        uint32_t currentHeap = ESP.getFreeHeap();
        Serial.printf("[HEAP] After request %d: %u bytes free\n", i + 1, currentHeap);

        delay(500);
    }

    uint32_t heapAfter = ESP.getFreeHeap();
    printHeapInfo("After 10 requests");

    int32_t totalLeak = (int32_t)heapBefore - (int32_t)heapAfter;
    int32_t leakPerRequest = totalLeak / iterations;

    Serial.printf("[HEAP] Total leak: %d bytes, per request: %d bytes\n", totalLeak, leakPerRequest);
    Serial.printf("[INFO] Success rate: %d/%d\n", successCount, iterations);

    TEST_ASSERT_GREATER_OR_EQUAL(iterations / 2, successCount);
    TEST_ASSERT_LESS_OR_EQUAL(256, abs(leakPerRequest));
}

#if ESPAI_ENABLE_STREAMING
void test_heap_after_streaming_request() {
    uint32_t heapBefore = ESP.getFreeHeap();
    printHeapInfo("Before streaming");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Count from 1 to 3"));

    ChatOptions options;
    options.maxTokens = 20;

    String accumulated;
    bool streamSuccess = provider->chatStream(messages, options, [&](const String& chunk, bool done) {
        accumulated += chunk;
        if (done) {
            Serial.printf("[STREAM] Complete: %s\n", accumulated.c_str());
        }
    });

    uint32_t heapAfter = ESP.getFreeHeap();
    printHeapInfo("After streaming");

    int32_t leak = (int32_t)heapBefore - (int32_t)heapAfter;
    Serial.printf("[HEAP] Streaming leak: %d bytes\n", leak);

    TEST_ASSERT_TRUE(streamSuccess);
    TEST_ASSERT_FALSE(accumulated.isEmpty());
    TEST_ASSERT_LESS_OR_EQUAL(1024, abs(leak));
}
#endif

void test_heap_fragmentation() {
    heap_caps_print_heap_info(MALLOC_CAP_8BIT);

    size_t freeHeap = ESP.getFreeHeap();
    size_t largestBlock = ESP.getMaxAllocHeap();
    float fragmentation = 1.0f - ((float)largestBlock / (float)freeHeap);

    Serial.printf("[HEAP] Fragmentation: %.2f%%\n", fragmentation * 100);
    TEST_ASSERT_LESS_OR_EQUAL(0.5f, fragmentation);
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    Serial.println("\n=== ESPAI Memory Tests ===");
    printHeapInfo("Initial");

    connectWiFi();

    provider = new OpenAIProvider(TEST_OPENAI_KEY, "gpt-4o-mini");
    provider->setTimeout(30000);

    UNITY_BEGIN();

    RUN_TEST(test_wifi_connected);
    RUN_TEST(test_heap_after_provider_creation);
    RUN_TEST(test_heap_after_single_request);
    RUN_TEST(test_memory_leak_multiple_requests);
#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_heap_after_streaming_request);
#endif
    RUN_TEST(test_heap_fragmentation);

    UNITY_END();

    delete provider;
    printHeapInfo("Final");
}

void loop() {
    delay(1000);
}
