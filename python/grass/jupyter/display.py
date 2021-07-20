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

    def __getattr__(self, name, **kwargs):
        """Parse attribute to GRASS display module. Attribute should be in
        the form 'd_{module}'.

        :param str name: display module in form 'd_{module}'
        :param kwargs: arguments to be passed to display module

        :return: output of display module

        ----- Example -----
        # Create map instance
        >>> m = GrassRenderer()
        # Add elevation raster with shortcut
        >>> m.d_rast(map="elevation")
        # Add legend with shortcut
        >>> m.d_legend(raster="elevation")
        # Show map
        >>> m.show()
        -------------------
        """

        # First check to make sure format is correct
        if not name.startswith("d_"):
            raise AttributeError("Module must begin with 'd_'.")
        # Now reformat string
        grass_module = f"d.{name[2:]}"

        def wrapper(**kwargs):
            # And try to run module
            try:
                self.run(grass_module, **kwargs)
            except FileNotFoundError as e:
                custom_message = f"Could not find GRASS module '{grass_module}'."
                raise FileNotFoundError(custom_message) from e

        return wrapper

    def show(self):
        """Displays a PNG image of the map (non-interactive)"""
        return Image(self._filename)
