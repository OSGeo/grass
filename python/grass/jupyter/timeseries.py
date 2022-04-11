# MODULE:    grass.jupyter.timeseries
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for visualizing raster and vector
#            space-time datasets in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.
"""Create and display visualizations for space-time datasets."""

import tempfile
import os
import weakref
import shutil
import grass.script as gs

from .display import GrassRenderer
from .region import RegionManagerForTimeSeries


def fill_none_values(names):
    """Replace `None` values in array with previous item"""
    for i, name in enumerate(names):
        if name == "None":
            names[i] = names[i - 1]
        else:
            pass
    return names


def collect_layers(timeseries, element_type, fill_gaps):
    """Create lists of layer names and start_times for a
    space-time raster or vector dataset.

    For datasets with variable time steps, makes step regular with
    "gran" method for t.rast.list or t.vect.list then fills in
    missing layers with previous time step layer.

    :param str timeseries: name of space-time dataset
    :param str element_type: element type, "stvds" or "strds"
    :param bool fill_gaps: fill empty time steps with data from previous step
    """
    # NEW WAY: Comment in after json output for t.rast.list and t.vect.list is merged
    # import json
    # if element_type == "strds":
    #     result = json.loads(
    #         gs.read_command(
    #             "t.rast.list", method="gran", input=timeseries, format="json"
    #         )
    #     )
    # elif element_type == "stvds":
    #     result = json.loads(
    #         gs.read_command(
    #             "t.vect.list", method="gran", input=timeseries, format="json"
    #         )
    #     )
    # else:
    #     raise NameError(
    #         _("Dataset {} must be element type 'strds' or 'stvds'").format(timeseries)
    #     )
    #
    # # Get layer names and start time from json
    # names = [item["name"] for item in result["data"]]
    # dates = [item["start_time"] for item in result["data"]]

    if element_type == "strds":
        rows = gs.read_command(
            "t.rast.list", method="gran", input=timeseries
        ).splitlines()
    elif element_type == "stvds":
        rows = gs.read_command(
            "t.vect.list", method="gran", input=timeseries
        ).splitlines()
    else:
        raise NameError(
            _("Dataset {} must be element type 'strds' or 'stvds'").format(timeseries)
        )

    # Parse string
    # Create list of list
    new_rows = [row.split("|") for row in rows]
    # Transpose into columns where the first value is the name of the column
    new_array = [list(row) for row in zip(*new_rows)]

    # Collect layer name and start time
    for column in new_array:
        if column[0] == "name":
            names = column[1:]
        if column[0] == "start_time":
            dates = column[1:]

    # For datasets with variable time steps, fill in gaps with
    # previous time step value, if fill_gaps==True.
    if fill_gaps:
        names = fill_none_values(names)

    return names, dates


class MethodCallCollector:
    """Records lists of GRASS modules calls to hand to GrassRenderer.run().

    Used for base layers and overlays in TimeSeries visualizations."""

    def __init__(self):
        """Create list of GRASS display module calls"""
        self.calls = []

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
            self.calls.append((grass_module, kwargs))

        return wrapper


class TimeSeries:
    """Creates visualizations of time-space raster and vector datasets in Jupyter
    Notebooks.

    Basic usage::

    >>> img = TimeSeries("series_name")
    >>> img.d_legend()  # Add legend
    >>> img.time_slider()  # Create TimeSlider
    >>> img.animate()

    This class of grass.jupyter is experimental and under development. The API can
    change at anytime.
    """

    # pylint: disable=too-many-instance-attributes
    # Need more attributes to build timeseries visuals

    def __init__(
        self,
        timeseries,
        element_type="strds",
        fill_gaps=False,
        env=None,
        use_region=False,
        saved_region=None,
    ):
        """Creates an instance of the TimeSeries visualizations class.

        :param str timeseries: name of space-time dataset
        :param str element_type: element type, strds (space-time raster dataset)
                          or stvds (space-time vector dataset)
        :param bool fill_gaps: fill empty time steps with data from previous step
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

        self.timeseries = timeseries
        self._element_type = element_type
        self._fill_gaps = fill_gaps
        self._bgcolor = "white"
        self._legend = None
        self._baselayers = MethodCallCollector()
        self._overlays = MethodCallCollector()
        self._layers_rendered = False

        self._layers = None
        self._dates = None

        self._date_layer_dict = {}
        self._date_filename_dict = {}

        # create list of layers to render and date/times
        self._layers, self._dates = collect_layers(
            self.timeseries, self._element_type, self._fill_gaps
        )
        self._date_layer_dict = {
            self._dates[i]: self._layers[i] for i in range(len(self._dates))
        }

        # Check that map is time space dataset
        test = gs.read_command("t.list", where=f"name='{timeseries}'")
        if not test:
            raise NameError(
                _(
                    "Could not find space time raster or vector " "dataset named {}"
                ).format(timeseries)
            )

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
        region_manager = RegionManagerForTimeSeries(use_region, saved_region, self._env)
        region_manager.set_region_from_timeseries(self.timeseries)

    @property
    def overlay(self):
        """Add overlay to TimeSeries visualization"""
        self._layers_rendered = False
        return self._overlays

    @property
    def baselayer(self):
        """Add base layer to TimeSeries visualization"""
        self._layers_rendered = False
        return self._baselayers

    def set_background_color(self, color):
        """Set background color of images.

        Passed to d.rast and d.erase. Either a standard color name, R:G:B triplet, or
        Hex. Default is white.

        >>> img = TimeSeries("series_name")
        >>> img.set_background_color("#088B36")  # GRASS GIS green
        >>> img.animate()

        """
        self._bgcolor = color
        self._layers_rendered = False

    def d_legend(self, **kwargs):
        """Display legend.

        Wraps d.legend and uses same keyword arguments.
        """
        self._legend = kwargs
        # If d_legend has been called, we need to re-render layers
        self._layers_rendered = False

    def _render_legend(self, img):
        """Add legend to GrassRenderer instance"""
        info = gs.parse_command(
            "t.info", input=self.timeseries, flags="g", env=self._env
        )
        min_min = info["min_min"]
        max_max = info["max_max"]
        img.d_legend(
            raster=self._layers[0],
            range=f"{min_min}, {max_max}",
            **self._legend,
        )

    def _render_blank_layer(self, filename):
        """Write blank image for gaps in time series.

        Adds overlays and legend to base map.
        """
        img = GrassRenderer(
            filename=filename, use_region=True, env=self._env, read_file=True
        )
        for grass_module, kwargs in self._overlays.calls:
            img.run(grass_module, **kwargs)
        # Add legend if needed
        if self._legend:
            self._render_legend(img)

    def _render_layer(self, layer, filename):
        img = GrassRenderer(
            filename=filename, use_region=True, env=self._env, read_file=True
        )
        if self._element_type == "strds":
            img.d_rast(map=layer)
        elif self._element_type == "stvds":
            img.d_vect(map=layer)
        for grass_module, kwargs in self._overlays.calls:
            img.run(grass_module, **kwargs)
        # Add legend if needed
        if self._legend:
            self._render_legend(img)

    def render(self):
        """Renders image for each time-step in space-time dataset.

        Save PNGs to temporary directory. Must be run before creating a visualization
        (i.e. time_slider or animate). Can be time-consuming to run with large
        space-time datasets.
        """

        # Make base image (background and baselayers)
        # Random name needed to avoid potential conflict with layer names
        random_name_base = gs.append_random("base", 8) + ".png"
        base_file = os.path.join(self._tmpdir.name, random_name_base)
        img = GrassRenderer(
            filename=base_file, use_region=True, env=self._env, read_file=True
        )
        # Fill image background
        img.d_erase(bgcolor=self._bgcolor)
        # Add baselayers
        for grass_module, kwargs in self._baselayers.calls:
            img.run(grass_module, **kwargs)

        # Create name for empty layers
        # Random name needed to avoid potential conflict with layer names
        random_name_none = gs.append_random("none", 8) + ".png"

        # Render each layer
        for date, layer in self._date_layer_dict.items():
            if layer == "None":
                # Create file
                filename = os.path.join(self._tmpdir.name, random_name_none)
                self._date_filename_dict[date] = filename
                # Render blank layer if it hasn't been done already
                if not os.path.exists(filename):
                    shutil.copyfile(base_file, filename)
                    self._render_blank_layer(filename)
            else:
                # Create file
                filename = os.path.join(self._tmpdir.name, f"{layer}.png")
                shutil.copyfile(base_file, filename)
                self._date_filename_dict[date] = filename
                # Render image
                self._render_layer(layer, filename)
        self._layers_rendered = True

    def time_slider(self, slider_width=None):
        """Create interactive timeline slider.

        param str slider_width: width of datetime selection slider

        The slider_width parameter sets the width of the slider in the output cell.
        It should be formantted as a percentage (%) of the cell width or in pixels (px).
        slider_width is passed to ipywidgets in ipywidgets.Layout(width=slider_width).
        """
        # Lazy Imports
        import ipywidgets as widgets  # pylint: disable=import-outside-toplevel
        from IPython.display import Image  # pylint: disable=import-outside-toplevel

        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        # Set default slider width
        if not slider_width:
            slider_width = "60%"

        # Datetime selection slider
        slider = widgets.SelectionSlider(
            options=self._dates,
            value=self._dates[0],
            description=_("Date/Time"),
            disabled=False,
            continuous_update=True,
            orientation="horizontal",
            readout=True,
            layout=widgets.Layout(width=slider_width),
        )

        # Display image associated with datetime
        def view_image(date):
            # Look up layer name for date
            filename = self._date_filename_dict[date]
            return Image(filename)

        # Return interact widget with image and slider
        widgets.interact(view_image, date=slider)

    def animate(
        self,
        duration=500,
        label=True,
        font="DejaVuSans.ttf",
        text_size=12,
        text_color="gray",
        filename=None,
    ):
        """
        Creates a GIF animation of rendered layers.

        Text color must be in a format accepted by PIL ImageColor module. For supported
        formats, visit:
        https://pillow.readthedocs.io/en/stable/reference/ImageColor.html#color-names

        param int duration: time to display each frame; milliseconds
        param bool label: include date/time stamp on each frame
        param str font: font file
        param int text_size: size of date/time text
        param str text_color: color to use for the text.
        param str filename: name of output GIF file
        """
        # Create a GIF from the PNG images
        import PIL.Image  # pylint: disable=import-outside-toplevel
        import PIL.ImageDraw  # pylint: disable=import-outside-toplevel
        import PIL.ImageFont  # pylint: disable=import-outside-toplevel
        import IPython.display  # pylint: disable=import-outside-toplevel

        # Render images if they have not been already
        if not self._layers_rendered:
            self.render()

        # filepath to output GIF
        if not filename:
            filename = os.path.join(self._tmpdir.name, "image.gif")

        images = []
        for date in self._dates:
            img_path = self._date_filename_dict[date]
            img = PIL.Image.open(img_path)
            img = img.convert("RGBA", dither=None)
            draw = PIL.ImageDraw.Draw(img)
            if label:
                draw.text(
                    (0, 0),
                    date,
                    fill=text_color,
                    font=PIL.ImageFont.truetype(font, text_size),
                )
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
        return IPython.display.Image(filename)
