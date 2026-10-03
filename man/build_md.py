import os
import re
import string

# File template pieces follow

header1_tmpl = string.Template(
    r"""---
title: ${title}
author: GRASS Development Team
---

"""
)

header2_tmpl = string.Template(r"""""")

cmd2_tmpl = string.Template(
    r"""
### ${cmd_label} tools (${cmd}.)

| Name | Description |
|--------|-------------|
"""
)

desc1_tmpl = string.Template(
    r"""| [${basename}](${cmd}) | ${desc} |
"""
)

modclass_intro_tmpl = string.Template(
    r"""# ${modclass} tools

To learn more, see the [${modclass} introduction](${modclass_lower}intro.md).
"""
)

modclass_tmpl = string.Template(
    r"""
| Name | Description |
|--------|-------------|
"""
)

desc2_tmpl = string.Template(
    r"""| [${basename}](${cmd}) | ${desc} |
"""
)

moduletopics_tmpl = string.Template(
    r"""
- [${name}](topic_${key}.md)
"""
)

headertopics_tmpl = r"""# Topics
"""

headerkeywords_tmpl = r"""# Keywords - Index of GRASS tools
"""

headerkey_tmpl = string.Template(
    r"""# Topic: ${keyword}

| Tool | Description |
|--------|-------------|
"""
)


headerpso_tmpl = r"""
# Standard Parser Options
"""

header_graphical_index_tmpl = """# Graphical index of tools
"""

############################################################################


def unquote_yaml(text):
    """Decode the description value from --md-description frontmatter.

    The value is emitted as a double-quoted YAML scalar so it can contain
    YAML-significant sequences such as ": ". This strips the surrounding
    quotes and undoes the backslash escaping of `"` and `\\`. A value that
    is not quoted is returned unchanged.
    """
    text = text.strip()
    if len(text) >= 2 and text.startswith('"') and text.endswith('"'):
        return re.sub(r"\\(.)", r"\1", text[1:-1])
    return text


def get_desc(cmd):
    desc = ""
    with open(cmd) as f:
        while True:
            line = f.readline()
            if not line:
                return desc
            if "description:" in line:
                desc = unquote_yaml(line.split(":", 1)[1])
                break

    return desc


############################################################################

man_dir = os.path.join(os.environ["ARCH_DISTDIR"], "docs", "mkdocs", "source")

############################################################################
