"""Make Python-Markdown accept the backslash escape of the less-than sign.

CommonMark (GitHub, pandoc) allows escaping any ASCII punctuation, so the
documentation uses, e.g., \\<name\\> or x \\< 0. Python-Markdown follows the
original Markdown rules where < is not escapable, so the backslash would be
rendered. Python-Markdown lets extensions add escapable characters
(the tables extension adds | the same way).
"""

from markdown.extensions import Extension


class EscapeLessThanExtension(Extension):
    """Add < to the characters which can be escaped with a backslash"""

    def extendMarkdown(self, md):
        if "<" not in md.ESCAPED_CHARS:
            md.ESCAPED_CHARS.append("<")


def on_config(config):
    """Register the extension with Python-Markdown"""
    config.markdown_extensions.append(EscapeLessThanExtension())
    return config
