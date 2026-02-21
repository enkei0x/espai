#include "AsyncTaskRunner.h"

#if ESPAI_ENABLE_ASYNC

namespace ESPAI {

void ChatRequest::lock() const {
    if (_mutex) xSemaphoreTake(_mutex, portMAX_DELAY);
}

void ChatRequest::unlock() const {
    if (_mutex) xSemaphoreGive(_mutex);
}

AsyncStatus ChatRequest::getStatus() const {
    lock();
    AsyncStatus s = _status;
    unlock();
    return s;
}

bool ChatRequest::isComplete() const {
    AsyncStatus s = getStatus();
    return s == AsyncStatus::Completed || s == AsyncStatus::Cancelled || s == AsyncStatus::Error;
}

bool ChatRequest::isCancelled() const {
    return _cancelRequested.load(std::memory_order_relaxed);
}

Response ChatRequest::getResult() const {
    lock();
    Response r = _result;
    unlock();
    return r;
}

void ChatRequest::cancel() {
    _cancelRequested.store(true, std::memory_order_relaxed);
}

bool ChatRequest::poll() {
    if (!isComplete()) return false;

    lock();
    if (_callbackInvoked) {
        unlock();
        return true;
    }
    _callbackInvoked = true;
    auto cb = _onComplete;
    Response result = _result;
    unlock();

    if (cb) {
        cb(result);
    }
    return true;
}

AsyncTaskRunner::AsyncTaskRunner() {
    _request._mutex = xSemaphoreCreateMutex();
    _taskDoneSemaphore = xSemaphoreCreateBinary();
}

AsyncTaskRunner::~AsyncTaskRunner() {
    waitForCompletion();
    if (_request._mutex) {
        vSemaphoreDelete(_request._mutex);
        _request._mutex = nullptr;
    }
    if (_taskDoneSemaphore) {
        vSemaphoreDelete(_taskDoneSemaphore);
        _taskDoneSemaphore = nullptr;
    }
}

void AsyncTaskRunner::waitForCompletion() {
    if (_taskHandle != nullptr) {
        _request.cancel();
        xSemaphoreTake(_taskDoneSemaphore, portMAX_DELAY);
        _taskHandle = nullptr;
    }
}

bool AsyncTaskRunner::isBusy() const {
    return _request.getStatus() == AsyncStatus::Running;
}

void AsyncTaskRunner::cancel() {
    _request.cancel();
}

void AsyncTaskRunner::signalDone() {
    xSemaphoreGive(_taskDoneSemaphore);
}

bool AsyncTaskRunner::launch(std::function<Response()> task, AsyncChatCallback onComplete) {
    if (isBusy()) return false;

    // If a previous task completed, consume its done semaphore before reuse
    if (_taskHandle != nullptr) {
        xSemaphoreTake(_taskDoneSemaphore, portMAX_DELAY);
        _taskHandle = nullptr;
    }

    _request._status = AsyncStatus::Running;
    _request._result = Response();
    _request._cancelRequested.store(false, std::memory_order_relaxed);
    _request._onComplete = onComplete;
    _request._callbackInvoked = false;

    auto* params = new TaskParams{this, std::move(task)};

#if defined(CONFIG_FREERTOS_UNICORE)
    BaseType_t core = tskNO_AFFINITY;
#else
    BaseType_t core = 1;
#endif

    BaseType_t ret = xTaskCreatePinnedToCore(
        taskFunction,
        "espai_async",
        ESPAI_ASYNC_STACK_SIZE,
        params,
        ESPAI_ASYNC_TASK_PRIORITY,
        &_taskHandle,
        core
    );

    if (ret != pdPASS) {
        delete params;
        _request._status = AsyncStatus::Error;
        _request._result = Response::fail(ErrorCode::OutOfMemory, "Failed to create async task");
        if (onComplete) onComplete(_request._result);
        return false;
    }

    return true;
}

bool AsyncTaskRunner::launchStream(
    std::function<bool(StreamCallback)> streamTask,
    StreamCallback streamCb,
    AsyncDoneCallback onDone
) {
    if (isBusy()) return false;

    // If a previous task completed, consume its done semaphore before reuse
    if (_taskHandle != nullptr) {
        xSemaphoreTake(_taskDoneSemaphore, portMAX_DELAY);
        _taskHandle = nullptr;
    }

    _request._status = AsyncStatus::Running;
    _request._result = Response();
    _request._cancelRequested.store(false, std::memory_order_relaxed);
    _request._onComplete = onDone;
    _request._callbackInvoked = false;

    auto* params = new StreamTaskParams{this, std::move(streamTask), std::move(streamCb)};

#if defined(CONFIG_FREERTOS_UNICORE)
    BaseType_t core = tskNO_AFFINITY;
#else
    BaseType_t core = 1;
#endif

    BaseType_t ret = xTaskCreatePinnedToCore(
        streamTaskFunction,
        "espai_stream",
        ESPAI_ASYNC_STACK_SIZE,
        params,
        ESPAI_ASYNC_TASK_PRIORITY,
        &_taskHandle,
        core
    );

    if (ret != pdPASS) {
        delete params;
        _request._status = AsyncStatus::Error;
        _request._result = Response::fail(ErrorCode::OutOfMemory, "Failed to create stream task");
        if (onDone) onDone(_request._result);
        return false;
    }

    return true;
}

void AsyncTaskRunner::taskFunction(void* param) {
    auto* params = static_cast<TaskParams*>(param);
    AsyncTaskRunner* runner = params->runner;
    ChatRequest& req = runner->_request;

    Response result = params->task();
    delete params;

    req.lock();
    if (req._cancelRequested.load(std::memory_order_relaxed)) {
        req._status = AsyncStatus::Cancelled;
        req._result = Response::fail(ErrorCode::NetworkError, "Request cancelled");
    } else {
        req._status = result.success ? AsyncStatus::Completed : AsyncStatus::Error;
        req._result = result;
    }
    req.unlock();

    // Signal completion before self-deleting; destructor blocks on this
    runner->signalDone();
    vTaskDelete(nullptr);
}

void AsyncTaskRunner::streamTaskFunction(void* param) {
    auto* params = static_cast<StreamTaskParams*>(param);
    AsyncTaskRunner* runner = params->runner;
    ChatRequest& req = runner->_request;

    StreamCallback wrappedCb = [&req, params](const String& chunk, bool done) {
        if (req._cancelRequested.load(std::memory_order_relaxed)) return;
        params->streamCb(chunk, done);
    };

    bool success = params->streamTask(wrappedCb);
    delete params;

    req.lock();
    if (req._cancelRequested.load(std::memory_order_relaxed)) {
        req._status = AsyncStatus::Cancelled;
        req._result = Response::fail(ErrorCode::NetworkError, "Stream cancelled");
    } else {
        req._status = success ? AsyncStatus::Completed : AsyncStatus::Error;
        if (success) {
            req._result = Response::ok("");
        } else {
            req._result = Response::fail(ErrorCode::StreamingError, "Stream failed");
        }
    }
    req.unlock();

    // Signal completion before self-deleting; destructor blocks on this
    runner->signalDone();
    vTaskDelete(nullptr);
}

} // namespace ESPAI

#endif // ESPAI_ENABLE_ASYNC
