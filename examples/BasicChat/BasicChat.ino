/**
 * ESPAI Library - Basic Chat Example
 *
 * This example demonstrates the simplest way to use the ESPAI library
 * to send a message to OpenAI and receive a response.
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

// Create secrets.h with your credentials (see secrets.h.example)
#include "secrets.h"

// WiFi credentials (defined in secrets.h)
// const char* WIFI_SSID = "your-wifi-ssid";
// const char* WIFI_PASSWORD = "your-wifi-password";
// const char* OPENAI_API_KEY = "sk-your-api-key";

using namespace ESPAI;

// Forward declarations
void connectToWiFi();
void sendChatMessage();

// Create the OpenAI provider
OpenAIProvider openai(OPENAI_API_KEY);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Basic Chat Example");
    Serial.println("=================================");

    // Connect to WiFi
    connectToWiFi();

    // Send a simple chat request
    sendChatMessage();
}

void loop() {
    // Nothing to do in loop for this example
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
        Serial.println("Please check your credentials in secrets.h");
        while (true) { delay(1000); }
    }

    Serial.println(" Connected!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}

void sendChatMessage() {
    Serial.println("Sending message to OpenAI...");
    Serial.println();

    // Build the message list - OpenAI expects an array of messages
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the capital of France? Reply in one sentence."));

    // Use default chat options
    ChatOptions options;

    // Send the request and get response
    Response response = openai.chat(messages, options);

    // Check if the request was successful
    if (response.success) {
        Serial.println("Response received:");
        Serial.println("------------------");
        Serial.println(response.content);
        Serial.println("------------------");
        Serial.println();

        // Print token usage
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
        Serial.print("HTTP status: ");
        Serial.println(response.httpStatus);
    }
}
