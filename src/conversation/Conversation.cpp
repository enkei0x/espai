#include "Conversation.h"
#include <ArduinoJson.h>

namespace ESPAI {

Conversation::Conversation(size_t maxMessages)
    : _messages()
    , _systemPrompt()
    , _maxMessages(maxMessages) {
}

void Conversation::setSystemPrompt(const String& prompt) {
    _systemPrompt = prompt;
}

const String& Conversation::getSystemPrompt() const {
    return _systemPrompt;
}

void Conversation::addMessage(Role role, const String& content) {
    _messages.push_back(Message(role, content));
    _pruneIfNeeded();
}

void Conversation::addUserMessage(const String& content) {
    addMessage(Role::User, content);
}

void Conversation::addAssistantMessage(const String& content) {
    addMessage(Role::Assistant, content);
}

const std::vector<Message>& Conversation::getMessages() const {
    return _messages;
}

size_t Conversation::size() const {
    return _messages.size();
}

void Conversation::clear() {
    _messages.clear();
}

void Conversation::setMaxMessages(size_t max) {
    _maxMessages = max;
    _pruneIfNeeded();
}

size_t Conversation::getMaxMessages() const {
    return _maxMessages;
}

size_t Conversation::estimateTokens() const {
    size_t chars = _systemPrompt.length();
    for (const auto& msg : _messages) {
        chars += msg.content.length();
    }
    // Rough estimate: 1 token per 4 characters
    return chars / 4;
}

String Conversation::toJson() const {
    JsonDocument doc;

    doc["systemPrompt"] = _systemPrompt.c_str();
    doc["maxMessages"] = _maxMessages;

    JsonArray messagesArray = doc["messages"].to<JsonArray>();
    for (const auto& msg : _messages) {
        JsonObject msgObj = messagesArray.add<JsonObject>();
        msgObj["role"] = static_cast<int>(msg.role);
        msgObj["content"] = msg.content.c_str();
        if (!msg.name.isEmpty()) {
            msgObj["name"] = msg.name.c_str();
        }
        if (!msg.toolCallsJson.isEmpty()) {
            msgObj["toolCallsJson"] = msg.toolCallsJson.c_str();
        }
    }

    String output;
    serializeJson(doc, output);
    return output;
}

bool Conversation::fromJson(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);

    if (error) {
        return false;
    }

    _systemPrompt = doc["systemPrompt"].as<const char*>();
    _maxMessages = doc["maxMessages"] | 20;
    _messages.clear();

    JsonArray messagesArray = doc["messages"].as<JsonArray>();
    for (JsonObject msgObj : messagesArray) {
        Role role = static_cast<Role>(msgObj["role"].as<int>());
        String content = msgObj["content"].as<const char*>();
        String name;
        if (msgObj["name"]) {
            name = msgObj["name"].as<const char*>();
        }
        Message msg(role, content, name);
        if (msgObj["toolCallsJson"]) {
            msg.toolCallsJson = msgObj["toolCallsJson"].as<const char*>();
        }
        _messages.push_back(msg);
    }

    return true;
}

void Conversation::_pruneIfNeeded() {
    while (_messages.size() > _maxMessages) {
        _messages.erase(_messages.begin());
    }
}

} // namespace ESPAI
