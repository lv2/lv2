#!/usr/bin/env python
# -*- coding: utf-8 -*-

import rdflib
import glob
import os
import shutil

lv2 = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
rdf = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')

# Get the first match for a triple pattern, or throw if no matches
def query(model, s, p, o):
    triples = model.triples([s, p, o])
    for i in triples:
        return i
    raise Exception('Bad LV2 extension data')

def genwscript(manifest):
    dir = os.path.dirname(manifest)
    name = os.path.basename(dir).replace('.lv2', '')

    m = rdflib.ConjunctiveGraph()
    m.parse('file:' + manifest, format='n3')

    try:
        # ?uri a lv2:Specification
        uri = query(m, None, rdf.type, lv2.Specification)[0]
        
        # uri lv2:minorVersion ?minor
        minor = query(m, uri, lv2.minorVersion, None)[2]
        
        # uri lv2:microVersion ?
        micro = query(m, uri, lv2.microVersion, None)[2]
    except:
        return False

    if int(minor) != 0 and int(micro) % 2 == 0:
        print('Packaging %s extension version %s.%s' % (name, minor, micro))

        # Make directory and copy all files to it
        distdir = 'build/spec/lv2-%s-%s.%s' % (name, minor, micro)
        os.mkdir(distdir)
        for f in glob.glob('%s/*.*' % dir):
            shutil.copy(f, '%s/%s' % (distdir, os.path.basename(f)))

        pkgconfig_name = str(uri).replace('http://', 'lv2-').replace('/', '-')

        # Generate wscript
        wscript_template = open('wscript.template')
        wscript = open('%s/wscript' % distdir, 'w')
        for l in wscript_template:
            wscript.write(l.replace(
                    '@NAME@', name).replace(
                    '@URI@', str(uri)).replace(
                    '@MINOR@', minor).replace(
                    '@MICRO@', micro).replace(
                    '@PKGCONFIG_NAME@', pkgconfig_name))
        wscript_template.close()
        wscript.close()

        # Generate pkgconfig file
        pkgconfig_template = open('ext.pc.template', 'r')
        pkgconfig = open('%s/%s.pc.in' % (distdir, pkgconfig_name), 'w')
        for l in pkgconfig_template:
            pkgconfig.write(l.replace(
                    '@NAME@', 'LV2 ' + name.title()).replace(
                    '@DESCRIPTION@', 'The LV2 "' + name + '" extension').replace(
                    '@VERSION@', '%s.%s' % (minor, micro)).replace(
                    '@INCLUDE_PATH@', str(uri).replace('http://', 'lv2/')))
        pkgconfig_template.close()
        pkgconfig.close()

        try:
            os.remove('%s/waf' % distdir)
        except:
            pass
        os.symlink('../../../waf', '%s/waf' % distdir)

        olddir = os.getcwd()
        os.chdir(distdir + '/..')
        os.system('tar --exclude=".*" -cjhf %s.tar.bz2 %s' % (
                os.path.basename(distdir), os.path.basename(distdir)))
        os.chdir(olddir)

    return True
