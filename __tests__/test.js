const process = require('process');
const addon = require('..');

const gemmaModel = new addon.GemmaModel({
    'tokenizer': '/home/user/Documents/projects/personal/gemmaweight/tokenizer.spm',
    'model': '2b-it',
    'compressed_weights': '/home/user/Documents/projects/personal/gemmaweight/2b-it-sfp.sbs',
})
gemmaModel.prompt({
    prompt: `Generate a README.md file for a node.js binding for gemma.cpp`,
    streamToken: (token) => {
        process.stdout.write(token)
    }
})