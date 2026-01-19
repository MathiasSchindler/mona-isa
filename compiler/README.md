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

## Tests (C0)

```sh
./tests/run.sh
```
