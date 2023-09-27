{
  'targets': [
    {
      'target_name': 'json-async-native',
      'sources': [ 'src/json_async.cc', 'deps/simdjson.cpp' ],
      'include_dirs': ["<!@(node -p \"require('node-addon-api').include\")", "<(module_root_dir)/deps"],
      'dependencies': ["<!(node -p \"require('node-addon-api').gyp\")"],
      'cflags': [ '-g' ],
      'cflags!': [ '-fno-exceptions' ],
      'cflags_cc!': [ '-fno-exceptions' ],
      'xcode_settings': {
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
        'CLANG_CXX_LIBRARY': 'libc++',
        'MACOSX_DEPLOYMENT_TARGET': '10.7'
      },
      'msvs_settings': {
        'VCCLCompilerTool': { 'ExceptionHandling': 1 },
      }
    }
  ]
}
