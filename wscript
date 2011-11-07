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
    opt.add_option('--experimental', action='store_true', default=False,
                   dest='experimental',
                   help='Install unreleased experimental extensions')
    for i in ['core.lv2']:
        opt.recurse(i)

def configure(conf):
    conf.load('compiler_cc')
    conf.load('compiler_cxx')
    autowaf.configure(conf)

    conf.env.append_unique('CFLAGS', '-std=c99')

    subdirs = ['core.lv2']
    subdirs += glob.glob('ext/*.lv2/')
    subdirs += glob.glob('extensions/*.lv2/')

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

    manifests = glob.glob('ext/*.lv2/manifest.ttl')
    manifests += glob.glob('extensions/*.lv2/manifest.ttl')
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
            try:
                subprocess.call(
                    ['./waf', 'distclean', 'configure', 'build', 'distcheck', 'distclean'],
                    cwd=dir)
            except:
                Logs.error('Error building %s release' % name)
        
def lint(ctx):
    for i in (['core.lv2/lv2.h']
              + glob.glob('ext/*/*.h')
              + glob.glob('extensions/*/*.h')):
        subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-whitespace/blank_line,-build/header_guard,-readability/casting,-readability/todo,-build/include ' + i, shell=True)
