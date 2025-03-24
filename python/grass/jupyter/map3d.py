# MODULE:    grass.jupyter.display
#
# AUTHOR(S): Vaclav Petras <wenzeslaus gmail com>
#
# PURPOSE:   This module contains functions for non-interactive display
#            in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021-2022 Vaclav Petras, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.

"""Render 3D visualizations"""

from __future__ import annotations

import os
import tempfile
import weakref

import grass.script as gs

from .map import Map
from .region import RegionManagerFor3D


class Map3D:
    """Creates and displays 3D visualization using GRASS GIS 3D rendering engine NVIZ.

    The 3D image is created using the *render* function which uses the *m.nviz.image*
    module in the background. Additional images can be
    placed on the image using the *overlay* attribute which is the 2D renderer, i.e.,
    has interface of the *Map* class.

    Basic usage::

    >>> img = Map()
    >>> img.render(elevation_map="elevation", color_map="elevation", perspective=20)
    >>> img.overlay.d_legend(raster="elevation", at=(60, 97, 87, 92))
    >>> img.show()

    For the OpenGL rendering with *m.nviz.image* to work, a display (screen) is needed.
    This is not guaranteed on headless systems such as continuous integration (CI) or
    Binder service(s). This class uses Xvfb and PyVirtualDisplay to support rendering
    in these environments.
    """

    def __init__(
        self,
        width: int = 600,
        height: int = 400,
        filename: str | None = None,
        mode: str = "fine",
        resolution_fine: int = 1,
        screen_backend: str = "auto",
        font: str = "sans",
        text_size: float = 12,
        renderer2d: str = "cairo",
        use_region: bool = False,
        saved_region: str | None = None,
    ):
        """Checks screen_backend and creates a temporary directory for rendering.

        :param width: width of image in pixels
        :param height: height of image in pixels
        :param filename: filename or path to save the resulting PNG image
        :param mode: 3D rendering mode (options: fine, coarse, both)
        :param resolution_fine: resolution multiplier for the fine mode
        :param screen_backend: backend for running the 3D rendering
        :param font: font to use in 2D rendering
        :param text_size: default text size in 2D rendering, usually overwritten
        :param renderer2d: GRASS 2D renderer driver (options: cairo, png)
        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering

        When *resolution_fine* is 1, rasters are used in the resolution according
        to the computational region as usual in GRASS GIS.
        Setting *resolution_fine* to values higher than one, causes rasters to
        be resampled to a coarser resolution (2 for twice as coarse than computational
        region resolution). This allows for fast rendering of large rasters without
        changing the computational region.

        By default (``screen_backend="auto"``), when
        pyvirtualdisplay Python package is present, the class assumes that it is
        running in a headless environment, so pyvirtualdisplay is used. When the
        package is not present, *m.nviz.image* is executed directly. When
        *screen_backend* is set to ``"pyvirtualdisplay"`` and the package cannot be
        imported, ValueError is raised. When *screen_backend* is set to ``"simple"``,
        *m.nviz.image* is executed directly. For other values of *screen_backend*,
        ValueError is raised.
        """
        self._width = width
        self._height = height
        self._mode = mode
        self._resolution_fine = resolution_fine

        # Temporary dir and files
        # Resource managed by weakref.finalize.
        self._tmpdir = (
            tempfile.TemporaryDirectory()  # pylint: disable=consider-using-with
        )

        def cleanup(tmpdir):
            tmpdir.cleanup()

        weakref.finalize(self, cleanup, self._tmpdir)

        if filename:
            self._filename = filename
        else:
            self._filename = os.path.join(self._tmpdir.name, "map.png")

        # Screen backend
        try:
            # This tests availability of the module and needs to work even
            # when the package is not installed.
            # pylint: disable=import-outside-toplevel,unused-import
            import pyvirtualdisplay  # noqa: F401

            pyvirtualdisplay_available = True
        except ImportError:
            pyvirtualdisplay_available = False
        if screen_backend == "auto" and pyvirtualdisplay_available:
            self._screen_backend = "pyvirtualdisplay"
        elif screen_backend == "auto":
            self._screen_backend = "simple"
        elif screen_backend == "pyvirtualdisplay" and not pyvirtualdisplay_available:
            raise ValueError(
                _(
                    "Screen backend '{}' cannot be used "
                    "because pyvirtualdisplay cannot be imported"
                ).format(screen_backend)
            )
        elif screen_backend in {"simple", "pyvirtualdisplay"}:
            self._screen_backend = screen_backend
        else:
            raise ValueError(
                _(
                    "Screen backend '{}' does not exist. "
                    "See documentation for the list of supported backends."
                ).format(screen_backend)
            )

        self.overlay = Map(
            height=height,
            width=width,
            filename=self._filename,
            font=font,
            text_size=text_size,
            renderer=renderer2d,
            use_region=use_region,
            saved_region=saved_region,
        )
        # rendering region setting
        self._region_manager = RegionManagerFor3D(use_region, saved_region)

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

    def render(self, **kwargs):
        """Run rendering using *m.nviz.image*.

        Keyword arguments are passed as parameters to the *m.nviz.image* module.
        Parameters set in constructor such as *mode* are used here unless another value
        is provided. Parameters related to size, file, and format are handled
        internally and will be ignored when passed here.

        Calling this function again, overwrites the previously rendered image,
        so typically, it is called only once.
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
            import inspect  # pylint: disable=import-outside-toplevel

            # This is imported only when needed and when the package is available,
            # but generally, it may not be available.
            # pylint: disable=import-outside-toplevel,import-error
            from pyvirtualdisplay import Display

            additional_kwargs = {}
            has_env_copy = False
            if "manage_global_env" in inspect.signature(Display).parameters:
                additional_kwargs["manage_global_env"] = False
                has_env_copy = True
            with Display(
                size=(self._width, self._height), **additional_kwargs
            ) as display:
                env = display.env() if has_env_copy else os.environ.copy()
                self._region_manager.set_region_from_command(env=env, **kwargs)
                self.overlay.region_manager.set_region_from_env(env)
                gs.run_command(module, env=env, **kwargs)
        else:
            env = os.environ.copy()
            self._region_manager.set_region_from_command(env=env, **kwargs)
            self.overlay.region_manager.set_region_from_env(env)
            gs.run_command(module, env=env, **kwargs)

        # Lazy import to avoid an import-time dependency on PIL.
        from PIL import Image  # pylint: disable=import-outside-toplevel

        img = Image.open(full_name)
        img.save(self._filename)

    def show(self):
        """Displays a PNG image of map"""
        # Lazy import to avoid an import-time dependency on IPython.
        from IPython.display import Image, display  # pylint: disable=import-outside-toplevel

        display(Image(self._filename))
