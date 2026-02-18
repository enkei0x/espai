#ifndef ESPAI_TOOL_REGISTRY_H
#define ESPAI_TOOL_REGISTRY_H

#include "../core/AIConfig.h"
#include "../core/AITypes.h"
#include <vector>

#if ESPAI_ENABLE_TOOLS

#ifndef ESPAI_MAX_TOOL_ITERATIONS
#define ESPAI_MAX_TOOL_ITERATIONS 10
#endif

namespace ESPAI {

class ToolRegistry {
public:
    ToolRegistry() = default;

    bool registerTool(const Tool& tool);
    bool unregisterTool(const String& name);
    void clearTools();

    Tool* findTool(const String& name);
    const Tool* findTool(const String& name) const;

    String executeToolCall(const String& name, const String& args) const;
    String executeToolCall(const ToolCall& call) const;
    std::vector<ToolResult> executeToolCalls(const std::vector<ToolCall>& calls) const;

    String toOpenAISchema() const;
    String toAnthropicSchema() const;

    size_t toolCount() const { return _tools.size(); }
    bool hasTool(const String& name) const { return findTool(name) != nullptr; }
    const std::vector<Tool>& getTools() const { return _tools; }

    uint8_t getMaxIterations() const { return _maxIterations; }
    void setMaxIterations(uint8_t max) { _maxIterations = max; }

private:
    std::vector<Tool> _tools;
    uint8_t _maxIterations = ESPAI_MAX_TOOL_ITERATIONS;
};

} // namespace ESPAI

#endif // ESPAI_ENABLE_TOOLS
#endif // ESPAI_TOOL_REGISTRY_H
