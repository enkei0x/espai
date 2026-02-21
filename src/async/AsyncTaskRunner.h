#ifndef ESPAI_ASYNC_TASK_RUNNER_H
#define ESPAI_ASYNC_TASK_RUNNER_H

#include "../core/AIConfig.h"

#if ESPAI_ENABLE_ASYNC

#include "../core/AITypes.h"
#include <functional>
#include <vector>
#include <atomic>

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace ESPAI {

enum class AsyncStatus : uint8_t {
    Idle = 0,
    Running,
    Completed,
    Cancelled,
    Error
};

struct ChatRequest {
    AsyncStatus getStatus() const;
    bool isComplete() const;
    bool isCancelled() const;
    Response getResult() const;
    void cancel();
    bool poll();

private:
    friend class AsyncTaskRunner;

    AsyncStatus _status = AsyncStatus::Idle;
    Response _result;
    std::atomic<bool> _cancelRequested{false};
    SemaphoreHandle_t _mutex = nullptr;
    std::function<void(const Response&)> _onComplete;
    bool _callbackInvoked = false;

    void lock() const;
    void unlock() const;
};

using AsyncChatCallback = std::function<void(const Response&)>;
using AsyncDoneCallback = std::function<void(const Response&)>;

class AsyncTaskRunner {
public:
    AsyncTaskRunner();
    ~AsyncTaskRunner();

    bool isBusy() const;
    void cancel();
    void waitForCompletion();
    ChatRequest* getRequest() { return &_request; }

    bool launch(std::function<Response()> task, AsyncChatCallback onComplete = nullptr);
    bool launchStream(
        std::function<bool(StreamCallback)> streamTask,
        StreamCallback streamCb,
        AsyncDoneCallback onDone = nullptr
    );

private:
    ChatRequest _request;
    TaskHandle_t _taskHandle = nullptr;
    SemaphoreHandle_t _taskDoneSemaphore = nullptr;

    struct TaskParams {
        AsyncTaskRunner* runner;
        std::function<Response()> task;
    };

    struct StreamTaskParams {
        AsyncTaskRunner* runner;
        std::function<bool(StreamCallback)> streamTask;
        StreamCallback streamCb;
    };

    static void taskFunction(void* param);
    static void streamTaskFunction(void* param);
    void signalDone();

    AsyncTaskRunner(const AsyncTaskRunner&) = delete;
    AsyncTaskRunner& operator=(const AsyncTaskRunner&) = delete;
};

} // namespace ESPAI

#endif // ESPAI_ENABLE_ASYNC
#endif // ESPAI_ASYNC_TASK_RUNNER_H
