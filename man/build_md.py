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

macosx_tmpl = string.Template(
    r"""
AppleTitle: GRASS GIS ${grass_version}
AppleIcon: GRASS-${grass_mmver}/grass_icon.png
"""
)

header2_tmpl = string.Template(
    r"""# GRASS GIS ${grass_version} Reference Manual

**Geographic Resources Analysis Support System**, commonly
referred to as [GRASS GIS](https://grass.osgeo.org), is a
[Geographic Information System](https://en.wikipedia.org/wiki/Geographic_information_system)
(GIS) used for geospatial data management and
analysis, image processing, graphics/maps production, spatial
modeling, and visualization. GRASS is currently used in academic and
commercial settings around the world, as well as by many governmental
agencies and environmental consulting companies.

This reference manual details the use of modules distributed with
Geographic Resources Analysis Support System (GRASS), an open source
([GNU GPLed](https://www.gnu.org/licenses/gpl.html), image
processing and geographic information system (GIS).

"""
)

# footer_tmpl = string.Template(
#     r"""
# ____
# [Main index](${index_url}) |
# [Topics index](topics.md) |
# [Keywords index](keywords.md) |
# [Graphical index](graphical_index.md) |
# [Full index](full_index.md)

# &copy; 2003-${year}
# [GRASS Development Team](https://grass.osgeo.org),
# GRASS GIS ${grass_version} Reference Manual
# """
# )
# replaced by footer
footer_tmpl = string.Template("")

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
    r"""Go to [${modclass} introduction](${modclass_lower}intro.md) | [topics](topics.md)
"""
)

modclass_tmpl = string.Template(
    r"""
## ${modclass} tools
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

headerkeywords_tmpl = r"""# Keywords - Index of GRASS GIS modules
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

header_graphical_index_tmpl = """# Graphical index
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
