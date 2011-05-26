#!/usr/bin/env python
# -*- coding: utf-8 -*-

import RDF
import glob
import os
import re
import shutil
import sys

lv2  = RDF.NS('http://lv2plug.in/ns/lv2core#')

manifests = glob.glob('ext/*.lv2/manifest.ttl')

try:
    os.mkdir('build')
    os.mkdir('spec')
except:
    pass

for i in manifests:
    match = re.search('.*/([^/]*).lv2/.*', i)
    name = match.group(1)

    match = re.search('(.*)/.*', i)
    dir = match.group(1)

    m = RDF.Model()
    p = RDF.Parser(name="turtle")
    p.parse_into_model(m, 'file:' + i)

    s = m.find_statements(RDF.Statement(None, lv2.minorVersion, None))
    if not s.current():
        #print("No minor version found for %s\n" % i)
        continue
    minor = s.current().object.literal_value['string']

    s = m.find_statements(RDF.Statement(None, lv2.microVersion, None))
    if not s.current():
        #print("No micro version found for %s\n" % i)
        continue
    micro = s.current().object.literal_value['string']

    if int(minor) != 0 and int(micro) % 2 == 0:
        print('Packaging %s extension version %s.%s' % (name, minor, micro))
        wscript_template = open('wscript.template')
        wscript = open('%s/wscript' % dir, 'w')
        for l in wscript_template:
            wscript.write(l.replace(
                    '@NAME@', name).replace(
                    '@MINOR@', minor).replace(
                    '@MICRO@', micro))
        wscript_template.close()
        wscript.close()
        try:
            os.remove('%s/waf' % dir)
        except:
            pass
        os.symlink('../../waf', '%s/waf' % dir)

        os.system('tar --exclude=".*" -cjhf build/spec/lv2-%s-%s.%s.tar.bz2 %s' % (
                name, minor, micro, dir))

