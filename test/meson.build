# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

########
# Data #
########

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
      '-q', '3',
      '-S', ','.join(ignore),
      lv2_source_root / 'doc',
      lv2_source_root / 'lv2',
      lv2_source_root / 'lv2specgen',
      lv2_source_root / 'plugins',
      lv2_source_root / 'schemas.lv2',
    ],
    suite: 'data',
  )
endif

# Check that specification data is strictly formatted
serdi = find_program('serdi', required: get_option('tests'))
native_build = not meson.is_cross_build() and host_machine.system() != 'windows'
if serdi.found() and native_build
  lv2_check_syntax = files(lv2_source_root / 'scripts' / 'lv2_check_syntax.py')

  test('syntax',
       lv2_check_syntax,
       args: ['--serdi', serdi.full_path()] + spec_files + schema_data,
       suite: 'data')
endif

# Check that specification data validates
sord_validate = find_program('sord_validate', required: get_option('tests'))
if sord_validate.found()
  test('valid',
       sord_validate,
       args: spec_files + schema_data,
       suite: 'data')
endif

########
# Code #
########

# Check that all the headers compile cleanly in C
test('c',
     executable(
       'test_build_c',
       files('test_build.c'),
       c_args: c_suppressions,
       dependencies: lv2_dep,
     ),
     suite: 'build')

# Check that all the headers compile cleanly in C++
if is_variable('cpp')
  test('cpp',
       executable(
         'test_build_cpp',
         files('test_build.cpp'),
         cpp_args: cpp_suppressions,
         dependencies: lv2_dep,
       ),
       suite: 'build')
endif

##########
# Python #
##########

if get_option('strict')
  flake8 = find_program('flake8', required: get_option('tests'))
  pylint = find_program('pylint', required: get_option('tests'))
  black = find_program('black', required: get_option('tests'))

  # Scripts that don't pass with pylint
  lax_python_scripts = files(
    '../lv2specgen/lv2docgen.py',
    '../lv2specgen/lv2specgen.py',
  )

  # Scripts that pass with everything including pylint
  strict_python_scripts = lv2_scripts + files('../plugins/literasc.py')

  all_python_scripts = lax_python_scripts + strict_python_scripts

  if is_variable('black') and black.found()
    black_opts = ['-l', '79', '-q', '--check']
    test('black', black, args: black_opts + all_python_scripts, suite: 'scripts')
  endif

  if is_variable('flake8') and flake8.found()
    test('flake8', flake8, args: all_python_scripts, suite: 'scripts')
  endif

  if is_variable('pylint') and pylint.found()
    test('pylint', pylint, args: strict_python_scripts, suite: 'scripts')
  endif
endif

##############
# Unit Tests #
##############

test_names = [
  'atom',
  'forge_overflow',
]

# Build and run tests
if not get_option('tests').disabled()
  foreach test_name : test_names
    test(
      test_name,
      executable(
        test_name,
        files('test_@0@.c'.format(test_name)),
        c_args: c_suppressions,
        dependencies: lv2_dep,
      ),
      suite: 'unit',
    )
  endforeach
endif
