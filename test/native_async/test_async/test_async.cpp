// waitForCompletion() calls cancel() internally (designed for destructor shutdown).
// Tests that need non-cancelled completion must use spinWaitComplete() instead.

#ifdef NATIVE_TEST

#include <unity.h>
#include "async/AsyncTaskRunner.h"

#include <thread>
#include <chrono>
#include <atomic>

using namespace ESPAI;

void setUp(void) {}
void tearDown(void) {}

static bool spinWaitComplete(ChatRequest* req, int timeoutMs = 5000) {
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeoutMs);
    while (!req->isComplete()) {
        if (std::chrono::steady_clock::now() > deadline) return false;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    return true;
}

void test_initial_state() {
    AsyncTaskRunner runner;
    ChatRequest* req = runner.getRequest();

    TEST_ASSERT_NOT_NULL(req);
    TEST_ASSERT_EQUAL(AsyncStatus::Idle, req->getStatus());
    TEST_ASSERT_FALSE(runner.isBusy());
    TEST_ASSERT_FALSE(req->isComplete());
    TEST_ASSERT_FALSE(req->isCancelled());
}

void test_launch_and_complete() {
    AsyncTaskRunner runner;

    bool launched = runner.launch([]() {
        return Response::ok("hello");
    });

    TEST_ASSERT_TRUE(launched);

    ChatRequest* req = runner.getRequest();
    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req), "Task did not complete in time");

    TEST_ASSERT_TRUE(req->isComplete());
    TEST_ASSERT_EQUAL(AsyncStatus::Completed, req->getStatus());

    Response result = req->getResult();
    TEST_ASSERT_TRUE(result.success);
    TEST_ASSERT_EQUAL_STRING("hello", result.content.c_str());
}

void test_callback_invoked_via_poll() {
    AsyncTaskRunner runner;

    bool callbackFired = false;
    Response cbResult;

    runner.launch(
        []() { return Response::ok("callback_test"); },
        [&](const Response& r) {
            callbackFired = true;
            cbResult = r;
        }
    );

    ChatRequest* req = runner.getRequest();
    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req), "Task did not complete in time");

    // Callback should NOT fire until poll() is called
    TEST_ASSERT_FALSE(callbackFired);

    bool pollResult = req->poll();
    TEST_ASSERT_TRUE(pollResult);
    TEST_ASSERT_TRUE(callbackFired);
    TEST_ASSERT_EQUAL_STRING("callback_test", cbResult.content.c_str());
}

void test_poll_returns_false_while_running() {
    AsyncTaskRunner runner;

    runner.launch([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return Response::ok("delayed");
    });

    bool pollResult = runner.getRequest()->poll();
    TEST_ASSERT_FALSE(pollResult);
}

void test_cancel_request() {
    AsyncTaskRunner runner;

    runner.launch([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        return Response::ok("should_not_see");
    });

    runner.cancel();

    ChatRequest* req = runner.getRequest();
    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req, 5000), "Task did not complete after cancel");

    TEST_ASSERT_TRUE(req->isComplete());
    TEST_ASSERT_EQUAL(AsyncStatus::Cancelled, req->getStatus());
}

void test_busy_during_execution() {
    AsyncTaskRunner runner;

    runner.launch([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return Response::ok("busy_test");
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    TEST_ASSERT_TRUE(runner.isBusy());

    ChatRequest* req = runner.getRequest();
    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req), "Task did not complete in time");
    TEST_ASSERT_FALSE(runner.isBusy());
}

void test_double_launch_rejected() {
    AsyncTaskRunner runner;

    bool first = runner.launch([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return Response::ok("first");
    });
    TEST_ASSERT_TRUE(first);

    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    bool second = runner.launch([]() {
        return Response::ok("second");
    });
    TEST_ASSERT_FALSE(second);
}

void test_stream_launch() {
    AsyncTaskRunner runner;

    std::atomic<int> chunkCount{0};
    bool streamDone = false;

    StreamCallback streamCb = [&](const String& chunk, bool done) {
        if (!chunk.isEmpty()) {
            chunkCount++;
        }
        if (done) {
            streamDone = true;
        }
    };

    bool launched = runner.launchStream(
        [](StreamCallback cb) {
            cb("chunk1", false);
            cb("chunk2", false);
            cb("chunk3", true);
            return true;
        },
        streamCb
    );

    TEST_ASSERT_TRUE(launched);

    ChatRequest* req = runner.getRequest();
    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req), "Stream task did not complete in time");

    TEST_ASSERT_TRUE(req->isComplete());
    TEST_ASSERT_EQUAL(AsyncStatus::Completed, req->getStatus());
    TEST_ASSERT_EQUAL(3, chunkCount.load());
    TEST_ASSERT_TRUE(streamDone);
}

void test_stream_cancel() {
    AsyncTaskRunner runner;

    StreamCallback streamCb = [](const String& chunk, bool done) {
        (void)chunk;
        (void)done;
    };

    runner.launchStream(
        [](StreamCallback cb) {
            for (int i = 0; i < 50; i++) {
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
                cb(String("chunk"), false);
            }
            cb("", true);
            return true;
        },
        streamCb
    );

    std::this_thread::sleep_for(std::chrono::milliseconds(30));
    runner.cancel();

    ChatRequest* req = runner.getRequest();
    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req, 5000), "Stream did not complete after cancel");

    TEST_ASSERT_TRUE(req->isComplete());
    TEST_ASSERT_EQUAL(AsyncStatus::Cancelled, req->getStatus());
}

void test_callback_invoked_once() {
    AsyncTaskRunner runner;

    int callbackCount = 0;

    runner.launch(
        []() { return Response::ok("once_test"); },
        [&](const Response& r) {
            (void)r;
            callbackCount++;
        }
    );

    ChatRequest* req = runner.getRequest();
    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req), "Task did not complete in time");

    req->poll();
    TEST_ASSERT_EQUAL(1, callbackCount);

    req->poll();
    TEST_ASSERT_EQUAL(1, callbackCount);
}

void test_relaunch_after_complete() {
    AsyncTaskRunner runner;

    bool first = runner.launch([]() {
        return Response::ok("first_run");
    });
    TEST_ASSERT_TRUE(first);

    ChatRequest* req = runner.getRequest();
    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req), "First task did not complete in time");

    Response r1 = req->getResult();
    TEST_ASSERT_TRUE(r1.success);
    TEST_ASSERT_EQUAL_STRING("first_run", r1.content.c_str());

    // launch() internally consumes the done semaphore when _taskHandle
    // is non-null from a previous completed task
    bool second = runner.launch([]() {
        return Response::ok("second_run");
    });
    TEST_ASSERT_TRUE(second);

    TEST_ASSERT_TRUE_MESSAGE(spinWaitComplete(req), "Second task did not complete in time");

    Response r2 = req->getResult();
    TEST_ASSERT_TRUE(r2.success);
    TEST_ASSERT_EQUAL_STRING("second_run", r2.content.c_str());
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_initial_state);
    RUN_TEST(test_launch_and_complete);
    RUN_TEST(test_callback_invoked_via_poll);
    RUN_TEST(test_poll_returns_false_while_running);
    RUN_TEST(test_cancel_request);
    RUN_TEST(test_busy_during_execution);
    RUN_TEST(test_double_launch_rejected);
    RUN_TEST(test_stream_launch);
    RUN_TEST(test_stream_cancel);
    RUN_TEST(test_callback_invoked_once);
    RUN_TEST(test_relaunch_after_complete);

    return UNITY_END();
}

#else
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
