{
  'targets': [
    {
      'target_name': 'libgemma',
      'type': 'none',
      'actions': [
          {
              'action_name': 'build_libgemma',
              'message': 'Building libgemma...',
              'inputs': ['package.json'],
              'outputs': ['deps/gemma.cpp/build/libgemma.a'],
              'action': ['node', 'scripts/build_deps.js'],
          },
      ],
    },
    {
      'target_name': 'binding',
      'dependencies': ['libgemma'],
      'sources': [ 'binding.cc' ],
      'cflags_cc': ['-std=c++17'],
      'include_dirs': [
        './deps/gemma.cpp',
        './deps/gemma.cpp/build/_deps/highway-src/hwy/contrib',
        './deps/gemma.cpp/build/_deps/highway-src',
        './deps/gemma.cpp/build/_deps/sentencepiece-src'
      ],
      'libraries': [
        "-L<!(pwd)/deps/gemma.cpp/build/", "-lgemma", 
        "-L<!(pwd)/deps/gemma.cpp/build/_deps/highway-build/", "-lhwy", "-lhwy_contrib",
        "-L<!(pwd)/deps/gemma.cpp/build/_deps/sentencepiece-build/src", "-lsentencepiece"
      ]
    }
  ]
}
