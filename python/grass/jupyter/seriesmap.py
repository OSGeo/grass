# MODULE:    grass.jupyter.seriesmap
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
from .region import RegionManagerForSeries


def is_raster(layer):
    """Check that map is a raster"""
    test = gs.read_command("r.info", map=layer)
    if not test:
        raise NameError(_("Could not find a raster named {}").format(layer))


def is_vector(layer):
    """Check that map is a vector"""
    test = gs.read_command("v.info", map=layer)
    if not test:
        raise NameError(_("Could not find a vector named {}".format(layer)))


class SeriesMap:
    """Creates visualizations of raster datasets in Jupyter
    Notebooks.

    Basic usage::

    >>> series = gj.SeriesMap(height = 500)
    >>> series.add_rasters(["elevation_shade", "geology", "soils"])
    >>> series.add_vectors(["streams", "streets", "viewpoints"])
    >>> series.d_barscale()
    >>> series.show()  # Create Slider
    >>> series.save("image.gif")

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
        """Creates an instance of the SeriesMap visualizations class.

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

        self._series_length = None
        self._base_layer_calls = []
        self._calls = []
        self._series_added = False
        self._layers_rendered = False
        self._layer_filename_dict = {}
        self._names = []
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
        self._region_manager = RegionManagerForSeries(
            use_region=use_region,
            saved_region=saved_region,
            width=width,
            height=height,
            env=self._env,
        )

    def add_rasters(self, rasters, **kwargs):
        """
        :param list rasters: list of raster layers to add to SeriesMap
        """
        for raster in rasters:
            is_raster(raster)
        # Update region to rasters if not use_region or saved_region
        self._region_manager.set_region_from_rasters(rasters)
        if self._series_added:
            assert self._series_length == len(rasters), _(
                "Number of vectors in series must match number of vectors"
            )
            for i in range(self._series_length):
                kwargs["map"] = rasters[i]
                self._calls[i].append(("d.rast", kwargs.copy()))
        else:
            self._series_length = len(rasters)
            for raster in rasters:
                kwargs["map"] = raster
                self._calls.append([("d.rast", kwargs.copy())])
            self._series_added = True
        if not self._names:
            self._names = rasters
        self._layers_rendered = False

    def add_vectors(self, vectors, **kwargs):
        """
        :param list vectors: list of vector layers to add to SeriesMap
        """
        for vector in vectors:
            is_vector(vector)
        # Update region extent to vectors if not use_region or saved_region
        self._region_manager.set_region_from_vectors(vectors)
        if self._series_added:
            assert self._series_length == len(vectors), _(
                "Number of rasters in series must match number of vectors"
            )
            for i in range(self._series_length):
                kwargs["map"] = vectors[i]
                self._calls[i].append(("d.vect", kwargs.copy()))
        else:
            self._series_length = len(vectors)
            for vector in vectors:
                kwargs["map"] = vector
                self._calls.append([("d.vect", kwargs.copy())])
            self._series_added = True
        if not self._names:
            self._names = vectors
        self._layers_rendered = False

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
        self._layers_rendered

        def wrapper(**kwargs):
            if not self._series_added:
                self._base_layer_calls.append((grass_module, kwargs))
            else:
                [row.append((grass_module, kwargs)) for row in self._calls]

        return wrapper

    def add_names(self, names):
        """Add list of names associated with layers.
        Default will be names of first series added."""
        assert self._series_length == len(names), _(
            "Number of vectors in series must match number of vectors"
        )
        self._names = names

    def _render_baselayers(self, img):
        """Add collected baselayers to Map instance"""
        for grass_module, kwargs in self._base_layer_calls:
            img.run(grass_module, **kwargs)

    def _render_layer(self, img, calls):
        """Add collected calls to Map instance"""
        for grass_module, kwargs in calls:
            img.run(grass_module, **kwargs)

    def render(self):
        """Renders image for each raster in series.

        Save PNGs to temporary directory. Must be run before creating a visualization
        (i.e. show or save).
        """

        if not self._series_added:
            raise RuntimeError(
                "Cannot render series since none has been added."
                "Use SeriesMap.add_rasters() or SeriesMap.add_vectors()"
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

        # Render each layer
        for i in range(self._series_length):
            # Create file
            filename = os.path.join(self._tmpdir.name, f"{i}.png")
            # Copying the base_file ensures that previous results are overwritten
            shutil.copyfile(base_file, filename)
            self._layer_filename_dict[i] = filename
            # Render image
            img = Map(
                width=self._width,
                height=self._height,
                filename=filename,
                use_region=True,
                env=self._env,
                read_file=True,
            )
            self._render_layer(img, self._calls[i])
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

        # Create lookup table for slider
        lookup = list(zip(self._names, range(self._series_length)))

        # Datetime selection slider
        slider = widgets.SelectionSlider(
            options=lookup,
            value=0,
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
            max=self._series_length - 1,
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
            # Look up layer name for date
            filename = self._layer_filename_dict[index]
            with open(filename, "rb") as rfile:
                out_img.value = rfile.read()

        # Return interact widget with image and slider
        widgets.interactive_output(change_image, {"index": slider})
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

        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        # filepath to output GIF
        filename = Path(filename)
        if filename.suffix.lower() != ".gif":
            raise ValueError(_("filename must end in '.gif'"))

        images = []
        for i in range(self._series_length):
            img_path = self._layer_filename_dict[i]
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
