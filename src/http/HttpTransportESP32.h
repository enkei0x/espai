#ifndef ESPAI_HTTP_TRANSPORT_ESP32_H
#define ESPAI_HTTP_TRANSPORT_ESP32_H

#include "../core/AIConfig.h"

#ifdef ARDUINO

#if defined(CONFIG_IDF_TARGET_ESP32H2)
#error "ESPAI requires WiFi. ESP32-H2 does not have WiFi hardware."
#endif

#include "HttpTransport.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#if ESPAI_ENABLE_ASYNC
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#endif

namespace ESPAI {

class HttpTransportESP32 : public HttpTransport {
public:
    HttpTransportESP32();
    ~HttpTransportESP32() override;

    HttpResponse execute(const HttpRequest& request) override;
    bool executeStream(const HttpRequest& request, StreamDataCallback callback) override;
    bool isReady() const override;
    const String& getLastError() const override { return _lastError; }
    void setCACert(const char* cert) override;
    void setInsecure(bool insecure) override;

    void setReuse(bool reuse) { _reuseConnection = reuse; }
    void setFollowRedirects(followRedirects_t follow) { _followRedirects = follow; }

private:
    WiFiClient _plainClient;
    WiFiClientSecure _wifiClient;
    String _lastError;
    const char* _caCert;
    bool _insecure;
    bool _reuseConnection;
    followRedirects_t _followRedirects;

#if ESPAI_ENABLE_ASYNC
    SemaphoreHandle_t _transportMutex = nullptr;
    void lockTransport();
    void unlockTransport();
#endif

    static bool isHttps(const String& url);
    void configureSSL();
    bool setupHttpClient(HTTPClient& http, const HttpRequest& request);
    void addHeaders(HTTPClient& http, const HttpRequest& request);
    String httpErrorToString(int errorCode);
};

HttpTransportESP32* getESP32Transport();

} // namespace ESPAI

#endif // ARDUINO
#endif // ESPAI_HTTP_TRANSPORT_ESP32_H
