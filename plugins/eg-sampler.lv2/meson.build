# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

plugin_sources = files('sampler.c')
bundle_name = 'eg-sampler.lv2'
data_filenames = ['manifest.ttl.in', 'sampler.ttl', 'click.wav']

sndfile_dep = dependency('sndfile',
                         version: '>= 1.0.0',
                         required: get_option('plugins'))

if sndfile_dep.found()
  module = shared_library(
    'sampler',
    plugin_sources,
    c_args: c_suppressions,
    dependencies: [lv2_dep, m_dep, sndfile_dep],
    gnu_symbol_visibility: 'hidden',
    install: true,
    install_dir: lv2dir / bundle_name,
    name_prefix: '',
  )

  extension = '.' + module.full_path().split('.')[-1]
  config = configuration_data({'LIB_EXT': extension})

  foreach filename : data_filenames
    if filename.endswith('.in')
      configure_file(
        configuration: config,
        input: files(filename),
        install_dir: lv2dir / bundle_name,
        output: filename.substring(0, -3),
      )
    else
      configure_file(
        copy: true,
        input: files(filename),
        install_dir: lv2dir / bundle_name,
        output: filename,
      )
    endif
  endforeach
endif
