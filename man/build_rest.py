#!/usr/bin/env python

"""
Created on Thu Aug  9 14:04:12 2012

@author: lucadelu
"""
# utilities for generating REST indices
# utilities for generating HTML indices
# (c) 2003-2006, 2009-2012 by the GRASS Development Team, Markus Neteler, Glynn Clements, Luca Delucchi

import sys
import os
import string

## TODO: better fix this in include/Make/Rest.make, see bug RT #5361

# exclude following list of modules from help index:

exclude_mods = [
    "i.find",
    "r.watershed.ram",
    "r.watershed.seg",
    "v.topo.check",
    "helptext.html"]

# these modules don't use G_parser()

desc_override = {
    "g.parser": "Provides automated parser, GUI, and help support for GRASS scipts.",
    "r.li.daemon": "Support module for r.li landscape index calculations."
    }

############################################################################

header2_tmpl = string.Template(\
r"""
==================================================================
GRASS GIS ${grass_version} Reference Manual
==================================================================
.. figure:: grass_logo.png
   :align: center
   :alt: GRASS logo

GRASS GIS ${grass_version} Reference Manual
--------------------------------------------------------------------

**Geographic Resources Analysis Support System**, commonly
referred to as `GRASS <http://grass.osgeo.org>`_, is a `Geographic
Information System <http://en.wikipedia.org/wiki/Geographic_information_system>`_ 
(GIS) used for geospatial data management and analysis, image processing, 
graphics/maps production, spatial modeling, and visualization. GRASS is 
currently used in academic and commercial settings around the world, as 
well as by many governmental agencies and environmental consulting companies.

This reference manual details the use of modules distributed with
Geographic Resources Analysis Support System (GRASS), an open source
(`GNU GPLed <http://www.gnu.org/licenses/gpl.html>`_), image
processing and geographic information system (GIS).
""")

overview_tmpl = string.Template(\
r"""
Quick Introduction
~~~~~~~~~~~~~~~~~~~~

.. toctree::
    :maxdepth: 1

	How to start with GRASS <helptext.html>
        Intro projections and spatial transformations <projectionintro>
        Intro 2D raster map processing <rasterintro>
        Intro 3D raster map (voxel) processing <raster3dintro>
        Intro image processing <imageryintro>
        Intro vector map processing and network analysis <vectorintro>
        Intro database management <databaseintro>
        Intro temporal data processing <temporalintro>

Display/Graphical User Interfaces
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
.. toctree::
    :maxdepth: 1
        
        wxGUI wxPython-based GUI frontend <wxGUI>
        Display commands manual <display>
        Display drivers <displaydrivers>
        nviz 3D visualization and animation tool <wxGUI.Nviz>
        xganim tool for animating a raster map series <xganim>


Raster and 3D raster processing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. toctree::
    :maxdepth: 1
    
        Raster commands manual <raster>
        3D raster (voxel) commands manual <raster3D>

Image processing
~~~~~~~~~~~~~~~~~~

.. toctree::
    :maxdepth: 1
    
         Imagery commands manual <imagery>
         
         

Vector processing
~~~~~~~~~~~~~~~~~~

.. toctree::
    :maxdepth: 1
    
        Vector commands manual <vector>
        GRASS ASCII vector format specification <vectorascii>
        
Database
~~~~~~~~~

.. toctree::
    :maxdepth: 1
    
        SQL support in GRASS GIS <sql>
        Database commands manual <database>

General
~~~~~~~~~

.. toctree::
    :maxdepth: 1
    
        GRASS startup manual page <grass7>
        General commands manual <general>

Miscellaneous & Variables
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. toctree::
    :maxdepth: 1
    
        Miscellaneous commands manual <misc>
        GRASS variables and environment variables <variables>

Temporal processing
~~~~~~~~~~~~~~~~~~~~

.. toctree::
    :maxdepth: 1
    
        Temporal commands manual <temporal>

Printing
~~~~~~~~~~~~

.. toctree::
    :maxdepth: 1
    
        Postscript commands manual <postscript>

""")

#TODO add copyright symbol
footer_tmpl = string.Template(\
r"""

--------------

:doc:`Help Index <index>` \| :doc:`Full Index <full_index>`
 2003-2012 `GRASS Development Team <http://grass.osgeo.org>`_, GRASS GIS ${grass_version} Reference Manual
""")

cmd1_tmpl = string.Template(\
r"""*`$cmd.\* <${cmd}>` *""")

cmd2_tmpl = string.Template(\
r"""

${cmd}.* commands:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. toctree::
    :maxdepth: 1
    

""")

desc1_tmpl = string.Template(\
r"""        ${basename} - ${desc} <${basename}>
""")

sections = \
r""" 

+-----------------------------+-------------------------------+
|`d.* <full_index.html#d>`_   | `display commands`            |
+-----------------------------+-------------------------------+
|`db.* <full_index.html#db>`_ | `database commands`           |
+-----------------------------+-------------------------------+
|`g.* <full_index.html#g>`_   | `general commands`            |
+-----------------------------+-------------------------------+
|`i.* <full_index.html#i>`_   | `imagery commands`            |
+-----------------------------+-------------------------------+
|`m.* <full_index.html#m>`_   | `miscellaneous commands`      |
+-----------------------------+-------------------------------+
|`ps.* <full_index.html#ps>`_ | `postscript commands`         |
+-----------------------------+-------------------------------+
|`r.* <full_index.html#r>`_   | `raster commands`             |
+-----------------------------+-------------------------------+
|`r3.* <full_index.html#r3>`_ | `raster3D commands`           |
+-----------------------------+-------------------------------+
|`t.* <full_index.html#t>`_   | `temporal commands`           |
+-----------------------------+-------------------------------+
|`v.* <full_index.html#v>`_   | `vector commands`             |
+-----------------------------+-------------------------------+
|`nviz <wxGUI.Nviz.html>`_    | `visualization suite`         |
+-----------------------------+-------------------------------+
|`wxGUI <wxGUI.html>`_        | `wxPython-based GUI frontend` |
+-----------------------------+-------------------------------+
|`xganim <xganim.html>`_      | `raster map slideshow`        |
+-----------------------------+-------------------------------+

"""

modclass_intro_tmpl = string.Template(\
r"""Go to :doc:`${modclass} introduction <${modclass_lower}intro>`
""")
#"

modclass_tmpl = string.Template(\
r"""Go :doc:`back to help overview<index>`



**${modclass} commands:**

.. toctree::
    :maxdepth: 1
    

""")
#"

desc2_tmpl = string.Template(\
r"""        ${basename} - ${desc} <${basename}>
""")
#"


full_index_header = \
r"""Go :doc:`back to help overview<index>`


Full command index:
~~~~~~~~~~~~~~~~~~~~
"""
#"


message_tmpl = string.Template(\
r"""Generated HTML docs in ${rest_dir}/index.txt
----------------------------------------------------------------------
Following modules are missing the 'description.txt' file in src code:
""")

def check_for_desc_override(basename):
    return desc_override.get(basename)

def read_file(name):
    f = open(name, 'rb')
    s = f.read()
    f.close()
    return s

def write_file(name, contents):
    f = open(name, 'wb')
    f.write(contents)
    f.close()

def try_mkdir(path):
    try:
        os.mkdir(path)
    except OSError, e:
        pass

def replace_file(name):
    temp = name + ".tmp"
    if os.path.exists(name) and os.path.exists(temp) and read_file(name) == read_file(temp):
        os.remove(temp)
    else:
        try:
            os.remove(name)
        except OSError, e:
            pass
        os.rename(temp, name)

def copy_file(src, dst):
    write_file(dst, read_file(src))

def rest_files(cls = None):
    for cmd in sorted(os.listdir(rest_dir)):
        if cmd.endswith(".txt") and \
           (cls in [None, '*'] or cmd.startswith(cls + ".")) and \
           (cls != '*' or len(cmd.split('.')) >= 3) and \
           cmd not in ["full_index.txt", "index.txt"] and \
           cmd not in exclude_mods and \
           not cmd.startswith("wxGUI."):
            yield cmd

def write_rest_header(f, title, ismain = False):
    f.write(header2_tmpl.substitute(grass_version = grass_version))

def write_rest_cmd_overview(f):
    box_color = "#e1ecd0"
    f.write(overview_tmpl.substitute(box_color = box_color))

def write_rest_footer(f, index_url):
    f.write(footer_tmpl.substitute(grass_version = grass_version,
                                   index_url = index_url))

def get_desc(cmd):
    f = open(cmd, 'r')
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
        if "*" in line:
            sp = line.split('-',1)
            if len(sp) > 1:
                return sp[1].strip()
            else:
                return None

    return ""

############################################################################

arch_dist_dir = os.environ['ARCH_DISTDIR']
rest_dir = os.path.join(arch_dist_dir, "docs", "rest")
gisbase = os.environ['GISBASE']
ver = read_file(os.path.join(gisbase, "etc", "VERSIONNUMBER"))
try:
    grass_version = ver.split()[0].strip()
except IndexError:
    grass_version = ver.split().strip()

############################################################################
