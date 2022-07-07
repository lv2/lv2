#!/usr/bin/env python3

# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

"""
Write an HTML index for a set of LV2 specifications.
"""

import datetime
import json
import os
import time
import sys
import argparse
import subprocess

import rdflib


doap = rdflib.Namespace("http://usefulinc.com/ns/doap#")
lv2 = rdflib.Namespace("http://lv2plug.in/ns/lv2core#")
owl = rdflib.Namespace("http://www.w3.org/2002/07/owl#")
rdf = rdflib.Namespace("http://www.w3.org/1999/02/22-rdf-syntax-ns#")


def _subst_file(template_path, output_file, substitutions):
    "Replace keys with values in a template file and write the result."

    with open(template_path, "r", encoding="utf-8") as template:
        for line in template:
            for key, value in substitutions.items():
                line = line.replace(key, value)

            output_file.write(line)


def _load_ttl(data_paths, exclude=None):
    "Load an RDF model from a Turtle file."

    model = rdflib.ConjunctiveGraph()
    for path in data_paths:
        if exclude is None or path not in exclude:
            model.parse(path, format="n3")

    return model


def _warn(message):
    "Load a warning message."

    assert not message.startswith("warning: ")
    assert not message.endswith("\n")
    sys.stderr.write(message)
    sys.stderr.write("\n")


def _spec_target(spec, root, online=False):
    "Return the relative link target for a specification."

    target = spec.removeprefix(root) if spec.startswith(root) else spec

    return target if online else target + ".html"


def _spec_date(model, spec, minor, micro):
    "Return the date for a release of a specification as an RDF node."

    # Get date
    date = None
    for release in model.objects(spec, doap.release):
        revision = model.value(release, doap.revision, None, any=False)
        if str(revision) == f"{minor}.{micro}":
            date = model.value(release, doap.created, None)
            break

    # Verify that this date is the latest
    if date is not None:
        for other_release in model.objects(spec, doap.release):
            for other_date in model.objects(other_release, doap.created):
                if other_date is None:
                    _warn(f"{spec} has no doap:created date")
                elif other_date > date:
                    _warn(f"{spec} {minor}.{micro} ({date}) is an old release")
                    break

    return date


def _spec_link_columns(spec, root, name, online):
    "Return the first two link columns in a spec row as an HTML string."

    # Find relative link target and stem
    target = _spec_target(spec, root, online)
    stem = os.path.splitext(os.path.basename(target))[0]

    # Prefix with a comment to act as a sort key for the row
    col = f"<!-- {stem} -->"

    # Specification
    col += f'<td><a rel="rdfs:seeAlso" href="{target}">{name}</a></td>'

    # API
    col += '<td><a rel="rdfs:seeAlso"'
    col += f' href="../html/group__{stem}.html">{name}'
    col += "</a></td>"

    return col


def _spec_description_column(model, spec):
    "Return the description column in a spec row as an HTML string."

    shortdesc = model.value(spec, doap.shortdesc, None, any=False)

    return "<td>" + str(shortdesc) + "</td>" if shortdesc else "<td></td>"


def index_row(model, spec, root_uri, online):
    "Return the row for a spec as an HTML string."

    # Get version
    minor = 0
    micro = 0
    try:
        minor = int(model.value(spec, lv2.minorVersion, None, any=False))
        micro = int(model.value(spec, lv2.microVersion, None, any=False))
    except rdflib.exceptions.UniquenessError:
        _warn(f"{spec} has no unique valid version")
        return ""

    # Check that date is present and valid
    if _spec_date(model, spec, minor, micro) is None:
        _warn(f"{spec} has no doap:created date")
        return ""

    row = "<tr>"

    # Specification and API
    row += _spec_link_columns(
        spec,
        root_uri,
        model.value(spec, doap.name, None).removeprefix("LV2 "),
        online,
    )

    # Description
    row += _spec_description_column(model, spec)

    # Version
    row += f"<td>{minor}.{micro}</td>"

    # Status
    deprecated = model.value(spec, owl.deprecated, None)
    deprecated = deprecated and str(deprecated) not in ["0", "false"]
    if minor == 0:
        row += '<td><span class="error">Experimental</span></td>'
    elif deprecated:
        row += '<td><span class="warning">Deprecated</span></td>'
    elif micro % 2 == 0:
        row += '<td><span class="success">Stable</span></td>'
    else:
        row += '<td><span class="warning">Development</span></td>'

    row += "</tr>"

    return row


def build_index(
    lv2_source_root,
    lv2_version,
    input_paths,
    root_uri,
    online,
):
    "Build the LV2 specification index and write it to stdout."

    model = _load_ttl(input_paths)

    # Get date for this version, and list of all LV2 distributions
    proj = rdflib.URIRef("http://lv2plug.in/ns/lv2")
    date = None
    for row in model.triples([proj, doap.release, None]):
        revision = model.value(row[2], doap.revision, None)
        created = model.value(row[2], doap.created, None)
        if str(revision) == lv2_version:
            date = created

        dist = model.value(row[2], doap["file-release"], None)
        if not dist or not created:
            _warn(f"{proj} has no file release")

    rows = []
    for spec in model.triples([None, rdf.type, lv2.Specification]):
        rows += [index_row(model, spec[0], root_uri, online)]

    if date is None:
        now = int(os.environ.get("SOURCE_DATE_EPOCH", time.time()))
        date = datetime.datetime.utcfromtimestamp(now).strftime("%F")

    _subst_file(
        os.path.join(lv2_source_root, "doc", "index.html.in"),
        sys.stdout,
        {
            "@ROWS@": "\n".join(rows),
            "@LV2_VERSION@": lv2_version,
            "@DATE@": date,
        },
    )


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        usage="%(prog)s [OPTION]... INPUT_PATH...",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    ap.add_argument("--lv2-version", help="LV2 release version")
    ap.add_argument("--lv2-source-root", help="path to LV2 source root")
    ap.add_argument(
        "--root-uri",
        default="http://lv2plug.in/ns/",
        help="root URI for specifications",
    )
    ap.add_argument(
        "--online",
        action="store_true",
        default=False,
        help="build online documentation",
    )
    ap.add_argument("input_paths", nargs="+", help="path to Turtle input file")

    args = ap.parse_args(sys.argv[1:])

    if args.lv2_version is None or args.lv2_source_root is None:
        introspect_command = ["meson", "introspect", "-a"]
        project_info = json.loads(
            subprocess.check_output(introspect_command).decode("utf-8")
        )

        if args.lv2_version is None:
            args.lv2_version = project_info["projectinfo"]["version"]

        if args.lv2_source_root is None:
            meson_build_path = project_info["buildsystem_files"][0]
            args.lv2_source_root = os.path.relpath(
                os.path.dirname(meson_build_path)
            )

    build_index(**vars(args))
