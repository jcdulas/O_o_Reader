#!/usr/bin/env bash
# Writes every model the C++ tests read (through readModelText) as a model file and reads it back, and lists the
# models whose card tables do not come back the same or whose model file is not a fixed point (tests/cpp/support/
# ModelFileRoundTrip.hpp). The validation decks and the sample cards have tests of their own ([model-file]).
#
# Usage: tools/oos_layouts/round_trip_tests.sh [log file]
#   Needs the native build of scripts/check.sh (build/release).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
LOG="${1:-$ROOT/build/model-file-round-trip.log}"
JOBS="$(nproc 2>/dev/null || echo 4)"

rm -f "$LOG"
OOR_MODEL_FILE_ROUND_TRIP_LOG="$LOG" ctest --test-dir "$ROOT/build/release" -j "$JOBS" > /dev/null || true
if [ ! -s "$LOG" ]; then
    echo "every model read by the tests went through a model file and back unchanged"
    exit 0
fi
echo "models that did not come back the same: $(grep -c '^=== model:' "$LOG") (details in $LOG)"
grep -A3 '^--- problem:' "$LOG" | grep -v '^--- problem:\|^--$' | sort | uniq -c | sort -rn | head -40
exit 1
