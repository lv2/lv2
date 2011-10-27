#!/usr/bin/python
# -*- coding: utf-8 -*-
#
# lv2specgen, an LV2 extension specification page generator
# Copyright (c) 2009-2011 David Robillard <d@drobilla.net>
#
# Based on SpecGen:
# <http://forge.morfeo-project.org/wiki_en/index.php/SpecGen>
# Copyright (c) 2003-2008 Christopher Schmidt <crschmidt@crschmidt.net>
# Copyright (c) 2005-2008 Uldis Bojars <uldis.bojars@deri.org>
# Copyright (c) 2007-2008 Sergio Fernández <sergio.fernandez@fundacionctic.org>
#
# This software is licensed under the terms of the MIT License.
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

__version__ = "1.0.2"
__authors__ = """
Christopher Schmidt,
Uldis Bojars,
Sergio Fernández,
David Robillard"""
__license__ = "MIT License <http://www.opensource.org/licenses/mit>"
__contact__ = "devel@lists.lv2plug.in"
__date__ = "2011-10-26"

import os
import sys
import datetime
import re
import xml.sax.saxutils

try:
    import RDF
    import Redland
except ImportError:
    sys.exit("Error importing Redland bindings for Python (python-librdf)")

# Global Variables
classranges = {}
classdomains = {}
linkmap = {}
spec_url = None
spec_ns_str = None
spec_ns = None
spec_pre = None
ns_list = {
    "http://www.w3.org/1999/02/22-rdf-syntax-ns#"   : "rdf",
    "http://www.w3.org/2000/01/rdf-schema#"         : "rdfs",
    "http://www.w3.org/2002/07/owl#"                : "owl",
    "http://www.w3.org/2001/XMLSchema#"             : "xsd",
    "http://rdfs.org/sioc/ns#"                      : "sioc",
    "http://xmlns.com/foaf/0.1/"                    : "foaf",
    "http://purl.org/dc/elements/1.1/"              : "dc",
    "http://purl.org/dc/terms/"                     : "dct",
    "http://purl.org/rss/1.0/modules/content/"      : "content",
    "http://www.w3.org/2003/01/geo/wgs84_pos#"      : "geo",
    "http://www.w3.org/2004/02/skos/core#"          : "skos",
    "http://lv2plug.in/ns/lv2core#"                 : "lv2",
    "http://usefulinc.com/ns/doap#"                 : "doap"
    }

rdf  = RDF.NS('http://www.w3.org/1999/02/22-rdf-syntax-ns#')
rdfs = RDF.NS('http://www.w3.org/2000/01/rdf-schema#')
owl  = RDF.NS('http://www.w3.org/2002/07/owl#')
lv2  = RDF.NS('http://lv2plug.in/ns/lv2core#')
doap = RDF.NS('http://usefulinc.com/ns/doap#')
foaf = RDF.NS('http://xmlns.com/foaf/0.1/')

doc_base = '.'


def findStatements(model, s, p, o):
    return model.find_statements(RDF.Statement(s, p, o))


def isEnd(s):
    return not s.current()


def findOne(m, s, p, o):
    l = findStatements(m, s, p, o)
    return l.current()


def getSubject(s):
    return s.subject


def getPredicate(s):
    return s.predicate


def getObject(s):
    return s.object


def niceName(uri):
    regexp = re.compile("^(.*[/#])([^/#]+)$")
    rez = regexp.search(uri)
    if not rez:
        return uri
    pref = rez.group(1)
    if pref in ns_list:
        return ns_list.get(pref, pref) + ":" + rez.group(2)
    else:
        return uri


def termName(m, urinode):
    "Trims the namespace out of a term to give a name to the term."
    return str(urinode).replace(spec_ns_str, "")


def getLabel(m, urinode):
    l = findOne(m, urinode, rdfs.label, None)
    if l:
        return getObject(l).literal_value['string']
    else:
        return ''


def getComment(m, urinode):
    c = findOne(m, urinode, lv2.documentation, None)
    if c:
        markup = getObject(c).literal_value['string']

        rgx = re.compile('([^a-zA-Z0-9_:])(' + \
                             '|'.join(map(re.escape, linkmap)) + \
                             ')([^a-aA-Z0-9_:])')

        def translate(match):
            return match.group(1) + linkmap[match.group(2)] + match.group(3)

        return rgx.sub(translate, markup)

    c = findOne(m, urinode, rdfs.comment, None)
    if c:
        text = getObject(c).literal_value['string']
        return xml.sax.saxutils.escape(text)

    return ''


def getProperty(val, first=True):
    "Return a string representing a property value in a property table"
    doc = ''
    if not first:
        doc += '<tr><td></td>'  # Empty cell in header column
    doc += '<td>%s</td></tr>\n' % val
    return doc


def endProperties(first):
    if first:
        return '</tr>'
    else:
        return ''


def owlVersionInfo(m):
    v = findOne(m, None, owl.versionInfo, None)
    if v:
        return getObject(v).literal_value['string']
    else:
        return ""


def rdfsPropertyInfo(term, m):
    """Generate HTML for properties: Domain, range"""
    global classranges
    global classdomains
    doc = ""
    range = ""
    domain = ""

    # Find subPropertyOf information
    o = findStatements(m, term, rdfs.subPropertyOf, None)
    if not isEnd(o):
        rlist = ''
        first = True
        for st in o:
            k = getTermLink(getObject(st), term, rdfs.subPropertyOf)
            rlist += getProperty(k, first)
            first = False
        doc += "<tr><th>Sub-property of</th>%s" % rlist

    # Domain stuff
    domains = findStatements(m, term, rdfs.domain, None)
    domainsdoc = ""
    for d in domains:
        collection = findStatements(m, getObject(d), owl.unionOf, None)
        if not isEnd(collection):
            uris = parseCollection(m, collection)
            first = True
            for uri in uris:
                domainsdoc += getProperty(getTermLink(uri, term, rdfs.domain), first)
                add(classdomains, uri, term.uri)
                first = False
        else:
            if not getObject(d).is_blank():
                domainsdoc += getProperty(getTermLink(getObject(d), term, rdfs.domain))
    if (len(domainsdoc) > 0):
        doc += "<tr><th>Domain</th>%s" % domainsdoc

    # Range stuff
    ranges = findStatements(m, term, rdfs.range, None)
    rangesdoc = ""
    for r in ranges:
        collection = findStatements(m, getObject(r), owl.unionOf, None)
        if not isEnd(collection):
            uris = parseCollection(m, collection)
            first = True
            for uri in uris:
                rangesdoc += getProperty(getTermLink(uri, term, rdfs.range), first)
                add(classranges, uri, term.uri)
                first = False
        else:
            if not getObject(r).is_blank():
                rangesdoc += getProperty(getTermLink(getObject(r), term, rdfs.range))
    if (len(rangesdoc) > 0):
        doc += "<tr><th>Range</th>%s" % rangesdoc

    return doc


def parseCollection(model, collection):
    uris = []
    rdflist = findStatements(model, collection.current().object, None, None)
    while (not isEnd(rdflist)) and (not rdflist.current().object.is_blank()):
        one = rdflist.current()
        if not one.object.is_blank():
            uris.append(str(one.object.uri))
        rdflist.next()
        one = rdflist.current()
        if getPredicate(one) == rdf.rest:
            rdflist = findStatements(model, one.object, None, None)

    return uris


def getTermLink(uri, subject=None, predicate=None):
    uri = str(uri)
    extra = ''
    if subject != None and predicate != None:
        extra = 'about="%s" rel="%s" resource="%s"' % (str(subject), niceName(str(predicate)), uri)
    if (uri.startswith(spec_ns_str)):
        return '<a href="#%s" %s>%s</a>' % (uri.replace(spec_ns_str, ""), extra, niceName(uri))
    else:
        return '<a href="%s" %s>%s</a>' % (uri, extra, niceName(uri))


def rdfsClassInfo(term, m):
    """Generate rdfs-type information for Classes: ranges, and domains."""
    global classranges
    global classdomains
    doc = ""

    # Find subClassOf information
    restrictions = []
    superclasses = []
    for st in findStatements(m, term, rdfs.subClassOf, None):
        if not getObject(st).is_blank():
            uri = getObject(st)
            if not uri in superclasses:
                superclasses.append(uri)
        else:
            meta_type = findOne(m, getObject(st), rdf.type, None)
            restrictions.append(getSubject(meta_type))
            
    if len(superclasses) > 0:
        doc += "\n<tr><th>Sub-class of</th>"
        first = True
        for superclass in superclasses:
            doc += getProperty(getTermLink(superclass), first)
            first = False

    for r in restrictions:
        props = findStatements(m, r, None, None)
        onProp = None
        comment = None
        for p in props:
            if getPredicate(p) == owl.onProperty:
                onProp = getObject(p)
            elif getPredicate(p) == rdfs.comment:
                comment = getObject(p)
        if onProp != None:
            doc += '<tr><th>Restriction on %s</th><td>' % getTermLink(onProp)

            prop_str = ''
            last_pred = None
            first = True
            for p in findStatements(m, r, None, None):
                if (getPredicate(p) == owl.onProperty
                    or getPredicate(p) == rdfs.comment
                    or (getPredicate(p) == rdf.type and getObject(p) == owl.Restriction)
                    or getPredicate(p) == lv2.documentation):
                    last_pred = None
                    continue

                if getPredicate(p) != last_pred:
                    prop_str += '<tr><th>%s</th>\n' % getTermLink(getPredicate(p))
                    first = True
                if getObject(p).is_resource():
                    prop_str += getProperty(getTermLink(getObject(p)), first)
                    first = False
                elif getObject(p).is_literal():
                    prop_str += getProperty(getObject(p).literal_value['string'], first)
                    first = False

                last_pred = getPredicate(p)

            prop_str += endProperties(first)

            if prop_str != '':
                doc += '<table class=\"restriction\">%s</table>\n' % prop_str
            if comment != None:
                doc += "<span>%s</span>\n" % comment.literal_value['string']
            doc += '</td></tr>'

    # Find out about properties which have rdfs:domain of t
    d = classdomains.get(str(term), "")
    if d:
        dlist = ''
        first = True
        for k in d:
            dlist += getProperty(getTermLink(k), first)
            first = False
        doc += "<tr><th>In domain of</th>%s" % dlist

    # Find out about properties which have rdfs:range of t
    r = classranges.get(str(term), "")
    if r:
        rlist = ''
        first = True
        for k in r:
            rlist += getProperty(getTermLink(k), first)
            first = False
        doc += "<tr><th>In range of</th>%s" % rlist

    return doc


def isSpecial(pred):
    """Return True if the predicate is "special" and shouldn't be emitted generically"""
    return pred in [rdf.type, rdfs.range, rdfs.domain, rdfs.label, rdfs.comment, rdfs.subClassOf, rdfs.subPropertyOf, lv2.documentation]


def blankNodeDesc(node, m):
    properties = findStatements(m, node, None, None)
    doc = ''
    last_pred = ''
    for p in properties:
        if isSpecial(getPredicate(p)):
            continue
        doc += '<tr>'
        doc += '<td class="blankterm">%s</td>\n' % getTermLink(getPredicate(p))
        if getObject(p).is_resource():
            doc += '<td class="blankdef">%s</td>\n' % getTermLink(getObject(p))
            # getTermLink(str(getObject(p).uri), node, getPredicate(p))
        elif getObject(p).is_literal():
            doc += '<td class="blankdef">%s</td>\n' % str(getObject(p).literal_value['string'])
        elif getObject(p).is_blank():
            doc += '<td class="blankdef">' + blankNodeDesc(getObject(p), m) + '</td>\n'
        else:
            doc += '<td class="blankdef">?</td>\n'
        doc += '</tr>'
    if doc != '':
        doc = '<table class="blankdesc">\n%s\n</table>\n' % doc
    return doc


def extraInfo(term, m):
    """Generate information about misc. properties of a term"""
    doc = ""
    properties = findStatements(m, term, None, None)
    last_pred = None
    first = True
    for p in properties:
        if isSpecial(getPredicate(p)):
            last_pred = None
            continue
        if getPredicate(p) != last_pred:
            doc += '<tr><th>%s</th>\n' % getTermLink(getPredicate(p))
            first = True
        if getObject(p).is_resource():
            doc += getProperty(getTermLink(getObject(p), term, getPredicate(p)), first)
        elif getObject(p).is_literal():
            doc += getProperty(str(getObject(p)), first)
        elif getObject(p).is_blank():
            doc += getProperty(str(blankNodeDesc(getObject(p), m)), first)
        else:
            doc += getProperty('?', first)
        first = False
        last_pred = getPredicate(p)

    #doc += endProperties(first)

    return doc


def rdfsInstanceInfo(term, m):
    """Generate rdfs-type information for instances"""
    doc = ""

    t = findStatements(m, term, rdf.type, None)
    if not isEnd(t):
        doc += "<tr><th>Type</th>"
    first = True
    for match in t:
        doc += getProperty(getTermLink(getObject(match),
                                       term,
                                       rdf.type),
                           first)
        first = False

    doc += endProperties(first)
    doc += extraInfo(term, m)

    return doc


def owlInfo(term, m):
    """Returns an extra information that is defined about a term using OWL."""
    res = ''

    # Inverse properties ( owl:inverseOf )
    o = findStatements(m, term, owl.inverseOf, None)
    if not isEnd(o):
        res += "<tr><th>Inverse:</th>\n"
        first = True
        for st in o:
            res += getProperty(getTermLink(getObject(st)), first)
            first = False
        res += endProperties(first)

    def owlTypeInfo(term, propertyType, name):
        o = findStatements(m, term, rdf.type, propertyType)
        if not isEnd(o):
            return "<tr><th>OWL Type</th><td>%s</td></tr>\n" % name
        else:
            return ""

    res += owlTypeInfo(term, owl.DatatypeProperty, "Datatype Property")
    res += owlTypeInfo(term, owl.ObjectProperty, "Object Property")
    res += owlTypeInfo(term, owl.AnnotationProperty, "Annotation Property")
    res += owlTypeInfo(term, owl.InverseFunctionalProperty, "Inverse Functional Property")
    res += owlTypeInfo(term, owl.SymmetricProperty, "Symmetric Property")

    return res


def docTerms(category, list, m):
    """
    A wrapper class for listing all the terms in a specific class (either
    Properties, or Classes. Category is 'Property' or 'Class', list is a
    list of term names (strings), return value is a chunk of HTML.
    """
    doc = ""
    nspre = spec_pre
    for item in list:
        t = termName(m, item)
        if (t.startswith(spec_ns_str)) and (
            len(t[len(spec_ns_str):].split("/")) < 2):
            term = t
            t = t.split(spec_ns_str[-1])[1]
            curie = "%s:%s" % (nspre, t)
        else:
            if t.startswith("http://"):
                term = t
                curie = getShortName(t)
                t = getAnchor(t)
            else:
                term = spec_ns[t]
                curie = "%s:%s" % (nspre, t)

        try:
            term_uri = term.uri
        except:
            term_uri = term

        doc += """<div class="specterm" id="%s" about="%s">\n<h3>%s <a href="#%s">%s</a></h3>\n""" % (t, term_uri, category, getAnchor(str(term_uri)), curie)

        label = getLabel(m, term)
        comment = getComment(m, term)

        doc += '<div class="spectermbody">'
        if label != '' or comment != '':
            doc += '<div class="description">'

        if label != '':
            doc += "<div property=\"rdfs:label\" class=\"label\">%s</div>" % label

        if comment != '':
            doc += "<div property=\"rdfs:comment\">%s</div>" % comment

        if label != '' or comment != '':
            doc += "</div>"

        terminfo = ""
        if category == 'Property':
            terminfo += owlInfo(term, m)
            terminfo += rdfsPropertyInfo(term, m)
        if category == 'Class':
            terminfo += rdfsClassInfo(term, m)
        if category == 'Instance':
            terminfo += rdfsInstanceInfo(term, m)

        terminfo += extraInfo(term, m)

        if (len(terminfo) > 0):  # to prevent empty list (bug #882)
            doc += '\n<table class="terminfo">%s</table>\n' % terminfo

        doc += '</div>'
        doc += "\n</div>\n\n"

    return doc


def getShortName(uri):
    uri = str(uri)
    if ("#" in uri):
        return uri.split("#")[-1]
    else:
        return uri.split("/")[-1]


def getAnchor(uri):
    uri = str(uri)
    if (uri.startswith(spec_ns_str)):
        return uri[len(spec_ns_str):].replace("/", "_")
    else:
        return getShortName(uri)


def buildIndex(m, classlist, proplist, instalist=None):
    """
    Builds the A-Z list of terms. Args are a list of classes (strings) and
    a list of props (strings)
    """

    if len(classlist) == 0 and len(proplist) == 0 and (
        not instalist or len(instalist) == 0):
        return ''

    azlist = '<dl class="index">'

    if (len(classlist) > 0):
        azlist += "<dt>Classes</dt><dd><ul>"
        classlist.sort()
        shown = {}
        for c in classlist:
            if c in shown:
                continue
            if not isEnd(findStatements(m, c, rdfs.subClassOf, None)):
                continue
            shown[c] = True
            name = termName(m, c)
            if name.startswith(spec_ns_str):
                name = name.split(spec_ns_str[-1])[1]
            azlist += '<li><a href="#%s">%s</a>' % (name, name)
            def class_tree(c):
                tree = ''
                shown[c] = True
                statements = findStatements(m, None, rdfs.subClassOf, c)
                if not isEnd(statements):
                    tree += '<ul>'
                    for s in statements:
                        s_name = termName(m, getSubject(s))
                        tree += '<li><a href="#%s">%s</a>\n' % (s_name, s_name)
                        tree += class_tree(getSubject(s))
                        tree += '</li>'
                    tree += '</ul>'
                return tree
            azlist += class_tree(c)
            azlist += '</li>'
        azlist += '</ul></dd>\n'

    if (len(proplist) > 0):
        azlist += "<dt>Properties</dt><dd>"
        proplist.sort()
        for p in proplist:
            name = termName(m, p)
            if name.startswith(spec_ns_str):
                name = name.split(spec_ns_str[-1])[1]
            azlist = """%s <a href="#%s">%s</a>, """ % (azlist, name, name)
        azlist = """%s</dd>\n""" % azlist

    if (instalist != None and len(instalist) > 0):
        azlist += "<dt>Instances</dt><dd>"
        for i in instalist:
            p = getShortName(i)
            anchor = getAnchor(i)
            azlist = """%s <a href="#%s">%s</a>, """ % (azlist, anchor, p)
        azlist = """%s</dd>\n""" % azlist

    azlist = """%s\n</dl>""" % azlist
    return azlist


def add(where, key, value):
    if not key in where:
        where[key] = []
    if not value in where[key]:
        where[key].append(value)


def specInformation(m, ns):
    """
    Read through the spec (provided as a Redland model) and return classlist
    and proplist. Global variables classranges and classdomains are also filled
    as appropriate.
    """
    global classranges
    global classdomains

    # Find the class information: Ranges, domains, and list of all names.
    classtypes = [rdfs.Class, owl.Class]
    classlist = []
    for onetype in classtypes:
        for classStatement in findStatements(m, None, rdf.type, onetype):
            for range in findStatements(m, None, rdfs.range, getSubject(classStatement)):
                if not getSubject(classStatement).is_blank():
                    add(classranges,
                        str(getSubject(classStatement)),
                        str(getSubject(range)))
            for domain in findStatements(m, None, rdfs.domain, getSubject(classStatement)):
                if not getSubject(classStatement).is_blank():
                    add(classdomains,
                        str(getSubject(classStatement)),
                        str(getSubject(domain)))
            if not getSubject(classStatement).is_blank():
                klass = getSubject(classStatement)
                if klass not in classlist and str(klass).startswith(ns):
                    classlist.append(klass)

    # Create a list of properties in the schema.
    proptypes = [rdf.Property, owl.ObjectProperty, owl.DatatypeProperty, owl.AnnotationProperty]
    proplist = []
    for onetype in proptypes:
        for propertyStatement in findStatements(m, None, rdf.type, onetype):
            prop = getSubject(propertyStatement)
            if prop not in proplist and str(prop).startswith(ns):
                proplist.append(prop)

    return classlist, proplist


def specProperty(m, subject, predicate):
    "Return a property of the spec."
    for c in findStatements(m, None, predicate, None):
        if getSubject(c).is_resource() and str(getSubject(c)) == str(subject):
            return getObject(c).literal_value['string']
    return ''


def specProperties(m, subject, predicate):
    "Return a property of the spec."
    properties = []
    for c in findStatements(m, None, predicate, None):
        if getSubject(c).is_resource() and str(getSubject(c)) == str(subject):
            properties += [getObject(c)]
    return properties


def specAuthors(m, subject):
    "Return an HTML description of the authors of the spec."
    dev = set()
    for i in findStatements(m, None, doap.developer, None):
        for j in findStatements(m, getObject(i), foaf.name, None):
            dev.add(getObject(j).literal_value['string'])

    maint = set()
    for i in findStatements(m, None, doap.maintainer, None):
        for j in findStatements(m, getObject(i), foaf.name, None):
            maint.add(getObject(j).literal_value['string'])

    doc = ''
    first = True
    for d in dev:
        if not first:
            doc += ', '
        doc += '<span class="author" property="doap:developer">%s</span>' % d
        first = False

    for m in maint:
        if not first:
            doc += ', '
        doc += '<span class="author" property="doap:maintainer">%s</span>' % m
        first = False

    n_authors = len(dev) + len(maint)
    if n_authors == 0:
        return ''
    elif n_authors == 1:
        return '<tr><th class="metahead">Author</th><td>' + doc + '</td></tr>'
    else:
        return '<tr><th class="metahead">Authors</th><td>' + doc + '</td></tr>'


def specVersion(m, subject):
    """
    Return a (minorVersion, microVersion, date) tuple
    """
    # Get the date from the latest doap release
    latest_doap_revision = ""
    latest_doap_release = None
    for i in findStatements(m, None, doap.release, None):
        for j in findStatements(m, getObject(i), doap.revision, None):
            revision = getObject(j).literal_value['string']
            if latest_doap_revision == "" or revision > latest_doap_revision:
                latest_doap_revision = revision
                latest_doap_release = getObject(i)
    date = ""
    if latest_doap_release != None:
        for i in findStatements(m, latest_doap_release, doap.created, None):
            date = getObject(i).literal_value['string']

    # Get the LV2 version
    minor_version = 0
    micro_version = 0
    for i in findStatements(m, None, lv2.minorVersion, None):
        minor_version = int(getObject(i).literal_value['string'])
    for i in findStatements(m, None, lv2.microVersion, None):
        micro_version = int(getObject(i).literal_value['string'])
    return (minor_version, micro_version, date)


def getInstances(model, classes, properties):
    """
    Extract all resources instanced in the ontology
    (aka "everything that is not a class or a property")
    """
    instances = []
    for c in classes:
        for i in findStatements(model, None, rdf.type, c):
            if not getSubject(i).is_resource():
                continue
            inst = getSubject(i)
            if inst not in instances and str(inst) != spec_url:
                instances.append(inst)
    for i in findStatements(model, None, rdf.type, None):
        if ((not getSubject(i).is_resource())
            or (getSubject(i) in classes)
            or (getSubject(i) in instances)
            or (getSubject(i) in properties)):
            continue
        full_uri = str(getSubject(i))
        if (full_uri.startswith(spec_ns_str)):
            instances.append(getSubject(i))
    return instances


def specgen(specloc, docdir, template, doclinks, instances=False, mode="spec"):
    """The meat and potatoes: Everything starts here."""

    global spec_url
    global spec_ns_str
    global spec_ns
    global spec_pre
    global ns_list

    # Build a symbol -> link mapping for external links
    dlfile = open(doclinks, 'r')
    for line in dlfile:
        sym, _, url = line.rstrip().partition(' ')
        linkmap[sym] = '<span><a href="%s">%s</a></span>' % (
            os.path.join(doc_base, url), sym)

    m = RDF.Model()
    try:
        base = specloc[0:specloc.rfind('/')]

        # Parse manifest.ttl
        manifest_path = os.path.join(base, 'manifest.ttl')
        p = RDF.Parser(name="guess")
        p.parse_into_model(m, manifest_path)

        # Parse ontology file
        p = RDF.Parser(name="guess")
        p.parse_into_model(m, specloc)

    except:
        e = sys.exc_info()[1]
        print('Error reading ontology: ' + str(e))
        return None

    spec_url = getOntologyNS(m)

    spec_ns_str = spec_url
    if (spec_ns_str[-1] != "/" and spec_ns_str[-1] != "#"):
        spec_ns_str += "#"

    spec_ns = RDF.NS(spec_ns_str)

    namespaces = getNamespaces(p)
    keys = namespaces.keys()
    keys.sort()
    prefixes_html = "<span>"
    for i in keys:
        uri = namespaces[i]
        if spec_pre is None and str(uri) == str(spec_url + '#'):
            spec_pre = i
        prefixes_html += '<a href="%s">%s</a> ' % (uri, i)
    prefixes_html += "</span>"

    if spec_pre is None:
        print('No namespace prefix for specification defined')
        sys.exit(1)

    ns_list[spec_ns_str] = spec_pre

    classlist, proplist = specInformation(m, spec_ns_str)
    classlist = sorted(classlist)
    proplist = sorted(proplist)

    instalist = None
    if instances:
        instalist = getInstances(m, classlist, proplist)
        instalist.sort(lambda x, y: cmp(getShortName(x).lower(), getShortName(y).lower()))

    azlist = buildIndex(m, classlist, proplist, instalist)

    # Generate Term HTML
    termlist = docTerms('Property', proplist, m)
    termlist = docTerms('Class', classlist, m) + termlist
    if instances:
        termlist += docTerms('Instance', instalist, m)

    template = re.sub(r"^#format \w*\n", "", template)
    template = re.sub(r"\$VersionInfo\$", owlVersionInfo(m).encode("utf-8"), template)

    template = template.replace('@NAME@', specProperty(m, spec_url, doap.name))
    template = template.replace('@URI@', spec_url)
    template = template.replace('@PREFIX@', spec_pre)
    if spec_pre == 'lv2':
        template = template.replace('@XMLNS@', '')
    else:
        template = template.replace('@XMLNS@', '      xmlns:%s="%s"' % (spec_pre, spec_ns_str))

    filename = os.path.basename(specloc)
    basename = filename[0:filename.rfind('.')]

    template = template.replace('@PREFIXES@', str(prefixes_html))
    template = template.replace('@BASE@', spec_ns_str)
    template = template.replace('@AUTHORS@', specAuthors(m, spec_url))
    template = template.replace('@INDEX@', azlist)
    template = template.replace('@REFERENCE@', termlist.encode("utf-8"))
    template = template.replace('@FILENAME@', filename)
    template = template.replace('@HEADER@', basename + '.h')
    template = template.replace('@MAIL@', 'devel@lists.lv2plug.in')

    version = specVersion(m, spec_url)  # (minor, micro, date)
    date_string = version[2]
    if date_string == "":
        date_string = "Undated"

    version_string = "%s.%s (%s)" % (version[0], version[1], date_string)
    if version[0] == 0 or version[1] % 2 == 1:
        version_string += ' <span style="color: red; font-weight: bold">EXPERIMENTAL</span>'

    template = template.replace('@REVISION@', version_string)

    bundle_path = os.path.split(specloc[specloc.find(':') + 1:])[0]
    header_path = bundle_path + '/' + basename + '.h'

    other_files = ''
    if version[0] != '0':
        release_name = "lv2-" + basename
        if basename == "lv2":
            release_name = "lv2core"
        other_files += '<a href="http://lv2plug.in/spec/%s-%d.%d.tar.bz2">Release</a>' % (release_name, version[0], version[1])
        other_files += ', <a href="http://lv2plug.in/spec">All releases</a>'
    if os.path.exists(os.path.abspath(header_path)):
        other_files += ', <a href="' + docdir + '/html/%s">API documentation</a>' % (
            basename + '_8h.html')

        header = basename + '.h'
        other_files += ', <a href="%s">%s</a>' % (header, header)

    #other_files += '<li><a href="%s">Ontology</a> %s</li>\n' % (filename, filename)

    see_also_files = specProperties(m, spec_url, rdfs.seeAlso)
    for f in see_also_files:
        uri = str(f)
        if uri[0:5] == 'file:':
            uri = uri[5:]

        other_files += ', <a href="%s">%s</a>' % (uri, uri)

    other_files = '<tr><th class="metahead">See Also</th><td>%s</td></tr>' % other_files

    template = template.replace('@FILES@', other_files)

    comment = getComment(m, RDF.Uri(spec_url))
    if comment != '':
        template = template.replace('@COMMENT@', comment)
    else:
        template = template.replace('@COMMENT@', '')

    template = template.replace('@TIME@', datetime.datetime.utcnow().strftime('%F %H:%M UTC'))

    return template


def save(path, text):
    try:
        f = open(path, "w")
        f.write(text)
        f.flush()
        f.close()
    except Exception:
        e = sys.exc_info()[1]
        print('Error writing to file "' + path + '": ' + str(e))


def getNamespaces(parser):
    """Return a prefix:URI dictionary of all namespaces seen during parsing"""
    count = Redland.librdf_parser_get_namespaces_seen_count(parser._parser)
    nspaces = {}
    for index in range(0, count):
        prefix = Redland.librdf_parser_get_namespaces_seen_prefix(parser._parser, index)
        uri_obj = Redland.librdf_parser_get_namespaces_seen_uri(parser._parser, index)
        if uri_obj is None:
            uri = None
        else:
            uri = RDF.Uri(from_object=uri_obj)
        nspaces[prefix] = uri
    return nspaces


def getOntologyNS(m):
    ns = None
    s = findOne(m, None, rdf.type, lv2.Specification)
    if s:
        if (not getSubject(s).is_blank()):
            ns = str(getSubject(s))

    if (ns == None):
        sys.exit("Impossible to get ontology's namespace")
    else:
        return ns


def usage():
    script = os.path.basename(sys.argv[0])
    print("""Usage: %s ONTOLOGY TEMPLATE STYLE OUTPUT [FLAGS]

        ONTOLOGY : Path to ontology file
        TEMPLATE : HTML template path
        STYLE    : CSS style path
        OUTPUT   : HTML output path
        BASE     : Documentation output base URI

        Optional flags:
                -i        : Document class/property instances (disabled by default)
                -p PREFIX : Set ontology namespace prefix from command line

Example:
    %s lv2_foos.ttl template.html style.css lv2_foos.html ../doc -i -p foos
""" % (script, script))
    sys.exit(-1)


if __name__ == "__main__":
    """Ontology specification generator tool"""

    args = sys.argv[1:]
    if (len(args) < 3):
        usage()
    else:

        # Ontology
        specloc = "file:" + str(args[0])

        # Template
        temploc = args[1]
        template = None
        try:
            f = open(temploc, "r")
            template = f.read()
        except Exception:
            e = sys.exc_info()[1]
            print("Error reading from template \"" + temploc + "\": " + str(e))
            usage()

        # Footer
        footerloc = temploc.replace('template', 'footer')
        footer = ''
        try:
            f = open(footerloc, "r")
            footer = f.read()
        except Exception:
            e = sys.exc_info()[1]
            print("Error reading from footer \"" + footerloc + "\": " + str(e))
            usage()

        template = template.replace('@FOOTER@', footer)

        # Style
        style_uri = args[2]

        # Destination
        dest = args[3]

        # Doxygen documentation directory
        doc_base = args[4]

        # C symbol -> doxygen link mapping
        doc_links = args[5]

        template = template.replace('@STYLE_URI@', os.path.join(doc_base, style_uri))

        docdir = os.path.join(doc_base, 'ns', 'doc')

        # Flags
        instances = False
        if len(args) > 5:
            flags = args[5:]
            i = 0
            while i < len(flags):
                if flags[i] == '-i':
                    instances = True
                elif flags[i] == '-p':
                    spec_pre = flags[i + 1]
                    i += 1
                i += 1

        save(dest, specgen(specloc, docdir, template, doc_links, instances=instances))
