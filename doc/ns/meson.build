# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data({'BASE': '/ns'})

if get_option('online_docs')
  htaccess = configure_file(
    configuration: config,
    input: files('..' / 'htaccess.in'),
    install_dir: lv2_docdir / 'ns',
    output: '.htaccess',
  )
endif

######################
# Core Documentation #
######################

if build_docs
  spec_file = files(lv2_source_root / 'lv2' / 'core.lv2' / 'lv2core.ttl')

  lv2_core_docs = custom_target(
    'lv2core.html',
    command: lv2specgen_command_prefix + [
      '--docdir=../c/html',
      '--style-uri=../style/style.css',
    ] + [
      '@INPUT@',
      '@OUTPUT@',
    ],
    input: spec_file,
    output: 'lv2core.html',
    depends: doc_deps,
    install: true,
    install_dir: lv2_docdir / 'ns',
  )

  if get_option('online_docs')
    configure_file(
      copy: true,
      input: spec_file,
      install_dir: lv2_docdir / 'ns' / 'ext',
      output: '@PLAINNAME@',
    )
  endif
endif

###########################
# Extension Documentation #
###########################

subdir('ext')
subdir('extensions')

#########
# Index #
#########

lv2_build_index = find_program(
  lv2_source_root / 'scripts' / 'lv2_build_index.py',
)

lv2_build_index_command = [
  lv2_build_index,
  ['--lv2-version', meson.project_version()],
  ['--lv2-source-root', lv2_source_root],
]

if get_option('online_docs')
  lv2_build_index_command += [
    '--online',
  ]
endif

index = custom_target(
  'index.html',
  capture: true,
  command: lv2_build_index_command + ['@INPUT@'],
  input: spec_files,
  install: true,
  install_dir: lv2_docdir / 'ns',
  output: 'index.html',
)
