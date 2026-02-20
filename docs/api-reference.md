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
    Gemini,    // Planned
    Ollama     // Planned
};
```

### ErrorCode

Error types returned in responses.

```cpp
enum class ErrorCode {
    None,              // No error
    NetworkError,      // Connection failed
    Timeout,           // Request timed out
    AuthError,         // Invalid API key
    RateLimited,       // Too many requests
    ServerError,       // API server error
    ParseError,        // Invalid JSON response
    InvalidRequest,    // Bad request parameters
    NotConfigured,     // Provider not set up
    ProviderNotSupported
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
    int httpStatus;
    int promptTokens;
    int completionTokens;

    // Methods
    int totalTokens() const;
    static Response fail(ErrorCode code, const String& message);
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
    float temperature = 0.7f;      // Creativity (0.0 - 2.0)
    int maxTokens = 0;             // Max response length (0 = default)
    float topP = 1.0f;             // Nucleus sampling
    float frequencyPenalty = 0.0f; // Reduce repetition
    float presencePenalty = 0.0f;  // Encourage new topics
    String model;                  // Override default model
    String systemPrompt;           // System instructions
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
    std::vector<std::pair<String, String>> headers;
    int timeout;
};
```

---

## OpenAIProvider

Provider for OpenAI GPT models.

### Constructor

```cpp
OpenAIProvider(const String& apiKey, const String& model = "gpt-4o-mini");
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
| `addMessage(message)` | Add any message |
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

The same `parametersJson` schema works for both OpenAI and Anthropic:
- OpenAI maps it to the `parameters` field
- Anthropic maps it to the `input_schema` field

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

Low-level HTTP transport for ESP32. Most users won't need to interact with this directly.

### SSL/TLS Configuration

ESPAI validates SSL certificates by default using built-in root CA certificates that cover api.openai.com and api.anthropic.com.

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

- **Default behavior:** SSL certificate validation is ENABLED
- **Built-in certificates:** DigiCert Global Root CA and ISRG Root X1 (valid until 2031+)
- **setInsecure(true):** Logs a warning and disables validation - use only for testing
- **Custom endpoints:** Use `setCACert()` with appropriate certificates
