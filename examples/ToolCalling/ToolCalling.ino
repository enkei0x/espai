// ESPAI Tool Calling Example
// Demonstrates AI-driven function calling with sensor reads and LED control.

#include <WiFi.h>
#include <ESPAI.h>

#include "secrets.h"

using namespace ESPAI;

// Forward declarations
void connectToWiFi();
void registerTools();
String executeTool(const ToolCall& call);
void demonstrateToolCalling();

OpenAIProvider ai(OPENAI_API_KEY);

// Simulated sensor values (replace with real sensor readings)
float currentTemperature = 23.5;
float currentHumidity = 65.0;
bool ledState = false;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Tool Calling Example");
    Serial.println("=================================");

    connectToWiFi();
    registerTools();
    demonstrateToolCalling();
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
    Serial.println("Registering tools...");

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
    Serial.print("  -> ");
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

void demonstrateToolCalling() {
    Serial.println("Sending request that requires tool calls...");
    Serial.println();

    std::vector<Message> messages;
    messages.push_back(Message(
        Role::User,
        "What's the current temperature and humidity? Also, please turn on the LED."
    ));

    ChatOptions options;
    options.maxTokens = 256;

    Serial.println("Step 1: Sending initial request...");
    Response response = ai.chat(messages, options);

    if (!response.success) {
        Serial.print("Error: ");
        Serial.println(response.errorMessage);
        return;
    }

    if (!ai.hasToolCalls()) {
        Serial.println("No tool calls requested. Response:");
        Serial.println(response.content);
        return;
    }

    const auto& toolCalls = ai.getLastToolCalls();
    Serial.print("Step 2: AI requested ");
    Serial.print(toolCalls.size());
    Serial.println(" tool call(s)");

    // Assistant message with tool_calls must precede tool results
    messages.push_back(ai.getAssistantMessageWithToolCalls(response.content));

    Serial.println("Step 3: Executing tool calls...");
    for (const auto& call : toolCalls) {
        String result = executeTool(call);
        messages.push_back(Message(Role::Tool, result, call.id));
    }

    Serial.println("Step 4: Sending tool results back to AI...");
    ai.clearTools();
    response = ai.chat(messages, options);

    if (response.success) {
        Serial.println();
        Serial.println("Final Response:");
        Serial.println("---------------");
        Serial.println(response.content);
        Serial.println("---------------");
    } else {
        Serial.print("Error: ");
        Serial.println(response.errorMessage);
    }
}
