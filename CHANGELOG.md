# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.8.0] - 2026-02-21

### Added
- Async API with FreeRTOS (`chatAsync`, `chatStreamAsync`, `ChatRequest`, `AsyncTaskRunner`)
- `AIClient` high-level convenience wrapper with simplified string-based API
- `GeminiProvider` for Google Gemini models with thinking budget support
- `OpenAICompatibleProvider` base class for custom OpenAI-compatible APIs (Groq, DeepSeek, Together AI, LM Studio, OpenRouter, etc.)
- `OllamaProvider` for local LLMs via Ollama
- Thread-safe HTTP transport with FreeRTOS mutexes
- Async examples (`AsyncChat`, `AnthropicChat`, `GeminiChat`)
- Async documentation (`docs/async.md`)
- `ESPAI_ENABLE_ASYNC`, `ESPAI_ASYNC_STACK_SIZE`, `ESPAI_ASYNC_TASK_PRIORITY` configuration defines
- ESP32-S2 board support

### Changed
- Updated documentation to reflect all current features and correct API signatures
- Expanded `keywords.txt` with all provider, tool, and async keywords
- Updated test count badge to 457+

### Fixed
- `ChatOptions::maxTokens` default documented as 0, actually 1024
- `Conversation::addMessage()` signature documented incorrectly
- `Response` field types documented as `int`, actually `int16_t`/`uint32_t`
- `Response::fail()` missing optional third parameter in docs
- SSL certificate description (GlobalSign + GTS Root R1, not DigiCert + ISRG)
- Missing `ErrorCode` values in docs (`OutOfMemory`, `StreamingError`, `ResponseTooLarge`)
- Missing `GeminiProvider` section in API reference

## [0.7.0] - 2026-02-20

### Added
- `RetryConfig` with exponential backoff for automatic request retrying
- Unified `AIProvider` base class with shared `chat()` and `chatStream()` implementation
- Tool call management in `AIProvider` (`addTool`, `clearTools`, `hasToolCalls`, `getLastToolCalls`)
- Enhanced `SSEParser` with provider-specific streaming formats
- `HttpResponse` struct with `responseTooLarge` and `retryAfterSeconds` fields
- `ESPAI_MAX_RESPONSE_SIZE` configuration define

### Changed
- Refactored streaming handling and SSEParser integration across all providers
- Updated default model references to `gpt-4.1-mini`

### Fixed
- SSL/TLS certificate validation with embedded root CA certificates
- Connection security for ESP32 HTTP transport

## [0.6.1] - 2026-02-18

### Fixed
- PlatformIO installation instructions in documentation

## [0.6.0] - 2026-02-18

### Added
- Initial release
- `OpenAIProvider` for OpenAI GPT models
- `AnthropicProvider` for Anthropic Claude models
- SSE streaming support with `chatStream()`
- `Conversation` class for multi-turn history with auto-pruning and JSON serialization
- Tool/function calling with unified `Tool`, `ToolCall`, `ToolResult` structs
- `ToolRegistry` for standalone tool management and execution
- `ProviderFactory` for runtime provider instantiation
- `HttpTransportESP32` with SSL/TLS support
- Conditional compilation for features and providers
- 400+ native unit tests
- Examples: `BasicChat`, `StreamingChat`, `ConversationHistory`, `ToolCalling`, `CustomOptions`, `ErrorHandling`
- Documentation: API reference, providers guide, streaming guide, tool calling guide, troubleshooting

[Unreleased]: https://github.com/enkei0x/espai/compare/v0.8.0...HEAD
[0.8.0]: https://github.com/enkei0x/espai/compare/v0.7.0...v0.8.0
[0.7.0]: https://github.com/enkei0x/espai/compare/v0.6.1...v0.7.0
[0.6.1]: https://github.com/enkei0x/espai/compare/v0.6.0...v0.6.1
[0.6.0]: https://github.com/enkei0x/espai/releases/tag/v0.6.0
