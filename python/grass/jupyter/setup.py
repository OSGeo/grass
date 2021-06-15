# MODULE:    grass.jupyter.setup
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for launching a GRASS session
#           in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.

import os

import grass.script as gs
import grass.script.setup as gsetup


def _set_notebook_defaults():
    """
    This function sets several GRASS environment variables that are
    important for GRASS to run smoothly in Jupyter.

    It also allows GRASS to overwrite existing maps of the same name.
    """
    # We want functions to raise exceptions and see standard output of
    # the modules in the notebook.
    gs.set_raise_on_error(True)
    gs.set_capture_stderr(True)

    # Allow overwrite of existing maps
    os.environ["GRASS_OVERWRITE"] = "1"


def init(path, location, mapset):
    """
    This function initiates a GRASS session and sets GRASS
    environment variables.

    Inputs:
        path - path to grass databases
        location - name of GRASS location
        mapset - name of mapset within location
    """
    # Create a GRASS GIS session.
    gsetup.init(os.environ["GISBASE"], path, location, mapset)
    # Set GRASS env. variables
    _set_notebook_defaults()


def display_settings(font="sans", driver="cairo"):
    """
    This function sets the display settings for a GRASS session
    in Jupyter Notebooks.

    Example Usage: display_settings(font="sans", driver="cairo")

    Inputs:
        font - specifies the font as either the name of a font from
        $GISBASE/etc/fontcap (or alternative fontcap file specified by
        GRASS_FONT_CAP), or alternatively the full path to a FreeType
        font file.

        driver - tell teh display library which driver to use
            Possible values: "cairo", "png", "ps", "html"
    """
    # Set display font
    os.environ["GRASS_FONT"] = font

    # Set display modeules to render to a file (named map.png by
    # default).
    os.environ["GRASS_RENDER_IMMEDIATE"] = driver
    os.environ["GRASS_RENDER_FILE_READ"] = "TRUE"
