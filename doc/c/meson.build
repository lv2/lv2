# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

lv2_source_doc = meson.current_source_dir()

if doxygen.found()
  reference_doxygen_in = files('reference.doxygen.in')

  config = configuration_data(
    {
      'LV2_SRCDIR': lv2_source_root,
      'LV2_BUILDDIR': lv2_build_root,
      'LV2_VERSION': meson.project_version(),
    },
  )

  reference_doxygen = configure_file(
    configuration: config,
    input: reference_doxygen_in,
    output: 'reference.doxygen',
  )

  docs = custom_target(
    'html',
    command: [doxygen, '@INPUT@'],
    input: reference_doxygen,
    install: true,
    install_dir: lv2_docdir / 'c',
    output: ['html', 'tags'],
  )

  # TODO: doc_deps is needed because Meson did not support using custom target
  # outputs as dependencies until 0.60.0.  When 0.60.0 is required, this can be
  # cleaned up by removing doc_deps and using lv2_tags (not its path) as a
  # command argument, which Meson will correctly make a dependency for.

  lv2_tags = docs[1]
  doc_deps = [docs]
else
  doc_deps = []
endif
