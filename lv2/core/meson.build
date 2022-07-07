# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

name = 'core'
path = 'ns' / 'lv2core'

core_data = files(
  'lv2core.meta.ttl',
  'lv2core.ttl',
  'manifest.ttl',
  'meta.ttl',
  'people.ttl',
)

headers = files(
  'attributes.h',
  'lv2.h',
  'lv2_util.h',
)

# Install specification bundle
install_data(core_data, install_dir: lv2dir / name + '.lv2')
install_headers(headers, subdir: 'lv2' / name)
if get_option('old_headers')
  install_headers(headers, subdir: 'lv2' / 'lv2plug.in' / 'ns' / 'lv2core')
endif

# Build documentation
if build_docs
  lv2_core_docs = custom_target(
    'lv2core.html',
    command: lv2specgen_command_prefix + [
      '--docdir=../html',
      '--style-uri=../aux/style.css',
      '@INPUT@',
      '@OUTPUT@',
    ],
    depends: doc_deps,
    input: files('lv2core.ttl'),
    install: true,
    install_dir: lv2_docdir / 'ns',
    output: 'lv2core.html',
  )
endif
