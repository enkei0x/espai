# 🔧 Troubleshooting

Common issues and how to fix them.

---

## 🔴 Connection Errors

### "Connection failed" / "Network error"

**Symptoms:**
- `response.error == ErrorCode::NetworkError`
- HTTP status 0 or -1

**Solutions:**

1. **Check WiFi connection**
   ```cpp
   if (WiFi.status() != WL_CONNECTED) {
       Serial.println("WiFi not connected!");
   }
   ```

2. **Verify internet access**
   ```cpp
   // Try pinging Google DNS
   if (WiFi.ping("8.8.8.8") > 0) {
       Serial.println("Internet OK");
   }
   ```

3. **Check HTTPS/SSL**
   - ESP32 needs enough memory for SSL (~40KB)
   - Try increasing heap if low on memory

4. **Firewall/proxy issues**
   - Some networks block API endpoints
   - Try a different network

---

## 🔑 Authentication Errors

### "Invalid API key" / "Authentication error"

**Symptoms:**
- `response.error == ErrorCode::AuthError`
- HTTP status 401

**Solutions:**

1. **Verify API key**
   - OpenAI: Starts with `sk-`
   - Anthropic: Starts with `sk-ant-`

2. **Check for typos/whitespace**
   ```cpp
   // ❌ Wrong - has space
   const char* key = "sk-abc123 ";

   // ✅ Correct
   const char* key = "sk-abc123";
   ```

3. **Check key permissions**
   - OpenAI: [API Keys page](https://platform.openai.com/api-keys)
   - Anthropic: [Console](https://console.anthropic.com/)

4. **Check billing**
   - Free tier may be exhausted
   - Payment method may be expired

---

## ⏱️ Timeout Errors

### "Request timeout"

**Symptoms:**
- `response.error == ErrorCode::Timeout`
- Long wait before error

**Solutions:**

1. **Increase timeout**
   ```cpp
   ai.setTimeout(60000);  // 60 seconds
   ```

2. **Reduce response length**
   ```cpp
   ChatOptions opts;
   opts.maxTokens = 100;  // Shorter response
   ```

3. **Check network stability**
   - Weak WiFi signal
   - High latency network

4. **Use streaming for long responses**
   ```cpp
   // Streaming has progressive output, feels faster
   ai.chatStream(messages, opts, callback);
   ```

---

## 💾 Memory Issues

### "Out of memory" / Crashes / Resets

**Symptoms:**
- ESP32 resets unexpectedly
- `heap_caps_get_free_size()` returns low value

**Solutions:**

1. **Check free heap**
   ```cpp
   Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
   // Need at least 50KB free for SSL
   ```

2. **Reduce response size**
   ```cpp
   ChatOptions opts;
   opts.maxTokens = 256;  // Limit response length
   ```

3. **Use streaming**
   ```cpp
   // Process chunks instead of buffering full response
   ai.chatStream(messages, opts, [](const String& chunk, bool done) {
       Serial.print(chunk);
   });
   ```

4. **Clear conversation history**
   ```cpp
   conv.setMaxMessages(10);  // Limit history
   conv.clear();             // Clear when done
   ```

5. **Use PSRAM if available (ESP32-S3)**
   ```cpp
   // In platformio.ini
   build_flags = -DBOARD_HAS_PSRAM
   ```

---

## ⚡ Rate Limiting

### "Rate limit exceeded" / "Too many requests"

**Symptoms:**
- `response.error == ErrorCode::RateLimited`
- HTTP status 429

**Solutions:**

1. **Add delay between requests**
   ```cpp
   delay(1000);  // Wait 1 second between requests
   ```

2. **Implement exponential backoff**
   ```cpp
   int retryDelay = 1000;
   for (int i = 0; i < 3; i++) {
       Response resp = ai.chat(msgs, opts);
       if (resp.success) break;

       if (resp.error == ErrorCode::RateLimited) {
           delay(retryDelay);
           retryDelay *= 2;  // Double delay each retry
       }
   }
   ```

3. **Upgrade API plan**
   - Free tiers have low limits
   - Paid plans have higher limits

---

## 📝 Parse Errors

### "Invalid JSON" / "Parse error"

**Symptoms:**
- `response.error == ErrorCode::ParseError`

**Solutions:**

1. **Check response content**
   ```cpp
   Serial.println("Raw response:");
   Serial.println(response.content);
   ```

2. **API may be returning HTML (error page)**
   - Check HTTP status code
   - May indicate endpoint issue

3. **Response too large**
   - ArduinoJson has limits
   - Reduce `maxTokens`

---

## 🛠️ Tool Calling Issues

### "messages with role 'tool' must be a response to a preceeding message with 'tool_calls'"

**Solution:** Include assistant message with tool_calls before tool results:

```cpp
// After getting tool calls
messages.push_back(ai.getAssistantMessageWithToolCalls(response.content));

// Then add tool results
for (const auto& call : ai.getLastToolCalls()) {
    messages.push_back(Message(Role::Tool, result, call.id));
}
```

### Tools not being called

**Solutions:**

1. **Improve tool description**
   ```cpp
   // ❌ Vague
   tool.description = "Temperature";

   // ✅ Clear
   tool.description = "Get the current room temperature in Celsius from the sensor";
   ```

2. **Make message clear**
   ```cpp
   // ❌ Indirect
   "Is it cold?"

   // ✅ Direct
   "What is the current temperature?"
   ```

---

## 🔍 Debug Tips

### Enable verbose output

```cpp
Response resp = ai.chat(messages, opts);

Serial.println("=== Debug Info ===");
Serial.printf("Success: %s\n", resp.success ? "true" : "false");
Serial.printf("HTTP Status: %d\n", resp.httpStatus);
Serial.printf("Error: %d\n", (int)resp.error);
Serial.printf("Error Message: %s\n", resp.errorMessage.c_str());
Serial.printf("Content length: %d\n", resp.content.length());
Serial.printf("Tokens: %d prompt, %d completion\n",
    resp.promptTokens, resp.completionTokens);
```

### Check memory before/after

```cpp
Serial.printf("Heap before: %d\n", ESP.getFreeHeap());
Response resp = ai.chat(messages, opts);
Serial.printf("Heap after: %d\n", ESP.getFreeHeap());
```

### Test with minimal code

```cpp
// Simplest possible test
std::vector<Message> msgs;
msgs.push_back(Message(Role::User, "Say OK"));

ChatOptions opts;
opts.maxTokens = 5;

Response resp = ai.chat(msgs, opts);
Serial.println(resp.success ? resp.content : resp.errorMessage);
```

---

## 📞 Getting Help

If you're still stuck:

1. **Check examples** - Working code in `examples/` folder
2. **Search issues** - [GitHub Issues](https://github.com/enkei0x/espai/issues)
3. **Open new issue** - Include:
   - ESP32 board type
   - Error message
   - Minimal code to reproduce
   - Serial output
