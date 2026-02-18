/**
 * ESPAI Library - Error Handling Example
 *
 * This example demonstrates proper error handling:
 * - Checking response.success
 * - Handling different ErrorCode types
 * - Implementing retry logic with exponential backoff
 * - Graceful degradation strategies
 *
 * ErrorCodes:
 * - None: No error
 * - NetworkError: Connection failed
 * - Timeout: Request timed out
 * - AuthError: Invalid API key (401/403)
 * - RateLimited: Too many requests (429)
 * - InvalidRequest: Bad request (4xx)
 * - ServerError: Server error (5xx)
 * - ParseError: Failed to parse response
 * - OutOfMemory: Insufficient memory
 * - ProviderNotSupported: Provider not available
 * - NotConfigured: Provider not configured
 * - StreamingError: Streaming failed
 *
 * Hardware: ESP32 (any variant)
 *
 * Setup:
 * 1. Copy secrets.h.example to secrets.h
 * 2. Fill in your WiFi credentials and OpenAI API key
 * 3. Upload to your ESP32
 * 4. Open Serial Monitor at 115200 baud
 */

#include <WiFi.h>
#include <ESPAI.h>

#include "secrets.h"

using namespace ESPAI;

// Forward declarations
void connectToWiFi();
void demonstrateBasicErrorHandling();
void handleError(const Response& response);
Response sendWithRetry(const std::vector<Message>& messages, const ChatOptions& options);
void demonstrateRetryLogic();
void demonstrateErrorRecovery();
Response sendWithFallback();
void demonstrateUserFriendlyErrors();
String getHumanReadableError(const Response& response);

OpenAIProvider openai(OPENAI_API_KEY);

// Configuration for retry logic
const int MAX_RETRIES = 3;
const int BASE_RETRY_DELAY_MS = 1000;

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }

    Serial.println();
    Serial.println("=================================");
    Serial.println("ESPAI Error Handling Example");
    Serial.println("=================================");

    connectToWiFi();

    // Demonstrate error handling
    demonstrateBasicErrorHandling();
    demonstrateRetryLogic();
    demonstrateErrorRecovery();
}

void loop() {
    delay(10000);
}

void connectToWiFi() {
    Serial.print("Connecting to WiFi");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("\nFailed to connect to WiFi!");
        while (true) { delay(1000); }
    }

    Serial.println(" Connected!");
    Serial.println();
}

void demonstrateBasicErrorHandling() {
    Serial.println("BASIC ERROR HANDLING");
    Serial.println("----------------------------------------");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello!"));

    ChatOptions options;
    Response response = openai.chat(messages, options);

    // Always check success first
    if (response.success) {
        Serial.println("Request successful!");
        Serial.print("Response: ");
        Serial.println(response.content);
    } else {
        // Handle the error based on type
        handleError(response);
    }

    Serial.println();
}

void handleError(const Response& response) {
    Serial.print("Error occurred: ");
    Serial.println(errorCodeToString(response.error));
    Serial.print("HTTP Status: ");
    Serial.println(response.httpStatus);
    Serial.print("Message: ");
    Serial.println(response.errorMessage);

    // Take action based on error type
    switch (response.error) {
        case ErrorCode::AuthError:
            Serial.println("ACTION: Check your API key in secrets.h");
            Serial.println("        Make sure the key is valid and has not expired.");
            break;

        case ErrorCode::RateLimited:
            Serial.println("ACTION: Too many requests. Wait before retrying.");
            Serial.println("        Consider implementing request throttling.");
            break;

        case ErrorCode::NetworkError:
            Serial.println("ACTION: Check WiFi connection.");
            Serial.println("        Verify network connectivity.");
            break;

        case ErrorCode::Timeout:
            Serial.println("ACTION: Request timed out.");
            Serial.println("        Try increasing timeout or simplifying request.");
            break;

        case ErrorCode::InvalidRequest:
            Serial.println("ACTION: Check request parameters.");
            Serial.println("        Verify message format and options.");
            break;

        case ErrorCode::ServerError:
            Serial.println("ACTION: OpenAI server error.");
            Serial.println("        Retry after a short delay.");
            break;

        case ErrorCode::ParseError:
            Serial.println("ACTION: Failed to parse response.");
            Serial.println("        This may indicate an API change or corruption.");
            break;

        case ErrorCode::OutOfMemory:
            Serial.println("ACTION: Not enough memory.");
            Serial.println("        Reduce message size or maxTokens.");
            break;

        default:
            Serial.println("ACTION: Unexpected error. Check logs.");
            break;
    }
}

// Send request with retry logic and exponential backoff
Response sendWithRetry(const std::vector<Message>& messages, const ChatOptions& options) {
    Response response;

    for (int attempt = 1; attempt <= MAX_RETRIES; attempt++) {
        Serial.print("Attempt ");
        Serial.print(attempt);
        Serial.print("/");
        Serial.print(MAX_RETRIES);
        Serial.println("...");

        response = openai.chat(messages, options);

        if (response.success) {
            Serial.println("Success!");
            return response;
        }

        // Check if error is retryable
        bool shouldRetry = false;
        int retryDelay = BASE_RETRY_DELAY_MS * (1 << (attempt - 1));  // Exponential backoff

        switch (response.error) {
            case ErrorCode::NetworkError:
            case ErrorCode::Timeout:
            case ErrorCode::ServerError:
                // These errors are transient and worth retrying
                shouldRetry = true;
                break;

            case ErrorCode::RateLimited:
                // Rate limited - wait longer
                shouldRetry = true;
                retryDelay = 5000 * attempt;  // Longer delay for rate limits
                break;

            case ErrorCode::AuthError:
            case ErrorCode::InvalidRequest:
            case ErrorCode::ProviderNotSupported:
            case ErrorCode::NotConfigured:
                // These errors won't be fixed by retrying
                shouldRetry = false;
                break;

            default:
                shouldRetry = attempt < MAX_RETRIES;
                break;
        }

        if (!shouldRetry || attempt == MAX_RETRIES) {
            Serial.println("Not retrying - error is not recoverable or max retries reached.");
            break;
        }

        Serial.print("Retrying in ");
        Serial.print(retryDelay);
        Serial.println(" ms...");
        delay(retryDelay);
    }

    return response;
}

void demonstrateRetryLogic() {
    Serial.println("RETRY LOGIC WITH EXPONENTIAL BACKOFF");
    Serial.println("----------------------------------------");

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "What is 1+1?"));

    ChatOptions options;
    options.maxTokens = 10;

    Response response = sendWithRetry(messages, options);

    if (response.success) {
        Serial.print("Final response: ");
        Serial.println(response.content);
    } else {
        Serial.println("All retries failed.");
        handleError(response);
    }

    Serial.println();
}

void demonstrateErrorRecovery() {
    Serial.println("GRACEFUL DEGRADATION STRATEGIES");
    Serial.println("----------------------------------------");

    // Strategy 1: Fallback to simpler request
    Serial.println("Strategy 1: Reduce complexity on failure");
    Response response = sendWithFallback();
    if (response.success) {
        Serial.print("Response: ");
        Serial.println(response.content);
    }

    Serial.println();

    // Strategy 2: Cache and offline mode (conceptual)
    Serial.println("Strategy 2: Use cached response on failure");
    Serial.println("(In production, you would store successful responses");
    Serial.println(" in NVS and use them when network is unavailable)");

    Serial.println();

    // Strategy 3: Graceful error message to user
    Serial.println("Strategy 3: User-friendly error handling");
    demonstrateUserFriendlyErrors();
}

Response sendWithFallback() {
    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Explain quantum computing in detail."));

    ChatOptions options;
    options.maxTokens = 500;

    Response response = openai.chat(messages, options);

    if (!response.success && (response.error == ErrorCode::Timeout ||
                               response.error == ErrorCode::OutOfMemory)) {
        Serial.println("Complex request failed. Trying simpler request...");

        // Fallback to simpler request
        messages.clear();
        messages.push_back(Message(Role::User, "What is quantum computing? One sentence."));
        options.maxTokens = 50;

        response = openai.chat(messages, options);
    }

    return response;
}

void demonstrateUserFriendlyErrors() {
    // In a real application, you would display these to the user
    // via LCD, LED patterns, or audio feedback

    std::vector<Message> messages;
    messages.push_back(Message(Role::User, "Hello!"));

    ChatOptions options;
    Response response = openai.chat(messages, options);

    String userMessage = getHumanReadableError(response);
    Serial.print("User message: ");
    Serial.println(userMessage);
}

String getHumanReadableError(const Response& response) {
    if (response.success) {
        return "Request completed successfully.";
    }

    switch (response.error) {
        case ErrorCode::NetworkError:
            return "Cannot connect to the internet. Please check your connection.";

        case ErrorCode::Timeout:
            return "The request is taking too long. Please try again.";

        case ErrorCode::AuthError:
            return "Authentication failed. Please check configuration.";

        case ErrorCode::RateLimited:
            return "Too many requests. Please wait a moment and try again.";

        case ErrorCode::ServerError:
            return "The AI service is temporarily unavailable. Please try later.";

        case ErrorCode::OutOfMemory:
            return "Device is low on memory. Restarting may help.";

        default:
            return "An error occurred. Please try again.";
    }
}
