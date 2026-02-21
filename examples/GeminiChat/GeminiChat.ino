/**
 * ESPAI Library - Gemini Chat Example
 *
 * This example demonstrates using the Google Gemini provider
 * to send a message and receive a response, including the
 * thinking budget feature for Gemini 2.5+ models.
 *
 * Hardware: ESP32 (any variant)
 *
 * Setup:
 * 1. Copy secrets.h.example to secrets.h
 * 2. Fill in your WiFi credentials and Gemini API key
 * 3. Upload to your ESP32
 * 4. Open Serial Monitor at 115200 baud
 */

#include <WiFi.h>
#include <ESPAI.h>

#include "secrets.h"

// WiFi credentials (defined in secrets.h)
// const char* WIFI_SSID = "your-wifi-ssid";
// const char* WIFI_PASSWORD = "your-wifi-password";
// const char* GEMINI_API_KEY = "AIza-your-api-key";

using namespace ESPAI;

// Forward declarations
void connectToWiFi();
void sendChatMessage();
void sendWithThinking();

// Create the Gemini provider
GeminiProvider gemini(GEMINI_API_KEY);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Gemini Chat Example");
    Serial.println("=================================");

    connectToWiFi();
    sendChatMessage();
    sendWithThinking();
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
    Serial.println("Sending message to Gemini...");
    Serial.println();

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is the capital of France? Reply in one sentence."));

    ChatOptions options;

    Response response = gemini.chat(messages, options);

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

void sendWithThinking() {
    Serial.println();
    Serial.println("Sending message with thinking budget (Gemini 2.5+)...");
    Serial.println();

    // Use a Gemini 2.5 model for thinking support
    gemini.setModel("gemini-2.5-flash");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Solve this step by step: If a train travels 120km in 2 hours, what is its average speed?"));

    ChatOptions options;
    options.thinkingBudget = 4096;  // Allow up to 4096 thinking tokens

    Response response = gemini.chat(messages, options);

    if (response.success) {
        Serial.println("Response with thinking:");
        Serial.println("------------------");
        Serial.println(response.content);
        Serial.println("------------------");
    } else {
        Serial.println("Error: ");
        Serial.println(response.errorMessage);
    }
}
