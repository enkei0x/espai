#ifndef ESPAI_GEMINI_PROVIDER_H
#define ESPAI_GEMINI_PROVIDER_H

#include "AIProvider.h"
#include <memory>

#if ESPAI_ENABLE_TOOLS
#include "../tools/ToolRegistry.h"
#endif

#if ESPAI_PROVIDER_GEMINI

namespace ESPAI {

class GeminiProvider : public AIProvider {
public:
    GeminiProvider(const String& apiKey, const String& model = ESPAI_DEFAULT_MODEL_GEMINI);

    const char* getName() const override { return "Gemini"; }
    Provider getType() const override { return Provider::Gemini; }
    bool supportsTools() const override { return true; }

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

    Response parseResponse(const String& responseBody) override;

#if ESPAI_ENABLE_STREAMING
    SSEFormat getSSEFormat() const override { return SSEFormat::Gemini; }
#endif

private:
    uint32_t _toolCallCounter = 0;
    String buildApiUrl(const String& model, const String& action) const;
    Response parseSingleResponse(const String& json);
};

std::unique_ptr<AIProvider> createGeminiProvider(const String& apiKey, const String& model);

} // namespace ESPAI

#endif // ESPAI_PROVIDER_GEMINI
#endif // ESPAI_GEMINI_PROVIDER_H
