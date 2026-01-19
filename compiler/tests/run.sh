#!/bin/sh
set -eu

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
BIN="$ROOT_DIR/minac"

if [ ! -x "$BIN" ]; then
  echo "minac binary not found. Run 'make' in compiler/." >&2
  exit 1
fi

# c0_smoke.c: should succeed
"$BIN" "$ROOT_DIR/tests/c0_smoke.c" >/dev/null
echo "PASS c0_smoke"

# c0_usage.c: invalid args should fail
if "$BIN" >/dev/null 2>&1; then
  echo "FAIL c0_usage" >&2
  exit 1
fi

echo "PASS c0_usage"

# c1_expr.c: should parse
"$BIN" "$ROOT_DIR/tests/c1_expr.c" >/dev/null
echo "PASS c1_expr"

# c1_paren.c: should parse
"$BIN" "$ROOT_DIR/tests/c1_paren.c" >/dev/null
echo "PASS c1_paren"

# c1_err.c: should fail with error
if "$BIN" "$ROOT_DIR/tests/c1_err.c" >/dev/null 2>&1; then
  echo "FAIL c1_err" >&2
  exit 1
fi
echo "PASS c1_err"

# c2_ir.c: should lower and match IR output
"$BIN" --emit-ir "$ROOT_DIR/tests/c2_ir.c" > "$ROOT_DIR/tests/c2_ir.out"
if ! cmp -s "$ROOT_DIR/tests/c2_ir.out" "$ROOT_DIR/tests/c2_ir.txt"; then
  echo "FAIL c2_ir" >&2
  exit 1
fi
rm -f "$ROOT_DIR/tests/c2_ir.out"
echo "PASS c2_ir"

# c2_ir_err.c: should fail in lowering
if "$BIN" --emit-ir "$ROOT_DIR/tests/c2_ir_err.c" >/dev/null 2>&1; then
  echo "FAIL c2_ir_err" >&2
  exit 1
fi
echo "PASS c2_ir_err"
