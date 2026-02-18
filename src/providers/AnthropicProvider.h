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

    Response chat(
        const std::vector<Message>& messages,
        const ChatOptions& options
    ) override;

#if ESPAI_ENABLE_STREAMING
    bool chatStream(
        const std::vector<Message>& messages,
        const ChatOptions& options,
        StreamCallback callback
    ) override;
#endif

#if ESPAI_ENABLE_TOOLS
    void addTool(const Tool& tool);
    void clearTools();
    const std::vector<ToolCall>& getLastToolCalls() const { return _lastToolCalls; }
    bool hasToolCalls() const { return !_lastToolCalls.empty(); }
    Message getAssistantMessageWithToolCalls(const String& content = "") const;
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
    bool parseStreamChunk(const String& chunk, String& content, bool& done);
#endif

    String extractSystemPrompt(const std::vector<Message>& messages);

    void setApiVersion(const String& version) { _apiVersion = version; }
    const String& getApiVersion() const { return _apiVersion; }

private:
    String _apiVersion;

#if ESPAI_ENABLE_TOOLS
    std::vector<Tool> _tools;
    std::vector<ToolCall> _lastToolCalls;
#endif
};

std::unique_ptr<AIProvider> createAnthropicProvider(const String& apiKey, const String& model);

} // namespace ESPAI

#endif // ESPAI_PROVIDER_ANTHROPIC
#endif // ESPAI_ANTHROPIC_PROVIDER_H
