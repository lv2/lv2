# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

config = configuration_data({'BASE': '/ns'})

if get_option('online_docs')
  htaccess = configure_file(
    configuration: config,
    input: files('..' / 'htaccess.in'),
    install_dir: lv2_docdir / 'ns',
    output: '.htaccess',
  )
endif

subdir('ext')
subdir('extensions')

lv2_build_index = files(lv2_source_root / 'scripts' / 'lv2_build_index.py')

index = custom_target(
  'index.html',
  capture: true,
  command: [
    lv2_build_index,
    '--lv2-version', meson.project_version(),
    '--lv2-source-root', lv2_source_root,
    '@INPUT@'
  ],
  input: spec_files,
  install: true,
  install_dir: lv2_docdir / 'ns',
  output: 'index.html',
)
