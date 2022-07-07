# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

name = 'units'
path = 'ns' / 'extensions' / 'units'

units_data = files(
  'units.meta.ttl',
  'units.ttl',
  'manifest.ttl',
)

headers = files(
  'units.h',
)

# Install specification bundle
install_data(units_data, install_dir: lv2dir / name + '.lv2')
install_headers(headers, subdir: 'lv2' / name)
if get_option('old_headers')
  install_headers(headers, subdir: 'lv2' / 'lv2plug.in' / path)
endif

# Build documentation
if build_docs
  lv2_units_docs = custom_target(
    name + '.html',
    command: lv2specgen_command_prefix + [
      '--docdir=../../html',
      '--style-uri=../../aux/style.css',
      '@INPUT@',
      '@OUTPUT@',
    ],
    depends: doc_deps,
    input: files('units.ttl'),
    install: true,
    install_dir: lv2_docdir / 'ns' / 'extensions',
    output: name + '.html',
  )
endif
