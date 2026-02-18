#ifndef ESPAI_OPENAI_PROVIDER_H
#define ESPAI_OPENAI_PROVIDER_H

#include "AIProvider.h"
#include <memory>

#if ESPAI_ENABLE_TOOLS
#include "../tools/ToolRegistry.h"
#endif

#if ESPAI_PROVIDER_OPENAI

namespace ESPAI {

class OpenAIProvider : public AIProvider {
public:
    OpenAIProvider(const String& apiKey, const String& model = ESPAI_DEFAULT_MODEL_OPENAI);

    const char* getName() const override { return "OpenAI"; }
    Provider getType() const override { return Provider::OpenAI; }
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

private:
#if ESPAI_ENABLE_TOOLS
    std::vector<Tool> _tools;
    std::vector<ToolCall> _lastToolCalls;
#endif
};

std::unique_ptr<AIProvider> createOpenAIProvider(const String& apiKey, const String& model);

} // namespace ESPAI

#endif // ESPAI_PROVIDER_OPENAI
#endif // ESPAI_OPENAI_PROVIDER_H
