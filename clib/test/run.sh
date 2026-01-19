#!/bin/sh
set -eu

ROOT_DIR=$(cd "$(dirname "$0")/../.." && pwd)
BIN="$ROOT_DIR/compiler/minac"
AS="$ROOT_DIR/mina-as/mina-as"
SIM="$ROOT_DIR/simulator/mina-sim"
OUT_ELF="$ROOT_DIR/out/elf"
OUT_TMP="$ROOT_DIR/out/tmp"
CLIB_SRC="$ROOT_DIR/clib/src/clib.c"

mkdir -p "$OUT_ELF" "$OUT_TMP"

if [ ! -x "$BIN" ]; then
  echo "minac binary not found. Run 'make' in compiler/." >&2
  exit 1
fi

if [ ! -x "$AS" ]; then
  make -s -C "$ROOT_DIR/mina-as"
fi

if [ ! -x "$SIM" ]; then
  make -s -C "$ROOT_DIR/simulator"
fi

"$BIN" --emit-asm --no-start "$CLIB_SRC" > "$OUT_TMP/clib.s"
# avoid label collisions when concatenating
sed 's/\.L/\.Lclib/g' "$OUT_TMP/clib.s" > "$OUT_TMP/clib.lib.s"

compile_and_run() {
  name="$1"
  src="$2"
  expect="$3"
  expect2="${4:-}"

  "$BIN" --emit-asm "$src" > "$OUT_TMP/$name.s"
  cat "$OUT_TMP/$name.s" "$OUT_TMP/clib.lib.s" > "$OUT_TMP/${name}_full.s"
  "$AS" --data-base 0x4000 "$OUT_TMP/${name}_full.s" -o "$OUT_ELF/$name.elf"
  OUT_LOG=$("$SIM" "$OUT_ELF/$name.elf" 2>&1 || true)
  echo "$OUT_LOG" | grep -q "$expect" || { echo "FAIL $name" >&2; echo "$OUT_LOG" >&2; exit 1; }
  if [ -n "$expect2" ]; then
    echo "$OUT_LOG" | grep -q "$expect2" || { echo "FAIL $name" >&2; echo "$OUT_LOG" >&2; exit 1; }
  fi
  echo "$OUT_LOG" | grep -q "halted on ebreak" || { echo "FAIL $name" >&2; echo "$OUT_LOG" >&2; exit 1; }
  echo "PASS $name"
}

compile_and_run "clib_sieve" "$ROOT_DIR/clib/test/sieve.c" "count=25" "last=97"
compile_and_run "clib_pi" "$ROOT_DIR/clib/test/monte_carlo_pi.c" "pi~3."
compile_and_run "clib_hanoi" "$ROOT_DIR/clib/test/hanoi.c" "move 1 A C"
