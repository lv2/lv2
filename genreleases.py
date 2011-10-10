#!/usr/bin/env python
# -*- coding: utf-8 -*-

import glob
import os

import genwscript

manifests = glob.glob('ext/*.lv2/manifest.ttl')
manifests += ['extensions/ui.lv2/manifest.ttl']
manifests += ['extensions/units.lv2/manifest.ttl']

try: os.mkdir('build')
except: pass

try: os.mkdir('build/spec')
except: pass

for i in manifests:
    genwscript.genwscript(i)
    
