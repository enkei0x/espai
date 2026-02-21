#ifdef NATIVE_TEST

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#include <mutex>
#include <condition_variable>
#include <thread>

struct MockSemaphore {
    std::mutex mtx;
    std::condition_variable cv;
    int count;

    explicit MockSemaphore(int initialCount) : count(initialCount) {}
};

SemaphoreHandle_t xSemaphoreCreateMutex() {
    return static_cast<SemaphoreHandle_t>(new MockSemaphore(1));
}

SemaphoreHandle_t xSemaphoreCreateBinary() {
    return static_cast<SemaphoreHandle_t>(new MockSemaphore(0));
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t sem, TickType_t timeout) {
    if (!sem) return pdFAIL;
    auto* s = static_cast<MockSemaphore*>(sem);

    std::unique_lock<std::mutex> lock(s->mtx);

    if (timeout == portMAX_DELAY) {
        s->cv.wait(lock, [s]() { return s->count > 0; });
    } else {
        bool acquired = s->cv.wait_for(
            lock,
            std::chrono::milliseconds(timeout),
            [s]() { return s->count > 0; }
        );
        if (!acquired) return pdFAIL;
    }

    s->count--;
    return pdPASS;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t sem) {
    if (!sem) return pdFAIL;
    auto* s = static_cast<MockSemaphore*>(sem);

    {
        std::lock_guard<std::mutex> lock(s->mtx);
        // Cap at 1 to emulate binary semaphore / mutex behavior
        if (s->count < 1) {
            s->count++;
        }
    }
    s->cv.notify_one();
    return pdPASS;
}

void vSemaphoreDelete(SemaphoreHandle_t sem) {
    if (!sem) return;
    delete static_cast<MockSemaphore*>(sem);
}

BaseType_t xTaskCreatePinnedToCore(
    TaskFunction_t pvTaskCode,
    const char* pcName,
    configSTACK_DEPTH_TYPE usStackDepth,
    void* pvParameters,
    int uxPriority,
    TaskHandle_t* pxCreatedTask,
    BaseType_t xCoreID
) {
    (void)pcName;
    (void)usStackDepth;
    (void)uxPriority;
    (void)xCoreID;

    std::thread t(pvTaskCode, pvParameters);
    t.detach();

    // Non-null sentinel so AsyncTaskRunner knows a task exists
    if (pxCreatedTask) {
        *pxCreatedTask = reinterpret_cast<TaskHandle_t>(0x1);
    }

    return pdPASS;
}

void vTaskDelete(TaskHandle_t xTask) {
    // No-op: std::thread returns naturally after signalDone()
    (void)xTask;
}

#endif // NATIVE_TEST
