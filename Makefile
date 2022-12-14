SDK=/usr/local/share/wasi-sdk-16.0
SYSROOT=$(SDK)/share/wasi-sysroot

CC=$(SDK)/bin/clang
LD=$(SDK)/bin/wasm-ld
LDFLAGS=-s -S -O3 --export=malloc --export=free --no-entry --allow-undefined -L$(SYSROOT)/lib/wasm32-wasi -lc -lm

WASM2WAT=/usr/local/share/wabt-1.0.29/bin/wasm2wat

sources = c/fuzzy.c
objects = $(sources:%.c=%.bc)
target = fuzzy

.PHONY: all

all: $(target).wat

$(target).wat: $(target).wasm
	$(WASM2WAT) $(target).wasm -o $(target).wat

$(target).wasm: $(objects)
	$(LD) $(objects) $(LDFLAGS) -o $(target).wasm
	
c/%.bc: c/%.c
	$(CC) -c -nostdlib -emit-llvm -Oz $< -o $@

clean:
	rm -f $(target).wasm
	rm -f $(target).wat
	rm -f $(objects)
