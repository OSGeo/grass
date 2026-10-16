"""Adjust which characters can be escaped with a backslash in Markdown.

The documentation follows CommonMark (as GitHub and pandoc do), while
MkDocs uses Python-Markdown which follows the original Markdown rules
for backslash escapes.
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


def on_config(config):
    """Register the extension with Python-Markdown"""
    config.markdown_extensions.append(EscapeLessThanExtension())
    return config
