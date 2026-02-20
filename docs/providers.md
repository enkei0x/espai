# 🔌 Providers Guide

ESPAI supports multiple AI providers. This guide covers the differences and how to use each one.

## Supported Providers

| Provider | Status | Models |
|----------|--------|--------|
| OpenAI | ✅ Supported | All text models |
| Anthropic | ✅ Supported | All text models |
| Gemini | 🔜 Planned | - |
| Ollama | 🔜 Planned | - |

---

## 🟢 OpenAI

### Setup

```cpp
#include <ESPAI.h>
using namespace ESPAI;

OpenAIProvider ai("sk-your-api-key");
```

### Default Model

`gpt-4.1-mini`

### Change Model

```cpp
ai.setModel("gpt-4.1");
ai.setModel("gpt-4.1-nano");
```

### Custom Endpoint

For Azure OpenAI or proxies:

```cpp
ai.setBaseUrl("https://your-endpoint.openai.azure.com/...");
```

### Authentication

Uses `Authorization: Bearer <key>` header.

---

## 🟣 Anthropic (Claude)

### Setup

```cpp
#include <ESPAI.h>
using namespace ESPAI;

AnthropicProvider claude("sk-ant-your-api-key");
```

### Default Model

`claude-sonnet-4-20250514`

### Change Model

```cpp
claude.setModel("claude-opus-4-20250514");
claude.setModel("claude-haiku-4-5-20251001");
```

### API Version

Default: `2023-06-01`. Change if needed:

```cpp
claude.setApiVersion("2024-01-01");
```

### Authentication

Uses `x-api-key: <key>` header (different from OpenAI).

---

## 🔄 Key Differences

### System Messages

**OpenAI:** System message is part of the messages array.

```cpp
messages.push_back(Message(Role::System, "You are helpful."));
messages.push_back(Message(Role::User, "Hello!"));
```

**Anthropic:** System message is sent as a separate field (handled automatically).

```cpp
// Same code works - ESPAI handles the conversion
messages.push_back(Message(Role::System, "You are helpful."));
messages.push_back(Message(Role::User, "Hello!"));
```

Or use `ChatOptions`:

```cpp
ChatOptions opts;
opts.systemPrompt = "You are helpful.";
```

### Tool Calling

Both providers use the same unified API:

```cpp
Tool tool;
tool.name = "get_weather";
tool.description = "Get weather";
tool.parametersJson = R"({"type":"object","properties":{}})";

ai.addTool(tool);  // Works for both OpenAI and Anthropic

if (ai.hasToolCalls()) {
    for (const auto& call : ai.getLastToolCalls()) {
        // call.id, call.name, call.arguments
    }
}
```

### Response Format

**OpenAI:** Content is a simple string.

**Anthropic:** Content is returned as an array internally, but ESPAI extracts it as a string for you.

### Token Counting

Both providers return token usage:

```cpp
Response resp = ai.chat(messages, options);
Serial.printf("Prompt: %d, Completion: %d, Total: %d\n",
    resp.promptTokens,
    resp.completionTokens,
    resp.totalTokens());
```

---

## 🔀 Switching Providers

The API is nearly identical, making it easy to switch:

```cpp
// OpenAI
OpenAIProvider ai("sk-...");
Response resp = ai.chat(messages, options);

// Anthropic - same interface!
AnthropicProvider claude("sk-ant-...");
Response resp = claude.chat(messages, options);
```

You can even use both in the same project:

```cpp
OpenAIProvider gpt("sk-...");
AnthropicProvider claude("sk-ant-...");

// Use GPT for quick tasks
auto quick = gpt.chat(simpleMessages, opts);

// Use Claude for complex reasoning
auto detailed = claude.chat(complexMessages, opts);
```
