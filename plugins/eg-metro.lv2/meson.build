# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

plugin_sources = files('metro.c')
bundle_name = 'eg-metro.lv2'
data_filenames = ['manifest.ttl.in', 'metro.ttl']

module = shared_library(
  'metro',
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
