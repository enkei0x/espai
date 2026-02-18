#ifndef ESPAI_CONVERSATION_H
#define ESPAI_CONVERSATION_H

#include "../core/AITypes.h"
#include <vector>

namespace ESPAI {

class Conversation {
public:
    explicit Conversation(size_t maxMessages = 20);

    void setSystemPrompt(const String& prompt);
    const String& getSystemPrompt() const;

    void addMessage(Role role, const String& content);
    void addUserMessage(const String& content);
    void addAssistantMessage(const String& content);

    const std::vector<Message>& getMessages() const;
    size_t size() const;
    void clear();

    void setMaxMessages(size_t max);
    size_t getMaxMessages() const;

    size_t estimateTokens() const;

    String toJson() const;
    bool fromJson(const String& json);

private:
    std::vector<Message> _messages;
    String _systemPrompt;
    size_t _maxMessages;

    void _pruneIfNeeded();
};

} // namespace ESPAI

#endif // ESPAI_CONVERSATION_H
