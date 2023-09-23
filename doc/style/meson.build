# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

style_files = files('pygments.css', 'style.css')

foreach file : style_files
  configure_file(
    copy: true,
    input: file,
    install_dir: lv2_docdir / 'style',
    output: '@PLAINNAME@',
  )
endforeach
