const fs = require('fs')

async function setupWasi(fileName) {
  // Read the wasm file.
  const buf = fs.readFileSync(fileName)
  // Instantiate the wasm module.
  const res = await WebAssembly.instantiate(buf, {
    env: {
      // This function is exported to the web assembly.
      consoleLog: function (ptr, length) {
        let memory = res.instance.exports.memory
        // This converts the pointer to a string and frees he memory.
        const array = new Uint8Array(memory.buffer, ptr, length)
        const decoder = new TextDecoder()
        const string = decoder.decode(array)
        console.log(string)
      }
    }
  })
  return res
}

async function main() {
  // Setup the WASI instance.
  const wasi = await setupWasi('./fuzzy.wasm')

  // Get the functions exported from the WebAssembly
  const {
    fuzzyMatch,
    malloc,
    memory
  } = wasi.instance.exports
  let contentPtr = malloc(2048)
  let patternPtr = malloc(1024)
  let resultPtr = malloc(1024)

  const changePattern = (pattern) => {
    let buf = Buffer.from(pattern, 'utf8')
    let len = buf.length
    if (len > 1024) throw new Error('pattern too long')
    let bytes = new Uint8Array(memory.buffer, patternPtr, len + 1)
    bytes.set(buf)
    bytes[len] = 0
  }

  const changeContent = (text) => {
    let buf = Buffer.from(text, 'utf8')
    let len = buf.length
    if (len > 2048) throw new Error('content too long')
    let contentBytes = new Uint8Array(memory.buffer, contentPtr, len + 1)
    contentBytes.set(buf)
    contentBytes[len] = 0
  }

  let pat = 'fb'
  changePattern(pat)
  const matchResults = text => {
    changeContent(text)
    let score = fuzzyMatch(contentPtr, patternPtr, resultPtr, 0)
    if (score) {
      const u32 = new Uint32Array(memory.buffer, resultPtr, 4)
      let arr = u32.slice(0, pat.length)
      console.log(`Match ${pat} => ${text}`)
      console.log(`Matched score & positions`, score, arr)
    } else {
      console.log(`Match ${pat} => ${text}, No match`)
    }
  }
  matchResults('fooBar')
  matchResults('fobbbdefo/Bar')
  matchResults('foot')
  pat = '好'
  changePattern(pat)
  matchResults('abc你好')
}

main().then(() => console.log('Done'))
