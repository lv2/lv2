#!/usr/bin/env python
import os
import sys
import shutil
from waflib.extras import autowaf as autowaf
import waflib.Scripting as Scripting
import waflib.Logs as Logs
import waflib.Options as Options
import waflib.Context as Context
import waflib.Utils as Utils

info = None

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
        m.parse(os.path.join(dir, 'manifest.ttl'), format='n3')

        spec = m.value(None, rdf.type, lv2.Specification)
        name = os.path.basename(spec.replace('http://', ''))
        m.parse(os.path.join(dir, name + '.ttl'), format='n3')
        
        info = type('lv2extinfo', (object,), {
           'NAME'      : str(name),
           'MINOR'     : int(m.value(spec, lv2.minorVersion, None)),
           'MICRO'     : int(m.value(spec, lv2.microVersion, None)),
           'URI'       : str(spec),
           'PKGNAME'   : 'lv2-' + spec.replace('http://', '').replace('/', '-'),
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
    autowaf.set_options(opt)
    opt.add_option('--copy-headers', action='store_true', default=False,
                   dest='copy_headers',
                   help='Copy headers instead of linking to bundle')
    opt.add_option('--experimental', action='store_true', default=False,
                   dest='experimental',
                   help='Install unreleased experimental extensions')

def should_build(ctx):
    top_level = (len(ctx.stack_path) <= 1)
    return top_level or Options.options.experimental or (
        info.MINOR > 0 and info.MICRO % 2 == 0)

def configure(conf):
    if not should_build(conf):
        return

    autowaf.configure(conf)
    autowaf.display_header('LV2 %s Configuration' % info.NAME)
    conf.env['COPY_HEADERS'] = Options.options.copy_headers
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
              target       = info.NAME + '.pc',
              install_path = '${LIBDIR}/pkgconfig',
              INCLUDEDIR   = bld.env['INCLUDEDIR'],
              INCLUDE_PATH = uri.replace('http://', 'lv2/'),
              NAME         = info.NAME,
              VERSION      = VERSION,
              DESCRIPTION  = info.SHORTDESC)
            
    # Install bundle
    bld.install_files(bundle_dir,
                      bld.path.ant_glob('?*.*', excl='*.pc.in'))

    # Install URI-like includes
    if bld.env['COPY_HEADERS']:
        bld.install_files(os.path.join(include_dir, info.NAME),
                          bld.path.ant_glob('*.h'))
    else:
        bld.symlink_as(os.path.join(include_dir, info.NAME),
                       os.path.relpath(bundle_dir, include_dir))
