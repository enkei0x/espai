#ifndef ESPAI_SSE_PARSER_H
#define ESPAI_SSE_PARSER_H

#include "../core/AIConfig.h"
#include "../core/AITypes.h"
#include <functional>
#include <vector>

#if ESPAI_ENABLE_STREAMING

namespace ESPAI {

enum class SSEFormat : uint8_t {
    OpenAI = 0,
    Anthropic,
    Gemini
};

struct SSEEvent {
    String eventType;
    String data;
    bool isDone;

    SSEEvent() : isDone(false) {}
};

#if ESPAI_ENABLE_TOOLS
struct PendingToolCall {
    String id;
    String name;
    String arguments;

    PendingToolCall() : id(), name(), arguments() {}
    PendingToolCall(const String& i, const String& n) : id(i), name(n), arguments() {}
};
#endif

class SSEParser {
public:
    using EventCallback = std::function<void(const SSEEvent& event)>;
    using ContentCallback = std::function<void(const String& content, bool done)>;
    using ErrorCallback = std::function<void(ErrorCode code, const String& message)>;
#if ESPAI_ENABLE_TOOLS
    using ToolCallCallback = std::function<void(const String& id, const String& name, const String& arguments)>;
#endif

    SSEParser();
    explicit SSEParser(SSEFormat format);

    void setFormat(SSEFormat format) { _format = format; }
    SSEFormat getFormat() const { return _format; }

    void setEventCallback(EventCallback cb) { _eventCallback = cb; }
    void setContentCallback(ContentCallback cb) { _contentCallback = cb; }
    void setErrorCallback(ErrorCallback cb) { _errorCallback = cb; }
#if ESPAI_ENABLE_TOOLS
    void setToolCallCallback(ToolCallCallback cb) { _toolCallCallback = cb; }
    const std::vector<PendingToolCall>& getToolCalls() const { return _pendingToolCalls; }
#endif

    void setTimeout(uint32_t timeoutMs) { _timeoutMs = timeoutMs; }
    uint32_t getTimeout() const { return _timeoutMs; }

    void feed(const char* data, size_t len);
    void feed(const String& data);

    void reset();

    void cancel() { _cancelled = true; }
    bool isCancelled() const { return _cancelled; }

    bool isDone() const { return _done; }
    bool hasError() const { return _hasError; }
    ErrorCode getError() const { return _errorCode; }
    const String& getErrorMessage() const { return _errorMessage; }

    const String& getAccumulatedContent() const { return _accumulatedContent; }
    void clearAccumulatedContent() { _accumulatedContent = ""; }
    void setAccumulateContent(bool accumulate) { _accumulateContent = accumulate; }

    bool checkTimeout();
    void updateActivity();

private:
    SSEFormat _format;
    String _buffer;
    String _accumulatedContent;
    String _currentEventType;
    String _errorMessage;

    EventCallback _eventCallback;
    ContentCallback _contentCallback;
    ErrorCallback _errorCallback;
#if ESPAI_ENABLE_TOOLS
    ToolCallCallback _toolCallCallback;
    std::vector<PendingToolCall> _pendingToolCalls;
    int16_t _currentToolCallIndex;
#endif

    uint32_t _timeoutMs;
    uint32_t _lastActivityMs;

    ErrorCode _errorCode;

    bool _done;
    bool _cancelled;
    bool _hasError;
    bool _accumulateContent;

    void processBuffer();
    void processLine(const String& line);
    void dispatchEvent();
    void parseAndDispatchContent(const String& data);
    bool parseOpenAIChunk(const String& data, String& content, bool& done);
    bool parseAnthropicChunk(const String& data, String& content, bool& done);
    bool parseGeminiChunk(const String& data, String& content, bool& done);
    void setError(ErrorCode code, const String& message);
#if ESPAI_ENABLE_TOOLS
    void finalizeToolCalls();
#endif

    uint32_t millis() const;
};

} // namespace ESPAI

#endif // ESPAI_ENABLE_STREAMING
#endif // ESPAI_SSE_PARSER_H
