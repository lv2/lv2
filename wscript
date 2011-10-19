#!/usr/bin/env python
import datetime
import glob
import os
import rdflib
import shutil
import subprocess

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

lv2 = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
rdf = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')

def genwscript(manifest, experimental):
    dir = os.path.dirname(manifest)
    name = os.path.basename(dir).replace('.lv2', '')

    m = rdflib.ConjunctiveGraph()
    m.parse(manifest, format='n3')

    # Get the first match for a triple pattern, or throw if no matches
    def query(model, s, p, o):
        for i in model.triples([s, p, o]):
            return i
        raise Exception('Insufficient data for %s' % manifest)

    try:
        uri = query(m, None, rdf.type, lv2.Specification)[0]
    except:
        print('Skipping:   %s' % name)
        return False

    minor = micro = '0'
    try:
        minor = query(m, uri, lv2.minorVersion, None)[2]
        micro = query(m, uri, lv2.microVersion, None)[2]
    except:
        if not experimental:
            print('Skipping:   %s' % name)
            return False

    if experimental or (int(minor) != 0 and int(micro) % 2 == 0):
        print('Generating: %s %s.%s' % (name, minor, micro))

        pkgconfig_name = str(uri).replace('http://', 'lv2-').replace('/', '-')

        def subst_file(source_path, target_path):
            source = open(source_path, 'r')
            target = open(target_path, 'w')
            for l in source:
                target.write(l.replace(
                        '@DESCRIPTION@', 'LV2 "' + name + '" extension').replace(
                        '@INCLUDE_PATH@', str(uri).replace('http://', 'lv2/')).replace(
                        '@MICRO@', micro).replace(
                        '@MINOR@', minor).replace(
                        '@NAME@', name).replace(
                        '@PKGCONFIG_NAME@', pkgconfig_name).replace(
                        '@URI@', str(uri)).replace(
                        '@VERSION@', '%s.%s' % (minor, micro)))
            source.close()
            target.close()
            
        # Generate wscript
        subst_file('wscript.template', '%s/wscript' % dir)

        # Generate pkgconfig file
        subst_file('ext.pc.template', '%s/%s.pc.in' % (dir, pkgconfig_name))

        try:
            os.remove('%s/waf' % dir)
        except:
            pass

        #os.symlink('../../waf', '%s/waf' % dir)
        shutil.copy('waf', '%s/waf' % dir)

        return True
    else:
        return False

def configure(conf):
    subdirs = ['core.lv2']

    manifests = glob.glob('ext/*.lv2/manifest.ttl') + [
        'extensions/ui.lv2/manifest.ttl',
        'extensions/units.lv2/manifest.ttl'
    ]

    manifests.sort()

    print('')
    autowaf.display_header("Generating build files")
    for manifest in manifests:
        if genwscript(manifest, Options.options.experimental):
            subdirs += [ os.path.dirname(manifest) ]
    print('')

    conf.load('compiler_cc')
    conf.load('compiler_cxx')

    autowaf.configure(conf)
    conf.env.append_unique('CFLAGS', '-std=c99')

    for i in subdirs:
        conf.recurse(i)

    conf.env['LV2_SUBDIRS'] = subdirs

def build(bld):
    for i in bld.env['LV2_SUBDIRS']:
        bld.recurse(i)

def release(ctx):
    for i in ['ext/data-access.lv2',
              'ext/midi.lv2',
              'ext/event.lv2',
              'ext/uri-map.lv2',
              'ext/instance-access.lv2',
              'extensions/ui.lv2',
              'extensions/units.lv2',
              'core.lv2']:
        try:
            subprocess.call(
                ['./waf', 'distclean', 'configure', 'build', 'distcheck'],
                cwd=i)
        except:
            print('Error building %s release' % i)
        
def lint(ctx):
    for i in (['core.lv2/lv2.h']
              + glob.glob('ext/*/*.h')
              + glob.glob('extensions/*/*.h')):
        subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-whitespace/blank_line,-build/header_guard,-readability/casting,-readability/todo,-build/include ' + i, shell=True)
