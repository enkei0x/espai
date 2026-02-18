# Tool Calling Guide

Tool calling (also known as function calling) allows AI models to request execution of functions you define. This enables powerful integrations with sensors, actuators, APIs, and more.

## How It Works

```
1. You define tools (functions the AI can call)
2. User sends a message
3. AI decides to call one or more tools
4. You execute the tools and return results
5. AI generates final response using tool results
```

---

## Quick Example

```cpp
#include <ESPAI.h>
using namespace ESPAI;

OpenAIProvider ai("sk-...");

void setup() {
    // 1. Define a tool (same struct for all providers)
    Tool tempTool;
    tempTool.name = "get_temperature";
    tempTool.description = "Get current room temperature";
    tempTool.parametersJson = R"({"type":"object","properties":{}})";

    ai.addTool(tempTool);

    // 2. Send message
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What's the temperature?"));

    ChatOptions options;
    Response response = ai.chat(messages, options);

    // 3. Check for tool calls
    if (ai.hasToolCalls()) {
        // 4. Execute tools
        for (const auto& call : ai.getLastToolCalls()) {
            if (call.name == "get_temperature") {
                float temp = readSensor();  // Your function

                // 5. Add assistant message with tool calls
                messages.push_back(ai.getAssistantMessageWithToolCalls(response.content));

                // 6. Add tool result
                messages.push_back(Message(Role::Tool,
                    "{\"temperature\": " + String(temp) + "}",
                    call.id));
            }
        }

        // 7. Get final response
        response = ai.chat(messages, options);
    }

    Serial.println(response.content);
}
```

---

## Defining Tools

### Unified Tool Struct (All Providers)

ESPAI uses a unified `Tool` struct that works with all providers:

```cpp
Tool tool;
tool.name = "function_name";           // Required: unique identifier
tool.description = "What it does";     // Required: helps AI decide when to use
tool.parametersJson = "...";           // Required: JSON schema for parameters
```

The same tool definition works for both OpenAI and Anthropic:
- OpenAI maps `parametersJson` to the `parameters` field
- Anthropic maps `parametersJson` to the `input_schema` field

---

## Parameter Schemas

Parameters use JSON Schema format (same for all providers).

### No Parameters

```cpp
tool.parametersJson = R"({
    "type": "object",
    "properties": {}
})";
```

### Single Parameter

```cpp
tool.parametersJson = R"({
    "type": "object",
    "properties": {
        "location": {
            "type": "string",
            "description": "City name"
        }
    },
    "required": ["location"]
})";
```

### Multiple Parameters

```cpp
tool.parametersJson = R"({
    "type": "object",
    "properties": {
        "room": {
            "type": "string",
            "description": "Room name",
            "enum": ["living_room", "bedroom", "kitchen"]
        },
        "brightness": {
            "type": "integer",
            "description": "Brightness 0-100"
        }
    },
    "required": ["room"]
})";
```

### Boolean Parameter

```cpp
tool.parametersJson = R"({
    "type": "object",
    "properties": {
        "state": {
            "type": "boolean",
            "description": "true = on, false = off"
        }
    },
    "required": ["state"]
})";
```

---

## Complete Flow

### Step 1: Register Tools

```cpp
Tool getTempTool;
getTempTool.name = "get_temperature";
getTempTool.description = "Read temperature sensor in Celsius";
getTempTool.parametersJson = R"({"type":"object","properties":{}})";

Tool setLedTool;
setLedTool.name = "set_led";
setLedTool.description = "Control the LED";
setLedTool.parametersJson = R"({
    "type": "object",
    "properties": {
        "state": {"type": "boolean"}
    },
    "required": ["state"]
})";

ai.addTool(getTempTool);
ai.addTool(setLedTool);
```

### Step 2: Send Initial Request

```cpp
std::vector<Message> messages;
messages.push_back(Message(Role::User, "Turn on the LED and tell me the temperature"));

ChatOptions options;
options.maxTokens = 256;

Response response = ai.chat(messages, options);
```

### Step 3: Process Tool Calls

```cpp
if (ai.hasToolCalls()) {
    // Add assistant's message (with tool_calls) to history
    messages.push_back(ai.getAssistantMessageWithToolCalls(response.content));

    // Execute each tool
    for (const auto& call : ai.getLastToolCalls()) {
        String result = executeMyTool(call.name, call.arguments);

        // Add tool result to messages
        messages.push_back(Message(Role::Tool, result, call.id));
    }

    // Get final response
    response = ai.chat(messages, options);
}
```

### Step 4: Execute Tools

```cpp
String executeMyTool(const String& name, const String& args) {
    if (name == "get_temperature") {
        float temp = dht.readTemperature();
        return "{\"temperature\": " + String(temp) + ", \"unit\": \"celsius\"}";
    }

    if (name == "set_led") {
        // Parse args JSON
        JsonDocument doc;
        deserializeJson(doc, args);
        bool state = doc["state"];

        digitalWrite(LED_PIN, state ? HIGH : LOW);
        return "{\"success\": true, \"state\": " + String(state ? "true" : "false") + "}";
    }

    return "{\"error\": \"Unknown tool\"}";
}
```

---

## Unified API (OpenAI and Anthropic)

ESPAI provides a unified tool calling API that works identically for both OpenAI and Anthropic:

| Aspect | All Providers |
|--------|---------------|
| Tool struct | `Tool` |
| Schema field | `parametersJson` |
| Check method | `hasToolCalls()` |
| Get results | `getLastToolCalls()` |
| Result type | `ToolCall` |
| Args field | `call.arguments` |
| Assistant msg | `getAssistantMessageWithToolCalls()` |

This means you can write provider-agnostic code:

```cpp
// Works with OpenAIProvider or AnthropicProvider
void processTool(AIProvider& provider) {
    if (provider.hasToolCalls()) {
        const auto& calls = provider.getLastToolCalls();
        for (const auto& call : calls) {
            Serial.println(call.name);
            Serial.println(call.arguments);
        }
    }
}
```

---

## Best Practices

### 1. Clear Descriptions

```cpp
// ❌ Bad
tool.description = "LED";

// ✅ Good
tool.description = "Turn the LED on or off. Use state=true for on, state=false for off.";
```

### 2. Validate Tool Results

```cpp
String executeMyTool(const String& name, const String& args) {
    if (name == "set_temperature") {
        JsonDocument doc;
        if (deserializeJson(doc, args)) {
            return "{\"error\": \"Invalid JSON\"}";
        }

        int temp = doc["temperature"];
        if (temp < 16 || temp > 30) {
            return "{\"error\": \"Temperature must be 16-30\"}";
        }

        // Execute...
        return "{\"success\": true}";
    }
}
```

### 3. Return Structured JSON

```cpp
// ❌ Bad
return "The temperature is 22 degrees";

// ✅ Good
return "{\"temperature\": 22, \"unit\": \"celsius\", \"humidity\": 45}";
```

### 4. Handle Multiple Tool Calls

AI may request multiple tools at once:

```cpp
if (ai.hasToolCalls()) {
    const auto& calls = ai.getLastToolCalls();
    Serial.printf("AI requested %d tool(s)\n", calls.size());

    for (const auto& call : calls) {
        // Execute each one
    }
}
```

### 5. Clear Tools When Done

```cpp
ai.clearTools();  // Remove all registered tools
```

---

## Use Cases

| Use Case | Tools |
|----------|-------|
| Smart Home | `set_light`, `set_thermostat`, `lock_door` |
| Weather Station | `get_temperature`, `get_humidity`, `get_pressure` |
| Robot Control | `move_forward`, `turn`, `grab_object` |
| Data Query | `search_database`, `get_user_info` |
| IoT Dashboard | `get_sensor_data`, `send_alert` |
