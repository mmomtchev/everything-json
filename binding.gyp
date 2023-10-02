{
  'target_defaults': {
    'includes': [ 'except.gypi' ]
  },
  'targets': [
    {
      'target_name': 'json-async-native',
      'sources': [
        'deps/simdjson.cpp',
        'src/main.cc',
        'src/JSON.cc',
        'src/parseAsync.cc',
        'src/toObjectAsync.cc'
      ],
      'include_dirs': [
        "<!@(node -p \"require('node-addon-api').include\")",
        "<(module_root_dir)/deps"
      ],
      'dependencies': [ "<!(node -p \"require('node-addon-api').gyp\")" ],
      'cflags': [ '-std=c++17' ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7'
      },
      'msvs_settings': {
        'VCCLCompilerTool': {
          'AdditionalOptions': [
            '/std:c++17'
          ]
        }
      }
    }
  ]
}
