#!/usr/bin/env python3

# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

"""
Write an HTML index for a set of LV2 specifications.
"""

import json
import os
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
            try:
                model.parse(path, format="n3")
            except SyntaxError as error:
                sys.stderr.write(f"error: Failed to parse {path}\n")
                raise error

    return model


def _warn(message):
    "Load a warning message."

    assert not message.startswith("warning: ")
    assert not message.endswith("\n")
    sys.stderr.write(message)
    sys.stderr.write("\n")


def _spec_target(spec, root, online=False):
    "Return the relative link target for a specification."

    target = spec.replace(root, "") if spec.startswith(root) else spec

    return target if online else target + ".html"


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
    col += f' href="../c/html/group__{stem}.html">{name}'
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

    row = "<tr>"

    # Specification and API
    row += _spec_link_columns(
        spec,
        root_uri,
        model.value(spec, doap.name, None).replace("LV2 ", ""),
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

    rows = []
    for spec in model.triples([None, rdf.type, lv2.Specification]):
        rows += [index_row(model, spec[0], root_uri, online)]

    _subst_file(
        os.path.join(lv2_source_root, "doc", "index.html.in"),
        sys.stdout,
        {
            "@ROWS@": "\n".join(sorted(rows)),
            "@LV2_VERSION@": lv2_version,
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
