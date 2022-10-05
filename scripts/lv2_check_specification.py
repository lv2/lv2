#!/usr/bin/env python3

# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

"""
Check an LV2 specification for issues.
"""

import argparse
import os
import sys

import rdflib

foaf = rdflib.Namespace("http://xmlns.com/foaf/0.1/")
lv2 = rdflib.Namespace("http://lv2plug.in/ns/lv2core#")
owl = rdflib.Namespace("http://www.w3.org/2002/07/owl#")
rdf = rdflib.Namespace("http://www.w3.org/1999/02/22-rdf-syntax-ns#")
rdfs = rdflib.Namespace("http://www.w3.org/2000/01/rdf-schema#")


class Checker:
    "A callable that checks conditions and records pass/fail counts."

    def __init__(self, verbose=False):
        self.num_checks = 0
        self.num_errors = 0
        self.verbose = verbose

    def __call__(self, condition, name):
        if not condition:
            sys.stderr.write(f"error: Unmet condition: {name}\n")
            self.num_errors += 1
        elif self.verbose:
            sys.stderr.write(f"note: {name}\n")

        self.num_checks += 1
        return condition

    def print_summary(self):
        "Print a summary (if verbose) when all checks are finished."

        if self.verbose:
            if self.num_errors:
                sys.stderr.write(f"note: Failed {self.num_errors}/")
            else:
                sys.stderr.write("note: Passed all ")

            sys.stderr.write(f"{self.num_checks} checks\n")


def _check(condition, name):
    "Check that condition is true, returning 1 on failure."

    if not condition:
        sys.stderr.write(f"error: Unmet condition: {name}\n")
        return 1

    return 0


def _has_statement(model, pattern):
    "Return true if model contains a triple matching pattern."

    for _ in model.triples(pattern):
        return True

    return False


def _has_property(model, subject, predicate):
    "Return true if subject has any value for predicate in model."

    return model.value(subject, predicate, None) is not None


def _check_version(checker, model, spec, is_stable):
    "Check that the version of a specification is present and valid."

    minor = model.value(spec, lv2.minorVersion, None, any=False)
    checker(minor is not None, f"{spec} has a lv2:minorVersion")

    micro = model.value(spec, lv2.microVersion, None, any=False)
    checker(micro is not None, f"{spec} has a lv2:microVersion")

    if is_stable:
        checker(int(minor) > 0, f"{spec} has a non-zero minor version")
        checker(int(micro) % 2 == 0, f"{spec} has an even micro version")


def _check_specification(checker, spec_dir, is_stable=False):
    "Check all specification data for errors and omissions."

    # Load manifest
    manifest_path = os.path.join(spec_dir, "manifest.ttl")
    model = rdflib.Graph()
    model.parse(manifest_path, format="n3")

    # Get the specification URI from the manifest
    spec_uri = model.value(None, rdf.type, lv2.Specification, any=False)
    if not checker(
        spec_uri is not None,
        manifest_path + " declares an lv2:Specification",
    ):
        return 1

    # Check that the manifest declares a valid version
    _check_version(checker, model, spec_uri, is_stable)

    # Get the link to the main document from the manifest
    document = model.value(spec_uri, rdfs.seeAlso, None, any=False)
    if not checker(
        document is not None,
        manifest_path + " has one rdfs:seeAlso link to the definition",
    ):
        return 1

    # Load main document into the model
    model.parse(document, format="n3")

    # Check that the main data files aren't bloated with extended documentation
    checker(
        not _has_statement(model, [None, lv2.documentation, None]),
        f"{document} has no lv2:documentation",
    )

    # Load all other directly linked data files (for any other subjects)
    for link in sorted(model.triples([None, rdfs.seeAlso, None])):
        if link[2] != document and link[2].endswith(".ttl"):
            model.parse(link[2], format="n3")

    # Check that all properties have a more specific type
    for typing in sorted(model.triples([None, rdf.type, rdf.Property])):
        subject = typing[0]

        checker(isinstance(subject, rdflib.term.URIRef), f"{subject} is a URI")

        if str(subject) == "http://lv2plug.in/ns/ext/patch#value":
            continue  # patch:value is just a "promiscuous" rdf:Property

        types = list(model.objects(subject, rdf.type))

        checker(
            (owl.DatatypeProperty in types)
            or (owl.ObjectProperty in types)
            or (owl.AnnotationProperty in types),
            f"{subject} is a Datatype, Object, or Annotation property",
        )

    # Get all subjects that have an explicit rdf:type
    typed_subjects = set()
    for subject in model.subjects(rdf.type, None):
        typed_subjects.add(subject)

    # Check that all named and typed resources have labels and comments
    for subject in typed_subjects:
        if isinstance(
            subject, rdflib.term.BNode
        ) or foaf.Person in model.objects(subject, rdf.type):
            continue

        if checker(
            _has_property(model, subject, rdfs.label),
            f"{subject} has a rdfs:label",
        ):
            label = str(model.value(subject, rdfs.label, None))

            checker(
                not label.endswith("."),
                f"{subject} label has no trailing '.'",
            )
            checker(
                label.find("\n") == -1,
                f"{subject} label is a single line",
            )
            checker(
                label == label.strip(),
                f"{subject} label has stripped whitespace",
            )

        if checker(
            _has_property(model, subject, rdfs.comment),
            f"{subject} has a rdfs:comment",
        ):
            comment = str(model.value(subject, rdfs.comment, None))

            checker(
                comment.endswith("."),
                f"{subject} comment has a trailing '.'",
            )
            checker(
                comment.find("\n") == -1 and comment.find("\r"),
                f"{subject} comment is a single line",
            )
            checker(
                comment == comment.strip(),
                f"{subject} comment has stripped whitespace",
            )

        # Check that lv2:documentation, if present, is proper Markdown
        documentation = model.value(subject, lv2.documentation, None)
        if documentation is not None:
            checker(
                documentation.datatype == lv2.Markdown,
                f"{subject} documentation is explicitly Markdown",
            )
            checker(
                str(documentation).startswith("\n\n"),
                f"{subject} documentation starts with blank line",
            )
            checker(
                str(documentation).endswith("\n\n"),
                f"{subject} documentation ends with blank line",
            )

    return checker.num_errors


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        usage="%(prog)s [OPTION]... BUNDLE",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    ap.add_argument(
        "--stable",
        action="store_true",
        help="enable checks for stable release versions",
    )

    ap.add_argument(
        "-v", "--verbose", action="store_true", help="print successful checks"
    )

    ap.add_argument(
        "BUNDLE", help="path to specification bundle or manifest.ttl"
    )

    args = ap.parse_args(sys.argv[1:])

    if os.path.basename(args.BUNDLE):
        args.BUNDLE = os.path.dirname(args.BUNDLE)

    sys.exit(
        _check_specification(Checker(args.verbose), args.BUNDLE, args.stable)
    )
