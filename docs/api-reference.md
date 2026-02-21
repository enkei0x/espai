# API Reference

Complete reference for all ESPAI classes and methods.

---

## Namespace

All ESPAI classes are in the `ESPAI` namespace:

```cpp
using namespace ESPAI;
// or
ESPAI::OpenAIProvider ai("key");
```

---

## Enums

### Role

Message roles in a conversation.

```cpp
enum class Role {
    System,     // System instructions
    User,       // User messages
    Assistant,  // AI responses
    Tool        // Tool/function results
};
```

### Provider

Supported AI providers.

```cpp
enum class Provider {
    OpenAI,
    Anthropic,
    Gemini,
    Ollama,
    Custom     // For OpenAI-compatible providers
};
```

### ErrorCode

Error types returned in responses.

```cpp
enum class ErrorCode {
    None,                // No error
    NetworkError,        // Connection failed
    Timeout,             // Request timed out
    AuthError,           // Invalid API key
    RateLimited,         // Too many requests
    InvalidRequest,      // Bad request parameters
    ServerError,         // API server error
    ParseError,          // Invalid JSON response
    OutOfMemory,         // Memory allocation failed
    ProviderNotSupported,// Provider not available
    NotConfigured,       // Provider not set up
    StreamingError,      // Error during streaming
    ResponseTooLarge     // Response exceeded max size
};
```

---

## Structs

### Message

A single message in the conversation.

```cpp
struct Message {
    Role role;
    String content;
    String name;           // Tool call ID (for Role::Tool)
    String toolCallsJson;  // Tool calls (for assistant messages)

    // Constructors
    Message();
    Message(Role role, const String& content);
    Message(Role role, const String& content, const String& name);

    // Methods
    bool hasToolCalls() const;
};
```

**Examples:**
```cpp
Message userMsg(Role::User, "Hello!");
Message systemMsg(Role::System, "You are helpful.");
Message toolResult(Role::Tool, "{\"temp\": 22}", "call_abc123");
```

### Response

Response from an AI provider.

```cpp
struct Response {
    bool success;
    String content;
    ErrorCode error;
    String errorMessage;
    int16_t httpStatus;
    uint32_t promptTokens;
    uint32_t completionTokens;

    // Methods
    uint32_t totalTokens() const;
    static Response ok(const String& content);
    static Response fail(ErrorCode code, const String& message, int16_t status = 0);
};
```

**Example:**
```cpp
Response resp = ai.chat(messages, options);
if (resp.success) {
    Serial.println(resp.content);
    Serial.printf("Tokens: %d\n", resp.totalTokens());
} else {
    Serial.println(resp.errorMessage);
}
```

### ChatOptions

Options for chat requests.

```cpp
struct ChatOptions {
    float temperature = 0.7f;       // Creativity (0.0 - 2.0)
    int16_t maxTokens = 1024;       // Max response length
    String model;                   // Override default model
    String systemPrompt;            // System instructions
    float topP = 1.0f;              // Nucleus sampling
    float frequencyPenalty = 0.0f;  // Reduce repetition
    float presencePenalty = 0.0f;   // Encourage new topics
    int32_t thinkingBudget = -1;    // Gemini 2.5+: thinking token budget (-1 = default, 0 = disable)
};
```

**Example:**
```cpp
ChatOptions opts;
opts.temperature = 0.3f;  // More focused
opts.maxTokens = 500;
opts.model = "gpt-4o";
```

### HttpRequest

HTTP request built by providers.

```cpp
struct HttpRequest {
    String url;
    String method;
    String body;
    String contentType;
    std::vector<std::pair<String, String>> headers;
    uint32_t timeout;
    uint32_t maxResponseSize;
};
```

### HttpResponse

HTTP response from transport layer.

```cpp
struct HttpResponse {
    int16_t statusCode;
    String body;
    bool success;
    bool responseTooLarge;
    int32_t retryAfterSeconds;
};
```

---

## OpenAIProvider

Provider for OpenAI GPT models.

### Constructor

```cpp
OpenAIProvider(const String& apiKey, const String& model = "gpt-4.1-mini");
```

### Methods

| Method | Description |
|--------|-------------|
| `chat(messages, options)` | Send chat request, get response |
| `chatStream(messages, options, callback)` | Stream response token by token |
| `setModel(model)` | Set default model |
| `getModel()` | Get current model |
| `setBaseUrl(url)` | Set custom API endpoint |
| `setTimeout(ms)` | Set request timeout |
| `addTool(tool)` | Register a tool/function |
| `clearTools()` | Remove all tools |
| `hasToolCalls()` | Check if last response has tool calls |
| `getLastToolCalls()` | Get tool calls from last response |
| `getAssistantMessageWithToolCalls(content)` | Build assistant message with tool calls |

### Example

```cpp
OpenAIProvider ai("sk-...");
ai.setModel("gpt-4o");
ai.setTimeout(30000);

std::vector<Message> msgs;
msgs.push_back(Message(Role::User, "Hello!"));

Response resp = ai.chat(msgs, ChatOptions());
```

---

## AnthropicProvider

Provider for Anthropic Claude models.

### Constructor

```cpp
AnthropicProvider(const String& apiKey, const String& model = "claude-sonnet-4-20250514");
```

### Methods

| Method | Description |
|--------|-------------|
| `chat(messages, options)` | Send chat request |
| `chatStream(messages, options, callback)` | Stream response |
| `setModel(model)` | Set default model |
| `setApiVersion(version)` | Set API version header |
| `addTool(tool)` | Register a tool |
| `clearTools()` | Remove all tools |
| `hasToolCalls()` | Check if last response has tool calls |
| `getLastToolCalls()` | Get tool calls from last response |
| `getAssistantMessageWithToolCalls(content)` | Build assistant message with tool calls |

### Example

```cpp
AnthropicProvider claude("sk-ant-...");
claude.setModel("claude-opus-4-20250514");

std::vector<Message> msgs;
msgs.push_back(Message(Role::User, "Hello Claude!"));

Response resp = claude.chat(msgs, ChatOptions());
```

---

## Conversation

Manages conversation history with automatic pruning.

### Constructor

```cpp
Conversation();
```

### Methods

| Method | Description |
|--------|-------------|
| `setSystemPrompt(prompt)` | Set system instructions |
| `addUserMessage(content)` | Add user message |
| `addAssistantMessage(content)` | Add AI response |
| `addMessage(role, content)` | Add any message |
| `getMessages()` | Get all messages |
| `setMaxMessages(n)` | Set max history size |
| `clear()` | Clear all messages |
| `size()` | Get message count |
| `estimateTokens()` | Estimate total tokens |
| `toJson()` | Serialize to JSON |
| `fromJson(json)` | Deserialize from JSON |

### Example

```cpp
Conversation conv;
conv.setSystemPrompt("You are a helpful assistant.");
conv.setMaxMessages(20);

conv.addUserMessage("Hi!");
Response resp = ai.chat(conv.getMessages(), ChatOptions());
conv.addAssistantMessage(resp.content);

conv.addUserMessage("Follow up question...");
// Context is preserved!
```

---

## Tool Calling

ESPAI provides unified tool calling structs that work with all providers.

### Tool

Define a tool/function the AI can call.

```cpp
struct Tool {
    String name;           // Unique function name
    String description;    // What the tool does
    String parametersJson; // JSON schema for parameters
    ToolHandler handler;   // Optional: for ToolRegistry execution
};
```

The same `parametersJson` schema works for all providers:
- OpenAI, Ollama, and OpenAI-compatible providers map it to the `parameters` field
- Anthropic maps it to the `input_schema` field
- Gemini maps it to the Gemini function declarations format

### ToolCall

Represents a tool call requested by the AI.

```cpp
struct ToolCall {
    String id;         // Unique call ID
    String name;       // Tool name
    String arguments;  // JSON string of arguments
};
```

### ToolResult

Result from executing a tool call.

```cpp
struct ToolResult {
    String toolCallId;  // Matches ToolCall.id
    String toolName;    // Tool name
    String result;      // JSON result string
    bool success;       // Execution status
};
```

See [Tool Calling Guide](tool-calling.md) for complete examples.

---

## GeminiProvider

Provider for Google Gemini models.

### Constructor

```cpp
GeminiProvider(const String& apiKey, const String& model = "gemini-2.5-flash");
```

### Methods

| Method | Description |
|--------|-------------|
| `chat(messages, options)` | Send chat request |
| `chatStream(messages, options, callback)` | Stream response |
| `setModel(model)` | Set default model |
| `setBaseUrl(url)` | Set custom API endpoint |
| `setTimeout(ms)` | Set request timeout |
| `addTool(tool)` | Register a tool |
| `clearTools()` | Remove all tools |
| `hasToolCalls()` | Check if last response has tool calls |
| `getLastToolCalls()` | Get tool calls from last response |
| `getAssistantMessageWithToolCalls(content)` | Build assistant message with tool calls |

### Gemini-Specific Features

- **Authentication:** Uses `x-goog-api-key` header (not `Authorization: Bearer`)
- **SSE format:** Uses Gemini-specific SSE streaming (`streamGenerateContent?alt=sse`)
- **Tool call IDs:** Synthesized as `gemini_tc_N` (Gemini API does not return tool call IDs)
- **Thinking budget:** Use `ChatOptions::thinkingBudget` to control thinking tokens for Gemini 2.5+ models

### Example

```cpp
GeminiProvider gemini("AIza...");
gemini.setModel("gemini-2.5-pro");

std::vector<Message> msgs;
msgs.push_back(Message(Role::User, "Hello Gemini!"));

// Use thinking budget for complex tasks (Gemini 2.5+ only)
ChatOptions opts;
opts.thinkingBudget = 8192;

Response resp = gemini.chat(msgs, opts);
```

---

## OllamaProvider

Provider for local LLMs via Ollama. Extends `OpenAICompatibleProvider` with no API key required.

### Constructor

```cpp
OllamaProvider(const String& apiKey = "", const String& model = "llama3.2");
```

### Methods

Inherits all methods from `OpenAICompatibleProvider`. No API key is needed by default.

### Example

```cpp
OllamaProvider ollama;
ollama.setModel("mistral");
ollama.setBaseUrl("http://192.168.1.100:11434/v1/chat/completions");

std::vector<Message> msgs;
msgs.push_back(Message(Role::User, "Hello!"));

Response resp = ollama.chat(msgs, ChatOptions());
```

---

## OpenAICompatibleProvider

Base class for any provider that uses the OpenAI chat completions API format. Used directly for custom providers (Groq, DeepSeek, Together AI, LM Studio, OpenRouter, etc.) and as the base for `OpenAIProvider` and `OllamaProvider`.

### OpenAICompatibleConfig

```cpp
struct OpenAICompatibleConfig {
    String name;                                    // Provider display name
    String baseUrl;                                 // API endpoint URL
    String apiKey;                                  // API key
    String model;                                   // Default model
    String authHeaderName = "Authorization";        // Auth header name
    String authHeaderValuePrefix = "Bearer ";       // Auth header value prefix
    bool requiresApiKey = true;                     // Whether API key is required
    bool toolCallingSupported = true;               // Whether provider supports tools
    Provider providerType = Provider::Custom;       // Provider enum type
};
```

### Constructors

```cpp
// Config-based (recommended for custom providers)
explicit OpenAICompatibleProvider(const OpenAICompatibleConfig& config);

// Legacy constructor (used by thin subclasses)
OpenAICompatibleProvider(const String& apiKey, const String& model,
    const String& baseUrl, const String& name, Provider providerType);
```

### Methods

| Method | Description |
|--------|-------------|
| `chat(messages, options)` | Send chat request, get response |
| `chatStream(messages, options, callback)` | Stream response token by token |
| `setModel(model)` | Set default model |
| `getModel()` | Get current model |
| `setBaseUrl(url)` | Set custom API endpoint |
| `setApiKey(key)` | Set API key |
| `setTimeout(ms)` | Set request timeout |
| `addTool(tool)` | Register a tool/function |
| `clearTools()` | Remove all tools |
| `hasToolCalls()` | Check if last response has tool calls |
| `getLastToolCalls()` | Get tool calls from last response |
| `getAssistantMessageWithToolCalls(content)` | Build assistant message with tool calls |
| `addCustomHeader(name, value)` | Add extra HTTP header |
| `isConfigured()` | Check if provider is ready to use |
| `supportsTools()` | Whether tools are supported |

### Example

```cpp
OpenAICompatibleConfig config;
config.name = "Groq";
config.baseUrl = "https://api.groq.com/openai/v1/chat/completions";
config.apiKey = "gsk-...";
config.model = "llama-3.3-70b-versatile";

OpenAICompatibleProvider ai(config);
ai.addCustomHeader("X-Custom-Header", "value");

std::vector<Message> msgs;
msgs.push_back(Message(Role::User, "Hello!"));

Response resp = ai.chat(msgs, ChatOptions());
```

---

## ProviderFactory

Create providers dynamically.

```cpp
// Register (usually done automatically)
ProviderFactory::registerProvider(Provider::OpenAI, createOpenAIProvider);

// Create
auto provider = ProviderFactory::create(Provider::OpenAI, "api-key", "model");

// Check
bool registered = ProviderFactory::isRegistered(Provider::OpenAI);
```

---

## HttpTransportESP32

Low-level HTTP transport for ESP32. Most users won't need to interact with this directly. Supports both HTTPS (for cloud APIs) and plain HTTP (for local providers like Ollama).

### SSL/TLS Configuration

ESPAI validates SSL certificates by default using built-in root CA certificates that cover api.openai.com and api.anthropic.com. Plain HTTP is used automatically for `http://` URLs (e.g., local Ollama).

```cpp
#include <ESPAI.h>

// Get the transport instance
HttpTransportESP32* transport = ESPAI::getESP32Transport();

// Use a custom CA certificate (for custom endpoints)
transport->setCACert(my_ca_cert);

// DANGEROUS: Disable certificate validation (testing only!)
transport->setInsecure(true);  // Logs a warning
```

### Methods

| Method | Description |
|--------|-------------|
| `setCACert(cert)` | Set custom CA certificate for SSL validation |
| `setInsecure(bool)` | Enable/disable certificate validation (default: false) |
| `setReuse(bool)` | Enable/disable connection reuse |
| `setFollowRedirects(mode)` | Set redirect following behavior |
| `isReady()` | Check if WiFi is connected |
| `getLastError()` | Get last error message |

### Security Notes

- **Default behavior:** SSL certificate validation is ENABLED for HTTPS URLs
- **Plain HTTP:** Used automatically for `http://` URLs (local providers) — no SSL overhead
- **Built-in certificates:** GlobalSign (OpenAI, Anthropic) and GTS Root R1 (Gemini)
- **setInsecure(true):** Logs a warning and disables validation - use only for testing
- **Custom endpoints:** Use `setCACert()` with appropriate certificates

---

## RetryConfig

Configure automatic retry with exponential backoff for failed requests.

```cpp
struct RetryConfig {
    bool enabled = false;           // Enable/disable retries
    uint8_t maxRetries = 3;         // Maximum retry attempts
    uint32_t initialDelayMs = 1000; // Initial delay between retries
    float backoffMultiplier = 2.0f; // Delay multiplier per retry
    uint32_t maxDelayMs = 30000;    // Maximum delay cap
};
```

### Usage

```cpp
RetryConfig retry;
retry.enabled = true;
retry.maxRetries = 3;
retry.initialDelayMs = 1000;

ai.setRetryConfig(retry);

// Requests will now automatically retry on 429, 500, 502, 503, 504
Response resp = ai.chat(messages, options);
```

Retries are attempted for rate limiting (429) and server errors (5xx). The delay between retries grows exponentially: `initialDelayMs * backoffMultiplier^attempt`, capped at `maxDelayMs`. If the server returns a `Retry-After` header, that value is used instead.

---

## AIClient

High-level convenience wrapper around providers. Provides a simplified string-based API and manages provider lifecycle.

### Constructors

```cpp
AIClient();
AIClient(Provider provider, const String& apiKey);
```

### Methods

| Method | Description |
|--------|-------------|
| `setProvider(provider, apiKey)` | Set provider type and API key |
| `setBaseUrl(url)` | Set custom API endpoint |
| `setModel(model)` | Override default model |
| `setTimeout(ms)` | Set request timeout |
| `getProvider()` | Get current provider type |
| `getModel()` | Get current model name |
| `isConfigured()` | Check if provider is ready |
| `chat(message)` | Send a simple message |
| `chat(message, options)` | Send with options |
| `chat(systemPrompt, message)` | Send with system prompt |
| `chatStream(message, callback)` | Stream a response |
| `chatStream(message, options, callback)` | Stream with options |
| `chatAsync(message, onComplete)` | Send async (FreeRTOS) |
| `chatStreamAsync(message, streamCb, onDone)` | Stream async (FreeRTOS) |
| `isAsyncBusy()` | Check if async request is running |
| `cancelAsync()` | Cancel running async request |
| `getLastError()` | Get last error message |
| `getLastHttpStatus()` | Get last HTTP status code |
| `reset()` | Clear error state |
| `getProviderInstance()` | Access underlying AIProvider |

### Example

```cpp
AIClient client(Provider::OpenAI, "sk-...");
client.setModel("gpt-4o");

// Simple string-based API
Response resp = client.chat("Hello from ESP32!");

// With system prompt
resp = client.chat("You are a helpful IoT assistant.", "Turn on the lights");

// Streaming
client.chatStream("Tell me a story", [](const String& chunk, bool done) {
    Serial.print(chunk);
});
```

---

## Async API (FreeRTOS)

Non-blocking chat requests using FreeRTOS tasks. Available on ESP32 when `ESPAI_ENABLE_ASYNC` is enabled (default on Arduino).

### AsyncStatus

```cpp
enum class AsyncStatus {
    Idle,       // No task running
    Running,    // Task in progress
    Completed,  // Task finished successfully
    Cancelled,  // Task was cancelled
    Error       // Task failed
};
```

### ChatRequest

Handle returned by async methods. Use to poll status, get results, or cancel.

```cpp
struct ChatRequest {
    AsyncStatus getStatus() const;
    bool isComplete() const;
    bool isCancelled() const;
    Response getResult() const;
    void cancel();
    bool poll();   // Check status & invoke callback if done
};
```

### Callbacks

```cpp
using AsyncChatCallback = std::function<void(const Response&)>;
using AsyncDoneCallback = std::function<void(const Response&)>;
```

### Provider-Level Async

```cpp
ChatRequest* chatAsync(messages, options, onComplete);
ChatRequest* chatStreamAsync(messages, options, streamCb, onDone);
bool isAsyncBusy() const;
void cancelAsync();
```

### AIClient-Level Async

```cpp
ChatRequest* chatAsync(message, onComplete);
ChatRequest* chatAsync(message, options, onComplete);
ChatRequest* chatStreamAsync(message, streamCb, onDone);
ChatRequest* chatStreamAsync(message, options, streamCb, onDone);
bool isAsyncBusy() const;
void cancelAsync();
```

### Example

```cpp
AIClient client(Provider::OpenAI, "sk-...");

// Fire-and-forget with callback
client.chatAsync("Hello!", [](const Response& resp) {
    Serial.println(resp.content);
});

// Poll-based
ChatRequest* req = client.chatAsync("Hello!");
while (!req->isComplete()) {
    req->poll();
    // Do other work...
    delay(10);
}
Serial.println(req->getResult().content);

// Async streaming
client.chatStreamAsync("Tell me a story",
    [](const String& chunk, bool done) { Serial.print(chunk); },
    [](const Response& resp) { Serial.println("\nDone!"); }
);

// Cancel
client.cancelAsync();
```

### Configuration

```cpp
#define ESPAI_ENABLE_ASYNC      1       // Enable async (default: 1 on Arduino, 0 on native)
#define ESPAI_ASYNC_STACK_SIZE  20480   // FreeRTOS task stack size (bytes)
#define ESPAI_ASYNC_TASK_PRIORITY 3     // FreeRTOS task priority
```

---

## ToolRegistry

Standalone tool management and execution engine. Use when you need to manage tools independently of a provider.

### Methods

| Method | Description |
|--------|-------------|
| `registerTool(tool)` | Register a tool with handler |
| `unregisterTool(name)` | Remove a tool by name |
| `clearTools()` | Remove all tools |
| `findTool(name)` | Find tool by name |
| `hasTool(name)` | Check if tool exists |
| `toolCount()` | Get number of registered tools |
| `executeToolCall(call)` | Execute a single tool call |
| `executeToolCalls(calls)` | Execute multiple tool calls |
| `toOpenAISchema()` | Generate OpenAI-format tool schema JSON |
| `toAnthropicSchema()` | Generate Anthropic-format tool schema JSON |
| `setMaxIterations(n)` | Set max tool iterations (default: 10) |

### Example

```cpp
ToolRegistry registry;

Tool tempTool("get_temperature", "Get room temperature",
    R"({"type":"object","properties":{}})",
    [](const String& args) { return "{\"temp\": 23.5}"; }
);

registry.registerTool(tempTool);

// Execute tool calls from AI response
auto results = registry.executeToolCalls(ai.getLastToolCalls());
```

---

## Configuration Reference

All compile-time configuration defines (set before `#include <ESPAI.h>`):

| Define | Default | Description |
|--------|---------|-------------|
| `ESPAI_ENABLE_STREAMING` | `1` | Enable SSE streaming support |
| `ESPAI_ENABLE_TOOLS` | `1` | Enable tool/function calling |
| `ESPAI_ENABLE_ASYNC` | `1` (Arduino) / `0` (native) | Enable FreeRTOS async API |
| `ESPAI_PROVIDER_OPENAI` | `1` | Include OpenAI provider |
| `ESPAI_PROVIDER_ANTHROPIC` | `1` | Include Anthropic provider |
| `ESPAI_PROVIDER_GEMINI` | `1` | Include Gemini provider |
| `ESPAI_PROVIDER_OLLAMA` | `1` | Include Ollama provider |
| `ESPAI_MAX_MESSAGES` | `20` | Default max conversation messages |
| `ESPAI_HTTP_TIMEOUT_MS` | `30000` | Default HTTP timeout (ms) |
| `ESPAI_MAX_TOOLS` | `10` | Maximum registered tools |
| `ESPAI_MAX_RESPONSE_SIZE` | `32768` | Maximum HTTP response body size (bytes) |
| `ESPAI_MAX_TOOL_ITERATIONS` | `10` | Maximum tool call iterations |
| `ESPAI_ASYNC_STACK_SIZE` | `20480` | FreeRTOS async task stack size |
| `ESPAI_ASYNC_TASK_PRIORITY` | `3` | FreeRTOS async task priority |
| `ESPAI_DEBUG` | `0` | Enable debug logging (`ESPAI_LOG_E/W/I/D`) |
