/**
 * ESPAI Library - Custom Options Example
 *
 * This example demonstrates all available ChatOptions:
 * - temperature: Controls randomness (0.0 = deterministic, 2.0 = very random)
 * - maxTokens: Maximum tokens in the response
 * - topP: Nucleus sampling parameter
 * - frequencyPenalty: Reduces repetition of tokens
 * - presencePenalty: Encourages new topics
 * - model: Override the default model
 * - systemPrompt: Set behavior instructions
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
void sendRequest(const char* label, const char* message, const ChatOptions& options);
void demonstrateTemperature();
void demonstrateCreativeWriting();
void demonstratePreciseAnswers();
void demonstrateModelOverride();

OpenAIProvider openai(OPENAI_API_KEY);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Custom Options Example");
    Serial.println("=================================");

    connectToWiFi();

    // Demonstrate different option configurations
    demonstrateTemperature();
    demonstrateCreativeWriting();
    demonstratePreciseAnswers();
    demonstrateModelOverride();
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
    Serial.println();
}

void sendRequest(const char* label, const char* message, const ChatOptions& options) {
    Serial.println(label);
    Serial.println("----------------------------------------");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, message));

    Response response = openai.chat(messages, options);

    if (response.success) {
        Serial.println(response.content);
    } else {
        Serial.print("Error: ");
        Serial.println(response.errorMessage);
    }

    Serial.println();
    delay(2000);  // Rate limiting
}

void demonstrateTemperature() {
    // Temperature controls randomness
    // Low temperature = more deterministic, focused responses
    // High temperature = more creative, varied responses

    const char* prompt = "Generate a single random word.";

    // Low temperature - more predictable
    ChatOptions lowTemp;
    lowTemp.temperature = 0.1f;
    lowTemp.maxTokens = 10;
    sendRequest("LOW TEMPERATURE (0.1) - Deterministic", prompt, lowTemp);

    // High temperature - more random
    ChatOptions highTemp;
    highTemp.temperature = 1.5f;
    highTemp.maxTokens = 10;
    sendRequest("HIGH TEMPERATURE (1.5) - Creative", prompt, highTemp);
}

void demonstrateCreativeWriting() {
    // Configuration for creative writing tasks
    ChatOptions creative;
    creative.temperature = 0.9f;      // Higher creativity
    creative.maxTokens = 150;         // Allow longer response
    creative.topP = 0.95f;            // Slightly restrict token pool
    creative.frequencyPenalty = 0.5f; // Reduce repetition
    creative.presencePenalty = 0.5f;  // Encourage variety
    creative.systemPrompt = "You are a creative writer who crafts vivid, "
                            "imaginative descriptions. Be concise but evocative.";

    sendRequest(
        "CREATIVE WRITING CONFIGURATION",
        "Describe an ESP32 as if it were a magical artifact.",
        creative
    );
}

void demonstratePreciseAnswers() {
    // Configuration for factual, precise answers
    ChatOptions precise;
    precise.temperature = 0.2f;       // Low randomness
    precise.maxTokens = 100;          // Concise responses
    precise.topP = 0.9f;              // Focused token selection
    precise.frequencyPenalty = 0.0f;  // Allow technical repetition
    precise.presencePenalty = 0.0f;   // Stay on topic
    precise.systemPrompt = "You are a technical expert. Provide accurate, "
                           "concise answers. Use bullet points when appropriate.";

    sendRequest(
        "PRECISE/TECHNICAL CONFIGURATION",
        "What are the key differences between ESP32 and ESP32-S3?",
        precise
    );
}

void demonstrateModelOverride() {
    // Override the default model for specific requests
    // Useful for using different models for different tasks

    ChatOptions withModel;
    withModel.model = "gpt-4o-mini";  // Explicitly set model
    withModel.temperature = 0.7f;
    withModel.maxTokens = 50;

    sendRequest(
        "MODEL OVERRIDE (gpt-4o-mini)",
        "What is 2 + 2? Reply with just the number.",
        withModel
    );
}
