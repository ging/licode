{
  'targets': [
  {
    'target_name': 'addon',
      'sources': [ 'addon.cc', 'WebRtcConnection.cc', 'OneToManyProcessor.cc', 'ExternalInput.cc', 'ExternalOutput.cc'],
      'include_dirs' : ["<!(node -e \"require('nan')\")", '$(ERIZO_HOME)/src/erizo', '$(ERIZO_HOME)/../build/libdeps/build/include'],
      'libraries': ['$(ERIZO_HOME)/build/erizo/liberizo.a', '-l boost_thread-mt', '-l log4cxx', '-lglib-2.0', '-lgobject-2.0', '-lgthread-2.0', '-lgio-2.0', '-lvpx'],
      'conditions': [
        [ 'OS=="mac"', {
          'xcode_settings': {
            'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
              'GCC_ENABLE_CPP_RTTI': 'YES',              # -fno-rtti
              'MACOSX_DEPLOYMENT_TARGET' : '10.11',      #from MAC OS 10.7
              'OTHER_CFLAGS': [
              '-g -O3 -stdlib=libc++ -std=c++0x',
            ]
          },
        }, { # OS!="mac"
          'cflags!' : ['-fno-exceptions'],
          'cflags' : ['-D__STDC_CONSTANT_MACROS'],
          'cflags_cc' : ['-Wall', '-O3', '-g' , '-std=c++0x', '-fexceptions'],
          'cflags_cc!' : ['-fno-exceptions'],
          'cflags_cc!' : ['-fno-rtti']
        }],
        ]
  }
  ]
}
