# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

config = configuration_data({'BASE': '/ns/extensions'})

if get_option('online_docs')
  htaccess = configure_file(
    configuration: config,
    input: files('..' / '..' / 'htaccess.in'),
    install_dir: lv2_docdir / 'ns' / 'extensions',
    output: '.htaccess',
  )
endif
