#include "ProviderFactory.h"

#if ESPAI_PROVIDER_OPENAI
#include "OpenAIProvider.h"
#endif

#if ESPAI_PROVIDER_ANTHROPIC
#include "AnthropicProvider.h"
#endif

#if ESPAI_PROVIDER_GEMINI
#include "GeminiProvider.h"
#endif

namespace ESPAI {

ProviderCreator ProviderFactory::_creators[MAX_PROVIDERS] = {nullptr};
bool ProviderFactory::_initialized = false;

void ProviderFactory::ensureInitialized() {
    if (!_initialized) {
        for (size_t i = 0; i < MAX_PROVIDERS; ++i) {
            _creators[i] = nullptr;
        }
        _initialized = true;

        // Auto-register built-in providers
#if ESPAI_PROVIDER_OPENAI
        _creators[providerIndex(Provider::OpenAI)] = createOpenAIProvider;
#endif
#if ESPAI_PROVIDER_ANTHROPIC
        _creators[providerIndex(Provider::Anthropic)] = createAnthropicProvider;
#endif
#if ESPAI_PROVIDER_GEMINI
        _creators[providerIndex(Provider::Gemini)] = createGeminiProvider;
#endif
    }
}

size_t ProviderFactory::providerIndex(Provider type) {
    return static_cast<size_t>(type);
}

std::unique_ptr<AIProvider> ProviderFactory::create(
    Provider type,
    const String& apiKey,
    const String& model
) {
    ensureInitialized();

    size_t idx = providerIndex(type);
    if (idx >= MAX_PROVIDERS || _creators[idx] == nullptr) {
        return nullptr;
    }

    String actualModel = model;
    if (actualModel.isEmpty()) {
        actualModel = getDefaultModel(type);
    }

    auto provider = _creators[idx](apiKey, actualModel);
    if (provider) {
        provider->setBaseUrl(getDefaultBaseUrl(type));
    }
    return provider;
}

std::unique_ptr<AIProvider> ProviderFactory::create(
    Provider type,
    const String& apiKey,
    const String& model,
    const String& baseUrl
) {
    auto provider = create(type, apiKey, model);
    if (provider && !baseUrl.isEmpty()) {
        provider->setBaseUrl(baseUrl);
    }
    return provider;
}

void ProviderFactory::registerProvider(Provider type, ProviderCreator creator) {
    ensureInitialized();
    size_t idx = providerIndex(type);
    if (idx < MAX_PROVIDERS) {
        _creators[idx] = creator;
    }
}

void ProviderFactory::unregisterProvider(Provider type) {
    ensureInitialized();
    size_t idx = providerIndex(type);
    if (idx < MAX_PROVIDERS) {
        _creators[idx] = nullptr;
    }
}

bool ProviderFactory::isRegistered(Provider type) {
    ensureInitialized();
    size_t idx = providerIndex(type);
    return idx < MAX_PROVIDERS && _creators[idx] != nullptr;
}

const char* ProviderFactory::getDefaultModel(Provider type) {
    switch (type) {
        case Provider::OpenAI:    return ESPAI_DEFAULT_MODEL_OPENAI;
        case Provider::Anthropic: return ESPAI_DEFAULT_MODEL_ANTHROPIC;
        case Provider::Gemini:    return ESPAI_DEFAULT_MODEL_GEMINI;
        case Provider::Ollama:    return ESPAI_DEFAULT_MODEL_OLLAMA;
        default:                  return "";
    }
}

const char* ProviderFactory::getDefaultBaseUrl(Provider type) {
    switch (type) {
        case Provider::OpenAI:
            return "https://api.openai.com/v1/chat/completions";
        case Provider::Anthropic:
            return "https://api.anthropic.com/v1/messages";
        case Provider::Gemini:
            return "https://generativelanguage.googleapis.com/v1beta";
        case Provider::Ollama:
            return "http://localhost:11434/api/chat";
        default:
            return "";
    }
}

} // namespace ESPAI
