# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

########
# Data #
########

if get_option('lint')
  # Check licensing metadata
  reuse = find_program('reuse', required: false)
  if reuse.found()
    reuse_args = ['--root', lv2_source_root, 'lint']
    test('REUSE', reuse, args: reuse_args, suite: 'data')
  endif

  # Check for spelling errors
  codespell = find_program('codespell', required: get_option('tests'))
  if codespell.found()
    ignore = [
      lv2_source_root / 'doc' / 'style' / 'pygments.css',
      lv2_source_root / 'lv2specgen' / 'DTD',
      lv2_source_root / 'schemas.lv2' / 'doap.ttl',
    ]

    test(
      'codespell',
      codespell,
      args: [
        '-d',
        ['-q', '3'],
        ['-S', ','.join(ignore)],
        lv2_source_root / 'doc',
        lv2_source_root / 'lv2',
        lv2_source_root / 'lv2specgen',
        lv2_source_root / 'schemas.lv2',
      ],
      suite: 'data',
    )
  endif

  # Check that specification data is strictly formatted
  serdi = find_program(
    'serdi',
    required: get_option('tests'),
    version: '>= 0.32.0',
  )
  native_build = (
    not meson.is_cross_build()
    and host_machine.system() != 'windows'
  )
  if serdi.found() and native_build
    lv2_check_syntax = files(
      lv2_source_root / 'scripts' / 'lv2_check_syntax.py',
    )

    test(
      'syntax',
      lv2_check_syntax,
      args: ['--serdi', serdi.full_path()] + spec_files + schema_data,
      suite: 'data',
    )
  endif

  # Check that specification data validates
  sord_validate = find_program('sord_validate', required: get_option('tests'))
  if sord_validate.found()
    test('valid', sord_validate, args: spec_files + schema_data, suite: 'data')
  endif
endif

#############################
# Compilers and Build Tools #
#############################

cc = meson.get_compiler('c')

if add_languages(['cpp'], native: false, required: get_option('tests'))
  cpp = meson.get_compiler('cpp')
endif

########################
# Warning Suppressions #
########################

warning_level = get_option('warning_level')

# C
test_c_suppressions = []
if cc.get_id() in ['clang', 'emscripten']
  if warning_level == 'everything'
    test_c_suppressions += [
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-declaration-after-statement',
      '-Wno-documentation-unknown-command',
      '-Wno-double-promotion',
      '-Wno-float-equal',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-unsafe-buffer-usage',
    ]

    if not meson.is_cross_build()
      test_c_suppressions += ['-Wno-poison-system-directories']
    endif

    if host_machine.system() == 'windows'
      test_c_suppressions += ['-Wno-format-nonliteral']
    endif
  endif

  if warning_level in ['everything', '3', '2']
    test_c_suppressions += ['-Wno-unused-parameter']
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    test_c_suppressions += [
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-double-promotion',
      '-Wno-float-equal',
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-suggest-attribute=const',
      '-Wno-suggest-attribute=malloc',
      '-Wno-suggest-attribute=pure',
      '-Wno-unsuffixed-float-constants',
      '-Wno-unused-const-variable',
    ]

    if target_machine.system() == 'windows'
      test_c_suppressions += [
        '-Wno-sign-conversion',
        '-Wno-suggest-attribute=format',
      ]
    endif
  endif

  if warning_level in ['everything', '3', '2']
    test_c_suppressions += ['-Wno-unused-parameter']
  endif

elif cc.get_id() == 'msvc'
  if warning_level == 'everything'
    test_c_suppressions += [
      '/wd4061', # enumerator in switch is not explicitly handled
      '/wd4244', # conversion with possible loss of data
      '/wd4365', # signed/unsigned mismatch
      '/wd4514', # unreferenced inline function has been removed
      '/wd4710', # function not inlined
      '/wd4711', # function selected for automatic inline expansion
      '/wd4820', # padding added after construct
      '/wd5045', # will insert Spectre mitigation for memory load
    ]
  endif

  if warning_level in ['everything', '3']
    test_c_suppressions += [
      '/wd4100', # unreferenced formal parameter
    ]
  endif

  if warning_level in ['everything', '3', '2']
    test_c_suppressions += [
      '/wd4267', # conversion from size_t to a smaller type
    ]
  endif
endif

test_c_suppressions = cc.get_supported_arguments(test_c_suppressions)

# C++
if is_variable('cpp')
  test_cpp_suppressions = []

  if warning_level == 'everything'
    if cpp.get_id() in ['clang', 'emscripten']
      test_cpp_suppressions += [
        '-Wno-c++98-compat',
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-documentation-unknown-command',
        '-Wno-nullability-extension',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-unsafe-buffer-usage',
      ]

      if not meson.is_cross_build()
        test_cpp_suppressions += ['-Wno-poison-system-directories']
      endif

      if host_machine.system() == 'windows'
        test_cpp_suppressions += ['-Wno-format-nonliteral']
      endif

    elif cpp.get_id() == 'gcc'
      test_cpp_suppressions += [
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-inline',
        '-Wno-padded',
        '-Wno-unused-const-variable',
        '-Wno-useless-cast',
      ]

      if target_machine.system() == 'windows'
        test_cpp_suppressions += ['-Wno-suggest-attribute=format']
      endif

    elif cpp.get_id() == 'msvc'
      test_cpp_suppressions += [
        '/wd4514', # unreferenced inline function has been removed
        '/wd4706', # assignment within conditional expression
        '/wd4710', # function not inlined
        '/wd4711', # function selected for automatic inline expansion
        '/wd4820', # padding added after data member
        '/wd5045', # will insert Spectre mitigation
        '/wd5264', # const variable is not used
      ]
    endif
  endif

  test_cpp_suppressions = cpp.get_supported_arguments(test_cpp_suppressions)
endif

########
# Code #
########

# Check that all the headers compile cleanly in C
test(
  'c',
  executable(
    'test_build_c',
    files('test_build.c'),
    c_args: test_c_suppressions,
    dependencies: [lv2_dep],
    implicit_include_directories: false,
  ),
  suite: 'build',
)

# Check that all the headers compile cleanly in C++
if is_variable('cpp')
  test(
    'cpp',
    executable(
      'test_build_cpp',
      files('cpp/test_build.cpp'),
      cpp_args: test_cpp_suppressions,
      dependencies: [lv2_dep],
      implicit_include_directories: false,
    ),
    suite: 'build',
  )
endif

##########
# Python #
##########

if get_option('lint')
  python_scripts = lv2_scripts + files(
    '../lv2specgen/lv2docgen.py',
    '../lv2specgen/lv2specgen.py',
  )

  # Check script formatting
  black = find_program('black', required: get_option('tests'))
  if black.found()
    black_opts = ['-l', '79', '-q', '--check']
    test(
      'black',
      black,
      args: black_opts + python_scripts,
      suite: 'scripts',
    )
  endif

  # Check scripts for errors with flake8
  flake8 = find_program('flake8', required: get_option('tests'))
  if flake8.found()
    test('flake8', flake8, args: python_scripts, suite: 'scripts')
  endif

  # Check scripts for errors with pylint
  pylint = find_program('pylint', required: get_option('tests'))
  if pylint.found()
    lint_py = pymod.find_installation(
      'python3',
      modules: ['pygments', 'rdflib'],
      required: false,
    )

    if lint_py.found()
      test('pylint', pylint, args: python_scripts, suite: 'scripts')
    endif
  endif
endif

###################
# Header Warnings #
###################

subdir('headers')

##############
# Unit Tests #
##############

test_names = [
  'atom',
  'forge_overflow',
]

atom_test_suppressions = []
if cc.get_id() == 'gcc'
  atom_test_suppressions += ['-Wno-stringop-overflow']
endif

# Build and run tests
foreach test_name : test_names
  test(
    test_name,
    executable(
      test_name,
      files('test_@0@.c'.format(test_name)),
      c_args: test_c_suppressions + atom_test_suppressions,
      dependencies: [lv2_dep],
      implicit_include_directories: false,
    ),
    suite: 'unit',
  )
endforeach
