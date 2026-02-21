#ifndef ESPAI_AI_TYPES_H
#define ESPAI_AI_TYPES_H

#ifdef ARDUINO
    #include <Arduino.h>
    #include <functional>
#else
    #include <string>
    #include <functional>
    #include <cstdint>
    #include <cstring>

    #ifndef NATIVE_STRING_DEFINED
    #define NATIVE_STRING_DEFINED
    class String : public std::string {
    public:
        String() : std::string() {}
        String(const char* s) : std::string(s ? s : "") {}
        String(const std::string& s) : std::string(s) {}
        String(int val) : std::string(std::to_string(val)) {}
        String(unsigned int val) : std::string(std::to_string(val)) {}
        String(long val) : std::string(std::to_string(val)) {}
        String(unsigned long val) : std::string(std::to_string(val)) {}
        String(float val) : std::string(std::to_string(val)) {}
        String(double val) : std::string(std::to_string(val)) {}

        bool isEmpty() const { return empty(); }
        const char* c_str() const { return std::string::c_str(); }

        int indexOf(char ch) const {
            size_t pos = find(ch);
            return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
        }

        int indexOf(char ch, unsigned int fromIndex) const {
            size_t pos = find(ch, fromIndex);
            return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
        }

        int indexOf(const String& str) const {
            size_t pos = find(str);
            return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
        }

        int indexOf(const char* str) const {
            size_t pos = find(str);
            return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
        }

        int indexOf(const String& str, unsigned int fromIndex) const {
            size_t pos = find(str, fromIndex);
            return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
        }

        int indexOf(const char* str, unsigned int fromIndex) const {
            size_t pos = find(str, fromIndex);
            return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
        }

        String substring(unsigned int beginIndex) const {
            if (beginIndex >= length()) return String("");
            return String(substr(beginIndex));
        }

        String substring(unsigned int beginIndex, unsigned int endIndex) const {
            if (beginIndex >= length()) return String("");
            if (endIndex > length()) endIndex = length();
            if (beginIndex >= endIndex) return String("");
            return String(substr(beginIndex, endIndex - beginIndex));
        }

        String& operator=(const char* s) {
            std::string::operator=(s ? s : "");
            return *this;
        }

        String operator+(const String& other) const {
            return String(static_cast<const std::string&>(*this) + static_cast<const std::string&>(other));
        }

        String operator+(const char* other) const {
            return String(static_cast<const std::string&>(*this) + (other ? other : ""));
        }

        friend String operator+(const char* lhs, const String& rhs) {
            return String(std::string(lhs ? lhs : "") + static_cast<const std::string&>(rhs));
        }

        bool operator==(const char* other) const {
            return std::string::compare(other ? other : "") == 0;
        }

        bool operator!=(const char* other) const {
            return !(*this == other);
        }

        size_t write(uint8_t c) {
            push_back(static_cast<char>(c));
            return 1;
        }

        size_t write(const uint8_t* s, size_t n) {
            append(reinterpret_cast<const char*>(s), n);
            return n;
        }
    };
    #endif
#endif

#include "AIConfig.h"

namespace ESPAI {

enum class Provider : uint8_t {
    OpenAI = 0,
    Anthropic,
    Gemini,
    Ollama,
    Custom
};

enum class Role : uint8_t {
    System = 0,
    User,
    Assistant,
    Tool
};

enum class ErrorCode : uint8_t {
    None = 0,
    NetworkError,
    Timeout,
    AuthError,
    RateLimited,
    InvalidRequest,
    ServerError,
    ParseError,
    OutOfMemory,
    ProviderNotSupported,
    NotConfigured,
    StreamingError,
    ResponseTooLarge
};

struct Message {
    Role role;
    String content;
    String name;
    String toolCallsJson;  // JSON array of tool_calls for assistant messages

    Message() : role(Role::User), content(), name(), toolCallsJson() {}
    Message(Role r, const String& c) : role(r), content(c), name(), toolCallsJson() {}
    Message(Role r, const String& c, const String& n) : role(r), content(c), name(n), toolCallsJson() {}

    bool hasToolCalls() const { return !toolCallsJson.isEmpty(); }
};

struct Response {
    bool success;
    String content;
    ErrorCode error;
    String errorMessage;
    int16_t httpStatus;
    uint32_t promptTokens;
    uint32_t completionTokens;

    Response()
        : success(false)
        , content()
        , error(ErrorCode::None)
        , errorMessage()
        , httpStatus(0)
        , promptTokens(0)
        , completionTokens(0) {}

    uint32_t totalTokens() const {
        return promptTokens + completionTokens;
    }

    static Response ok(const String& content) {
        Response r;
        r.success = true;
        r.content = content;
        return r;
    }

    static Response fail(ErrorCode code, const String& message, int16_t status = 0) {
        Response r;
        r.success = false;
        r.error = code;
        r.errorMessage = message;
        r.httpStatus = status;
        return r;
    }
};

struct ChatOptions {
    float temperature;
    int16_t maxTokens;
    String model;
    String systemPrompt;
    float topP;
    float frequencyPenalty;
    float presencePenalty;
    int32_t thinkingBudget; // Gemini 2.5+: thinking token budget. -1 = provider default, 0 = disable thinking.

    ChatOptions()
        : temperature(0.7f)
        , maxTokens(1024)
        , model()
        , systemPrompt()
        , topP(1.0f)
        , frequencyPenalty(0.0f)
        , presencePenalty(0.0f)
        , thinkingBudget(-1) {}
};

struct RetryConfig {
    bool enabled;
    uint8_t maxRetries;
    uint32_t initialDelayMs;
    float backoffMultiplier;
    uint32_t maxDelayMs;

    RetryConfig()
        : enabled(false)
        , maxRetries(3)
        , initialDelayMs(1000)
        , backoffMultiplier(2.0f)
        , maxDelayMs(30000) {}
};

#if ESPAI_ENABLE_STREAMING
using StreamCallback = std::function<void(const String& chunk, bool done)>;
#endif

inline const char* providerToString(Provider provider) {
    switch (provider) {
        case Provider::OpenAI:    return "OpenAI";
        case Provider::Anthropic: return "Anthropic";
        case Provider::Gemini:    return "Gemini";
        case Provider::Ollama:    return "Ollama";
        case Provider::Custom:    return "Custom";
        default:                  return "Unknown";
    }
}

inline const char* roleToString(Role role) {
    switch (role) {
        case Role::System:    return "system";
        case Role::User:      return "user";
        case Role::Assistant: return "assistant";
        case Role::Tool:      return "tool";
        default:              return "unknown";
    }
}

inline const char* errorCodeToString(ErrorCode code) {
    switch (code) {
        case ErrorCode::None:                return "None";
        case ErrorCode::NetworkError:        return "NetworkError";
        case ErrorCode::Timeout:             return "Timeout";
        case ErrorCode::AuthError:           return "AuthError";
        case ErrorCode::RateLimited:         return "RateLimited";
        case ErrorCode::InvalidRequest:      return "InvalidRequest";
        case ErrorCode::ServerError:         return "ServerError";
        case ErrorCode::ParseError:          return "ParseError";
        case ErrorCode::OutOfMemory:         return "OutOfMemory";
        case ErrorCode::ProviderNotSupported:return "ProviderNotSupported";
        case ErrorCode::NotConfigured:       return "NotConfigured";
        case ErrorCode::StreamingError:      return "StreamingError";
        case ErrorCode::ResponseTooLarge:    return "ResponseTooLarge";
        default:                             return "Unknown";
    }
}

inline double roundFloat(float val) {
    return static_cast<double>(static_cast<int>(val * 100.0f + 0.5f)) / 100.0;
}

#if ESPAI_ENABLE_TOOLS
using ToolHandler = std::function<String(const String& args)>;

/**
 * Unified tool definition for all providers.
 *
 * All providers use the same JSON schema format for parameters.
 * Each provider maps parametersJson to its own field name:
 * - OpenAI: "parameters" in function definition
 * - Anthropic: "input_schema" in tool definition
 * - Gemini: "parameters" in functionDeclarations
 */
struct Tool {
    String name;
    String description;
    String parametersJson;  // JSON schema (same format for all providers)
    ToolHandler handler;    // Optional: for local tool execution via ToolRegistry

    Tool() : name(), description(), parametersJson(), handler(nullptr) {}
    Tool(const String& n, const String& d, const String& p)
        : name(n), description(d), parametersJson(p), handler(nullptr) {}
    Tool(const String& n, const String& d, const String& p, ToolHandler h)
        : name(n), description(d), parametersJson(p), handler(h) {}
};

/**
 * Represents a tool call from the AI response.
 *
 * - OpenAI: maps to tool_calls[].function
 * - Anthropic: maps to tool_use content block
 * - Gemini: maps to functionCall part (id is synthesized as gemini_tc_N)
 */
struct ToolCall {
    String id;
    String name;
    String arguments;

    ToolCall() : id(), name(), arguments() {}
    ToolCall(const String& i, const String& n, const String& a)
        : id(i), name(n), arguments(a) {}
};

/**
 * Result of executing a tool call.
 */
struct ToolResult {
    String toolCallId;
    String toolName;
    String result;
    bool success;

    ToolResult() : toolCallId(), toolName(), result(), success(false) {}
    ToolResult(const String& id, const String& name, const String& res, bool ok = true)
        : toolCallId(id), toolName(name), result(res), success(ok) {}
};
#endif // ESPAI_ENABLE_TOOLS

} // namespace ESPAI

#endif // ESPAI_AI_TYPES_H
