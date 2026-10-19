"""Python-Markdown extension for GRASS documentation.

The documentation follows CommonMark (as GitHub and pandoc do), while
Python-Markdown follows the original Markdown rules for backslash escapes.
This extension adjusts the escapes.
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
