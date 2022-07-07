# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

name = 'ui'
path = 'ns' / 'extensions' / 'ui'

ui_data = files(
  'ui.meta.ttl',
  'ui.ttl',
  'manifest.ttl',
)

headers = files(
  'ui.h',
)

# Install specification bundle
install_data(ui_data, install_dir: lv2dir / name + '.lv2')
install_headers(headers, subdir: 'lv2' / name)
if get_option('old_headers')
  install_headers(headers, subdir: 'lv2' / 'lv2plug.in' / path)
endif

# Build documentation
if build_docs
  lv2_ui_docs = custom_target(
    name + '.html',
    command: lv2specgen_command_prefix + [
      '--docdir=../../html',
      '--style-uri=../../aux/style.css',
      '@INPUT@',
      '@OUTPUT@',
    ],
    depends: doc_deps,
    input: files('ui.ttl'),
    install: true,
    install_dir: lv2_docdir / 'ns' / 'extensions',
    output: name + '.html',
  )
endif
