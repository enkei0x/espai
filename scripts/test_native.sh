#!/bin/bash
# Run native (host) tests

set -e

# Remove Unity examples that break compilation
rm -rf .pio/libdeps/*/Unity/examples 2>/dev/null || true

# Run native tests
pio test -e native -f "native/*" -v "$@"

# Run native async tests
pio test -e native_async -f "native_async/*" -v "$@"
