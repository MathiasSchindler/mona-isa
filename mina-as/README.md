# mina-as

Minimal assembler for the MINA ISA. No external dependencies beyond the C standard library.

## Build

```sh
make
```

## Usage

```sh
./mina-as input.s -o output.elf
```

Control ELF layout:

```sh
./mina-as --text-base 0x1000 --data-base 0x3000 --bss-base 0x4000 --segment-align 0x1000 input.s -o output.elf
```

Emit raw binary instead of ELF64:

```sh
./mina-as --bin input.s -o output.bin
```

## Example

```sh
./mina-as examples/hello.s -o hello.bin
```

Run with the simulator:

```sh
../simulator/mina-sim hello.bin
```

## Notes

- Two-pass assembler with labels.
- Supports base ISA, CSR, CAP, and MINA-T.
- Emits ELF64 by default; use `--bin` for raw binary output.
- Directives: `.org`, `.align`, `.byte`, `.half`, `.word`, `.dword`, `.ascii`, `.asciz`, `.text`, `.data`, `.bss`, `.section`.
- Pseudo-instructions: `nop`, `mov`, `not`, `ret`, `j`, `jr`, `li`.

## Layout options

- `--text-base N`: base address for `.text`.
- `--data-base N`: base address for `.data`.
- `--bss-base N`: base address for `.bss`.
- `--segment-align N` / `--seg-align N`: file/segment alignment (ELF).

