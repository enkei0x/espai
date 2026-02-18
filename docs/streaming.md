# 🌊 Streaming Guide

Streaming lets you receive AI responses token-by-token in real-time, providing a better user experience and reducing perceived latency.

## Why Streaming?

| Without Streaming | With Streaming |
|-------------------|----------------|
| Wait for full response | See text appear instantly |
| Higher memory usage | Lower memory (process chunks) |
| All-or-nothing | Can stop early |
| Feels slow | Feels responsive |

---

## Basic Usage

Works the same for both OpenAI and Anthropic:

```cpp
#include <ESPAI.h>
using namespace ESPAI;

// Use any provider
OpenAIProvider ai("sk-...");
// or: AnthropicProvider ai("sk-ant-...");

void streamResponse() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Tell me a short story."));

    ChatOptions options;
    options.maxTokens = 500;

    bool success = ai.chatStream(messages, options,
        [](const String& chunk, bool done) {
            Serial.print(chunk);
            if (done) {
                Serial.println("\n--- Done! ---");
            }
        }
    );

    if (!success) {
        Serial.println("Streaming failed!");
    }
}
```

---

## Callback Function

The callback receives two parameters:

```cpp
void callback(const String& chunk, bool done) {
    // chunk - Text fragment (may be a word, part of word, or punctuation)
    // done  - true when stream is complete
}
```

### Using Lambda

```cpp
ai.chatStream(messages, options, [](const String& chunk, bool done) {
    Serial.print(chunk);
});
```

### Using Regular Function

```cpp
void onChunk(const String& chunk, bool done) {
    Serial.print(chunk);
}

ai.chatStream(messages, options, onChunk);
```

### Capturing Variables

```cpp
String accumulated;
int chunkCount = 0;

ai.chatStream(messages, options, [&](const String& chunk, bool done) {
    accumulated += chunk;
    chunkCount++;

    if (done) {
        Serial.printf("\nReceived %d chunks\n", chunkCount);
        Serial.printf("Full response: %s\n", accumulated.c_str());
    }
});
```

---

## 📊 Timing Statistics

Track performance with timing:

```cpp
unsigned long startTime = millis();
unsigned long firstChunkTime = 0;
int chunks = 0;

ai.chatStream(messages, options, [&](const String& chunk, bool done) {
    if (chunks == 0) {
        firstChunkTime = millis() - startTime;
    }
    chunks++;

    Serial.print(chunk);

    if (done) {
        unsigned long totalTime = millis() - startTime;
        Serial.printf("\n\nStats:\n");
        Serial.printf("  Time to first token: %lu ms\n", firstChunkTime);
        Serial.printf("  Total time: %lu ms\n", totalTime);
        Serial.printf("  Chunks received: %d\n", chunks);
    }
});
```

---

## 🖥️ Display Integration

### LCD Display

```cpp
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

String line = "";

ai.chatStream(messages, options, [&](const String& chunk, bool done) {
    line += chunk;

    // Word wrap at 16 chars
    if (line.length() > 16 || done) {
        lcd.clear();
        lcd.print(line.substring(0, 16));
        if (line.length() > 16) {
            lcd.setCursor(0, 1);
            lcd.print(line.substring(16, 32));
        }
    }
});
```

### OLED Display

```cpp
#include <Adafruit_SSD1306.h>
Adafruit_SSD1306 display(128, 64);

String buffer;

ai.chatStream(messages, options, [&](const String& chunk, bool done) {
    buffer += chunk;

    display.clearDisplay();
    display.setCursor(0, 0);
    display.print(buffer);
    display.display();
});
```

---

## ⚠️ Error Handling

```cpp
bool success = ai.chatStream(messages, options, callback);

if (!success) {
    Serial.println("Stream failed!");
    Serial.println("Possible causes:");
    Serial.println("- Network error");
    Serial.println("- Invalid API key");
    Serial.println("- Server error");
}
```

---

## 💾 Memory Considerations

Streaming uses less peak memory than buffered responses:

| Method | Memory Usage |
|--------|--------------|
| `chat()` | Stores full response in RAM |
| `chatStream()` | Only current chunk in RAM |

For long responses, streaming is recommended:

```cpp
// ❌ May run out of memory for long responses
Response resp = ai.chat(messages, opts);

// ✅ Memory-efficient
ai.chatStream(messages, opts, [](const String& chunk, bool done) {
    Serial.print(chunk);  // Process immediately, don't store
});
```

---

## 🔄 Streaming vs Regular

| Feature | `chat()` | `chatStream()` |
|---------|----------|----------------|
| Returns | `Response` object | `bool` (success) |
| Content access | `response.content` | Via callback |
| Token counts | Available | Not available |
| Tool calls | Supported | Limited support |
| Memory usage | Higher | Lower |
| UX | Wait for complete | Real-time |

### When to Use Each

**Use `chat()`:**
- Need token counts
- Using tool calling
- Processing response programmatically
- Short responses

**Use `chatStream()`:**
- User-facing output
- Long responses
- Memory constrained
- Want real-time feel
