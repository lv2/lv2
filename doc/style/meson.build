# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

style_files = files(
  'pygments.css',
  'style.css'
)

foreach file : style_files
  configure_file(
    copy: true,
    input: file,
    install_dir: lv2_docdir / 'style',
    output: '@PLAINNAME@',
  )
endforeach
