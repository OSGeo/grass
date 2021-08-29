# MODULE:    grass.jupyter.display
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   This module contains functions for non-interactive display
#            in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021 Vaclav Petras, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.

import os
import tempfile

import grass.script as gs
from grass.jupyter import GrassRenderer


class Grass3dRenderer:
    """Creates and displays 3D visualization using GRASS GIS 3D rendering engine NVIZ.

    The 3D image is created using the *render* function. Additional images can be
    placed on the image using the *overlay* attribute which is the 2D renderer, i.e.,
    has interface of the *GrassRenderer* class.

    Basic usage::
    >>> m = Grass3dRenderer()
    >>> img.render(elevation_map="elevation", color_map="elevation", perspective=20)
    >>> img.overlay.d_legend(raster="elevation", at=(60, 97, 87, 92))
    >>> m.show()
    """

    def __init__(
        self,
        width=600,
        height=400,
        filename=None,
        mode="fine",
        resolution_fine=1,
        screen_backend="auto",
        font="sans",
        text_size=12,
        renderer2d="cairo",
    ):
        """Creates an instance of the GrassRenderer class.

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
        """
        self._width = width
        self._height = height
        self._mode = mode
        self._resolution_fine = resolution_fine

        self._tmpdir = tempfile.TemporaryDirectory()
        if filename:
            self._filename = filename
        else:
            self._filename = os.path.join(self._tmpdir.name, "map.png")

        try:
            # This tests availability of the module and needs to work even
            # when the package is not installed.
            import pyvirtualdisplay  # pylint: disable=import-outside-toplevel

            pyvirtualdisplay_available = True
        except ImportError:
            pyvirtualdisplay_available = False
        if screen_backend == "auto" and pyvirtualdisplay_available:
            self._screen_backend = pyvirtualdisplay
        elif screen_backend == "pyvirtualdisplay" and not pyvirtualdisplay_available:
            raise ValueError(
                _(
                    "Screen backend '{}' cannot be used "
                    "because pyvirtualdisplay cannot be imported"
                ).format(screen_backend)
            )
        elif screen_backend == "simple":
            self._screen_backend = screen_backend
        else:
            raise ValueError(
                _(
                    "Screen backend '{}' does not exist. "
                    "See documentation for the list of supported backends."
                ).format(screen_backend)
            )

        self.overlay = GrassRenderer(
            height=height,
            width=width,
            filename=self._filename,
            font=font,
            text_size=text_size,
            renderer=renderer2d,
        )

    def render(self, **kwargs):
        """Run rendering using *m.nviz.image*.

        Keyword arguments are passed as parameters to the *m.nviz.image* module.
        Parameters set in constructor such as *mode* are used here unless another value
        is provided. Parameters related to size, file, and format are handled
        internally and will be ignored when passed here.

        For the OpenGL rendering to work, a display is needed. This is not guaranteed
        on headless systems such as continuous integration (CI) or Binder service(s).
        When pyvirtualdisplay Python package is present, the function assumes that it
        """
        module = "m.nviz.image"
        name = os.path.join(self._tmpdir.name, "nviz")
        ext = "tif"
        full_name = f"{name}.{ext}"
        kwargs["output"] = name
        kwargs["format"] = ext
        kwargs["size"] = (self._width, self._height)
        if "mode" not in kwargs:
            kwargs["mode"] = self._mode
        if "resolution_fine" not in kwargs:
            kwargs["resolution_fine"] = self._resolution_fine

        if self._screen_backend == "pyvirtualdisplay":
            # This is imported only when needed and when the package is available,
            # but generally, it may not be available.
            # pylint: disable=import-outside-toplevel,import-error
            from pyvirtualdisplay import Display

            with Display(
                size=(self._width, self._height), manage_global_env=False
            ) as display:
                gs.run_command(module, env=display.env(), **kwargs)
        else:
            gs.run_command(module, **kwargs)

        # Lazy import to avoid an import-time dependency on PIL.
        from PIL import Image  # pylint: disable=import-outside-toplevel

        img = Image.open(full_name)
        img.save(self._filename)

    def show(self):
        """Displays a PNG image of map"""
        # Lazy import to avoid an import-time dependency on IPython.
        from IPython.display import Image  # pylint: disable=import-outside-toplevel

        return Image(self._filename)
