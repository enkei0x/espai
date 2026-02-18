#include "ToolRegistry.h"
#include <ArduinoJson.h>

#if ESPAI_ENABLE_TOOLS

namespace ESPAI {

namespace {
    String makeErrorJson(const char* message) {
        JsonDocument doc;
        doc["error"] = message;
        String output;
        serializeJson(doc, output);
        return output;
    }
}

bool ToolRegistry::registerTool(const Tool& tool) {
    if (tool.name.isEmpty()) {
        return false;
    }
    if (_tools.size() >= ESPAI_MAX_TOOLS) {
        return false;
    }
    if (hasTool(tool.name)) {
        return false;
    }
    _tools.push_back(tool);
    return true;
}

bool ToolRegistry::unregisterTool(const String& name) {
    for (auto it = _tools.begin(); it != _tools.end(); ++it) {
        if (it->name == name) {
            _tools.erase(it);
            return true;
        }
    }
    return false;
}

void ToolRegistry::clearTools() {
    _tools.clear();
}

Tool* ToolRegistry::findTool(const String& name) {
    for (auto& tool : _tools) {
        if (tool.name == name) {
            return &tool;
        }
    }
    return nullptr;
}

const Tool* ToolRegistry::findTool(const String& name) const {
    for (const auto& tool : _tools) {
        if (tool.name == name) {
            return &tool;
        }
    }
    return nullptr;
}

String ToolRegistry::executeToolCall(const String& name, const String& args) const {
    const Tool* tool = findTool(name);
    if (!tool) {
        return makeErrorJson("Tool not found");
    }
    if (!tool->handler) {
        return makeErrorJson("Tool has no handler");
    }
    return tool->handler(args);
}

String ToolRegistry::executeToolCall(const ToolCall& call) const {
    return executeToolCall(call.name, call.arguments);
}

std::vector<ToolResult> ToolRegistry::executeToolCalls(const std::vector<ToolCall>& calls) const {
    std::vector<ToolResult> results;
    results.reserve(calls.size());

    for (const auto& call : calls) {
        ToolResult result;
        result.toolCallId = call.id;
        result.toolName = call.name;

        const Tool* tool = findTool(call.name);
        if (!tool) {
            result.result = makeErrorJson("Tool not found");
            result.success = false;
        } else if (!tool->handler) {
            result.result = makeErrorJson("Tool has no handler");
            result.success = false;
        } else {
            result.result = tool->handler(call.arguments);
            result.success = true;
        }

        results.push_back(result);
    }

    return results;
}

String ToolRegistry::toOpenAISchema() const {
    if (_tools.empty()) {
        return "[]";
    }

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& tool : _tools) {
        JsonObject t = arr.add<JsonObject>();
        t["type"] = "function";

        JsonObject func = t["function"].to<JsonObject>();
        func["name"] = tool.name.c_str();

        if (!tool.description.isEmpty()) {
            func["description"] = tool.description.c_str();
        }

        if (!tool.parametersJson.isEmpty()) {
            JsonDocument paramsDoc;
            DeserializationError err = deserializeJson(paramsDoc, tool.parametersJson);
            if (!err) {
                func["parameters"] = paramsDoc.as<JsonObject>();
            }
        }
    }

    String output;
    serializeJson(doc, output);
    return output;
}

String ToolRegistry::toAnthropicSchema() const {
    if (_tools.empty()) {
        return "[]";
    }

    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();

    for (const auto& tool : _tools) {
        JsonObject t = arr.add<JsonObject>();
        t["name"] = tool.name.c_str();

        if (!tool.description.isEmpty()) {
            t["description"] = tool.description.c_str();
        }

        if (!tool.parametersJson.isEmpty()) {
            JsonDocument schemaDoc;
            DeserializationError err = deserializeJson(schemaDoc, tool.parametersJson);
            if (!err) {
                t["input_schema"] = schemaDoc.as<JsonObject>();
            }
        }
    }

    String output;
    serializeJson(doc, output);
    return output;
}

} // namespace ESPAI

#endif // ESPAI_ENABLE_TOOLS
