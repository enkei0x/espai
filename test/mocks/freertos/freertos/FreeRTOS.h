#ifndef MOCK_FREERTOS_H
#define MOCK_FREERTOS_H

#include <cstdint>

typedef int32_t BaseType_t;
typedef uint32_t TickType_t;

#define pdPASS 1
#define pdFAIL 0
#define portMAX_DELAY 0xFFFFFFFF
#define tskNO_AFFINITY 0x7FFFFFFF
#define configSTACK_DEPTH_TYPE uint32_t

#endif // MOCK_FREERTOS_H
