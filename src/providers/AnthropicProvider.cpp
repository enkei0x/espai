#include "AnthropicProvider.h"
#include <ArduinoJson.h>

#if ESPAI_PROVIDER_ANTHROPIC

namespace ESPAI {

AnthropicProvider::AnthropicProvider(const String& apiKey, const String& model) {
    _apiKey = apiKey;
    _model = model.isEmpty() ? ESPAI_DEFAULT_MODEL_ANTHROPIC : model;
    _baseUrl = "https://api.anthropic.com/v1/messages";
    _apiVersion = "2023-06-01";
}

HttpRequest AnthropicProvider::buildHttpRequest(
    const std::vector<Message>& messages,
    const ChatOptions& options
) {
    HttpRequest req;
    req.url = _baseUrl;
    req.method = "POST";
    req.body = buildRequestBody(messages, options);
    req.timeout = _timeout;

    addHeader(req, "x-api-key", _apiKey);
    addHeader(req, "anthropic-version", _apiVersion);

    return req;
}

String AnthropicProvider::extractSystemPrompt(const std::vector<Message>& messages) {
    for (const auto& msg : messages) {
        if (msg.role == Role::System) {
            return msg.content;
        }
    }
    return "";
}

String AnthropicProvider::buildRequestBody(
    const std::vector<Message>& messages,
    const ChatOptions& options
) {
    JsonDocument doc;

    String model = options.model.isEmpty() ? _model : options.model;
    doc["model"] = model.c_str();

    int16_t maxTokens = options.maxTokens > 0 ? options.maxTokens : 1024;
    doc["max_tokens"] = static_cast<int>(maxTokens);

    String systemPrompt = extractSystemPrompt(messages);
    if (!options.systemPrompt.isEmpty()) {
        systemPrompt = options.systemPrompt;
    }
    if (!systemPrompt.isEmpty()) {
        doc["system"] = systemPrompt.c_str();
    }

    JsonArray messagesArr = doc["messages"].to<JsonArray>();
    for (const auto& msg : messages) {
        if (msg.role == Role::System) {
            continue;
        }

        JsonObject m = messagesArr.add<JsonObject>();

#if ESPAI_ENABLE_TOOLS
        if (msg.role == Role::Tool) {
            m["role"] = "user";
            JsonArray contentArr = m["content"].to<JsonArray>();
            JsonObject toolResult = contentArr.add<JsonObject>();
            toolResult["type"] = "tool_result";
            toolResult["tool_use_id"] = msg.name.c_str();
            toolResult["content"] = msg.content.c_str();
        } else if (msg.role == Role::Assistant && msg.hasToolCalls()) {
            m["role"] = "assistant";
            JsonDocument contentDoc;
            deserializeJson(contentDoc, msg.toolCallsJson);
            m["content"] = contentDoc.as<JsonArray>();
        } else {
#endif
            m["role"] = roleToString(msg.role);
            m["content"] = msg.content.c_str();
#if ESPAI_ENABLE_TOOLS
        }
#endif
    }

    if (options.temperature != 0.7f) {
        doc["temperature"] = roundFloat(options.temperature);
    }

    if (options.topP < 1.0f) {
        doc["top_p"] = roundFloat(options.topP);
    }

#if ESPAI_ENABLE_TOOLS
    if (!_tools.empty()) {
        JsonArray toolsArr = doc["tools"].to<JsonArray>();
        for (const auto& tool : _tools) {
            JsonObject t = toolsArr.add<JsonObject>();
            t["name"] = tool.name.c_str();
            if (!tool.description.isEmpty()) {
                t["description"] = tool.description.c_str();
            }
            if (!tool.parametersJson.isEmpty()) {
                JsonDocument schemaDoc;
                deserializeJson(schemaDoc, tool.parametersJson);
                t["input_schema"] = schemaDoc.as<JsonObject>();
            }
        }
    }
#endif

    String output;
    serializeJson(doc, output);
    return output;
}

Response AnthropicProvider::parseResponse(const String& json) {
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

    if (doc["content"].isNull()) {
        return Response::fail(ErrorCode::ParseError, "No content in response");
    }

    JsonArray contentArr = doc["content"];
    if (contentArr.size() == 0) {
        return Response::fail(ErrorCode::ParseError, "Empty content array");
    }

    String textContent;
    for (JsonObject block : contentArr) {
        const char* type = block["type"] | "";

        if (strcmp(type, "text") == 0) {
            if (!textContent.isEmpty()) {
                textContent += "\n";
            }
            textContent += block["text"].as<String>();
        }
#if ESPAI_ENABLE_TOOLS
        else if (strcmp(type, "tool_use") == 0) {
            ToolCall toolCall;
            toolCall.id = block["id"].as<String>();
            toolCall.name = block["name"].as<String>();

            JsonObject input = block["input"];
            String inputStr;
            serializeJson(input, inputStr);
            toolCall.arguments = inputStr;

            if (!toolCall.id.isEmpty() && !toolCall.name.isEmpty()) {
                _lastToolCalls.push_back(toolCall);
            }
        }
#endif
    }

    response.content = textContent;

    if (!doc["usage"].isNull()) {
        JsonObject usage = doc["usage"];
        response.promptTokens = usage["input_tokens"] | 0;
        response.completionTokens = usage["output_tokens"] | 0;
    }

    response.success = true;
    return response;
}

#if ESPAI_ENABLE_TOOLS
Message AnthropicProvider::getAssistantMessageWithToolCalls(const String& content) const {
    Message msg(Role::Assistant, content);

    if (!_lastToolCalls.empty()) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        if (!content.isEmpty()) {
            JsonObject textBlock = arr.add<JsonObject>();
            textBlock["type"] = "text";
            textBlock["text"] = content.c_str();
        }

        for (const auto& toolCall : _lastToolCalls) {
            JsonObject tu = arr.add<JsonObject>();
            tu["type"] = "tool_use";
            tu["id"] = toolCall.id.c_str();
            tu["name"] = toolCall.name.c_str();

            JsonDocument inputDoc;
            deserializeJson(inputDoc, toolCall.arguments);
            tu["input"] = inputDoc.as<JsonObject>();
        }

        serializeJson(doc, msg.toolCallsJson);
    }

    return msg;
}
#endif

std::unique_ptr<AIProvider> createAnthropicProvider(const String& apiKey, const String& model) {
    return std::unique_ptr<AIProvider>(new AnthropicProvider(apiKey, model));
}

} // namespace ESPAI

#endif // ESPAI_PROVIDER_ANTHROPIC
