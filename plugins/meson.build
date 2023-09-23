# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

if not get_option('plugins').disabled()
  m_dep = cc.find_library('m', required: false)

  subdir('eg-amp.lv2')
  subdir('eg-fifths.lv2')
  subdir('eg-metro.lv2')
  subdir('eg-midigate.lv2')
  subdir('eg-params.lv2')
  subdir('eg-sampler.lv2')
  subdir('eg-scope.lv2')
endif

if not get_option('docs').disabled()
  literasc_py = files('literasc.py')
  asciidoc = find_program('asciidoc', required: get_option('docs'))

  if asciidoc.found()
    book_inputs = files('README.txt')
    book_inputs += files(
      'eg-amp.lv2/README.txt',
      'eg-amp.lv2/amp.c',
      'eg-amp.lv2/amp.ttl',
      'eg-fifths.lv2/README.txt',
      'eg-fifths.lv2/fifths.c',
      'eg-fifths.lv2/fifths.ttl',
      'eg-fifths.lv2/uris.h',
      'eg-metro.lv2/README.txt',
      'eg-metro.lv2/metro.c',
      'eg-metro.lv2/metro.ttl',
      'eg-midigate.lv2/README.txt',
      'eg-midigate.lv2/midigate.c',
      'eg-midigate.lv2/midigate.ttl',
      'eg-params.lv2/README.txt',
      'eg-params.lv2/params.c',
      'eg-params.lv2/params.ttl',
      'eg-params.lv2/state_map.h',
      'eg-sampler.lv2/README.txt',
      'eg-sampler.lv2/atom_sink.h',
      'eg-sampler.lv2/peaks.h',
      'eg-sampler.lv2/sampler.c',
      'eg-sampler.lv2/sampler.ttl',
      'eg-sampler.lv2/sampler_ui.c',
      'eg-sampler.lv2/uris.h',
      'eg-scope.lv2/README.txt',
      'eg-scope.lv2/examploscope.c',
      'eg-scope.lv2/examploscope_ui.c',
      'eg-scope.lv2/uris.h',
    )

    # Compile book sources into book.txt asciidoc source
    book_txt = custom_target(
      'book.txt',
      command: [
        literasc_py,
        '@OUTPUT@',
        '@INPUT@',
      ],
      input: book_inputs,
      output: 'book.txt',
    )

    # Run asciidoc to generate book.html
    book_html = custom_target(
      'book.html',
      build_by_default: true,
      command: [
        asciidoc,
        '-a', 'stylesdir=' + lv2_source_root / 'doc' / 'style',
        '-a', 'source-highlighter=pygments',
        '-a', 'pygments-style=' + lv2_source_root / 'doc' / 'style' / 'style.css',
        '-b', 'html',
        '-o', '@OUTPUT@',
        '@INPUT@',
      ],
      input: book_txt,
      output: 'book.html',
    )
  endif
endif
