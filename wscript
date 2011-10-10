#!/usr/bin/env python
import datetime
import os
import subprocess
import glob

from waflib.extras import autowaf as autowaf
import waflib.Logs as Logs

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
    for i in ['core.lv2']: #, 'plugins/eg-amp.lv2', 'plugins/eg-sampler.lv2']:
        opt.recurse(i)

def configure(conf):
    conf.load('compiler_cc')
    conf.load('compiler_cxx')
    autowaf.set_recursive()
    autowaf.configure(conf)
    for i in ['core.lv2']: #, 'plugins/eg-amp.lv2', 'plugins/eg-sampler.lv2']:
        conf.recurse(i)
    conf.env.append_value('CFLAGS', '-std=c99')
    pat = conf.env['cshlib_PATTERN']
    ext = pat[pat.rfind('.'):]
    conf.env.append_value('cshlib_EXTENSION', ext)
    conf.write_config_header('lv2-config.h', remove=False)

def build_extension(bld, name, dir):
    # Bundle
    data_file     = '%s/%s.lv2/%s.ttl' % (dir, name, name)
    manifest_file = '%s/%s.lv2/manifest.ttl' % (dir, name)
    header_files  = '%s/%s.lv2/*.h' % (dir, name)
    bld.install_files('${LV2DIR}/' + name + '.lv2', bld.path.ant_glob(data_file))
    bld.install_files('${LV2DIR}/' + name + '.lv2', bld.path.ant_glob(manifest_file))
    bld.install_files('${LV2DIR}/' + name + '.lv2', bld.path.ant_glob(header_files))

    # URI-style include
    include_dir = bld.env['INCLUDEDIR'] + '/lv2/lv2plug.in/ns/%s' % dir
    bundle_dir  = os.path.join(bld.env['LV2DIR'], name + '.lv2')
    bld.symlink_as(os.path.join(include_dir, name),
                   os.path.relpath(bundle_dir, include_dir))

def build(bld):
    autowaf.set_recursive()
    bld.recurse('core.lv2')
    ext = '''
            atom
            contexts
            cv-port
            data-access
            dyn-manifest
            event
            files
            host-info
            instance-access
            midi
            osc
            parameter
            persist
            port-groups
            presets
            pui
            pui-event
            pui-gtk
            reference
            resize-port
            string-port
            uri-map
            uri-unmap
            urid
    '''
    for e in ext.split():
        build_extension(bld, e, 'ext')

    extensions = '''
            ui
            units
    '''
    for e in extensions.split():
        build_extension(bld, e, 'extensions')

    #for i in ['plugins/eg-amp.lv2', 'plugins/eg-sampler.lv2']:
    #    bld.recurse(i)

def lint(ctx):
    for i in (['core.lv2/lv2.h']
              + glob.glob('ext/*/*.h')
              + glob.glob('extensions/*/*.h')):
        subprocess.call('cpplint.py --filter=+whitespace/comments,-whitespace/tab,-whitespace/braces,-whitespace/labels,-whitespace/blank_line,-build/header_guard,-readability/casting,-readability/todo,-build/include ' + i, shell=True)
