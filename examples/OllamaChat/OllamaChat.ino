/**
 * ESPAI Library - Ollama Chat Example
 *
 * Demonstrates streaming, tool calling, and async requests
 * against a local Ollama instance from an ESP32.
 *
 * Hardware: ESP32 (any variant)
 *
 * Prerequisites:
 * - Ollama running on your local network
 * - OLLAMA_HOST env set to 0.0.0.0 so the ESP32 can reach it
 *   macOS app:  launchctl setenv OLLAMA_HOST "0.0.0.0" + restart app
 *   CLI:        OLLAMA_HOST=0.0.0.0 ollama serve
 *
 * Setup:
 * 1. Copy secrets.h.example to secrets.h
 * 2. Fill in WiFi credentials and your computer's local IP
 * 3. Upload to your ESP32
 * 4. Open Serial Monitor at 115200 baud
 */

#include <WiFi.h>
#include <ESPAI.h>

#include "secrets.h"

using namespace ESPAI;

// Change model to whatever you have pulled in Ollama
#define OLLAMA_MODEL "qwen3:4b"

void connectToWiFi();
void demoBasicChat();
void demoStreaming();
void demoToolCalling();
void demoAsync();

// Build the Ollama endpoint URL at startup
String ollamaUrl;
OllamaProvider ollama;

// Simulated sensor data for tool calling demo
float sensorTemp = 22.7;
float sensorHumidity = 58.3;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Ollama Chat Example");
    Serial.println("=================================");

    connectToWiFi();

    // Configure Ollama provider
    ollamaUrl = "http://" + String(OLLAMA_HOST) + ":" + String(OLLAMA_PORT) + "/v1/chat/completions";
    ollama.setBaseUrl(ollamaUrl);
    ollama.setModel(OLLAMA_MODEL);
    ollama.setTimeout(60000);  // local models can be slow on first inference

    Serial.print("Ollama: ");
    Serial.println(ollamaUrl);
    Serial.print("Model:  ");
    Serial.println(OLLAMA_MODEL);
    Serial.println();

    // Run demos sequentially
    demoBasicChat();
    demoStreaming();
    demoToolCalling();
    demoAsync();
}

void loop() {
    delay(10000);
}

// ----------------------------------------------------------------
// WiFi
// ----------------------------------------------------------------

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

// ----------------------------------------------------------------
// 1. Basic chat — simple request/response
// ----------------------------------------------------------------

void demoBasicChat() {
    Serial.println("--- 1. Basic Chat ---");

    std::vector<Message> messages;
    messages.push_back(Message(Role::System, "Reply concisely in one sentence."));
    messages.push_back(Message(Role::User, "What is the ESP32?"));

    ChatOptions options;
    options.maxTokens = 256;

    Response resp = ollama.chat(messages, options);

    if (resp.success) {
        Serial.println(resp.content);
        Serial.printf("Tokens: %u prompt, %u completion\n", resp.promptTokens, resp.completionTokens);
    } else {
        Serial.printf("Error: %s\n", resp.errorMessage.c_str());
    }

    Serial.println();
}

// ----------------------------------------------------------------
// 2. Streaming — tokens printed as they arrive
// ----------------------------------------------------------------

void demoStreaming() {
    Serial.println("--- 2. Streaming ---");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "List 5 popular programming languages, one per line."));

    ChatOptions options;
    options.maxTokens = 256;

    bool success = ollama.chatStream(messages, options, [](const String& chunk, bool done) {
        Serial.print(chunk);
        if (done) {
            Serial.println("\n");
        }
    });

    if (!success) {
        Serial.println("Streaming failed!");
    }
}

// ----------------------------------------------------------------
// 3. Tool calling — AI invokes functions you define
// ----------------------------------------------------------------

String executeTool(const ToolCall& call) {
    Serial.printf("  -> Executing tool: %s\n", call.name.c_str());

    if (call.name == "get_temperature") {
        return "{\"temperature\": " + String(sensorTemp, 1) + ", \"unit\": \"celsius\"}";
    }
    if (call.name == "get_humidity") {
        return "{\"humidity\": " + String(sensorHumidity, 1) + ", \"unit\": \"percent\"}";
    }

    return "{\"error\": \"Unknown tool\"}";
}

void demoToolCalling() {
    Serial.println("--- 3. Tool Calling ---");

    // Register tools
    Tool tempTool;
    tempTool.name = "get_temperature";
    tempTool.description = "Get the current temperature from the sensor in Celsius";
    tempTool.parametersJson = R"({"type":"object","properties":{}})";
    ollama.addTool(tempTool);

    Tool humTool;
    humTool.name = "get_humidity";
    humTool.description = "Get the current humidity percentage from the sensor";
    humTool.parametersJson = R"({"type":"object","properties":{}})";
    ollama.addTool(humTool);

    // Ask the AI something that requires tools
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What are the current temperature and humidity readings?"));

    ChatOptions options;
    options.maxTokens = 256;

    // Step 1: AI decides which tools to call
    Response resp = ollama.chat(messages, options);

    if (!resp.success) {
        Serial.printf("Error: %s\n", resp.errorMessage.c_str());
        ollama.clearTools();
        return;
    }

    if (!ollama.hasToolCalls()) {
        Serial.println("Model replied without tools:");
        Serial.println(resp.content);
        ollama.clearTools();
        Serial.println();
        return;
    }

    const auto& toolCalls = ollama.getLastToolCalls();
    Serial.printf("AI requested %d tool call(s)\n", toolCalls.size());

    // Add assistant message with tool call metadata
    messages.push_back(ollama.getAssistantMessageWithToolCalls(resp.content));

    // Step 2: Execute tools and feed results back
    for (const auto& call : toolCalls) {
        String result = executeTool(call);
        messages.push_back(Message(Role::Tool, result, call.id));
    }

    // Step 3: AI summarises the results — streamed
    ollama.clearTools();

    Serial.println("AI response:");
    ollama.chatStream(messages, options, [](const String& chunk, bool done) {
        Serial.print(chunk);
        if (done) Serial.println("\n");
    });
}

// ----------------------------------------------------------------
// 4. Async — non-blocking request with FreeRTOS
// ----------------------------------------------------------------

void demoAsync() {
    Serial.println("--- 4. Async ---");

    AIClient client(Provider::Ollama, "");
    client.setBaseUrl(ollamaUrl);
    client.setModel(OLLAMA_MODEL);
    client.setTimeout(60000);

    // 4a. Async with callback
    Serial.println("Sending async request (main loop stays free)...");

    client.chatAsync("What is 1+1? Reply with just the number.", [](const Response& resp) {
        if (resp.success) {
            Serial.print("Async callback: ");
            Serial.println(resp.content);
        } else {
            Serial.print("Async error: ");
            Serial.println(resp.errorMessage);
        }
    });

    // Do other work while waiting
    int dots = 0;
    while (client.isAsyncBusy()) {
        Serial.print(".");
        dots++;
        delay(200);
    }
    if (dots > 0) Serial.println();

    delay(500);

    // 4b. Async streaming
    Serial.println("Async streaming:");

    client.chatStreamAsync(
        "Count from 1 to 3.",
        [](const String& chunk, bool done) {
            Serial.print(chunk);
            if (done) Serial.println();
        },
        [](const Response& resp) {
            Serial.println("--- Async streaming done ---");
            Serial.println();
        }
    );

    while (client.isAsyncBusy()) {
        delay(10);
    }

    Serial.println("All demos complete!");
}
