"""Python-Markdown extension for GRASS documentation.

The documentation follows CommonMark (as GitHub and pandoc do), while
the site generator uses Python-Markdown which follows the original
Markdown rules for backslash escapes. This extension adjusts the escapes.

It is a Python-Markdown extension rather than a site generator hook, so it
works with any generator which uses Python-Markdown and reads the
markdown_extensions setting (MkDocs and Zensical). The generator needs to
import it by name, so this directory needs to be on PYTHONPATH.
"""

from markdown.extensions import Extension


class EscapeLessThanExtension(Extension):
    r"""Add < to the characters which can be escaped with a backslash

    CommonMark allows escaping any ASCII punctuation, so the documentation
    uses, e.g., \<name\> or x \< 0. In the original Markdown rules, < is
    not escapable, so Python-Markdown would render the backslash.
    """

    def extendMarkdown(self, md):
        if "<" not in md.ESCAPED_CHARS:
            md.ESCAPED_CHARS.append("<")


def makeExtension(**kwargs):
    """Return the extension for Python-Markdown"""
    return EscapeLessThanExtension(**kwargs)
