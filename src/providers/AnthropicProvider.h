#ifndef ESPAI_ANTHROPIC_PROVIDER_H
#define ESPAI_ANTHROPIC_PROVIDER_H

#include "AIProvider.h"
#include <memory>

#if ESPAI_PROVIDER_ANTHROPIC

namespace ESPAI {

class AnthropicProvider : public AIProvider {
public:
    AnthropicProvider(const String& apiKey, const String& model = ESPAI_DEFAULT_MODEL_ANTHROPIC);

    const char* getName() const override { return "Anthropic"; }
    Provider getType() const override { return Provider::Anthropic; }
    bool supportsTools() const override { return true; }

#if ESPAI_ENABLE_TOOLS
    Message getAssistantMessageWithToolCalls(const String& content = "") const override;
#endif

    void setApiVersion(const String& version) { _apiVersion = version; }
    const String& getApiVersion() const { return _apiVersion; }

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
    SSEFormat getSSEFormat() const override { return SSEFormat::Anthropic; }
#endif

    String extractSystemPrompt(const std::vector<Message>& messages);

private:
    String _apiVersion;
};

std::unique_ptr<AIProvider> createAnthropicProvider(const String& apiKey, const String& model);

} // namespace ESPAI

#endif // ESPAI_PROVIDER_ANTHROPIC
#endif // ESPAI_ANTHROPIC_PROVIDER_H
