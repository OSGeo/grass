#
# AUTHOR(S): Riya Saxena <29riyasaxena AT gmail>
#
# PURPOSE:   This module provides the base class for interactive visualizations
#            used by `TimeSeriesMap` and `SeriesMap` in Jupyter Notebooks. It
#            includes methods for rendering visualizations and creating interactive
#            sliders to navigate through time-series or series data, while reducing
#            redundancy and enhancing functionality.
#
# COPYRIGHT: (C) 2024 Riya Saxena, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Base class for SeriesMap and TimeSeriesMap"""

import os
from pathlib import Path
import tempfile
import weakref
import shutil
import multiprocessing

import grass.script as gs

from .map import Map
from .utils import get_number_of_cores, save_gif


class BaseSeriesMap:
    """
    Base class for SeriesMap and TimeSeriesMap
    """

    def __init__(self, width=None, height=None, env=None):
        """Creates an instance of the visualizations class.

        :param int width: width of map in pixels
        :param int height: height of map in pixels
        :param str env: environment
        """

        # Copy Environment
        if env:
            self._env = env.copy()
        else:
            self._env = os.environ.copy()

        self._base_layer_calls = []
        self._calls = []
        self._baseseries_added = False
        self._layers_rendered = False
        self._base_filename_dict = {}
        self._width = width
        self._height = height
        self._slider_description = ""
        self._labels = []
        self._indices = []
        self.base_file = None

        # Create a temporary directory for our PNG images
        # Resource managed by weakref.finalize.
        self._tmpdir = (
            # pylint: disable=consider-using-with
            tempfile.TemporaryDirectory()
        )

        def cleanup(tmpdir):
            tmpdir.cleanup()

        weakref.finalize(self, cleanup, self._tmpdir)

    def __getattr__(self, name):
        """
        Parse attribute to GRASS display module. Attribute should be in
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
        # if this function is called, the images need to be rendered again
        self._layers_rendered = False

        def wrapper(**kwargs):
            if not self._baseseries_added:
                self._base_layer_calls.append((grass_module, kwargs))
            else:
                for row in self._calls:
                    row.append((grass_module, kwargs))

        return wrapper

    def _render_baselayers(self, img):
        """Add collected baselayers to Map instance"""
        for grass_module, kwargs in self._base_layer_calls:
            img.run(grass_module, **kwargs)

    def _render_worker(self, i):
        """Function to render a single layer."""
        filename = os.path.join(self._tmpdir.name, f"{i}.png")
        shutil.copyfile(self.base_file, filename)
        img = Map(
            width=self._width,
            height=self._height,
            filename=filename,
            use_region=True,
            env=self._env,
            read_file=True,
        )
        for grass_module, kwargs in self._calls[i]:
            if grass_module is not None:
                img.run(grass_module, **kwargs)
        return self._indices[i], filename

    def render(self):
        """Renders image for each raster in series.

        Save PNGs to temporary directory. Must be run before creating a visualization
        (i.e. show or save).
        """
        if not self._baseseries_added:
            msg = (
                "Cannot render series since none has been added."
                "Use SeriesMap.add_rasters() or SeriesMap.add_vectors()"
            )
            raise RuntimeError(msg)
        tasks = [(i,) for i in range(len(self._indices))]

        # Make base image (background and baselayers)
        # Random name needed to avoid potential conflict with layer names
        random_name_base = gs.append_random("base", 8) + ".png"
        self.base_file = os.path.join(self._tmpdir.name, random_name_base)
        img = Map(
            width=self._width,
            height=self._height,
            filename=self.base_file,
            use_region=True,
            env=self._env,
            read_file=True,
        )
        # We have to call d_erase to ensure the file is created. If there are no
        # base layers, then there is nothing to render in random_base_name
        img.d_erase()
        # Add baselayers
        self._render_baselayers(img)

        # Render layers in respective classes
        cores = get_number_of_cores(len(tasks), env=self._env)
        with multiprocessing.Pool(processes=cores) as pool:
            results = pool.starmap(self._render_worker, tasks)

        for i, filename in results:
            self._base_filename_dict[i] = filename

        self._layers_rendered = True

    def show(self, slider_width=None):
        """Create interactive timeline slider.

        param str slider_width: width of datetime selection slider

        The slider_width parameter sets the width of the slider in the output cell.
        It should be formatted as a percentage (%) between 0 and 100 of the cell width
        or in pixels (px). Values should be formatted as strings and include the "%"
        or "px" suffix. For example, slider_width="80%" or slider_width="500px".
        slider_width is passed to ipywidgets in ipywidgets.Layout(width=slider_width).
        """
        # Lazy Imports
        import ipywidgets as widgets  # pylint: disable=import-outside-toplevel

        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        # Set default slider width
        if not slider_width:
            slider_width = "70%"

        lookup = list(zip(self._labels, self._indices, strict=False))
        description = self._slider_description  # modify description

        # Datetime selection slider
        slider = widgets.SelectionSlider(
            options=lookup,
            value=self._indices[0],
            description=description,
            disabled=False,
            continuous_update=True,
            orientation="horizontal",
            readout=True,
            layout=widgets.Layout(width=slider_width),
        )
        play = widgets.Play(
            interval=500,
            value=0,
            min=0,
            max=len(self._labels) - 1,
            step=1,
            description="Press play",
            disabled=False,
        )
        out_img = widgets.Image(value=b"", format="png")

        def change_slider(change):
            slider.value = slider.options[change.new][1]

        play.observe(change_slider, names="value")

        # Display image associated with datetime
        def change_image(index):
            filename = self._base_filename_dict[index]
            out_img.value = Path(filename).read_bytes()

        widgets.interactive_output(change_image, {"index": slider})

        layout = widgets.Layout(
            width="100%", display="inline-flex", flex_flow="row wrap"
        )
        return widgets.HBox([play, slider, out_img], layout=layout)

    def save(
        self,
        filename,
        duration=500,
        label=True,
        font=None,
        text_size=12,
        text_color="gray",
    ):
        """
        Creates a GIF animation of rendered layers.

        Text color must be in a format accepted by PIL ImageColor module. For supported
        formats, visit:
        https://pillow.readthedocs.io/en/stable/reference/ImageColor.html#color-names

        param str filename: name of output GIF file
        param int duration: time to display each frame; milliseconds
        param bool label: include label on each frame
        param str font: font file
        param int text_size: size of label text
        param str text_color: color to use for the text.
        """

        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        input_files = [self._base_filename_dict[index] for index in self._indices]

        save_gif(
            input_files,
            filename,
            duration=duration,
            label=label,
            labels=self._labels,
            font=font,
            text_size=text_size,
            text_color=text_color,
        )

        # Display the GIF
        return filename
