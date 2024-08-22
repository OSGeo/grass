import os
import string

from build import arch_dist_dir

# File template pieces follow

header1_tmpl = string.Template(
    r"""<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.0 Transitional//EN">
<html>
<head>
 <meta http-equiv="Content-Type" content="text/html; charset=utf-8">
 <title>${title} - GRASS GIS Manual</title>
 <meta name="Author" content="GRASS Development Team">
 <meta http-equiv="content-language" content="en-us">
 <meta name="viewport" content="width=device-width, initial-scale=1">
"""
)

macosx_tmpl = string.Template(
    r"""
 <meta name="AppleTitle" content="GRASS GIS ${grass_version} Help">
 <meta name="AppleIcon" content="GRASS-${grass_mmver}/grass_icon.png">
 <meta name="robots" content="anchors">
"""
)

header2_tmpl = string.Template(
    r""" <link rel="stylesheet" href="grassdocs.css" type="text/css">
</head>
<body style="width: ${body_width}">
<div id="container">
<!-- this file is generated by man/build_html.py -->

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">

<h2>GRASS GIS ${grass_version} Reference Manual</h2>

<p><b>Geographic Resources Analysis Support System</b>, commonly
referred to as <a href="https://grass.osgeo.org">GRASS GIS</a>, is a <a
href="https://en.wikipedia.org/wiki/Geographic_information_system">Geographic
Information System</a> (GIS) used for geospatial data management and
analysis, image processing, graphics/maps production, spatial
modeling, and visualization. GRASS is currently used in academic and
commercial settings around the world, as well as by many governmental
agencies and environmental consulting companies.</p>

<p>This reference manual details the use of modules distributed with
Geographic Resources Analysis Support System (GRASS), an open source
(<a href="https://www.gnu.org/licenses/gpl.html">GNU GPLed</a>), image
processing and geographic information system (GIS).</p>

"""
)
# "

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
    r"""<hr class="header">
<p>
<a href="${index_url}">Main index</a> |
<a href="topics.html">Topics index</a> |
<a href="keywords.html">Keywords index</a> |
<a href="graphical_index.html">Graphical index</a> |
<a href="full_index.html">Full index</a>
</p>
<p>
&copy; 2003-${year}
<a href="https://grass.osgeo.org">GRASS Development Team</a>,
GRASS GIS ${grass_version} Reference Manual
</p>

</div>
</body>
</html>
"""
)
# "

cmd2_tmpl = string.Template(
    r"""<a name="${cmd}"></a>
<h3>${cmd_label} commands (${cmd}.*)</h3>
<table>
"""
)
# "

desc1_tmpl = string.Template(
    r"""<tr><td valign="top"><a href="${cmd}">${basename}</a></td> <td>${desc}</td></tr>
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


full_index_header = r"""
Go <a href="index.html">back to help overview</a>
"""
# "

moduletopics_tmpl = string.Template(
    r"""
<li> <a href="topic_${key}.html">${name}</a></li>
"""
)
# "

headertopics_tmpl = r"""
<link rel="stylesheet" href="grassdocs.css" type="text/css">
</head>
<body style="width: 99%">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
<h2>Topics</h2>
<ul>
"""
# "

headerkeywords_tmpl = r"""
<link rel="stylesheet" href="grassdocs.css" type="text/css">
</head>
<body style="width: 99%">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
<h2>Keywords - Index of GRASS GIS modules</h2>
"""
# "

headerkey_tmpl = string.Template(
    r"""
<link rel="stylesheet" href="grassdocs.css" type="text/css">
</head>
<body bgcolor="white">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">

<h2>Topic: ${keyword}</h2>

<table>
"""
)
# "

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

# TODO: all HTML manual building needs refactoring (perhaps grass.tools?)
header_graphical_index_tmpl = """\
<link rel="stylesheet" href="grassdocs.css" type="text/css">
<style>
.img-list {
    list-style-type: none;
    margin: 0;
    padding: 0;
    text-align: center;
}

.img-list li {
    display: inline-block;
    position: relative;
    width: 8em;
    margin: 0;
    padding: 0.5em;
    margin-bottom: 1em;
}

.img-list li:hover {
    background-color: #eee;
}

.img-list li img {
    float: left;
    max-width: 100%;
    background: white;
}

.img-list li span {
    text-align: center;
}

.img-list li a {
    color: initial;
    text-decoration: none;
}

.img-list li .name {
    margin: 0.1em;
    display: block;
    color: #409940;
    font-weight: bold;
    font-style: normal;
    font-size: 120%;
}
</style>
</head>
<body style="width: 99%">
<div id="container">

<a href="index.html"><img src="grass_logo.png" alt="GRASS logo"></a>
<hr class="header">
<h2>Graphical index of GRASS GIS modules</h2>
"""

############################################################################


def get_desc(cmd):
    f = open(cmd, "r")
    while True:
        line = f.readline()
        if not line:
            return ""
        if "NAME" in line:
            break

    while True:
        line = f.readline()
        if not line:
            return ""
        if "SYNOPSIS" in line:
            break
        if "<em>" in line:
            sp = line.split("-", 1)
            if len(sp) > 1:
                return sp[1].strip()
            else:
                return None

    return ""


############################################################################

man_dir = os.path.join(os.environ["ARCH_DISTDIR"], "docs", "html")

############################################################################
