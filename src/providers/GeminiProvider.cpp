#include "GeminiProvider.h"
#include <ArduinoJson.h>

#if ESPAI_PROVIDER_GEMINI

namespace ESPAI {

GeminiProvider::GeminiProvider(const String& apiKey, const String& model) {
    _apiKey = apiKey;
    _model = model.isEmpty() ? ESPAI_DEFAULT_MODEL_GEMINI : model;
    _baseUrl = "https://generativelanguage.googleapis.com/v1beta";
}

String GeminiProvider::buildApiUrl(const String& model, const String& action) const {
    return _baseUrl + "/models/" + model + ":" + action;
}

HttpRequest GeminiProvider::buildHttpRequest(
    const std::vector<Message>& messages,
    const ChatOptions& options
) {
    HttpRequest req;

    String model = options.model.isEmpty() ? _model : options.model;

    if (_streamingRequest) {
        req.url = buildApiUrl(model, "streamGenerateContent") + "?alt=sse";
    } else {
        req.url = buildApiUrl(model, "generateContent");
    }
    req.method = "POST";
    req.body = buildRequestBody(messages, options);
    req.timeout = _timeout;

    addHeader(req, "x-goog-api-key", _apiKey);

    return req;
}

String GeminiProvider::buildRequestBody(
    const std::vector<Message>& messages,
    const ChatOptions& options
) {
    JsonDocument doc;

    String systemPrompt;
    for (const auto& msg : messages) {
        if (msg.role == Role::System) {
            systemPrompt = msg.content;
            break;
        }
    }
    if (!options.systemPrompt.isEmpty()) {
        systemPrompt = options.systemPrompt;
    }

    if (!systemPrompt.isEmpty()) {
        JsonObject sysInstr = doc["system_instruction"].to<JsonObject>();
        JsonArray sysParts = sysInstr["parts"].to<JsonArray>();
        JsonObject sysPart = sysParts.add<JsonObject>();
        sysPart["text"] = systemPrompt.c_str();
    }

    JsonArray contentsArr = doc["contents"].to<JsonArray>();
    for (const auto& msg : messages) {
        if (msg.role == Role::System) {
            continue;
        }

        JsonObject content = contentsArr.add<JsonObject>();

        if (msg.role == Role::Assistant) {
            content["role"] = "model";
        } else if (msg.role == Role::Tool) {
            content["role"] = "user";
        } else {
            content["role"] = "user";
        }

        JsonArray parts = content["parts"].to<JsonArray>();

#if ESPAI_ENABLE_TOOLS
        if (msg.role == Role::Assistant && msg.hasToolCalls()) {
            if (!msg.content.isEmpty()) {
                JsonObject textPart = parts.add<JsonObject>();
                textPart["text"] = msg.content.c_str();
            }

            JsonDocument toolCallsDoc;
            deserializeJson(toolCallsDoc, msg.toolCallsJson);
            JsonArray tcArr = toolCallsDoc.as<JsonArray>();
            for (JsonObject tc : tcArr) {
                JsonObject fcPart = parts.add<JsonObject>();
                JsonObject fcObj = fcPart["functionCall"].to<JsonObject>();
                fcObj["name"] = tc["name"].as<const char*>();

                if (!tc["arguments"].isNull()) {
                    JsonDocument argsDoc;
                    if (tc["arguments"].is<const char*>()) {
                        deserializeJson(argsDoc, tc["arguments"].as<const char*>());
                    } else {
                        argsDoc.set(tc["arguments"]);
                    }
                    fcObj["args"] = argsDoc.as<JsonObject>();
                }
            }
        } else if (msg.role == Role::Tool) {
            String funcName;
            for (int i = static_cast<int>(&msg - &messages[0]) - 1; i >= 0; i--) {
                if (messages[i].role == Role::Assistant && messages[i].hasToolCalls()) {
                    JsonDocument tcDoc;
                    deserializeJson(tcDoc, messages[i].toolCallsJson);
                    JsonArray tcArr = tcDoc.as<JsonArray>();
                    for (JsonObject tc : tcArr) {
                        if (tc["id"].as<String>() == msg.name) {
                            funcName = tc["name"].as<String>();
                            break;
                        }
                    }
                    if (!funcName.isEmpty()) break;
                }
            }
            if (funcName.isEmpty()) {
                funcName = msg.name;
            }

            JsonObject frPart = parts.add<JsonObject>();
            JsonObject frObj = frPart["functionResponse"].to<JsonObject>();
            frObj["name"] = funcName.c_str();
            JsonObject response = frObj["response"].to<JsonObject>();

            JsonDocument resultDoc;
            DeserializationError err = deserializeJson(resultDoc, msg.content);
            if (!err) {
                response["result"] = resultDoc.as<JsonVariant>();
            } else {
                response["result"] = msg.content.c_str();
            }
        } else {
#endif
            JsonObject textPart = parts.add<JsonObject>();
            textPart["text"] = msg.content.c_str();
#if ESPAI_ENABLE_TOOLS
        }
#endif
    }

    JsonObject genConfig = doc["generationConfig"].to<JsonObject>();

    genConfig["temperature"] = roundFloat(options.temperature);

    if (options.maxTokens > 0) {
        genConfig["maxOutputTokens"] = static_cast<int>(options.maxTokens);
    }

    if (options.topP < 1.0f) {
        genConfig["topP"] = roundFloat(options.topP);
    }

    if (options.frequencyPenalty != 0.0f) {
        genConfig["frequencyPenalty"] = roundFloat(options.frequencyPenalty);
    }

    if (options.presencePenalty != 0.0f) {
        genConfig["presencePenalty"] = roundFloat(options.presencePenalty);
    }

    if (options.thinkingBudget >= 0) {
        JsonObject thinkingConfig = genConfig["thinkingConfig"].to<JsonObject>();
        thinkingConfig["thinkingBudget"] = static_cast<int>(options.thinkingBudget);
    }

#if ESPAI_ENABLE_TOOLS
    if (!_tools.empty()) {
        JsonArray toolsArr = doc["tools"].to<JsonArray>();
        JsonObject toolObj = toolsArr.add<JsonObject>();
        JsonArray funcDecls = toolObj["functionDeclarations"].to<JsonArray>();

        for (const auto& tool : _tools) {
            JsonObject func = funcDecls.add<JsonObject>();
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

void GeminiProvider::parseCandidateParts(JsonArray parts, String& outText, bool joinWithNewline) {
    for (JsonObject part : parts) {
        if (part["thought"] | false) {
            continue;
        }

        if (!part["text"].isNull()) {
            if (joinWithNewline && !outText.isEmpty()) {
                outText += "\n";
            }
            outText += part["text"].as<String>();
        }
#if ESPAI_ENABLE_TOOLS
        if (!part["functionCall"].isNull()) {
            JsonObject fc = part["functionCall"];
            ToolCall toolCall;
            toolCall.name = fc["name"].as<String>();

            String argsStr;
            serializeJson(fc["args"], argsStr);
            toolCall.arguments = argsStr;

            toolCall.id = "gemini_tc_" + String(static_cast<int>(_toolCallCounter++));

            if (!toolCall.name.isEmpty()) {
                _lastToolCalls.push_back(toolCall);
            }
        }
#endif
    }
}

Response GeminiProvider::parseSingleResponse(const String& json) {
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
        int16_t errorStatus = doc["error"]["code"] | 0;
        return Response::fail(ErrorCode::ServerError, errorMsg, errorStatus);
    }

    if (doc["candidates"].isNull()) {
        return Response::fail(ErrorCode::ParseError, "No candidates in response");
    }

    JsonArray candidates = doc["candidates"];
    if (candidates.size() == 0) {
        return Response::fail(ErrorCode::ParseError, "Empty candidates array");
    }

    JsonObject firstCandidate = candidates[0];

    if (firstCandidate["content"].isNull()) {
        const char* finishReason = firstCandidate["finishReason"] | "";
        if (strlen(finishReason) > 0 && strcmp(finishReason, "STOP") != 0) {
            return Response::fail(ErrorCode::InvalidRequest,
                String("Response blocked: ") + finishReason);
        }
        return Response::fail(ErrorCode::ParseError, "No content in candidate");
    }

    JsonObject content = firstCandidate["content"];
    String textContent;

    if (!content["parts"].isNull()) {
        parseCandidateParts(content["parts"], textContent, true);
    }

    response.content = textContent;

    if (!doc["usageMetadata"].isNull()) {
        JsonObject usage = doc["usageMetadata"];
        response.promptTokens = usage["promptTokenCount"] | 0;
        response.completionTokens = usage["candidatesTokenCount"] | 0;
    }

    response.success = true;
    return response;
}

Response GeminiProvider::parseResponse(const String& responseBody) {
    if (responseBody.indexOf("data: ") != -1) {
        String allText;
        Response lastChunkResponse;
        bool foundAny = false;

#if ESPAI_ENABLE_TOOLS
        _lastToolCalls.clear();
#endif

        int pos = 0;
        while (pos < static_cast<int>(responseBody.length())) {
            int dataStart = responseBody.indexOf("data: ", pos);
            if (dataStart == -1) break;

            dataStart += 6;

            int dataEnd = responseBody.indexOf('\n', dataStart);
            if (dataEnd == -1) {
                dataEnd = responseBody.length();
            }

            String chunk = responseBody.substring(dataStart, dataEnd);
            if (!chunk.isEmpty()) {
                JsonDocument chunkDoc;
                DeserializationError err = deserializeJson(chunkDoc, chunk);
                if (!err && !chunkDoc["candidates"].isNull()) {
                    JsonArray candidates = chunkDoc["candidates"];
                    if (candidates.size() > 0) {
                        JsonObject firstCandidate = candidates[0];

                        if (!foundAny && firstCandidate["content"].isNull()) {
                            const char* finishReason = firstCandidate["finishReason"] | "";
                            if (strlen(finishReason) > 0 && strcmp(finishReason, "STOP") != 0) {
                                return Response::fail(ErrorCode::InvalidRequest,
                                    String("Response blocked: ") + finishReason);
                            }
                        }

                        if (!firstCandidate["content"].isNull()) {
                            JsonObject content = firstCandidate["content"];
                            if (!content["parts"].isNull()) {
                                parseCandidateParts(content["parts"], allText, false);
                            }
                        }

                        foundAny = true;
                    }

                    if (!chunkDoc["usageMetadata"].isNull()) {
                        JsonObject usage = chunkDoc["usageMetadata"];
                        lastChunkResponse.promptTokens = usage["promptTokenCount"] | 0;
                        lastChunkResponse.completionTokens = usage["candidatesTokenCount"] | 0;
                    }
                }
            }

            pos = dataEnd + 1;
        }

        if (!foundAny) {
            return Response::fail(ErrorCode::ParseError, "No valid data in SSE response");
        }

        Response response;
        response.success = true;
        response.content = allText;
        response.promptTokens = lastChunkResponse.promptTokens;
        response.completionTokens = lastChunkResponse.completionTokens;
        return response;
    }

    return parseSingleResponse(responseBody);
}

#if ESPAI_ENABLE_TOOLS
Message GeminiProvider::getAssistantMessageWithToolCalls(const String& content) const {
    Message msg(Role::Assistant, content);

    if (!_lastToolCalls.empty()) {
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();

        for (const auto& toolCall : _lastToolCalls) {
            JsonObject tc = arr.add<JsonObject>();
            tc["id"] = toolCall.id.c_str();
            tc["name"] = toolCall.name.c_str();
            tc["arguments"] = toolCall.arguments.c_str();
        }

        serializeJson(doc, msg.toolCallsJson);
    }

    return msg;
}
#endif

std::unique_ptr<AIProvider> createGeminiProvider(const String& apiKey, const String& model) {
    return std::unique_ptr<AIProvider>(new GeminiProvider(apiKey, model));
}

} // namespace ESPAI

#endif // ESPAI_PROVIDER_GEMINI
