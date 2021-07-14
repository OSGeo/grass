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
import tempfile
import grass.script as gs


class GrassRenderer:
    """The grassRenderer class creates and displays GRASS maps in
    Jupyter Notebooks."""

    def __init__(
        self, env=None, width=600, height=400, filename=None, text_size=12
    ):
        """Initiates an instance of the GrassRenderer class."""
        # Copy Environment
        if env:
            self._env = env.copy()
        else:
            self._env = os.environ.copy()
        # Create PNG file for map
        # If not user-supplied, create temporary file
        if filename:
            self._env["GRASS_RENDER_FILE"] = filename
        else:
            # Make temporary file
            tmpfile = tempfile.NamedTemporaryFile(suffix=".png")
            self._env["GRASS_RENDER_FILE"] = tmpfile.name
        self._filename = tmpfile.name
        # Environment Settings
        self._env["GRASS_RENDER_WIDTH"] = str(width)
        self._env["GRASS_RENDER_HEIGHT"] = str(height)
        self._env["GRASS_TEXT_SIZE"] = str(text_size)
        self._env["GRASS_RENDER_IMMEDIATE"] = "cairo"
        self._env["GRASS_RENDER_FILE_READ"] = "TRUE"
        # Temporary Legend File
        self._legend_file = tempfile.NamedTemporaryFile()
        self._env["GRASS_LEGEND_FILE"] = str(self._legend_file.name)

    def run(self, module, **kwargs):
        """Run modules from "d." GRASS library"""
        # Check module is from display library then run
        if module[0] == "d":
            gs.run_command(module, env=self._env, **kwargs)
        else:
            raise ValueError("Module must begin with letter 'd'.")

    def show(self):
        """Displays a PNG image of the map (non-interactive)"""
        return Image(self._filename)
