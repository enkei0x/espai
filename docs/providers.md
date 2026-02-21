# 🔌 Providers Guide

ESPAI supports multiple AI providers. This guide covers the differences and how to use each one.

## Supported Providers

| Provider | Status | Models |
|----------|--------|--------|
| OpenAI | ✅ Supported | All text models |
| Anthropic | ✅ Supported | All text models |
| Gemini | ✅ Supported | All text models |
| Ollama | ✅ Supported | Any local model |
| OpenAI-compatible | ✅ Supported | Groq, DeepSeek, Together AI, LM Studio, OpenRouter, etc. |

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

## 🟡 Google Gemini

### Setup

```cpp
#include <ESPAI.h>
using namespace ESPAI;

GeminiProvider gemini("AIza-your-api-key");
```

### Default Model

`gemini-2.5-flash`

### Change Model

```cpp
gemini.setModel("gemini-2.5-pro");
gemini.setModel("gemini-2.0-flash");
```

### Authentication

Uses `x-goog-api-key: <key>` header (different from OpenAI and Anthropic).

### Thinking Budget (Gemini 2.5+)

Control the thinking token budget for complex reasoning tasks:

```cpp
ChatOptions opts;
opts.thinkingBudget = 8192;   // Set thinking budget
opts.thinkingBudget = 0;      // Disable thinking
opts.thinkingBudget = -1;     // Use provider default (default)
```

### Gemini-Specific Notes

- Tool call IDs are synthesized as `gemini_tc_N` since the Gemini API doesn't return tool call IDs
- Uses a Gemini-specific SSE streaming format (`streamGenerateContent?alt=sse`)
- API URL is constructed per-model: `https://generativelanguage.googleapis.com/v1beta/models/{model}:generateContent`

---

## 🟠 Ollama (Local LLMs)

### Setup

No API key needed — just a running Ollama instance on your network.

```cpp
#include <ESPAI.h>
using namespace ESPAI;

OllamaProvider ai;  // Connects to localhost:11434
```

### Default Model

`llama3.2`

### Change Model

```cpp
ai.setModel("mistral");
ai.setModel("codellama");
ai.setModel("gemma2");
```

### Custom Host

For Ollama running on another machine:

```cpp
ai.setBaseUrl("http://192.168.1.100:11434/v1/chat/completions");
```

### Authentication

None required by default. Uses the OpenAI-compatible `/v1/chat/completions` endpoint over plain HTTP.

---

## 🔵 OpenAI-Compatible (Custom Providers)

For any provider that follows the OpenAI chat completions API format (Groq, DeepSeek, Together AI, LM Studio, OpenRouter, etc.).

### Setup

```cpp
#include <ESPAI.h>
using namespace ESPAI;

OpenAICompatibleConfig config;
config.name = "Groq";
config.baseUrl = "https://api.groq.com/openai/v1/chat/completions";
config.apiKey = "gsk-...";
config.model = "llama-3.3-70b-versatile";

OpenAICompatibleProvider ai(config);
```

### Configuration Options

```cpp
OpenAICompatibleConfig config;
config.name = "MyProvider";
config.baseUrl = "https://api.example.com/v1/chat/completions";
config.apiKey = "your-key";
config.model = "your-model";
config.authHeaderName = "Authorization";      // default
config.authHeaderValuePrefix = "Bearer ";     // default
config.requiresApiKey = true;                 // set false for local providers
config.toolCallingSupported = true;           // set false if provider doesn't support tools
```

### Custom Headers

```cpp
OpenAICompatibleProvider ai(config);
ai.addCustomHeader("X-Custom-Header", "value");
```

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

All providers use the same unified API:

```cpp
Tool tool;
tool.name = "get_weather";
tool.description = "Get weather";
tool.parametersJson = R"({"type":"object","properties":{}})";

ai.addTool(tool);  // Works for OpenAI, Anthropic, Gemini, and Ollama

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

// Gemini - same interface!
GeminiProvider gemini("AIza...");
Response resp = gemini.chat(messages, options);

// Ollama - same interface, no key needed!
OllamaProvider ollama;
Response resp = ollama.chat(messages, options);
```

You can even use multiple providers in the same project:

```cpp
OpenAIProvider gpt("sk-...");
AnthropicProvider claude("sk-ant-...");
GeminiProvider gemini("AIza...");
OllamaProvider ollama;

// Use GPT for quick tasks
auto quick = gpt.chat(simpleMessages, opts);

// Use Claude for complex reasoning
auto detailed = claude.chat(complexMessages, opts);

// Use Gemini for multimodal tasks
auto geminiResp = gemini.chat(messages, opts);

// Use Ollama for offline/local processing
auto local = ollama.chat(simpleMessages, opts);
```
