import os

import grass.script as gs
import grass.script.setup as gsetup


def set_notebook_defaults(font="sans", renderer="cairo"):
    """
    This function sets several GRASS environment variables that are
    important for GRASS to run smoothly in Jupyter. A complete list and
    description of environment variables can be found here:
    https://grass.osgeo.org/grass78/manuals/variables.html

    Inputs:
        font - specifies the font as either the name of a font from
        $GISBASE/etc/fontcap (or alternative fontcap file specified by
        GRASS_FONT_CAP), or alternatively the full path to a FreeType
        font file.

        renderer - tells the display library which driver to use
                Possible Values: 'cairo', 'png', 'ps', 'html'
    """
    # We want functions to raise exceptions and see standard output of
    # the modules in the notebook.
    gs.set_raise_on_error(True)
    gs.set_capture_stderr(True)

    # Allow overwrite of existing maps
    os.environ["GRASS_OVERWRITE"] = "1"

    # Set display font
    os.environ["GRASS_FONT"] = font

    # Set display modules to render into a file (named map.png by
    # default).
    os.environ["GRASS_RENDER_IMMEDIATE"] = renderer
    os.environ["GRASS_RENDER_FILE_READ"] = "TRUE"


def init(grassdata_filepath, location, mapset, **kwargs):
    """
    This function initiates a GRASS session and sets GRASS
    environment variables.

    Inputs:
        grassdata_filepath - path to grass database
        location - name of GRASS location
        mapset - name of mapset within location
    """
    # Create a GRASS GIS session.
    gsetup.init(gisbase, grassdata_filepath, location, mapset)
    # Set GRASS env. variables
    set_notebook_defaults(**kwargs)
