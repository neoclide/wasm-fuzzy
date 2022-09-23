SDK=/usr/local/share/wasi-sdk-16.0
SYSROOT=$(SDK)/share/wasi-sysroot

LLVM_VERSION=10
CC=$(SDK)/bin/clang
LD=$(SDK)/bin/wasm-ld
CFLAGS=-std=c99 --target=wasm32-unknown-unknown-wasm --optimize=3 --sysroot=$(SYSROOT)
LDFLAGS=--export=malloc --export=free --export=fuzzyMatch --no-entry --allow-undefined -L$(SYSROOT)/lib/wasm32-wasi -lc -lm

WASM2WAT=/usr/local/share/wabt-1.0.29/bin/wasm2wat

sources = c/strings.c c/mbyte.c c/alloc.c c/search.c c/fuzzy.c
objects = $(sources:%.c=%.bc)
target = fuzzy

.PHONY: all

all: $(target).wat

$(target).wat: $(target).wasm
	$(WASM2WAT) $(target).wasm -o $(target).wat

$(target).wasm: $(objects)
	$(LD) $(objects) $(LDFLAGS) -o $(target).wasm
	
c/%.bc: c/%.c
	$(CC) -c -emit-llvm $(CFLAGS) $< -o $@

clean:
	rm -f $(target).wasm
	rm -f $(target).wat
	rm -f $(objects)
