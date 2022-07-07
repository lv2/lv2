# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

if doxygen.found()
  aux_files = files(
    'pygments.css',
    'style.css'
  )

  foreach file : aux_files
    configure_file(
      input: file,
      output: '@PLAINNAME@',
      copy: true,
      install_dir: lv2_docdir / 'aux')
  endforeach

  subdir('ns')

  build_docs = true
else
  build_docs = false
endif
