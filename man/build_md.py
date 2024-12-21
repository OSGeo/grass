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

# TODO: avoid HTML tags
overview_tmpl = string.Template(
    r"""<!-- the files grass${grass_version_major}.html & helptext.html file live in lib/init/ -->

<table align="center" border="0" cellspacing="8">
  <tbody>
    <tr>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Quick Introduction</h3>
      <ul>
       <li class="box"><a href="helptext.html">How to start with GRASS GIS</a></li>
       <li class="box"><span>Index of <a href="topics.html">topics</a> and <a href="keywords.html">keywords</a></span></li>
      </ul>
      <p>
      <ul>
       <li class="box"><a href="projectionintro.html">Intro: projections and spatial transformations</a></li>
      </ul>
      <p>
      <ul>
       <li class="box"><span><a href="https://grasswiki.osgeo.org/wiki/Faq">FAQ - Frequently Asked Questions</a> (Wiki)</span></li>
      </ul>
      <p>
      <ul>
       <li class="box"><span><a href="graphical_index.html">Graphical index of functionality</a></span></li>
      </ul>
      </td>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Graphical User Interface</h3>
       <ul>
        <li class="box"><a href="wxguiintro.html">Intro: Graphical User Interface</a></li>
        <li class="box"><span><a href="wxGUI.html">wxGUI</a></span></li>
        <li class="box"><a href="wxGUI.components.html">wxGUI components</a></li>
        <li class="box"><a href="wxGUI.toolboxes.html">wxGUI toolboxes</a></li>
       </ul>

       <ul>
        <li class="box"><a href="topic_GUI.html">GUI commands</a></li>
       </ul>
       <h3>&nbsp;Display</h3>
       <ul>
        <li class="box"><a href="display.html">Display commands manual</a></li>
        <li class="box"><a href="displaydrivers.html">Display drivers</a></li>
       </ul>
      </td>
      <td width="33%" valign="top" class="box"><h3>&nbsp;General</h3>
       <ul>
        <li class="box"><a href="grass.html">GRASS GIS startup manual</a></li>
        <li class="box"><a href="general.html">General commands manual</a></li>
       </ul>
        <h3>&nbsp;Addons</h3>
        <ul>
        <li class="box"><a href="https://grass.osgeo.org/grass8/manuals/addons/">Addons manual pages</a></li>
       </ul>
        <h3>&nbsp;Programmer's Manual</h3>
        <ul>
        <li class="box"><a href="https://grass.osgeo.org/programming8/">Programmer's Manual</a></li>
       </ul>
      </td>
    </tr>
    <tr>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Raster processing</h3>
       <ul>
        <li class="box"><a href="rasterintro.html">Intro: 2D raster map processing</a></li>
        <li class="box"><a href="raster.html">Raster commands manual</a></li>
       </ul>
      </td>
      <td width="33%" valign="top" class="box"><h3>&nbsp;3D raster processing</h3>
       <ul>
        <li class="box"><a href="raster3dintro.html">Intro: 3D raster map (voxel) processing</a></li>
        <li class="box"><a href="raster3d.html">3D raster (voxel) commands manual</a></li>
      </ul></td>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Image processing</h3>
       <ul>
        <li class="box"><a href="imageryintro.html">Intro: image processing</a></li>
        <li class="box"><a href="imagery.html">Imagery commands manual</a></li>
      </ul></td>
    </tr>
    <tr>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Vector processing</h3>
       <ul>
        <li class="box"><a href="vectorintro.html">Intro: vector map processing and network analysis</a></li>
        <li class="box"><a href="vector.html">Vector commands manual</a></li>
        <li class="box"><a href="vectorascii.html">GRASS ASCII vector format specification</a></li>
      </ul></td>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Database</h3>
       <ul>
        <li class="box"><a href="databaseintro.html">Intro: database management</a></li>
        <li class="box"><a href="sql.html">SQL support in GRASS GIS</a></li>
        <li class="box"><a href="database.html">Database commands manual</a></li>
       </ul>
      </td>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Temporal processing</h3>
       <ul>
        <li class="box"><a href="temporalintro.html">Intro: temporal data processing</a></li>
        <li class="box"><a href="temporal.html">Temporal commands manual</a></li>
       </ul>
      </td>
    </tr>
    <tr>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Cartography</h3>
       <ul>
        <li class="box"><a href="postscript.html">PostScript commands manual</a></li>
        <li class="box"><a href="g.gui.psmap.html">wxGUI Cartographic Composer</a></li>
       </ul>
      </td>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Miscellaneous&nbsp;&amp;&nbsp;Variables</h3>
       <ul>
        <li class="box"><a href="miscellaneous.html">Miscellaneous commands manual</a></li>
        <li class="box"><a href="variables.html">GRASS variables and environment variables</a></li>
       </ul>
      </td>
      <td width="33%" valign="top" class="box"><h3>&nbsp;Python</h3>
       <ul>
        <li class="box"><a href="https://grass.osgeo.org/grass${grass_version_major}${grass_version_minor}/manuals/libpython/index.html">GRASS GIS Python library documentation</a></li>
        <li class="box"><a href="https://grass.osgeo.org/grass${grass_version_major}${grass_version_minor}/manuals/libpython/pygrass_index.html">PyGRASS documentation</a></li>
        <li class="box"><a href="https://grass.osgeo.org/grass${grass_version_major}${grass_version_minor}/manuals/libpython/grass.jupyter.html">GRASS GIS in Jupyter Notebooks</a></li>
       </ul>
      </td>
    </tr>
  </tbody>
</table>
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
### ${cmd_label} commands (${cmd}.*)

| Module | Description |
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
# "


modclass_tmpl = string.Template(
    r"""Go [back to help overview](index.md)
### ${modclass} commands
| Module | Description |
|--------|-------------|
"""
)

desc2_tmpl = string.Template(
    r"""| [${basename}](${cmd}) | ${desc} |
"""
)

full_index_header = r"""Go [back to help overview](index.md)
"""

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

| Module | Description |
|--------|-------------|
"""
)


headerpso_tmpl = r"""
## Parser standard options
"""

header_graphical_index_tmpl = """# Graphical index of GRASS GIS modules
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
