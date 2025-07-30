# Copyright 2021-2025 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project(
  'lv2',
  ['c'],
  default_options: [
    'b_ndebug=if-release',
    'buildtype=release',
    'c_std=c99',
    'c_winlibs=',
    'cpp_winlibs=',
  ],
  license: 'ISC',
  meson_version: '>= 0.56.0',
  version: '1.18.11',
)

lv2_docdir = get_option('datadir') / 'doc' / 'lv2'
lv2_source_root = meson.current_source_dir()
lv2_build_root = meson.current_build_dir()

#############################
# Compilers and Build Tools #
#############################

# Required tools
pkg = import('pkgconfig')
pymod = import('python')
cc = meson.get_compiler('c')

# Optional C++ compiler and Python tools for tests
if not get_option('tests').disabled()
  if add_languages(['cpp'], native: false, required: get_option('tests'))
    cpp = meson.get_compiler('cpp')
  endif
endif

########################
# Warning Suppressions #
########################

warning_level = get_option('warning_level')

# C
c_suppressions = []
if cc.get_id() in ['clang', 'emscripten']
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-cast-align',
      '-Wno-cast-function-type-strict',
      '-Wno-cast-qual',
      '-Wno-declaration-after-statement',
      '-Wno-documentation-unknown-command',
      '-Wno-double-promotion',
      '-Wno-float-conversion',
      '-Wno-float-equal',
      '-Wno-implicit-float-conversion',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-shorten-64-to-32',
      '-Wno-sign-conversion',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsafe-buffer-usage',
    ]

    if not meson.is_cross_build()
      c_suppressions += ['-Wno-poison-system-directories']
    endif

    if host_machine.system() == 'windows'
      c_suppressions += ['-Wno-format-nonliteral']
    endif
  endif

  if warning_level in ['everything', '3', '2']
    c_suppressions += ['-Wno-unused-parameter']
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-conversion',
      '-Wno-double-promotion',
      '-Wno-float-equal',
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-suggest-attribute=const',
      '-Wno-suggest-attribute=malloc',
      '-Wno-suggest-attribute=pure',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsuffixed-float-constants',
      '-Wno-unused-const-variable',
    ]

    if target_machine.system() == 'windows'
      c_suppressions += ['-Wno-suggest-attribute=format']
    endif
  endif

  if warning_level in ['everything', '3', '2']
    c_suppressions += ['-Wno-unused-parameter']
  endif

elif cc.get_id() == 'msvc'
  if warning_level == 'everything'
    c_suppressions += [
      '/wd4061', # enumerator in switch is not explicitly handled
      '/wd4244', # conversion with possible loss of data
      '/wd4310', # cast truncates constant value
      '/wd4365', # signed/unsigned mismatch
      '/wd4464', # relative include path contains ".."
      '/wd4514', # unreferenced inline function has been removed
      '/wd4514', # unreferenced inline function has been removed
      '/wd4706', # assignment within conditional expression
      '/wd4710', # function not inlined
      '/wd4711', # function selected for automatic inline expansion
      '/wd4820', # padding added after construct
      '/wd5045', # will insert Spectre mitigation for memory load
    ]
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '/wd4100', # unreferenced formal parameter
    ]
  endif

  if warning_level in ['everything', '3', '2']
    c_suppressions += [
      '/wd4267', # conversion from size_t to a smaller type
    ]
  endif
endif

c_suppressions = cc.get_supported_arguments(c_suppressions)

# C++
if is_variable('cpp')
  cpp_suppressions = []

  if warning_level == 'everything'
    if cpp.get_id() in ['clang', 'emscripten']
      cpp_suppressions += [
        '-Wno-c++98-compat',
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-documentation-unknown-command',
        '-Wno-nullability-extension',
        '-Wno-padded',
        '-Wno-reserved-id-macro',
        '-Wno-unsafe-buffer-usage',
      ]

      if not meson.is_cross_build()
        cpp_suppressions += ['-Wno-poison-system-directories']
      endif

      if host_machine.system() == 'windows'
        cpp_suppressions += ['-Wno-format-nonliteral']
      endif

    elif cpp.get_id() == 'gcc'
      cpp_suppressions += [
        '-Wno-cast-align',
        '-Wno-cast-qual',
        '-Wno-inline',
        '-Wno-padded',
        '-Wno-unused-const-variable',
        '-Wno-useless-cast',
      ]

      if target_machine.system() == 'windows'
        cpp_suppressions += ['-Wno-suggest-attribute=format']
      endif

    elif cpp.get_id() == 'msvc'
      cpp_suppressions += [
        '/wd4514', # unreferenced inline function has been removed
        '/wd4706', # assignment within conditional expression
        '/wd4710', # function not inlined
        '/wd4711', # function selected for automatic inline expansion
        '/wd4820', # padding added after data member
        '/wd5045', # will insert Spectre mitigation
        '/wd5264', # const variable is not used
      ]
    endif
  endif

  cpp_suppressions = cpp.get_supported_arguments(cpp_suppressions)
endif

##########################
# LV2 Path Configuration #
##########################

lv2dir = get_option('lv2dir')
if lv2dir == ''
  prefix = get_option('prefix')
  if target_machine.system() == 'darwin' and prefix == '/'
    lv2dir = '/Library/Audio/Plug-Ins/LV2'
  elif target_machine.system() == 'haiku' and prefix == '/'
    lv2dir = '/boot/common/add-ons/lv2'
  elif target_machine.system() == 'windows' and prefix == 'C:/'
    lv2dir = 'C:/Program Files/Common Files/LV2'
  else
    lv2dir = prefix / get_option('libdir') / 'lv2'
  endif
endif

######################
# Package/Dependency #
######################

# Generate pkg-config file for external dependants
pkg.generate(
  description: 'Plugin standard for audio systems',
  filebase: 'lv2',
  name: 'LV2',
  variables: [
    'lv2dir=' + lv2dir,
    'plugindir=' + lv2dir,
  ],
  version: meson.project_version(),
)

# Declare dependency for internal meson dependants
lv2_dep = declare_dependency(
  include_directories: include_directories('include'),
  variables: [
    'lv2dir=' + lv2_source_root / 'lv2',
    'plugindir=' + lv2_build_root / 'plugins',
  ],
  version: meson.project_version(),
)

# Override pkg-config dependency for internal meson dependants
meson.override_dependency('lv2', lv2_dep)

##################
# Specifications #
##################

# Extensions in http://lv2plug.in/ns/ext/
ext_names = [
  'atom',
  'buf-size',
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
  'uri-map',
  'urid',
  'worker',
]

# Extensions in http://lv2plug.in/ns/extensions/
extensions_names = [
  'ui',
  'units',
]

# All extensions (not core) in http://lv2plug.in/ns/
all_extension_names = ext_names + extensions_names

# All specifications in http://lv2plug.in/ns/
all_spec_names = ['core'] + ext_names + extensions_names

###############
# API Headers #
###############

prefix = get_option('prefix')
includedir = get_option('includedir')

install_subdir('include/lv2', install_dir: includedir)

if get_option('old_headers')
  uri_include_dir = prefix / includedir / 'lv2' / 'lv2plug.in' / 'ns'
  include_prefix = 'include' / 'lv2'

  # Install the core headers specially, because they are inconsistent

  core_headers = files(
    include_prefix / 'core' / 'attributes.h',
    include_prefix / 'core' / 'lv2.h',
    include_prefix / 'core' / 'lv2_util.h',
  )

  # Special case lv2.h is also in top-level include directory
  install_headers(files(include_prefix / 'core' / 'lv2.h'))

  # Core headers are inconsistently in "lv2plug.in/ns/lv2core"
  install_data(core_headers, install_dir: uri_include_dir / 'lv2core')

  # Some extensions are in "lv2plug.in/ns/ext"
  foreach dir : ext_names
    install_subdir(
      include_prefix / dir,
      install_dir: uri_include_dir / 'ext',
    )
  endforeach

  # Some extensions are in "lv2plug.in/ns/extensions"
  foreach dir : extensions_names
    install_subdir(
      include_prefix / dir,
      install_dir: uri_include_dir / 'extensions',
    )
  endforeach
endif

#######################
# Scripts and Schemas #
#######################

subdir('scripts')
subdir('schemas.lv2')

##################
# Specifications #
##################

lv2_check_specification = files('scripts' / 'lv2_check_specification.py')

check_python = pymod.find_installation(
  'python3',
  modules: ['rdflib'],
  required: get_option('tests'),
)

if (
  check_python.found()
  and check_python.language_version().version_compare('<3.7')
)
  warning('Python 3.7 is required for tests')
  check_python = disabler()
endif

foreach bundle_name : all_spec_names
  bundle = 'lv2' / bundle_name + '.lv2'

  # Check specification
  if check_python.found()
    test(
      bundle_name,
      lv2_check_specification,
      args: files(bundle / 'manifest.ttl'),
      suite: ['spec'],
    )
  endif

  # Install specification bundle
  install_subdir(bundle, install_dir: lv2dir)
endforeach

spec_files = files(
  'lv2/atom.lv2/atom.meta.ttl',
  'lv2/atom.lv2/atom.ttl',
  'lv2/atom.lv2/manifest.ttl',
  'lv2/buf-size.lv2/buf-size.meta.ttl',
  'lv2/buf-size.lv2/buf-size.ttl',
  'lv2/buf-size.lv2/manifest.ttl',
  'lv2/core.lv2/lv2core.meta.ttl',
  'lv2/core.lv2/lv2core.ttl',
  'lv2/core.lv2/manifest.ttl',
  'lv2/core.lv2/meta.ttl',
  'lv2/core.lv2/people.ttl',
  'lv2/data-access.lv2/data-access.meta.ttl',
  'lv2/data-access.lv2/data-access.ttl',
  'lv2/data-access.lv2/manifest.ttl',
  'lv2/dynmanifest.lv2/dynmanifest.meta.ttl',
  'lv2/dynmanifest.lv2/dynmanifest.ttl',
  'lv2/dynmanifest.lv2/manifest.ttl',
  'lv2/event.lv2/event.meta.ttl',
  'lv2/event.lv2/event.ttl',
  'lv2/event.lv2/manifest.ttl',
  'lv2/instance-access.lv2/instance-access.meta.ttl',
  'lv2/instance-access.lv2/instance-access.ttl',
  'lv2/instance-access.lv2/manifest.ttl',
  'lv2/log.lv2/log.meta.ttl',
  'lv2/log.lv2/log.ttl',
  'lv2/log.lv2/manifest.ttl',
  'lv2/midi.lv2/manifest.ttl',
  'lv2/midi.lv2/midi.meta.ttl',
  'lv2/midi.lv2/midi.ttl',
  'lv2/morph.lv2/manifest.ttl',
  'lv2/morph.lv2/morph.meta.ttl',
  'lv2/morph.lv2/morph.ttl',
  'lv2/options.lv2/manifest.ttl',
  'lv2/options.lv2/options.meta.ttl',
  'lv2/options.lv2/options.ttl',
  'lv2/parameters.lv2/manifest.ttl',
  'lv2/parameters.lv2/parameters.meta.ttl',
  'lv2/parameters.lv2/parameters.ttl',
  'lv2/patch.lv2/manifest.ttl',
  'lv2/patch.lv2/patch.meta.ttl',
  'lv2/patch.lv2/patch.ttl',
  'lv2/port-groups.lv2/manifest.ttl',
  'lv2/port-groups.lv2/port-groups.meta.ttl',
  'lv2/port-groups.lv2/port-groups.ttl',
  'lv2/port-props.lv2/manifest.ttl',
  'lv2/port-props.lv2/port-props.meta.ttl',
  'lv2/port-props.lv2/port-props.ttl',
  'lv2/presets.lv2/manifest.ttl',
  'lv2/presets.lv2/presets.meta.ttl',
  'lv2/presets.lv2/presets.ttl',
  'lv2/resize-port.lv2/manifest.ttl',
  'lv2/resize-port.lv2/resize-port.meta.ttl',
  'lv2/resize-port.lv2/resize-port.ttl',
  'lv2/state.lv2/manifest.ttl',
  'lv2/state.lv2/state.meta.ttl',
  'lv2/state.lv2/state.ttl',
  'lv2/time.lv2/manifest.ttl',
  'lv2/time.lv2/time.meta.ttl',
  'lv2/time.lv2/time.ttl',
  'lv2/ui.lv2/manifest.ttl',
  'lv2/ui.lv2/ui.meta.ttl',
  'lv2/ui.lv2/ui.ttl',
  'lv2/units.lv2/manifest.ttl',
  'lv2/units.lv2/units.meta.ttl',
  'lv2/units.lv2/units.ttl',
  'lv2/uri-map.lv2/manifest.ttl',
  'lv2/uri-map.lv2/uri-map.meta.ttl',
  'lv2/uri-map.lv2/uri-map.ttl',
  'lv2/urid.lv2/manifest.ttl',
  'lv2/urid.lv2/urid.meta.ttl',
  'lv2/urid.lv2/urid.ttl',
  'lv2/worker.lv2/manifest.ttl',
  'lv2/worker.lv2/worker.meta.ttl',
  'lv2/worker.lv2/worker.ttl',
)

#################
# Documentation #
#################

# Determine if all the dependencies for building documentation are present
doxygen = find_program('doxygen', required: get_option('docs'))
build_docs = false
build_lv2specgen = false
doc_deps = []
if not get_option('docs').disabled()
  doc_python_modules = ['lxml', 'markdown', 'pygments', 'rdflib']

  python = pymod.find_installation(
    'python3',
    modules: doc_python_modules,
    required: get_option('docs'),
  )

  if python.found() and python.language_version().version_compare('<3.7')
    warning('Python 3.7 is required for documentation')
    build_docs = false
  endif

  build_docs = doxygen.found() and python.found()
  build_lv2specgen = python.found()
endif

# Run Doxygen first to generate tags
subdir('doc/c')

# Set up lv2specgen and lv2specgen_command_prefix (which references tags)
if build_lv2specgen
  subdir('lv2specgen')
endif

# Generate specification documentation
if build_docs
  subdir('doc/style')
  subdir('doc/ns')
endif

###########
# Plugins #
###########

# Example plugins and "Programming LV2 Plugins" book
if not get_option('plugins').disabled()
  subdir('plugins')
endif

############
# Programs #
############

# Command-line utilities
subdir('util')

# Data and build tests
if not get_option('tests').disabled()
  subdir('test')
endif

if not meson.is_subproject()
  summary(
    {
      'Tests': not get_option('tests').disabled(),
      'Documentation': build_docs,
    },
    bool_yn: true,
    section: 'Components',
  )

  summary(
    {
      'Install prefix': get_option('prefix'),
      'Headers': get_option('prefix') / get_option('includedir'),
      'LV2 bundles': lv2dir,
    },
    section: 'Directories',
  )
endif
