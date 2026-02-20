#include "OpenAIProvider.h"
#include <ArduinoJson.h>

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
        if (msg.role == Role::Assistant && msg.hasToolCalls()) {
            JsonDocument toolCallsDoc;
            deserializeJson(toolCallsDoc, msg.toolCallsJson);
            m["tool_calls"] = toolCallsDoc.as<JsonArray>();
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

#if ESPAI_ENABLE_TOOLS
Message OpenAIProvider::getAssistantMessageWithToolCalls(const String& content) const {
    Message msg(Role::Assistant, content);

    if (!_lastToolCalls.empty()) {
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
