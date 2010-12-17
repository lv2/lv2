#!/usr/bin/env python
import autowaf
import datetime
import os

import Logs

# Version of this package (even if built as a child)
LV2EXT_VERSION = datetime.date.isoformat(datetime.datetime.now()).replace('-', '.')

# Variables for 'waf dist'
APPNAME = 'lv2plug.in'
VERSION = LV2EXT_VERSION

# Mandatory variables
top = '.'
out = 'build'

def options(opt):
	autowaf.set_options(opt)
	opt.tool_options('compiler_cc')
	opt.tool_options('compiler_cxx')
	opt.sub_options('core.lv2')

def configure(conf):
	autowaf.set_recursive()
	autowaf.configure(conf)
	conf.sub_config('core.lv2');
	conf.check_tool('compiler_cc')
	conf.check_tool('compiler_cxx')
	conf.env.append_value('CFLAGS', '-std=c99')
	pat = conf.env['cshlib_PATTERN']
	ext = pat[pat.rfind('.'):]
	conf.env.append_value('cshlib_EXTENSION', ext)

def build_extension(bld, name, dir):
	data_file     = '%s/%s.lv2/%s.ttl' % (dir, name, name)
	manifest_file = '%s/%s.lv2/manifest.ttl' % (dir, name)
	header_files  = '%s/%s.lv2/*.h' % (dir, name)
	bld.install_files('${LV2DIR}/' + name + '.lv2', bld.path.ant_glob(data_file))
	bld.install_files('${LV2DIR}/' + name + '.lv2', bld.path.ant_glob(manifest_file))
	bld.install_files('${LV2DIR}/' + name + '.lv2', bld.path.ant_glob(header_files))

def build(bld):
	autowaf.set_recursive()
	bld.add_subdirs('core.lv2')
	ext = '''
		atom
		contexts
		data-access
		dyn-manifest
		event
		host-info
		instance-access
		midi
		osc
		parameter
		port-groups
		presets
		resize-port
		string-port
		uri-map
		uri-unmap
	'''
	for e in ext.split():
		build_extension(bld, e, 'ext')

	extensions = '''
		ui
		units
	'''
	for e in extensions.split():
		build_extension(bld, e, 'extensions')

	bld.add_post_fun(warn_lv2config)

def warn_lv2config(ctx):
	if ctx.cmd == 'install':
		Logs.pprint('BOLD', '''
***************************************************************************
* LV2 Extension(s) Installed                                              *
* You need to run lv2config to compile against extension headers          *
* e.g. $ sudo ldconfig                                                    *
* (If you are packaging, extension packages MUST do this on installation) *
***************************************************************************
''')
