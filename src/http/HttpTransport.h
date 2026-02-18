#ifndef ESPAI_HTTP_TRANSPORT_H
#define ESPAI_HTTP_TRANSPORT_H

#include "../core/AIConfig.h"
#include "../core/AITypes.h"
#include "../providers/AIProvider.h"
#include <functional>

namespace ESPAI {

using StreamDataCallback = std::function<bool(const uint8_t* data, size_t len)>;

class HttpTransport {
public:
    virtual ~HttpTransport() = default;

    virtual HttpResponse execute(const HttpRequest& request) = 0;
    virtual bool executeStream(const HttpRequest& request, StreamDataCallback callback) = 0;
    virtual bool isReady() const = 0;
    virtual const String& getLastError() const = 0;
    virtual void setCACert(const char* cert) = 0;
    virtual void setInsecure(bool insecure) = 0;
};

HttpTransport* getDefaultTransport();

} // namespace ESPAI

#endif // ESPAI_HTTP_TRANSPORT_H
