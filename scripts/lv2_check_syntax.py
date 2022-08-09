#!/usr/bin/env python3

# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

"""
Check that a Turtle file has valid syntax and strict formatting.

This is a strict tool that enforces machine formatting with serdi.
"""

import argparse
import difflib
import filecmp
import sys
import tempfile
import os
import subprocess


def _show_diff(from_lines, to_lines, from_path, to_path):
    "Show a diff between two files, returning non-zero if they differ."

    differences = False
    for line in difflib.unified_diff(
        from_lines,
        to_lines,
        fromfile=from_path,
        tofile=to_path,
    ):
        sys.stderr.write(line)
        differences = True

    return int(differences)


def _check_file_equals(patha, pathb):
    "Check that two files are equal, returning non-zero if they differ."

    for path in (patha, pathb):
        if not os.access(path, os.F_OK):
            sys.stderr.write(f"error: missing file {path}")
            return 1

    if filecmp.cmp(patha, pathb, shallow=False):
        return 0

    with open(patha, "r", encoding="utf-8") as in_a:
        with open(pathb, "r", encoding="utf-8") as in_b:
            return _show_diff(in_a.readlines(), in_b.readlines(), patha, pathb)


def run(serdi, filenames):
    "Check that every file in filenames has valid formatted syntax."

    status = 0

    for filename in filenames:
        rel_path = os.path.relpath(filename)
        with tempfile.NamedTemporaryFile(mode="w", delete=False) as out:
            out_name = out.name
            command = [serdi, "-o", "turtle", rel_path]
            subprocess.check_call(command, stdout=out)

        if _check_file_equals(rel_path, out_name):
            status = 1

        os.remove(out_name)

    return status


if __name__ == "__main__":
    ap = argparse.ArgumentParser(
        usage="%(prog)s [OPTION]... TURTLE_FILE...",
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    ap.add_argument("--serdi", default="serdi", help="path to serdi")
    ap.add_argument("TURTLE_FILE", nargs="+", help="input file to check")

    args = ap.parse_args(sys.argv[1:])

    sys.exit(run(args.serdi, args.TURTLE_FILE))
