#!/usr/bin/env python3
import argparse
import re
from pathlib import Path


def strip_yaml_from_markdown(content):
    # Remove YAML front matter
    return re.sub(r"^---\n.*?\n---\n", "", content, flags=re.DOTALL)


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
        else:
            buffer += line

        if line.endswith("  "):
            processing_block.append(buffer)
            buffer = ""

    if buffer:
        processing_block.append(buffer)
    if processing_block:
        processed_content.append(
            {"markdown": "\n".join(processing_block), "type": state}
        )

    merged_content = []
    for item in processed_content:
        if not item["markdown"]:
            continue
        if merged_content and merged_content[-1]["type"] == item["type"]:
            merged_content[-1]["markdown"] += "\n" + item["markdown"]
        else:
            merged_content.append(item)

    return merged_content


def process_links(markdown):
    """Replace Markdown links with only their display text."""
    markdown = re.sub(r"!\[.*?\]\(.*?\)", "", markdown)
    return re.sub(r"\[(.*?)\]\((.*?)\)", r"\1", markdown)


def process_parameters(markdown):
    return re.sub(
        r"^\*\*([a-z0-9_]*)\*\*=\*([a-z]*)\*( \*\*\[required\]\*\*)?",
        r'.IP "**\1**=*\2*\3" 4m',
        markdown,
        flags=re.MULTILINE,
    )


def process_flags(markdown):
    return re.sub(r"^\*\*-(.*?)\*\*", r'.IP "**-\1**" 4m', markdown, flags=re.MULTILINE)


def process_formatting(markdown):
    """Apply inline formatting for bold, italic, and bold+italic."""
    markdown = re.sub(r"\*\*\*(.+?)\*\*\*", r"\\fB\\fI\1\\fR", markdown)
    markdown = re.sub(r"\*\*(.+?)\*\*", r"\\fB\1\\fR", markdown)
    return re.sub(r"\*(.+?)\*", r"\\fI\1\\fR", markdown)


def process_br(markdown):
    return re.sub(r"([^\n\s])  $", r"\1\n.br", markdown, flags=re.MULTILINE)


def process_headings(markdown):
    def convert_sh(match):
        return f".SH {match.group(1).upper()}"

    def convert_ss(match):
        return f".SS {match.group(1)}"

    markdown = re.sub(r"^#{1,2} (.*)", convert_sh, markdown, flags=re.MULTILINE)
    return re.sub(r"^#{3,} (.*)", convert_ss, markdown, flags=re.MULTILINE)


def process_code(markdown):
    in_code_block = False
    output = []
    for line in markdown.splitlines():
        if line.lstrip().startswith("```"):
            if in_code_block:
                output.append("\\fR\n.fi\n")  # End code block
            else:
                output.append(".nf\n\\fC\n")  # Start code block
            in_code_block = not in_code_block
        else:
            output.append(re.sub(r"\\", r"\(rs", line))

    return "\n".join(output)


def process_lists(markdown):
    markdown = process_special_characters(markdown)
    markdown = process_formatting(markdown)
    markdown = process_links(markdown)

    output = []
    indent_levels = []

    for line in markdown.splitlines():
        match = re.match(r"^(\s*)([-*]|\d+\.)\s+(.*)", line)  # Match bullets or numbers
        if not match:
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
    markdown = markdown.replace("`", "")
    # eliminate extra spaces between words
    markdown = re.sub(r"(?<=\S) {2,}(?=\S)", " ", markdown)
    return re.sub(r"\\", r"\(rs", markdown)


def process_default(markdown):
    markdown = process_br(markdown)
    markdown = process_parameters(markdown)
    markdown = process_flags(markdown)
    markdown = markdown.replace("&nbsp;&nbsp;&nbsp;&nbsp;", "")
    markdown = process_special_characters(markdown)
    markdown = process_formatting(markdown)
    markdown = process_links(markdown)
    return process_headings(markdown)


def convert_markdown_to_man(input_file, output_file):
    """Read Markdown file and convert to man page."""
    markdown = Path(input_file).read_text()
    markdown = strip_yaml_from_markdown(markdown)
    blocks = parse_markdown(markdown)
    result = ['.TH MAN 1 "Manual"\n']
    for block in blocks:
        if block["type"] == "code":
            result.append(process_code(block["markdown"]))
        elif block["type"] == "list":
            result.append(process_lists(block["markdown"]))
        else:
            result.append(process_default(block["markdown"]))

    Path(output_file).write_text("\n".join(result))


def main():
    parser = argparse.ArgumentParser(description="Convert Markdown to Unix man page.")
    parser.add_argument("input_file", help="Path to the input Markdown file.")
    parser.add_argument("output_file", help="Path to the output man page file.")
    args = parser.parse_args()

    convert_markdown_to_man(args.input_file, args.output_file)


if __name__ == "__main__":
    main()
