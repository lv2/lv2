#!/usr/bin/env python
import datetime
import os
import autowaf

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
	conf.env.append_value('CCFLAGS', '-std=c99')
	pat = conf.env['cshlib_PATTERN']
	ext = pat[pat.rfind('.'):]
	conf.env.append_value('cshlib_EXTENSION', ext)

def build_plugin(bld, lang, name):
	ext = bld.env['cshlib_EXTENSION'][0]

	penv = bld.env.copy()
	penv['cshlib_PATTERN'] = '%s' + ext

	# Library
	ext = 'c'
	if lang != 'cc':
		ext = 'cpp'

	obj              = bld.new_task_gen(lang, 'shlib')
	obj.env          = penv
	obj.source       = [ 'plugins/%s.lv2/%s.%s' % (name, name, ext) ]
	obj.includes     = ['.', './ext']
	obj.name         = name
	obj.target       = name
	obj.install_path = '${LV2DIR}/' + name + '.lv2'

	if lang == 'cxx':
		obj.source += [ 'ext/lv2plugin.cpp' ]

	# Data
	data_file     = 'plugins/%s.lv2/%s.ttl' % (name, name)
	manifest_file = 'plugins/%s.lv2/manifest.ttl' % (name)
	bld.install_files('${LV2DIR}/' + name + '.lv2', bld.path.ant_glob(data_file))
	bld.install_files('${LV2DIR}/' + name + '.lv2', bld.path.ant_glob(manifest_file))

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

