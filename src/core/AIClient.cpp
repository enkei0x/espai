#include "AIClient.h"
#include "../providers/ProviderFactory.h"

namespace ESPAI {

AIClient::AIClient()
    : _provider(Provider::OpenAI)
    , _apiKey()
    , _baseUrl()
    , _model()
    , _timeout(ESPAI_HTTP_TIMEOUT_MS)
    , _configured(false)
    , _lastError()
    , _lastHttpStatus(0)
    , _providerInstance(nullptr) {
}

AIClient::AIClient(Provider provider, const String& apiKey)
    : _provider(provider)
    , _apiKey(apiKey)
    , _baseUrl()
    , _model()
    , _timeout(ESPAI_HTTP_TIMEOUT_MS)
    , _configured(false)
    , _lastError()
    , _lastHttpStatus(0)
    , _providerInstance(nullptr) {
    _configured = _validateConfig();
    if (_configured) {
        _createProvider();
    }
}

AIClient::~AIClient() {
}

bool AIClient::setProvider(Provider provider, const String& apiKey) {
    _provider = provider;
    _apiKey = apiKey;
    _baseUrl = "";
    _model = "";
    _providerInstance.reset();
    _configured = _validateConfig();
    if (_configured) {
        _createProvider();
    }
    return _configured;
}

void AIClient::setBaseUrl(const String& baseUrl) {
    _baseUrl = baseUrl;
    if (_providerInstance) {
        _providerInstance->setBaseUrl(baseUrl);
    }
}

void AIClient::setModel(const String& model) {
    _model = model;
    if (_providerInstance) {
        _providerInstance->setModel(model);
    }
}

void AIClient::setTimeout(uint32_t timeoutMs) {
    _timeout = timeoutMs;
    if (_providerInstance) {
        _providerInstance->setTimeout(timeoutMs);
    }
}

Provider AIClient::getProvider() const {
    return _provider;
}

const String& AIClient::getModel() const {
    if (_providerInstance) {
        return _providerInstance->getModel();
    }
    if (_model.isEmpty()) {
        static String defaultModel;
        defaultModel = _getDefaultModel();
        return defaultModel;
    }
    return _model;
}

bool AIClient::isConfigured() const {
    return _configured && _providerInstance != nullptr;
}

Response AIClient::chat(const String& message) {
    ChatOptions options;
    return chat(message, options);
}

Response AIClient::chat(const String& message, const ChatOptions& options) {
    if (!_configured || !_providerInstance) {
        _lastError = "Client not configured. Call setProvider() first.";
        return Response::fail(ErrorCode::NotConfigured, _lastError);
    }

    if (message.isEmpty()) {
        _lastError = "Message cannot be empty";
        return Response::fail(ErrorCode::InvalidRequest, _lastError);
    }

    std::vector<Message> messages;
    if (!options.systemPrompt.isEmpty()) {
        messages.push_back(Message(Role::System, options.systemPrompt));
    }
    messages.push_back(Message(Role::User, message));

    Response response = _providerInstance->chat(messages, options);
    _lastHttpStatus = response.httpStatus;
    if (!response.success) {
        _lastError = response.errorMessage;
    }
    return response;
}

Response AIClient::chat(const String& systemPrompt, const String& message) {
    ChatOptions options;
    options.systemPrompt = systemPrompt;
    return chat(message, options);
}

#if ESPAI_ENABLE_STREAMING
Response AIClient::chatStream(const String& message, StreamCallback callback) {
    ChatOptions options;
    return chatStream(message, options, callback);
}

Response AIClient::chatStream(const String& message, const ChatOptions& options, StreamCallback callback) {
    if (!_configured || !_providerInstance) {
        _lastError = "Client not configured. Call setProvider() first.";
        return Response::fail(ErrorCode::NotConfigured, _lastError);
    }

    if (message.isEmpty()) {
        _lastError = "Message cannot be empty";
        return Response::fail(ErrorCode::InvalidRequest, _lastError);
    }

    if (!callback) {
        _lastError = "Callback cannot be null for streaming";
        return Response::fail(ErrorCode::InvalidRequest, _lastError);
    }

    std::vector<Message> messages;
    if (!options.systemPrompt.isEmpty()) {
        messages.push_back(Message(Role::System, options.systemPrompt));
    }
    messages.push_back(Message(Role::User, message));

    bool success = _providerInstance->chatStream(messages, options, callback);
    if (success) {
        return Response::ok("");
    }
    _lastError = "Streaming failed";
    return Response::fail(ErrorCode::StreamingError, _lastError);
}
#endif

#if ESPAI_ENABLE_ASYNC
ChatRequest* AIClient::chatAsync(const String& message, AsyncChatCallback onComplete) {
    ChatOptions options;
    return chatAsync(message, options, onComplete);
}

ChatRequest* AIClient::chatAsync(const String& message, const ChatOptions& options, AsyncChatCallback onComplete) {
    if (!_configured || !_providerInstance) return nullptr;
    if (message.isEmpty()) return nullptr;

    std::vector<Message> messages;
    if (!options.systemPrompt.isEmpty()) {
        messages.push_back(Message(Role::System, options.systemPrompt));
    }
    messages.push_back(Message(Role::User, message));

    return _providerInstance->chatAsync(messages, options, onComplete);
}

ChatRequest* AIClient::chatStreamAsync(const String& message, StreamCallback streamCb, AsyncDoneCallback onDone) {
    ChatOptions options;
    return chatStreamAsync(message, options, streamCb, onDone);
}

ChatRequest* AIClient::chatStreamAsync(const String& message, const ChatOptions& options, StreamCallback streamCb, AsyncDoneCallback onDone) {
    if (!_configured || !_providerInstance) return nullptr;
    if (message.isEmpty()) return nullptr;

    std::vector<Message> messages;
    if (!options.systemPrompt.isEmpty()) {
        messages.push_back(Message(Role::System, options.systemPrompt));
    }
    messages.push_back(Message(Role::User, message));

    return _providerInstance->chatStreamAsync(messages, options, streamCb, onDone);
}

bool AIClient::isAsyncBusy() const {
    if (!_providerInstance) return false;
    return _providerInstance->isAsyncBusy();
}

void AIClient::cancelAsync() {
    if (_providerInstance) {
        _providerInstance->cancelAsync();
    }
}
#endif // ESPAI_ENABLE_ASYNC

const String& AIClient::getLastError() const {
    return _lastError;
}

int16_t AIClient::getLastHttpStatus() const {
    return _lastHttpStatus;
}

void AIClient::reset() {
    _lastError = "";
    _lastHttpStatus = 0;
}

String AIClient::_getDefaultModel() const {
    switch (_provider) {
        case Provider::OpenAI:    return ESPAI_DEFAULT_MODEL_OPENAI;
        case Provider::Anthropic: return ESPAI_DEFAULT_MODEL_ANTHROPIC;
        case Provider::Gemini:    return ESPAI_DEFAULT_MODEL_GEMINI;
        case Provider::Ollama:    return ESPAI_DEFAULT_MODEL_OLLAMA;
        case Provider::Custom:    return "gpt-4.1-mini";
        default:                  return "";
    }
}

bool AIClient::_validateConfig() {
    if (_provider == Provider::Ollama) {
        return true;
    }

    if (_provider == Provider::Custom) {
        return !_baseUrl.isEmpty();
    }

    if (_apiKey.isEmpty()) {
        _lastError = "API key is required";
        return false;
    }

    return true;
}

bool AIClient::_createProvider() {
    _providerInstance = ProviderFactory::create(_provider, _apiKey, _model);
    if (!_providerInstance) {
        _lastError = "Failed to create provider";
        _configured = false;
        return false;
    }

    if (!_baseUrl.isEmpty()) {
        _providerInstance->setBaseUrl(_baseUrl);
    }
    _providerInstance->setTimeout(_timeout);

    return true;
}

} // namespace ESPAI
