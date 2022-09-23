## Wasm-fuzzy

C code mostly form source code of vim in `src/search.c`.

## Build

Requires [wasi](https://github.com/WebAssembly/wasi-sdk) and optional [wabt](https://github.com/WebAssembly/wabt).

Change the SDK part in Makefile.

Run command:

```sh
make
node index.js
```

## LICENSE

MIT
