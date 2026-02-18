#ifndef ESPAI_HTTP_TRANSPORT_ESP32_H
#define ESPAI_HTTP_TRANSPORT_ESP32_H

#include "../core/AIConfig.h"

#ifdef ARDUINO

#include "HttpTransport.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

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
    WiFiClientSecure _wifiClient;
    String _lastError;
    const char* _caCert;
    bool _insecure;
    bool _reuseConnection;
    followRedirects_t _followRedirects;

    void configureSSL();
    bool setupHttpClient(HTTPClient& http, const HttpRequest& request);
    void addHeaders(HTTPClient& http, const HttpRequest& request);
    String httpErrorToString(int errorCode);
};

HttpTransportESP32* getESP32Transport();

} // namespace ESPAI

#endif // ARDUINO
#endif // ESPAI_HTTP_TRANSPORT_ESP32_H
