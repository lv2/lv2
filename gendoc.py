#!/usr/bin/env python

import datetime
import glob
import os
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

devnull = open(os.devnull, 'w')

# Generate code (headers) documentation
print('** Generating header documentation')
print(' * Calling doxygen in ' + os.getcwd())
subprocess.call('doxygen', stdout=devnull)


# Rescue Doxygen tag file from XML hell

# Return the content of the first child node with a certain tag name
def getChildText(elt, tagname):
    elements = elt.getElementsByTagName(tagname)
    text = ''
    for e in elements:
        if e.parentNode == elt:
            text = e.firstChild.nodeValue
            return text
    return text

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

print('** Generating core documentation')

lv2_outdir = os.path.join(out_base, 'lv2core')
os.mkdir(lv2_outdir)
shutil.copy('core.lv2/lv2.h',        lv2_outdir)
shutil.copy('core.lv2/lv2.ttl',      lv2_outdir)
shutil.copy('core.lv2/manifest.ttl', lv2_outdir)
shutil.copy('doc/index.php',         lv2_outdir)

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
<title>LV2 Extensions</title>
<link rel="stylesheet" type="text/css" href="../../""" + STYLEURI + """\" />
</head>
<body>
<div id="titleheader"><h1 id="title">LV2 Extensions</h1></div>
<div class="content">
<h2>""" + URIPREFIX + dir + "/</h2><ul>\n"

    extensions = []

    for bundle in glob.glob(os.path.join(dir, '*.lv2')):
        b = bundle.replace('.lv2', '')
        b = b[b.find('/') + 1:]

        # Get extension URI
        ext = str(subprocess.Popen(['roqet', '-q', '-e', """
PREFIX lv2: <http://lv2plug.in/ns/lv2core#>
SELECT ?ext FROM <%s.lv2/%s.ttl> WHERE { ?ext a lv2:Specification }
""" % (os.path.join(dir, b), b)], stdout=subprocess.PIPE).communicate()[0])

        if ext == "":
            continue

        ext = re.sub("^b'", '', ext)
        ext = re.sub("\n'$", '', ext)
        ext = re.sub('.*result: \[ext=uri<', '', ext)
        ext = re.sub('>.*$', '', ext).strip()

        # Get revision
        query = """
PREFIX lv2: <http://lv2plug.in/ns/lv2core#>
PREFIX doap: <http://usefulinc.com/ns/doap#>
SELECT ?rev FROM <%s.lv2/%s.ttl> WHERE { <%s> doap:release [ doap:revision ?rev ] }
""" % (os.path.join(dir, b), b, ext)

        rev = str(subprocess.Popen(['roqet', '-q', '-e', query],
                                   stdout=subprocess.PIPE).communicate()[0])

        if rev != '':
            rev = re.sub("^b'", '', rev)
            rev = re.sub("\n'$", '', rev)
            rev = re.sub('.*result: \[rev=string\("', '', rev)
            rev = re.sub('"\)\].*$', '', rev).strip()
        else:
            rev = '0'

        minor = '0'
        micro = '0'
        match = re.search('([^\.]*)\.([^\.]*)', rev)
        if match:
            minor = match.group(1)
            micro = match.group(2)

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

            li = '<li>'
            if minor == '0' or (int(micro) % 2) != 0:
                li += '<span style="color: red;">Experimental: </span>'
            li += '<a rel="rdfs:seeAlso" href="%s">%s</a>' % (b, b)
            li += '</li>'

            extensions.append(li)

        shutil.copy('doc/index.php', os.path.join(outdir, b + '.lv2', 'index.php'))
    
        # Remove .lv2 suffix from bundle name (to make URI resolvable)
        os.rename(outdir + '/%s.lv2' % b, outdir + '/%s' % b)

    extensions.sort()
    for i in extensions:
        index_html += i + '\n'
    
    index_html += '</ul>\n</div>\n'

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
