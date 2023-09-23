# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data({'BASE': '/ns/extensions'})

if get_option('online_docs')
  htaccess = configure_file(
    configuration: config,
    input: files('..' / '..' / 'htaccess.in'),
    install_dir: lv2_docdir / 'ns' / 'extensions',
    output: '.htaccess',
  )
endif

spec_names = [
  'ui',
  'units',
]

if build_docs
  foreach name : spec_names
    spec_file = files(lv2_source_root / 'lv2' / name + '.lv2' / name + '.ttl')

    custom_target(
      name + '.html',
      command: lv2specgen_command_prefix + [
        '--docdir=../../c/html',
        '--style-uri=../../style/style.css',
      ] + [
        '@INPUT@',
        '@OUTPUT@',
      ],
      depends: doc_deps,
      input: spec_file,
      install: true,
      install_dir: lv2_docdir / 'ns' / 'extensions',
      output: name + '.html',
    )

    if get_option('online_docs')
      configure_file(
        copy: true,
        input: spec_file,
        install_dir: lv2_docdir / 'ns' / 'ext',
        output: '@PLAINNAME@',
      )
    endif
  endforeach
endif
