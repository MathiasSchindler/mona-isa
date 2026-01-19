#!/bin/sh
set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BIN="$ROOT_DIR/minac"

if [ ! -x "$BIN" ]; then
  echo "minac binary not found. Run 'make' in compiler/." >&2
  exit 1
fi

# c0_smoke.c: should succeed and print stub message
OUT=$("$BIN" "$ROOT_DIR/tests/c0_smoke.c")
case "$OUT" in
  *"not implemented yet"*)
    echo "PASS c0_smoke"
    ;;
  *)
    echo "FAIL c0_smoke" >&2
    exit 1
    ;;
esac

# c0_usage.c: invalid args should fail
if "$BIN" >/dev/null 2>&1; then
  echo "FAIL c0_usage" >&2
  exit 1
fi

echo "PASS c0_usage"
