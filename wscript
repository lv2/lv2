#!/usr/bin/env python
import glob
import os
import re
import shutil
import subprocess
import sys

from waflib.extras import autowaf as autowaf
import waflib.Context as Context
import waflib.Logs as Logs
import waflib.Options as Options
import waflib.Scripting as Scripting
import waflib.Utils as Utils

# Variables for 'waf dist'
APPNAME = 'lv2'
VERSION = '1.15.1'

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c')
    opt.load('lv2')
    autowaf.set_options(opt, False, True)
    opt.add_option('--test', action='store_true', dest='build_tests',
                   help='Build unit tests')
    opt.add_option('--no-coverage', action='store_true', dest='no_coverage',
                   help='Do not use gcov for code coverage')
    opt.add_option('--online-docs', action='store_true', dest='online_docs',
                   help='Build documentation for web hosting')
    opt.add_option('--no-plugins', action='store_true', dest='no_plugins',
                   help='Do not build example plugins')
    opt.add_option('--copy-headers', action='store_true', dest='copy_headers',
                   help='Copy headers instead of linking to bundle')
    opt.recurse('lv2/lv2plug.in/ns/lv2core')

def configure(conf):
    try:
        conf.load('compiler_c')
    except:
        Options.options.build_tests = False
        Options.options.no_plugins = True

    conf.load('lv2')
    autowaf.configure(conf)
    autowaf.set_c99_mode(conf)

    if Options.options.online_docs:
        Options.options.docs = True

    if Options.options.ultra_strict:
        conf.env.append_value('CFLAGS', ['-Wconversion'])

    if Options.platform == 'win32' or not hasattr(os.path, 'relpath'):
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

    # Check for gcov library (for test coverage)
    if (conf.env.BUILD_TESTS
        and not Options.options.no_coverage
        and not conf.is_defined('HAVE_GCOV')):
        conf.check_cc(lib='gcov', define_name='HAVE_GCOV', mandatory=False)

    autowaf.set_recursive()

    conf.recurse('lv2/lv2plug.in/ns/lv2core')

    conf.env.LV2_BUILD = ['lv2/lv2plug.in/ns/lv2core']
    if conf.env.BUILD_PLUGINS:
        for i in conf.path.ant_glob('plugins/*', src=False, dir=True):
            try:
                conf.recurse(i.srcpath())
                conf.env.LV2_BUILD += [i.srcpath()]
            except:
                Logs.warn('Configuration failed, %s will not be built\n' % i)

    autowaf.configure(conf)
    autowaf.display_header('LV2 Configuration')
    autowaf.display_msg(conf, 'Bundle directory', conf.env.LV2DIR)
    autowaf.display_msg(conf, 'Copy (not link) headers', conf.env.COPY_HEADERS)
    autowaf.display_msg(conf, 'Version', VERSION)

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
    return ([path.find_node('lv2/lv2plug.in/ns/lv2core')] +
            path.ant_glob('plugins/*', dir=True) +
            path.ant_glob('lv2/lv2plug.in/ns/ext/*', dir=True) +
            path.ant_glob('lv2/lv2plug.in/ns/extensions/*', dir=True))

def ttl_files(path, specdir):
    def abspath(node):
        return node.abspath()

    return map(abspath,
               path.ant_glob(specdir.path_from(path) + '/*.ttl'))

def load_ttl(files):
    import rdflib
    model = rdflib.ConjunctiveGraph()
    for f in files:
        model.parse(f, format='n3')
    return model

# Task to build extension index
def build_index(task):
    sys.path.append('./lv2specgen')
    import rdflib
    import lv2specgen

    doap = rdflib.Namespace('http://usefulinc.com/ns/doap#')
    lv2  = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
    rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')

    model = load_ttl(['lv2/lv2plug.in/ns/lv2core/meta.ttl'])

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

    # Get history for this LV2 release
    entries = lv2specgen.specHistoryEntries(model, proj, {})

    # Add entries for every spec that has the same distribution
    ctx     = task.generator.bld
    subdirs = specdirs(ctx.path)
    for specdir in subdirs:
        m    = load_ttl(ttl_files(ctx.path, specdir))
        name = os.path.basename(specdir.abspath())
        spec = m.value(None, rdf.type, lv2.Specification)
        if spec:
            for dist in dists:
                release = m.value(None, doap['file-release'], dist[1])
                if release:
                    entries[dist] += lv2specgen.releaseChangeset(m, release, str(name))

    # Generate history for all post-unification LV2 distributions
    history = lv2specgen.specHistoryMarkup(entries)

    rows = []
    for f in task.inputs:
        if not f.abspath().endswith('index.html.in'):
            rowfile = open(f.abspath(), 'r')
            rows += rowfile.readlines()
            rowfile.close()

    if date is None:
        import datetime
        date = datetime.datetime.now().isoformat()

    subst_file(task.inputs[0].abspath(), task.outputs[0].abspath(),
               { '@ROWS@': ''.join(rows),
                 '@LV2_VERSION@': VERSION,
                 '@DATE@' : date,
                 '@HISTORY@' : history})

# Task for making a link in the build directory to a source file
def link(task):
    if not task.env.COPY_HEADERS and hasattr(os, 'symlink'):
        func = os.symlink
    else:
        func = shutil.copy  # Symlinks unavailable, make a copy

    try:
        os.remove(task.outputs[0].abspath())  # Remove old target
    except:
        pass  # No old target, whatever

    func(task.inputs[0].abspath(), task.outputs[0].abspath())

def build_ext(bld, path):
    name        = os.path.basename(path)
    bundle_dir  = os.path.join(bld.env.LV2DIR, name + '.lv2')
    include_dir = os.path.join(bld.env.INCLUDEDIR, path)

    # Copy headers to URI-style include paths in build directory
    for i in bld.path.ant_glob(path + '/*.h'):
        bld(rule   = link,
            source = i,
            target = bld.path.get_bld().make_node('%s/%s' % (path, i)))

    # Build test program if applicable
    if bld.env.BUILD_TESTS and bld.path.find_node(path + '/%s-test.c' % name):
        test_lib       = []
        test_cflags    = ['']
        test_linkflags = ['']
        if bld.is_defined('HAVE_GCOV'):
            test_lib       += ['gcov', 'rt']
            test_cflags    += ['--coverage']
            test_linkflags += ['--coverage']

        # Unit test program
        bld(features     = 'c cprogram',
            source       = path + '/%s-test.c' % name,
            lib          = test_lib,
            target       = path + '/%s-test' % name,
            install_path = None,
            cflags       = test_cflags,
            linkflags    = test_linkflags)

    # Install bundle
    bld.install_files(bundle_dir,
                      bld.path.ant_glob(path + '/?*.*', excl='*.in'))

    # Install URI-like includes
    headers = bld.path.ant_glob(path + '/*.h')
    if headers:
        if bld.env.COPY_HEADERS:
            bld.install_files(include_dir, headers)
        else:
            bld.symlink_as(include_dir,
                           os.path.relpath(bundle_dir,
                                           os.path.dirname(include_dir)))

def build(bld):
    exts = (bld.path.ant_glob('lv2/lv2plug.in/ns/ext/*', dir=True) +
            bld.path.ant_glob('lv2/lv2plug.in/ns/extensions/*', dir=True))

    # Copy lv2.h to URI-style include path in build directory
    lv2_h_path = 'lv2/lv2plug.in/ns/lv2core/lv2.h'
    bld(rule   = link,
        source = bld.path.find_node(lv2_h_path),
        target = bld.path.get_bld().make_node(lv2_h_path))

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
    for i in exts:
        build_ext(bld, i.srcpath())

    # Build plugins
    for i in bld.env.LV2_BUILD:
        bld.recurse(i)

    # Install lv2specgen
    bld.install_files('${DATADIR}/lv2specgen/',
                      ['lv2specgen/style.css',
                       'lv2specgen/template.html'])
    bld.install_files('${DATADIR}/lv2specgen/DTD/',
                      bld.path.ant_glob('lv2specgen/DTD/*'))
    bld.install_files('${BINDIR}', 'lv2specgen/lv2specgen.py', chmod=Utils.O755)

    # Install schema bundle
    bld.install_files('${LV2DIR}/schemas.lv2/',
                      bld.path.ant_glob('schemas.lv2/*.ttl'))

    if bld.env.DOCS or bld.env.ONLINE_DOCS:
        # Prepare spec output directories
        specs = exts + [bld.path.find_node('lv2/lv2plug.in/ns/lv2core')]
        for i in specs:
            # Copy spec files to build dir
            for f in bld.path.ant_glob(i.srcpath() + '/*.*'):
                bld(features = 'subst',
                    is_copy  = True,
                    name     = 'copy',
                    source   = f,
                    target   = chop_lv2_prefix(f.srcpath()))

            base = i.srcpath()[len('lv2/lv2plug.in'):]
            name = os.path.basename(i.srcpath())

            # Generate .htaccess file
            if bld.env.ONLINE_DOCS:
                bld(features     = 'subst',
                    source       = 'doc/htaccess.in',
                    target       = os.path.join(base, '.htaccess'),
                    install_path = None,
                    NAME         = name,
                    BASE         = base)


        # Copy stylesheets to build directory
        for i in ['style.css', 'pygments.css']:
            bld(features = 'subst',
                is_copy  = True,
                name     = 'copy',
                source   = 'doc/%s' % i,
                target   = 'aux/%s' % i)

        bld(features = 'subst',
            is_copy  = True,
            name     = 'copy',
            source   = 'doc/doxy-style.css',
            target   = 'doc/html/doxy-style.css')

        # Build Doxygen documentation (and tags file)
        autowaf.build_dox(bld, 'LV2', VERSION, top, out, 'lv2plug.in/doc', False)
        bld.add_group()

        index_files = []
        for i in specs:
            # Call lv2specgen to generate spec docs
            name         = os.path.basename(i.srcpath())
            index_file   = os.path.join('index_rows', name)
            index_files += [index_file]
            root_path    = os.path.relpath('lv2/lv2plug.in/ns', name)
            html_path    = '%s/%s.html' % (chop_lv2_prefix(i.srcpath()), name)
            out_bundle   = os.path.dirname(html_path)
            bld(rule = '../lv2specgen/lv2specgen.py --root=' + root_path +
                ' --list-email=devel@lists.lv2plug.in'
                ' --list-page=http://lists.lv2plug.in/listinfo.cgi/devel-lv2plug.in'
                ' --style-uri=' + os.path.relpath('aux/style.css', out_bundle) +
                ' --docdir=' + os.path.relpath('doc/html', os.path.dirname(html_path)) +
                ' --tags=doc/tags' +
                ' --index=' + index_file +
                ' ${SRC} ${TGT}',
                source = os.path.join(i.srcpath(), name + '.ttl'),
                target = [html_path, index_file])

            # Install documentation
            if not bld.env.ONLINE_DOCS:
                base = chop_lv2_prefix(i.srcpath())
                bld.install_files('${DOCDIR}/' + i.srcpath(),
                                  bld.path.get_bld().ant_glob(base + '/*.html'))

        index_files.sort()
        bld.add_group()

        # Build extension index
        bld(rule   = build_index,
            name   = 'index',
            source = ['lv2/lv2plug.in/ns/index.html.in'] + index_files,
            target = 'ns/index.html')

        # Install main documentation files
        if not bld.env.ONLINE_DOCS:
            bld.install_files('${DOCDIR}/lv2/lv2plug.in/aux/', 'aux/style.css')
            bld.install_files('${DOCDIR}/lv2/lv2plug.in/ns/', 'ns/index.html')

    if bld.env.BUILD_TESTS:
        # Generate a compile test .c file that includes all headers
        def gen_build_test(task):
            out = open(task.outputs[0].abspath(), 'w')
            for i in task.inputs:
                out.write('#include "%s"\n' % i.bldpath())
            out.write('int main(void) { return 0; }\n')
            out.close()

        bld(rule         = gen_build_test,
            source       = bld.path.ant_glob('lv2/**/*.h'),
            target       = 'build-test.c',
            install_path = None)

        bld(features     = 'c cprogram',
            source       = bld.path.get_bld().make_node('build-test.c'),
            target       = 'build-test',
            install_path = None)

    if bld.env.BUILD_BOOK:
        # Build "Programming LV2 Plugins" book from plugin examples
        bld.recurse('plugins')

def lint(ctx):
    for i in ctx.path.ant_glob('lv2/**/*.h'):
        subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-build/include,-runtime/sizeof ' + i.abspath(), shell=True)

def test(ctx):
    "runs unit tests"
    autowaf.pre_test(ctx, APPNAME, dirs=['.'])
    for i in ctx.path.ant_glob('**/*-test'):
        name = os.path.basename(i.abspath())
        os.environ['PATH'] = '.' + os.pathsep + os.getenv('PATH')
        autowaf.run_test(
            ctx, APPNAME, i.path_from(ctx.path.find_node('build')), dirs=['.'], name=i)
    autowaf.post_test(ctx, APPNAME, dirs=['.'])

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

def posts(ctx):
    "generates news posts in Pelican Markdown format"
    subdirs = specdirs(ctx.path)
    dev_dist = 'http://lv2plug.in/spec/lv2-%s.tar.bz2' % VERSION

    try:
        os.mkdir(os.path.join(out, 'posts'))
    except:
        pass

    # Get all entries (as in dist())
    top_entries = {}
    for specdir in subdirs:
        entries = autowaf.get_rdf_news(os.path.basename(specdir.abspath()),
                                       ttl_files(ctx.path, specdir),
                                       top_entries,
                                       dev_dist = dev_dist)

    entries = autowaf.get_rdf_news('lv2',
                                   ['lv2/lv2plug.in/ns/lv2core/meta.ttl'],
                                   None,
                                   top_entries,
                                   dev_dist = dev_dist)

    autowaf.write_posts(entries,
                        { 'Author': 'drobilla' },
                        os.path.join(out, 'posts'))

def dist(ctx):
    subdirs  = specdirs(ctx.path)
    dev_dist = 'http://lv2plug.in/spec/lv2-%s.tar.bz2' % VERSION

    # Write NEWS files in source directory
    top_entries = {}
    for specdir in subdirs:
        entries = autowaf.get_rdf_news(os.path.basename(specdir.abspath()),
                                       ttl_files(ctx.path, specdir),
                                       top_entries,
                                       dev_dist = dev_dist)
        autowaf.write_news(entries, specdir.abspath() + '/NEWS')

    # Write top level amalgamated NEWS file
    entries = autowaf.get_rdf_news('lv2',
                                   ['lv2/lv2plug.in/ns/lv2core/meta.ttl'],
                                   None,
                                   top_entries,
                                   dev_dist = dev_dist)
    autowaf.write_news(entries, 'NEWS')

    # Build archive
    ctx.archive()

    # Delete generated NEWS files from source directory
    for i in subdirs + [ctx.path]:
        try:
            os.remove(os.path.join(i.abspath(), 'NEWS'))
        except:
            pass
