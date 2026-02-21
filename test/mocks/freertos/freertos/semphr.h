#ifndef MOCK_FREERTOS_SEMPHR_H
#define MOCK_FREERTOS_SEMPHR_H

#include "FreeRTOS.h"

typedef void* SemaphoreHandle_t;

extern SemaphoreHandle_t xSemaphoreCreateMutex();
extern SemaphoreHandle_t xSemaphoreCreateBinary();
extern BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout);
extern BaseType_t xSemaphoreGive(SemaphoreHandle_t sem);
extern void vSemaphoreDelete(SemaphoreHandle_t sem);

#endif // MOCK_FREERTOS_SEMPHR_H
