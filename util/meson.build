# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data({'LV2DIR': lv2dir})

lv2_validate = configure_file(
  configuration: config,
  input: files('lv2_validate.in'),
  install_dir: get_option('bindir'),
  install_mode: 'rwxr-xr-x',
  output: 'lv2_validate',
)
