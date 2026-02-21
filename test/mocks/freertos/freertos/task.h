#ifndef MOCK_FREERTOS_TASK_H
#define MOCK_FREERTOS_TASK_H

#include "FreeRTOS.h"

typedef void* TaskHandle_t;
typedef void (*TaskFunction_t)(void*);

extern BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char* pcName,
    configSTACK_DEPTH_TYPE usStackDepth,
    void* pvParameters,
    int uxPriority,
    TaskHandle_t* pxCreatedTask,
    BaseType_t xCoreID
);

extern void vTaskDelete(TaskHandle_t xTask);

#endif // MOCK_FREERTOS_TASK_H
