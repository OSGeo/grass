# MODULE:    grass.jupyter.map
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for non-interactive display
#            in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021-2022 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.

"""2D rendering and display functionality"""

import os
import shutil
import tempfile
import weakref

import grass.script as gs

from .region import RegionManagerFor2D


class Map:
    """Map creates and displays GRASS maps in
    Jupyter Notebooks.

    Elements are added to the display by calling GRASS display modules.

    Basic usage::

    >>> m = Map()
    >>> m.run("d.rast", map="elevation")
    >>> m.run("d.legend", raster="elevation")
    >>> m.show()

    GRASS display modules can also be called by using the name of module
    as a class method and replacing "." with "_" in the name.

    Shortcut usage::

    >>> m = Map()
    >>> m.d_rast(map="elevation")
    >>> m.d_legend(raster="elevation")
    >>> m.show()

    """

    def __init__(
        self,
        width=None,
        height=None,
        filename=None,
        env=None,
        font="sans",
        text_size=12,
        renderer="cairo",
        use_region=False,
        saved_region=None,
        read_file=False,
    ):
        """Creates an instance of the Map class.

        :param int height: height of map in pixels
        :param int width: width of map in pixels
        :param str filename: filename or path to save a PNG of map
        :param str env: environment
        :param str font: font to use in rendering; either the name of a font from
                        $GISBASE/etc/fontcap (or alternative fontcap file specified
                        by GRASS_FONT_CAP), or alternatively the full path to a FreeType
                        font file
        :param int text_size: default text size, overwritten by most display modules
        :param renderer: GRASS renderer driver (options: cairo, png, ps, html)
        :param use_region: if True, use either current or provided saved region,
                        else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                        this region is then used for rendering
        :param bool read_file: if False (default), erase filename before re-writing to
                         clear contents. If True, read file without clearing contents
                         first.
        """

        # Copy Environment
        if env:
            self._env = env.copy()
        else:
            self._env = os.environ.copy()
        # Environment Settings
        self._env["GRASS_RENDER_WIDTH"] = str(width) if width else "600"
        self._env["GRASS_RENDER_HEIGHT"] = str(height) if height else "400"
        self._env["GRASS_FONT"] = font
        self._env["GRASS_RENDER_TEXT_SIZE"] = str(text_size)
        self._env["GRASS_RENDER_IMMEDIATE"] = renderer
        self._env["GRASS_RENDER_FILE_READ"] = "TRUE"
        self._env["GRASS_RENDER_TRANSPARENT"] = "TRUE"

        # Create PNG file for map
        # If not user-supplied, we will write it to a map.png in a
        # temporary directory that we can delete later. We need
        # this temporary directory for the legend anyways so we'll
        # make it now
        # Resource managed by weakref.finalize.
        self._tmpdir = (
            tempfile.TemporaryDirectory()  # pylint: disable=consider-using-with
        )

        def cleanup(tmpdir):
            tmpdir.cleanup()

        weakref.finalize(self, cleanup, self._tmpdir)

        if filename:
            self._filename = filename
            if not read_file and os.path.exists(self._filename):
                os.remove(self._filename)
        else:
            self._filename = os.path.join(self._tmpdir.name, "map.png")
        # Set environment var for file
        self._env["GRASS_RENDER_FILE"] = self._filename

        # Create Temporary Legend File
        self._legend_file = os.path.join(self._tmpdir.name, "legend.txt")
        self._env["GRASS_LEGEND_FILE"] = str(self._legend_file)

        # rendering region setting
        self._region_manager = RegionManagerFor2D(
            use_region=use_region,
            saved_region=saved_region,
            width=width,
            height=height,
            env=self._env,
        )

    @property
    def filename(self):
        """Filename or full path to the file with the resulting image.

        The value can be set during initialization. When the filename was not provided
        during initialization, a path to temporary file is returned. In that case, the
        file is guaranteed to exist as long as the object exists.
        """
        return self._filename

    @property
    def region_manager(self):
        """Region manager object"""
        return self._region_manager

    def run(self, module, **kwargs):
        """Run modules from the GRASS display family (modules starting with "d.").

         This function passes arguments directly to grass.script.run_command()
         so the syntax is the same.

        :param str module: name of GRASS module
        :param `**kwargs`: named arguments passed to run_command()"""

        # Check module is from display library then run
        if module[0] != "d":
            msg = "Module must begin with letter 'd'."
            raise ValueError(msg)
        self._region_manager.set_region_from_command(module, **kwargs)
        self._region_manager.adjust_rendering_size_from_region()
        gs.run_command(module, env=self._env, **kwargs)

    def __getattr__(self, name):
        """Parse attribute to GRASS display module. Attribute should be in
        the form 'd_module_name'. For example, 'd.rast' is called with 'd_rast'.
        """

        # Check to make sure format is correct
        if not name.startswith("d_"):
            raise AttributeError(_("Module must begin with 'd_'"))
        # Reformat string
        grass_module = name.replace("_", ".")
        # Assert module exists
        if not shutil.which(grass_module):
            raise AttributeError(_("Cannot find GRASS module {}").format(grass_module))

        def wrapper(**kwargs):
            # Run module
            self.run(grass_module, **kwargs)

        return wrapper

    def show(self):
        """Displays a PNG image of map"""
        # Lazy import to avoid an import-time dependency on IPython.
        from IPython.display import Image, display  # pylint: disable=import-outside-toplevel

        display(Image(self._filename))

    def save(self, filename):
        """Saves a PNG image of map to the specified *filename*"""
        shutil.copy(self._filename, filename)
