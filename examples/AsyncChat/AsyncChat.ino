/**
 * ESPAI Library - Async Chat Example
 *
 * This example demonstrates non-blocking async chat requests
 * using FreeRTOS tasks. The main loop remains responsive while
 * the AI request runs in the background.
 *
 * Hardware: ESP32 (any variant)
 *
 * Setup:
 * 1. Copy secrets.h.example to secrets.h
 * 2. Fill in your WiFi credentials and OpenAI API key
 * 3. Upload to your ESP32
 * 4. Open Serial Monitor at 115200 baud
 */

#include <WiFi.h>
#include <ESPAI.h>

#include "secrets.h"

// WiFi credentials (defined in secrets.h)
// const char* WIFI_SSID = "your-wifi-ssid";
// const char* WIFI_PASSWORD = "your-wifi-password";
// const char* OPENAI_API_KEY = "sk-your-api-key";

using namespace ESPAI;

// Forward declarations
void connectToWiFi();
void asyncWithCallback();
void asyncWithPolling();
void asyncStreaming();

// Use AIClient for simplified async API
AIClient client(Provider::OpenAI, OPENAI_API_KEY);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Async Chat Example");
    Serial.println("=================================");

    connectToWiFi();

    // Example 1: Callback-based async
    asyncWithCallback();

    // Wait for it to finish before next example
    while (client.isAsyncBusy()) {
        delay(10);
    }

    delay(1000);

    // Example 2: Poll-based async
    asyncWithPolling();

    delay(1000);

    // Example 3: Async streaming
    asyncStreaming();
}

void loop() {
    delay(10000);
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi!");
        while (true) { delay(1000); }
    }

    Serial.println(" Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void asyncWithCallback() {
    Serial.println("--- Async with Callback ---");
    Serial.println("Sending async request...");

    // Fire-and-forget: callback runs when done
    client.chatAsync("What is 2+2? Reply with just the number.", [](const Response& resp) {
        if (resp.success) {
            Serial.print("Callback received: ");
            Serial.println(resp.content);
        } else {
            Serial.print("Error: ");
            Serial.println(resp.errorMessage);
        }
    });

    // Main loop is free to do other work
    Serial.println("Main thread is free while waiting...");
}

void asyncWithPolling() {
    Serial.println();
    Serial.println("--- Async with Polling ---");
    Serial.println("Sending async request...");

    ChatRequest* req = client.chatAsync("Name a color. Reply with one word.");

    // Poll until complete, doing other work in between
    int dots = 0;
    while (!req->isComplete()) {
        req->poll();
        Serial.print(".");
        dots++;
        delay(100);
    }

    Serial.println();
    Response result = req->getResult();
    if (result.success) {
        Serial.print("Poll result: ");
        Serial.println(result.content);
    } else {
        Serial.print("Error: ");
        Serial.println(result.errorMessage);
    }
}

void asyncStreaming() {
    Serial.println();
    Serial.println("--- Async Streaming ---");
    Serial.println("Sending async streaming request...");

    client.chatStreamAsync(
        "Count from 1 to 5, one number per line.",
        // Stream callback - receives chunks as they arrive
        [](const String& chunk, bool done) {
            Serial.print(chunk);
            if (done) {
                Serial.println();
            }
        },
        // Done callback - called when streaming is complete
        [](const Response& resp) {
            Serial.println("--- Streaming complete ---");
        }
    );

    // Wait for streaming to finish
    while (client.isAsyncBusy()) {
        delay(10);
    }
}
