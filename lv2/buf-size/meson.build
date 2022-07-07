# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

name = 'buf-size'
path = 'ns' / 'ext' / name

buf_size_data = files(
  'buf-size.meta.ttl',
  'buf-size.ttl',
  'manifest.ttl',
)

headers = files(
  'buf-size.h',
)

# Install extension bundle
install_data(buf_size_data, install_dir: lv2dir / name + '.lv2')
install_headers(headers, subdir: 'lv2' / name)
if get_option('old_headers')
  install_headers(headers, subdir: 'lv2' / 'lv2plug.in' / path)
endif

# Build documentation
if build_docs
  lv2_buf_size_docs = custom_target(
    name + '.html',
    command: lv2specgen_command_prefix + [
      '--docdir=../../html',
      '--style-uri=../../aux/style.css',
      '@INPUT@',
      '@OUTPUT@',
    ],
    depends: doc_deps,
    input: files('buf-size.ttl'),
    install: true,
    install_dir: lv2_docdir / 'ns' / 'ext',
    output: name + '.html',
  )
endif
