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

import argparse
import os
from pathlib import Path
import re
import textwrap


def strip_yaml_from_markdown_and_reformat(content):
    # Remove YAML front matter
    match = re.match(r"^---\n(.*?)\n---\n", content, re.DOTALL)
    if not match:
        return {}, content.strip()

    yaml_block = match.group(1)
    markdown = content[match.end() :].strip()

    yaml = {}
    for line in yaml_block.splitlines():
        key, value = line.strip().split(":")
        key = key.strip()
        value = value.strip()
        if value.startswith("[") and value.endswith("]"):
            yaml[key] = [v.strip() for v in value[1:-1].split(",")]
        else:
            yaml[key] = value

    split_string = '=== "Command line"'
    before, after = markdown.split(split_string, 1)

    before = f"""

# NAME

{yaml["name"]} - {yaml["description"]}

# KEYWORDS

{", ".join(yaml["keywords"])}

# SYNOPSIS

    """

    markdown = before + after.strip()
    markdown = markdown.replace("## Parameters", "### Parameters")
    return yaml, markdown


def parse_markdown(content):
    lines = content.splitlines()
    processing_block = []
    processed_content = []

    buffer = ""
    state = "default"

    for line in lines:
        if line.strip().startswith("```"):
            # end of code block
            if state == "code":
                processing_block.append(line)
                processed_content.append(
                    {"markdown": "\n".join(processing_block), "type": state}
                )
                processing_block = []
                state = "default"
            # start of code block
            else:
                processed_content.append(
                    {"markdown": "\n".join(processing_block), "type": state}
                )
                processing_block = []
                processing_block.append(line)
                state = "code"
            continue

        if state == "code":
            processing_block.append(line)
            continue

        if re.match(r"^(\s*)([-*]|\d+\.)\s+(.*)", line.strip()):
            if buffer:
                processing_block.append(buffer)
                buffer = ""
            # start of ordered list
            if state != "list":
                processed_content.append(
                    {"markdown": "\n".join(processing_block), "type": state}
                )
                processing_block = []
                state = "list"

        if line.strip().startswith("|") and line.strip().endswith("|"):
            if buffer:
                processing_block.append(buffer)
                buffer = ""
            processing_block.append(line)
            continue

        # empty line at the start and end of code, list blocks
        if line == "":
            if buffer:
                processing_block.append(buffer)
                buffer = ""
            if state != "default":
                processed_content.append(
                    {"markdown": "\n".join(processing_block), "type": state}
                )
                processing_block = []
                state = "default"
            processing_block.append(line)
            continue

        if buffer:
            buffer += " " + line
        elif state == "list":
            buffer += line
        else:
            buffer += line.lstrip()

        if line.endswith("  "):
            processing_block.append(buffer)
            buffer = ""

    if buffer:
        processing_block.append(buffer)
    if processing_block:
        processed_content.append(
            {"markdown": "\n".join(processing_block), "type": state}
        )

    return processed_content


def process_links(markdown):
    """Replace Markdown links with only their display text."""
    markdown = re.sub(r"!\[.*?\]\(.*?\)", "", markdown)
    return re.sub(r"\[(.*?)\]\((.*?)\)", r"\1", markdown)


def process_parameters(markdown):
    parts = markdown.split("## DESCRIPTION", 1)
    if len(parts) == 1:
        return markdown
    before, after = parts
    before_processed = re.sub(
        r"^\*\*([a-z0-9_]*)\*\*=\*([a-zA-Z,_ ]*)\*( \*\*\[required\]\*\*)?",
        r'.IP "**\1**=*\2*\3" 4m',
        before,
        flags=re.MULTILINE,
    )
    return before_processed + "## DESCRIPTION" + after


def process_flags(markdown):
    parts = markdown.split("## DESCRIPTION", 1)
    if len(parts) == 1:
        return markdown

    before, after = parts
    before_processed = re.sub(
        r"^\*\*-(.*?)\*\*", r'.IP "**-\1**" 4m', before, flags=re.MULTILINE
    )
    return before_processed + "## DESCRIPTION" + after


def process_formatting(markdown):
    """Apply inline formatting for bold, italic, and bold+italic."""
    markdown = re.sub(r"\*\*\*(.+?)\*\*\*", r"\\fB\\fI\1\\fR", markdown)
    markdown = re.sub(r"\*\*(.+?)\*\*", r"\\fB\1\\fR", markdown)
    # avoid detecting \*
    return re.sub(r"(?<!\\)\*(.+?)(?<!\\)\*", r"\\fI\1\\fR", markdown)


def process_headings(markdown):
    def convert_sh(match):
        return f".SH {match.group(1).upper()}\n"

    def convert_ss(match):
        return f".SS {match.group(1)}\n"

    markdown = re.sub(r"^#{1,2} (.*)", convert_sh, markdown, flags=re.MULTILINE)
    return re.sub(r"^#{3,} (.*)", convert_ss, markdown, flags=re.MULTILINE)


def remove_python_content_blocks(markdown):
    pattern = re.compile(
        r"""
        ^===\s*"Python[^\n]*"\n   # Match the Python block title
        (?:
            (?![!-~])             # As long as the line does NOT start with ascii
            .*\n                  # Match that whole line
        )*
        """,
        re.MULTILINE | re.VERBOSE,
    )
    return pattern.sub("", markdown)


def unindent_command_content_blocks(markdown):
    pattern = re.compile(
        r"""
        ^===\s*"Command[^\n]*\n   # Match the Python block title
        (                         # Capture group for the content block
        (?:\n?(?![!-~]).*)*       # Match all lines NOT starting with visible ASCII (indented)
        )
        """,
        re.MULTILINE | re.VERBOSE,
    )

    def unindent_block(match):
        return textwrap.dedent(match.group(1))

    markdown = pattern.sub(unindent_block, markdown)
    return re.sub(r'^=== "Command line"\n', "", markdown, flags=re.MULTILINE)


def process_code(markdown):
    markdown = re.sub(r"\\", r"\(rs", markdown)

    pattern = re.compile(r"(?m)^( *)```(?:\w+)?\n(.*?)(?<=\n)\1```", re.DOTALL)

    def repl(match):
        code = match.group(2)
        dedented = textwrap.dedent(code).rstrip()
        return f".PP\n.nf\n\\fC\n{dedented}\n\\fR\n.fi"

    return pattern.sub(repl, markdown)


def remove_comments(markdown):
    return re.sub(r"<!--.*?-->", "", markdown, flags=re.DOTALL)


def process_list(markdown):
    markdown = process_formatting(markdown)
    markdown = process_special_characters(markdown)
    markdown = process_links(markdown)
    markdown = process_br(markdown)

    output = []
    indent_levels = []

    for line in markdown.splitlines():
        match = re.match(r"^(\s*)([-*]|\d+\.)\s+(.*)", line)  # Match bullets or numbers
        if not match:
            output.append(line)
            continue  # Skip non-list lines (shouldn't happen if input is all lists)

        spaces, bullet, item_text = match.groups()
        level = len(spaces)  # Determine indentation level

        while indent_levels and indent_levels[-1] > level:
            output.append(".RE")  # Close previous indentation level
            indent_levels.pop()

        if not indent_levels or indent_levels[-1] < level:
            output.append(".RS 4n")  # Open new indentation level
            indent_levels.append(level)

        if re.match(r"^\d+\.$", bullet):  # Numbered list
            output.append(f'.IP "{bullet}" 4n\n{item_text}')
        else:  # Bullet list
            output.append(".IP \\(bu 4n\n" + item_text)

    # Close any remaining indentation levels
    while indent_levels:
        output.append(".RE")
        indent_levels.pop()

    return "\n".join(output)


def process_special_characters(markdown):
    markdown = markdown.replace(r"\[", "[")
    markdown = markdown.replace(r"\]", "]")
    markdown = markdown.replace(r"\#", "#")
    markdown = markdown.replace(r"\>", ">")
    markdown = markdown.replace(r"\<", "<")
    markdown = markdown.replace(r"\*", "*")
    markdown = markdown.replace("`", "")
    # eliminate extra spaces between words
    return re.sub(r"(?<=\S) {2,}(?=\S)", " ", markdown)


def process_br(markdown):
    return re.sub(r"([^\n\s])  $", r"\1\n.br", markdown, flags=re.MULTILINE)


def process_default(markdown):
    markdown = process_table(markdown)
    markdown = process_formatting(markdown)
    markdown = process_special_characters(markdown)
    markdown = process_links(markdown)
    markdown = process_headings(markdown)
    return process_br(markdown)


def process_table(markdown: str) -> str:
    def markdown_to_roff_table(md_table: str) -> str:
        lines = md_table.strip().splitlines()
        if len(lines) < 2:
            return md_table  # not a valid table

        # Remove divider line (2nd line)
        header = lines[0].strip("|").split("|")
        rows = [line.strip("|").split("|") for line in lines[2:]]

        # Trim spaces in cells
        header = [cell.strip() for cell in header]
        header = [f"**{cell.strip()}**" for cell in header]
        rows = [[cell.strip() for cell in row] for row in rows]

        # Generate column format line (left aligned)
        format_line = " ".join(["l"] * len(header)) + "."
        # Build the roff table
        lines = [".TS", "tab(|);", format_line, "|".join(header)]
        for row in rows:
            lines.append("|".join(row))
        lines.extend((".TE", ".PP"))

        return "\n".join(lines)

    markdown_table_pattern = re.compile(
        r"""
    (                                   # full table match
        ^\|.*\|\s*\n                    # header row: starts and ends with |
        ^\|[:\-| ]+\|\s*\n              # divider row: like | --- | :--: |
        (?:^\|.*\|\s*\n?)+              # one or more data rows
    )
    """,
        re.MULTILINE | re.VERBOSE,
    )
    return markdown_table_pattern.sub(
        lambda match: markdown_to_roff_table(match.group(0)), markdown
    )


def add_paragraphs(markdown):
    return re.sub(
        r"(?m)(?:^|\n)([^\n\S]*[^\n]+(?:\n[^\n\S]*[^\n]+)*)",
        lambda m: f"\n.PP\n{m.group(1)}",
        markdown,
    ).strip()


def markdown_to_man(markdown_text):
    """Convert a Markdown text to a Unix man page format"""
    yaml, markdown_text = strip_yaml_from_markdown_and_reformat(markdown_text)
    markdown_text = remove_python_content_blocks(markdown_text)
    markdown_text = unindent_command_content_blocks(markdown_text)
    markdown_text = remove_comments(markdown_text)
    # process synopsis
    markdown_text = process_parameters(markdown_text)
    markdown_text = process_flags(markdown_text)
    markdown_text = markdown_text.replace("&nbsp;&nbsp;&nbsp;&nbsp;", "")

    blocks = parse_markdown(markdown_text)
    result = []
    for block in blocks:
        if block["type"] == "code":
            result.append(process_code(block["markdown"]))
        elif block["type"] == "list":
            result.append(process_list(block["markdown"]))
        else:
            result.append(process_default(block["markdown"]))
    markdown_text = "\n".join(result)
    markdown_text = add_paragraphs(markdown_text)

    version = os.environ.get("VERSION_NUMBER", "")
    man_page = (
        f'.TH {yaml.get("name", "MAN")} 1 "" "GRASS {version}" "GRASS User\'s Manual"\n'
    )
    man_page += markdown_text

    return man_page


def main():
    parser = argparse.ArgumentParser(description="Convert Markdown to Unix man page.")
    parser.add_argument("input_file", help="Path to the input Markdown file.")
    parser.add_argument("output_file", help="Path to the output man page file.")
    args = parser.parse_args()

    markdown_text = Path(args.input_file).read_text()
    man_text = markdown_to_man(markdown_text)
    Path(args.output_file).write_text(man_text)


if __name__ == "__main__":
    main()
