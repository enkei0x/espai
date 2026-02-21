#ifndef ESPAI_H
#define ESPAI_H

#include "core/AIConfig.h"
#include "core/AITypes.h"
#include "core/AIClient.h"
#include "conversation/Conversation.h"
#include "providers/AIProvider.h"
#include "providers/OpenAICompatibleProvider.h"
#include "providers/ProviderFactory.h"

#if ESPAI_PROVIDER_OPENAI
#include "providers/OpenAIProvider.h"
#endif

#if ESPAI_PROVIDER_ANTHROPIC
#include "providers/AnthropicProvider.h"
#endif

#if ESPAI_PROVIDER_GEMINI
#include "providers/GeminiProvider.h"
#endif

#if ESPAI_PROVIDER_OLLAMA
#include "providers/OllamaProvider.h"
#endif

#if ESPAI_ENABLE_STREAMING
#include "http/SSEParser.h"
#endif

#include "http/HttpTransport.h"
#ifdef ARDUINO
#include "http/HttpTransportESP32.h"
#endif

#if ESPAI_ENABLE_TOOLS
#include "tools/ToolRegistry.h"
#endif

#define ESPAI_VERSION ESPAI_VERSION_STRING

#endif // ESPAI_H
