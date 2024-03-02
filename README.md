# gemma.cpp


This binding provides a node.js interface for the gemma.cpp library.

```bash
npm install gemma.cpp
```

##  Dependencies

* Node.js (>v16.0.0)
* CMake
* Clang

## Usage

```javascript
const addon = require('gemma.cpp');
const process = require('process');

const gemmaModel = new addon.GemmaModel({
    'tokenizer': '/home/user/Documents/gemmaweight/tokenizer.spm',
    'model': '2b-it',
    'compressed_weights': '/home/user/Documents/gemmaweight/2b-it-sfp.sbs',
})
gemmaModel.prompt({
    prompt: `Hello`,
    streamToken: (token) => {
        process.stdout.write(token)
    }
})
```

## Good first issues

- [ ] Only Linux is supported for now (No Windows or MacOS) but please tackle ones below first.
- [ ] Only supports `8-bit switched floating point weights (sfp)`, add support for `16-bit floats` (`-DWEIGHT_TYPE=hwy::bfloat16_t`).
- [ ] Building process still uses CMake to build `libsentencepiece`, `libhighway` and `libgemma`  static libraries. Convert this to use gyp files instead.
- [ ]  No prebuilds, compiles from source.
- [ ]  Context is not saved, model has no history and every prompt runs without history, can be changed.
- [ ]  Uses raw ABI for now (no N-API) but will to accept changes here.
- [ ]  Few cases where libgemma will abort and crash (e.g invalid weights)