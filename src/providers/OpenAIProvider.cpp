#include "OpenAIProvider.h"
#include <ArduinoJson.h>

#ifdef ARDUINO
#include "../http/HttpTransportESP32.h"
#endif

#if ESPAI_PROVIDER_OPENAI

namespace ESPAI {

OpenAIProvider::OpenAIProvider(const String& apiKey, const String& model) {
    _apiKey = apiKey;
    _model = model.isEmpty() ? ESPAI_DEFAULT_MODEL_OPENAI : model;
    _baseUrl = "https://api.openai.com/v1/chat/completions";
}

HttpRequest OpenAIProvider::buildHttpRequest(
    const std::vector<Message>& messages,
    const ChatOptions& options
) {
    HttpRequest req;
    req.url = _baseUrl;
    req.method = "POST";
    req.body = buildRequestBody(messages, options);
    req.timeout = _timeout;
    addAuthHeader(req, "Bearer ");
    return req;
}

String OpenAIProvider::buildRequestBody(
    const std::vector<Message>& messages,
    const ChatOptions& options
) {
    JsonDocument doc;

    String model = options.model.isEmpty() ? _model : options.model;
    doc["model"] = model.c_str();

    JsonArray messagesArr = doc["messages"].to<JsonArray>();
    for (const auto& msg : messages) {
        JsonObject m = messagesArr.add<JsonObject>();
        m["role"] = roleToString(msg.role);

#if ESPAI_ENABLE_TOOLS
        if (msg.role == Role::Tool) {
            m["tool_call_id"] = msg.name.c_str();
        }
        // For assistant messages with tool_calls, include the tool_calls array
        if (msg.role == Role::Assistant && msg.hasToolCalls()) {
            JsonDocument toolCallsDoc;
            deserializeJson(toolCallsDoc, msg.toolCallsJson);
            m["tool_calls"] = toolCallsDoc.as<JsonArray>();
            // Content can be null when tool_calls are present
            if (msg.content.isEmpty()) {
                m["content"] = nullptr;
            } else {
                m["content"] = msg.content.c_str();
            }
        } else {
            m["content"] = msg.content.c_str();
        }
#else
        m["content"] = msg.content.c_str();
#endif
    }

    doc["temperature"] = roundFloat(options.temperature);

    if (options.maxTokens > 0) {
        doc["max_tokens"] = static_cast<int>(options.maxTokens);
    }

    if (options.topP < 1.0f) {
        doc["top_p"] = roundFloat(options.topP);
    }

    if (options.frequencyPenalty != 0.0f) {
        doc["frequency_penalty"] = roundFloat(options.frequencyPenalty);
    }

    if (options.presencePenalty != 0.0f) {
        doc["presence_penalty"] = roundFloat(options.presencePenalty);
    }

#if ESPAI_ENABLE_TOOLS
    if (!_tools.empty()) {
        JsonArray toolsArr = doc["tools"].to<JsonArray>();
        for (const auto& tool : _tools) {
            JsonObject t = toolsArr.add<JsonObject>();
            t["type"] = "function";
            JsonObject func = t["function"].to<JsonObject>();
            func["name"] = tool.name.c_str();
            if (!tool.description.isEmpty()) {
                func["description"] = tool.description.c_str();
            }
            if (!tool.parametersJson.isEmpty()) {
                JsonDocument paramsDoc;
                deserializeJson(paramsDoc, tool.parametersJson);
                func["parameters"] = paramsDoc.as<JsonObject>();
            }
        }
    }
#endif

    String output;
    serializeJson(doc, output);
    return output;
}

Response OpenAIProvider::parseResponse(const String& json) {
    Response response;

#if ESPAI_ENABLE_TOOLS
    _lastToolCalls.clear();
#endif

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
        return Response::fail(ErrorCode::ParseError, error.c_str());
    }

    if (!doc["error"].isNull()) {
        String errorMsg = doc["error"]["message"].as<String>();
        if (errorMsg.isEmpty()) {
            errorMsg = "API error";
        }
        return Response::fail(ErrorCode::ServerError, errorMsg);
    }

    if (doc["choices"].isNull()) {
        return Response::fail(ErrorCode::ParseError, "No choices in response");
    }

    JsonArray choices = doc["choices"];
    if (choices.size() == 0) {
        return Response::fail(ErrorCode::ParseError, "No choices in response");
    }

    JsonObject firstChoice = choices[0];
    if (firstChoice["message"].isNull()) {
        return Response::fail(ErrorCode::ParseError, "No message in response");
    }

    JsonObject message = firstChoice["message"];

#if ESPAI_ENABLE_TOOLS
    if (!message["tool_calls"].isNull()) {
        JsonArray toolCalls = message["tool_calls"];
        for (JsonObject tc : toolCalls) {
            ToolCall call;
            call.id = tc["id"].as<String>();
            JsonObject func = tc["function"];
            call.name = func["name"].as<String>();
            call.arguments = func["arguments"].as<String>();
            if (!call.id.isEmpty() && !call.name.isEmpty()) {
                _lastToolCalls.push_back(call);
            }
        }
    }
#endif

    if (message["content"].isNull()) {
        response.content = "";
    } else {
        response.content = message["content"].as<String>();
    }

    if (!doc["usage"].isNull()) {
        JsonObject usage = doc["usage"];
        response.promptTokens = usage["prompt_tokens"] | 0;
        response.completionTokens = usage["completion_tokens"] | 0;
    }

    response.success = true;
    return response;
}

Response OpenAIProvider::chat(
    const std::vector<Message>& messages,
    const ChatOptions& options
) {
    if (!isConfigured()) {
        return Response::fail(ErrorCode::NotConfigured, "Provider not configured");
    }

#ifdef ARDUINO
    HttpTransport* transport = getDefaultTransport();
    if (transport == nullptr) {
        return Response::fail(ErrorCode::NotConfigured, "HTTP transport not available");
    }

    if (!transport->isReady()) {
        return Response::fail(ErrorCode::NetworkError, "Network not ready");
    }

    HttpRequest req = buildHttpRequest(messages, options);
    ESPAI_LOG_D("OpenAI", "Sending chat request to %s", req.url.c_str());

    HttpResponse httpResp = transport->execute(req);

    if (!httpResp.success) {
        return handleHttpError(httpResp.statusCode, httpResp.body);
    }

    Response response = parseResponse(httpResp.body);
    response.httpStatus = httpResp.statusCode;

    return response;
#else
    (void)messages;
    (void)options;
    return Response::fail(ErrorCode::NotConfigured, "HTTP client not available in native build");
#endif
}

#if ESPAI_ENABLE_STREAMING
bool OpenAIProvider::chatStream(
    const std::vector<Message>& messages,
    const ChatOptions& options,
    StreamCallback callback
) {
    if (!isConfigured()) {
        return false;
    }

#ifdef ARDUINO
    HttpTransport* transport = getDefaultTransport();
    if (transport == nullptr || !transport->isReady()) {
        return false;
    }

    HttpRequest req = buildHttpRequest(messages, options);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, req.body);
    if (error) {
        return false;
    }
    doc["stream"] = true;
    req.body = "";
    serializeJson(doc, req.body);

    ESPAI_LOG_D("OpenAI", "Starting streaming chat to %s", req.url.c_str());

    String lineBuffer;

    bool success = transport->executeStream(req, [&](const uint8_t* data, size_t len) -> bool {
        for (size_t i = 0; i < len; i++) {
            char c = static_cast<char>(data[i]);
            if (c == '\n') {
                if (!lineBuffer.isEmpty()) {
                    String content;
                    bool done = false;

                    if (parseStreamChunk(lineBuffer, content, done)) {
                        if (!content.isEmpty()) {
                            callback(content, false);
                        }
                        if (done) {
                            callback("", true);
                            lineBuffer = "";
                            return false;
                        }
                    }
                    lineBuffer = "";
                }
            } else if (c != '\r') {
                lineBuffer += c;
            }
        }
        return true;
    });

    return success;
#else
    (void)messages;
    (void)options;
    (void)callback;
    return false;
#endif
}

bool OpenAIProvider::parseStreamChunk(const String& chunk, String& content, bool& done) {
    done = false;
    content = "";

    if (chunk.indexOf("[DONE]") != -1) {
        done = true;
        return true;
    }

    int dataPos = chunk.indexOf("data:");
    String jsonStr;
    if (dataPos == -1) {
        jsonStr = chunk;
    } else {
        unsigned int jsonStart = dataPos + 5;
        while (jsonStart < chunk.length() && chunk[jsonStart] == ' ') {
            jsonStart++;
        }
        jsonStr = chunk.substring(jsonStart);
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonStr);
    if (error) {
        return false;
    }

    if (doc["choices"].isNull()) {
        return false;
    }

    JsonArray choices = doc["choices"];
    if (choices.size() == 0) {
        return true;
    }

    JsonObject firstChoice = choices[0];
    if (firstChoice["delta"].isNull()) {
        return true;
    }

    JsonObject delta = firstChoice["delta"];
    if (!delta["content"].isNull()) {
        content = delta["content"].as<String>();
    }

    return true;
}
#endif

#if ESPAI_ENABLE_TOOLS
void OpenAIProvider::addTool(const Tool& tool) {
    if (_tools.size() < ESPAI_MAX_TOOLS) {
        _tools.push_back(tool);
    }
}

void OpenAIProvider::clearTools() {
    _tools.clear();
    _lastToolCalls.clear();
}

Message OpenAIProvider::getAssistantMessageWithToolCalls(const String& content) const {
    Message msg(Role::Assistant, content);

    if (!_lastToolCalls.empty()) {
        // Build tool_calls JSON array
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        for (const auto& call : _lastToolCalls) {
            JsonObject tc = arr.add<JsonObject>();
            tc["id"] = call.id.c_str();
            tc["type"] = "function";
            JsonObject func = tc["function"].to<JsonObject>();
            func["name"] = call.name.c_str();
            func["arguments"] = call.arguments.c_str();
        }

        serializeJson(doc, msg.toolCallsJson);
    }

    return msg;
}
#endif

std::unique_ptr<AIProvider> createOpenAIProvider(const String& apiKey, const String& model) {
    return std::unique_ptr<AIProvider>(new OpenAIProvider(apiKey, model));
}

} // namespace ESPAI

#endif // ESPAI_PROVIDER_OPENAI
