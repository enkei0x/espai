/**
 * ESPAI Library - Conversation History Example
 *
 * This example demonstrates multi-turn conversations where the AI
 * maintains context across multiple messages. Uses the Conversation
 * class to manage message history.
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
bool sendMessage(const char* userMessage);
void runConversation();

OpenAIProvider openai(OPENAI_API_KEY);

// Conversation object maintains message history
// maxMessages parameter limits history to prevent memory issues
Conversation conversation(10);

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("==========================================");
    Serial.println("ESPAI Conversation History Example");
    Serial.println("==========================================");

    connectToWiFi();

    // Set up the conversation with a system prompt
    conversation.setSystemPrompt(
        "You are a helpful assistant that specializes in ESP32 programming. "
        "Keep your responses concise and practical."
    );

    // Demonstrate multi-turn conversation
    runConversation();
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

// Send a message and add both user message and response to history
bool sendMessage(const char* userMessage) {
    Serial.print("USER: ");
    Serial.println(userMessage);

    // Add user message to conversation history
    conversation.addUserMessage(userMessage);

    // Build messages array including system prompt
    std::vector<Message> messages;

    // Add system message if set
    if (conversation.getSystemPrompt().length() > 0) {
        messages.push_back(Message(Role::System, conversation.getSystemPrompt()));
    }

    // Add all conversation messages
    const auto& history = conversation.getMessages();
    for (const auto& msg : history) {
        messages.push_back(msg);
    }

    // Send to OpenAI
    ChatOptions options;
    options.maxTokens = 256;

    Response response = openai.chat(messages, options);

    if (response.success) {
        Serial.print("ASSISTANT: ");
        Serial.println(response.content);
        Serial.println();

        // Add assistant response to history
        conversation.addAssistantMessage(response.content);
        return true;
    } else {
        Serial.print("ERROR: ");
        Serial.println(response.errorMessage);
        Serial.println();
        return false;
    }
}

void runConversation() {
    Serial.println("Starting multi-turn conversation...");
    Serial.println("------------------------------------------");
    Serial.println();

    // Turn 1: Initial question
    if (!sendMessage("What GPIO pins on ESP32 support ADC?")) {
        return;
    }

    delay(1000);

    // Turn 2: Follow-up question (AI should remember context)
    if (!sendMessage("What is the resolution of those ADCs?")) {
        return;
    }

    delay(1000);

    // Turn 3: Another follow-up
    if (!sendMessage("Can I use them while WiFi is active?")) {
        return;
    }

    // Display conversation statistics
    Serial.println("------------------------------------------");
    Serial.println("Conversation Statistics:");
    Serial.print("  Messages in history: ");
    Serial.println(conversation.size());
    Serial.print("  Estimated tokens: ");
    Serial.println(conversation.estimateTokens());
    Serial.print("  Max messages allowed: ");
    Serial.println(conversation.getMaxMessages());

    // Show how to clear conversation
    Serial.println();
    Serial.println("Clearing conversation history...");
    conversation.clear();
    Serial.print("Messages after clear: ");
    Serial.println(conversation.size());
}
