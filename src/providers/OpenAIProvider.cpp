#include "OpenAIProvider.h"

#if ESPAI_PROVIDER_OPENAI

namespace ESPAI {

OpenAIProvider::OpenAIProvider(const String& apiKey, const String& model)
    : OpenAICompatibleProvider(
        apiKey,
        model.isEmpty() ? ESPAI_DEFAULT_MODEL_OPENAI : model,
        "https://api.openai.com/v1/chat/completions",
        "OpenAI",
        Provider::OpenAI
    )
{
}

std::unique_ptr<AIProvider> createOpenAIProvider(const String& apiKey, const String& model) {
    return std::unique_ptr<AIProvider>(new OpenAIProvider(apiKey, model));
}

} // namespace ESPAI

#endif // ESPAI_PROVIDER_OPENAI
