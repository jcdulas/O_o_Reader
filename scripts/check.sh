#!/usr/bin/env bash
# The checks of a change, natively: build, C++ tests, layering of the modules.
#
# Usage: scripts/check.sh [test name pattern]
#   On Windows, run it from the MSYS2 UCRT64 shell (or put C:\msys64\ucrt64\bin first on the PATH).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
JOBS="$(nproc 2>/dev/null || echo 4)"
TEST_JOBS=1

cmake -S "$ROOT" -B "$ROOT/build/release" -G Ninja -DCMAKE_BUILD_TYPE=Release -DOOR_TEST_TIMEOUT=1
cmake --build "$ROOT/build/release" -j "$JOBS"
if [ $# -gt 0 ]; then
    ctest --test-dir "$ROOT/build/release" -j "$TEST_JOBS" --timeout 1 --output-on-failure --no-tests=error -R "$1"
else
    ctest --test-dir "$ROOT/build/release" -j "$TEST_JOBS" --timeout 1 --output-on-failure --no-tests=error
fi
python "$ROOT/tools/check_layering.py"
