EMCC ?= emcc
CFLAGS ?= -O2 -std=c11 -Wall -Wextra

SIM_SRC = ../simulator/src/main.c ../simulator/src/cpu.c ../simulator/src/mem.c
SIM_INC = -I../simulator/src

OUT = mina-sim.js
AS_OUT = mina-as.js
CC_OUT = minac.js
ELFINFO_OUT = mina-elf-info.js
CC_SRC = \
	../compiler/src/main.c \
	../compiler/src/preproc.c \
	../compiler/src/lexer.c \
	../compiler/src/parser.c \
	../compiler/src/ast.c \
	../compiler/src/ir.c \
	../compiler/src/lower.c \
	../compiler/src/codegen.c \
	../compiler/src/opt.c

CC_INC = -I../compiler/src

EMCC_FLAGS = \
	$(CFLAGS) \
	-s MODULARIZE=1 \
	-s EXPORT_ES6=1 \
	-s ALLOW_MEMORY_GROWTH=1 \
	-s EXIT_RUNTIME=1 \
	-s ENVIRONMENT=web,node \
	-s EXPORTED_RUNTIME_METHODS='["FS","callMain"]'

AS_SRC = \
	../mina-as/src/main.c \
	../mina-as/src/assemble.c \
	../mina-as/src/buffer.c \
	../mina-as/src/elf.c \
	../mina-as/src/encode.c \
	../mina-as/src/labels.c \
	../mina-as/src/lexer.c \
	../mina-as/src/parse.c \
	../mina-as/src/util.c \
	../mina-as/src/optimize.c

AS_INC = -I../mina-as/src

ELFINFO_SRC = ../tools/mina_elf_info.c

all: $(OUT) $(AS_OUT) $(CC_OUT) $(ELFINFO_OUT)

$(OUT): $(SIM_SRC)
	$(EMCC) $(EMCC_FLAGS) $(SIM_INC) -o $@ $(SIM_SRC) -lm

$(AS_OUT): $(AS_SRC)
	$(EMCC) $(EMCC_FLAGS) $(AS_INC) -o $@ $(AS_SRC)

$(CC_OUT): $(CC_SRC)
	$(EMCC) $(EMCC_FLAGS) $(CC_INC) -o $@ $(CC_SRC) -lm

$(ELFINFO_OUT): $(ELFINFO_SRC)
	$(EMCC) $(EMCC_FLAGS) -o $@ $(ELFINFO_SRC)

clean:
	rm -f $(OUT) mina-sim.wasm $(AS_OUT) mina-as.wasm $(CC_OUT) minac.wasm $(ELFINFO_OUT) mina-elf-info.wasm

.PHONY: all clean
