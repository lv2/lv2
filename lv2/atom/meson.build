# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

name = 'atom'
path = 'ns' / 'ext' / name

atom_data = files(
  'atom.meta.ttl',
  'atom.ttl',
  'manifest.ttl',
)

headers = files(
  'atom.h',
  'forge.h',
  'util.h',
)

tests = [
  'atom-test',
  'forge-overflow-test',
]

# Install specification bundle
install_data(atom_data, install_dir: lv2dir / name + '.lv2')
install_headers(headers, subdir: 'lv2' / name)
if get_option('old_headers')
  install_headers(headers, subdir: 'lv2' / 'lv2plug.in' / path)
endif

# Build and run tests
if not get_option('tests').disabled()
  foreach test : tests
    test(test,
         executable(
           test,
           files('@0@.c'.format(test)),
           c_args: c_suppressions,
           include_directories: include_directories('../../'),
         ),
         suite: 'unit')
  endforeach
endif

# Build documentation
if build_docs
  custom_target(
    name + '.html',
    command: lv2specgen_command_prefix + [
      '--docdir=../../html',
      '--style-uri=../../aux/style.css',
      '@INPUT@',
      '@OUTPUT@',
    ],
    depends: doc_deps,
    input: files('atom.ttl'),
    install: true,
    install_dir: lv2_docdir / 'ns' / 'ext',
    output: name + '.html',
  )
endif
