# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

plugin_sources = files('examploscope.c')
bundle_name = 'eg-scope.lv2'
data_filenames = ['manifest.ttl.in', 'examploscope.ttl.in']

module = shared_library(
  'examploscope',
  plugin_sources,
  c_args: c_suppressions,
  dependencies: [lv2_dep, m_dep],
  gnu_symbol_visibility: 'hidden',
  install: true,
  install_dir: lv2dir / bundle_name,
  name_prefix: '',
)

config = configuration_data(
  {
    'LIB_EXT': '.' + module.full_path().split('.')[-1],
  }
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