#!/usr/bin/env python3

# Copyright 2012-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: ISC

"""
A simple literate programming tool for C, C++, and Turtle.

Unlike many LP tools, this tool uses normal source code as input, there is no
tangle/weave and no special file format.  The literate parts of the program are
written in comments, which are emitted as paragraphs of regular text
interleaved with code.  Asciidoc is both the comment and output syntax.
"""

import os
import re
import sys


def format_text(text):
    "Format a text (comment) fragment and return it as a marked up string."
    return "\n\n" + re.sub("\n *", "\n", text.strip()) + "\n\n"


def format_code(lang, code):
    "Format a block of code and return it as a marked up string."

    if code.strip() == "":
        return code

    head = f"[source,{lang}]"
    code = code.strip("\n")
    sep = "-" * len(head)
    return "\n".join([head, sep, code, sep]) + "\n"


def format_c_source(filename, in_file):
    "Format an annotated C source file as a marked up string."

    output = f"=== {os.path.basename(filename)} ===\n"
    chunk = ""
    prev_c = 0
    in_comment = False
    in_comment_start = False
    n_stars = 0
    code = "".join(in_file)

    # Skip initial license comment
    if code[0:2] == "/*":
        end = code.find("*/") + 2
        code = code[end:]

    def last_chunk(chunk):
        length = len(chunk) - 1
        return chunk[0:length]

    for char in code:
        if prev_c == "/" and char == "*":
            in_comment_start = True
            n_stars = 1
        elif in_comment_start:
            if char == "*":
                n_stars += 1
            else:
                if n_stars > 1:
                    output += format_code("c", last_chunk(chunk))
                    chunk = ""
                    in_comment = True
                else:
                    chunk += "*" + char
                in_comment_start = False
        elif in_comment and prev_c == "*" and char == "/":
            if n_stars > 1:
                output += format_text(last_chunk(chunk))
            else:
                output += format_code("c", "/* " + last_chunk(chunk) + "*/")
            in_comment = False
            in_comment_start = False
            chunk = ""
        else:
            chunk += char

        prev_c = char

    return output + format_code("c", chunk)


def format_ttl_source(filename, in_file):
    "Format an annotated Turtle source file as a marked up string."

    output = f"=== {os.path.basename(filename)} ===\n"

    in_comment = False
    chunk = ""
    for line in in_file:
        is_comment = line.strip().startswith("#")
        if in_comment:
            if is_comment:
                chunk += line.strip().lstrip("# ") + " \n"
            else:
                output += format_text(chunk)
                in_comment = False
                chunk = line
        else:
            if is_comment:
                output += format_code("turtle", chunk)
                in_comment = True
                chunk = line.strip().lstrip("# ") + " \n"
            else:
                chunk += line

    if in_comment:
        return output + format_text(chunk)

    return output + format_code("turtle", chunk)


def gen(out, filenames):
    "Write markup generated from filenames to an output file."

    for filename in filenames:
        with open(filename, "r", encoding="utf-8") as in_file:
            if filename.endswith(".c") or filename.endswith(".h"):
                out.write(format_c_source(filename, in_file))
            elif filename.endswith(".ttl") or filename.endswith(".ttl.in"):
                out.write(format_ttl_source(filename, in_file))
            elif filename.endswith(".txt"):
                for line in in_file:
                    out.write(line)
                out.write("\n")
            else:
                sys.stderr.write(
                    f"Unknown source format `{filename.splitext()[1]}`\n"
                )


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.stderr.write(f"Usage: {sys.argv[0]} OUT_FILE IN_FILE...\n")
        sys.exit(1)

    with open(sys.argv[1], "w", encoding="utf-8") as out_file:
        gen(out_file, sys.argv[2:])
