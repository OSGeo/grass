# SPDX-FileCopyrightText: 2026 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
"""Parse the Markdown subset used by GRASS documentation into a tag tree.

The tree consists of (tag, attrs, body) tuples using HTML tag names, the
same structure that ghtml.HTMLParser produces, so it can be formatted to
groff by ggroff.Formatter (shared with g.html2man).

Beyond standard Markdown, the parser handles the MkDocs constructs that
appear in the generated documentation: YAML front matter, content tabs
(=== "Title"), admonitions (!!! type "Title"), and the &nbsp;-indented
parameter lists produced by --md-description. Tabs are returned as
("tabs", [], [("tab", [("title", ...)], body), ...]) nodes so the caller
can decide which tabs to keep.
"""

import html
import re
import sys
from pathlib import Path

__all__ = ["parse", "parse_blocks", "parse_inline"]

FENCE_RE = re.compile(r"^(```+|~~~+)\s*(\S*)\s*$")
HEADING_RE = re.compile(r"^(#{1,6})\s+(.*?)\s*#*\s*$")
TAB_RE = re.compile(r'^===\s+"(.*)"\s*$')
ADMONITION_RE = re.compile(r'^!!!\s+(\S+)(?:\s+"(.*)")?\s*$')
HR_RE = re.compile(r"^(-{3,}|\*{3,}|_{3,})\s*$")
LIST_ITEM_RE = re.compile(r"^(\s*)([-*+]|\d+[.)])\s+(.*)$")
TABLE_ROW_RE = re.compile(r"^\s*\|")
TABLE_SEP_RE = re.compile(r"^\s*\|?[\s:|-]*-[\s:|-]*\|?\s*$")
BLOCKQUOTE_RE = re.compile(r"^\s*>\s?")
NBSP_RUN_RE = re.compile(r"^(?:&nbsp;)+")
FRONT_MATTER_KEY_RE = re.compile(r"^([A-Za-z_][\w-]*):\s*(.*)$")

# Only these names trigger HTML parsing, so that non-HTML angle-bracket
# text (e.g. <elevation> placeholders) is left alone. The lookahead
# excludes matches like <i.group> (a tool name, not an <i> tag).
HTML_TAG_RE = re.compile(
    r"</?(a|span|img|ul|ol|li|p|div|table|thead|tbody|tfoot|tr|td|th|style"
    r"|script|center|blockquote|b|i|em|strong|code|tt|u|sup|sub|br|hr"
    r"|dl|dt|dd|label|font|small|big|pre|h[1-6])(?=[\s/>])",
    re.IGNORECASE,
)

# The emphasis content groups are fully lazy ((?:...)?? rather than
# (?:...)?) so that a span closes at the earliest closing marker. The em
# pattern allows internal **strong** spans so that emphasis like
# *text with **bold** inside* nests correctly.
INLINE_RE = re.compile(
    r"(?P<code>`+)"
    r"|(?P<limg>\[!\[(?P<limgalt>[^\]]*)\]\([^)]*\)\]\((?P<limgurl>[^)]*)\))"
    r"|(?P<img>!\[(?P<imgalt>[^\]]*)\]\((?P<imgsrc>[^)]*)\))"
    r"|(?P<link>\[(?P<linktext>[^\]]+)\]\((?P<linkurl>[^)]*)\))"
    r"|(?P<auto><(?P<autourl>https?://[^>\s]+)>)"
    r"|\*\*\*(?P<strongem>[^*\s](?:.*?[^*\s])??)\*\*\*"
    r"|\*\*(?P<strong>[^*\s](?:.*?[^*\s])??)\*\*"
    r"|\*(?P<em>[^*\s](?:(?:[^*]|\*\*[^*]+?\*\*)*?[^*\s])??)\*(?!\*)",
    re.DOTALL,
)

# Backslash escapes are replaced with private use area placeholders
# before inline parsing so that an emphasis span cannot pair with an
# escaped asterisk; they are restored when text is emitted.
ESCAPE_RE = re.compile(r"\\([\\`*_{}\[\]()#+\-.!<>|])")
ESCAPE_PLACEHOLDER_BASE = 0xE000
ESCAPE_PLACEHOLDER_RE = re.compile(r"[\ue000-\ue0ff]")


def protect_escapes(text):
    return ESCAPE_RE.sub(lambda m: chr(ESCAPE_PLACEHOLDER_BASE + ord(m.group(1))), text)


def restore_escapes(text, keep_backslash=False):
    def restore(match):
        char = chr(ord(match.group(0)) - ESCAPE_PLACEHOLDER_BASE)
        return "\\" + char if keep_backslash else char

    return ESCAPE_PLACEHOLDER_RE.sub(restore, text)


# Entities that html.unescape maps to characters groff input should not
# contain; ggroff escapes the rest itself.
CHARACTER_REPLACEMENTS = {"\xa0": " ", "•": "*"}

GHTML_ENTITIES = {"nbsp": " ", "bull": "*"}


def _ghtml_parser():
    """Create an HTML parser, importing ghtml from g.html2man if needed."""
    try:
        from ghtml import HTMLParser
    except ImportError:
        sys.path.append(str(Path(__file__).resolve().parent.parent / "g.html2man"))
        from ghtml import HTMLParser
    return HTMLParser(GHTML_ENTITIES)


def unescape_text(text):
    """Decode HTML entities and escape placeholders in plain text."""
    text = html.unescape(text)
    for char, replacement in CHARACTER_REPLACEMENTS.items():
        text = text.replace(char, replacement)
    return restore_escapes(text)


def parse_inline(text):
    """Parse inline Markdown into a list of tree nodes and strings."""
    # Protecting escapes again on recursive calls is a no-op.
    text = protect_escapes(text)
    nodes = []

    def emit_text(segment):
        if segment:
            nodes.append(unescape_text(segment))

    pos = 0
    while pos < len(text):
        match = INLINE_RE.search(text, pos)
        if not match:
            emit_text(text[pos:])
            break
        emit_text(text[pos : match.start()])
        pos = match.end()
        if match.group("code"):
            fence = match.group("code")
            end = text.find(fence, pos)
            if end < 0:
                emit_text(fence)
            else:
                # Code spans are literal; escaped characters keep their
                # backslash.
                content = restore_escapes(text[pos:end].strip(), keep_backslash=True)
                nodes.append(("code", [], [content]))
                pos = end + len(fence)
        elif match.group("img"):
            # Images cannot be rendered in a man page; g.html2man drops
            # them as well.
            pass
        elif match.group("limg"):
            # For an image wrapped in a link, keep the alt text.
            nodes.extend(parse_inline(match.group("limgalt")))
        elif match.group("link"):
            # Man pages cannot follow links; keep the link text only,
            # like g.html2man does.
            nodes.extend(parse_inline(match.group("linktext")))
        elif match.group("auto"):
            nodes.append(unescape_text(match.group("autourl")))
        elif match.group("strongem"):
            nodes.append(("b", [], [("i", [], parse_inline(match.group("strongem")))]))
        elif match.group("strong"):
            nodes.append(("b", [], parse_inline(match.group("strong"))))
        elif match.group("em"):
            nodes.append(("i", [], parse_inline(match.group("em"))))
    return nodes


def unquote_yaml(value):
    """Decode a YAML scalar as written by parser_md.c's print_yaml_escaped.

    A double-quoted value has its surrounding quotes removed and the only
    escapes the producer emits, \\" and \\\\, undone. Unquoted values are
    returned unchanged.
    """
    if len(value) >= 2 and value[0] == '"' and value[-1] == '"':
        value = value[1:-1]
        value = re.sub(r"\\(.)", r"\1", value)
    return value


def parse_front_matter(lines):
    """Parse YAML front matter, returning (metadata, remaining lines).

    Only the flat key: value and key: [list] forms used by the generated
    documentation are recognized; nested keys are ignored.
    """
    meta = {}
    if not lines or lines[0].strip() != "---":
        return meta, lines
    i = 1
    while i < len(lines) and lines[i].strip() != "---":
        match = FRONT_MATTER_KEY_RE.match(lines[i])
        if match:
            key, value = match.group(1), match.group(2).strip()
            if value.startswith("[") and value.endswith("]"):
                meta[key] = [
                    item.strip().strip("\"'")
                    for item in value[1:-1].split(",")
                    if item.strip()
                ]
            elif value:
                meta[key] = unquote_yaml(value)
        i += 1
    if i >= len(lines):
        # Unterminated block: treat the file as having no front matter.
        return {}, lines
    return meta, lines[i + 1 :]


def strip_line_comments(line):
    """Remove complete HTML comments from a single line.

    Returns the cleaned line and whether a comment is left open (an
    unterminated ``<!--`` whose ``-->`` is on a later line).
    """
    while True:
        start = line.find("<!--")
        if start < 0:
            return line, False
        end = line.find("-->", start + 4)
        if end < 0:
            return line[:start], True
        line = line[:start] + line[end + 3 :]


def strip_html_comments(lines):
    """Remove HTML comments, except inside fenced code blocks.

    Comments are handled line by line (rather than with a single regular
    expression) so that fenced code blocks are preserved and comments
    spanning multiple lines are removed correctly.
    """
    out = []
    in_fence = False
    in_comment = False
    for line in lines:
        if not in_comment and FENCE_RE.match(line.strip()):
            in_fence = not in_fence
        if in_fence:
            out.append(line)
            continue
        if in_comment:
            end = line.find("-->")
            if end < 0:
                continue
            line = line[end + 3 :]
            in_comment = False
        line, in_comment = strip_line_comments(line)
        out.append(line)
    return out


# The opener is anchored to the start of the line so that a <style> or
# <script> mentioned mid-line (e.g. inside an inline code span) is not
# mistaken for the block that heads the *_graphical.md pages.
RAW_BLOCK_OPEN_RE = re.compile(r"^\s*<(style|script)\b[^>]*>", re.IGNORECASE)
RAW_BLOCK_CLOSE_RE = re.compile(r"</(style|script)\s*>", re.IGNORECASE)


def strip_raw_html_blocks(lines):
    """Remove line-leading <style> and <script> blocks, except in code fences.

    Stylesheets and scripts (present at the top of the *_graphical.md index
    pages) cannot render in a man page. Removing them here keeps their
    content, which is not valid flow content, out of the HTML parser.
    """
    out = []
    in_fence = False
    in_block = False
    for line in lines:
        if not in_block and FENCE_RE.match(line.strip()):
            in_fence = not in_fence
        if in_fence:
            out.append(line)
            continue
        if in_block:
            close = RAW_BLOCK_CLOSE_RE.search(line)
            if not close:
                continue
            line = line[close.end() :]
            in_block = False
        opening = RAW_BLOCK_OPEN_RE.match(line)
        if opening:
            close = RAW_BLOCK_CLOSE_RE.search(line, opening.end())
            if close:
                line = line[close.end() :]
            else:
                line = ""
                in_block = True
        out.append(line)
    return out


def parse(text):
    """Parse a Markdown document, returning (metadata, tree nodes)."""
    meta, lines = parse_front_matter(text.splitlines())
    lines = strip_raw_html_blocks(strip_html_comments(lines))
    return meta, parse_blocks(lines)


def parse_blocks(lines):
    """Parse a list of Markdown lines into a list of block-level nodes."""
    nodes = []
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        if not stripped:
            i += 1
            continue
        if FENCE_RE.match(stripped):
            node, i = parse_fence(lines, i)
            nodes.append(node)
        elif TAB_RE.match(stripped):
            node, i = parse_tabs(lines, i)
            nodes.append(node)
        elif ADMONITION_RE.match(stripped):
            new_nodes, i = parse_admonition(lines, i)
            nodes.extend(new_nodes)
        elif HEADING_RE.match(stripped):
            match = HEADING_RE.match(stripped)
            # groff has no deeper levels than .SS; clamp h5/h6 to h4.
            level = min(len(match.group(1)), 4)
            nodes.append(("h%d" % level, [], parse_inline(match.group(2))))
            i += 1
        elif HR_RE.match(stripped):
            nodes.append(("hr", [], None))
            i += 1
        elif is_table_start(lines, i):
            node, i = parse_table(lines, i)
            nodes.append(node)
        elif LIST_ITEM_RE.match(line):
            node, i = parse_list(lines, i)
            nodes.append(node)
        elif BLOCKQUOTE_RE.match(line):
            new_nodes, i = parse_blockquote(lines, i)
            nodes.extend(new_nodes)
        elif stripped.startswith("<") and HTML_TAG_RE.match(stripped):
            new_nodes, i = parse_html_block(lines, i)
            nodes.extend(new_nodes)
        else:
            node, i = parse_paragraph(lines, i)
            nodes.append(node)
    return nodes


def list_interrupts_paragraph(line):
    """Check whether a list item on this line may interrupt a paragraph.

    Following CommonMark, an ordered list interrupts a paragraph only when
    it starts at 1, so a wrapped line beginning with a number or a year
    (e.g. "2005. ISSN ...") is not mistaken for a list item.
    """
    match = LIST_ITEM_RE.match(line)
    if not match:
        return False
    marker = match.group(2)
    if marker[0].isdigit():
        return marker[:-1] == "1"
    return True


def is_block_start(lines, i):
    """Check whether line i starts a non-paragraph block."""
    stripped = lines[i].strip()
    if not stripped:
        return True
    return bool(
        FENCE_RE.match(stripped)
        or TAB_RE.match(stripped)
        or ADMONITION_RE.match(stripped)
        or HEADING_RE.match(stripped)
        or HR_RE.match(stripped)
        or is_table_start(lines, i)
        or list_interrupts_paragraph(lines[i])
        or BLOCKQUOTE_RE.match(lines[i])
    )


def is_table_start(lines, i):
    return (
        TABLE_ROW_RE.match(lines[i])
        and i + 1 < len(lines)
        and TABLE_SEP_RE.match(lines[i + 1])
    )


def parse_fence(lines, i):
    fence = FENCE_RE.match(lines[i].strip()).group(1)
    i += 1
    content = []
    while i < len(lines):
        if lines[i].strip().startswith(fence[:3]):
            i += 1
            break
        content.append(lines[i])
        i += 1
    return ("pre", [], ["\n".join(content)]), i


def parse_indented_body(lines, i):
    """Collect the four-space indented body of a tab or admonition."""
    body = []
    while i < len(lines):
        line = lines[i]
        if not line.strip():
            body.append("")
        elif line.startswith("    "):
            body.append(line[4:])
        else:
            break
        i += 1
    # Drop trailing blank lines so a blank gap before unindented content
    # is not part of the body.
    while body and not body[-1]:
        body.pop()
    return body, i


def parse_tabs(lines, i):
    tabs = []
    while i < len(lines):
        match = TAB_RE.match(lines[i].strip())
        if not match:
            break
        body, i = parse_indented_body(lines, i + 1)
        tabs.append(("tab", [("title", match.group(1))], parse_blocks(body)))
        # parse_indented_body has already consumed the blank lines between
        # tabs, so the loop condition sees the next tab (or ends the group).
    return ("tabs", [], tabs), i


def parse_admonition(lines, i):
    match = ADMONITION_RE.match(lines[i].strip())
    title = match.group(2) or match.group(1).replace("-", " ").capitalize()
    body, i = parse_indented_body(lines, i + 1)
    return [("p", [], [("b", [], [title])]), *parse_blocks(body)], i


CELL_BOUNDARY_RE = re.compile(r"(?<!\\)\|")


def split_table_row(line):
    # Split on unescaped pipes only; a \| inside a cell is a literal pipe
    # (e.g. the bitwise-or operator in the r.mapcalc table). parse_inline
    # later restores the escape.
    cells = CELL_BOUNDARY_RE.split(line.strip())
    # Drop the empty cells produced by the leading and trailing border
    # pipes, keeping genuinely empty interior cells.
    if cells and not cells[0].strip():
        cells = cells[1:]
    if cells and not cells[-1].strip():
        cells = cells[:-1]
    return [cell.strip() for cell in cells]


def parse_table(lines, i):
    rows = [
        ("tr", [], [("th", [], parse_inline(c)) for c in split_table_row(lines[i])])
    ]
    i += 2  # skip the header and separator lines
    while i < len(lines) and TABLE_ROW_RE.match(lines[i]):
        cells = split_table_row(lines[i])
        rows.append(("tr", [], [("td", [], parse_inline(c)) for c in cells]))
        i += 1
    return ("table", [], rows), i


def parse_list(lines, i):
    match = LIST_ITEM_RE.match(lines[i])
    indent = len(match.group(1))
    ordered = match.group(2)[0].isdigit()
    items = []
    while i < len(lines):
        match = LIST_ITEM_RE.match(lines[i])
        if not (
            match
            and len(match.group(1)) == indent
            and match.group(2)[0].isdigit() == ordered
        ):
            break
        content_indent = match.start(3)
        item_lines = [match.group(3)]
        i += 1
        while i < len(lines):
            line = lines[i]
            if not line.strip():
                # A blank line ends the item unless indented content follows.
                if i + 1 < len(lines) and lines[i + 1].startswith(" " * (indent + 2)):
                    item_lines.append("")
                    i += 1
                    continue
                break
            if not line.startswith(" " * (indent + 2)):
                break
            # Remove the item's indentation, but only leading spaces and
            # never more than the content column, so a lazily indented
            # continuation (fewer spaces than the marker width) does not
            # lose its first characters.
            leading_spaces = len(line) - len(line.lstrip(" "))
            item_lines.append(line[min(leading_spaces, content_indent) :])
            i += 1
        items.append(("li", [], unwrap_first_paragraph(parse_blocks(item_lines))))
        # Skip a blank line separating items of the same list.
        if i < len(lines) and not lines[i].strip():
            if i + 1 < len(lines) and LIST_ITEM_RE.match(lines[i + 1]):
                i += 1
    return ("ol" if ordered else "ul", [], items), i


def unwrap_first_paragraph(nodes):
    """Replace a leading paragraph node with its content.

    Inside a list item, a .PP paragraph macro would reset the indentation
    of the item, so the first paragraph's content is inlined.
    """
    if nodes and isinstance(nodes[0], tuple) and nodes[0][0] == "p":
        return nodes[0][2] + nodes[1:]
    return nodes


def parse_blockquote(lines, i):
    """Parse a blockquote as its content; groff man has no quote markup.

    This matches g.html2man, which also renders blockquote content as
    regular text.
    """
    block = []
    while i < len(lines) and BLOCKQUOTE_RE.match(lines[i]):
        block.append(BLOCKQUOTE_RE.sub("", lines[i], count=1))
        i += 1
    return parse_blocks(block), i


def parse_html_block(lines, i):
    """Parse raw HTML lines with ghtml and splice in the resulting nodes.

    The lines are parsed inside a synthetic <body> so ghtml has a valid
    root context. Without it, force-closing an inline-only container such
    as an unclosed <p> (legacy HTML omits the closing tag) would empty the
    tag stack and raise, silently dropping content. Any tags still open at
    the end of the block are closed, as an HTML renderer would.
    """
    parser = _ghtml_parser()
    parser.handle_starttag("body", [])
    start = i
    try:
        while i < len(lines):
            # A blank line ends the block only when no element is still
            # open; an unclosed <p> continues across it to the next block.
            if not lines[i].strip() and len(parser.tag_stack) == 1:
                i += 1
                break
            parser.feed(lines[i] + "\n")
            i += 1
        parser.close()
        while len(parser.tag_stack) > 1:
            parser.pop()
    except Exception as error:  # noqa: BLE001
        # Malformed HTML, e.g. a stray closing tag. Drop the offending
        # line, like a web browser rendering it would.
        sys.stderr.write(f"invalid HTML ({error}): {lines[start]}\n")
        return [], start + 1
    # parser.data now holds the body's children. Stylesheets cannot be
    # rendered in a man page (scripts are already masked by ghtml itself).
    nodes = [
        node
        for node in parser.data
        if not (isinstance(node, tuple) and node[0] == "style")
    ]
    return nodes, i


# Matches an inline code span (a backtick run closed by an equal run), used
# to mask code spans before sniffing a paragraph for HTML: a tag inside a
# code span is literal text, and a `<script>` mention in particular must not
# reach the HTML parser, which would treat it as a real script opener and
# swallow the rest of the paragraph.
CODE_SPAN_RE = re.compile(r"(`+).+?\1", re.DOTALL)


def parse_paragraph(lines, i):
    block = []
    while i < len(lines) and not is_block_start(lines, i):
        block.append(lines[i])
        i += 1
    if any(NBSP_RUN_RE.match(line) for line in block):
        return build_definition_list(block), i
    text = "\n".join(block)
    if HTML_TAG_RE.search(CODE_SPAN_RE.sub("", text)):
        # A paragraph containing inline HTML is handed to the HTML parser as
        # a whole, so any Markdown markup in it (e.g. **bold** next to a
        # <br>) is left literal. No core page mixes the two this way; pages
        # that do (some legacy addon docs) should be fixed to use one or the
        # other. Revisit with the g.extension/addon docs follow-up.
        parser = _ghtml_parser()
        try:
            parser.feed(text + "\n")
            parser.close()
        except Exception as error:  # noqa: BLE001
            # Malformed HTML; fall back to plain Markdown parsing.
            sys.stderr.write(f"invalid HTML ({error}): {block[0]}\n")
            return build_paragraph(block), i
        return ("p", [], parser.data), i
    return build_paragraph(block), i


def build_definition_list(block):
    """Build a definition list from a &nbsp;-indented parameter list.

    The generated parameter documentation uses lines of the form
    **name**=*type* followed by &nbsp;-indented description lines. Mapping
    them to dt/dd produces the same .IP hanging indents as the previous
    HTML-based man pages.
    """
    body = []
    for line in block:
        match = NBSP_RUN_RE.match(line)
        if match:
            body.append(("dd", [], parse_inline(line[match.end() :].rstrip())))
        else:
            body.append(("dt", [], parse_inline(line.rstrip())))
    return ("dl", [], body)


def build_paragraph(block):
    # Inline constructs may span lines, so the paragraph is parsed as a
    # whole and split only at hard line breaks (trailing double space).
    body = []
    segment = []

    def flush():
        if segment:
            body.extend(parse_inline("\n".join(segment)))
            segment.clear()

    for line in block:
        segment.append(line.rstrip())
        if line.endswith("  "):
            flush()
            body.append(("br", [], None))
    flush()
    return ("p", [], body)
