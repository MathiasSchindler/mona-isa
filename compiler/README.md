# MINA C Compiler (minac)

This directory contains the prototype C compiler for MINA.

## Build

```sh
make
```

## Run

```sh
./minac path/to/file.c
```

Emit IR for debugging:

```sh
./minac --emit-ir path/to/file.c
```

Emit MINA assembly:

```sh
./minac --emit-asm path/to/file.c
```

Assemble and link with `mina-as`:

```sh
./minac -o out.elf path/to/file.c
```

Emit raw binary:

```sh
./minac --bin -o out.bin path/to/file.c
```

## Tests (C0)

```sh
./tests/run.sh
```
