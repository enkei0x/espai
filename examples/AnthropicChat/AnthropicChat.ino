/**
 * ESPAI Library - Anthropic Chat Example
 *
 * This example demonstrates using the Anthropic (Claude) provider
 * to send a message and receive a response.
 *
 * Hardware: ESP32 (any variant)
 *
 * Setup:
 * 1. Copy secrets.h.example to secrets.h
 * 2. Fill in your WiFi credentials and Anthropic API key
 * 3. Upload to your ESP32
 * 4. Open Serial Monitor at 115200 baud
 */

#include <WiFi.h>
#include <ESPAI.h>

#include "secrets.h"

// WiFi credentials (defined in secrets.h)
// const char* WIFI_SSID = "your-wifi-ssid";
// const char* WIFI_PASSWORD = "your-wifi-password";
// const char* ANTHROPIC_API_KEY = "sk-ant-your-api-key";

using namespace ESPAI;

// Forward declarations
void connectToWiFi();
void sendChatMessage();

// Create the Anthropic provider
AnthropicProvider claude(ANTHROPIC_API_KEY);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Anthropic Chat Example");
    Serial.println("=================================");

    connectToWiFi();
    sendChatMessage();
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

void sendChatMessage() {
    Serial.println("Sending message to Anthropic Claude...");
    Serial.println();

    // Optionally change the model
    // claude.setModel("claude-opus-4-20250514");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the capital of France? Reply in one sentence."));

    ChatOptions options;

    Response response = claude.chat(messages, options);

    if (response.success) {
        Serial.println("Response received:");
        Serial.println("------------------");
        Serial.println(response.content);
        Serial.println("------------------");
        Serial.println();

        Serial.print("Tokens used - Prompt: ");
        Serial.print(response.promptTokens);
        Serial.print(", Completion: ");
        Serial.print(response.completionTokens);
        Serial.print(", Total: ");
        Serial.println(response.totalTokens());
    } else {
        Serial.println("Error occurred!");
        Serial.print("Error code: ");
        Serial.println(errorCodeToString(response.error));
        Serial.print("Error message: ");
        Serial.println(response.errorMessage);
    }
}
