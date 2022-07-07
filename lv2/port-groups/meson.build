# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

name = 'port-groups'
path = 'ns' / 'ext' / 'port-groups'

port_groups_data = files(
  'port-groups.meta.ttl',
  'port-groups.ttl',
  'manifest.ttl',
)

headers = files(
  'port-groups.h',
)

# Install specification bundle
install_data(port_groups_data, install_dir: lv2dir / name + '.lv2')
install_headers(headers, subdir: 'lv2' / name)
if get_option('old_headers')
  install_headers(headers, subdir: 'lv2' / 'lv2plug.in' / path)
endif

# Build documentation
if build_docs
  lv2_port_groups_docs = custom_target(
    name + '.html',
    command: lv2specgen_command_prefix + [
      '--docdir=../../html',
      '--style-uri=../../aux/style.css',
      '@INPUT@',
      '@OUTPUT@',
    ],
    depends: doc_deps,
    input: files('port-groups.ttl'),
    install: true,
    install_dir: lv2_docdir / 'ns' / 'ext',
    output: name + '.html',
  )
endif
