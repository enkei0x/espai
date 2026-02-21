#include "AIProvider.h"
#include <cmath>
#include <memory>

#ifdef ARDUINO
#include "../http/HttpTransportESP32.h"
#endif

namespace ESPAI {

bool AIProvider::isRetryableStatus(int16_t statusCode) {
    return statusCode == 429 || (statusCode >= 500 && statusCode < 600);
}

uint32_t AIProvider::calculateRetryDelay(const RetryConfig& config, uint8_t attempt, int32_t retryAfterSeconds) {
    if (retryAfterSeconds > 0) {
        uint32_t retryAfterMs = static_cast<uint32_t>(retryAfterSeconds) * 1000;
        return (retryAfterMs < config.maxDelayMs) ? retryAfterMs : config.maxDelayMs;
    }

    float delay = static_cast<float>(config.initialDelayMs) * powf(config.backoffMultiplier, static_cast<float>(attempt));
    if (delay >= static_cast<float>(config.maxDelayMs)) {
        return config.maxDelayMs;
    }
    return static_cast<uint32_t>(delay);
}

Response AIProvider::chat(
    const std::vector<Message>& messages,
    const ChatOptions& options
) {
    if (!isConfigured()) {
        return Response::fail(ErrorCode::NotConfigured, "Provider not configured");
    }

#ifdef ARDUINO
    HttpTransport* transport = getDefaultTransport();
    if (transport == nullptr) {
        return Response::fail(ErrorCode::NotConfigured, "HTTP transport not available");
    }

    if (!transport->isReady()) {
        return Response::fail(ErrorCode::NetworkError, "Network not ready");
    }

    HttpRequest req = buildHttpRequest(messages, options);
    ESPAI_LOG_D(getName(), "Sending chat request to %s", req.url.c_str());

    uint16_t maxAttempts = (_retryConfig.enabled) ? static_cast<uint16_t>(_retryConfig.maxRetries) + 1 : 1;
    HttpResponse httpResp;

    for (uint16_t attempt = 0; attempt < maxAttempts; attempt++) {
        httpResp = transport->execute(req);

        if (httpResp.success) {
            break;
        }

        bool lastAttempt = (attempt + 1 >= maxAttempts);
        if (!_retryConfig.enabled || lastAttempt || !isRetryableStatus(httpResp.statusCode)) {
            break;
        }

        uint32_t delayMs = calculateRetryDelay(_retryConfig, attempt, httpResp.retryAfterSeconds);
        ESPAI_LOG_W(getName(), "Retry %d/%d after %lums (HTTP %d)",
                    attempt + 1, _retryConfig.maxRetries, (unsigned long)delayMs, httpResp.statusCode);
        delay(delayMs);

        if (!transport->isReady()) {
            return Response::fail(ErrorCode::NetworkError, "Network lost during retry");
        }
    }

    if (!httpResp.success) {
        if (httpResp.responseTooLarge) {
            return Response::fail(ErrorCode::ResponseTooLarge, httpResp.body, httpResp.statusCode);
        }
        return handleHttpError(httpResp.statusCode, httpResp.body);
    }

    Response response = parseResponse(httpResp.body);
    response.httpStatus = httpResp.statusCode;

    return response;
#else
    (void)messages;
    (void)options;
    return Response::fail(ErrorCode::NotConfigured, "HTTP client not available in native build");
#endif
}

#if ESPAI_ENABLE_STREAMING
bool AIProvider::chatStream(
    const std::vector<Message>& messages,
    const ChatOptions& options,
    StreamCallback callback
) {
    if (!isConfigured()) {
        return false;
    }

#ifdef ARDUINO
    HttpTransport* transport = getDefaultTransport();
    if (transport == nullptr || !transport->isReady()) {
        return false;
    }

    _streamingRequest = true;
    HttpRequest req = buildHttpRequest(messages, options);
    _streamingRequest = false;

    // Gemini uses URL endpoint for streaming, not "stream":true in body
    if (getSSEFormat() != SSEFormat::Gemini) {
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, req.body);
        if (error) {
            return false;
        }
        doc["stream"] = true;
        req.body = "";
        serializeJson(doc, req.body);
    }

    ESPAI_LOG_D(getName(), "Starting streaming chat to %s", req.url.c_str());

    uint16_t maxAttempts = (_retryConfig.enabled) ? static_cast<uint16_t>(_retryConfig.maxRetries) + 1 : 1;

    for (uint16_t attempt = 0; attempt < maxAttempts; attempt++) {
#if ESPAI_ENABLE_TOOLS
        _lastToolCalls.clear();
#endif
        SSEParser parser(getSSEFormat());
        parser.setTimeout(_timeout);
        parser.setAccumulateContent(false);
        parser.setContentCallback([&callback](const String& content, bool done) {
            callback(content, done);
        });

#if ESPAI_ENABLE_TOOLS
        parser.setToolCallCallback(
            [this](const String& id, const String& name, const String& arguments) {
                _lastToolCalls.push_back(ToolCall(id, name, arguments));
            });
#endif

        bool success = transport->executeStream(req, [&parser](const uint8_t* data, size_t len) -> bool {
            parser.feed(reinterpret_cast<const char*>(data), len);
            return !parser.isDone() && !parser.hasError();
        });

        if (success && !parser.hasError()) {
            return true;
        }

        bool lastAttempt = (attempt + 1 >= maxAttempts);
        if (!_retryConfig.enabled || lastAttempt) {
            break;
        }

        uint32_t delayMs = calculateRetryDelay(_retryConfig, attempt, -1);
        ESPAI_LOG_W(getName(), "Stream retry %d/%d after %lums",
                    attempt + 1, _retryConfig.maxRetries, (unsigned long)delayMs);
        delay(delayMs);

        if (!transport->isReady()) {
            return false;
        }
    }

    return false;
#else
    (void)messages;
    (void)options;
    (void)callback;
    return false;
#endif
}
#endif

#if ESPAI_ENABLE_ASYNC
ChatRequest* AIProvider::chatAsync(
    const std::vector<Message>& messages,
    const ChatOptions& options,
    AsyncChatCallback onComplete
) {
    if (_asyncRunner.isBusy()) return nullptr;

    auto msgsCopy = std::make_shared<std::vector<Message>>(messages);
    auto optsCopy = std::make_shared<ChatOptions>(options);

    bool launched = _asyncRunner.launch(
        [this, msgsCopy, optsCopy]() -> Response {
            return this->chat(*msgsCopy, *optsCopy);
        },
        onComplete
    );

    return launched ? _asyncRunner.getRequest() : nullptr;
}

ChatRequest* AIProvider::chatStreamAsync(
    const std::vector<Message>& messages,
    const ChatOptions& options,
    StreamCallback streamCb,
    AsyncDoneCallback onDone
) {
    if (_asyncRunner.isBusy()) return nullptr;

    auto msgsCopy = std::make_shared<std::vector<Message>>(messages);
    auto optsCopy = std::make_shared<ChatOptions>(options);

    bool launched = _asyncRunner.launchStream(
        [this, msgsCopy, optsCopy](StreamCallback cb) -> bool {
            return this->chatStream(*msgsCopy, *optsCopy, cb);
        },
        streamCb,
        onDone
    );

    return launched ? _asyncRunner.getRequest() : nullptr;
}

bool AIProvider::isAsyncBusy() const {
    return _asyncRunner.isBusy();
}

void AIProvider::cancelAsync() {
    _asyncRunner.cancel();
}
#endif // ESPAI_ENABLE_ASYNC

#if ESPAI_ENABLE_TOOLS
void AIProvider::addTool(const Tool& tool) {
    if (_tools.size() < ESPAI_MAX_TOOLS) {
        _tools.push_back(tool);
    }
}

void AIProvider::clearTools() {
    _tools.clear();
    _lastToolCalls.clear();
}
#endif

} // namespace ESPAI
