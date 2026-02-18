/**
 * ESPAI Library - Streaming Chat Example
 *
 * This example demonstrates streaming responses from OpenAI.
 * Tokens are printed in real-time as they arrive, providing
 * a more responsive user experience.
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

using namespace ESPAI;

// Forward declarations
void connectToWiFi();
void sendStreamingMessage();

OpenAIProvider openai(OPENAI_API_KEY);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Streaming Chat Example");
    Serial.println("=================================");

    connectToWiFi();
    sendStreamingMessage();
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

void sendStreamingMessage() {
    Serial.println("Sending streaming request to OpenAI...");
    Serial.println();

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Write a short poem about embedded systems programming."));

    ChatOptions options;
    options.maxTokens = 256;

    Serial.println("Response (streaming):");
    Serial.println("----------------------");

    // Track timing for demonstration
    unsigned long startTime = millis();
    unsigned long firstTokenTime = 0;
    int chunkCount = 0;

    // Use streaming with a callback function
    // The callback receives each chunk of text as it arrives
    bool success = openai.chatStream(messages, options, [&](const String& chunk, bool done) {
        if (chunkCount == 0 && chunk.length() > 0) {
            firstTokenTime = millis();
        }

        // Print each chunk immediately as it arrives
        Serial.print(chunk);
        chunkCount++;

        if (done) {
            Serial.println();
            Serial.println("----------------------");
            Serial.println();

            // Print timing statistics
            unsigned long totalTime = millis() - startTime;
            unsigned long timeToFirstToken = firstTokenTime > 0 ? firstTokenTime - startTime : 0;

            Serial.print("Time to first token: ");
            Serial.print(timeToFirstToken);
            Serial.println(" ms");

            Serial.print("Total streaming time: ");
            Serial.print(totalTime);
            Serial.println(" ms");

            Serial.print("Chunks received: ");
            Serial.println(chunkCount);
        }
    });

    if (!success) {
        Serial.println();
        Serial.println("Streaming failed!");
        Serial.println("Check your API key and network connection.");
    }
}
