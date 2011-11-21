#!/usr/bin/env python

import datetime
import os
import rdflib
import shutil
import subprocess
import sys
import xml.dom
import xml.dom.minidom

sys.path.append("./lv2specgen")
import lv2specgen

try:
    shutil.rmtree('build', 'ns')
except:
    pass

# Copy bundles (less build files) to build directory
shutil.copytree('lv2/ns', 'build/ns',
                ignore=shutil.ignore_patterns('.*', 'waf', 'wscript', '*.in'))

# Copy stylesheet to build directory
try:
    os.mkdir('build/aux')
except:
    pass

shutil.copy('lv2specgen/style.css', 'build/aux/style.css')

URIPREFIX  = 'http://lv2plug.in/ns/'
DOXPREFIX  = 'ns/doc/html/'
SPECGENDIR = os.path.abspath('lv2specgen')
STYLEPATH  = os.path.abspath('build/aux/style.css')
TAGFILE    = os.path.abspath('doclinks')
BUILDDIR   = os.path.abspath('build')

doap = rdflib.Namespace('http://usefulinc.com/ns/doap#')
lv2  = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
owl  = rdflib.Namespace('http://www.w3.org/2002/07/owl#')
rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')

devnull = open(os.devnull, 'w')

# Generate code (headers) documentation
print('## Generating header documentation with doxygen ##')
subprocess.call('doxygen', stdout=devnull)

def rescue_tags(in_path, out_path):
    "Rescue Doxygen tag file from XML hell."

    def getChildText(elt, tagname):
        "Return the content of the first child node with a certain tag name."
        elements = elt.getElementsByTagName(tagname)
        for e in elements:
            if e.parentNode == elt:
                return e.firstChild.nodeValue
        return ''

    tagdoc = xml.dom.minidom.parse(in_path)
    root = tagdoc.documentElement
    bettertags = open(out_path, 'w')
    for cn in root.childNodes:
        if cn.nodeType == xml.dom.Node.ELEMENT_NODE and cn.tagName == 'compound':
            if cn.getAttribute('kind') == 'page':
                continue
            name = getChildText(cn, 'name')
            filename = getChildText(cn, 'filename')
            # Sometimes the .html is there, sometimes it isn't...
            if not filename.endswith('.html'):
                filename += '.html'
            bettertags.write('%s %s%s\n' % (name, DOXPREFIX, filename))
            if cn.getAttribute('kind') == 'file':
                prefix = ''
            else:
                prefix = name + '::'
            members = cn.getElementsByTagName('member')
            for m in members:
                mname = prefix + getChildText(m, 'name')
                mafile = getChildText(m, 'anchorfile')
                manchor = getChildText(m, 'anchor')
                bettertags.write('%s %s%s#%s\n' % (mname, DOXPREFIX, \
                                                       mafile, manchor))
    bettertags.close()

rescue_tags('c_tags', TAGFILE)

def subst_file(template, output, dict):
    i = open(template, 'r')
    o = open(output, 'w')
    for line in i:
        for key in dict:
            line = line.replace(key, dict[key])
        o.write(line)
    i.close()
    o.close()

print("Entering directory `%s'" % os.path.abspath('build'))
oldcwd = os.getcwd()
os.chdir('build')

extensions = []

print('\n## Generating specification documentation with lv2specgen ##')

for root, dirs, files in os.walk('ns'):
    if '.svn' in dirs:
        dirs.remove('.svn')

    if root in ['ns', 'ns/ext', 'ns/extensions']:
        if 'doc' in dirs:
            dirs.remove('doc')
        continue

    abs_root = os.path.abspath(root)
    outdir = root
    bundle = root
    b = os.path.basename(root)

    if not os.access(outdir + '/%s.ttl' % b, os.R_OK):
        print('warning: extension %s has no %s.ttl file' % (root, root))
        continue

    print(' * %s' % outdir)

    try:
        model = rdflib.ConjunctiveGraph()
        model.parse('%s/manifest.ttl' % bundle, format='n3')
        model.parse('%s/%s.ttl' % (bundle, b), format='n3')
    except:
        e = sys.exc_info()[1]
        print('error parsing %s: %s' % (bundle, str(e)))
        continue

    # Get extension URI
    ext_node = model.value(None, rdf.type, lv2.Specification)
    if not ext_node:
        continue
    
    ext = str(ext_node)

    # Get version
    minor = 0
    micro = 0
    try:
        minor = int(model.value(ext_node, lv2.minorVersion, None))
        micro = int(model.value(ext_node, lv2.microVersion, None))
    except Exception as e:
        print("warning: %s: failed to find version for %s" % (bundle, ext))

    # Get date
    date = None
    for r in model.triples([ext_node, doap.release, None]):
        revision = model.value(r[2], doap.revision, None)
        if revision == ("%d.%d" % (minor, micro)):
            date = model.value(r[2], doap.created, None)
            break

    # Verify that this date is the latest
    for r in model.triples([ext_node, doap.release, None]):
        revision = model.value(r[2], doap.revision, None)
        this_date = model.value(r[2], doap.created, None)
        if this_date > date:
            print("warning: revision %d.%d (%s) is not the latest release" % (
                minor, micro, date))
            break
    
    # Get short description
    shortdesc = model.value(ext_node, doap.shortdesc, None)

    specdoc = lv2specgen.specgen(
        root + '/%s.ttl' % b,
        SPECGENDIR,
        os.path.relpath(os.path.join('ns', 'doc'), abs_root),
        os.path.relpath(STYLEPATH, abs_root),
        os.path.relpath(BUILDDIR, abs_root),
        TAGFILE,
        instances=True)

    lv2specgen.save(root + '/%s.html' % b, specdoc)

    # Name
    row = '<tr><td><a rel="rdfs:seeAlso" href="%s">%s</a></td>' % (
        os.path.relpath(root, 'ns'), b)

    # Description
    if shortdesc:
        row += '<td>' + str(shortdesc) + '</td>'
    else:
        row += '<td></td>'

    # Version
    version_str = '%s.%s' % (minor, micro)
    if minor == 0 or (micro % 2 != 0):
        row += '<td><span style="color: red">' + version_str + ' dev</span></td>'
    else:
        row += '<td>' + version_str + '</td>'

    # Date
    row += '<td>%s</td>' % (str(date) if date else '')

    # Status
    deprecated = model.value(ext_node, owl.deprecated, None)
    if minor == 0:
        row += '<td><span class="error">Experimental</span></td>'
    elif deprecated and str(deprecated[2]) != "false":
            row += '<td><span class="warning">Deprecated</span></td>'
    elif micro % 2 == 0:
        row += '<td><span class="success">Stable</span></td>'

    row += '</tr>'
    extensions.append(row)

    subst_file('../doc/htaccess.in', outdir + '.htaccess',
               { '@NAME@': b,
                 '@BASE@': '/ns/%s/%s' % (dir, b) })

index_rows = ''
extensions.sort()
for i in extensions:
    index_rows += i + '\n'

subst_file('../lv2/ns/index.html.in', 'ns/index.html',
           { '@ROWS@': index_rows,
             '@TIME@': datetime.datetime.utcnow().strftime('%F %H:%M UTC') })
           
print("\nLeaving directory `%s'" % os.path.abspath('build'))
os.chdir(oldcwd)

devnull.close()
