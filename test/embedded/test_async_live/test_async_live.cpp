#include <unity.h>
#include <Arduino.h>
#include <WiFi.h>
#include "ESPAI.h"
#include "../secrets.h"

using namespace ESPAI;

static AIClient* client = nullptr;

void printHeapInfo(const char* label) {
    Serial.printf("[HEAP] %s: free=%u, min=%u\n",
                  label, ESP.getFreeHeap(), ESP.getMinFreeHeap());
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

void test_chat_async() {
    bool callbackFired = false;
    Response callbackResult;

    ChatRequest* req = client->chatAsync(
        "What is 2+2? Reply with just the number.",
        [&](const Response& r) {
            callbackFired = true;
            callbackResult = r;
        }
    );

    TEST_ASSERT_NOT_NULL(req);

    uint32_t deadline = millis() + 30000;
    while (!req->isComplete() && millis() < deadline) {
        req->poll();
        delay(100);
    }

    // Final poll to ensure callback fires
    req->poll();

    TEST_ASSERT_TRUE_MESSAGE(req->isComplete(), "Request did not complete within timeout");
    TEST_ASSERT_TRUE_MESSAGE(callbackFired, "Callback was not invoked");
    TEST_ASSERT_TRUE_MESSAGE(callbackResult.success, "Response was not successful");
    TEST_ASSERT_FALSE_MESSAGE(callbackResult.content.isEmpty(), "Response content is empty");

    Serial.printf("[INFO] Async chat result: %s\n", callbackResult.content.c_str());
    TEST_ASSERT_TRUE(callbackResult.content.indexOf("4") >= 0);
}

void test_chat_async_poll() {
    ChatRequest* req = client->chatAsync("What is 3+3? Reply with just the number.");

    TEST_ASSERT_NOT_NULL(req);

    uint32_t deadline = millis() + 30000;
    while (!req->isComplete() && millis() < deadline) {
        req->poll();
        delay(100);
    }

    TEST_ASSERT_TRUE_MESSAGE(req->isComplete(), "Request did not complete within timeout");

    Response result = req->getResult();
    TEST_ASSERT_TRUE_MESSAGE(result.success, "Response was not successful");
    TEST_ASSERT_FALSE_MESSAGE(result.content.isEmpty(), "Response content is empty");

    Serial.printf("[INFO] Async poll result: %s\n", result.content.c_str());
}

#if ESPAI_ENABLE_STREAMING
void test_chat_stream_async() {
    int chunkCount = 0;

    StreamCallback streamCb = [&](const String& chunk, bool done) {
        if (!chunk.isEmpty()) {
            chunkCount++;
            Serial.printf("[STREAM] Chunk %d: %s\n", chunkCount, chunk.c_str());
        }
        if (done) {
            Serial.println("[STREAM] Done");
        }
    };

    ChatRequest* req = client->chatStreamAsync(
        "Count from 1 to 3, one number per line.",
        streamCb
    );

    TEST_ASSERT_NOT_NULL(req);

    uint32_t deadline = millis() + 30000;
    while (!req->isComplete() && millis() < deadline) {
        req->poll();
        delay(100);
    }

    TEST_ASSERT_TRUE_MESSAGE(req->isComplete(), "Stream request did not complete within timeout");
    TEST_ASSERT_GREATER_THAN(0, chunkCount);

    Serial.printf("[INFO] Stream async chunks received: %d\n", chunkCount);
}
#endif

void test_is_async_busy() {
    ChatRequest* req = client->chatAsync("Say hello in one word.");

    TEST_ASSERT_NOT_NULL(req);
    TEST_ASSERT_TRUE(client->isAsyncBusy());

    uint32_t deadline = millis() + 30000;
    while (!req->isComplete() && millis() < deadline) {
        req->poll();
        delay(100);
    }

    TEST_ASSERT_TRUE_MESSAGE(req->isComplete(), "Request did not complete within timeout");
    TEST_ASSERT_FALSE(client->isAsyncBusy());
}

void test_cancel_async() {
    ChatRequest* req = client->chatAsync("Write a long poem about the ocean.");

    TEST_ASSERT_NOT_NULL(req);
    client->cancelAsync();

    uint32_t deadline = millis() + 30000;
    while (!req->isComplete() && millis() < deadline) {
        req->poll();
        delay(100);
    }

    TEST_ASSERT_TRUE_MESSAGE(req->isComplete(), "Request did not complete after cancel");

    // Race: request may complete before cancel takes effect
    AsyncStatus status = req->getStatus();
    bool validStatus = (status == AsyncStatus::Completed || status == AsyncStatus::Cancelled);
    TEST_ASSERT_TRUE_MESSAGE(validStatus, "Expected Completed or Cancelled status after cancelAsync");

    Serial.printf("[INFO] Cancel async status: %d\n", static_cast<int>(status));
}

void setup() {
    delay(2000);
    Serial.begin(115200);

    Serial.println("\n=== ESPAI Async Live Tests ===");
    printHeapInfo("Initial");

    connectWiFi();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[ERROR] WiFi connection failed");
        return;
    }
    Serial.printf("[INFO] Connected to %s, IP: %s\n",
                  TEST_WIFI_SSID, WiFi.localIP().toString().c_str());

    ChatOptions opts;
    opts.maxTokens = 50;
    opts.temperature = 0.0f;

    client = new AIClient(Provider::OpenAI, TEST_OPENAI_KEY);
    client->setTimeout(30000);

    UNITY_BEGIN();

    RUN_TEST(test_chat_async);
    RUN_TEST(test_chat_async_poll);
#if ESPAI_ENABLE_STREAMING
    RUN_TEST(test_chat_stream_async);
#endif
    RUN_TEST(test_is_async_busy);
    RUN_TEST(test_cancel_async);

    UNITY_END();

    delete client;
    printHeapInfo("Final");
}

void loop() {
    delay(1000);
}
