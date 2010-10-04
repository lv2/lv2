#!/usr/bin/env python
import autowaf

# Version of this package (even if built as a child)
LV2EXT_VERSION = '0.0.0'

# Variables for 'waf dist'
APPNAME = 'lv2ext'
VERSION = LV2EXT_VERSION

# Mandatory variables
srcdir = '.'
blddir = 'build'

def set_options(opt):
	autowaf.set_options(opt)
	opt.tool_options('compiler_cc')
	opt.tool_options('compiler_cxx')

def configure(conf):
	autowaf.configure(conf)
	conf.check_tool('compiler_cc')
	conf.check_tool('compiler_cxx')
	conf.env.append_value('CCFLAGS', '-std=c99')
	pat = conf.env['shlib_PATTERN']
	ext = pat[pat.rfind('.'):]
	conf.env.append_value('shlib_EXTENSION', ext)
	
def build_plugin(bld, lang, name):
	ext = bld.env['shlib_EXTENSION'][0]

	penv = bld.env.copy()
	penv['shlib_PATTERN'] = '%s' + ext

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
	bld.install_files('${LV2DIR}/' + name + '.lv2', data_file)
	bld.install_files('${LV2DIR}/' + name + '.lv2', manifest_file)

def build_extension(bld, name, dir):
	data_file     = '%s/%s.lv2/%s.ttl' % (dir, name, name)
	manifest_file = '%s/%s.lv2/manifest.ttl' % (dir, name)
	bld.install_files('${LV2DIR}/' + name + '.lv2', data_file)
	bld.install_files('${LV2DIR}/' + name + '.lv2', manifest_file)

def build(bld):
	ext = '''
		atom
		atom-port
		command
		contexts
		data-access
		dyn-manifest
		event
		host-info
		instance-access
		midi
		osc
		parameter
		polymorphic-port
		port-groups
		presets
		string-port
		uri-map
		variables
	'''
	for e in ext.split():
		build_extension(bld, e, './ext')
	
	extensions = '''
		ui
		units
	'''
	for e in extensions.split():
		build_extension(bld, e, './extensions')

