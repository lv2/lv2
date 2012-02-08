#!/usr/bin/env python
import datetime
import glob
import os
import rdflib
import shutil
import subprocess
import sys

from waflib.extras import autowaf as autowaf
import waflib.Logs as Logs
import waflib.Options as Options

# Version of this package (even if built as a child)
LV2EXT_VERSION = datetime.date.isoformat(datetime.datetime.now()).replace('-', '.')

# Variables for 'waf dist'
APPNAME = 'lv2plug.in'
VERSION = LV2EXT_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_cc')
    opt.load('compiler_cxx')
    autowaf.set_options(opt)
    opt.add_option('--test', action='store_true', default=False, dest='build_tests',
                   help="Build unit tests")
    opt.add_option('--experimental', action='store_true', default=False,
                   dest='experimental',
                   help='Install unreleased experimental extensions')
    for i in ['lv2/ns/lv2core']:
        opt.recurse(i)

def configure(conf):
    conf.load('compiler_cc')
    conf.load('compiler_cxx')
    autowaf.configure(conf)

    conf.env.append_unique('CFLAGS', '-std=c99')

    subdirs = ['lv2/ns/lv2core']
    subdirs += glob.glob('lv2/ns/ext/*/')
    subdirs += glob.glob('lv2/ns/extensions/*/')

    for i in subdirs:
        conf.recurse(i)

    conf.env['LV2_SUBDIRS'] = subdirs

def build(bld):
    for i in bld.env['LV2_SUBDIRS']:
        bld.recurse(i)

def release(ctx):
    lv2  = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
    rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
    doap = rdflib.Namespace('http://usefulinc.com/ns/doap#')

    try:
        shutil.rmtree('build/spec')
    except:
        pass

    os.makedirs('build/spec')

    manifests = glob.glob('lv2/ns/lv2core/manifest.ttl')
    manifests += glob.glob('lv2/ns/*/*/manifest.ttl')
    for manifest in manifests:
        dir = os.path.dirname(manifest)
        name = os.path.basename(dir).replace('.lv2', '')

        m = rdflib.ConjunctiveGraph()
        m.parse(manifest, format='n3')

        uri = minor = micro = None
        try:
            spec  = m.value(None, rdf.type, lv2.Specification)
            uri   = str(spec)
            minor = int(m.value(spec, lv2.minorVersion, None))
            micro = int(m.value(spec, lv2.microVersion, None))
        except:
            e = sys.exc_info()[1]
            Logs.error('error: %s: %s' % (manifest, str(e)))
            continue

        if minor != 0 and micro % 2 == 0:
            autowaf.display_header('\nBuilding %s Release\n' % dir)
            try:
                subprocess.call(
                    ['./waf', 'distclean', 'configure', 'build', 'distcheck'],
                    cwd=dir)
                for i in glob.glob(dir + '/*.tar.bz2'):
                    shutil.move(i, 'build/spec')
            except:
                Logs.error('Error building %s release' % (name, e))

            subprocess.call(['./waf', 'distclean'], cwd=dir)
        
def lint(ctx):
    for i in (['lv2/ns/lv2core/lv2.h']
              + glob.glob('lv2/ns/ext/*/*.h')
              + glob.glob('lv2/ns/extensions/*/*.h')):
        subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-whitespace/blank_line,-build/header_guard,-readability/casting,-readability/todo,-build/include ' + i, shell=True)
