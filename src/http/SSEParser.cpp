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
#if ESPAI_ENABLE_TOOLS
    , _currentToolCallIndex(-1)
#endif
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
#if ESPAI_ENABLE_TOOLS
    , _currentToolCallIndex(-1)
#endif
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
#if ESPAI_ENABLE_TOOLS
    _pendingToolCalls.clear();
    _currentToolCallIndex = -1;
#endif
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
    } else if (_format == SSEFormat::Gemini) {
        if (!parseGeminiChunk(data, content, done)) {
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
#if ESPAI_ENABLE_TOOLS
        finalizeToolCalls();
#endif
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

#if ESPAI_ENABLE_TOOLS
    if (!delta["tool_calls"].isNull()) {
        JsonArray toolCalls = delta["tool_calls"];
        for (JsonObject tc : toolCalls) {
            int index = tc["index"] | 0;
            if (index < 0 || index >= ESPAI_MAX_TOOLS) {
                continue;
            }

            while (static_cast<int>(_pendingToolCalls.size()) <= index) {
                _pendingToolCalls.push_back(PendingToolCall());
            }

            PendingToolCall& pending = _pendingToolCalls[index];

            if (!tc["id"].isNull()) {
                pending.id = tc["id"].as<String>();
            }
            if (!tc["function"].isNull()) {
                JsonObject func = tc["function"];
                if (!func["name"].isNull()) {
                    pending.name = func["name"].as<String>();
                }
                if (!func["arguments"].isNull()) {
                    pending.arguments += func["arguments"].as<String>();
                }
            }
        }
    }
#endif

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
        if (_currentEventType == "message_start" ||
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

#if ESPAI_ENABLE_TOOLS
    if (strcmp(type, "content_block_start") == 0) {
        JsonObject block = doc["content_block"];
        const char* blockType = block["type"] | "";
        if (strcmp(blockType, "tool_use") == 0) {
            if (static_cast<int>(_pendingToolCalls.size()) >= ESPAI_MAX_TOOLS) {
                return true;
            }
            PendingToolCall tc;
            tc.id = block["id"].as<String>();
            tc.name = block["name"].as<String>();
            _pendingToolCalls.push_back(tc);
            _currentToolCallIndex = static_cast<int16_t>(_pendingToolCalls.size() - 1);
        }
        return true;
    }
#endif

    if (strcmp(type, "content_block_delta") == 0) {
        JsonObject delta = doc["delta"];
        const char* deltaType = delta["type"] | "";

        if (strcmp(deltaType, "text_delta") == 0) {
            content = delta["text"].as<String>();
        }
#if ESPAI_ENABLE_TOOLS
        else if (strcmp(deltaType, "input_json_delta") == 0) {
            if (_currentToolCallIndex >= 0 &&
                _currentToolCallIndex < static_cast<int16_t>(_pendingToolCalls.size())) {
                _pendingToolCalls[_currentToolCallIndex].arguments +=
                    delta["partial_json"].as<String>();
            }
        }
#endif
    }

#if ESPAI_ENABLE_TOOLS
    if (strcmp(type, "content_block_stop") == 0) {
        if (_currentToolCallIndex >= 0 &&
            _currentToolCallIndex < static_cast<int16_t>(_pendingToolCalls.size())) {
            PendingToolCall& tc = _pendingToolCalls[_currentToolCallIndex];
            if (!tc.name.isEmpty() && _toolCallCallback) {
                _toolCallCallback(tc.id, tc.name, tc.arguments);
            }
            _currentToolCallIndex = -1;
        }
    }
#endif

    return true;
}

#if ESPAI_ENABLE_TOOLS
void SSEParser::finalizeToolCalls() {
    if (_toolCallCallback) {
        for (const auto& tc : _pendingToolCalls) {
            if (!tc.name.isEmpty()) {
                _toolCallCallback(tc.id, tc.name, tc.arguments);
            }
        }
    }
}
#endif

bool SSEParser::parseGeminiChunk(const String& data, String& content, bool& done) {
    content = "";
    done = false;

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, data);
    if (error) {
        return false;
    }

    if (!doc["error"].isNull()) {
        String errorMsg = doc["error"]["message"].as<String>();
        if (errorMsg.isEmpty()) {
            errorMsg = "API error";
        }
        setError(ErrorCode::ServerError, errorMsg);
        return false;
    }

    if (doc["candidates"].isNull()) {
        return false;
    }

    JsonArray candidates = doc["candidates"];
    if (candidates.size() == 0) {
        return true;
    }

    JsonObject firstCandidate = candidates[0];

    if (!firstCandidate["content"].isNull()) {
        JsonObject contentObj = firstCandidate["content"];
        if (!contentObj["parts"].isNull()) {
            JsonArray parts = contentObj["parts"];
            for (JsonObject part : parts) {
                if (part["thought"] | false) {
                    continue;
                }
                if (!part["text"].isNull()) {
                    content += part["text"].as<String>();
                }
#if ESPAI_ENABLE_TOOLS
                if (!part["functionCall"].isNull()) {
                    JsonObject fc = part["functionCall"];
                    PendingToolCall tc;
                    tc.name = fc["name"].as<String>();
                    String argsStr;
                    serializeJson(fc["args"], argsStr);
                    tc.arguments = argsStr;
                    tc.id = "gemini_tc_" + String(static_cast<int>(_pendingToolCalls.size()));
                    if (!tc.name.isEmpty()) {
                        _pendingToolCalls.push_back(tc);
                    }
                }
#endif
            }
        }
    }

    const char* finishReason = firstCandidate["finishReason"] | "";
    if (strlen(finishReason) > 0) {
#if ESPAI_ENABLE_TOOLS
        finalizeToolCalls();
#endif
        done = true;
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
