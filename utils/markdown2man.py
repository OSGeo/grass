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

    markdown = before + after
    markdown = markdown.replace("## Parameters", "### Parameters")
    return yaml, markdown


def parse_markdown(content):
    lines = content.splitlines()
    processing_block = []
    processed_content = []

    buffer = ""
    state = "default"
    code_indent = 0

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
                code_indent = len(re.match(r" *", line).group())
                if buffer:
                    processing_block.append(buffer)
                    buffer = ""
                processed_content.append(
                    {"markdown": "\n".join(processing_block), "type": state}
                )
                processing_block = [line]
                state = "code"
            continue

        if state == "code":
            processing_block.append(
                line[code_indent:] if line.startswith(" " * code_indent) else line
            )
            continue

        if line.strip().startswith("- ") or line.startswith("* "):
            if buffer:
                processing_block.append(buffer)
                buffer = ""

            # start of list
            if state != "list":
                processed_content.append(
                    {"markdown": "\n".join(processing_block), "type": state}
                )
                processing_block = []
                state = "list"

            processing_block.append(line)
            continue

        if re.match(r"^\d+\.", line.strip()):
            if buffer:
                processing_block.append(buffer)
                buffer = ""
            # start of ordered list
            if state != "olist":
                processed_content.append(
                    {"markdown": "\n".join(processing_block), "type": state}
                )
                processing_block = []
                state = "olist"

            processing_block.append(line)
            continue

        if line.strip().startswith("|") and line.strip().endswith("|"):
            if buffer:
                processing_block.append(buffer)
                buffer = ""
            processing_block.append(line)
            continue

        if line.endswith("  "):
            buffer += line  # Keep trailing spaces for markdown line breaks
            processing_block.append(buffer)
            buffer = ""
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

        buffer += line.lstrip() + " "

    if buffer:
        processing_block.append(buffer)
    if processing_block:
        processed_content.append(
            {"markdown": "\n".join(processing_block), "type": state}
        )
    for each in processed_content:
        print(each["type"], "\n")
        print(each["markdown"])
    return processed_content


def process_multiline_links(markdown):
    # Regular expression to match links that may span multiple lines
    link_pattern = re.compile(r"\[([^\]]*)\]\(([^)]+)\)", re.DOTALL)
    image_link_pattern = re.compile(r"!\[([^\]]*)\]\(([^)]+)\)", re.DOTALL)

    def replace_link(match):
        # Strip leading and trailing whitespace or newlines from the link URL and text
        alt_text = match.group(1).replace("\n", " ").strip()
        url = match.group(2).replace("\n", "").strip()
        return f"[{alt_text}]({url})"

    def replace_image_link(match):
        # Strip leading and trailing whitespace or newlines from the link URL and text
        alt_text = match.group(1).replace("\n", " ").strip()
        url = match.group(2).replace("\n", "").strip()
        return f"![{alt_text}]({url})"

    # Replace all matched links with the single-line version
    markdown = re.sub(link_pattern, replace_link, markdown)
    return re.sub(image_link_pattern, replace_image_link, markdown)


def process_markdown_formatting(md_text):
    # Regular expression to find multi-line formatting for bold, emphasis, and combined bold and emphasis
    pattern = r"(\*\*\*([^\*]+)\*\*\*|\*\*([^\*]+)\*\*|\*([^\*]+)\*)"

    def replace_match(match):
        # Match for combined bold and emphasis (***text***)
        if match.group(1).startswith("***"):
            content = match.group(2).replace("\n", " ").strip()
            return f"***{content}***"

        # Match for bold (**text**)
        if match.group(1).startswith("**"):
            content = match.group(3).replace("\n", " ").strip()
            return f"**{content}**"

        # Match for emphasis (*text*)
        if match.group(1).startswith("*"):
            content = match.group(4).replace("\n", " ").strip()
            return f"*{content}*"

        return match.group(0)  # Return the original text if no match

    # Apply the regex pattern to replace formatting spans
    return re.sub(pattern, replace_match, md_text)


def process_links(line):
    """Replace Markdown links with only their display text."""
    line = re.sub(r"!\[.*?\]\(.*?\)", "", line)
    return re.sub(r"\[(.*?)\]\((.*?)\)", r"\1", line)


def process_parameters(markdown):
    parts = markdown.split("## DESCRIPTION", 1)
    if len(parts) == 1:
        return markdown
    before, after = parts
    before_processed = re.sub(
        r"^\*\*([a-z0-9_]*)\*\*=\*([a-z,]*)\*( \*\*\[required\]\*\*)?",
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


def process_formatting(line):
    """Apply inline formatting for bold, italic, and bold+italic."""
    line = re.sub(r"\*\*\*(.+?)\*\*\*", r"\\fB\\fI\1\\fR", line)  # Bold+Italic
    line = re.sub(r"\*\*(.+?)\*\*", r"\\fB\1\\fR", line)  # Bold
    line = re.sub(r"\*(.+?)\*", r"\\fI\1\\fR", line)  # Italic
    return line.replace("\u00a0", "    ")  # Replace non-breaking spaces with indent


def process_br(line):
    return re.sub(r"([^\n\s])  $", r"\1\n.br", line)


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
    # Match list items and their continuation lines
    pattern = re.compile(
        r"""
        ^[ \t]*[-*+]                # Bullet list marker at start of line
        [ \t]+                      # Space after bullet
        .+                          # First line of text
        (?:\n(?![ \t]*[-*+] )       # Continuation lines NOT starting with another bullet
            [^\n]+)*                # Non-blank continuation lines
    """,
        re.MULTILINE | re.VERBOSE,
    )

    matches = pattern.findall(markdown)
    if not matches:
        return markdown  # nothing to convert

    # Build roff output
    lines = [".RS 4n"]
    for item in matches:
        # Clean bullet
        item_text = re.sub(r"^[ \t]*[-*][ \t]+", "", item, count=1)
        lines.extend((".IP \\(bu 4n", item_text.strip()))
    lines.append(".RE")
    return "\n".join(lines)


def process_olist(markdown):
    # Match list items and their continuation lines
    pattern = re.compile(
        r"""
        ^[ \t]*                   # Optional leading whitespace
        (\d+)[.)]                 # Ordered list number (capture this)
        [ \t]+                    # At least one space
        .+                      # First line of list item (non-greedy)
          (?:\n                     # Followed by continuation lines
            (?![ \t]*\d+[.)][ \t])  # Not starting with another list number
            [^\n]+                  # Continuation line content
          )*              # More content lines
        """,
        re.MULTILINE | re.VERBOSE,
    )

    matches = list(pattern.finditer(markdown))
    if not matches:
        return markdown  # nothing to convert

    lines = [".RS 4n"]
    for match in matches:
        number = match.group(1)
        full_item = match.group(0)
        # Remove the number and punctuation from the beginning
        item_text = re.sub(r"^[ \t]*\d+[.)][ \t]+", "", full_item, count=1)
        lines.append(f'.IP "{number}." 4n\n{item_text.strip()}')
    lines.append(".RE")
    return "\n".join(lines)


def process_non_code(markdown):
    markdown = process_table(markdown)
    markdown = markdown.replace(r"`", "")
    markdown = re.sub(r"\\#", "#", markdown)
    markdown = re.sub(r"\\\[", "[", markdown)
    markdown = re.sub(r"\\\]", "]", markdown)
    markdown = re.sub(r"\\>", ">", markdown)
    markdown = re.sub(r"\\<", "<", markdown)
    markdown = re.sub(r"\\", r"\(rs", markdown)
    markdown = re.sub(r"(?<=\S) {2,}(?=\S)", " ", markdown)
    markdown = re.sub(r"(?<=\S) {2,}(?=\S)", " ", markdown)
    markdown = process_formatting(markdown)
    markdown = process_links(markdown)
    markdown = process_headings(markdown)
    return re.sub(r"([^\n\s])  $", r"\1\n.br", markdown, flags=re.MULTILINE)


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
            text = process_non_code(block["markdown"])
            result.append(process_list(text))
        elif block["type"] == "olist":
            text = process_non_code(block["markdown"])
            result.append(process_olist(text))
        else:
            result.append(process_non_code(block["markdown"]))
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
