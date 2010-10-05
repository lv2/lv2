#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# lv2includegen, a tool to generate directory trees for including
# extension headers from source code.

__authors__ = 'David Robillard'
__license   = 'GNU GPL v3 or later <http://www.gnu.org/licenses/gpl.html>'
__contact__ = 'devel@lists.lv2plug.in'
__date__    = '2010-10-05'

import errno
import glob
import os
import sys

import RDF

rdf  = RDF.NS('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
lv2  = RDF.NS('http://lv2plug.in/ns/lv2core#')

def lv2_path():
    "Return the LV2 search path (LV2_PATH)."
    if 'LV2_PATH' in os.environ:
        return os.environ['LV2_PATH']
    else:
        ret = '/usr/lib/lv2' + os.pathsep + '/usr/local/lib/lv2'
        print 'LV2_PATH unset, using default ' + ret
        return ret

def lv2_bundles(search_path):
    "Return a list of all LV2 bundles found in a search path."
    dirs = search_path.split(os.pathsep)
    bundles = []
    for dir in dirs:
        bundles += glob.glob(os.path.join(dir, '*.lv2'))
    return bundles

def usage():
    script = os.path.basename(sys.argv[0])
    print """Usage: 
    %s OUTDIR

        OUTDIR : Directory to build include tree

Example:
    %s /usr/local/include/lv2
""" % (script, script)
    sys.exit(-1)

def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise

if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) != 1:
        usage()
        exit(1)

    outdir = args[0]
    print "Building LV2 include tree at", outdir
    
    for bundle in lv2_bundles(lv2_path()):
        # Load manifest into model
        manifest = RDF.Model()
        parser = RDF.Parser(name="guess")
        parser.parse_into_model(manifest, 'file://' + os.path.join(bundle, 'manifest.ttl'))

        # Query extension URI
        results = manifest.find_statements(RDF.Statement(None, rdf.type, lv2.Specification))
        for r in results:
            ext_uri        = str(r.subject.uri)
            ext_path       = os.path.normpath(ext_uri[ext_uri.find(':') + 1:].lstrip('/'))
            ext_parent_dir = os.path.join(outdir, os.path.dirname(ext_path))
            ext_dir        = os.path.basename(ext_path)

            # Make symlink to bundle directory
            mkdir_p(ext_parent_dir)
            os.symlink(bundle, os.path.join(ext_parent_dir, ext_dir))
