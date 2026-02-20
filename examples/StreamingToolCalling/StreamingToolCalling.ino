// ESPAI Streaming Tool Calling Example
// Combines streaming responses with AI-driven function calling.
// The AI's text is streamed in real-time, and tool calls are
// accumulated during the stream and executed after it completes.

#include <WiFi.h>
#include <ESPAI.h>

#include "secrets.h"

using namespace ESPAI;

void connectToWiFi();
void registerTools();
String executeTool(const ToolCall& call);
void demonstrateStreamingToolCalling();

OpenAIProvider ai(OPENAI_API_KEY);

float currentTemperature = 23.5;
float currentHumidity = 65.0;
bool ledState = false;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Streaming Tool Calling");
    Serial.println("=================================");

    connectToWiFi();
    registerTools();
    demonstrateStreamingToolCalling();
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

void registerTools() {
    Tool tempTool;
    tempTool.name = "get_temperature";
    tempTool.description = "Get the current temperature reading from the sensor in Celsius";
    tempTool.parametersJson = R"({"type":"object","properties":{},"required":[]})";
    ai.addTool(tempTool);

    Tool humidityTool;
    humidityTool.name = "get_humidity";
    humidityTool.description = "Get the current humidity reading from the sensor as a percentage";
    humidityTool.parametersJson = R"({"type":"object","properties":{},"required":[]})";
    ai.addTool(humidityTool);

    Tool ledTool;
    ledTool.name = "set_led";
    ledTool.description = "Turn the LED on or off";
    ledTool.parametersJson = R"({
        "type":"object",
        "properties":{"state":{"type":"boolean","description":"true=on, false=off"}},
        "required":["state"]
    })";
    ai.addTool(ledTool);

    Serial.println("Tools registered: get_temperature, get_humidity, set_led");
    Serial.println();
}

String executeTool(const ToolCall& call) {
    Serial.print("  Executing: ");
    Serial.println(call.name);

    if (call.name == "get_temperature") {
        return "{\"temperature\": " + String(currentTemperature, 1) + ", \"unit\": \"celsius\"}";
    }
    else if (call.name == "get_humidity") {
        return "{\"humidity\": " + String(currentHumidity, 1) + ", \"unit\": \"percent\"}";
    }
    else if (call.name == "set_led") {
        bool newState = call.arguments.indexOf("true") >= 0;
        ledState = newState;
        return "{\"success\": true, \"led_state\": " + String(ledState ? "true" : "false") + "}";
    }

    return "{\"error\": \"Unknown tool\"}";
}

void demonstrateStreamingToolCalling() {
    std::vector<Message> messages;
    messages.push_back(Message(
        Role::User,
        "What's the current temperature and humidity? Also, please turn on the LED."
    ));

    ChatOptions options;
    options.maxTokens = 256;

    // Step 1: Stream the initial request
    // Text chunks arrive in real-time via the callback.
    // Tool calls are accumulated silently during the stream
    // and available via hasToolCalls() after it completes.
    Serial.println("Step 1: Streaming request...");
    Serial.println();

    String streamedText;
    bool success = ai.chatStream(messages, options, [&](const String& chunk, bool done) {
        if (!chunk.isEmpty()) {
            Serial.print(chunk);
            streamedText += chunk;
        }
        if (done) {
            Serial.println();
        }
    });

    if (!success) {
        Serial.println("Streaming failed!");
        return;
    }

    // Step 2: Check if the AI requested tool calls
    if (!ai.hasToolCalls()) {
        Serial.println("No tool calls requested.");
        return;
    }

    const auto& toolCalls = ai.getLastToolCalls();
    Serial.print("\nStep 2: AI requested ");
    Serial.print(toolCalls.size());
    Serial.println(" tool call(s)");

    // Add the assistant message (with tool call metadata) to conversation
    messages.push_back(ai.getAssistantMessageWithToolCalls(streamedText));

    // Step 3: Execute each tool and add results to conversation
    Serial.println("Step 3: Executing tools...");
    for (const auto& call : toolCalls) {
        String result = executeTool(call);
        messages.push_back(Message(Role::Tool, result, call.id));
    }

    // Step 4: Stream the final response with tool results
    Serial.println("\nStep 4: Streaming final response...");
    Serial.println();

    ai.clearTools();
    success = ai.chatStream(messages, options, [](const String& chunk, bool done) {
        Serial.print(chunk);
        if (done) {
            Serial.println();
            Serial.println();
            Serial.println("Done!");
        }
    });

    if (!success) {
        Serial.println("Final streaming failed!");
    }
}
