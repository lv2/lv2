#!/usr/bin/env python

import os
import re
import sys

from waflib import Context, Logs, Options, Scripting, Utils
from waflib.extras import autowaf as autowaf

# Mandatory waf variables
APPNAME = 'lv2'     # Package name for waf dist
VERSION = '1.18.0'  # Package version for waf dist
top     = '.'       # Source directory
out     = 'build'   # Build directory

# Release variables
title        = 'LV2'
uri          = 'http://lv2plug.in/ns/lv2'
dist_pattern = 'http://lv2plug.in/spec/lv2-%d.%d.%d.tar.bz2'
post_tags    = []

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

def options(ctx):
    ctx.load('compiler_c')
    ctx.load('compiler_cxx')
    ctx.load('lv2')
    ctx.add_flags(
        ctx.configuration_options(),
        {'no-coverage':    'Do not use gcov for code coverage',
         'online-docs':    'Build documentation for web hosting',
         'no-check-links': 'Do not check documentation for broken links',
         'no-plugins':     'Do not build example plugins',
         'copy-headers':   'Copy headers instead of linking to bundle'})

def configure(conf):
    try:
        conf.load('compiler_c', cache=True)
    except:
        Options.options.build_tests = False
        Options.options.no_plugins = True

    try:
        conf.load('compiler_cxx', cache=True)
    except Exception:
        pass

    if Options.options.online_docs:
        Options.options.docs = True

    conf.load('lv2', cache=True)
    conf.load('autowaf', cache=True)
    autowaf.set_c_lang(conf, 'c99')

    if Options.options.ultra_strict and not conf.env.MSVC_COMPILER:
        conf.env.append_value('CFLAGS', ['-Wconversion'])

    if conf.env.DEST_OS == 'win32' or not hasattr(os.path, 'relpath'):
        Logs.warn('System does not support linking headers, copying')
        Options.options.copy_headers = True

    conf.env.BUILD_TESTS   = Options.options.build_tests
    conf.env.BUILD_PLUGINS = not Options.options.no_plugins
    conf.env.COPY_HEADERS  = Options.options.copy_headers
    conf.env.ONLINE_DOCS   = Options.options.online_docs

    if conf.env.DOCS or conf.env.ONLINE_DOCS:
        try:
            conf.find_program('asciidoc')
            conf.env.BUILD_BOOK = True
        except:
            Logs.warn('Asciidoc not found, book will not be built')

        if not Options.options.no_check_links:
            if not conf.find_program('linkchecker',
                                     var='LINKCHECKER', mandatory=False):
                Logs.warn('Documentation will not be checked for broken links')

    # Check for gcov library (for test coverage)
    if (conf.env.BUILD_TESTS
        and not Options.options.no_coverage
        and not conf.is_defined('HAVE_GCOV')):
        conf.check_cc(lib='gcov', define_name='HAVE_GCOV', mandatory=False)

    if conf.env.BUILD_TESTS:
        conf.find_program('serdi', mandatory=False)
        conf.find_program('sord_validate', mandatory=False)

    autowaf.set_lib_env(conf, 'lv2', VERSION, has_objects=False)
    autowaf.set_local_lib(conf, 'lv2', has_objects=False)

    conf.run_env.append_unique('LV2_PATH',
                               [os.path.join(conf.path.abspath(), 'lv2')])

    if conf.env.BUILD_PLUGINS:
        for i in ['eg-amp.lv2',
                  'eg-fifths.lv2',
                  'eg-metro.lv2',
                  'eg-midigate.lv2',
                  'eg-params.lv2',
                  'eg-sampler.lv2',
                  'eg-scope.lv2']:
            try:
                path = os.path.join('plugins', i)
                conf.recurse(path)
                conf.env.LV2_BUILD += [path]
                conf.run_env.append_unique(
                    'LV2_PATH', [conf.build_path('plugins/%s/lv2' % i)])
            except Exception as e:
                Logs.warn('Configuration failed, not building %s (%s)' % (i, e))

    autowaf.display_summary(
        conf,
        {'Bundle directory': conf.env.LV2DIR,
         'Copy (not link) headers': bool(conf.env.COPY_HEADERS),
         'Version': VERSION})

def chop_lv2_prefix(s):
    if s.startswith('lv2/lv2plug.in/'):
        return s[len('lv2/lv2plug.in/'):]
    return s

def subst_file(template, output, dict):
    i = open(template, 'r')
    o = open(output, 'w')
    for line in i:
        for key in dict:
            line = line.replace(key, dict[key])
        o.write(line)
    i.close()
    o.close()

def specdirs(path):
    return (path.ant_glob('lv2/*', dir=True) +
            path.ant_glob('plugins/*.lv2', dir=True))

def ttl_files(path, specdir):
    def abspath(node):
        return node.abspath()

    return map(abspath,
               path.ant_glob(specdir.path_from(path) + '/*.ttl'))

def load_ttl(files, exclude = []):
    import rdflib
    model = rdflib.ConjunctiveGraph()
    for f in files:
        if f not in exclude:
            model.parse(f, format='n3')
    return model

# Task to build extension index
def build_index(task):
    src_dir = task.inputs[0].parent.parent
    sys.path.append(str(src_dir.find_node('lv2specgen')))
    import rdflib
    import lv2specgen

    doap = rdflib.Namespace('http://usefulinc.com/ns/doap#')
    rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')

    model = load_ttl([str(src_dir.find_node('lv2/core/meta.ttl')),
                      str(src_dir.find_node('lv2/core/people.ttl'))])

    # Get date for this version, and list of all LV2 distributions
    proj  = rdflib.URIRef('http://lv2plug.in/ns/lv2')
    date  = None
    dists = []
    for r in model.triples([proj, doap.release, None]):
        revision = model.value(r[2], doap.revision, None)
        created  = model.value(r[2], doap.created, None)
        if str(revision) == VERSION:
            date = created

        dist = model.value(r[2], doap['file-release'], None)
        if dist and created:
            dists += [(created, dist)]
        else:
            print('warning: %s has no file release\n' % proj)

    rows = []
    for f in task.inputs:
        if not f.abspath().endswith('index.html.in'):
            rowfile = open(f.abspath(), 'r')
            rows += rowfile.readlines()
            rowfile.close()

    if date is None:
        import datetime
        import time
        now = int(os.environ.get('SOURCE_DATE_EPOCH', time.time()))
        date = datetime.datetime.utcfromtimestamp(now).strftime('%F')

    subst_file(task.inputs[0].abspath(), task.outputs[0].abspath(),
               {'@ROWS@': ''.join(rows),
                '@LV2_VERSION@': VERSION,
                '@DATE@': date})

def build_spec(bld, path):
    name            = os.path.basename(path)
    bundle_dir      = os.path.join(bld.env.LV2DIR, name + '.lv2')
    include_dir     = os.path.join(bld.env.INCLUDEDIR, path)
    old_include_dir = os.path.join(bld.env.INCLUDEDIR, spec_map[name])

    # Build test program if applicable
    for test in bld.path.ant_glob(os.path.join(path, '*-test.c')):
        test_lib       = []
        test_cflags    = ['']
        test_linkflags = ['']
        if bld.is_defined('HAVE_GCOV'):
            test_lib       += ['gcov']
            test_cflags    += ['--coverage']
            test_linkflags += ['--coverage']
            if bld.env.DEST_OS not in ['darwin', 'win32']:
                test_lib += ['rt']

        # Unit test program
        bld(features     = 'c cprogram',
            source       = test,
            lib          = test_lib,
            uselib       = 'LV2',
            target       = os.path.splitext(str(test.get_bld()))[0],
            install_path = None,
            cflags       = test_cflags,
            linkflags    = test_linkflags)

    # Install bundle
    bld.install_files(bundle_dir,
                      bld.path.ant_glob(path + '/?*.*', excl='*.in'))

    # Install URI-like includes
    headers = bld.path.ant_glob(path + '/*.h')
    if headers:
        for d in [include_dir, old_include_dir]:
            if bld.env.COPY_HEADERS:
                bld.install_files(d, headers)
            else:
                bld.symlink_as(d,
                               os.path.relpath(bundle_dir, os.path.dirname(d)))

def build(bld):
    specs = (bld.path.ant_glob('lv2/*', dir=True))

    # Copy lv2.h to include directory for backwards compatibility
    old_lv2_h_path = os.path.join(bld.env.INCLUDEDIR, 'lv2.h')
    if bld.env.COPY_HEADERS:
        bld.install_files(os.path.dirname(old_lv2_h_path), 'lv2/core/lv2.h')
    else:
        bld.symlink_as(old_lv2_h_path, 'lv2/core/lv2.h')

    # LV2 pkgconfig file
    bld(features     = 'subst',
        source       = 'lv2.pc.in',
        target       = 'lv2.pc',
        install_path = '${LIBDIR}/pkgconfig',
        PREFIX       = bld.env.PREFIX,
        INCLUDEDIR   = bld.env.INCLUDEDIR,
        VERSION      = VERSION)

    # Validator
    bld(features     = 'subst',
        source       = 'util/lv2_validate.in',
        target       = 'lv2_validate',
        chmod        = Utils.O755,
        install_path = '${BINDIR}',
        LV2DIR       = bld.env.LV2DIR)

    # Build extensions
    for spec in specs:
        build_spec(bld, spec.path_from(bld.path))

    # Build plugins
    for plugin in bld.env.LV2_BUILD:
        bld.recurse(plugin)

    # Install lv2specgen
    bld.install_files('${DATADIR}/lv2specgen/',
                      ['lv2specgen/style.css',
                       'lv2specgen/template.html'])
    bld.install_files('${DATADIR}/lv2specgen/DTD/',
                      bld.path.ant_glob('lv2specgen/DTD/*'))
    bld.install_files('${BINDIR}', 'lv2specgen/lv2specgen.py',
                      chmod=Utils.O755)

    # Install schema bundle
    bld.install_files('${LV2DIR}/schemas.lv2/',
                      bld.path.ant_glob('schemas.lv2/*.ttl'))

    if bld.env.ONLINE_DOCS:
        # Generate .htaccess files
        for d in ('ns', 'ns/ext', 'ns/extensions'):
            path = os.path.join(str(bld.path.get_bld()), d)
            bld(features     = 'subst',
                source       = 'doc/htaccess.in',
                target       = os.path.join(path, '.htaccess'),
                install_path = None,
                BASE         = '/' + d)

    if bld.env.DOCS or bld.env.ONLINE_DOCS:
        # Copy spec files to build dir
        for spec in specs:
            srcpath   = spec.path_from(bld.path)
            basename  = os.path.basename(srcpath)
            full_path = spec_map[basename]
            name      = 'lv2core' if basename == 'core' else basename
            path      = chop_lv2_prefix(full_path)
            spec_path = os.path.join(path[3:], name + '.ttl')

            bld(features = 'subst',
                is_copy  = True,
                source   = os.path.join(srcpath, name + '.ttl'),
                target   = path + '.ttl')

        # Copy stylesheets to build directory
        for i in ['style.css', 'pygments.css']:
            bld(features = 'subst',
                is_copy  = True,
                name     = 'copy',
                source   = 'doc/%s' % i,
                target   = 'aux/%s' % i)

        # Build Doxygen documentation (and tags file)
        autowaf.build_dox(bld, 'LV2', VERSION, top, out, 'doc', False)
        bld.add_group()

        index_files = []
        for spec in specs:
            # Call lv2specgen to generate spec docs
            srcpath      = spec.path_from(bld.path)
            basename     = os.path.basename(srcpath)
            full_path    = spec_map[basename]
            name         = 'lv2core' if basename == 'core' else basename
            ttl_name     = name + '.ttl'
            index_file   = bld.path.get_bld().make_node('index_rows/' + name)
            index_files += [index_file]
            chopped_path = chop_lv2_prefix(full_path)

            assert chopped_path.startswith('ns/')
            root_path    = os.path.relpath('/', os.path.dirname(chopped_path[2:]))
            html_path    = '%s.html' % chopped_path
            out_dir      = os.path.dirname(html_path)

            cmd = (str(bld.path.find_node('lv2specgen/lv2specgen.py')) +
                   ' --root-uri=http://lv2plug.in/ns/ --root-path=' + root_path +
                   ' --list-email=devel@lists.lv2plug.in'
                   ' --list-page=http://lists.lv2plug.in/listinfo.cgi/devel-lv2plug.in'
                   ' --style-uri=' + os.path.relpath('aux/style.css', out_dir) +
                   ' --docdir=' + os.path.relpath('doc/html', out_dir) +
                   ' --tags=%s' % bld.path.get_bld().make_node('doc/tags') +
                   ' --index=' + str(index_file) +
                   ' ${SRC} ${TGT}')

            bld(rule   = cmd,
                source = os.path.join(srcpath, ttl_name),
                target = [html_path, index_file],
                shell  = False)

            # Install documentation
            base = chop_lv2_prefix(srcpath)
            bld.install_files(os.path.join('${DOCDIR}', 'lv2', os.path.dirname(html_path)),
                              html_path)

        index_files.sort(key=lambda x: x.path_from(bld.path))
        bld.add_group()

        # Build extension index
        bld(rule   = build_index,
            name   = 'index',
            source = ['doc/index.html.in'] + index_files,
            target = 'ns/index.html')

        # Install main documentation files
        bld.install_files('${DOCDIR}/lv2/aux/', 'aux/style.css')
        bld.install_files('${DOCDIR}/lv2/ns/', 'ns/index.html')

        def check_links(ctx):
            import subprocess
            if ctx.env.LINKCHECKER:
                if subprocess.call([ctx.env.LINKCHECKER[0], '--no-status', out]):
                    ctx.fatal('Documentation contains broken links')

        if bld.cmd == 'build':
            bld.add_post_fun(check_links)

    if bld.env.BUILD_TESTS:
        # Generate a compile test file that includes all headers
        def gen_build_test(task):
            with open(task.outputs[0].abspath(), 'w') as out:
                for i in task.inputs:
                    out.write('#include "%s"\n' % i.bldpath())
                out.write('int main(void) { return 0; }\n')

        bld(rule         = gen_build_test,
            source       = bld.path.ant_glob('lv2/**/*.h'),
            target       = 'build-test.c',
            install_path = None)

        bld(features     = 'c cprogram',
            source       = bld.path.get_bld().make_node('build-test.c'),
            target       = 'build-test',
            includes     = '.',
            uselib       = 'LV2',
            install_path = None)

        if 'COMPILER_CXX' in bld.env:
            bld(rule         = gen_build_test,
                source       = bld.path.ant_glob('lv2/**/*.h'),
                target       = 'build-test.cpp',
                install_path = None)

            bld(features     = 'cxx cxxprogram',
                source       = bld.path.get_bld().make_node('build-test.cpp'),
                target       = 'build-test-cpp',
                includes     = '.',
                uselib       = 'LV2',
                install_path = None)

    if bld.env.BUILD_BOOK:
        # Build "Programming LV2 Plugins" book from plugin examples
        bld.recurse('plugins')

def lint(ctx):
    "checks code for style issues"
    import subprocess

    subprocess.call("flake8 --ignore E203,E221,W503,W504,E302,E305,E251,E241,E722 "
                    "wscript lv2specgen/lv2docgen.py lv2specgen/lv2specgen.py "
                    "plugins/literasc.py",
                    shell=True)

    cmd = ("clang-tidy -p=. -header-filter=.* -checks=\"*," +
           "-hicpp-signed-bitwise," +
           "-llvm-header-guard," +
           "-misc-unused-parameters," +
           "-readability-else-after-return\" " +
           "build-test.c")
    subprocess.call(cmd, cwd='build', shell=True)


def test_vocabularies(check, specs, files):
    import rdflib

    foaf = rdflib.Namespace('http://xmlns.com/foaf/0.1/')
    lv2 = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
    owl = rdflib.Namespace('http://www.w3.org/2002/07/owl#')
    rdf = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
    rdfs = rdflib.Namespace('http://www.w3.org/2000/01/rdf-schema#')

    # Check if this is a stable LV2 release to enable additional tests
    version_tuple = tuple(map(int, VERSION.split(".")))
    is_stable = version_tuple[1] % 2 == 0 and version_tuple[2] % 2 == 0

    # Check that extended documentation is not in main specification file
    for spec in specs:
        path = str(spec.abspath())
        name = os.path.basename(path)
        name = 'lv2core' if name == 'core' else name
        vocab = os.path.join(path, name + '.ttl')

        spec_model = rdflib.ConjunctiveGraph()
        spec_model.parse(vocab, format='n3')

        def has_statement(s, p, o):
            for t in spec_model.triples([s, p, o]):
                return True

            return False

        check(lambda: not has_statement(None, lv2.documentation, None),
              name = name + ".ttl does not contain lv2:documentation")

    # Check specification manifests
    for spec in specs:
        path = str(spec.abspath())
        manifest_path = os.path.join(path, 'manifest.ttl')
        manifest_model = rdflib.ConjunctiveGraph()
        manifest_model.parse(manifest_path, format='n3')

        uri = manifest_model.value(None, rdf.type, lv2.Specification)
        minor = manifest_model.value(uri, lv2.minorVersion, None)
        micro = manifest_model.value(uri, lv2.microVersion, None)
        check(lambda: uri is not None,
              name = manifest_path + " has a lv2:Specification")
        check(lambda: minor is not None,
              name = manifest_path + " has a lv2:minorVersion")
        check(lambda: micro is not None,
              name = manifest_path + " has a lv2:microVersion")

        if is_stable:
            check(lambda: int(minor) > 0,
                  name = manifest_path + " has even non-zero minor version")
            check(lambda: int(micro) % 2 == 0,
                  name = manifest_path + " has even micro version")

    # Load everything into one big model
    model = rdflib.ConjunctiveGraph()
    for f in files:
        model.parse(f, format='n3')

    # Check that all named and typed resources have labels and comments
    for r in sorted(model.triples([None, rdf.type, None])):
        subject = r[0]
        if (type(subject) == rdflib.term.BNode or
            foaf.Person in model.objects(subject, rdf.type)):
            continue

        def has_property(subject, prop):
            return model.value(subject, prop, None) is not None

        check(lambda: has_property(subject, rdfs.label),
              name = '%s has rdfs:label' % subject)

        if check(lambda: has_property(subject, rdfs.comment),
                 name = '%s has rdfs:comment' % subject):
            comment = str(model.value(subject, rdfs.comment, None))

            check(lambda: comment.endswith('.'),
                  name = "%s comment ends in '.'" % subject)
            check(lambda: comment.find('\n') == -1,
                  name = "%s comment contains no newlines" % subject)
            check(lambda: comment == comment.strip(),
                  name = "%s comment has stripped whitespace" % subject)

        # Check that lv2:documentation, if present, is proper Markdown
        documentation = model.value(subject, lv2.documentation, None)
        if documentation is not None:
            check(lambda: documentation.datatype == lv2.Markdown,
                  name = "%s documentation is explicitly Markdown" % subject)
            check(lambda: str(documentation).startswith('\n\n'),
                  name = "%s documentation starts with blank line" % subject)
            check(lambda: str(documentation).endswith('\n\n'),
                  name = "%s documentation ends with blank line" % subject)

    # Check that all properties are either datatype or object properties
    for r in sorted(model.triples([None, rdf.type, rdf.Property])):
        subject = r[0]

        check(lambda: ((owl.DatatypeProperty in model.objects(subject, rdf.type)) or
                       (owl.ObjectProperty in model.objects(subject, rdf.type)) or
                       (owl.AnnotationProperty in model.objects(subject, rdf.type))),
              name = "%s is a Datatype/Object/Annotation property" % subject)


def test(tst):
    import tempfile

    with tst.group("Data") as check:
        specs = (tst.path.ant_glob('lv2/*', dir=True))
        schemas = list(map(str, tst.path.ant_glob("schemas.lv2/*.ttl")))
        spec_files = list(map(str, tst.path.ant_glob("lv2/**/*.ttl")))
        plugin_files = list(map(str, tst.path.ant_glob("plugins/**/*.ttl")))
        bld_files = list(map(str, tst.path.get_bld().ant_glob("**/*.ttl")))

        if "SERDI" in tst.env and sys.platform != 'win32':
            for f in spec_files:
                with tempfile.NamedTemporaryFile(mode="w") as tmp:
                    base_dir = os.path.dirname(f)
                    cmd = tst.env.SERDI + ["-o", "turtle", f, base_dir]
                    check(cmd, stdout=tmp.name)
                    check.file_equals(f, tmp.name)

        if "SORD_VALIDATE" in tst.env:
            all_files = schemas + spec_files + plugin_files + bld_files
            check(tst.env.SORD_VALIDATE + all_files)

        try:
            test_vocabularies(check, specs, spec_files)
        except ImportError as e:
            Logs.warn('Not running vocabulary tests (%s)' % e)

    with tst.group('Unit') as check:
        pattern = tst.env.cprogram_PATTERN % '**/*-test'
        for test in tst.path.get_bld().ant_glob(pattern):
            check([str(test)])

class Dist(Scripting.Dist):
    def execute(self):
        'Execute but do not call archive() since dist() has already done so.'
        self.recurse([os.path.dirname(Context.g_module.root_path)])

    def get_tar_path(self, node):
        'Resolve symbolic links to avoid broken links in tarball.'
        return os.path.realpath(node.abspath())

class DistCheck(Dist, Scripting.DistCheck):
    def execute(self):
        Dist.execute(self)
        self.check()

    def archive(self):
        Dist.archive(self)

def _get_news_entries(ctx):
    from waflib.extras import autoship

    # Get project-level news entries
    lv2_entries = autoship.read_ttl_news('lv2',
                                         ['lv2/core/meta.ttl',
                                          'lv2/core/people.ttl'],
                                         dist_pattern = dist_pattern)

    release_pattern = r'http://lv2plug.in/spec/lv2-([0-9\.]*).tar.bz2'
    current_version = sorted(lv2_entries.keys(), reverse=True)[0]

    # Add items from every specification
    for specdir in specdirs(ctx.path):
        name = os.path.basename(specdir.abspath())
        files = list(ttl_files(ctx.path, specdir))
        if name == "core":
            files = [f for f in files if (not f.endswith('/meta.ttl') and
                                          not f.endswith('/people.ttl') and
                                          not f.endswith('/manifest.ttl'))]

        entries = autoship.read_ttl_news(name, files)

        def add_items(lv2_version, name, items):
            for item in items:
                lv2_entries[lv2_version]["items"] += ["%s: %s" % (name, item)]

        if entries:
            latest_revision = sorted(entries.keys(), reverse=True)[0]
            for revision, entry in entries.items():
                if "dist" in entry:
                    match = re.match(release_pattern, entry["dist"])
                    if match:
                        # Append news items to corresponding LV2 version
                        version = tuple(map(int, match.group(1).split('.')))
                        add_items(version, name, entry["items"])

                elif revision == latest_revision:
                    # Dev version that isn't in a release yet, append to current
                    add_items(current_version, name, entry["items"])

    # Sort news items in each versions
    for revision, entry in lv2_entries.items():
        entry["items"].sort()

    return lv2_entries

def posts(ctx):
    "generates news posts in Pelican Markdown format"

    from waflib.extras import autoship

    try:
        os.mkdir(os.path.join(out, 'posts'))
    except:
        pass

    autoship.write_posts(_get_news_entries(ctx),
                         os.path.join(out, 'posts'),
                         {'Author': 'drobilla'})

def news(ctx):
    """write an amalgamated NEWS file to the source directory"""

    from waflib.extras import autoship

    autoship.write_news(_get_news_entries(ctx), 'NEWS')

def dist(ctx):
    news(ctx)
    ctx.archive()

def distcheck(ctx):
    news(ctx)
    ctx.archive()
