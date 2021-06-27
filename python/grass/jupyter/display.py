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
from pathlib import Path
from IPython.display import Image
import grass.script as gs


class GrassRenderer:
    """The grassRenderer class creates and displays GRASS maps in
    Jupyter Notebooks."""

    def __init__(
        self, env=None, width=600, height=400, filename="map.png", text_size=12
    ):
        """Initiates an instance of the GrassRenderer class."""

        if env is None:
            os.environ["GRASS_RENDER_WIDTH"] = str(width)
            os.environ["GRASS_RENDER_HEIGHT"] = str(height)
            os.environ["GRASS_TEXT_SIZE"] = str(text_size)
            os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"
            os.environ["GRASS_RENDER_FILE"] = filename
            os.environ["GRASS_RENDER_FILE_READ"] = "TRUE"
            self._legend_file = Path(filename).with_suffix(".grass_vector_legend")
            os.environ["GRASS_LEGEND_FILE"] = str(self._legend_file)
            self._env = os.environ.copy()
        else:
            self._env = os.environ.copy()
            self._env["GRASS_RENDER_WIDTH"] = str(width)
            self._env["GRASS_RENDER_HEIGHT"] = str(height)
            self._env["GRASS_TEXT_SIZE"] = str(text_size)
            self._legend_file = Path(filename).with_suffix(".grass_vector_legend")
            self._env["GRASS_LEGEND_FILE"] = str(self._legend_file)

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
