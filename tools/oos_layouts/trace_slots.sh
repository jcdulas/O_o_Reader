#!/usr/bin/env bash
# Traces the slots every card reader and writer uses while the C++ tests run, then generates the card layout catalogue
# from the traces (generate_layouts.py) and the card catalogue document (write_cards_doc.py).
#
# Usage: tools/oos_layouts/trace_slots.sh [trace folder]
#   On Windows, run it from the MSYS2 UCRT64 shell (or put C:\msys64\ucrt64\bin first on the PATH).
#   The reviewed layouts (review/) are kept: git diff shows what the new traces changed.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
TRACES="${1:-$ROOT/build/trace-slots}"
JOBS="$(nproc 2>/dev/null || echo 4)"

cmake -S "$ROOT" -B "$ROOT/build/trace" -G Ninja -DCMAKE_BUILD_TYPE=Release -DOOR_TRACE_SLOTS=ON
cmake --build "$ROOT/build/trace" -j "$JOBS"
rm -rf "$TRACES"
mkdir -p "$TRACES"
OOR_SLOT_TRACE_DIR="$TRACES" ctest --test-dir "$ROOT/build/trace" -j "$JOBS" --output-on-failure
python "$ROOT/tools/oos_layouts/generate_layouts.py" --traces "$TRACES"
python "$ROOT/tools/oos_layouts/write_cards_doc.py"
