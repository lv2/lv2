# Copyright 2020 Alexandros Theodotou <alex@zrythm.org>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

project (
  'lv2', 'c',
  version: '1.18.1',
  license: 'ISC',
  )

# Release variables
title = 'LV2'
uri = 'http://lv2plug.in/ns/lv2'
dist_pattern = 'http://lv2plug.in/spec/lv2-%d.%d.%d.tar.bz2'

# Map of specification base name to old URI-style include path
spec_map = {
  'atom'            : 'lv2/lv2plug.in/ns/ext/atom',
  'buf-size'        : 'lv2/lv2plug.in/ns/ext/buf-size',
  'core'            : 'lv2/lv2plug.in/ns/lv2core',
  'data-access'     : 'lv2/lv2plug.in/ns/ext/data-access',
  'dynmanifest'     : 'lv2/lv2plug.in/ns/ext/dynmanifest',
  'event'           : 'lv2/lv2plug.in/ns/ext/event',
  'instance-access' : 'lv2/lv2plug.in/ns/ext/instance-access',
  'log'             : 'lv2/lv2plug.in/ns/ext/log',
  'midi'            : 'lv2/lv2plug.in/ns/ext/midi',
  'morph'           : 'lv2/lv2plug.in/ns/ext/morph',
  'options'         : 'lv2/lv2plug.in/ns/ext/options',
  'parameters'      : 'lv2/lv2plug.in/ns/ext/parameters',
  'patch'           : 'lv2/lv2plug.in/ns/ext/patch',
  'port-groups'     : 'lv2/lv2plug.in/ns/ext/port-groups',
  'port-props'      : 'lv2/lv2plug.in/ns/ext/port-props',
  'presets'         : 'lv2/lv2plug.in/ns/ext/presets',
  'resize-port'     : 'lv2/lv2plug.in/ns/ext/resize-port',
  'state'           : 'lv2/lv2plug.in/ns/ext/state',
  'time'            : 'lv2/lv2plug.in/ns/ext/time',
  'ui'              : 'lv2/lv2plug.in/ns/extensions/ui',
  'units'           : 'lv2/lv2plug.in/ns/extensions/units',
  'uri-map'         : 'lv2/lv2plug.in/ns/ext/uri-map',
  'urid'            : 'lv2/lv2plug.in/ns/ext/urid',
  'worker'          : 'lv2/lv2plug.in/ns/ext/worker'}

# detect OS
os_darwin = false
os_linux = false
os_freebsd = false
os_windows = false
if host_machine.system() == 'darwin'
  os_darwin = true
elif host_machine.system() == 'linux'
  os_linux = true
elif host_machine.system() == 'freebsd'
  os_freebsd = true
elif host_machine.system() == 'windows'
  os_windows = true
endif

cc = meson.get_compiler ('c')
is_gcc = cc.get_id () == 'gcc'
is_clang = cc.get_id () == 'clang'
# not sure if this works
is_msvc = cc.get_id () == 'msvc'
# not sure how to check this
is_mingw = false

ultra_strict = get_option ('ultra-strict')
coverage = get_option ('coverage')
online_docs = get_option ('online-docs')
build_tests = get_option ('tests')
build_plugins = get_option ('plugins')
copy_headers = get_option ('copy-headers') or os_windows
build_docs = get_option ('docs')
check_links = get_option ('check-links')

common_flags = []
c_flags = []
cxx_flags = []

if ultra_strict
  if is_gcc
    c_flags += [
      '-Wno-bad-function-cast',
      ]
    common_flags += [
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-conversion',
      '-Wno-double-promotion',
      '-Wno-float-equal',
      '-Wno-padded',
      '-Wno-parentheses',
      '-Wno-suggest-attribute=const',
      '-Wno-switch-enum',
      '-Wno-unused-parameter',
      ]
    cxx_flags += [
      '-Wno-useless-cast',
      '-Wno-zero-as-null-pointer-constant',
      ]
    if is_mingw
      common_flags += [
        '-Wno-format',
        '-Wno-suggest-attribute=format',
        ]
    endif
  elif is_clang
    c_flags += [
      '-Wno-bad-function-cast',
      ]
    common_flags += [
      '-Wno-cast-align',
      '-Wno-cast-qual',
      '-Wno-documentation-unknown-command',
      '-Wno-double-promotion',
      '-Wno-float-conversion',
      '-Wno-float-equal',
      '-Wno-implicit-float-conversion',
      '-Wno-padded',
      '-Wno-reserved-id-macro',
      '-Wno-shorten-64-to-32',
      '-Wno-sign-conversion',
      '-Wno-switch-enum',
      '-Wno-unused-parameter',
      ]
    cxx_flags += [
      '-Wno-old-style-cast',
      '-Wno-zero-as-null-pointer-constant',
      ]
  elif is_msvc
    common_flags += [
      '/wd4061',  # enumerator in switch is not explicitly handled
      '/wd4100',  # unreferenced formal parameter
      '/wd4244',  # conversion with possible loss of data
      '/wd4267',  # conversion from size_t to a smaller type
      '/wd4310',  # cast truncates constant value
      '/wd4365',  # signed/unsigned mismatch
      '/wd4464',  # relative include path contains ".."
      '/wd4514',  # unreferenced inline function has been removed
      '/wd4706',  # assignment within conditional expression
      '/wd4710',  # function not inlined
      '/wd4820',  # padding added after construct
      '/wd5045',  # will insert Spectre mitigation for memory load
      ]
  endif
endif

if build_docs or online_docs
  asciidoc = find_program ('asciidoc', required: false)
  build_book = asciidoc.found ()
  if not asciidoc.found ()
    warning ('Asciidoc not found, book will not be built')
  endif
  if check_links
    linkchecker = find_program ('linkchecker', required: false)
    if not linkchecker.found ()
      check_links = false
      warning ('Documentation will not be checked for broken links')
    endif
  endif
endif
