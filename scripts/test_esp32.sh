#!/bin/bash
# Run ESP32 embedded tests

set -e

# Remove Unity examples that break compilation
rm -rf .pio/libdeps/*/Unity/examples 2>/dev/null || true

# Run ESP32 tests (default: esp32s3_test)
ENV="${1:-esp32s3_test}"
shift 2>/dev/null || true

pio test -e "$ENV" -f "embedded/*" -v "$@"
