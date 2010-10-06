#!/usr/bin/python
# -*- coding: utf-8 -*-

"""A program (and Python module) to generate a tree of symlinks to LV2
extension bundles, where the path of the symlink corresponds to the URI of
the extension.  This allows including extension headers in code without using
the bundle name.  Including extension headers in this way is much better,
since there is no dependency on the (meaningless and non-persistent) bundle
name in the code using the header.

For example, after running lv2includegen (and setting the compiler include
path appropriately), LV2 headers could be included like so:

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/event/event.h"
#include "lv2/example.org/foo/foo.h"

Where the initial "lv2" is arbitrary; in this case lv2includegen's output
directory was "lv2", and that directory's parent was added to the compiler
include search path.  It is a good idea to use such a prefix directory so
domain names do not conflict with anything else in the include path.
"""

__authors__ = 'David Robillard'
__license   = 'GNU GPL v3 or later <http://www.gnu.org/licenses/gpl.html>'
__contact__ = 'devel@lists.lv2plug.in'
__date__    = '2010-10-05'

import errno
import glob
import os
import stat
import sys

import RDF

rdf  = RDF.NS('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
lv2  = RDF.NS('http://lv2plug.in/ns/lv2core#')

def lv2_path():
    "Return the LV2 search path (LV2_PATH in the environment, or a default)."
    if 'LV2_PATH' in os.environ:
        return os.environ['LV2_PATH']
    else:
        ret = '/usr/lib/lv2' + os.pathsep + '/usr/local/lib/lv2'
        print 'LV2_PATH unset, using default ' + ret
        return ret

def __bundles(search_path):
    "Return a list of all LV2 bundles found in search_path."
    dirs = search_path.split(os.pathsep)
    bundles = []
    for dir in dirs:
        bundles += glob.glob(os.path.join(dir, '*.lv2'))
    return bundles

def __usage():
    script = os.path.basename(sys.argv[0])
    print """Usage: 
    %s OUTDIR

        OUTDIR : Directory to build include tree

Example:
    %s /usr/local/include/lv2
""" % (script, script)

def __mkdir_p(path):
    "Equivalent of UNIX mkdir -p"
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno == errno.EEXIST:
            pass
        else:
            raise

def build_tree(search_path, outdir):
    """Build a directory tree under outdir containing symlinks to all LV2
    extensions found in search_path, such that the symlink paths correspond to
    the extension URIs."""
    for bundle in __bundles(search_path):
        # Load manifest into model
        manifest = RDF.Model()
        parser = RDF.Parser(name="guess")
        parser.parse_into_model(manifest, 'file://' + os.path.join(bundle, 'manifest.ttl'))

        # Query extension URI
        results = manifest.find_statements(RDF.Statement(None, rdf.type, lv2.Specification))
        for r in results:
            ext_uri  = str(r.subject.uri)
            ext_path = os.path.normpath(ext_uri[ext_uri.find(':') + 1:].lstrip('/'))
            ext_dir  = os.path.join(outdir, ext_path)

            # Make parent directories
            __mkdir_p(os.path.dirname(ext_dir))

            # Remove existing symlink if necessary
            if os.access(ext_dir, os.F_OK):
                mode = os.lstat(ext_dir)[stat.ST_MODE]
                if stat.S_ISLNK(mode):
                    os.remove(ext_dir)
                else:
                    raise Exception(ext_dir + " exists and is not a link")

            # Make symlink to bundle directory
            os.symlink(bundle, ext_dir)
            
if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) != 1:
        usage()
        sys.exit(1)

    outdir = args[0]
    print "Building LV2 include tree at", outdir

    build_tree(lv2_path(), outdir)
