#ifndef ESPAI_AI_CONFIG_H
#define ESPAI_AI_CONFIG_H

#ifndef ESPAI_ENABLE_STREAMING
#define ESPAI_ENABLE_STREAMING      1
#endif

#ifndef ESPAI_ENABLE_TOOLS
#define ESPAI_ENABLE_TOOLS          1
#endif

#ifndef ESPAI_PROVIDER_OPENAI
#define ESPAI_PROVIDER_OPENAI       1
#endif

#ifndef ESPAI_PROVIDER_ANTHROPIC
#define ESPAI_PROVIDER_ANTHROPIC    1
#endif

// Planned providers - not yet implemented
#ifndef ESPAI_PROVIDER_GEMINI
#define ESPAI_PROVIDER_GEMINI       0
#endif

#ifndef ESPAI_PROVIDER_OLLAMA
#define ESPAI_PROVIDER_OLLAMA       0
#endif

#ifndef ESPAI_MAX_MESSAGES
#define ESPAI_MAX_MESSAGES          20
#endif

#ifndef ESPAI_HTTP_TIMEOUT_MS
#define ESPAI_HTTP_TIMEOUT_MS       30000
#endif

#ifndef ESPAI_MAX_TOOLS
#define ESPAI_MAX_TOOLS             10
#endif

#ifndef ESPAI_DEFAULT_MODEL_OPENAI
#define ESPAI_DEFAULT_MODEL_OPENAI      "gpt-4.1-mini"
#endif

#ifndef ESPAI_DEFAULT_MODEL_ANTHROPIC
#define ESPAI_DEFAULT_MODEL_ANTHROPIC   "claude-sonnet-4-20250514"
#endif

#ifndef ESPAI_DEFAULT_MODEL_GEMINI
#define ESPAI_DEFAULT_MODEL_GEMINI      "gemini-1.5-flash"
#endif

#ifndef ESPAI_DEFAULT_MODEL_OLLAMA
#define ESPAI_DEFAULT_MODEL_OLLAMA      "llama3.2"
#endif

#ifndef ESPAI_DEBUG
#define ESPAI_DEBUG                 0
#endif

#if ESPAI_DEBUG
    #define ESPAI_LOG_E(tag, fmt, ...) Serial.printf("[E][%s] " fmt "\n", tag, ##__VA_ARGS__)
    #define ESPAI_LOG_W(tag, fmt, ...) Serial.printf("[W][%s] " fmt "\n", tag, ##__VA_ARGS__)
    #define ESPAI_LOG_I(tag, fmt, ...) Serial.printf("[I][%s] " fmt "\n", tag, ##__VA_ARGS__)
    #define ESPAI_LOG_D(tag, fmt, ...) Serial.printf("[D][%s] " fmt "\n", tag, ##__VA_ARGS__)
#else
    #define ESPAI_LOG_E(tag, fmt, ...)
    #define ESPAI_LOG_W(tag, fmt, ...)
    #define ESPAI_LOG_I(tag, fmt, ...)
    #define ESPAI_LOG_D(tag, fmt, ...)
#endif

#define ESPAI_VERSION_MAJOR     0
#define ESPAI_VERSION_MINOR     7
#define ESPAI_VERSION_PATCH     0
#define ESPAI_VERSION_STRING    "0.7.0"

#endif // ESPAI_AI_CONFIG_H
