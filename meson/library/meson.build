# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: CC0-1.0 OR ISC

# General definitions for building libraries.
#
# These are essentially workarounds for meson and/or MSVC.  Unfortunately,
# meson's default_library option doesn't support shared and static builds very
# well.  In particular, it's often necessary to define different symbols for
# static and shared builds of libraries so that symbols can be exported.  To
# work around this, we do not support default_library=both on Windows.  On
# other platforms with GCC-like compilers, we can support both because symbols
# can safely be exported in the same way (giving them default visibility) in
# both static and shared builds.

# Abort on Windows with default_library=both
if get_option('default_library') == 'both'
  if host_machine.system() == 'windows'
    error('default_library=both is not supported on Windows')
  endif
endif

# Set library_suffix to the suffix for libraries
if cc.get_id() == 'msvc'
  # Meson appends a version to the name only on MS, which leads to inconsistent
  # library names, like `mylib-1-1`.  So, provide no suffix to ultimately get
  # the same name as on other platforms, like `mylib-1`.
  library_suffix = ''
else
  library_suffix = '-@0@'.format(meson.project_version().split('.')[0])
endif
