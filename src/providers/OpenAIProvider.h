#ifndef ESPAI_OPENAI_PROVIDER_H
#define ESPAI_OPENAI_PROVIDER_H

#include "OpenAICompatibleProvider.h"
#include <memory>

#if ESPAI_PROVIDER_OPENAI

namespace ESPAI {

class OpenAIProvider : public OpenAICompatibleProvider {
public:
    OpenAIProvider(const String& apiKey, const String& model = ESPAI_DEFAULT_MODEL_OPENAI);

    const char* getName() const override { return "OpenAI"; }
    Provider getType() const override { return Provider::OpenAI; }
};

std::unique_ptr<AIProvider> createOpenAIProvider(const String& apiKey, const String& model);

} // namespace ESPAI

#endif // ESPAI_PROVIDER_OPENAI
#endif // ESPAI_OPENAI_PROVIDER_H
