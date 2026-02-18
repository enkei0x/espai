#ifdef NATIVE_TEST

#include <unity.h>
#include "tools/ToolRegistry.h"
#include <ArduinoJson.h>

using namespace ESPAI;

// Helper function to build tool schema JSON using ArduinoJson
String buildToolSchema(const char* propName, const char* propType, const char* description = nullptr, bool required = false) {
    JsonDocument doc;
    doc["type"] = "object";

    auto props = doc["properties"].to<JsonObject>();
    auto prop = props[propName].to<JsonObject>();
    prop["type"] = propType;
    if (description) {
        prop["description"] = description;
    }

    if (required) {
        auto req = doc["required"].to<JsonArray>();
        req.add(propName);
    }

    String output;
    serializeJson(doc, output);
    return output;
}

// Helper function for complex schemas with multiple properties
String buildComplexToolSchema(
    std::initializer_list<std::tuple<const char*, const char*, const char*>> properties,
    std::initializer_list<const char*> requiredProps = {}
) {
    JsonDocument doc;
    doc["type"] = "object";

    auto props = doc["properties"].to<JsonObject>();
    for (const auto& [name, type, desc] : properties) {
        auto prop = props[name].to<JsonObject>();
        prop["type"] = type;
        if (desc && strlen(desc) > 0) {
            prop["description"] = desc;
        }
    }

    if (requiredProps.size() > 0) {
        auto req = doc["required"].to<JsonArray>();
        for (const char* name : requiredProps) {
            req.add(name);
        }
    }

    String output;
    serializeJson(doc, output);
    return output;
}

// Helper for empty schema
String buildEmptySchema() {
    JsonDocument doc;
    doc["type"] = "object";
    doc["properties"].to<JsonObject>();

    String output;
    serializeJson(doc, output);
    return output;
}

// Helper for complex schema with advanced property types
String buildAdvancedToolSchema() {
    JsonDocument doc;
    doc["type"] = "object";

    auto props = doc["properties"].to<JsonObject>();

    // name property with description
    auto nameProp = props["name"].to<JsonObject>();
    nameProp["type"] = "string";
    nameProp["description"] = "The name";

    // count property with minimum
    auto countProp = props["count"].to<JsonObject>();
    countProp["type"] = "integer";
    countProp["minimum"] = 0;

    // enabled property (boolean)
    auto enabledProp = props["enabled"].to<JsonObject>();
    enabledProp["type"] = "boolean";

    // tags property (array)
    auto tagsProp = props["tags"].to<JsonObject>();
    tagsProp["type"] = "array";
    auto items = tagsProp["items"].to<JsonObject>();
    items["type"] = "string";

    // required array
    auto req = doc["required"].to<JsonArray>();
    req.add("name");

    String output;
    serializeJson(doc, output);
    return output;
}

static ToolRegistry* registry = nullptr;

void setUp() {
    registry = new ToolRegistry();
}

void tearDown() {
    delete registry;
    registry = nullptr;
}

void test_register_tool_basic() {
    Tool tool;
    tool.name = "get_temperature";
    tool.description = "Get current temperature";
    tool.parametersJson = buildToolSchema("unit", "string");
    tool.handler = [](const String& args) { return String("23.5"); };

    bool result = registry->registerTool(tool);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, registry->toolCount());
}

void test_register_tool_without_handler() {
    Tool tool;
    tool.name = "no_handler_tool";
    tool.description = "Tool without handler";

    bool result = registry->registerTool(tool);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(1, registry->toolCount());
}

void test_register_tool_empty_name_fails() {
    Tool tool;
    tool.name = "";
    tool.description = "Invalid tool";

    bool result = registry->registerTool(tool);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(0, registry->toolCount());
}

void test_register_duplicate_tool_fails() {
    Tool tool1;
    tool1.name = "duplicate_tool";
    tool1.description = "First tool";

    Tool tool2;
    tool2.name = "duplicate_tool";
    tool2.description = "Second tool";

    registry->registerTool(tool1);
    bool result = registry->registerTool(tool2);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(1, registry->toolCount());
}

void test_register_multiple_tools() {
    Tool tool1;
    tool1.name = "tool1";

    Tool tool2;
    tool2.name = "tool2";

    Tool tool3;
    tool3.name = "tool3";

    registry->registerTool(tool1);
    registry->registerTool(tool2);
    registry->registerTool(tool3);

    TEST_ASSERT_EQUAL(3, registry->toolCount());
}

void test_max_tools_limit() {
    for (int i = 0; i < ESPAI_MAX_TOOLS; i++) {
        Tool tool;
        tool.name = "tool_" + String(i);
        bool result = registry->registerTool(tool);
        TEST_ASSERT_TRUE(result);
    }

    TEST_ASSERT_EQUAL(ESPAI_MAX_TOOLS, registry->toolCount());

    Tool extraTool;
    extraTool.name = "extra_tool";
    bool result = registry->registerTool(extraTool);

    TEST_ASSERT_FALSE(result);
    TEST_ASSERT_EQUAL(ESPAI_MAX_TOOLS, registry->toolCount());
}

void test_find_tool_exists() {
    Tool tool;
    tool.name = "findable_tool";
    tool.description = "A tool to find";
    registry->registerTool(tool);

    Tool* found = registry->findTool("findable_tool");

    TEST_ASSERT_NOT_NULL(found);
    TEST_ASSERT_EQUAL_STRING("findable_tool", found->name.c_str());
    TEST_ASSERT_EQUAL_STRING("A tool to find", found->description.c_str());
}

void test_find_tool_not_exists() {
    Tool* found = registry->findTool("nonexistent_tool");

    TEST_ASSERT_NULL(found);
}

void test_find_tool_const() {
    Tool tool;
    tool.name = "const_findable";
    registry->registerTool(tool);

    const ToolRegistry* constRegistry = registry;
    const Tool* found = constRegistry->findTool("const_findable");

    TEST_ASSERT_NOT_NULL(found);
}

void test_has_tool() {
    Tool tool;
    tool.name = "has_tool_test";
    registry->registerTool(tool);

    TEST_ASSERT_TRUE(registry->hasTool("has_tool_test"));
    TEST_ASSERT_FALSE(registry->hasTool("nonexistent"));
}

void test_unregister_tool() {
    Tool tool;
    tool.name = "removable_tool";
    registry->registerTool(tool);

    TEST_ASSERT_EQUAL(1, registry->toolCount());

    bool result = registry->unregisterTool("removable_tool");

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL(0, registry->toolCount());
    TEST_ASSERT_FALSE(registry->hasTool("removable_tool"));
}

void test_unregister_nonexistent_tool() {
    bool result = registry->unregisterTool("nonexistent");

    TEST_ASSERT_FALSE(result);
}

void test_clear_tools() {
    Tool tool1;
    tool1.name = "tool1";
    Tool tool2;
    tool2.name = "tool2";

    registry->registerTool(tool1);
    registry->registerTool(tool2);

    TEST_ASSERT_EQUAL(2, registry->toolCount());

    registry->clearTools();

    TEST_ASSERT_EQUAL(0, registry->toolCount());
}

void test_execute_tool_call_basic() {
    Tool tool;
    tool.name = "echo_tool";
    tool.handler = [](const String& args) { return "Echo: " + args; };
    registry->registerTool(tool);

    String result = registry->executeToolCall("echo_tool", "test input");

    TEST_ASSERT_EQUAL_STRING("Echo: test input", result.c_str());
}

void test_execute_tool_call_with_json_args() {
    Tool tool;
    tool.name = "json_tool";
    tool.handler = [](const String& args) {
        JsonDocument doc;
        deserializeJson(doc, args);
        String unit = doc["unit"] | "unknown";
        return "Unit: " + unit;
    };
    registry->registerTool(tool);

    String result = registry->executeToolCall("json_tool", "{\"unit\":\"celsius\"}");

    TEST_ASSERT_EQUAL_STRING("Unit: celsius", result.c_str());
}

void test_execute_tool_call_not_found() {
    String result = registry->executeToolCall("nonexistent", "{}");

    TEST_ASSERT_TRUE(result.find("error") != std::string::npos);
    TEST_ASSERT_TRUE(result.find("not found") != std::string::npos);
}

void test_execute_tool_call_no_handler() {
    Tool tool;
    tool.name = "no_handler";
    tool.handler = nullptr;
    registry->registerTool(tool);

    String result = registry->executeToolCall("no_handler", "{}");

    TEST_ASSERT_TRUE(result.find("error") != std::string::npos);
    TEST_ASSERT_TRUE(result.find("no handler") != std::string::npos);
}

void test_execute_tool_call_struct() {
    Tool tool;
    tool.name = "struct_tool";
    tool.handler = [](const String& args) { return "Result: " + args; };
    registry->registerTool(tool);

    ToolCall call;
    call.id = "call_123";
    call.name = "struct_tool";
    call.arguments = "test_args";

    String result = registry->executeToolCall(call);

    TEST_ASSERT_EQUAL_STRING("Result: test_args", result.c_str());
}

void test_execute_multiple_tool_calls() {
    Tool tool1;
    tool1.name = "tool_a";
    tool1.handler = [](const String& args) { return "A:" + args; };

    Tool tool2;
    tool2.name = "tool_b";
    tool2.handler = [](const String& args) { return "B:" + args; };

    registry->registerTool(tool1);
    registry->registerTool(tool2);

    std::vector<ToolCall> calls;

    ToolCall call1;
    call1.id = "id_1";
    call1.name = "tool_a";
    call1.arguments = "arg1";
    calls.push_back(call1);

    ToolCall call2;
    call2.id = "id_2";
    call2.name = "tool_b";
    call2.arguments = "arg2";
    calls.push_back(call2);

    auto results = registry->executeToolCalls(calls);

    TEST_ASSERT_EQUAL(2, results.size());
    TEST_ASSERT_EQUAL_STRING("id_1", results[0].toolCallId.c_str());
    TEST_ASSERT_EQUAL_STRING("A:arg1", results[0].result.c_str());
    TEST_ASSERT_TRUE(results[0].success);
    TEST_ASSERT_EQUAL_STRING("id_2", results[1].toolCallId.c_str());
    TEST_ASSERT_EQUAL_STRING("B:arg2", results[1].result.c_str());
    TEST_ASSERT_TRUE(results[1].success);
}

void test_execute_tool_calls_with_failure() {
    Tool tool;
    tool.name = "working_tool";
    tool.handler = [](const String& args) { return "OK"; };
    registry->registerTool(tool);

    std::vector<ToolCall> calls;

    ToolCall call1;
    call1.id = "id_1";
    call1.name = "working_tool";
    call1.arguments = "{}";
    calls.push_back(call1);

    ToolCall call2;
    call2.id = "id_2";
    call2.name = "nonexistent_tool";
    call2.arguments = "{}";
    calls.push_back(call2);

    auto results = registry->executeToolCalls(calls);

    TEST_ASSERT_EQUAL(2, results.size());
    TEST_ASSERT_TRUE(results[0].success);
    TEST_ASSERT_FALSE(results[1].success);
}

void test_to_openai_schema_empty() {
    String schema = registry->toOpenAISchema();

    TEST_ASSERT_EQUAL_STRING("[]", schema.c_str());
}

void test_to_openai_schema_single_tool() {
    Tool tool;
    tool.name = "get_weather";
    tool.description = "Get weather for a location";
    tool.parametersJson = buildToolSchema("location", "string", nullptr, true);
    registry->registerTool(tool);

    String schema = registry->toOpenAISchema();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, schema);
    TEST_ASSERT_FALSE(err);

    JsonArray arr = doc.as<JsonArray>();
    TEST_ASSERT_EQUAL(1, arr.size());

    JsonObject t = arr[0];
    TEST_ASSERT_EQUAL_STRING("function", t["type"].as<const char*>());

    JsonObject func = t["function"];
    TEST_ASSERT_EQUAL_STRING("get_weather", func["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Get weather for a location", func["description"].as<const char*>());

    JsonObject params = func["parameters"];
    TEST_ASSERT_EQUAL_STRING("object", params["type"].as<const char*>());
    TEST_ASSERT_TRUE(params["properties"]["location"].is<JsonObject>());
}

void test_to_openai_schema_multiple_tools() {
    Tool tool1;
    tool1.name = "tool_one";
    tool1.description = "First tool";

    Tool tool2;
    tool2.name = "tool_two";
    tool2.description = "Second tool";

    registry->registerTool(tool1);
    registry->registerTool(tool2);

    String schema = registry->toOpenAISchema();

    JsonDocument doc;
    deserializeJson(doc, schema);
    JsonArray arr = doc.as<JsonArray>();

    TEST_ASSERT_EQUAL(2, arr.size());
    TEST_ASSERT_EQUAL_STRING("tool_one", arr[0]["function"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("tool_two", arr[1]["function"]["name"].as<const char*>());
}

void test_to_openai_schema_no_description() {
    Tool tool;
    tool.name = "minimal_tool";
    registry->registerTool(tool);

    String schema = registry->toOpenAISchema();

    JsonDocument doc;
    deserializeJson(doc, schema);

    TEST_ASSERT_TRUE(doc[0]["function"]["description"].isNull());
}

void test_to_anthropic_schema_empty() {
    String schema = registry->toAnthropicSchema();

    TEST_ASSERT_EQUAL_STRING("[]", schema.c_str());
}

void test_to_anthropic_schema_single_tool() {
    Tool tool;
    tool.name = "get_weather";
    tool.description = "Get weather for a location";
    tool.parametersJson = buildToolSchema("location", "string", nullptr, true);
    registry->registerTool(tool);

    String schema = registry->toAnthropicSchema();

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, schema);
    TEST_ASSERT_FALSE(err);

    JsonArray arr = doc.as<JsonArray>();
    TEST_ASSERT_EQUAL(1, arr.size());

    JsonObject t = arr[0];
    TEST_ASSERT_EQUAL_STRING("get_weather", t["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Get weather for a location", t["description"].as<const char*>());

    JsonObject inputSchema = t["input_schema"];
    TEST_ASSERT_EQUAL_STRING("object", inputSchema["type"].as<const char*>());
    TEST_ASSERT_TRUE(inputSchema["properties"]["location"].is<JsonObject>());
}

void test_to_anthropic_schema_multiple_tools() {
    Tool tool1;
    tool1.name = "tool_a";
    tool1.description = "Tool A";

    Tool tool2;
    tool2.name = "tool_b";
    tool2.description = "Tool B";

    registry->registerTool(tool1);
    registry->registerTool(tool2);

    String schema = registry->toAnthropicSchema();

    JsonDocument doc;
    deserializeJson(doc, schema);
    JsonArray arr = doc.as<JsonArray>();

    TEST_ASSERT_EQUAL(2, arr.size());
    TEST_ASSERT_EQUAL_STRING("tool_a", arr[0]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("tool_b", arr[1]["name"].as<const char*>());
}

void test_openai_vs_anthropic_schema_difference() {
    Tool tool;
    tool.name = "test_tool";
    tool.description = "Test description";
    tool.parametersJson = buildEmptySchema();
    registry->registerTool(tool);

    String openai = registry->toOpenAISchema();
    String anthropic = registry->toAnthropicSchema();

    JsonDocument openaiDoc;
    JsonDocument anthropicDoc;
    deserializeJson(openaiDoc, openai);
    deserializeJson(anthropicDoc, anthropic);

    TEST_ASSERT_TRUE(openaiDoc[0]["type"].is<const char*>());
    TEST_ASSERT_EQUAL_STRING("function", openaiDoc[0]["type"].as<const char*>());
    TEST_ASSERT_TRUE(openaiDoc[0]["function"]["parameters"].is<JsonObject>());

    TEST_ASSERT_TRUE(anthropicDoc[0]["type"].isNull());
    TEST_ASSERT_TRUE(anthropicDoc[0]["input_schema"].is<JsonObject>());
}

void test_get_tools() {
    Tool tool1;
    tool1.name = "tool_x";

    Tool tool2;
    tool2.name = "tool_y";

    registry->registerTool(tool1);
    registry->registerTool(tool2);

    const auto& tools = registry->getTools();

    TEST_ASSERT_EQUAL(2, tools.size());
    TEST_ASSERT_EQUAL_STRING("tool_x", tools[0].name.c_str());
    TEST_ASSERT_EQUAL_STRING("tool_y", tools[1].name.c_str());
}

void test_max_iterations_default() {
    TEST_ASSERT_EQUAL(ESPAI_MAX_TOOL_ITERATIONS, registry->getMaxIterations());
}

void test_max_iterations_set() {
    registry->setMaxIterations(5);
    TEST_ASSERT_EQUAL(5, registry->getMaxIterations());
}

void test_tool_call_default_constructor() {
    ToolCall call;

    TEST_ASSERT_TRUE(call.id.isEmpty());
    TEST_ASSERT_TRUE(call.name.isEmpty());
    TEST_ASSERT_TRUE(call.arguments.isEmpty());
}

void test_tool_result_default_constructor() {
    ToolResult result;

    TEST_ASSERT_TRUE(result.toolCallId.isEmpty());
    TEST_ASSERT_TRUE(result.toolName.isEmpty());
    TEST_ASSERT_TRUE(result.result.isEmpty());
    TEST_ASSERT_FALSE(result.success);
}

void test_tool_result_parameterized_constructor() {
    ToolResult result("call_abc", "my_tool", "result_data", true);

    TEST_ASSERT_EQUAL_STRING("call_abc", result.toolCallId.c_str());
    TEST_ASSERT_EQUAL_STRING("my_tool", result.toolName.c_str());
    TEST_ASSERT_EQUAL_STRING("result_data", result.result.c_str());
    TEST_ASSERT_TRUE(result.success);
}

void test_tool_handler_receives_correct_args() {
    String receivedArgs;

    Tool tool;
    tool.name = "capture_tool";
    tool.handler = [&receivedArgs](const String& args) {
        receivedArgs = args;
        return "done";
    };
    registry->registerTool(tool);

    registry->executeToolCall("capture_tool", "{\"key\":\"value\"}");

    TEST_ASSERT_EQUAL_STRING("{\"key\":\"value\"}", receivedArgs.c_str());
}

void test_complex_parameters_json() {
    Tool tool;
    tool.name = "complex_tool";
    tool.parametersJson = buildAdvancedToolSchema();
    registry->registerTool(tool);

    String openai = registry->toOpenAISchema();
    String anthropic = registry->toAnthropicSchema();

    JsonDocument openaiDoc;
    deserializeJson(openaiDoc, openai);

    JsonObject params = openaiDoc[0]["function"]["parameters"];
    TEST_ASSERT_EQUAL_STRING("object", params["type"].as<const char*>());
    TEST_ASSERT_TRUE(params["properties"]["name"].is<JsonObject>());
    TEST_ASSERT_TRUE(params["properties"]["count"].is<JsonObject>());
    TEST_ASSERT_TRUE(params["properties"]["enabled"].is<JsonObject>());
    TEST_ASSERT_TRUE(params["properties"]["tags"].is<JsonObject>());

    JsonArray required = params["required"];
    TEST_ASSERT_EQUAL(1, required.size());
    TEST_ASSERT_EQUAL_STRING("name", required[0].as<const char*>());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_register_tool_basic);
    RUN_TEST(test_register_tool_without_handler);
    RUN_TEST(test_register_tool_empty_name_fails);
    RUN_TEST(test_register_duplicate_tool_fails);
    RUN_TEST(test_register_multiple_tools);
    RUN_TEST(test_max_tools_limit);

    RUN_TEST(test_find_tool_exists);
    RUN_TEST(test_find_tool_not_exists);
    RUN_TEST(test_find_tool_const);
    RUN_TEST(test_has_tool);

    RUN_TEST(test_unregister_tool);
    RUN_TEST(test_unregister_nonexistent_tool);
    RUN_TEST(test_clear_tools);

    RUN_TEST(test_execute_tool_call_basic);
    RUN_TEST(test_execute_tool_call_with_json_args);
    RUN_TEST(test_execute_tool_call_not_found);
    RUN_TEST(test_execute_tool_call_no_handler);
    RUN_TEST(test_execute_tool_call_struct);
    RUN_TEST(test_execute_multiple_tool_calls);
    RUN_TEST(test_execute_tool_calls_with_failure);

    RUN_TEST(test_to_openai_schema_empty);
    RUN_TEST(test_to_openai_schema_single_tool);
    RUN_TEST(test_to_openai_schema_multiple_tools);
    RUN_TEST(test_to_openai_schema_no_description);

    RUN_TEST(test_to_anthropic_schema_empty);
    RUN_TEST(test_to_anthropic_schema_single_tool);
    RUN_TEST(test_to_anthropic_schema_multiple_tools);

    RUN_TEST(test_openai_vs_anthropic_schema_difference);

    RUN_TEST(test_get_tools);
    RUN_TEST(test_max_iterations_default);
    RUN_TEST(test_max_iterations_set);

    RUN_TEST(test_tool_call_default_constructor);
    RUN_TEST(test_tool_result_default_constructor);
    RUN_TEST(test_tool_result_parameterized_constructor);

    RUN_TEST(test_tool_handler_receives_correct_args);
    RUN_TEST(test_complex_parameters_json);

    return UNITY_END();
}

#else
// For ESP32 - empty file
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
