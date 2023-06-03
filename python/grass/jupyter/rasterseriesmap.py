# MODULE:    grass.jupyter.rasterseriesmap
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for visualizing series of rasters in
#            Jupyter Notebooks
#
# COPYRIGHT: (C) 2022 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.
"""Create and display visualizations for a series of rasters."""

import tempfile
import os
import weakref
import shutil
from pathlib import Path

import grass.script as gs

from .map import Map
from .region import RegionManagerFor2D


def is_raster(layer):
    """Check that map is a raster"""
    test = gs.read_command("r.info", map=layer)
    if not test:
        raise NameError(_("Could not find a raster named {}").format(layer))


class RasterSeriesMap:
    """Creates visualizations of raster datasets in Jupyter
    Notebooks.

    Basic usage::

    >>> img = RasterSeriesMap()
    >>> img.add_rasters(maps=["elev", "precip", "temp"])
    >>> img.show()  # Create Slider
    >>> img.save("image.gif")

    This class of grass.jupyter is experimental and under development. The API can
    change at anytime.
    """

    # pylint: disable=too-many-instance-attributes

    def __init__(
        self,
        width=None,
        height=None,
        env=None,
        use_region=False,
        saved_region=None,
    ):
        """Creates an instance of the RasterSeriesMap visualizations class.

        :param int width: width of map in pixels
        :param int height: height of map in pixels
        :param str env: environment
        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        """

        # Copy Environment
        if env:
            self._env = env.copy()
        else:
            self._env = os.environ.copy()

        self.rasters = []
        self._base_layer_calls = []
        self._overlay_calls = []
        self._rasters_added = False
        self._layers_rendered = False
        self._layers = None
        self._dates = None
        self._layer_filename_dict = {}
        self._width = width
        self._height = height

        # Create a temporary directory for our PNG images
        # Resource managed by weakref.finalize.
        self._tmpdir = (
            # pylint: disable=consider-using-with
            tempfile.TemporaryDirectory()
        )

        def cleanup(tmpdir):
            tmpdir.cleanup()

        weakref.finalize(self, cleanup, self._tmpdir)

        # Handle Regions
        self._region_manager = RegionManagerFor2D(
            use_region=use_region,
            saved_region=saved_region,
            width=width,
            height=height,
            env=self._env,
        )

    def add_rasters(self, maps):
        """
        :param str maps: list of rasters
        """
        for raster in maps:
            is_raster(raster)
        self.rasters = maps
        self._rasters_added = True
        # Update Region: just use first raster in series, this should be improved
        self._region_manager.set_region_from_command("d.rast", map=self.rasters[0])

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
            if not self._rasters_added:
                self._base_layer_calls.append((grass_module, kwargs))
            if self._rasters_added:
                self._overlay_calls.append((grass_module, kwargs))

        return wrapper

    def _render_baselayers(self, img):
        """Add collected baselayers to Map instance"""
        for grass_module, kwargs in self._base_layer_calls:
            img.run(grass_module, **kwargs)

    def _render_overlays(self, img):
        """Add collected overlays to Map instance"""
        for grass_module, kwargs in self._overlay_calls:
            img.run(grass_module, **kwargs)

    def _render_blank_layer(self, filename):
        """Write blank image for gaps in time series.

        Adds overlays and legend to base map.
        """
        img = Map(
            width=self._width,
            height=self._height,
            filename=filename,
            use_region=True,
            env=self._env,
            read_file=True,
        )
        # Add overlays
        self._render_overlays(img)
        # Add legend if needed
        if self._legend:
            self._render_legend(img)

    def _render_layer(self, layer, filename):
        """Render layer to file with overlays and legend"""
        img = Map(
            width=self._width,
            height=self._height,
            filename=filename,
            use_region=True,
            env=self._env,
            read_file=True,
        )
        img.d_rast(map=layer)
        # Add overlays
        self._render_overlays(img)

    def render(self):
        """Renders image for each raster in series.

        Save PNGs to temporary directory. Must be run before creating a visualization
        (i.e. show or save).
        """

        if not self._rasters_added:
            raise RuntimeError(
                "Cannot render rasters series since none has been added."
                "Use RasterSeriesMap.add_rasters() to add rasters"
            )

        # Make base image (background and baselayers)
        # Random name needed to avoid potential conflict with layer names
        random_name_base = gs.append_random("base", 8) + ".png"
        base_file = os.path.join(self._tmpdir.name, random_name_base)
        img = Map(
            width=self._width,
            height=self._height,
            filename=base_file,
            use_region=True,
            env=self._env,
            read_file=True,
        )
        # We have to call d_erase to ensure the file is created. If there are no
        # base layers, then there is nothing to render in random_base_name
        img.d_erase()
        # Add baselayers
        self._render_baselayers(img)

        # Create name for empty layers
        # Random name needed to avoid potential conflict with layer names
        # A new random_name_none is created each time the render function is run,
        # and any existing random_name_none file will be ignored
        random_name_none = gs.append_random("none", 8) + ".png"

        # Render each layer
        for layer in self.rasters:
            # Create file
            filename = os.path.join(self._tmpdir.name, f"{layer}.png")
            # Copying the base_file ensures that previous results are overwritten
            shutil.copyfile(base_file, filename)
            self._layer_filename_dict[layer] = filename
            # Render image
            self._render_layer(layer, filename)
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

        # Datetime selection slider
        slider = widgets.SelectionSlider(
            options=self._dates,
            value=self._dates[0],
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
            max=len(self._dates) - 1,
            step=1,
            description="Press play",
            disabled=False,
        )
        out_img = widgets.Image(value=b"", format="png")

        def change_slider(change):
            slider.value = slider.options[change.new]

        play.observe(change_slider, names="value")

        # Display image associated with datetime
        def change_image(layer):
            # Look up layer name for date
            filename = self._layer_filename_dict[layer]
            with open(filename, "rb") as rfile:
                out_img.value = rfile.read()

        # Return interact widget with image and slider
        widgets.interactive_output(change_image, {"layer": slider})
        layout = widgets.Layout(
            width="100%", display="inline-flex", flex_flow="row wrap"
        )
        return widgets.HBox([play, slider, out_img], layout=layout)

    def save(self, filename, duration=500):
        """
        Creates a GIF animation of rendered layers.

        Text color must be in a format accepted by PIL ImageColor module. For supported
        formats, visit:
        https://pillow.readthedocs.io/en/stable/reference/ImageColor.html#color-names

        param str filename: name of output GIF file
        param int duration: time to display each frame; milliseconds
        param bool label: include date/time stamp on each frame
        param str font: font file
        param int text_size: size of date/time text
        param str text_color: color to use for the text.
        """
        # Create a GIF from the PNG images
        import PIL.Image  # pylint: disable=import-outside-toplevel
        import PIL.ImageDraw  # pylint: disable=import-outside-toplevel
        import PIL.ImageFont  # pylint: disable=import-outside-toplevel

        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        # filepath to output GIF
        filename = Path(filename)
        if filename.suffix.lower() != ".gif":
            raise ValueError(_("filename must end in '.gif'"))

        images = []
        for layer in self.rasters:
            img_path = self._layer_filename_dict[layer]
            img = PIL.Image.open(img_path)
            img = img.convert("RGBA", dither=None)
            PIL.ImageDraw.Draw(img)
            images.append(img)

        images[0].save(
            fp=filename,
            format="GIF",
            append_images=images[1:],
            save_all=True,
            duration=duration,
            loop=0,
        )

        # Display the GIF
        return filename
