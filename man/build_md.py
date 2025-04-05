import os
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

To learn more about these tool in general, go to [${modclass} introduction](${modclass_lower}intro.md).
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

header_graphical_index_tmpl = """# Graphical index of GRASS tools
"""

############################################################################


def get_desc(cmd):
    desc = ""
    with open(cmd) as f:
        while True:
            line = f.readline()
            if not line:
                return desc
            if "description:" in line:
                desc = line.split(":", 1)[1].strip()
                break

    return desc


############################################################################

man_dir = os.path.join(os.environ["ARCH_DISTDIR"], "docs", "mkdocs", "source")

############################################################################
