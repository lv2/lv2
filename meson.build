# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

project('lv2', ['c'],
        version: '1.18.5',
        license: 'ISC',
        meson_version: '>= 0.56.0',
        default_options: [
          'b_ndebug=if-release',
          'buildtype=release',
          'c_std=c99',
        ])

lv2_docdir = get_option('datadir') / 'doc' / 'lv2'
lv2_source_root = meson.current_source_dir()
lv2_build_root = meson.current_build_dir()

#######################
# Compilers and Flags #
#######################

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

# Set global warning flags
if get_option('strict') and not meson.is_subproject()
  subdir('meson/warnings')
endif
subdir('meson/suppressions')

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
    lv2dir = 'C:/Program Files/Common/LV2'
  else
    lv2dir = prefix / get_option('libdir') / 'lv2'
  endif
endif

######################
# Package/Dependency #
######################

# Generage pkg-config file for external dependants
pkg.generate(
  name: 'LV2',
  filebase: 'lv2',
  subdirs: ['lv2'],
  version: meson.project_version(),
  description: 'Plugin standard for audio systems')

# Declare dependency for internal meson dependants
lv2_dep = declare_dependency(
  include_directories: include_directories('.'),
  version: meson.project_version())

##################
# Specifications #
##################

doc_python_modules = [
  'lxml',
  'markdown',
  'pygments',
  'rdflib',
]

# Determine if all the dependencies for building documentation are present
build_docs = false
doc_deps = []
if not get_option('docs').disabled()
  doxygen = find_program('doxygen', required: get_option('docs'))

  python = pymod.find_installation(
    'python3',
    modules: doc_python_modules,
    required: get_option('docs'),
  )

  build_docs = doxygen.found() and python.found()
endif

# Basic scripts and schema data
subdir('scripts')
subdir('schemas.lv2')

# Run Doxygen to generate tags file and HTML code documentation
if build_docs
  subdir('doc/c')
endif

# Set up lv2specgen for generating individual specification documentation
subdir('lv2specgen')

# Specifications (and their individual documentation)
subdir('lv2')
spec_files = (atom_data +
              buf_size_data +
              core_data +
              data_access_data +
              dynmanifest_data +
              event_data +
              instance_access_data +
              log_data +
              midi_data +
              morph_data +
              options_data +
              parameters_data +
              patch_data +
              port_groups_data +
              port_props_data +
              presets_data +
              resize_port_data +
              state_data +
              time_data +
              ui_data +
              units_data +
              uri_map_data +
              urid_data +
              worker_data)

# Plugins and "Programming LV2 Plugins" book
if not get_option('plugins').disabled()
  subdir('plugins')
endif

############
# Programs #
############

# Command-line utilities
subdir('util')

# Data and build tests
subdir('test')

#################
# Documentation #
#################

# Top-level documentation
if build_docs
  subdir('doc')
endif

########
# News #
########

lv2_write_news_py = find_program('scripts' / 'lv2_write_news.py')

write_news_command = [
  lv2_write_news_py,
  '-t', 'http://lv2plug.in/ns/lv2',
  files(lv2_source_root / 'lv2' / 'core.lv2' / 'people.ttl'),
  files(lv2_source_root / 'lv2' / 'core.lv2' / 'meta.ttl'),
  spec_files,
]

custom_target(
  'NEWS',
  capture: true,
  command: write_news_command,
  output: 'NEWS',
)

if not meson.is_subproject()
  # Generate NEWS file from data in distribution archive
  meson.add_dist_script(write_news_command)

  summary('Tests', not get_option('tests').disabled(), bool_yn: true)
  summary('Documentation', build_docs, bool_yn: true)
  summary('Prefix', get_option('prefix'), section: 'Paths')
  summary('LV2 bundles', lv2dir, section: 'Paths')
  summary('Headers', get_option('prefix') / get_option('includedir'), section: 'Paths')
endif
