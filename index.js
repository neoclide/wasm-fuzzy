const fs = require('fs')

// functions required by WASI.
class Wasi {
  constructor(env) {
    this.env = env
    this.instance = null
  }

  // Initialise the instance from the WebAssembly.
  init(instance) {
    this.instance = instance
  }

  get memory() {
    return this.instance.exports.memory
  }

  static WASI_ESUCCESS = 0

  // Get the environment variables.
  environ_get = (environ, environBuf) => {
    const encoder = new TextEncoder()
    const view = new DataView(this.memory.buffer)

    Object.entries(this.env).map(
      ([key, value]) => `${key}=${value}`
    ).forEach(envVar => {
      view.setUint32(environ, environBuf, true)
      environ += 4

      const bytes = encoder.encode(envVar)
      const buf = new Uint8Array(this.memory.buffer, environBuf, bytes.length + 1)
      environBuf += buf.byteLength
    })
    return this.WASI_ESUCCESS
  }

  // Get the size required to store the environment variables.
  environ_sizes_get = (environCount, environBufSize) => {
    const encoder = new TextEncoder()
    const view = new DataView(this.memory.buffer)

    const envVars = Object.entries(this.env).map(
      ([key, value]) => `${key}=${value}`
    )
    const size = envVars.reduce(
      (acc, envVar) => acc + encoder.encode(envVar).byteLength + 1,
      0
    )
    view.setUint32(environCount, envVars.length, true)
    view.setUint32(environBufSize, size, true)

    return this.WASI_ESUCCESS
  }

  // This gets called on exit to stop the running program.
  // We don't have anything to stop!
  proc_exit = (rval) => {
    return this.WASI_ESUCCESS
  }
}

async function setupWasi(fileName) {
  // Read the wasm file.
  const buf = fs.readFileSync(fileName)

  // Create the Wasi instance passing in some environment variables.
  const wasi = new Wasi({
    LANG: 'en_GB.UTF-8',
    TERM: 'xterm'
  })

  // Instantiate the wasm module.
  const res = await WebAssembly.instantiate(buf, {
    wasi_snapshot_preview1: wasi,
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

  // Initialise the wasi instance
  wasi.init(res.instance)
  return wasi
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
}

main().then(() => console.log('Done'))
