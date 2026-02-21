# Async API (FreeRTOS)

ESPAI supports non-blocking async chat and streaming requests using FreeRTOS tasks. This lets your main loop remain responsive while AI requests run in the background.

---

## Requirements

- ESP32 with FreeRTOS (default Arduino framework)
- `ESPAI_ENABLE_ASYNC` must be `1` (enabled by default on Arduino)

---

## Quick Start

### Using AIClient (Recommended)

```cpp
#include <ESPAI.h>
using namespace ESPAI;

AIClient client(Provider::OpenAI, "sk-...");

// Callback-based: fire and forget
client.chatAsync("Hello!", [](const Response& resp) {
    Serial.println(resp.content);
});
```

### Using Providers Directly

```cpp
OpenAIProvider ai("sk-...");

std::vector<Message> msgs;
msgs.push_back(Message(Role::User, "Hello!"));

ChatRequest* req = ai.chatAsync(msgs, ChatOptions(), [](const Response& resp) {
    Serial.println(resp.content);
});
```

---

## Patterns

### Callback-Based

The simplest approach. The callback runs when the request completes:

```cpp
client.chatAsync("What is 2+2?", [](const Response& resp) {
    if (resp.success) {
        Serial.println(resp.content);
    } else {
        Serial.println("Error: " + resp.errorMessage);
    }
});

// Main loop continues immediately
```

### Poll-Based

For more control, poll the `ChatRequest` handle:

```cpp
ChatRequest* req = client.chatAsync("What is 2+2?");

// Do other work while waiting
while (!req->isComplete()) {
    req->poll();  // Check status & invoke callback if done
    // ... do sensor readings, handle buttons, etc.
    delay(10);
}

Response result = req->getResult();
Serial.println(result.content);
```

### Async Streaming

Stream tokens in the background:

```cpp
client.chatStreamAsync(
    "Tell me a story",
    // Stream callback - each chunk as it arrives
    [](const String& chunk, bool done) {
        Serial.print(chunk);
    },
    // Done callback - when streaming is fully complete
    [](const Response& resp) {
        Serial.println("\nDone!");
    }
);
```

### Cancellation

Cancel a running async request:

```cpp
ChatRequest* req = client.chatAsync("Long question...");

// Changed our mind
delay(1000);
client.cancelAsync();

// Or cancel via the request handle:
// req->cancel();
```

---

## ChatRequest

The `ChatRequest` handle provides status and result access:

| Method | Description |
|--------|-------------|
| `getStatus()` | Returns `AsyncStatus` (Idle, Running, Completed, Cancelled, Error) |
| `isComplete()` | `true` when finished (success, error, or cancelled) |
| `isCancelled()` | `true` if cancelled |
| `getResult()` | Get the `Response` (valid after completion) |
| `cancel()` | Request cancellation |
| `poll()` | Check status and invoke callback if complete |

---

## Configuration

Override defaults before `#include <ESPAI.h>`:

```cpp
#define ESPAI_ENABLE_ASYNC      1       // Enable/disable async (default: 1 on Arduino)
#define ESPAI_ASYNC_STACK_SIZE  20480   // FreeRTOS task stack size in bytes
#define ESPAI_ASYNC_TASK_PRIORITY 3     // FreeRTOS task priority (1-24)
```

### Stack Size

If you get stack overflow crashes during async requests, increase the stack size:

```cpp
#define ESPAI_ASYNC_STACK_SIZE 32768  // 32KB stack
```

### Thread Safety

The HTTP transport layer uses mutexes to protect shared state. Only one async request can run at a time per provider — calling `chatAsync()` while another is running will return `nullptr`.

---

## AIClient vs Provider Async

| Feature | AIClient | AIProvider |
|---------|----------|------------|
| Input | Simple strings | `vector<Message>` + `ChatOptions` |
| Setup | `AIClient(Provider, key)` | Create provider directly |
| Best for | Simple use cases | Full control |

### AIClient Methods

```cpp
ChatRequest* chatAsync(const String& message, AsyncChatCallback onComplete = nullptr);
ChatRequest* chatAsync(const String& message, const ChatOptions& options, AsyncChatCallback onComplete = nullptr);
ChatRequest* chatStreamAsync(const String& message, StreamCallback streamCb, AsyncDoneCallback onDone = nullptr);
ChatRequest* chatStreamAsync(const String& message, const ChatOptions& options, StreamCallback streamCb, AsyncDoneCallback onDone = nullptr);
bool isAsyncBusy() const;
void cancelAsync();
```

### AIProvider Methods

```cpp
ChatRequest* chatAsync(const vector<Message>& messages, const ChatOptions& options, AsyncChatCallback onComplete = nullptr);
ChatRequest* chatStreamAsync(const vector<Message>& messages, const ChatOptions& options, StreamCallback streamCb, AsyncDoneCallback onDone = nullptr);
bool isAsyncBusy() const;
void cancelAsync();
```

---

## Disabling Async

If you don't need async and want to save memory:

```cpp
#define ESPAI_ENABLE_ASYNC 0
#include <ESPAI.h>
```

This removes all FreeRTOS task management code and the `AsyncTaskRunner` class.
