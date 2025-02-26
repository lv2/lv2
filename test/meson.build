# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

########
# Data #
########

if get_option('lint')
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
        lv2_source_root / 'plugins',
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

########
# Code #
########

# Check that all the headers compile cleanly in C
test(
  'c',
  executable(
    'test_build_c',
    files('test_build.c'),
    c_args: c_suppressions,
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
      cpp_args: cpp_suppressions,
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
  # Scripts that don't pass with pylint
  lax_python_scripts = files(
    '../lv2specgen/lv2docgen.py',
    '../lv2specgen/lv2specgen.py',
  )

  # Scripts that pass with everything including pylint
  strict_python_scripts = lv2_scripts + files('../plugins/literasc.py')

  all_python_scripts = lax_python_scripts + strict_python_scripts

  # Check script formatting
  black = find_program('black', required: get_option('tests'))
  if black.found()
    black_opts = ['-l', '79', '-q', '--check']
    test(
      'black',
      black,
      args: black_opts + all_python_scripts,
      suite: 'scripts',
    )
  endif

  # Check scripts for errors with flake8
  flake8 = find_program('flake8', required: get_option('tests'))
  if flake8.found()
    test('flake8', flake8, args: all_python_scripts, suite: 'scripts')
  endif

  # Check scripts for errors with pylint
  pylint = find_program('pylint', required: get_option('tests'))
  if pylint.found()
    pymod = import('python')
    lint_py = pymod.find_installation(
      'python3',
      modules: ['pygments', 'rdflib'],
      required: false,
    )

    if lint_py.found()
      test('pylint', pylint, args: strict_python_scripts, suite: 'scripts')
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
      c_args: c_suppressions + atom_test_suppressions,
      dependencies: [lv2_dep],
      implicit_include_directories: false,
    ),
    suite: 'unit',
  )
endforeach
