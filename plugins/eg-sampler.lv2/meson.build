# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

plugin_sources = files('sampler.c')
ui_sources = files('sampler_ui.c')
bundle_name = 'eg-sampler.lv2'
data_filenames = ['manifest.ttl.in', 'sampler.ttl', 'click.wav']

samplerate_dep = dependency(
  'samplerate',
  include_type: 'system',
  required: get_option('plugins'),
  version: '>= 0.1.0',
)

sndfile_dep = dependency(
  'sndfile',
  include_type: 'system',
  required: get_option('plugins'),
  version: '>= 1.0.0',
)

gtk2_dep = dependency(
  'gtk+-2.0',
  include_type: 'system',
  required: get_option('plugins'),
  version: '>= 2.18.0',
)

if samplerate_dep.found() and sndfile_dep.found()
  module = shared_library(
    'sampler',
    plugin_sources,
    c_args: c_suppressions,
    dependencies: [lv2_dep, m_dep, samplerate_dep, sndfile_dep],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
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

  if gtk2_dep.found()
    ui_suppressions = c_suppressions
    if cc.get_id() == 'gcc'
      ui_suppressions += ['-Wno-strict-overflow']
    endif

    shared_library(
      'sampler_ui',
      ui_sources,
      c_args: ui_suppressions,
      dependencies: [lv2_dep, gtk2_dep],
      gnu_symbol_visibility: 'hidden',
      implicit_include_directories: false,
      install: true,
      install_dir: lv2dir / bundle_name,
      name_prefix: '',
    )
  endif
endif
