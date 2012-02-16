#!/usr/bin/env python
import glob
import os
import shutil
import sys
from waflib.extras import autowaf as autowaf
from waflib.TaskGen import feature, before
import waflib.Scripting as Scripting
import waflib.Logs as Logs
import waflib.Options as Options
import waflib.Context as Context
import waflib.Utils as Utils

info = None

# A rule for making a link in the build directory to a source file
def link(task):
    func = os.symlink
    if not func:
        func = shutil.copy  # Symlinks unavailable, make a copy

    try:
        os.remove(task.outputs[0].abspath())  # Remove old target
    except:
        pass  # No old target, whatever

    func(task.inputs[0].abspath(), task.outputs[0].abspath())

try:
    # Read version information from lv2extinfo.py (in a release tarball)
    import lv2extinfo
    info = lv2extinfo
except:
    # Read version information from RDF files
    try:
        import rdflib
        doap = rdflib.Namespace('http://usefulinc.com/ns/doap#')
        rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
        lv2  = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')

        dir = sys.path[0]
        m   = rdflib.ConjunctiveGraph()

        for i in glob.glob(os.path.join(dir, '*.ttl')):
            m.parse(i, format='n3')

        spec = m.value(None, rdf.type, lv2.Specification)
        name = os.path.basename(spec.replace('http://', ''))
        
        info = type('lv2extinfo', (object,), {
           'NAME'      : str(name),
           'MINOR'     : int(m.value(spec, lv2.minorVersion, None)),
           'MICRO'     : int(m.value(spec, lv2.microVersion, None)),
           'URI'       : str(spec),
           'PKGNAME'   : str('lv2-' + spec.replace('http://', '').replace('/', '-')),
           'SHORTDESC' : str(m.value(spec, doap.shortdesc, None))})
        
    except:
        e = sys.exc_info()[1]
        Logs.error('Error reading version information: '  + str(e))

if not info:
    Logs.error("Failed to find version information from lv2extinfo.py or RDF")
    sys.exit(1)

# Variables for 'waf dist'
APPNAME = 'lv2-' + info.NAME
VERSION = '%d.%d' % (info.MINOR, info.MICRO)

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c')
    autowaf.set_options(opt)
    opt.add_option('--test', action='store_true', default=False, dest='build_tests',
                   help="Build unit tests")
    opt.add_option('--copy-headers', action='store_true', default=False,
                   dest='copy_headers',
                   help='Copy headers instead of linking to bundle')
    opt.add_option('--experimental', action='store_true', default=False,
                   dest='experimental',
                   help='Install unreleased experimental extensions')

def should_build(ctx):
    top_level = (len(ctx.stack_path) <= 1)
    return top_level or ctx.env['EXPERIMENTAL'] or (
        info.MINOR > 0 and info.MICRO % 2 == 0)

def configure(conf):
    try:
        conf.load('compiler_c')
    except:
        Options.options.build_tests = False

    conf.env['BUILD_TESTS']  = Options.options.build_tests
    conf.env['COPY_HEADERS'] = Options.options.copy_headers
    conf.env['EXPERIMENTAL'] = Options.options.experimental

    if not should_build(conf):
        return

    if not hasattr(os.path, 'relpath') and not Options.options.copy_headers:
        conf.fatal(
            'os.path.relpath missing, get Python 2.6 or use --copy-headers')

    # Check for gcov library (for test coverage)
    if conf.env['BUILD_TESTS'] and not conf.is_defined('HAVE_GCOV'):
        conf.check_cc(lib='gcov', define_name='HAVE_GCOV', mandatory=False)

    autowaf.configure(conf)
    autowaf.display_header('LV2 %s Configuration' % info.NAME)
    autowaf.display_msg(conf, 'LV2 bundle directory', conf.env['LV2DIR'])
    autowaf.display_msg(conf, 'URI', info.URI)
    autowaf.display_msg(conf, 'Version', VERSION)
    autowaf.display_msg(conf, 'Pkgconfig name', info.PKGNAME)
    print('')

def build(bld):
    if not should_build(bld):
        return

    uri          = info.URI
    include_base = os.path.dirname(uri[uri.find('://') + 3:])
    bundle_dir   = os.path.join(bld.env['LV2DIR'], info.NAME + '.lv2')
    include_dir  = os.path.join(bld.env['INCLUDEDIR'], 'lv2', include_base)

    # Pkgconfig file
    obj = bld(features     = 'subst',
              source       = 'ext.pc.in',
              target       = info.PKGNAME + '.pc',
              install_path = '${LIBDIR}/pkgconfig',
              INCLUDEDIR   = bld.env['INCLUDEDIR'],
              INCLUDE_PATH = uri.replace('http://', 'lv2/'),
              NAME         = info.NAME,
              VERSION      = VERSION,
              DESCRIPTION  = info.SHORTDESC)

    if bld.env['BUILD_TESTS'] and bld.path.find_node('%s-test.c' % info.NAME):
        test_lib    = []
        test_cflags = ['']
        if bld.is_defined('HAVE_GCOV'):
            test_lib    += ['gcov']
            test_cflags += ['-fprofile-arcs', '-ftest-coverage']

        # Copy headers to URI-style include paths in build directory
        for i in bld.path.ant_glob('*.h'):
            obj = bld(rule   = link,
                      name   = 'link',
                      cwd    = 'build/lv2/%s/%s' % (include_base, info.NAME),
                      source = '%s' % i,
                      target = 'lv2/%s/%s/%s' % (include_base, info.NAME, i))

        # Unit test program
        obj = bld(features     = 'c cprogram',
                  source       = '%s-test.c' % info.NAME,
                  lib          = test_lib,
                  target       = '%s-test' % info.NAME,
                  install_path = '',
                  cflags       = test_cflags)
            
    # Install bundle
    bld.install_files(bundle_dir,
                      bld.path.ant_glob('?*.*', excl='*.pc.in lv2extinfo.*'))

    # Install URI-like includes
    if bld.env['COPY_HEADERS']:
        bld.install_files(os.path.join(include_dir, info.NAME),
                          bld.path.ant_glob('*.h'))
    else:
        bld.symlink_as(os.path.join(include_dir, info.NAME),
                       os.path.relpath(bundle_dir, include_dir))

def test(ctx):
    autowaf.pre_test(ctx, APPNAME, dirs=['.'])
    os.environ['PATH'] = '.' + os.pathsep + os.getenv('PATH')
    autowaf.run_tests(ctx, APPNAME, ['%s-test' % info.NAME], dirs=['.'])
    autowaf.post_test(ctx, APPNAME, dirs=['.'])

def news(ctx):
    path = ctx.path.abspath()
    autowaf.write_news(APPNAME,
                       glob.glob(os.path.join(path, '*.ttl')),
                       os.path.join(path, 'NEWS'))

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

def pre_dist(ctx):
    # Write lv2extinfo.py in source directory
    path = ctx.path.abspath()
    lv2extinfo_py = open(os.path.join(path, 'lv2extinfo.py'), 'w')
    for i in info.__dict__:
        if i.isupper():
            lv2extinfo_py.write("%s = %s\n" % (i, repr(info.__dict__[i])))
    lv2extinfo_py.close()

    # Write NEWS file in source directory
    news(ctx)

def dist(ctx):
    pre_dist(ctx)
    ctx.archive()
    post_dist(ctx)

def distcheck(ctx):
    dist(ctx)

def post_dist(ctx):
    # Delete generated files from source tree
    path = ctx.path.abspath()
    for i in [os.path.join(path, 'NEWS'),
              os.path.join(path, 'lv2extinfo.py'),
              os.path.join(path, 'lv2extinfo.pyc')]:
        try:
            os.remove(i)
        except:
            pass
