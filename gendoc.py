#!/usr/bin/env python

import datetime
import glob
import os
import rdflib
import re
import shutil
import subprocess
import sys
import xml.dom
import xml.dom.minidom

sys.path.append("./lv2specgen")
import lv2specgen

out_base = os.path.join('build', 'ns')
try:
    shutil.rmtree(out_base)
except:
    pass
    
os.makedirs(out_base)

URIPREFIX  = 'http://lv2plug.in/ns/'
DOXPREFIX  = 'ns/doc/html/'
SPECGENDIR = './specgen'
STYLEURI   = os.path.join('aux', 'style.css')
TAGFILE    = './doclinks'

doap = rdflib.Namespace('http://usefulinc.com/ns/doap#')
lv2  = rdflib.Namespace('http://lv2plug.in/ns/lv2core#')
owl  = rdflib.Namespace('http://www.w3.org/2002/07/owl#')
rdf  = rdflib.Namespace('http://www.w3.org/1999/02/22-rdf-syntax-ns#')

devnull = open(os.devnull, 'w')

# Generate code (headers) documentation
print('** Generating header documentation')
print(' * Calling doxygen in ' + os.getcwd())
subprocess.call('doxygen', stdout=devnull)


# Rescue Doxygen tag file from XML hell

# Return the content of the first child node with a certain tag name
def getChildText(elt, tagname):
    elements = elt.getElementsByTagName(tagname)
    for e in elements:
        if e.parentNode == elt:
            return e.firstChild.nodeValue
    return ''

tagdoc = xml.dom.minidom.parse('c_tags')
root = tagdoc.documentElement
bettertags = open(TAGFILE, 'w')
for cn in root.childNodes:
    if cn.nodeType == xml.dom.Node.ELEMENT_NODE and cn.tagName == 'compound':
        if cn.getAttribute('kind') == 'page':
            continue
        name = getChildText(cn, 'name')
        filename = getChildText(cn, 'filename')
        # Sometimes the .html is there, sometimes it isn't...
        if filename[-5:] != '.html':
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

def subst_file(template, output, dict):
    i = open(template, 'r')
    o = open(output, 'w')
    for line in i:
        for key in dict:
            line = line.replace(key, dict[key])
        o.write(line)
    i.close()
    o.close()

print('** Generating core documentation')

lv2_outdir = os.path.join(out_base, 'lv2core')
os.mkdir(lv2_outdir)
shutil.copy('core.lv2/lv2.h',        lv2_outdir)
shutil.copy('core.lv2/lv2.ttl',      lv2_outdir)
shutil.copy('core.lv2/manifest.ttl', lv2_outdir)

oldcwd = os.getcwd()
os.chdir(lv2_outdir)
print(' * Running lv2specgen for lv2core in ' + os.getcwd())
lv2specgen.save('lv2.html',
                lv2specgen.specgen('../../../core.lv2/lv2.ttl',
                                   '../../../lv2specgen',
                                   os.path.join('..', '..', 'ns', 'doc'),
                                   STYLEURI,
                                   os.path.join('..', '..'),
                                   os.path.join('..', '..', '..', TAGFILE),
                                   instances=True))
os.chdir(oldcwd)
subst_file('doc/htaccess.in', '%s/lv2core/.htaccess' % out_base,
           { '@NAME@': 'lv2core',
             '@BASE@': '/ns/lv2core' })

footer = open('./lv2specgen/footer.html', 'r')

# Generate main (ontology) documentation and indices
for dir in ['ext', 'extensions']:
    print("** Generating %s%s documentation" % (URIPREFIX, dir))

    outdir = os.path.join(out_base, dir)

    shutil.copytree(dir, outdir, ignore=shutil.ignore_patterns('.*', 'waf', 'wscript', '*.in'))

    index_html = """<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML+RDFa 1.0//EN" "http://www.w3.org/MarkUp/DTD/xhtml-rdfa-1.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="application/xhtml+xml;charset=utf-8" />
<title>LV2 Extension Index</title>
<link rel="stylesheet" type="text/css" href="../../""" + STYLEURI + """\" />
</head>
<body>
<div id="header"><h1 id="title">LV2 Extension Index</h1></div>
<div class="content">
<table summary="An index of LV2 extensions">
<tr><th>Name</th><th>Description</th><th>Version</th><th>Date</th><th>Status</th></tr>\n"""

    extensions = []

    for bundle in glob.glob(os.path.join(dir, '*.lv2')):
        b = bundle.replace('.lv2', '')
        b = b[b.find('/') + 1:]

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
            print "warning: %s: failed to find version for %s" % (bundle, ext)
            pass

        # Get date
        date = None
        for r in model.triples([ext_node, doap.release, None]):
            revision = model.value(r[2], doap.revision, None)
            if revision != ("%d.%d" % (minor, micro)):
                print("warning: %s: doap:revision %s != %d.%d" % (
                        bundle, revision, minor, micro))
                continue

            date = model.value(r[2], doap.created, None)
            break
        
        # Get short description
        shortdesc = model.value(ext_node, doap.shortdesc, None)

        specgendir = '../../../lv2specgen/'
        if (os.access(outdir + '/%s.lv2/%s.ttl' % (b, b), os.R_OK)):
            oldcwd = os.getcwd()
            os.chdir(outdir)
            print(' * Running lv2specgen for %s in %s' % (b, os.getcwd()))
            lv2specgen.save('%s.lv2/%s.html' % (b, b),
                            lv2specgen.specgen('%s.lv2/%s.ttl' % (b, b),
                                               specgendir,
                                               os.path.join('..', '..', '..', 'ns', 'doc'),
                                               STYLEURI,
                                               os.path.join('..', '..', '..'),
                                               os.path.join('..', '..', '..', TAGFILE),
                                               instances=True))
            os.chdir(oldcwd)

            # Name
            row = '<tr><td><a rel="rdfs:seeAlso" href="%s">%s</a></td>' % (b, b)

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

        subst_file('doc/htaccess.in', '%s/%s.lv2/.htaccess' % (outdir, b),
                   { '@NAME@': b,
                     '@BASE@': '/ns/%s/%s' % (dir, b) })
    
        # Remove .lv2 suffix from bundle name (to make URI resolvable)
        os.rename(outdir + '/%s.lv2' % b, outdir + '/%s' % b)

    extensions.sort()
    for i in extensions:
        index_html += i + '\n'
    
    index_html += '</table>\n</div>\n'

    index_html += '<div id="footer">'
    index_html += '<span class="footer-text">Generated on '
    index_html += datetime.datetime.utcnow().strftime('%F %H:%M UTC')
    index_html += ' by gendoc.py</span>&nbsp;'
    index_html += footer.read() + '</div>'

    index_html += '</body></html>\n'

    index_file = open(os.path.join(outdir, 'index.html'), 'w')
    index_file.write(index_html)
    index_file.close()

# Copy stylesheet
try:
    os.mkdir(os.path.join('build', 'aux'))
except:
    pass
shutil.copy('lv2specgen/style.css', os.path.join('build', STYLEURI))

devnull.close()
footer.close()
