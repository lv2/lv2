# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

plugin_sources = files('examploscope.c')
ui_sources = files('examploscope_ui.c')
bundle_name = 'eg-scope.lv2'
data_filenames = ['manifest.ttl.in', 'examploscope.ttl.in']

gtk2_dep = dependency(
  'gtk+-2.0',
  include_type: 'system',
  required: get_option('plugins'),
  version: '>= 2.18.0',
)

module = shared_library(
  'examploscope',
  plugin_sources,
  c_args: c_suppressions,
  dependencies: [lv2_dep, m_dep],
  gnu_symbol_visibility: 'hidden',
  implicit_include_directories: false,
  install: true,
  install_dir: lv2dir / bundle_name,
  name_prefix: '',
)

config = configuration_data(
  {
    'LIB_EXT': '.' + module.full_path().split('.')[-1],
  },
)

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
  shared_library(
    'examploscope_ui',
    ui_sources,
    c_args: c_suppressions,
    dependencies: [lv2_dep, gtk2_dep],
    gnu_symbol_visibility: 'hidden',
    implicit_include_directories: false,
    install: true,
    install_dir: lv2dir / bundle_name,
    name_prefix: '',
  )
endif
