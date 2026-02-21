#ifndef ESPAI_ROOT_CA_CERTS_H
#define ESPAI_ROOT_CA_CERTS_H

#ifdef ESP32
#include <pgmspace.h>
#define ESPAI_PROGMEM PROGMEM
#else
#define ESPAI_PROGMEM
#endif

namespace ESPAI {

// GlobalSign (OpenAI, Anthropic) + GTS Root R1 (Gemini)
extern const char ESPAI_CA_BUNDLE[];

// Standalone Google cert for Gemini-only setups (saves memory)
extern const char ESPAI_GOOGLE_CA[];

} // namespace ESPAI

#endif // ESPAI_ROOT_CA_CERTS_H
