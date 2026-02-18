#ifndef ESPAI_AI_PROVIDER_H
#define ESPAI_AI_PROVIDER_H

#include "../core/AIConfig.h"
#include "../core/AITypes.h"
#include <ArduinoJson.h>
#include <vector>

namespace ESPAI {

struct HttpRequest {
    String url;
    String method;
    String body;
    String contentType;
    std::vector<std::pair<String, String>> headers;
    uint32_t timeout;

    HttpRequest()
        : method("POST")
        , contentType("application/json")
        , timeout(ESPAI_HTTP_TIMEOUT_MS) {}
};

struct HttpResponse {
    int16_t statusCode;
    String body;
    bool success;

    HttpResponse() : statusCode(0), success(false) {}
};

class AIProvider {
public:
    virtual ~AIProvider() = default;

    virtual Response chat(
        const std::vector<Message>& messages,
        const ChatOptions& options
    ) = 0;

#if ESPAI_ENABLE_STREAMING
    virtual bool chatStream(
        const std::vector<Message>& messages,
        const ChatOptions& options,
        StreamCallback callback
    ) = 0;
#endif

    virtual const char* getName() const = 0;
    virtual Provider getType() const = 0;

    virtual bool supportsStreaming() const { return true; }
    virtual bool supportsTools() const { return false; }

    virtual void setApiKey(const String& key) { _apiKey = key; }
    virtual void setModel(const String& model) { _model = model; }
    virtual void setBaseUrl(const String& url) { _baseUrl = url; }
    virtual void setTimeout(uint32_t timeoutMs) { _timeout = timeoutMs; }

    const String& getApiKey() const { return _apiKey; }
    const String& getModel() const { return _model; }
    const String& getBaseUrl() const { return _baseUrl; }
    uint32_t getTimeout() const { return _timeout; }

    virtual bool isConfigured() const {
        return _apiKey.length() > 0 && _model.length() > 0;
    }

protected:
    String _apiKey;
    String _model;
    String _baseUrl;
    uint32_t _timeout = ESPAI_HTTP_TIMEOUT_MS;

    virtual String buildRequestBody(
        const std::vector<Message>& messages,
        const ChatOptions& options
    ) = 0;

    virtual Response parseResponse(const String& json) = 0;

    virtual HttpRequest buildHttpRequest(
        const std::vector<Message>& messages,
        const ChatOptions& options
    ) {
        HttpRequest req;
        req.url = _baseUrl;
        req.body = buildRequestBody(messages, options);
        req.timeout = _timeout;
        return req;
    }

    void addHeader(HttpRequest& req, const String& name, const String& value) {
        req.headers.push_back({name, value});
    }

    void addAuthHeader(HttpRequest& req, const String& prefix = "Bearer ") {
        addHeader(req, "Authorization", prefix + _apiKey);
    }

    Response handleHttpError(int16_t statusCode, const String& body) {
        ErrorCode code = ErrorCode::ServerError;

        if (statusCode == 401 || statusCode == 403) {
            code = ErrorCode::AuthError;
        } else if (statusCode == 429) {
            code = ErrorCode::RateLimited;
        } else if (statusCode >= 400 && statusCode < 500) {
            code = ErrorCode::InvalidRequest;
        } else if (statusCode == 0) {
            code = ErrorCode::NetworkError;
        }

        return Response::fail(code, body, statusCode);
    }
};

} // namespace ESPAI

#endif // ESPAI_AI_PROVIDER_H
