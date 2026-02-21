#include "OllamaProvider.h"

#if ESPAI_PROVIDER_OLLAMA

namespace ESPAI {

OllamaProvider::OllamaProvider(const String& apiKey, const String& model)
    : OpenAICompatibleProvider(
        apiKey,
        model.isEmpty() ? ESPAI_DEFAULT_MODEL_OLLAMA : model,
        "http://localhost:11434/v1/chat/completions",
        "Ollama",
        Provider::Ollama
    )
{
    _config.requiresApiKey = false;
    _config.authHeaderName = "";
}

std::unique_ptr<AIProvider> createOllamaProvider(const String& apiKey, const String& model) {
    return std::unique_ptr<AIProvider>(new OllamaProvider(apiKey, model));
}

} // namespace ESPAI

#endif // ESPAI_PROVIDER_OLLAMA
