#include "AIProvider.h"

#ifdef ARDUINO
#include "../http/HttpTransportESP32.h"
#endif

namespace ESPAI {

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

    HttpResponse httpResp = transport->execute(req);

    if (!httpResp.success) {
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

    HttpRequest req = buildHttpRequest(messages, options);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, req.body);
    if (error) {
        return false;
    }
    doc["stream"] = true;
    req.body = "";
    serializeJson(doc, req.body);

    ESPAI_LOG_D(getName(), "Starting streaming chat to %s", req.url.c_str());

    SSEParser parser(getSSEFormat());
    parser.setTimeout(_timeout);
    parser.setAccumulateContent(false);
    parser.setContentCallback([&callback](const String& content, bool done) {
        callback(content, done);
    });

    bool success = transport->executeStream(req, [&parser](const uint8_t* data, size_t len) -> bool {
        parser.feed(reinterpret_cast<const char*>(data), len);
        return !parser.isDone() && !parser.hasError();
    });

    return success && !parser.hasError();
#else
    (void)messages;
    (void)options;
    (void)callback;
    return false;
#endif
}
#endif

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
