#ifndef ESPAI_OLLAMA_PROVIDER_H
#define ESPAI_OLLAMA_PROVIDER_H

#include "OpenAICompatibleProvider.h"
#include <memory>

#if ESPAI_PROVIDER_OLLAMA

namespace ESPAI {

class OllamaProvider : public OpenAICompatibleProvider {
public:
    OllamaProvider(const String& apiKey = "", const String& model = ESPAI_DEFAULT_MODEL_OLLAMA);

    const char* getName() const override { return "Ollama"; }
    Provider getType() const override { return Provider::Ollama; }
};

std::unique_ptr<AIProvider> createOllamaProvider(const String& apiKey, const String& model);

} // namespace ESPAI

#endif // ESPAI_PROVIDER_OLLAMA
#endif // ESPAI_OLLAMA_PROVIDER_H
