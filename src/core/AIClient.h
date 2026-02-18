#ifndef ESPAI_AI_CLIENT_H
#define ESPAI_AI_CLIENT_H

#include "AIConfig.h"
#include "AITypes.h"
#include "../providers/AIProvider.h"
#include <memory>

namespace ESPAI {

class AIClient {
public:
    AIClient();
    AIClient(Provider provider, const String& apiKey);
    ~AIClient();

    bool setProvider(Provider provider, const String& apiKey);
    void setBaseUrl(const String& baseUrl);
    void setModel(const String& model);
    void setTimeout(uint32_t timeoutMs);

    Provider getProvider() const;
    const String& getModel() const;
    bool isConfigured() const;

    Response chat(const String& message);
    Response chat(const String& message, const ChatOptions& options);
    Response chat(const String& systemPrompt, const String& message);

#if ESPAI_ENABLE_STREAMING
    Response chatStream(const String& message, StreamCallback callback);
    Response chatStream(const String& message, const ChatOptions& options, StreamCallback callback);
#endif

    const String& getLastError() const;
    int16_t getLastHttpStatus() const;
    void reset();

    AIProvider* getProviderInstance() { return _providerInstance.get(); }

private:
    Provider _provider;
    String _apiKey;
    String _baseUrl;
    String _model;
    uint32_t _timeout;
    bool _configured;

    String _lastError;
    int16_t _lastHttpStatus;

    std::unique_ptr<AIProvider> _providerInstance;

    String _getDefaultModel() const;
    bool _validateConfig();
    bool _createProvider();

    AIClient(const AIClient&) = delete;
    AIClient& operator=(const AIClient&) = delete;
};

} // namespace ESPAI

#endif // ESPAI_AI_CLIENT_H
