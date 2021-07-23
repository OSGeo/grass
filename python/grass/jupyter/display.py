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
import shutil
from pathlib import Path
from IPython.display import Image
import grass.script as gs


class GrassRenderer:
    """GrassRenderer creates and displays GRASS maps in
    Jupyter Notebooks.
    
    Elements are added to the display by calling GRASS display modules.

    Basic usage::
    >>> m = GrassRenderer()
    >>> m.run("d.rast", map="elevation")
    >>> m.run("d.legend", raster="elevation")
    >>> m.show()
    
    GRASS display modules can also be called by using the name of module
    as a class method and replacing "." with "_" in the name.
    
    Shortcut usage::
    >>> m = GrassRenderer()
    >>> m.d_rast(map="elevation")
    >>> m.d_legend(raster="elevation")
    >>> m.show()
    """

    def __init__(
        self, env=None, width=600, height=400, filename="map.png", text_size=12
    ):
        """Creates an instance of the GrassRenderer class.
        
        :param int height: height of map in pixels
        :param int width: width of map in pixels
        :param str filename: filename or path to save a PNG of map
        :param str env: environment
        :param int text_size: default text size, overwritten by most display modules
        :param renderer: GRASS renderer driver (options: cairo, png, ps, html)
        """

        if env:
            self._env = env.copy()
        else:
            self._env = os.environ.copy()

        self._env["GRASS_RENDER_WIDTH"] = str(width)
        self._env["GRASS_RENDER_HEIGHT"] = str(height)
        self._env["GRASS_RENDER_TEXT_SIZE"] = str(text_size)
        self._env["GRASS_RENDER_IMMEDIATE"] = "cairo"
        self._env["GRASS_RENDER_FILE"] = str(filename)
        self._env["GRASS_RENDER_FILE_READ"] = "TRUE"

        self._legend_file = Path(filename).with_suffix(".grass_vector_legend")
        self._env["GRASS_LEGEND_FILE"] = str(self._legend_file)

        self._filename = filename

        self.run("d.erase")

    def run(self, module, **kwargs):
        """Run modules from "d." GRASS library"""
        # Check module is from display library then run
        if module[0] == "d":
            gs.run_command(module, env=self._env, **kwargs)
        else:
            raise ValueError("Module must begin with letter 'd'.")

    def __getattr__(self, name):
        """Parse attribute to GRASS display module. Attribute should be in
        the format 'd_{module}'.
        """

        # Check to make sure format is correct
        if not name.startswith("d_"):
            raise AttributeError(_("Module must begin with 'd_'"))
        # Reformat string
        grass_module = name.replace("_", ".")
        # Assert module exists
        if not shutil.which(grass_module):
            raise AttributeError(_("Grass module {} does not exist").format(grass_module))

        def wrapper(**kwargs):
            # Run module
            self.run(grass_module, **kwargs)

        return wrapper

    def show(self):
        """Displays a PNG image of the map"""
        return Image(self._filename)
