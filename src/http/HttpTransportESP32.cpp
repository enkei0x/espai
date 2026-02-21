#include "HttpTransportESP32.h"
#include "RootCACerts.h"

#ifdef ARDUINO

#include <WiFi.h>

namespace ESPAI {

HttpTransportESP32* getESP32Transport() {
    static HttpTransportESP32 instance;
    return &instance;
}

HttpTransport* getDefaultTransport() {
    return getESP32Transport();
}

HttpTransportESP32::HttpTransportESP32()
    : _caCert(nullptr)
    , _insecure(false)
    , _reuseConnection(true)
    , _followRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS)
{
#if ESPAI_ENABLE_ASYNC
    _transportMutex = xSemaphoreCreateMutex();
#endif
    configureSSL();
}

HttpTransportESP32::~HttpTransportESP32() {
#if ESPAI_ENABLE_ASYNC
    if (_transportMutex) {
        vSemaphoreDelete(_transportMutex);
        _transportMutex = nullptr;
    }
#endif
}

#if ESPAI_ENABLE_ASYNC
void HttpTransportESP32::lockTransport() {
    if (_transportMutex) xSemaphoreTake(_transportMutex, portMAX_DELAY);
}

void HttpTransportESP32::unlockTransport() {
    if (_transportMutex) xSemaphoreGive(_transportMutex);
}
#endif

void HttpTransportESP32::configureSSL() {
    if (_insecure) {
        _wifiClient.setInsecure();
    } else if (_caCert != nullptr) {
        _wifiClient.setCACert(_caCert);
    } else {
        _wifiClient.setCACert(ESPAI_CA_BUNDLE);
    }
}

void HttpTransportESP32::setCACert(const char* cert) {
    _caCert = cert;
    if (cert != nullptr) {
        _insecure = false;
        _wifiClient.setCACert(cert);
    }
}

void HttpTransportESP32::setInsecure(bool insecure) {
    _insecure = insecure;
    if (insecure) {
        ESPAI_LOG_W("HTTP", "SSL certificate validation disabled! This is insecure and vulnerable to MITM attacks.");
        _wifiClient.setInsecure();
    } else {
        configureSSL();
    }
}

bool HttpTransportESP32::isReady() const {
    return WiFi.status() == WL_CONNECTED;
}

bool HttpTransportESP32::isHttps(const String& url) {
    return url.startsWith("https://");
}

bool HttpTransportESP32::setupHttpClient(HTTPClient& http, const HttpRequest& request) {
    bool result;
    if (isHttps(request.url)) {
        configureSSL();
        result = http.begin(_wifiClient, request.url);
    } else {
        result = http.begin(_plainClient, request.url);
    }

    if (!result) {
        _lastError = "Failed to begin HTTP connection";
        ESPAI_LOG_E("HTTP", "Failed to begin connection to: %s", request.url.c_str());
        return false;
    }

    http.setTimeout(request.timeout);
    http.setConnectTimeout(request.timeout);
    http.setReuse(_reuseConnection);
    http.setFollowRedirects(_followRedirects);

    const char* headersToCollect[] = {"Retry-After"};
    http.collectHeaders(headersToCollect, 1);

    addHeaders(http, request);

    return true;
}

void HttpTransportESP32::addHeaders(HTTPClient& http, const HttpRequest& request) {
    if (!request.contentType.isEmpty()) {
        http.addHeader("Content-Type", request.contentType);
    }

    for (const auto& header : request.headers) {
        http.addHeader(header.first, header.second);
    }

    http.addHeader("User-Agent", "ESPAI/" ESPAI_VERSION_STRING);
}

String HttpTransportESP32::httpErrorToString(int errorCode) {
    switch (errorCode) {
        case HTTPC_ERROR_CONNECTION_REFUSED:
            return "Connection refused";
        case HTTPC_ERROR_SEND_HEADER_FAILED:
            return "Failed to send headers";
        case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
            return "Failed to send payload";
        case HTTPC_ERROR_NOT_CONNECTED:
            return "Not connected";
        case HTTPC_ERROR_CONNECTION_LOST:
            return "Connection lost";
        case HTTPC_ERROR_NO_STREAM:
            return "No stream";
        case HTTPC_ERROR_NO_HTTP_SERVER:
            return "No HTTP server";
        case HTTPC_ERROR_TOO_LESS_RAM:
            return "Out of memory";
        case HTTPC_ERROR_ENCODING:
            return "Encoding error";
        case HTTPC_ERROR_STREAM_WRITE:
            return "Stream write error";
        case HTTPC_ERROR_READ_TIMEOUT:
            return "Read timeout";
        default:
            return "HTTP error: " + String(errorCode);
    }
}

HttpResponse HttpTransportESP32::execute(const HttpRequest& request) {
#if ESPAI_ENABLE_ASYNC
    lockTransport();
#endif
    HttpResponse response;

    if (!isReady()) {
        _lastError = "WiFi not connected";
        response.statusCode = 0;
        response.body = _lastError;
        response.success = false;
        ESPAI_LOG_E("HTTP", "WiFi not connected");
    } else {
        HTTPClient http;

        if (!setupHttpClient(http, request)) {
            response.statusCode = 0;
            response.body = _lastError;
            response.success = false;
        } else {
            ESPAI_LOG_D("HTTP", "Executing %s to %s", request.method.c_str(), request.url.c_str());
            ESPAI_LOG_D("HTTP", "Body length: %d", request.body.length());

            int httpCode;
            if (request.method == "POST") {
                httpCode = http.POST(request.body);
            } else if (request.method == "GET") {
                httpCode = http.GET();
            } else if (request.method == "PUT") {
                httpCode = http.PUT(request.body);
            } else if (request.method == "DELETE") {
                httpCode = http.sendRequest("DELETE", request.body);
            } else {
                httpCode = http.sendRequest(request.method.c_str(), request.body);
            }

            response.statusCode = httpCode;

            if (httpCode > 0) {
                int contentLength = http.getSize();
                if (contentLength > 0 && static_cast<uint32_t>(contentLength) > request.maxResponseSize) {
                    _lastError = "Response too large: " + String(contentLength) + " bytes (max " + String(request.maxResponseSize) + ")";
                    response.responseTooLarge = true;
                    response.body = _lastError;
                    response.success = false;
                    ESPAI_LOG_W("HTTP", "%s", _lastError.c_str());
                } else {
                    response.body = http.getString();

                    // Post-read size check (catches chunked responses where Content-Length is unknown)
                    if (response.body.length() > request.maxResponseSize) {
                        _lastError = "Response too large: " + String(response.body.length()) + " bytes (max " + String(request.maxResponseSize) + ")";
                        response.responseTooLarge = true;
                        response.body = _lastError;
                        response.success = false;
                        ESPAI_LOG_W("HTTP", "%s", _lastError.c_str());
                    } else {
                        response.success = (httpCode >= 200 && httpCode < 300);

                        if (httpCode == 429 || httpCode >= 500) {
                            String retryAfter = http.header("Retry-After");
                            if (retryAfter.length() > 0) {
                                response.retryAfterSeconds = atoi(retryAfter.c_str());
                            }
                        }

                        ESPAI_LOG_D("HTTP", "Response code: %d, body length: %d", httpCode, response.body.length());

                        if (!response.success) {
                            _lastError = "HTTP " + String(httpCode) + ": " + response.body;
                            ESPAI_LOG_W("HTTP", "Request failed with code %d", httpCode);
                        }
                    }
                }
            } else {
                _lastError = httpErrorToString(httpCode);
                response.body = _lastError;
                response.success = false;
                ESPAI_LOG_E("HTTP", "Request failed: %s", _lastError.c_str());
            }

            http.end();
        }
    }

#if ESPAI_ENABLE_ASYNC
    unlockTransport();
#endif
    return response;
}

bool HttpTransportESP32::executeStream(const HttpRequest& request, StreamDataCallback callback) {
#if ESPAI_ENABLE_ASYNC
    lockTransport();
#endif
    bool success = false;

    if (!isReady()) {
        _lastError = "WiFi not connected";
        ESPAI_LOG_E("HTTP", "WiFi not connected");
    } else {
        HTTPClient http;

        if (!setupHttpClient(http, request)) {
        } else {
            http.addHeader("Accept", "text/event-stream");

            ESPAI_LOG_D("HTTP", "Starting stream to %s", request.url.c_str());

            int httpCode = http.POST(request.body);

            if (httpCode != HTTP_CODE_OK) {
                if (httpCode > 0) {
                    _lastError = "HTTP " + String(httpCode);
                    ESPAI_LOG_W("HTTP", "Stream request failed with code %d", httpCode);
                } else {
                    _lastError = httpErrorToString(httpCode);
                    ESPAI_LOG_E("HTTP", "Stream request failed: %s", _lastError.c_str());
                }
            } else {
                WiFiClient* stream = http.getStreamPtr();
                if (stream == nullptr) {
                    _lastError = "Failed to get stream";
                    ESPAI_LOG_E("HTTP", "Failed to get stream pointer");
                } else {
                    success = true;
                    uint8_t buffer[256];
                    uint32_t startTime = millis();
                    uint32_t timeout = request.timeout;

                    while (http.connected() || stream->available()) {
                        if (millis() - startTime > timeout) {
                            _lastError = "Stream timeout";
                            ESPAI_LOG_W("HTTP", "Stream read timeout");
                            success = false;
                            break;
                        }

                        size_t available = stream->available();
                        if (available > 0) {
                            size_t toRead = (available < sizeof(buffer)) ? available : sizeof(buffer);
                            size_t bytesRead = stream->readBytes(buffer, toRead);

                            if (bytesRead > 0) {
                                startTime = millis();

                                if (!callback(buffer, bytesRead)) {
                                    ESPAI_LOG_D("HTTP", "Stream stopped by callback");
                                    break;
                                }
                            }
                        } else {
                            delay(1);
                        }

                        yield();
                    }
                }
            }

            http.end();
            ESPAI_LOG_D("HTTP", "Stream ended, success=%d", success);
        }
    }

#if ESPAI_ENABLE_ASYNC
    unlockTransport();
#endif
    return success;
}

} // namespace ESPAI

#endif // ARDUINO
