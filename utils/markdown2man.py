#!/usr/bin/env python3

###############################################################################
# Convert manual pages from markdown to MAN format
#
# Author(s): Anna Petrasova
#
# COPYRIGHT: (C) 2025 by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.
#
###############################################################################

import re
import argparse
from pathlib import Path

def strip_yaml_from_markdown(markdown):
    if markdown.startswith('---'):
        parts = markdown.split('---', 2)
        if len(parts) == 3:
            return parts[2].strip()
    return markdown

def process_tables(markdown):
    lines = markdown.strip().splitlines()
    if len(lines) < 2:
        return markdown  # Not a valid table

    headers = lines[0].strip("|").split("|")
    rows = [line.strip("|").split("|") for line in lines[2:] if '|' in line]

    output = [".TS", "allbox;", "c" * len(headers) + "."]
    output.append("\t".join([h.strip() for h in headers]))

    for row in rows:
        output.append("\t".join([cell.strip() for cell in row]))

    output.append(".TE")
    return "\n".join(output)

def parse_markdown(markdown):
    blocks = []
    lines = markdown.splitlines()
    current = {"type": "text", "markdown": ""}

    def flush():
        nonlocal current
        if current["markdown"].strip():
            blocks.append(current)
        current = {"type": "text", "markdown": ""}

    in_code = False
    for i, line in enumerate(lines): 
        if line.strip().startswith("```"):
            flush()
            in_code = not in_code
            current = {"type": "code" if in_code else "text", "markdown": ""}
            continue

        if re.match(r"^(\s*)([-*]|\d+\.)\s+.*", line):
            if current["type"] != "list":
                flush()
                current = {"type": "list", "markdown": ""}
        elif current["type"] != "code" and not line.strip():
            flush()
            continue

        if re.match(r"^\|.*\|$", line) and "|" in lines[i + 1] if i + 1 < len(lines) else False:
            flush()
            current = {"type": "table", "markdown": ""}
            current["markdown"] += line + "\n"
            continue

        current["markdown"] += line + "\n"

    flush()
    return blocks

def process_headings(markdown):
    def convert_sh(match):
        return f".SH {match.group(1).upper()}"
    def convert_ss(match):
        return f".SS {match.group(1)}"

    markdown = re.sub(r"^# (.*)", convert_sh, markdown, flags=re.MULTILINE)
    markdown = re.sub(r"^## (.*)", convert_ss, markdown, flags=re.MULTILINE)
    return markdown

def process_code(markdown):
    output = []
    output.append(".nf\n\\fC")
    for line in markdown.splitlines():
        output.append(line.replace("\\", r"\(rs"))
    output.append("\\fR\n.fi")
    return "\n".join(output)

def process_lists(markdown):
    output = []
    indent_levels = []

    for line in markdown.splitlines():
        match = re.match(r"^(\s*)([-*]|\d+\.)\s+(.*)", line)
        if not match:
            continue

        spaces, bullet, item_text = match.groups()
        level = len(spaces)

        while indent_levels and indent_levels[-1] > level:
            output.append(".RE")
            indent_levels.pop()

        if not indent_levels or indent_levels[-1] < level:
            output.append(".RS 4n")
            indent_levels.append(level)

        if re.match(r"^\d+\.$", bullet):
            output.append(f'.IP "{bullet}" 4n\n{item_text}')
        else:
            output.append(f".IP \\(bu 4n\n{item_text}")

    while indent_levels:
        output.append(".RE")
        indent_levels.pop()

    return "\n".join(output)

def process_special_characters(text):
    text = text.replace(r"\[", "[").replace(r"\]", "]")
    text = text.replace(r"\#", "#").replace(r"\>", ">").replace(r"\<", "<")
    text = text.replace("`", "")
    text = re.sub(r"(?<=\S) {2,}(?=\S)", " ", text)
    return text.replace("\\", r"\(rs")

def process_default(markdown):
    markdown = process_special_characters(markdown)
    markdown = process_headings(markdown)
    return markdown

def convert_markdown_to_man(input_file, output_file):
    markdown = Path(input_file).read_text(encoding='utf-8')
    markdown = strip_yaml_from_markdown(markdown)
    blocks = parse_markdown(markdown)

    result = ['.TH "MANPAGE" "1" "" "" ""']
    for block in blocks:
        if block["type"] == "code":
            result.append(process_code(block["markdown"]))
        elif block["type"] == "list":
            result.append(process_lists(block["markdown"]))
        elif block["type"] == "table":
            result.append(process_tables(block["markdown"]))
        else:
            result.append(process_default(block["markdown"]))

    Path(output_file).write_text("\n".join(result), encoding='utf-8')
    print(f"Successfully created: {output_file}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Convert Markdown file to man page")
    parser.add_argument('input_file', type=str, help="Path to the input Markdown file")
    parser.add_argument('output_file', type=str, help="Path to the output man page file")
    
    args = parser.parse_args()
    
    convert_markdown_to_man(args.input_file, args.output_file)
