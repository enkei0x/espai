#ifndef ESPAI_PROVIDER_FACTORY_H
#define ESPAI_PROVIDER_FACTORY_H

#include "AIProvider.h"
#include <memory>

namespace ESPAI {

using ProviderCreator = std::unique_ptr<AIProvider>(*)(const String& apiKey, const String& model);

class ProviderFactory {
public:
    static std::unique_ptr<AIProvider> create(
        Provider type,
        const String& apiKey,
        const String& model = ""
    );

    static std::unique_ptr<AIProvider> create(
        Provider type,
        const String& apiKey,
        const String& model,
        const String& baseUrl
    );

    static void registerProvider(Provider type, ProviderCreator creator);
    static void unregisterProvider(Provider type);
    static bool isRegistered(Provider type);

    static const char* getDefaultModel(Provider type);
    static const char* getDefaultBaseUrl(Provider type);

private:
    static constexpr size_t MAX_PROVIDERS = 5;
    static ProviderCreator _creators[MAX_PROVIDERS];
    static bool _initialized;

    static void ensureInitialized();
    static size_t providerIndex(Provider type);
};

} // namespace ESPAI

#endif // ESPAI_PROVIDER_FACTORY_H
