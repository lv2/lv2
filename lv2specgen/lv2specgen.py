#!/usr/bin/python
# -*- coding: utf8 -*-
#
# lv2specgen, an LV2 extension specification page generator
# Copyright (c) 2009 David Robillard <d@drobilla.net>
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

__version__ = "1.0.0"
__authors__ = "Christopher Schmidt, Uldis Bojars, Sergio Fernández, David Robillard"
__license__ = "MIT License <http://www.opensource.org/licenses/mit-license.php>"
__contact__ = "devel@lists.lv2plug.in"
__date__    = "2009-06-11"
 
import os
import sys
import datetime
import re
import urllib

try:
    import RDF
    import Redland
except ImportError:
    version = sys.version.split(" ")[0]
    if version.startswith("2.5"):
        sys.path.append("/usr/lib/python2.4/site-packages/")
    else:
        sys.path.append("/usr/lib/python2.5/site-packages/")
    try:
        import RDF
    except:
        sys.exit("Error importing Redland bindings for Python; check if it is installed correctly")

#global vars
classranges = {}
classdomains = {}
spec_url = None
spec_ns_str = None
spec_ns = None
spec_pre = None
ns_list = { "http://www.w3.org/1999/02/22-rdf-syntax-ns#"   : "rdf",
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


def niceName(uri):
    regexp = re.compile( "^(.*[/#])([^/#]+)$" )
    rez = regexp.search( uri )
    if not rez:
        return uri
    pref = rez.group(1)
    if ns_list.has_key(pref):
        return ns_list.get(pref, pref) + ":" + rez.group(2)
    else:
        return uri


def return_name(m, urinode):
    "Trims the namespace out of a term to give a name to the term."
    return str(urinode.uri).replace(spec_ns_str, "")


def get_rdfs(m, urinode):
    "Returns label and comment given an RDF.Node with a URI in it"
    comment = ''
    label = ''
    if (type(urinode)==str):
        urinode = RDF.Uri(urinode)
    l = m.find_statements(RDF.Statement(urinode, rdfs.label, None))
    if l.current():
        label = l.current().object.literal_value['string']
    c = m.find_statements(RDF.Statement(urinode, rdfs.comment, None))
    if c.current():
        comment = c.current().object.literal_value['string']
    return label, comment


def owlVersionInfo(m):
    v = m.find_statements(RDF.Statement(None, owl.versionInfo, None))
    if v.current():
        return v.current().object.literal_value['string']
    else:
        return ""


def rdfsPropertyInfo(term,m):
    """Generate HTML for properties: Domain, range"""
    global classranges
    global classdomains
    doc = ""
    range = ""
    domain = ""

    # Find subPropertyOf information
    o = m.find_statements( RDF.Statement(term, rdfs.subPropertyOf, None) )
    if o.current():
        rlist = ''
        for st in o:
            k = getTermLink(str(st.object.uri), term, rdfs.subPropertyOf)
            rlist += "<dd>%s</dd>" % k
        doc += "<dt>Sub-property of</dt> %s" % rlist

    # Domain stuff
    domains = m.find_statements(RDF.Statement(term, rdfs.domain, None))
    domainsdoc = ""
    for d in domains:
        collection = m.find_statements(RDF.Statement(d.object, owl.unionOf, None))
        if collection.current():
            uris = parseCollection(m, collection)
            for uri in uris:
                domainsdoc += "<dd>%s</dd>" % getTermLink(uri, term, rdfs.domain)
                add(classdomains, uri, term.uri)
        else:
            if not d.object.is_blank():
                domainsdoc += "<dd>%s</dd>" % getTermLink(str(d.object.uri), term, rdfs.domain)
    if (len(domainsdoc)>0):
        doc += "<dt>Domain</dt> %s" % domainsdoc

    # Range stuff
    ranges = m.find_statements(RDF.Statement(term, rdfs.range, None))
    rangesdoc = ""
    for r in ranges:
        collection = m.find_statements(RDF.Statement(r.object, owl.unionOf, None))
        if collection.current():
            uris = parseCollection(m, collection)
            for uri in uris:
                rangesdoc += "<dd>%s</dd>" % getTermLink(uri, term, rdfs.range)
                add(classranges, uri, term.uri)
        else:
            if not r.object.is_blank():
                rangesdoc += "<dd>%s</dd>" % getTermLink(str(r.object.uri), term, rdfs.range)
    if (len(rangesdoc)>0):
        doc += "<dt>Range</dt> %s" % rangesdoc

    return doc


def parseCollection(model, collection):
    uris = []
    rdflist = model.find_statements(RDF.Statement(collection.current().object, None, None))
    while rdflist and rdflist.current() and not rdflist.current().object.is_blank():
        one = rdflist.current()
        if not one.object.is_blank():
            uris.append(str(one.object.uri))
        rdflist.next()
        one = rdflist.current()
        if one.predicate == rdf.rest:
            rdflist = model.find_statements(RDF.Statement(one.object, None, None))
    
    return uris


def getTermLink(uri, subject=None, predicate=None):
    uri = str(uri)
    extra = ''
    if subject != None and predicate != None:
        extra = 'about="%s" rel="%s" resource="%s"' % (str(subject.uri), niceName(str(predicate.uri)), uri)
    if (uri.startswith(spec_ns_str)):
        return '<a href="#%s" style="font-family: monospace;" %s>%s</a>' % (uri.replace(spec_ns_str, ""), extra, niceName(uri))
    else:
        return '<a href="%s" style="font-family: monospace;" %s>%s</a>' % (uri, extra, niceName(uri))


def rdfsClassInfo(term,m):
    """Generate rdfs-type information for Classes: ranges, and domains."""
    global classranges
    global classdomains
    doc = ""

    # Find subClassOf information
    o = m.find_statements( RDF.Statement(term, rdfs.subClassOf, None) )
    restrictions = []
    if o.current():
        superclasses = []
        for st in o:
            if not st.object.is_blank():
                uri = str(st.object.uri)
                if (not uri in superclasses):
                    superclasses.append(uri)
            else:
                meta_types = m.find_statements(RDF.Statement(o.current().object, rdf.type, None))
                restrictions.append(meta_types.current().subject)

        if len(superclasses) > 0:
            doc += "\n<dt>Sub-class of</dt>"
            for superclass in superclasses:
                doc += "<dd>%s</dd>" % getTermLink(superclass)

    for r in restrictions:
        props = m.find_statements(RDF.Statement(r, None, None))
        onProp = None
        comment = None
        for p in props:
            if p.predicate == owl.onProperty:
                onProp = p.object
            elif p.predicate == rdfs.comment:
                comment = p.object
        if onProp != None:
            doc += '<dt>Restriction on property %s</dt>\n' % getTermLink(onProp.uri)
            doc += '<dd class="restriction">\n'
            if comment != None:
                doc += "<p>%s</p>\n" % comment

            prop_str = ''
            for p in m.find_statements(RDF.Statement(r, None, None)):
                if p.predicate != owl.onProperty and p.predicate != rdfs.comment and not(
                        p.predicate == rdf.type and p.object == owl.Restriction):
                    if p.object.is_resource():
                        prop_str += '\n<dt>%s</dt><dd>%s</dd>\n' % (
                                getTermLink(p.predicate.uri), getTermLink(p.object.uri))
                    elif p.object.is_literal():
                        prop_str += '\n<dt>%s</dt><dd>%s</dd>\n' % (
                                getTermLink(p.predicate.uri), p.object.literal_value['string'])
            if prop_str != '':
                doc += '<dl class=\"prop\">%s</dl>\n' % prop_str
            doc += '</dd>'

    # Find out about properties which have rdfs:domain of t
    d = classdomains.get(str(term.uri), "")
    if d:
        dlist = ''
        for k in d:
            dlist += "<dd>%s</dd>" % getTermLink(k)
        doc += "<dt>In domain of</dt>" + dlist

    # Find out about properties which have rdfs:range of t
    r = classranges.get(str(term.uri), "")
    if r:
        rlist = ''
        for k in r:
            rlist += "<dd>%s</dd>" % getTermLink(k)
        doc += "<dt>In range of</dt>" + rlist

    return doc


def isSpecial(pred):
    """Return True if the predicate is "special" and shouldn't be emitted generically"""
    return pred == rdf.type or pred == rdfs.range or pred == rdfs.domain or pred == rdfs.label or pred == rdfs.comment or pred == rdfs.subClassOf


def blankNodeDesc(node,m):
    properties = m.find_statements(RDF.Statement(node, None, None))
    doc = ''
    last_pred = ''
    for p in properties:
        if isSpecial(p.predicate):
            continue
        doc += '<tr>'
        doc += '<td class="blankterm">%s</td>\n' % getTermLink(str(p.predicate.uri))
        if p.object.is_resource():
            doc += '<td class="blankdef">%s</td>\n' % getTermLink(str(p.object.uri))#getTermLink(str(p.object.uri), node, p.predicate)
        elif p.object.is_literal():
            doc += '<td class="blankdef">%s</td>\n' % str(p.object.literal_value['string'])
        elif p.object.is_blank():
            doc += '<td class="blankdef">' + blankNodeDesc(p.object,m) + '</td>\n'
        else:
            doc += '<td class="blankdef">?</td>\n'
        doc += '</tr>'
    if doc != '':
        doc = '<table class="blankdesc">\n%s\n</table>\n' % doc
    return doc


def extraInfo(term,m):
    """Generate information about misc. properties of a term"""
    doc = ""
    properties = m.find_statements(RDF.Statement(term, None, None))
    last_pred = ''
    for p in properties:
        if isSpecial(p.predicate):
            continue
        if p.predicate != last_pred:
            doc += '<dt>%s</dt>\n' % getTermLink(str(p.predicate.uri))
        if p.object.is_resource():
            doc += '<dd>%s</dd>\n' % getTermLink(str(p.object.uri), term, p.predicate)
        elif p.object.is_literal():
            doc += '<dd>%s</dd>\n' % str(p.object)
        elif p.object.is_blank():
            doc += '<dd>' + blankNodeDesc(p.object,m) + '</dd>\n'
        else:
            doc += '<dd>?</dd>\n'
        last_pred = p.predicate
    return doc


def rdfsInstanceInfo(term,m):
    """Generate rdfs-type information for instances"""
    doc = ""
    
    t = m.find_statements( RDF.Statement(term, rdf.type, None) )
    if t.current():
        doc += "<dt>Type</dt>"
    while t.current():
        doc += "<dd>%s</dd>" % getTermLink(str(t.current().object.uri), term, rdf.type)
        t.next()

    doc += extraInfo(term, m)

    return doc


def owlInfo(term,m):
    """Returns an extra information that is defined about a term (an RDF.Node()) using OWL."""
    res = ''

    # Inverse properties ( owl:inverseOf )
    o = m.find_statements(RDF.Statement(term, owl.inverseOf, None))
    if o.current():
        res += "<dt>Inverse:</dt>"
        for st in o:
            res += "<dd>%s</dd>" % getTermLink(str(st.object.uri))
    
    def owlTypeInfo(term, propertyType, name):
        o = m.find_statements(RDF.Statement(term, rdf.type, propertyType))
        if o.current():
            return "<dt>OWL Type</dt><dd>%s</dd>\n" % name
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
    for t in list:
        if (t.startswith(spec_ns_str)) and (len(t[len(spec_ns_str):].split("/"))<2):
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
        
        doc += """<div class="specterm" id="%s" about="%s">\n<h3>%s <a href="%s">%s</a></h3>\n""" % (t, term_uri, category, term_uri, curie)

        label, comment = get_rdfs(m, term)    
        if label!='' or comment != '':
            doc += '<div class="description">'
        if label!='':
            doc += "<div property=\"rdfs:label\" class=\"label\">%s</div>" % label
        if comment!='':
            doc += "<div property=\"rdfs:comment\">%s</div>" % comment.replace('\n\n', '<br /><br />')
        if label!='' or comment != '':
            doc += "</div>"
        terminfo = ""
        if category=='Property':
            terminfo += owlInfo(term,m)
            terminfo += rdfsPropertyInfo(term,m)
        if category=='Class':
            terminfo += rdfsClassInfo(term,m)
        if category=='Instance':
            terminfo += rdfsInstanceInfo(term,m)
        
        terminfo += extraInfo(term,m)
        
        if (len(terminfo)>0): #to prevent empty list (bug #882)
            doc += '\n<dl class="terminfo">%s</dl>\n' % terminfo
        
        doc += "\n\n</div>\n\n"
    
    return doc


def getShortName(uri):
    if ("#" in uri):
        return uri.split("#")[-1]
    else:
        return uri.split("/")[-1]


def getAnchor(uri):
    if (uri.startswith(spec_ns_str)):
        return uri[len(spec_ns_str):].replace("/","_")
    else:
        return getShortName(uri)


def buildIndex(classlist, proplist, instalist=None):
    """
    Builds the A-Z list of terms. Args are a list of classes (strings) and 
    a list of props (strings)
    """

    if len(classlist)==0 and len(proplist)==0 and (not instalist or len(instalist)==0):
        return ''

    azlist = '<dl class="index">'

    if (len(classlist)>0):
        azlist += "<dt>Classes</dt><dd>"
        classlist.sort()
        for c in classlist:
            if c.startswith(spec_ns_str):
                c = c.split(spec_ns_str[-1])[1]
            azlist = """%s <a href="#%s">%s</a>, """ % (azlist, c, c)
        azlist = """%s</dd>\n""" % azlist

    if (len(proplist)>0):
        azlist += "<dt>Properties</dt><dd>"
        proplist.sort()
        for p in proplist:
            if p.startswith(spec_ns_str):
                p = p.split(spec_ns_str[-1])[1]
            azlist = """%s <a href="#%s">%s</a>, """ % (azlist, p, p)
        azlist = """%s</dd>\n""" % azlist

    if (instalist!=None and len(instalist)>0):
        azlist += "<dt>Instances</dt><dd>"
        for i in instalist:
            p = getShortName(i)
            anchor = getAnchor(i)
            azlist = """%s <a href="#%s">%s</a>, """ % (azlist, anchor, p)
        azlist = """%s</dd>\n""" % azlist

    azlist = """%s\n</dl>""" % azlist
    return azlist


def add(where, key, value):
    if not where.has_key(key):
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
        for classStatement in m.find_statements(RDF.Statement(None, rdf.type, onetype)):
            for range in m.find_statements(RDF.Statement(None, rdfs.range, classStatement.subject)):
                if not m.contains_statement( RDF.Statement( range.subject, rdf.type, owl.DeprecatedProperty )):
                    if not classStatement.subject.is_blank():
                        add(classranges, str(classStatement.subject.uri), str(range.subject.uri))
            for domain in m.find_statements(RDF.Statement(None, rdfs.domain, classStatement.subject)):
                if not m.contains_statement( RDF.Statement( domain.subject, rdf.type, owl.DeprecatedProperty )):
                    if not classStatement.subject.is_blank():
                        add(classdomains, str(classStatement.subject.uri), str(domain.subject.uri))
            if not classStatement.subject.is_blank():
                uri = str(classStatement.subject.uri)
                name = return_name(m, classStatement.subject)
                if name not in classlist and uri.startswith(ns):
                    classlist.append(return_name(m, classStatement.subject))

    # Create a list of properties in the schema.
    proptypes = [rdf.Property, owl.ObjectProperty, owl.DatatypeProperty, owl.AnnotationProperty]
    proplist = []
    for onetype in proptypes: 
        for propertyStatement in m.find_statements(RDF.Statement(None, rdf.type, onetype)):
            uri = str(propertyStatement.subject.uri)
            name = return_name(m, propertyStatement.subject)
            if uri.startswith(ns) and not name in proplist:
                proplist.append(name)

    return classlist, proplist


def specProperty(m, subject, predicate):
    "Return a property of the spec."
    for c in m.find_statements(RDF.Statement(None, predicate, None)):
        if c.subject.is_resource() and str(c.subject.uri) == str(subject):
            return c.object.literal_value['string']
    return ''


def specProperties(m, subject, predicate):
    "Return a property of the spec."
    properties=[]
    for c in m.find_statements(RDF.Statement(None, predicate, None)):
        if c.subject.is_resource() and str(c.subject.uri) == str(subject):
            properties += [c.object]
    return properties


def specAuthors(m, subject):
    "Return an HTML description of the authors of the spec."
    ret = ''
    for i in m.find_statements(RDF.Statement(None, doap.maintainer, None)):
        for j in m.find_statements(RDF.Statement(i.object, foaf.name, None)):
            ret += '<div class="author" property="dc:creator">' + j.object.literal_value['string'] + '</div>'
    return ret


def specRevision(m, subject):
    """
    Return a (revision date) tuple, both strings, about the latest
    release of the specification
    """
    latest_revision = ""
    latest_release = None
    for i in m.find_statements(RDF.Statement(None, doap.release, None)):
        for j in m.find_statements(RDF.Statement(i.object, doap.revision, None)):
            revision = j.object.literal_value['string']
            if latest_revision == "" or revision > latest_revision:
                latest_revision = revision
                latest_release = i.object
    if latest_release != None:
        for i in m.find_statements(RDF.Statement(latest_release, doap.created, None)):
            return (latest_revision, i.object.literal_value['string'])


def getInstances(model, classes, properties):
    """
    Extract all resources instanced in the ontology
    (aka "everything that is not a class or a property")
    """
    instances = []
    for one in classes:
        for i in model.find_statements(RDF.Statement(None, rdf.type, spec_ns[one])):
            uri = str(i.subject.uri)
            if not uri in instances:
                instances.append(uri)
    for i in model.find_statements(RDF.Statement(None, rdf.type, None)):
        if not i.subject.is_resource():
            continue;
        full_uri = str(i.subject.uri)
        if (full_uri.startswith(spec_ns_str)):
            uri = full_uri[len(spec_ns_str):]
        else:
            uri = full_uri
        if ((not full_uri in instances) and (not uri in classes) and (not uri in properties) and (full_uri != spec_url)):
            instances.append(full_uri)
    return instances
   
 
def specgen(specloc, docdir, template, instances=False, mode="spec"):
    """The meat and potatoes: Everything starts here."""

    global spec_url
    global spec_ns_str
    global spec_ns
    global spec_pre
    global ns_list
        
    m = RDF.Model()
    p = RDF.Parser(name="guess")
    try:
        p.parse_into_model(m, specloc)
    except IOError, e:
        print "Error reading from ontology:", str(e)
        usage()
    except RDF.RedlandError, e:
        print "Error parsing the ontology"

    spec_url = getOntologyNS(m)

    spec_ns_str = spec_url
    if (spec_ns_str[-1]!="/" and spec_ns_str[-1]!="#"):
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
        print 'No namespace prefix for specification defined'
        sys.exit(1)
    
    ns_list[spec_ns_str] = spec_pre

    classlist, proplist = specInformation(m, spec_ns_str)
    classlist = sorted(classlist)
    proplist = sorted(proplist)
        
    instalist = None
    if instances:
        instalist = getInstances(m, classlist, proplist)
        instalist.sort(lambda x, y: cmp(getShortName(x).lower(), getShortName(y).lower()))
    
    azlist = buildIndex(classlist, proplist, instalist)

    # Generate Term HTML
    termlist = docTerms('Property', proplist, m)
    termlist = docTerms('Class', classlist, m) + termlist
    if instances:
        termlist += docTerms('Instance', instalist, m)
    
    # Generate RDF from original namespace.
    u = urllib.urlopen(specloc)
    rdfdata = u.read()
    rdfdata = re.sub(r"(<\?xml version.*\?>)", "", rdfdata)
    rdfdata = re.sub(r"(<!DOCTYPE[^]]*]>)", "", rdfdata)
    #rdfdata.replace("""<?xml version="1.0"?>""", "")
    
    # print template % (azlist.encode("utf-8"), termlist.encode("utf-8"), rdfdata.encode("ISO-8859-1"))
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
    template = template.replace('@MAINTAINERS@', specAuthors(m, spec_url))
    template = template.replace('@INDEX@', azlist)
    template = template.replace('@REFERENCE@', termlist.encode("utf-8"))
    template = template.replace('@FILENAME@', filename)
    template = template.replace('@HEADER@', basename + '.h')
    template = template.replace('@MAIL@', 'devel@lists.lv2plug.in')

    revision = specRevision(m, spec_url) # (revision, date)
    if revision:
        template = template.replace('@REVISION@', revision[0] + " (" + revision[1] + ")")
    else:
        template = template.replace('@REVISION@', '<span style="color: red; font-weight: bold">EXPERIMENTAL</span>')

    bundle_path = os.path.split(specloc[specloc.find(':')+1:])[0]
    header_path = bundle_path + '/' + basename + '.h'

    other_files = '<li><a href=".">Bundle</a></li>\n'
    other_files += '<li><a href="../releases">Releases</a></li>\n'
    if os.path.exists(os.path.abspath(header_path)):
        other_files += '<li><a href="' + docdir + '/html/%s">Header Documentation</a></li>\n' % (
            basename + '_8h.html')

        other_files += '<li><a href="%s">Header</a> %s</li>' % (basename + '.h', basename + '.h')

    other_files += '<li><a href="%s">Ontology</a> %s</li>\n' % (filename, filename)

    see_also_files = specProperties(m, spec_url, rdfs.seeAlso)
    for f in see_also_files:
        other_files += '<li><a href="%s">%s</a></li>' % (f, f)

    template = template.replace('@FILES@', other_files);

    comment = specProperty(m, spec_url, rdfs.comment)
    if not comment:
        comment = specProperty(m, spec_url, doap.shortdesc)
    #template = template.replace('@COMMENT@', '<p>' + comment.strip().replace('\n\n', '</p><p>') + '</p>')
    template = template.replace('@COMMENT@', comment.strip().replace('\n\n', '<br /><br />'))
    #template = template.replace('@COMMENT@', comment)

    template = template.replace('@TIME@', datetime.datetime.utcnow().strftime('%F %H:%M UTC'))

    return template


def save(path, text):
    try:
        f = open(path, "w")
        f.write(text)
        f.flush()
        f.close()
    except Exception, e:
        print "Error writing to file \"" + path + "\": " + str(e)


def getNamespaces(parser):
    """Return a prefix:URI dictionary of all namespaces seen during parsing"""
    count = Redland.librdf_parser_get_namespaces_seen_count(parser._parser)
    nspaces = {}
    for index in range(0, count - 1):
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
    o = m.find_statements(RDF.Statement(None, rdf.type, lv2.Specification))
    if o.current():
        s = o.current().subject
        if (not s.is_blank()):
            ns = str(s.uri)

    if (ns == None):
        sys.exit("Impossible to get ontology's namespace")
    else:
        return ns


def usage():
    script = os.path.basename(sys.argv[0])
    print """Usage: 
    %s ONTOLOGY TEMPLATE STYLE OUTPUT [FLAGS]

        ONTOLOGY : Path to ontology file
        TEMPLATE : HTML template path
        STYLE    : CSS style path
        OUTPUT   : HTML output path
        DOCDIR   : Doxygen documentation directory

        Optional flags:
                -i        : Document class/property instances (disabled by default)
                -p PREFIX : Set ontology namespace prefix from command line

Example:
    %s lv2_foos.ttl template.html style.css lv2_foos.html ../docs -i -p foos

""" % (script, script)
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
        except Exception, e:
            print "Error reading from template \"" + temploc + "\": " + str(e)
            usage()
        
        # Footer
        footerloc = temploc.replace('template', 'footer')
        footer = ''
        try:
            f = open(footerloc, "r")
            footer = f.read()
        except Exception, e:
            print "Error reading from footer \"" + footerloc + "\": " + str(e)
            usage()

        template = template.replace('@FOOTER@', footer)
        
        # Style
        styleloc = args[2]
        style = ''
        try:
            f = open(styleloc, "r")
            style = f.read()
        except Exception, e:
            print "Error reading from style \"" + styleloc + "\": " + str(e)
            usage()

        template = template.replace('@STYLE@', style)

        # Destination
        dest = args[3]

        # Doxygen documentation directory
        docdir = args[4]
        
        # Flags
        instances = False
        if len(args) > 4:
            flags = args[4:]
            i = 0
            while i < len(flags):
                if flags[i] == '-i':
                    instances = True
                elif flags[i] == '-p':
                    spec_pre = flags[i + 1]
                    i += 1
                i += 1
        
        save(dest, specgen(specloc, docdir, template, instances=instances))

