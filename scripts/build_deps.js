const util = require('node:util');
const path = require('node:path');
const exec = util.promisify(require('node:child_process').exec);
const controller = new AbortController();
const { signal } = controller;

const GEMMA_REPO = "https://github.com/google/gemma.cpp.git"
const COMMIT = "808dbdc42b216c3ac1f1c40dfa638bcff24bbd2b"

const depsDir = path.join(process.cwd(), 'deps')
const gemmaDir = path.join(depsDir, 'gemma.cpp')
const gemmaBuildDir = path.join(gemmaDir, 'build')

async function rimraf_deps() {
    console.log(`rimraffing deps`)
    try {
        await exec(`rm -rf ${depsDir}`, { signal });
        await exec(`mkdir  ${depsDir}`, { signal });
        console.log(`rimraffed deps`)
    } catch (err) {
        console.log('rimraffed deps', err)
    }
}


async function fetch_gemma() {
    console.log(`cloning gemma`)
    try {
        await exec(`git clone ${GEMMA_REPO}`, { signal, cwd: depsDir });
        await exec(`git checkout ${COMMIT}`, { signal, cwd: gemmaDir });
        console.log(`cloned gemma`)
    } catch (err) {
        console.log('failed to clone gemma', err)
    }
}

async function cmake_gemma() {
    console.log(`cmaking gemma`)
    try {
        await exec('cmake -B build -DSPM_ENABLE_SHARED=OFF', { signal, cwd: gemmaDir });
        console.log(`cmaked gemma`)
    } catch (err) {
        console.log('failed to cmake gemma', err)
    }
}

async function build_gemma() {
    console.log(`building gemma`)
    try {
        await exec('make -j4 libgemma', { signal, cwd: gemmaBuildDir });
        console.log(`built gemma`)
    } catch (err) {
        console.log('failed to build gemma', err)
    }
}

(async () => {
    await rimraf_deps()
    await fetch_gemma()
    await cmake_gemma()
    await build_gemma()
})();
