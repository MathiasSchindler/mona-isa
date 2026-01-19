# C Library Plan (MINA Target)

Goal: Provide a small, growing C standard library layer for `minac`, kept in its own folder and **optional** to link. The compiler must keep working without the library so users can write syscall-only programs.

Assumptions:
- Output runs in the MINA simulator.
- Library code lives under its own top-level folder (e.g., `clib/`), separate from `compiler/` and `mina-as/`.
- The compiler remains usable without linking the library (syscall-only programs still compile/run).
- Tests live under `compiler/tests/` and are run via `compiler/tests/run.sh`.

## Milestone L0 — Baseline I/O Intrinsics
**Scope**
- Keep existing builtins: `putchar`, `puts`, `exit`.
- Document behavior and limitations.

**Implementation Steps**
1. Ensure `putchar`, `puts`, `exit` are available as intrinsics.
2. Document return values and side effects.
3. Add tests to verify output and exit status.

**Tests**
- `tests/l0_putchar.c`: prints two characters and exits 0.
- `tests/l0_puts.c`: prints a string and exits 0.
- `tests/l0_exit.c`: exits with non-zero code.

## Milestone L1 — Tiny libc header + prototypes (separate folder)
**Scope**
- Provide a `clib.h` with prototypes for supported functions.
- No code changes yet; this is to standardize signatures.

**Implementation Steps**
1. Add `clib/include/clib.h` with `int putchar(int); int puts(const char*); void exit(int);`.
2. Teach preprocessor (C13) to include it when explicitly requested.
3. Update tests to include `#include "clib.h"`.

**Tests**
- `tests/l1_header.c`: compile with `#include "clib.h"` and call functions.

## Milestone L2 — String length and memory utilities (library object)
**Scope**
- Implement `strlen`, `memcpy`, `memset` in a real library object.

**Implementation Steps**
1. Add `clib/src/clib.c` implementing functions in portable C.
2. Add a build step to compile `clib/src/clib.c` to an object and **optionally** link with programs.
3. Keep compiler intrinsics working even if the library is not linked.

**Tests**
- `tests/l2_strlen.c`: verify `strlen("abc") == 3`.
- `tests/l2_memcpy.c`: copy a buffer and compare bytes.
- `tests/l2_memset.c`: fill a buffer and verify bytes.

## Milestone L3 — Basic formatted output (minimal)
**Scope**
- Implement `printf` subset: `%s`, `%c`, `%d`.

**Implementation Steps**
1. Add `vprintf` helper and `printf` wrapper.
2. Implement integer formatting without heap allocations.
3. Use `putchar` internally.

**Tests**
- `tests/l3_printf.c`: check output for string, char, and integer.

## Milestone L4 — Character classification
**Scope**
- Implement a minimal `ctype` subset: `isdigit`, `isalpha`, `isspace`.

**Implementation Steps**
1. Add `ctype` functions to `clib.c` and prototypes in `clib.h`.
2. Keep them ASCII-only.

**Tests**
- `tests/l4_ctype.c`: verify expected true/false cases.

## Milestone L5 — Simple conversion helpers
**Scope**
- Implement `atoi` and `strtol` (subset: base 10, no errno).

**Implementation Steps**
1. Implement parsing with optional leading whitespace and sign.
2. Document overflow behavior (wrap or clamp).

**Tests**
- `tests/l5_atoi.c`: parse common values and signs.

## Milestone L6 — File-descriptor style I/O (optional)
**Scope**
- Implement `write(int fd, const void* buf, size_t len)` and `read` if supported.

**Implementation Steps**
1. Map to simulator syscalls or MMIO.
2. Keep it minimal; only `fd=1/2` guaranteed.

**Tests**
- `tests/l6_write.c`: write to stdout and verify output.

## Milestone L7 — Heap allocation (optional)
**Scope**
- Implement a tiny `malloc`/`free` using a bump allocator or freelist.

**Implementation Steps**
1. Add a simple heap region in the runtime.
2. Provide `malloc`, `free`, `calloc`, `realloc` (optional).
3. Document limitations and fragmentation.

**Tests**
- `tests/l7_malloc.c`: allocate, write, and free memory.

## Milestone L8 — Prefer library calls (intrinsics remain optional)
**Scope**
- Ensure `putchar`, `puts`, `exit` are real functions in the library.

**Implementation Steps**
1. Implement `putchar`, `puts`, `exit` in `clib/src/clib.c`.
2. Update compiler to **optionally** emit calls when the library is linked; keep intrinsic fallback when not linked.
3. Keep tests passing for both modes.

**Tests**
- Re-run all library tests to ensure ABI stability.

## Milestone L9 — Integration polish (opt-in)
**Scope**
- Documentation, versioning, and packaging.

**Implementation Steps**
1. Add `README` in `clib/` with build usage and opt-in linking.
2. Provide a `clib.version` and changelog.
3. Add CI test step for the library.

**Tests**
- Full `compiler/tests/run.sh` and library suite.

## Notes
- Keep everything small and deterministic.
- Avoid undefined behavior where possible; document deviations.
- Prefer tests that verify output and program exit status.
