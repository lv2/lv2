#!/usr/bin/env python3

# Copyright 2020-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

"""
Write a NEWS file from RDF data.

The output is in Debian changelog format, which can be parsed by
dpkg-parsechangelog, among other things.
"""

import argparse
import os
import sys
import datetime
import textwrap
import urllib
import re

import rdflib

doap = rdflib.Namespace("http://usefulinc.com/ns/doap#")
dcs = rdflib.Namespace("http://ontologi.es/doap-changeset#")
rdfs = rdflib.Namespace("http://www.w3.org/2000/01/rdf-schema#")
foaf = rdflib.Namespace("http://xmlns.com/foaf/0.1/")
rdf = rdflib.Namespace("http://www.w3.org/1999/02/22-rdf-syntax-ns#")


def _is_release_version(version):
    "Return true if `version` is a stable version number."

    if len(version) not in [2, 3] or version[0] == 0:
        return False

    minor = version[len(version) - 2]
    micro = version[len(version) - 1]

    return micro % 2 == 0 and (len(version) == 2 or minor % 2 == 0)


def _parse_datetime(string):
    "Parse string as either a datetime or a date."

    try:
        return datetime.datetime.strptime(string, "%Y-%m-%dT%H:%M:%S%z")
    except ValueError:
        return datetime.datetime.strptime(string, "%Y-%m-%d")


def _release_entry(graph, release):
    "Return a news entry for a release."

    revision = graph.value(release, doap.revision, None)
    date = graph.value(release, doap.created, None)
    blamee = graph.value(release, dcs.blame, None)
    changeset = graph.value(release, dcs.changeset, None)
    dist = graph.value(release, doap["file-release"], None)

    if not revision or not date or not blamee or not changeset:
        return None

    version = tuple(map(int, revision.split(".")))

    entry = {
        "version": version,
        "revision": str(revision),
        "date": _parse_datetime(date),
        "status": "stable" if _is_release_version(version) else "unstable",
        "items": [],
    }

    if dist is not None:
        entry["dist"] = dist

    for j in graph.triples([changeset, dcs.item, None]):
        item = str(graph.value(j[2], rdfs.label, None))
        entry["items"] += [item]

    entry["blamee_name"] = str(graph.value(blamee, foaf.name, None))
    entry["blamee_mbox"] = str(graph.value(blamee, foaf.mbox, None))
    return entry


def _project_entries(graph, project):
    "Return a map from version to news entries for a project"

    entries = {}
    for link in graph.triples([project, doap.release, None]):
        entry = _release_entry(graph, link[2])
        if entry is not None:
            entries[entry["version"]] = entry
        else:
            sys.stderr.write(f"warning: Ignored partial {project} release\n")

    return entries


def _read_turtle_news(in_files):
    "Read news entries from Turtle."

    graph = rdflib.Graph()

    # Parse input files
    for i in in_files:
        graph.parse(i)

    # Read news for every project in the data
    projects = {t[0] for t in graph.triples([None, rdf.type, doap.Project])}
    entries_by_project = {}
    for project in projects:
        # Load any associated files
        for uri in graph.triples([project, rdfs.seeAlso, None]):
            if uri[2].endswith(".ttl"):
                graph.parse(uri[2])

        # Use the symbol from the URI as a name, or failing that, the doap:name
        name = os.path.basename(urllib.parse.urlparse(str(project)).path)
        if not name:
            name = graph.value(project, doap.name, None)

        entries = _project_entries(graph, project)
        for _, entry in entries.items():
            entry["name"] = name

        entries_by_project[str(project)] = entries

    return entries_by_project


def _write_news_item(out, item):
    "Write a single item (change) in NEWS format."

    out.write("\n  * " + "\n    ".join(textwrap.wrap(item, width=74)))


def _write_news_entry(out, entry):
    "Write an entry (version) to out in NEWS format."

    # Summary header
    summary = f'{entry["name"]} ({entry["revision"]}) {entry["status"]}'
    out.write(f"{summary}; urgency=medium\n")

    # Individual change items
    for item in sorted(entry["items"]):
        _write_news_item(out, item)

    # Trailer line
    mbox = entry["blamee_mbox"].replace("mailto:", "")
    author = f'{entry["blamee_name"]} <{mbox}>'
    date = entry["date"]
    if date.tzinfo is None:  # Assume UTC (dpkg-parsechangelog requires it)
        date = date.strftime("%a, %d %b %Y %H:%M:%S +0000")
    else:
        date = date.strftime("%a, %d %b %Y %H:%M:%S %z")

    out.write(f"\n\n -- {author}  {date}\n")


def _write_single_project_news(out, entries):
    "Write a NEWS file for entries of a single project to out."

    revisions = sorted(entries.keys(), reverse=True)
    for revision in revisions:
        entry = entries[revision]
        out.write("\n" if revision != revisions[0] else "")
        _write_news_entry(out, entry)


def _write_meta_project_news(out, top_project, entries_by_project):
    "Write a NEWS file for a meta-project that contains others."

    top_name = os.path.basename(urllib.parse.urlparse(str(top_project)).path)
    release_pattern = rf".*/{top_name}-([0-9\.]*).tar.bz2"

    # Pop the entries for the top project
    top_entries = entries_by_project.pop(top_project)

    # Add items from the other projects to the corresponding top entry
    for _, entries in entries_by_project.items():
        for version, entry in entries.items():
            if "dist" in entry:
                match = re.match(release_pattern, entry["dist"])
                if match:
                    version = tuple(map(int, match.group(1).split(".")))
                    for item in entry["items"]:
                        top_entries[version]["items"] += [
                            f'{entry["name"]}: {item}'
                        ]

    for version in sorted(top_entries.keys(), reverse=True):
        out.write("\n" if version != max(top_entries.keys()) else "")
        _write_news_entry(out, top_entries[version])


def _write_text_news(out, entries_by_project, top_project=None):
    "Write NEWS in standard Debian changelog format."

    if len(entries_by_project) > 1:
        if top_project is None:
            sys.stderr.write("error: --top is required for multi-projects\n")
            return 1

        _write_meta_project_news(out, top_project, entries_by_project)
    else:
        project = next(iter(entries_by_project))
        _write_single_project_news(out, entries_by_project[project])

    return 0


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        usage="%(prog)s [OPTION]... DATA_FILE...",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    ap.add_argument(
        "-o",
        "--output",
        metavar="OUTPUT_FILE",
        help="output file path",
    )

    ap.add_argument(
        "-t",
        "--top-project",
        metavar="OUTPUT_FILE",
        help="URI of parent meta-project with file releases",
    )

    ap.add_argument(
        "DATA_FILE",
        nargs="+",
        help="path to a Turtle file with release data",
    )

    args = ap.parse_args(sys.argv[1:])

    if not args.output and "MESON_DIST_ROOT" in os.environ:
        args.output = os.path.join(os.getenv("MESON_DIST_ROOT"), "NEWS")

    if not args.output:
        sys.exit(
            _write_text_news(
                sys.stdout, _read_turtle_news(args.DATA_FILE), args.top_project
            )
        )
    else:
        with open(args.output, "w", encoding="utf-8") as output_file:
            sys.exit(
                _write_text_news(
                    output_file,
                    _read_turtle_news(args.DATA_FILE),
                    args.top_project,
                )
            )
