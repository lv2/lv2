# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

lv2specgen_py = files('lv2specgen.py')

lv2_list_email = 'devel@lists.lv2plug.in'
lv2_list_page = 'http://lists.lv2plug.in/listinfo.cgi/devel-lv2plug.in'

lv2specgen_command_prefix = [
  lv2specgen_py,
  '--list-email=' + lv2_list_email,
  '--list-page=' + lv2_list_page,
  '--style-dir=' + lv2_source_root / 'doc' / 'style',
  '--template',
  files('template.html'),
]

if is_variable('lv2_tags')
  lv2specgen_command_prefix += [
    ['--tags', lv2_tags.full_path()], # TODO: Remove full_path() in meson 0.60.0
  ]
endif

install_data(
  files('lv2specgen.py'),
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x',
)

meson.override_find_program('lv2specgen.py', lv2specgen_py)

install_data(
  files(
    '../doc/style/pygments.css',
    '../doc/style/style.css',
    'template.html',
  ),
  install_dir: get_option('datadir') / 'lv2specgen',
)

install_subdir(
  'DTD',
  install_dir: get_option('datadir') / 'lv2specgen',
)
