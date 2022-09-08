# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Project-specific warning suppressions.
#
# This should be used in conjunction with the generic "warnings" sibling that
# enables all reasonable warnings for the compiler.  It lives here just to keep
# the top-level meson.build more readable.

#####
# C #
#####

if is_variable('cc')
  c_suppressions = []

  if get_option('strict')
    if cc.get_id() in ['clang', 'emscripten']
      c_suppressions += [
        '-Wno-bad-function-cast',
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-declaration-after-statement',
        '-Wno-documentation-unknown-command',
        '-Wno-double-promotion',
        '-Wno-float-conversion',
        '-Wno-float-equal',
        '-Wno-implicit-float-conversion',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-shorten-64-to-32',
        '-Wno-sign-conversion',
        '-Wno-switch-enum',
        '-Wno-unused-parameter',
      ]
    elif cc.get_id() == 'gcc'
      c_suppressions += [
        '-Wno-bad-function-cast',
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-conversion',
        '-Wno-double-promotion',
        '-Wno-float-equal',
        '-Wno-inline',
        '-Wno-padded',
        '-Wno-suggest-attribute=const',
        '-Wno-suggest-attribute=malloc',
        '-Wno-suggest-attribute=pure',
        '-Wno-switch-default',
        '-Wno-switch-enum',
        '-Wno-unsuffixed-float-constants',
        '-Wno-unused-const-variable',
        '-Wno-unused-parameter',
      ]

      if target_machine.system() == 'windows'
        c_suppressions += [
          '-Wno-suggest-attribute=format',
        ]
      endif

    elif cc.get_id() == 'msvc'
      c_suppressions += [
        '/wd4061', # enumerator in switch is not explicitly handled
        '/wd4100', # unreferenced formal parameter
        '/wd4244', # conversion with possible loss of data
        '/wd4267', # conversion from size_t to a smaller type
        '/wd4310', # cast truncates constant value
        '/wd4365', # signed/unsigned mismatch
        '/wd4464', # relative include path contains ".."
        '/wd4514', # unreferenced inline function has been removed
        '/wd4514', # unreferenced inline function has been removed
        '/wd4706', # assignment within conditional expression
        '/wd4710', # function not inlined
        '/wd4711', # function selected for automatic inline expansion
        '/wd4820', # padding added after construct
        '/wd5045', # will insert Spectre mitigation for memory load
      ]
    endif
  endif

  c_suppressions = cc.get_supported_arguments(c_suppressions)
endif

#######
# C++ #
#######

if is_variable('cpp')
  cpp_suppressions = []

  if get_option('strict')
    if cpp.get_id() in ['clang', 'emscripten']
      cpp_suppressions = [
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-documentation-unknown-command',
        '-Wno-nullability-extension',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
      ]

    elif cpp.get_id() == 'gcc'
      cpp_suppressions = [
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-inline',
        '-Wno-padded',
        '-Wno-unused-const-variable',
        '-Wno-useless-cast',
      ]

      if target_machine.system() == 'windows'
        cpp_suppressions += [
          '-Wno-suggest-attribute=format',
        ]
      endif

    elif cpp.get_id() == 'msvc'
      cpp_suppressions = [
        '/wd4514', # unreferenced inline function has been removed
        '/wd4706', # assignment within conditional expression
        '/wd4710', # function not inlined
        '/wd4711', # function selected for automatic inline expansion
        '/wd4820', # padding added after data member
        '/wd5045', # will insert Spectre mitigation
      ]
    endif
  endif

  cpp_suppressions = cpp.get_supported_arguments(cpp_suppressions)
endif
