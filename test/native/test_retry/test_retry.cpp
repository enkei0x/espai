#ifdef NATIVE_TEST

#include <unity.h>
#include "core/AITypes.h"
#include "providers/AIProvider.h"

using namespace ESPAI;

class TestableProvider : public AIProvider {
public:
    const char* getName() const override { return "Test"; }
    Provider getType() const override { return Provider::Custom; }

#if ESPAI_ENABLE_STREAMING
    SSEFormat getSSEFormat() const override { return SSEFormat::OpenAI; }
#endif

    String buildRequestBody(
        const std::vector<Message>&,
        const ChatOptions&
    ) override {
        return "{}";
    }

    Response parseResponse(const String&) override {
        return Response::ok("ok");
    }

#if ESPAI_ENABLE_TOOLS
    Message getAssistantMessageWithToolCalls(const String& content = "") const override {
        return Message(Role::Assistant, content);
    }
#endif

    using AIProvider::isRetryableStatus;
    using AIProvider::calculateRetryDelay;
};

void setUp() {}
void tearDown() {}

void test_retry_config_defaults() {
    RetryConfig config;
    TEST_ASSERT_FALSE(config.enabled);
    TEST_ASSERT_EQUAL(3, config.maxRetries);
    TEST_ASSERT_EQUAL(1000, config.initialDelayMs);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 2.0f, config.backoffMultiplier);
    TEST_ASSERT_EQUAL(30000, config.maxDelayMs);
}

void test_retry_config_set_get() {
    TestableProvider provider;

    RetryConfig config;
    config.enabled = true;
    config.maxRetries = 5;
    config.initialDelayMs = 500;
    config.backoffMultiplier = 3.0f;
    config.maxDelayMs = 60000;

    provider.setRetryConfig(config);

    const RetryConfig& got = provider.getRetryConfig();
    TEST_ASSERT_TRUE(got.enabled);
    TEST_ASSERT_EQUAL(5, got.maxRetries);
    TEST_ASSERT_EQUAL(500, got.initialDelayMs);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 3.0f, got.backoffMultiplier);
    TEST_ASSERT_EQUAL(60000, got.maxDelayMs);
}

void test_retryable_status_429() {
    TEST_ASSERT_TRUE(TestableProvider::isRetryableStatus(429));
}

void test_retryable_status_500() {
    TEST_ASSERT_TRUE(TestableProvider::isRetryableStatus(500));
}

void test_retryable_status_502() {
    TEST_ASSERT_TRUE(TestableProvider::isRetryableStatus(502));
}

void test_retryable_status_503() {
    TEST_ASSERT_TRUE(TestableProvider::isRetryableStatus(503));
}

void test_retryable_status_599() {
    TEST_ASSERT_TRUE(TestableProvider::isRetryableStatus(599));
}

void test_not_retryable_status_200() {
    TEST_ASSERT_FALSE(TestableProvider::isRetryableStatus(200));
}

void test_not_retryable_status_400() {
    TEST_ASSERT_FALSE(TestableProvider::isRetryableStatus(400));
}

void test_not_retryable_status_401() {
    TEST_ASSERT_FALSE(TestableProvider::isRetryableStatus(401));
}

void test_not_retryable_status_403() {
    TEST_ASSERT_FALSE(TestableProvider::isRetryableStatus(403));
}

void test_not_retryable_status_404() {
    TEST_ASSERT_FALSE(TestableProvider::isRetryableStatus(404));
}

void test_not_retryable_status_600() {
    TEST_ASSERT_FALSE(TestableProvider::isRetryableStatus(600));
}

void test_calculate_delay_attempt_0() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 30000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 0, -1);
    TEST_ASSERT_EQUAL(1000, delay);
}

void test_calculate_delay_attempt_1() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 30000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 1, -1);
    TEST_ASSERT_EQUAL(2000, delay);
}

void test_calculate_delay_attempt_2() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 30000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 2, -1);
    TEST_ASSERT_EQUAL(4000, delay);
}

void test_calculate_delay_attempt_3() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 30000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 3, -1);
    TEST_ASSERT_EQUAL(8000, delay);
}

void test_calculate_delay_capped_at_max() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 5000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 10, -1);
    TEST_ASSERT_EQUAL(5000, delay);
}

void test_calculate_delay_retry_after_header() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 30000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 0, 5);
    TEST_ASSERT_EQUAL(5000, delay);
}

void test_calculate_delay_retry_after_capped_at_max() {
    RetryConfig config;
    config.maxDelayMs = 10000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 0, 60);
    TEST_ASSERT_EQUAL(10000, delay);
}

void test_calculate_delay_negative_retry_after_uses_backoff() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 30000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 2, -1);
    TEST_ASSERT_EQUAL(4000, delay);
}

void test_calculate_delay_zero_retry_after_uses_backoff() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 30000;

    uint32_t delay = TestableProvider::calculateRetryDelay(config, 1, 0);
    TEST_ASSERT_EQUAL(2000, delay);
}

void test_calculate_delay_custom_multiplier() {
    RetryConfig config;
    config.initialDelayMs = 500;
    config.backoffMultiplier = 3.0f;
    config.maxDelayMs = 60000;

    TEST_ASSERT_EQUAL(500, TestableProvider::calculateRetryDelay(config, 0, -1));
    TEST_ASSERT_EQUAL(1500, TestableProvider::calculateRetryDelay(config, 1, -1));
    TEST_ASSERT_EQUAL(4500, TestableProvider::calculateRetryDelay(config, 2, -1));
    TEST_ASSERT_EQUAL(13500, TestableProvider::calculateRetryDelay(config, 3, -1));
}

void test_calculate_delay_large_attempt_no_overflow() {
    RetryConfig config;
    config.initialDelayMs = 1000;
    config.backoffMultiplier = 2.0f;
    config.maxDelayMs = 30000;

    // attempt=50 would cause powf to exceed uint32_t max, must not overflow
    uint32_t delay = TestableProvider::calculateRetryDelay(config, 50, -1);
    TEST_ASSERT_EQUAL(30000, delay);
}

void test_http_response_retry_after_default() {
    HttpResponse resp;
    TEST_ASSERT_EQUAL(-1, resp.retryAfterSeconds);
}

void test_provider_retry_disabled_by_default() {
    TestableProvider provider;
    TEST_ASSERT_FALSE(provider.getRetryConfig().enabled);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    UNITY_BEGIN();

    RUN_TEST(test_retry_config_defaults);
    RUN_TEST(test_retry_config_set_get);

    RUN_TEST(test_retryable_status_429);
    RUN_TEST(test_retryable_status_500);
    RUN_TEST(test_retryable_status_502);
    RUN_TEST(test_retryable_status_503);
    RUN_TEST(test_retryable_status_599);

    RUN_TEST(test_not_retryable_status_200);
    RUN_TEST(test_not_retryable_status_400);
    RUN_TEST(test_not_retryable_status_401);
    RUN_TEST(test_not_retryable_status_403);
    RUN_TEST(test_not_retryable_status_404);
    RUN_TEST(test_not_retryable_status_600);

    RUN_TEST(test_calculate_delay_attempt_0);
    RUN_TEST(test_calculate_delay_attempt_1);
    RUN_TEST(test_calculate_delay_attempt_2);
    RUN_TEST(test_calculate_delay_attempt_3);
    RUN_TEST(test_calculate_delay_capped_at_max);
    RUN_TEST(test_calculate_delay_retry_after_header);
    RUN_TEST(test_calculate_delay_retry_after_capped_at_max);
    RUN_TEST(test_calculate_delay_negative_retry_after_uses_backoff);
    RUN_TEST(test_calculate_delay_zero_retry_after_uses_backoff);
    RUN_TEST(test_calculate_delay_custom_multiplier);
    RUN_TEST(test_calculate_delay_large_attempt_no_overflow);

    RUN_TEST(test_http_response_retry_after_default);
    RUN_TEST(test_provider_retry_disabled_by_default);

    return UNITY_END();
}

#else
void setUp(void) {}
void tearDown(void) {}
int main(void) { return 0; }
#endif
