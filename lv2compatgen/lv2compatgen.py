#!/usr/bin/python
# -*- coding: utf8 -*-
#
# lv2compatgen
# Generates compatiblity documentation for LV2 hosts and plugins.
# Copyright (c) 2009 David Robillard <d@drobilla.net>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

__authors__ = "David Robillard"
__license__ = "MIT License <http://www.opensource.org/licenses/mit-license.php>"
__contact__ = "devel@lists.lv2plug.in"
__date__    = "2009-11-08"
 
import os
import sys
import datetime
import re
import urllib
import RDF

rdf  = RDF.NS('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
rdfs = RDF.NS('http://www.w3.org/2000/01/rdf-schema#')
owl  = RDF.NS('http://www.w3.org/2002/07/owl#')
vs   = RDF.NS('http://www.w3.org/2003/06/sw-vocab-status/ns#')
lv2  = RDF.NS('http://lv2plug.in/ns/lv2core#')
doap = RDF.NS('http://usefulinc.com/ns/doap#')
foaf = RDF.NS('http://xmlns.com/foaf/0.1/')

def print_usage():
	print """
Usage: lv2compatgen.py DATA

DATA must be a Redland RDF store containing all relevant LV2 data
(a file would be too slow to parse).

You can create one with something like this:

find /usr/lib/lv2 /usr/local/lib/lv2 ~/.lv2 -name '*.ttl' >> lv2_files.txt
for i in `cat lv2_files.txt`; do
	rapper -g $i -o turtle >> lv2_all.ttl;
done
rdfproc ./data parse lv2_all.ttl turtle
"""

if len(sys.argv) != 2:
	print_usage()
	sys.exit(1)

store_name = sys.argv[1]

storage = RDF.HashStorage(store_name, options="hash-type='bdb'")
model = RDF.Model(storage=storage)

class Plugin:
	def __init__(self):
		self.name = ""
		self.optional = []
		self.required = []

class Feature:
	def __init__(self):
		self.name = ""

# Find plugins and their required and optional features
plugins = {}
features = {}
for i in model.find_statements(RDF.Statement(None, rdf.type, lv2.Plugin)):
	plug = Plugin()
	for j in model.find_statements(RDF.Statement(i.subject, lv2.requiredFeature, None)):
		plug.required += [j.object.uri]
		if not j.object.uri in features:
			features[j.object.uri] = Feature()
	for j in model.find_statements(RDF.Statement(i.subject, lv2.optionalFeature, None)):
		plug.optional += [j.object.uri]
		if not j.object.uri in features:
			features[j.object.uri] = Feature()
	for j in model.find_statements(RDF.Statement(i.subject, doap.name, None)):
		plug.name = str(j.object)
	plugins[i.subject.uri] = plug

# Find feature names
for uri, feature in features.items():
	for j in model.find_statements(RDF.Statement(uri, doap.name, None)):
		feature.name = j.object.literal_value['string']
	for j in model.find_statements(RDF.Statement(uri, rdfs.label, None)):
		print "LABEL:", j.object

# Generate body
body = '<table><tr><td></td>'
for uri, feature in features.items():
	if feature.name != "":
		body += '<td>%s</td>' % feature.name
	else:
		body += '<td>%s</td>' % uri

for uri, plug in plugins.items():
	#body += '<tr><td>%s</td>' % uri
	body += '<tr><td>%s</td>' % plug.name
	for e in features.keys():
		if e in plug.required:
			body += '<td class="required">Required</td>'
		elif e in plug.optional:
			body += '<td class="optional">Optional</td>'
		else:
			body += '<td></td>'
	body += '</tr>\n'
body += '</table>'

# Load output template
temploc = 'template.html'
template = None
try:
    f = open(temploc, "r")
    template = f.read()
except Exception, e:
	print 'Error reading template:', str(e)
	sys.exit(2)

# Load style
styleloc = 'style.css'
style = ''
try:
    f = open(styleloc, "r")
    style = f.read()
except Exception, e:
    print "Error reading from style \"" + styleloc + "\": " + str(e)
    usage()

# Replace tokens in template
template = template.replace('@STYLE@', style)
template = template.replace('@BODY@', body)
template = template.replace('@TIME@', datetime.datetime.utcnow().strftime('%F %H:%M UTC'))

# Write output
output = open('./compat.html', 'w')
print >>output, template
output.close()
