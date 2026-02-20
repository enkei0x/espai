#include "SSEParser.h"

#if ESPAI_ENABLE_STREAMING

#ifdef ARDUINO
    #include <Arduino.h>
#else
    #include <chrono>
#endif

#include <ArduinoJson.h>

namespace ESPAI {

SSEParser::SSEParser()
    : _format(SSEFormat::OpenAI)
    , _timeoutMs(ESPAI_HTTP_TIMEOUT_MS)
    , _lastActivityMs(0)
    , _errorCode(ErrorCode::None)
    , _done(false)
    , _cancelled(false)
    , _hasError(false)
    , _accumulateContent(true) {
    updateActivity();
}

SSEParser::SSEParser(SSEFormat format)
    : _format(format)
    , _timeoutMs(ESPAI_HTTP_TIMEOUT_MS)
    , _lastActivityMs(0)
    , _errorCode(ErrorCode::None)
    , _done(false)
    , _cancelled(false)
    , _hasError(false)
    , _accumulateContent(true) {
    updateActivity();
}

void SSEParser::feed(const char* data, size_t len) {
    if (_done || _cancelled || _hasError) {
        return;
    }

    updateActivity();
#ifdef ARDUINO
    _buffer.concat(data, len);
#else
    _buffer.append(data, len);
#endif
    processBuffer();
}

void SSEParser::feed(const String& data) {
    feed(data.c_str(), data.length());
}

void SSEParser::reset() {
    _buffer = "";
    _accumulatedContent = "";
    _currentEventType = "";
    _errorMessage = "";
    _errorCode = ErrorCode::None;
    _done = false;
    _cancelled = false;
    _hasError = false;
    updateActivity();
}

void SSEParser::processBuffer() {
    int pos;
    while ((pos = _buffer.indexOf('\n')) != -1) {
        String line = _buffer.substring(0, pos);
        _buffer = _buffer.substring(pos + 1);

        if (!line.isEmpty() && line[line.length() - 1] == '\r') {
            line = line.substring(0, line.length() - 1);
        }

        processLine(line);

        if (_done || _cancelled || _hasError) {
            break;
        }
    }
}

void SSEParser::processLine(const String& line) {
    if (line.isEmpty()) {
        dispatchEvent();
        return;
    }

    int colonPos = line.indexOf(':');
    if (colonPos == -1) {
        return;
    }

    if (colonPos == 0) {
        return;
    }

    String field = line.substring(0, colonPos);
    String value = line.substring(colonPos + 1);

    while (!value.isEmpty() && value[0] == ' ') {
        value = value.substring(1);
    }

    if (field == "event") {
        _currentEventType = value;
    } else if (field == "data") {
        parseAndDispatchContent(value);
    }
}

void SSEParser::dispatchEvent() {
    _currentEventType = "";
}

void SSEParser::parseAndDispatchContent(const String& data) {
    SSEEvent event;
    event.eventType = _currentEventType;
    event.data = data;

    String content;
    bool done = false;

    if (_format == SSEFormat::OpenAI) {
        if (!parseOpenAIChunk(data, content, done)) {
            return;
        }
    } else {
        if (!parseAnthropicChunk(data, content, done)) {
            return;
        }
    }

    event.isDone = done;
    _done = done;

    if (_eventCallback) {
        _eventCallback(event);
    }

    if (!content.isEmpty()) {
        if (_accumulateContent) {
            _accumulatedContent += content;
        }
        if (_contentCallback) {
            _contentCallback(content, false);
        }
    }

    if (done && _contentCallback) {
        _contentCallback("", true);
    }
}

bool SSEParser::parseOpenAIChunk(const String& data, String& content, bool& done) {
    content = "";
    done = false;

    if (data.indexOf("[DONE]") != -1) {
        done = true;
        return true;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        return false;
    }

    if (doc["choices"].isNull()) {
        return false;
    }

    JsonArray choices = doc["choices"];
    if (choices.size() == 0) {
        return true;
    }

    JsonObject firstChoice = choices[0];
    if (firstChoice["delta"].isNull()) {
        return true;
    }

    JsonObject delta = firstChoice["delta"];
    if (!delta["content"].isNull()) {
        content = delta["content"].as<String>();
    }

    return true;
}

bool SSEParser::parseAnthropicChunk(const String& data, String& content, bool& done) {
    content = "";
    done = false;

    if (_currentEventType == "message_stop") {
        done = true;
        return true;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        if (_currentEventType == "message_stop" || _currentEventType == "message_start" ||
            _currentEventType == "content_block_start" || _currentEventType == "content_block_stop" ||
            _currentEventType == "ping") {
            return true;
        }
        return false;
    }

    const char* type = doc["type"] | "";

    if (strcmp(type, "message_stop") == 0) {
        done = true;
        return true;
    }

    if (strcmp(type, "content_block_delta") == 0) {
        JsonObject delta = doc["delta"];
        const char* deltaType = delta["type"] | "";

        if (strcmp(deltaType, "text_delta") == 0) {
            content = delta["text"].as<String>();
        }
    }

    return true;
}

bool SSEParser::checkTimeout() {
    if (_timeoutMs == 0) {
        return false;
    }

    uint32_t elapsed = millis() - _lastActivityMs;
    if (elapsed >= _timeoutMs) {
        setError(ErrorCode::Timeout, "Stream timeout");
        return true;
    }
    return false;
}

void SSEParser::updateActivity() {
    _lastActivityMs = millis();
}

void SSEParser::setError(ErrorCode code, const String& message) {
    _hasError = true;
    _errorCode = code;
    _errorMessage = message;

    if (_errorCallback) {
        _errorCallback(code, message);
    }
}

uint32_t SSEParser::millis() const {
#ifdef ARDUINO
    return ::millis();
#else
    static auto start = std::chrono::steady_clock::now();
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
#endif
}

} // namespace ESPAI

#endif // ESPAI_ENABLE_STREAMING
