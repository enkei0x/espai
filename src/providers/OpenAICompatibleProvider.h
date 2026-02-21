// No compile guard — always available for custom OpenAI-compatible providers

#ifndef ESPAI_OPENAI_COMPATIBLE_PROVIDER_H
#define ESPAI_OPENAI_COMPATIBLE_PROVIDER_H

#include "AIProvider.h"
#include <memory>
#include <vector>

#if ESPAI_ENABLE_TOOLS
#include "../tools/ToolRegistry.h"
#endif

namespace ESPAI {

struct OpenAICompatibleConfig {
    String name;
    String baseUrl;
    String apiKey;
    String model;
    String authHeaderName;
    String authHeaderValuePrefix;
    bool requiresApiKey;
    bool toolCallingSupported;
    Provider providerType;

    OpenAICompatibleConfig()
        : name("Custom")
        , baseUrl()
        , apiKey()
        , model()
        , authHeaderName("Authorization")
        , authHeaderValuePrefix("Bearer ")
        , requiresApiKey(true)
        , toolCallingSupported(true)
        , providerType(Provider::Custom) {}
};

class OpenAICompatibleProvider : public AIProvider {
public:
    explicit OpenAICompatibleProvider(const OpenAICompatibleConfig& config);

    OpenAICompatibleProvider(
        const String& apiKey,
        const String& model,
        const String& baseUrl,
        const String& name,
        Provider providerType
    );

    const char* getName() const override { return _name.c_str(); }
    Provider getType() const override { return _config.providerType; }
    bool supportsTools() const override { return _config.toolCallingSupported; }

    void setApiKey(const String& key) override { AIProvider::setApiKey(key); _config.apiKey = key; }
    void setModel(const String& model) override { AIProvider::setModel(model); _config.model = model; }
    void setBaseUrl(const String& url) override { AIProvider::setBaseUrl(url); _config.baseUrl = url; }

    bool isConfigured() const override;
    void addCustomHeader(const String& name, const String& value);

#if ESPAI_ENABLE_TOOLS
    Message getAssistantMessageWithToolCalls(const String& content = "") const override;
#endif

    HttpRequest buildHttpRequest(
        const std::vector<Message>& messages,
        const ChatOptions& options
    ) override;

    String buildRequestBody(
        const std::vector<Message>& messages,
        const ChatOptions& options
    ) override;

    Response parseResponse(const String& json) override;

#if ESPAI_ENABLE_STREAMING
    SSEFormat getSSEFormat() const override { return SSEFormat::OpenAI; }
#endif

protected:
    OpenAICompatibleConfig _config;
    String _name; // getName() returns c_str() — needs stable storage
    std::vector<std::pair<String, String>> _customHeaders;

    virtual void addAuthHeaders(HttpRequest& req);
    virtual void addProviderHeaders(HttpRequest& req);
};

} // namespace ESPAI

#endif // ESPAI_OPENAI_COMPATIBLE_PROVIDER_H
