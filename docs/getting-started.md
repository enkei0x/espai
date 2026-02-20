# 🚀 Getting Started

Welcome to ESPAI! This guide will help you get up and running in minutes.

## Prerequisites

- ESP32 board (any variant: ESP32, ESP32-S3, ESP32-C3)
- Arduino IDE or PlatformIO
- WiFi network
- API key from [OpenAI](https://platform.openai.com/api-keys) or [Anthropic](https://console.anthropic.com/)

## Installation

### PlatformIO (Recommended)

Add to your `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
lib_deps =
    enkei0x/ESPAI@^0.7.0
```

### Arduino IDE

1. Download the latest release from [GitHub](https://github.com/enkei0x/espai/releases)
2. Go to **Sketch → Include Library → Add .ZIP Library**
3. Select the downloaded file

## Your First Sketch

```cpp
#include <WiFi.h>
#include <ESPAI.h>

using namespace ESPAI;

// Replace with your credentials
const char* WIFI_SSID = "your-wifi";
const char* WIFI_PASS = "your-password";
const char* API_KEY = "sk-your-openai-key";

OpenAIProvider ai(API_KEY);

void setup() {
    Serial.begin(115200);

    // Connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nConnected!");

    // Send a message
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello! Say hi in 5 words or less."));

    Response response = ai.chat(messages, ChatOptions());

    if (response.success) {
        Serial.println(response.content);
    } else {
        Serial.println("Error: " + response.errorMessage);
    }
}

void loop() {
    delay(10000);
}
```

## 🔑 Managing API Keys

**Never hardcode API keys in your code!** Use a separate `secrets.h` file:

```cpp
// secrets.h (add to .gitignore!)
#ifndef SECRETS_H
#define SECRETS_H

const char* WIFI_SSID = "your-wifi";
const char* WIFI_PASS = "your-password";
const char* OPENAI_API_KEY = "sk-...";
const char* ANTHROPIC_API_KEY = "sk-ant-...";

#endif
```

Then include it:
```cpp
#include "secrets.h"
```

## 💾 Reducing Flash Size

To save ~10KB flash, disable unused providers before including ESPAI:

```cpp
#define ESPAI_PROVIDER_ANTHROPIC 0  // Disable Anthropic (~10KB)
// or
#define ESPAI_PROVIDER_OPENAI 0     // Disable OpenAI (~10KB)

#include <ESPAI.h>
```

## 🎯 Next Steps

- [API Reference](api-reference.md) - All classes and methods
- [Providers Guide](providers.md) - OpenAI vs Anthropic
- [Streaming](streaming.md) - Real-time responses
- [Tool Calling](tool-calling.md) - Let AI call your functions
