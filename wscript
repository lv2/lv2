#!/usr/bin/env python
import datetime
import glob
import os
import shutil
import subprocess
import sys

from waflib.extras import autowaf as autowaf
import waflib.Context as Context
import waflib.Logs as Logs
import waflib.Options as Options
import waflib.Scripting as Scripting

# Variables for 'waf dist'
APPNAME = 'lv2'
VERSION = '1.0.5'

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_cc')
    opt.load('compiler_cxx')
    autowaf.set_options(opt)
    opt.add_option('--test', action='store_true', default=False,
                   dest='build_tests', help="Build unit tests")
    opt.add_option('--no-plugins', action='store_true', default=False,
                   dest='no_plugins', help="Do not build example plugins")
    opt.add_option('--copy-headers', action='store_true', default=False,
                   dest='copy_headers',
                   help='Copy headers instead of linking to bundle')
    for i in ['lv2/lv2plug.in/ns/lv2core']:
        opt.recurse(i)

def get_subdirs(with_plugins=True):
    subdirs = ['lv2/lv2plug.in/ns/lv2core/',
               'lv2/lv2plug.in/ns/meta/']
    subdirs += glob.glob('lv2/lv2plug.in/ns/ext/*/')
    subdirs += glob.glob('lv2/lv2plug.in/ns/extensions/*/')
    if with_plugins:
        subdirs += glob.glob('plugins/*/')
    return subdirs
    
def configure(conf):
    try:
        conf.load('compiler_c')
        conf.load('compiler_cxx')
    except:
        Options.options.build_tests = False
        Options.options.no_plugins = True

    autowaf.configure(conf)
    autowaf.set_recursive()

    if conf.env['MSVC_COMPILER']:
        conf.env.append_unique('CFLAGS', ['-TP', '-MD'])
    else:
        conf.env.append_unique('CFLAGS', '-std=c99')

    conf.env['BUILD_TESTS']   = Options.options.build_tests
    conf.env['BUILD_PLUGINS'] = not Options.options.no_plugins
    conf.env['COPY_HEADERS']  = Options.options.copy_headers

    if not hasattr(os.path, 'relpath') and not Options.options.copy_headers:
        conf.fatal(
            'os.path.relpath missing, get Python 2.6 or use --copy-headers')

    # Check for gcov library (for test coverage)
    if conf.env['BUILD_TESTS'] and not conf.is_defined('HAVE_GCOV'):
        if conf.env['MSVC_COMPILER']:
            conf.env.append_unique('CFLAGS', ['-TP', '-MD'])
        else:
            conf.env.append_unique('CFLAGS', '-std=c99')
        conf.check_cc(lib='gcov', define_name='HAVE_GCOV', mandatory=False)

    subdirs = get_subdirs(conf.env['BUILD_PLUGINS'])

    for i in subdirs:
        try:
            conf.recurse(i)
        except:
            Logs.warn('Configuration failed, %s will not be built\n' % i)
            subdirs.remove(i)

    conf.env['LV2_SUBDIRS'] = subdirs

    autowaf.configure(conf)
    autowaf.display_header('LV2 Configuration')
    autowaf.display_msg(conf, 'Bundle directory', conf.env['LV2DIR'])
    autowaf.display_msg(conf, 'Version', VERSION)

# Rule for copying a file to the build directory
def copy(task):
    shutil.copy(task.inputs[0].abspath(), task.outputs[0].abspath())

def chop_lv2_prefix(s):
    if s.startswith('lv2/lv2plug.in/'):
        return s[len('lv2/lv2plug.in/'):]
    return s

# Rule for calling lv2specgen on a spec bundle
def specgen(task):
    import rdflib
    doap = rdflib.Namespace('http://usefulinc.com/ns/doap#')
    lv2  = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
    owl  = rdflib.Namespace('http://www.w3.org/2002/07/owl#')
    rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')

    sys.path.append("./lv2specgen")
    import lv2specgen

    spec   = task.inputs[0]
    path   = os.path.dirname(spec.srcpath())
    outdir = os.path.abspath(os.path.join(out, chop_lv2_prefix(path)))

    bundle = str(outdir)
    b = os.path.basename(outdir)

    if not os.access(spec.abspath(), os.R_OK):
        print('warning: extension %s has no %s.ttl file' % (b, b))
        return

    try:
        model = rdflib.ConjunctiveGraph()
        for i in glob.glob('%s/*.ttl' % bundle):
            model.parse(i, format='n3')
    except:
        e = sys.exc_info()[1]
        print('error parsing %s: %s' % (bundle, str(e)))
        return

    # Get extension URI
    ext_node = model.value(None, rdf.type, lv2.Specification)
    if not ext_node:
        print('no extension found in %s' % bundle)
        return
    
    ext = str(ext_node)

    # Get version
    minor = 0
    micro = 0
    try:
        minor = int(model.value(ext_node, lv2.minorVersion, None))
        micro = int(model.value(ext_node, lv2.microVersion, None))
    except:
        e = sys.exc_info()[1]
        print("warning: %s: failed to find version for %s" % (bundle, ext))

    # Get date
    date = None
    for r in model.triples([ext_node, doap.release, None]):
        revision = model.value(r[2], doap.revision, None)
        if revision == ("%d.%d" % (minor, micro)):
            date = model.value(r[2], doap.created, None)
            break

    # Verify that this date is the latest
    for r in model.triples([ext_node, doap.release, None]):
        revision = model.value(r[2], doap.revision, None)
        this_date = model.value(r[2], doap.created, None)
        if this_date > date:
            print("warning: %s revision %d.%d (%s) is not the latest release" % (
                ext_node, minor, micro, date))
            break
    
    # Get short description
    shortdesc = model.value(ext_node, doap.shortdesc, None)

    SPECGENDIR = 'lv2specgen'
    STYLEPATH  = 'build/aux/style.css'
    TAGFILE    = 'build/tags'

    specdoc = lv2specgen.specgen(
        spec.abspath(),
        SPECGENDIR,
        os.path.relpath(STYLEPATH, bundle),
        os.path.relpath('build/doc/html', bundle),
        TAGFILE,
        instances=True)

    lv2specgen.save(task.outputs[0].abspath(), specdoc)

    # Name (comment is to act as a sort key)
    row = '<tr><!-- %s --><td><a rel="rdfs:seeAlso" href="%s">%s</a></td>' % (
        b, path[len('lv2/lv2plug.in/ns/'):], b)

    # Description
    if shortdesc:
        row += '<td>' + str(shortdesc) + '</td>'
    else:
        row += '<td></td>'

    # Version
    version_str = '%s.%s' % (minor, micro)
    if minor == 0 or (micro % 2 != 0):
        row += '<td><span style="color: red">' + version_str + '</span></td>'
    else:
        row += '<td>' + version_str + '</td>'

    # Status
    deprecated = model.value(ext_node, owl.deprecated, None)
    if minor == 0:
        row += '<td><span class="error">Experimental</span></td>'
    elif deprecated and str(deprecated[2]) != "false":
        row += '<td><span class="warning">Deprecated</span></td>'
    elif micro % 2 == 0:
        row += '<td><span class="success">Stable</span></td>'

    row += '</tr>'

    index = open(os.path.join('build', 'index_rows', b), 'w')
    index.write(row)
    index.close()

def subst_file(template, output, dict):
    i = open(template, 'r')
    o = open(output, 'w')
    for line in i:
        for key in dict:
            line = line.replace(key, dict[key])
        o.write(line)
    i.close()
    o.close()

# Task to build extension index
def build_index(task):
    global index_lines
    rows = []
    for f in task.inputs:
        if not f.abspath().endswith('index.html.in'):
            rowfile = open(f.abspath(), 'r')
            rows += rowfile.readlines()
            rowfile.close()

    subst_file(task.inputs[0].abspath(), task.outputs[0].abspath(),
               { '@ROWS@': ''.join(rows),
                 '@TIME@': datetime.datetime.utcnow().strftime('%F %H:%M UTC') })

def build(bld):
    for i in bld.env['LV2_SUBDIRS']:
        bld.recurse(i)

    # LV2 pkgconfig file
    bld(features     = 'subst',
        source       = 'lv2.pc.in',
        target       = 'lv2.pc',
        install_path = '${LIBDIR}/pkgconfig',
        PREFIX       = bld.env['PREFIX'],
        INCLUDEDIR   = bld.env['INCLUDEDIR'],
        VERSION      = VERSION)

    if bld.env['DOCS']:
        # Build Doxygen documentation (and tags file)
        autowaf.build_dox(bld, 'LV2', VERSION, top, out)

        # Copy stylesheet to build directory
        bld(rule     = copy,
            name     = 'copy',
            source   = 'doc/style.css',
            target   = 'aux/style.css')

        index_files = []

        # Prepare spec output directories
        for i in bld.env['LV2_SUBDIRS']:
            if i.startswith('lv2/lv2plug.in'):
                # Copy spec files to build dir
                for f in bld.path.ant_glob(i + '*.*'):
                    bld(rule   = copy,
                        name   = 'copy',
                        source = f,
                        target = chop_lv2_prefix(f.srcpath()))

                base = i[len('lv2/lv2plug.in'):]
                name = os.path.basename(i[:len(i)-1])

                # Generate .htaccess file
                bld(features     = 'subst',
                    source       = 'doc/htaccess.in',
                    target       = os.path.join(base, '.htaccess'),
                    install_path = None,
                    NAME         = name,
                    BASE         = base)

        # Call lv2specgen for each spec
        for i in bld.env['LV2_SUBDIRS']:
            if i.startswith('lv2/lv2plug.in'):
                name = os.path.basename(i[:len(i)-1])
                index_file = os.path.join('index_rows', name)
                index_files += [index_file]

                bld.add_group()  # Barrier (don't call lv2specgen in parallel)

                # Call lv2specgen to generate spec docs
                bld(rule   = specgen,
                    name   = 'specgen',
                    source = os.path.join(i, name + '.ttl'),
                    target = ['%s%s.html' % (chop_lv2_prefix(i), name),
                              index_file])

        index_files.sort()
        bld.add_group()  # Barrier (wait for lv2specgen to build index)

        # Build extension index
        bld(rule   = build_index,
            name   = 'index',
            source = ['lv2/lv2plug.in/ns/index.html.in'] + index_files,
            target = 'ns/index.html')

    if bld.env['BUILD_TESTS']:
        # Generate a compile test .c file that includes all headers
        def gen_build_test(task):
            out = open(task.outputs[0].abspath(), 'w')
            for i in task.inputs:
                out.write('#include "%s"\n' % i.bldpath())
            out.write('int main() { return 0; }\n')
            out.close()

        bld(rule         = gen_build_test,
            source       = bld.path.ant_glob('lv2/**/*.h'),
            target       = 'build_test.c',
            install_path = None)

        bld(features     = 'c',
            source       = bld.path.get_bld().make_node('build_test.c'),
            target       = 'build_test',
            install_path = None)

def lint(ctx):
    for i in (['lv2/lv2plug.in/ns/lv2core/lv2.h']
              + glob.glob('lv2/lv2plug.in/ns/ext/*/*.h')
              + glob.glob('lv2/lv2plug.in/ns/extensions/*/*.h')
              + glob.glob('plugins/*/*.c') + glob.glob('plugins/*.*.h')):
        subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-build/header_guard,-readability/casting,-build/include,-runtime/sizeof ' + i, shell=True)

class Dist(Scripting.Dist):
    def execute(self):
        "Execute but do not call archive() since dist() has already done so."
        self.recurse([os.path.dirname(Context.g_module.root_path)])

    def get_tar_path(self, node):
        "Resolve symbolic links to avoid broken links in tarball."
        return os.path.realpath(node.abspath())

class DistCheck(Dist, Scripting.DistCheck):
    def execute(self):
        Dist.execute(self)
        self.check()
    
    def archive(self):
        Dist.archive(self)

def news(ctx):
    path = ctx.path.abspath()
    autowaf.write_news('lv2',
                       [os.path.join(path, 'lv2/lv2plug.in/ns/meta/meta.ttl')],
                       'NEWS')

def pre_dist(ctx):
    # Write NEWS file in source directory
    news(ctx)

def post_dist(ctx):
    # Delete generated NEWS file from source directory
    try:
        os.remove(os.path.join(ctx.path.abspath(), 'NEWS'))
    except:
        pass

def dist(ctx):
    ctx.recurse(['.'] + get_subdirs(False), name='pre_dist')
    ctx.archive()
    ctx.recurse(['.'] + get_subdirs(False), name='post_dist')
