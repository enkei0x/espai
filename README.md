<p align="center">
  <img src="assets/logo-white.svg" alt="ESPAI Logo" width="300">
</p>

<p align="center">
  <strong>Unified AI API Client for ESP32</strong>
</p>

<p align="center">
  <a href="https://platformio.org/"><img src="https://img.shields.io/badge/PlatformIO-Compatible-orange?logo=platformio" alt="PlatformIO"></a>
  <a href="https://www.arduino.cc/"><img src="https://img.shields.io/badge/Arduino-Compatible-00979D?logo=arduino" alt="Arduino"></a>
  <a href="https://opensource.org/licenses/MIT"><img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License: MIT"></a>
  <a href="https://www.espressif.com/"><img src="https://img.shields.io/badge/ESP32-Supported-green?logo=espressif" alt="ESP32"></a>
  <img src="https://img.shields.io/badge/Tests-287%20passed-brightgreen" alt="Tests">
</p>

> 🚀 Bring the power of GPT and Claude to your ESP32 projects!

ESPAI is a lightweight, easy-to-use Arduino library that lets you integrate OpenAI and Anthropic APIs into your ESP32 projects. Build smart IoT devices, voice assistants, and AI-powered gadgets with just a few lines of code.

---

## ✨ Features

- 🎯 **Simple API** — Clean, intuitive interface for chat completions
- 🌊 **Streaming Support** — Real-time token-by-token responses
- 🛠️ **Tool Calling** — Function calling for agentic workflows
- 💬 **Conversation History** — Built-in context management
- 🔄 **Multiple Providers** — OpenAI and Anthropic (Claude) supported
- 📦 **Lightweight** — Minimal memory footprint, optimized for ESP32
- ⚡ **Non-blocking** — Async-friendly design

---

## 📦 Installation

### PlatformIO (Recommended)

Add to your `platformio.ini`:

```ini
lib_deps =
    espai
```

### Arduino IDE

1. Download the latest release from [GitHub Releases](https://github.com/enkei0x/espai/releases)
2. In Arduino IDE: **Sketch → Include Library → Add .ZIP Library**
3. Select the downloaded ZIP file

### Manual Installation

```bash
cd ~/Arduino/libraries  # or PlatformIO lib folder
git clone https://github.com/enkei0x/espai.git
```

---

## 🚀 Quick Start

```cpp
#include <WiFi.h>
#include <ESPAI.h>

using namespace ESPAI;

OpenAIProvider openai("sk-your-api-key");

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin("your-ssid", "your-password");
    while (WiFi.status() != WL_CONNECTED) delay(500);

    // Send a message
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello! What's 2+2?"));

    Response response = openai.chat(messages, ChatOptions());

    if (response.success) {
        Serial.println(response.content);
    }
}

void loop() {}
```

---

## 📖 Examples

| Example | Description |
|---------|-------------|
| [BasicChat](examples/BasicChat) | Simple request/response |
| [StreamingChat](examples/StreamingChat) | Real-time streaming output |
| [ConversationHistory](examples/ConversationHistory) | Multi-turn conversations |
| [ToolCalling](examples/ToolCalling) | Function calling workflow |
| [CustomOptions](examples/CustomOptions) | All configuration options |
| [ErrorHandling](examples/ErrorHandling) | Retry logic and error handling |

---

## 🌊 Streaming Responses

Get responses token-by-token for a better user experience:

```cpp
openai.chatStream(messages, options, [](const String& chunk, bool done) {
    Serial.print(chunk);  // Print each token as it arrives
    if (done) Serial.println("\n--- Done! ---");
});
```

---

## 🛠️ Tool Calling

Let the AI call functions in your code:

```cpp
// Define a tool
Tool tempTool;
tempTool.name = "get_temperature";
tempTool.description = "Get current temperature from sensor";
tempTool.parametersJson = R"({"type":"object","properties":{}})";

ai.addTool(tempTool);

// Send message - AI may request to call the tool
Response response = ai.chat(messages, options);

if (ai.hasToolCalls()) {
    // Add assistant message with tool calls to history
    messages.push_back(ai.getAssistantMessageWithToolCalls(response.content));

    for (const auto& call : ai.getLastToolCalls()) {
        // Execute your function
        String result = "{\"temperature\": 23.5}";
        messages.push_back(Message(Role::Tool, result, call.id));
    }
    // Get final response
    response = ai.chat(messages, options);
}
```

---

## 💬 Conversation History

Maintain context across multiple turns:

```cpp
Conversation conv;
conv.setSystemPrompt("You are a helpful IoT assistant.");
conv.setMaxMessages(20);  // Auto-prune old messages

// User asks something
conv.addUserMessage("Turn on the lights");
Response resp = openai.chat(conv.getMessages(), options);
conv.addAssistantMessage(resp.content);

// Follow-up question (context preserved)
conv.addUserMessage("Make them brighter");
resp = openai.chat(conv.getMessages(), options);
```

---

## ⚙️ Configuration

### ChatOptions

```cpp
ChatOptions options;
options.temperature = 0.7;       // Creativity (0.0 - 2.0)
options.maxTokens = 1024;        // Max response length
options.topP = 0.9;              // Nucleus sampling
options.frequencyPenalty = 0.0;  // Reduce repetition
options.presencePenalty = 0.0;   // Encourage new topics
options.model = "gpt-4o-mini";   // Model override
options.systemPrompt = "...";    // System instructions
```

### Provider Setup

```cpp
// OpenAI
OpenAIProvider openai("sk-...");
openai.setModel("gpt-4o");
openai.setTimeout(30000);

// Anthropic (Claude)
AnthropicProvider claude("sk-ant-...");
claude.setModel("claude-sonnet-4-20250514");
```

---

## 📊 Memory Usage

ESPAI is optimized for constrained environments:

| Component | RAM Usage |
|-----------|-----------|
| Provider instance | ~200 bytes |
| Per message | ~50 bytes + content |
| SSL connection | ~40 KB (one-time) |

💡 **Tip:** Use streaming for long responses to reduce peak memory usage.

To save ~10KB flash, disable unused providers:

```cpp
#define ESPAI_PROVIDER_ANTHROPIC 0  // Disable Anthropic
#include <ESPAI.h>
```

---

## 🔧 Troubleshooting

### Common Issues

**"Connection failed"**
- Check WiFi connection
- Verify API endpoint is reachable
- Ensure HTTPS/SSL is working

**"Authentication error"**
- Verify your API key is correct
- Check API key has proper permissions

**"Out of memory"**
- Reduce `maxTokens`
- Use streaming instead of buffered responses
- Clear conversation history periodically

**"Timeout"**
- Increase timeout: `provider.setTimeout(60000)`
- Check network stability

---

## 🗺️ Roadmap

- [x] OpenAI provider
- [x] Anthropic (Claude) provider
- [x] Streaming support
- [x] Tool/function calling
- [x] Conversation history management
- [ ] **Ollama** — Local LLM support (run models on your own server)
- [ ] **Gemini** — Google AI integration
- [ ] Vision support (image inputs)
- [ ] Embeddings API

Have a feature request? [Open an issue](https://github.com/enkei0x/espai/issues)!

---

## 🤝 Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing`)
5. Open a Pull Request

---

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## 🙏 Acknowledgments

- [ArduinoJson](https://arduinojson.org/) — JSON parsing
- [OpenAI](https://openai.com/) — GPT models
- [Anthropic](https://anthropic.com/) — Claude models

---

<p align="center">
  Made with ❤️ for the ESP32 community
</p>
