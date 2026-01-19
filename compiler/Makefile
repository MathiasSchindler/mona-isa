CC ?= clang
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic

BIN := minac
SRC := src/main.c src/lexer.c src/parser.c src/ast.c src/ir.c src/lower.c

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(BIN)

test: $(BIN)
	./tests/run.sh

.PHONY: all clean test
