## Wasm-fuzzy

Wasm module for fuzzy match with native performance.

C code mostly form source code of vim in `src/search.c`.

## Build

Requires [wasi](https://github.com/WebAssembly/wasi-sdk) and optional [wabt](https://github.com/WebAssembly/wabt).

Binary file also provided from [release page](https://github.com/neoclide/wasm-fuzzy/releases)

Change the SDK part in Makefile.

Run command:

```sh
make
node index.js
```

Result should looks like:

```
Match fb => fooBar
Matched score & positions 135 Uint32Array(2) [ 0, 3 ]
Match fb => fobbbdefo/Bar
Matched score & positions 114 Uint32Array(2) [ 0, 10 ]
Match fb => foot, No match
Done
```

## LICENSE

MIT
