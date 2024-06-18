#!/usr/bin/env python3

# utilities for generating Markdown indices
# inspired by build_md.py
# (C) 2003-2024 Markus Neteler and the GRASS Development Team
# Authors:
#   Markus Neteler
#   Glynn Clements
#   Luca Delucchi
#   Martin Landa

import os
import string
from datetime import datetime

# TODO: better fix this in include/Make/Html.make, see bug RT #5361

# exclude following list of modules from help index:

exclude_mods = [
    "i.find",
    "r.watershed.ram",
    "r.watershed.seg",
    "v.topo.check",
    "helptext.html",
]

# these modules don't use G_parser()

desc_override = {
    "g.parser": "Provides automated parser, GUI, and help support for GRASS scipts.",
    "r.li.daemon": "Support module for r.li landscape index calculations.",
}

# File template pieces follow

header2_tmpl = string.Template(
    r"""# GRASS GIS ${grass_version} Reference Manual

**Geographic Resources Analysis Support System**, commonly
referred to as [GRASS GIS](https://grass.osgeo.org), is a
[Geographic Information System](https://en.wikipedia.org/wiki/Geographic_information_system) (GIS) used for geospatial data management and
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
# "

footer_tmpl = string.Template(  # TODO: https://trac.osgeo.org/grass/ticket/3987
    # r"""<a name="wxGUI"></a>
    # <h3>wxGUI: Graphical user interface</h3>
    # <table><tbody>
    # <tr><td valign="top"><a href="wxGUI.html">wxGUI Graphical User Interface</a></td> <td>wxGUI Graphical User Interface</td></tr>
    # <tr><td valign="top"><a href="wxGUI.nviz.html">3D visualization suite</a></td>    <td>wxGUI.nviz 3D visualization suite</td></tr>
    # </tbody></table>
    # <p>
    # <a name="further"></a>
    # <h3>Further pages</h3>
    # <table><tbody>
    # <tr><td valign="top"><a href="databaseintro.html">database intro</a></td> <td>database intro</td></tr>
    # <tr><td valign="top"><a href="imageryintro.html">imagery intro</a></td> <td>imagery intro</td></tr>
    # <tr><td valign="top"><a href="projectionintro.html">projection intro</a></td> <td>projection intro</td></tr>
    # <tr><td valign="top"><a href="raster3dintro.html">raster3D intro</a></td> <td>raster3D intro</td></tr>
    # <tr><td valign="top"><a href="rasterintro.html">raster intro</a></td> <td>raster intro</td></tr>
    # <tr><td valign="top"><a href="temporalintro.html">temporal intro</a></td> <td>temporal intro</td></tr>
    # <tr><td valign="top"><a href="vectorintro.html">vector intro</a></td> <td>vector intro</td></tr>
    # <tr><td valign="top"> </td> <td> </td></tr>
    # <tr><td valign="top"><a href="sql.html">SQL</a></td> <td>SQL</td></tr>
    # <tr><td valign="top"><a href="variables.html">Variables</a></td> <td>Variables</td></tr>
    # </tbody></table>
    #
    # <p>
    # <hr class="header">
    r"""
____
[Main index](${index_url}) |
[Topics index](topics.md) |
[Keywords index](keywords.md) |
[Graphical index](graphical_index.md) |
[Full index](full_index.md)

&copy; 2003-${year}
[GRASS Development Team](https://grass.osgeo.org),
GRASS GIS ${grass_version} Reference Manual
"""
)
# "

cmd2_tmpl = string.Template(
    r"""
### ${cmd_label} commands (${cmd}.*)

| Module | Description |
|--------|-------------|
"""
)
# "

desc1_tmpl = string.Template(
    r"""| [${basename}](${cmd}) | ${desc} |
"""
)
# "

toc = r"""
<div class="toc">
<h4 class="toc">Table of contents</h4>
<ul class="toc">
<li class="toc"><a class="toc" href="full_index.html#d">Display commands (d.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#db">Database commands (db.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#g">General commands (g.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#i">Imagery commands (i.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#m">Miscellaneous commands (m.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#ps">PostScript commands (ps.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#r">Raster commands (r.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#r3">3D raster commands (r3.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#t">Temporal commands (t.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#v">Vector commands (v.*)</a></li>
<li class="toc"><a class="toc" href="full_index.html#wxGUI">wxGUI Graphical User Interface</a></li>
<li class="toc"><a class="toc" href="full_index.html#further">Further pages</a></li>
</ul>
</div>
"""
# "

modclass_intro_tmpl = string.Template(
    r"""Go to <a href="${modclass_lower}intro.html">${modclass} introduction</a> | <a href="topics.html">topics</a> <p>
"""
)
# "

modclass_tmpl = string.Template(
    r"""Go <a href="index.html">back to help overview</a>
<h3>${modclass} commands:</h3>
<table>
"""
)
# "

desc2_tmpl = string.Template(
    r"""<tr><td valign="top"><a href="${cmd}">${basename}</a></td> <td>${desc}</td></tr>
"""
)
# "


full_index_header = r"""Go [back to help overview](index.md)
"""
# "


message_tmpl = string.Template(
    r"""Generated HTML docs in ${md_dir}/index.html
----------------------------------------------------------------------
Following modules are missing the 'modulename.html' file in src code:
"""
)
# "

moduletopics_tmpl = string.Template(
    r"""
- [${name}](topic_${key}.html)
"""
)

headertopics_tmpl = r"""# Topics
"""

headerkeywords_tmpl = r"""# Keywords - Index of GRASS GIS modules
"""
# "

headerkey_tmpl = string.Template(
    r"""# Topic: ${keyword}

| Module | Description |
|--------|-------------|
"""
)


headerpso_tmpl = r"""
<link rel="stylesheet" href="grassdocs.css" type="text/css">
<link rel="stylesheet" href="parser_standard_options.css" type="text/css">
<script src="https://code.jquery.com/jquery-1.11.3.min.js"></script>
<script type="text/javascript" src="jquery.fixedheadertable.min.js"></script>
<script type="text/javascript" src="parser_standard_options.js"></script>
</head>
<body style="width: 99%">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
<h2>Parser standard options</h2>
<ul>
"""
# "


header_graphical_index_tmpl = """# Graphical index of GRASS GIS modules
"""

############################################################################


def check_for_desc_override(basename):
    return desc_override.get(basename)


def read_file(name):
    with open(name, "r") as f:
        s = f.read()

    return s


def write_file(name, contents):
    with open(name, "w") as f:
        f.write(contents)


def try_mkdir(path):
    try:
        os.mkdir(path)
    except OSError:
        pass


# TODO: comment-out when build_html is removed
# def replace_file(name):
#     temp = name + ".tmp"
#     if (
#         os.path.exists(name)
#         and os.path.exists(temp)
#         and read_file(name) == read_file(temp)
#     ):
#         os.remove(temp)
#     else:
#         try:
#             os.remove(name)
#         except OSError:
#             pass
#         os.rename(temp, name)


def copy_file(src, dst):
    write_file(dst, read_file(src))


def md_files(cls=None, ignore_gui=True):
    for cmd in sorted(os.listdir(md_dir)):
        if (
            cmd.endswith(".md")
            and (cls in [None, "*"] or cmd.startswith(cls + "."))
            and (cls != "*" or len(cmd.split(".")) >= 3)
            and cmd not in ["full_index.md", "index.md"]
            and cmd not in exclude_mods
            and (ignore_gui and not cmd.startswith("wxGUI.") or not ignore_gui)
        ):
            yield cmd


def write_md_header(f, title, ismain=False, body_width="99%"):
    # f.write(header1_tmpl.substitute(title=title))
    # if ismain and macosx:
    #     f.write(
    #         macosx_tmpl.substitute(grass_version=grass_version,
    #                                grass_mmver=grass_mmver)
    #     )
    f.write(header2_tmpl.substitute(grass_version=grass_version, body_width=body_width))


def write_md_cmd_overview(f):
    f.write(
        overview_tmpl.substitute(
            grass_version_major=grass_version_major,
            grass_version_minor=grass_version_minor,
        )
    )


def write_md_footer(f, index_url, year=None):
    if year is None:
        cur_year = default_year
    else:
        cur_year = year
    f.write(
        footer_tmpl.substitute(
            grass_version=grass_version, index_url=index_url, year=cur_year
        )
    )


def get_desc(cmd):
    desc = ""
    with open(cmd, "r") as f:
        while True:
            line = f.readline()
            if not line:
                return desc
            if "meta page module description" in line:
                desc = line.split(":", 1)[1].rstrip("-->\n").strip()
                break

    return desc


def to_title(name):
    """Convert name of command class/family to form suitable for title"""
    if name == "PostScript":
        return name
    return name.capitalize()


arch_dist_dir = os.environ["ARCH_DISTDIR"]
md_dir = os.path.join(arch_dist_dir, "docs", "markdown", "source")
gisbase = os.environ["GISBASE"]
grass_version = os.getenv("VERSION_NUMBER", "unknown")
grass_version_major = grass_version.split(".")[0]
grass_version_minor = grass_version.split(".")[1]
grass_mmver = ".".join(grass_version.split(".")[0:2])
macosx = "darwin" in os.environ["ARCH"].lower()
default_year = os.getenv("VERSION_DATE")
if not default_year:
    default_year = str(datetime.now().year)
