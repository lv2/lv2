# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

header_c_suppressions = []

if get_option('strict')
  if cc.get_id() == 'clang'
    header_c_suppressions += [
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-declaration-after-statement',
      '-Wno-padded',
    ]

    if host_machine.system() == 'windows'
      header_c_suppressions += [
        '-Wno-format-nonliteral',
      ]
    endif

  elif cc.get_id() == 'gcc'
    header_c_suppressions += [
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-padded',
      '-Wno-sign-conversion',
      '-Wno-suggest-attribute=format',
      '-Wno-unused-const-variable',
    ]

    if host_machine.system() == 'windows'
      header_c_suppressions += [
        '-Wno-sign-conversion',
      ]
    endif

  elif cc.get_id() == 'msvc'
    header_c_suppressions += [
      '/wd4820', # padding added after construct
    ]
  endif
endif

if cc.get_id() == 'clang'
  header_c_suppressions += [
    '-Wno-nullability-extension',
  ]
endif

test(
  'headers',
  executable(
    'test_headers',
    files('test_headers.c'),
    c_args: header_c_suppressions,
    dependencies: [lv2_dep],
  ),
  suite: 'unit',
)
