# clib (MINA C Library)

clib is a small, optional C library for minac programs. It provides a minimal subset of standard functions that can be linked into programs when needed. The compiler still works without clib, so syscall-only programs remain valid.

## Build and link (tests)

The test runner builds clib with minac and links it by concatenating assembly:

- clib is compiled with `minac --emit-asm --no-start clib/src/clib.c`.
- Each test’s assembly is concatenated with clib’s assembly.
- The combined file is assembled with `mina-as` and run in the simulator.

To run the full suite (including clib tests):

- `cd compiler && ./tests/run.sh`

## Functions

clib provides the following functions (subset):

- I/O: `putchar`, `puts`, `exit`, `write`, `read`
- String/memory: `strlen`, `memcpy`, `memset`
- Formatting: `printf`, `vprintf`
- Ctype: `isdigit`, `isalpha`, `isspace`
- Conversion: `atoi`, `strtol`
- Heap (minimal): `malloc`, `free`, `calloc`, `realloc`

## Notes and limitations

- `write` supports `fd=1/2` only; `read` returns 0.
- The heap allocator is minimal and supports a single active allocation.
- `realloc` returns the same pointer and does not grow the allocation.
- `puts` does not append a newline.
- `printf` supports `%s`, `%c`, `%d`, and `%%` only.

## Versioning

The current library version is stored in clib/clib.version.
See clib/CHANGELOG.md for updates.
