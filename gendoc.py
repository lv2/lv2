#!/usr/bin/env python

import os
import shutil
import subprocess
import glob
import re
import datetime
import xml.dom.minidom
import xml.dom

out_base = os.path.join('build', 'ns')
try:
    shutil.rmtree(out_base)
except:
    pass
    
os.makedirs(out_base)

URIPREFIX  = 'http://lv2plug.in/ns/'
DOXPREFIX  = 'http://lv2plug.in/ns/doc/html/'
SPECGENDIR = './specgen'
STYLEURI   = os.path.join('aux', 'style.css')

release_dir = os.path.join('build', 'spec')
try:
    os.mkdir(release_dir)
except:
    pass

devnull = open(os.devnull, 'w')

# Generate code (headers) documentation
print "** Generating header documentation"
#shutil.copy('Doxyfile', os.path.join('upload', 'Doxyfile'))
print ' * Calling doxygen in ' + os.getcwd()
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
bettertags = open('c_bettertags', 'w')
for cn in root.childNodes:
    if cn.nodeType == xml.dom.Node.ELEMENT_NODE and cn.tagName == 'compound':
        if cn.getAttribute('kind') == 'page':
            continue
        name = getChildText(cn, 'name')
        filename = getChildText(cn, 'filename')
        bettertags.write('%s %s%s.html\n' % (name, DOXPREFIX, filename))
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

print '** Generating core documentation'

lv2_outdir = os.path.join(out_base, 'lv2core')
os.mkdir(lv2_outdir)
shutil.copy('core.lv2/lv2.h',        lv2_outdir)
shutil.copy('core.lv2/lv2.ttl',      lv2_outdir)
shutil.copy('core.lv2/manifest.ttl', lv2_outdir)
shutil.copy('doc/index.php',         lv2_outdir)

def gendoc(specgen_dir, bundle_dir, ttl_filename, html_filename):
    subprocess.call([os.path.join(specgen_dir, 'lv2specgen.py'),
              os.path.join(bundle_dir, ttl_filename),
              os.path.join(specgen_dir, 'template.html'),
              STYLEURI,
              os.path.join(out_base, html_filename),
              os.path.join('..', '..'),
              '-i'])

gendoc('./lv2specgen', 'core.lv2', 'lv2.ttl', 'lv2core/lv2core.html')

footer = open('./lv2specgen/footer.html', 'r')

# Generate main (ontology) documentation and indices
for dir in ['ext', 'extensions']:
    print "** Generating %s%s documentation" % (URIPREFIX, dir)

    outdir = os.path.join(out_base, dir)

    shutil.copytree(dir, outdir, ignore = lambda src, names: '.svn')

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
        ext = subprocess.Popen(['roqet', '-q', '-e', """
PREFIX lv2: <http://lv2plug.in/ns/lv2core#>
SELECT ?ext FROM <%s.lv2/%s.ttl> WHERE { ?ext a lv2:Specification }
""" % (os.path.join(dir, b), b)], stdout=subprocess.PIPE).communicate()[0]

        if ext == "":
            continue

        ext = re.sub('^result: \[ext=uri<', '', ext)
        ext = re.sub('>\]$', '', ext).strip()

        # Get revision
        query = """
PREFIX lv2: <http://lv2plug.in/ns/lv2core#>
PREFIX doap: <http://usefulinc.com/ns/doap#>
SELECT ?rev FROM <%s.lv2/%s.ttl> WHERE { <%s> doap:release [ doap:revision ?rev ] }
""" % (os.path.join(dir, b), b, ext)

        rev = subprocess.Popen(['roqet', '-q', '-e', query],
                               stdout=subprocess.PIPE).communicate()[0]

        if rev != '':
            rev = re.sub('^result: \[rev=string\("', '', rev)
            rev = re.sub('"\)\]$', '', rev).strip()
        else:
            rev = '0'

        if rev != '0' and rev.find('pre') == -1:
            path = os.path.join(os.path.abspath(release_dir), 'lv2-%s-%s.tar.gz' % (b, rev))
            subprocess.call(['tar', '--exclude-vcs', '-czf', path,
                             bundle[bundle.find('/') + 1:]], cwd=dir)

        specgendir = '../../../lv2specgen/'
        if (os.access(outdir + '/%s.lv2/%s.ttl' % (b, b), os.R_OK)):
            print ' * Calling lv2specgen for %s%s/%s' %(URIPREFIX, dir, b)
            subprocess.call([specgendir + 'lv2specgen.py',
                             '%s.lv2/%s.ttl' % (b, b),
                             specgendir + 'template.html',
                             STYLEURI,
                             '%s.lv2/%s.html' % (b, b),
                             os.path.join('..', '..', '..'),
                             '-i'], cwd=outdir);

            li = '<li>'
            if rev == '0':
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
    print >>index_file, index_html
    index_file.close()

# Copy stylesheet
try:
    os.mkdir(os.path.join('build', 'aux'))
except:
    pass
shutil.copy('lv2specgen/style.css', os.path.join('build', STYLEURI))

devnull.close()
footer.close()
