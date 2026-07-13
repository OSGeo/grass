# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for the g.md2man Markdown to man page converter."""

import os
import subprocess
import sys
from pathlib import Path

import pytest

TOOL_DIR = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(TOOL_DIR))

import gmd  # noqa: E402

TOOL_PAGE = """\
---
name: r.example
description: "Does example things. Computes examples from a raster map."
keywords: [ raster, example ]
---

# r.example

Does example things.

Computes examples from a raster map.

=== "Command line"

    **r.example**
    [**-a**]
    **input**=*name*

    Example:

    ```sh
    r.example input=name
    ```

=== "Python (grass.tools)"

    *grass.tools.Tools.r_example*(**input**)

    ```python
    tools.r_example(input="name")
    ```

## Parameters

=== "Command line"

    **input**=*name* **[required]**
    &nbsp;&nbsp;&nbsp;&nbsp;Name of input raster map
    &nbsp;&nbsp;&nbsp;&nbsp;Default: *none*
    **-a**
    &nbsp;&nbsp;&nbsp;&nbsp;Align region

=== "Python (grass.tools)"

    **input** : str, *required*
    &nbsp;&nbsp;&nbsp;&nbsp;Name of input raster map

## DESCRIPTION

*r.example* computes **examples** with values &gt; 0 from
[r.mapcalc](r.mapcalc.md) output.

## SEE ALSO

*[r.mapcalc](r.mapcalc.md)*
"""


def convert(md_text, tmp_path, filename="r.example.md"):
    """Run g.md2man.py on the given text and return the groff output."""
    infile = tmp_path / filename
    outfile = tmp_path / (filename + ".1")
    infile.write_text(md_text, encoding="UTF-8")
    subprocess.run(
        [
            sys.executable,
            str(TOOL_DIR / "g.md2man.py"),
            str(infile),
            str(outfile),
        ],
        env={**os.environ, "VERSION_NUMBER": "9.9"},
        check=True,
    )
    return outfile.read_text(encoding="UTF-8")


@pytest.fixture
def tool_page(tmp_path):
    return convert(TOOL_PAGE, tmp_path)


def test_title_and_name_section(tool_page):
    assert tool_page.startswith('.TH r.example 1 "" "GRASS 9.9" "GRASS User\'s Manual"')
    assert ".SH NAME" in tool_page
    assert "\\fBr.example\\fR \\- Does example things." in tool_page


def test_keywords_section(tool_page):
    assert ".SH KEYWORDS\nraster, example" in tool_page


def test_synopsis_from_command_line_tab(tool_page):
    name = tool_page.index(".SH NAME")
    synopsis = tool_page.index(".SH SYNOPSIS")
    description = tool_page.index(".SH DESCRIPTION")
    assert name < synopsis < description
    assert "\\fBinput\\fR=\\fIname\\fR" in tool_page


def test_python_tabs_dropped(tool_page):
    assert "grass.tools" not in tool_page
    assert "r_example" not in tool_page


def test_description_not_duplicated(tool_page):
    # The lead paragraphs repeating the front matter description are
    # dropped; the description appears only in NAME.
    assert tool_page.count("Does example things.") == 1


def test_parameter_definition_list(tool_page):
    assert '.IP "\\fBinput\\fR=\\fIname\\fR \\fB[required]\\fR" 4m' in tool_page
    assert "Name of input raster map" in tool_page
    assert "Default: \\fInone\\fR" in tool_page


def test_code_block(tool_page):
    assert ".nf" in tool_page
    assert "r.example input=name" in tool_page


def test_entities_and_link_text(tool_page):
    assert "values > 0" in tool_page
    # Link target is dropped, link text is kept.
    assert "r.mapcalc.md" not in tool_page
    assert "r.mapcalc" in tool_page


def test_page_without_front_matter(tmp_path):
    text = "# Topic: hydrology\n\nSome tools.\n\n## Tools\n\nMore text.\n"
    result = convert(text, tmp_path, filename="topic_hydrology.md")
    assert result.startswith(".TH topic_hydrology 1")
    assert ".SH Topic: hydrology" in result
    assert ".SH Tools" in result


def test_headings(tmp_path):
    result = convert("## Section\n\n### Subsection\n\n#### Deeper\n", tmp_path)
    assert ".SH Section" in result
    assert ".SS Subsection" in result
    assert ".SS Deeper" in result


def test_unordered_list_nested(tmp_path):
    result = convert("- first\n- second\n    - nested\n", tmp_path)
    assert result.count(".IP \\(bu 4n") == 3  # codespell:ignore bu
    assert ".RS 4n" in result


def test_wrapped_number_line_is_not_a_list(tmp_path):
    # A paragraph continuation beginning with a number (e.g. a year in a
    # citation) must not be treated as an ordered list item.
    md = "M. Neteler, 2005. Title.\nVol.3, pp. 2-6, June\n2005. ISSN 1614-8746.\n"
    result = convert(md, tmp_path)
    assert "2005. ISSN" in result
    assert ".IP \\fB1\\fR" not in result


def test_ordered_list_may_interrupt_paragraph_at_one(tmp_path):
    md = "Intro text.\n1. first\n2. second\n"
    result = convert(md, tmp_path)
    assert "Intro text." in result
    assert ".IP \\fB1\\fR" in result
    assert ".IP \\fB2\\fR" in result


def test_ordered_list(tmp_path):
    result = convert("1. first\n2. second\n", tmp_path)
    assert ".IP \\fB1\\fR" in result
    assert ".IP \\fB2\\fR" in result


def test_list_lazy_continuation_keeps_first_char(tmp_path):
    # A continuation line indented less than the marker width must not lose
    # its leading characters ("Integers" -> "ntegers").
    md = '1. J. Rissanen, "A Universal Prior for\n  Integers and Estimation."\n'
    result = convert(md, tmp_path)
    assert "Integers and Estimation." in result


def test_table(tmp_path):
    md = "| Tool | Description |\n|------|-------------|\n| r.info | Info tool |\n"
    result = convert(md, tmp_path)
    assert ".TS" in result
    assert ".TE" in result  # codespell:ignore TE
    assert "r.info" in result


def test_table_escaped_pipe():
    # An escaped pipe is a literal cell value (a bitwise/logical operator in
    # the r.mapcalc table), not a cell boundary.
    assert gmd.split_table_row(r"| \|\|\| | logical or | Logical | 2 |") == [
        r"\|\|\|",
        "logical or",
        "Logical",
        "2",
    ]
    assert gmd.split_table_row("| a |  | b |") == ["a", "", "b"]


def test_hard_break(tmp_path):
    result = convert("first line  \nsecond line\n", tmp_path)
    assert "first line\n.br\nsecond line" in result


def test_admonition(tmp_path):
    md = '!!! grass-tip "Remember"\n\n    Useful advice.\n'
    result = convert(md, tmp_path)
    assert "\\fBRemember\\fR" in result
    assert "Useful advice." in result


def test_example_tabs_kept_with_title(tmp_path):
    md = (
        "## EXAMPLES\n\n"
        '=== "Bash"\n\n    ```sh\n    g.region -p\n    ```\n\n'
        '=== "Python (grass.script)"\n\n    ```python\n    gs.run_command()\n    ```\n'
    )
    result = convert(md, tmp_path)
    assert ".SS Bash" in result
    assert "g.region \\-p" in result
    assert "gs.run_command" not in result


def test_raw_html_block(tmp_path):
    md = "<ul>\n<li><b>bold</b> text</li>\n</ul>\n"
    result = convert(md, tmp_path)
    assert "\\fBbold\\fR text" in result


def test_inline_style_script_mention_not_swallowed(tmp_path):
    # A <script>/<style> token in a code span must not start a raw block or
    # send the paragraph to the HTML parser; both would delete text.
    md = "Use `<script>` tags in HTML.\n\nNext paragraph stays.\n"
    result = convert(md, tmp_path)
    assert "tags in HTML." in result
    assert "Next paragraph stays." in result


def test_style_block_dropped(tmp_path):
    md = "<style>\n.cls {\n    color: red;\n}\n</style>\n\nVisible text.\n"
    result = convert(md, tmp_path)
    assert "color" not in result
    assert "Visible text." in result


def test_front_matter_parsing():
    meta, nodes = gmd.parse(TOOL_PAGE)
    assert meta["name"] == "r.example"
    assert meta["keywords"] == ["raster", "example"]
    assert meta["description"].startswith("Does example things.")


def test_front_matter_unescapes_quotes(tmp_path):
    # parser_md.c wraps the description in double quotes and escapes any
    # embedded " and \ as \" and \\; the parser must undo that so the NAME
    # line is clean and matches the (unescaped) body paragraph for dedup.
    md = (
        "---\nname: r.volume\n"
        'description: "Calculates the volume of data \\"clumps\\"."\n'
        "keywords: [ raster ]\n---\n\n# r.volume\n\n"
        'Calculates the volume of data "clumps".\n\n'
        "## DESCRIPTION\n\nText.\n"
    )
    meta, _nodes = gmd.parse(md)
    assert meta["description"] == 'Calculates the volume of data "clumps".'
    result = convert(md, tmp_path, filename="r.volume.md")
    assert "\\fBr.volume\\fR \\- Calculates the volume of data" in result
    # The escaped backslash sequences must not survive into the output, and
    # the description must not be duplicated below SYNOPSIS.
    assert "\\\\" not in result
    assert result.count("clumps") == 1


def test_front_matter_nested_keys_ignored():
    meta, nodes = gmd.parse("---\nhide:\n  - toc\n---\n\n# Title\n")
    assert nodes[0][0] == "h1"


def test_inline_emphasis():
    nodes = gmd.parse_inline("**bold** and *italic* and `code`")
    assert ("b", [], ["bold"]) in nodes
    assert ("i", [], ["italic"]) in nodes
    assert ("code", [], ["code"]) in nodes


def test_inline_escapes_and_autolink():
    # The escaped asterisk stays literal and the autolink becomes a plain
    # text node holding the URL.
    nodes = gmd.parse_inline(r"a \* literal <https://example.com>")
    assert nodes == ["a * literal ", "https://example.com"]


def test_escaped_asterisk_does_not_pair_with_emphasis():
    nodes = gmd.parse_inline(r"The *db.\** set and *v.db.\** set")
    assert ("i", [], ["db.*"]) in nodes
    assert ("i", [], ["v.db.*"]) in nodes


def test_emphasis_closes_at_earliest_marker():
    nodes = gmd.parse_inline(r"\[-**s**\] \[-**t**\]")
    assert nodes == ["[-", ("b", [], ["s"]), "] [-", ("b", [], ["t"]), "]"]


def test_linked_image_keeps_alt_text():
    nodes = gmd.parse_inline("[![Raster](gi_raster.jpg)](raster_graphical.md)")
    assert nodes == ["Raster"]


def test_code_span_keeps_backslash_escapes():
    nodes = gmd.parse_inline(r"`a\*b`")
    assert nodes == [("code", [], ["a\\*b"])]


def test_tool_placeholder_not_html():
    # Angle-bracket placeholders like <i.group> are not HTML tags.
    (node,) = gmd.parse_blocks(["Use <i.group> here."])
    assert node[0] == "p"
    assert "Use <i.group> here." in node[2]


def test_blockquote_content_kept(tmp_path):
    md = "Intro text.\n\n> xps \\>= 0 means the target is at sea level.\n> More quoted text.\n"
    result = convert(md, tmp_path)
    assert ">= 0 means the target" in result
    assert "More quoted text." in result
    assert "\n> " not in result


def test_stray_closing_tag_does_not_crash(tmp_path):
    md = "Some text.\n\n  </li>\n\nMore text.\n"
    result = convert(md, tmp_path)
    assert "Some text." in result
    assert "More text." in result
    assert "li" not in result


def test_html_only_page(tmp_path):
    # g.extension copies <tool>.html to <tool>.md for addons that have
    # no Markdown documentation; the converter must handle HTML headings.
    md = "<h2>DESCRIPTION</h2>\n\n<em>r.fake</em> uses <b>input</b>.\n\n<pre>\nr.fake input=elev\n</pre>\n"
    result = convert(md, tmp_path)
    assert ".SH DESCRIPTION" in result
    assert "<h2>" not in result
    assert "\\fBinput\\fR" in result
    assert "r.fake input=elev" in result


def test_html_unclosed_paragraphs_kept(tmp_path):
    # Legacy HTML omits the closing </p>; the following <p> must not empty
    # the parser's tag stack and silently drop the first paragraph.
    md = "<h2>DESCRIPTION</h2>\n\n<p>First paragraph.\n\n<p>Second paragraph.\n"
    result = convert(md, tmp_path)
    assert "First paragraph." in result
    assert "Second paragraph." in result


def test_inline_comment_removed(tmp_path):
    result = convert("Before <!-- hide this --> after.\n", tmp_path)
    assert "Before  after." in result
    assert "hide this" not in result


def test_multiline_comment_removed(tmp_path):
    md = "Visible start.\n\n<!-- comment\nspanning several\nlines -->\n\nVisible end.\n"
    result = convert(md, tmp_path)
    assert "Visible start." in result
    assert "Visible end." in result
    assert "comment" not in result
    assert "spanning" not in result


def test_comment_kept_in_fenced_code(tmp_path):
    # A comment inside a code block is documentation content, not markup,
    # so it must survive (groff escapes the dashes in the output).
    md = "```html\n<!-- required -->\n```\n"
    result = convert(md, tmp_path)
    assert "required" in result


def test_comment_parsing_unit():
    lines = ["a <!-- x --> b <!-- y", "z --> c", "<!-- lone -->d"]
    assert gmd.strip_html_comments(lines) == ["a  b ", " c", "d"]


def test_strip_html_comments_keeps_comment_in_fenced_code_unit():
    lines = ["```html", "<!-- required -->", "```", "x <!-- drop --> y"]
    assert gmd.strip_html_comments(lines) == ["```html", "<!-- required -->", "```", "x  y"]
