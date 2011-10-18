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
    for i in model.triples([s, p, o]):
        return i
    raise Exception('Bad LV2 extension data')

def genwscript(manifest):
    dir = os.path.dirname(manifest)
    name = os.path.basename(dir).replace('.lv2', '')

    m = rdflib.ConjunctiveGraph()
    m.parse(manifest, format='n3')

    try:
        uri   = query(m, None, rdf.type, lv2.Specification)[0]
        minor = query(m, uri, lv2.minorVersion, None)[2]
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
        subst_file('wscript.template', '%s/wscript' % distdir)

        # Generate pkgconfig file
        subst_file('ext.pc.template', '%s/%s.pc.in' % (distdir, pkgconfig_name))

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
