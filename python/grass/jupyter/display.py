# MODULE:    grass.jupyter.display
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for non-interactive display
#            in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.

import os
import grass.script as gs
from IPython.display import Image


class GrassRenderer:
    """The grassRenderer class creates and displays GRASS maps in
    Jupyter Notebooks."""

    def __init__(self, width=600, height=400, text_size=12):
        """Initiates an instance of the GrassRenderer class."""
        os.environ["GRASS_RENDER_WIDTH"] = str(width)
        os.environ["GRASS_RENDER_HEIGHT"] = str(height)
        os.environ["GRASS_TEXT_SIZE"] = str(text_size)
        gs.run_command("d.erase")

    def d_rast(self, raster, **kwargs):
        """Adds a raster to the display"""
        # gs.run_command("r.colors", map=raster, color=color)
        gs.run_command("d.rast", map=raster, **kwargs)

    def d_vect(self, vector, **kwargs):
        """Adds a vector to the display"""
        gs.run_command("d.vect", map=vector, **kwargs)

    def show(self):
        """Displays a PNG image of the map (non-interactive)"""
        return Image("map.png")
