#!/usr/bin/env python
import glob
import os
import shutil
import sys
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
    autowaf.set_options(opt)
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
    conf.env['EXPERIMENTAL'] = Options.options.experimental
    if not should_build(conf):
        return

    if not hasattr(os.path, 'relpath') and not Options.options.copy_headers:
        conf.fatal(
            'os.path.relpath missing, get Python 2.6 or use --copy-headers')

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
              target       = info.PKGNAME + '.pc',
              install_path = '${LIBDIR}/pkgconfig',
              INCLUDEDIR   = bld.env['INCLUDEDIR'],
              INCLUDE_PATH = uri.replace('http://', 'lv2/'),
              NAME         = info.NAME,
              VERSION      = VERSION,
              DESCRIPTION  = info.SHORTDESC)
            
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

def write_news():
    import rdflib
    import textwrap
    from time import strftime, strptime

    doap = rdflib.Namespace('http://usefulinc.com/ns/doap#')
    dcs  = rdflib.Namespace('http://ontologi.es/doap-changeset#')
    rdfs = rdflib.Namespace('http://www.w3.org/2000/01/rdf-schema#')
    foaf = rdflib.Namespace('http://xmlns.com/foaf/0.1/')
    rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')

    m = rdflib.ConjunctiveGraph()

    try:
        for i in glob.glob('*.ttl'):
            m.parse(i, format='n3')
    except:
        print('warning: error parsing data, unable to generate NEWS')
        return

    spec = m.value(None, rdf.type, doap.Project)

    entries = {}
    for r in m.triples([spec, doap.release, None]):
        release   = r[2]
        revision  = m.value(release, doap.revision, None) or '9999'
        date      = m.value(release, doap.created, None) or '9999-01-01'
        blamee    = m.value(release, dcs.blame, None)
        changeset = m.value(release, dcs.changeset, None)

        entry = '%s (%s) stable;\n' % (APPNAME, revision)

        if changeset:
            for i in m.triples([changeset, dcs.item, None]):
                entry += '\n  * ' + '\n    '.join(
                    textwrap.wrap(m.value(i[2], rdfs.label, None), width=79))

        entry += '\n\n -- %s <%s>  %s\n\n' % (
            m.value(blamee, foaf.name, None),
            m.value(blamee, foaf.mbox, None).replace('mailto:', ''),
            strftime('%a, %d %b %Y %H:%M:%S +0000', strptime(date, '%Y-%m-%d')))

        entries[revision] = entry

    news = open('NEWS', 'w')
    for e in sorted(entries.keys(), reverse=True):
        news.write(entries[e])
    news.close()

class Dist(Scripting.Dist):
    fun = 'dist'
    cmd = 'dist'

    def get_tar_path(self, node):
        "Resolve symbolic links to avoid broken links in tarball."
        return os.path.realpath(node.abspath())

    def archive(self):
        # Generate lv2extinfo.py in source tree
        lv2extinfo_py = open('lv2extinfo.py', 'w')
        for i in info.__dict__:
            if i.isupper():
                lv2extinfo_py.write("%s = %s\n" % (i, repr(info.__dict__[i])))
        lv2extinfo_py.close()

        # Write NEWS file
        write_news()

        # Build distribution
        Scripting.Dist.archive(self)

        # Delete generated files from source tree
        for i in ['NEWS', 'lv2extinfo.py', 'lv2extinfo.pyc']:
            try:
                os.remove(i)
            except:
                pass

class DistCheck(Dist, Scripting.DistCheck):
    fun = 'distcheck'
    cmd = 'distcheck'

    def archive(self):
        Dist.archive(self)
