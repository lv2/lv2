# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

dirs = [
  'atom',
  'buf-size',
  'core',
  'data-access',
  'dynmanifest',
  'event',
  'instance-access',
  'log',
  'midi',
  'morph',
  'options',
  'parameters',
  'patch',
  'port-groups',
  'port-props',
  'presets',
  'resize-port',
  'state',
  'time',
  'ui',
  'units',
  'uri-map',
  'urid',
  'worker',
]

foreach dir : dirs
  subdir(dir)
endforeach

if not get_option('tests').disabled()
  check_python = pymod.find_installation('python3',
                                         modules: ['rdflib'],
                                         required: get_option('tests'))

  if check_python.found()
    lv2_check_specification = files(
      lv2_source_root / 'scripts' / 'lv2_check_specification.py'
    )

    foreach dir : dirs
      test(dir,
           lv2_check_specification,
           args: files(dir / 'manifest.ttl'),
           suite: ['spec'])
    endforeach
  endif
endif
