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
import grass.script as gs
from .display import GrassRenderer


def collect_lyr_dates(timeseries, etype):
    """Create lists of layer names and start_times for a
    space-time raster or vector dataset.

    For datasets with variable timesteps, makes step regular with
    "gran" method for t.rast.list or t.vect.list then fills in
    missing layers with previous timestep layer. If first time step
    is missing, uses the first available layer.

    :param str timeseries: name of space-time dataset
    :param str etype: element type, "stvds" or "strds"
    """
    if etype == "strds":
        rows = gs.read_command(
            "t.rast.list", method="gran", input=timeseries
        ).splitlines()
    elif etype == "stvds":
        rows = gs.read_command(
            "t.vect.list", method="gran", input=timeseries
        ).splitlines()
    else:
        raise NameError(
            _("Dataset {} must be element type 'strds' or 'stvds'").format(timeseries)
        )

    # Parse string
    new_rows = [row.split("|") for row in rows] # split row by pipe separator
    new_array = [list(row) for row in zip(*new_rows)] # transpose into columns where the first value is the name of the column

    # Collect layer name and start time
    for column in new_array:
        if column[0] == "name":
            names = column[1:]
        if column[0] == "start_time":
            dates = column[1:]

    # For variable timestep datasets, fill in None values with
    # previous time step value. If first time step is missing data,
    # use the next non-None value
    for i, name in enumerate(names):
        if name == "None":
            if i > 0:
                names[i] = names[i - 1]
            else:
                search_count = 1
                while name[i + search_count] == "None":
                    search_count += 1
                names[i] = name[i + 1]
        else:
            pass
    return names, dates


class TimeSeries:
    """Creates visualizations of time-space raster and vector datasets in Jupyter
    Notebooks.

    Basic usage::
    >>> img = TimeSeries("series_name")
    >>> img.d_legend() #Add legend
    >>> img.render_layers() #Render Layers
    >>> img.time_slider() #Create TimeSlider
    >>> img.animate()

    This class of grass.jupyter is experimental and under development. The API can
    change at anytime.
    """

    def __init__(self, timeseries, etype="strds", basemap=None, overlay=None):
        """Creates an instance of the TimeSeries visualizations class.

        :param str timeseries: name of space-time dataset
        :param str etype: element type, strds (space-time raster dataset)
                          or stvds (space-time vector dataset)
        :param str basemap: name of raster to use as basemap/background for visuals
        :param str overlay: name of vector to add to visuals
        """
        self.timeseries = timeseries
        self._legend = False
        self._etype = etype  # element type, borrowing convention from tgis
        self._renderlist = []
        self._legend_kwargs = None
        # self._filenames = []
        # self._file_date_dict = {}
        self._date_name_dict = {}

        # Currently does not support multiple basemaps or overlays
        # (i.e. if you wanted to have two vectors rendered, like
        # roads and streams, this isn't possible - you can only have one
        # We should be put a better method here someday
        self.basemap = basemap
        self.overlay = overlay

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

        # create list of layers to render and date/times
        self._renderlist, self._dates = collect_lyr_dates(self.timeseries, self._etype)
        self._date_name_dict = {
            self._dates[i]: self._renderlist[i] for i in range(len(self._dates))
        }

    def d_legend(self, **kwargs):
        """Wrapper for d.legend. Passes keyword arguments to d.legend in render_layers
        ethod.
        """
        self._legend = True
        self._legend_kwargs = kwargs

    def render_layers(self):
        """Renders map for each time-step in space-time dataset and save to PNG
        image in temporary directory.

        Must be run before creating a visualization (i.e. time_slider or animate).

        Can be time-consuming to run with large space-time datasets.
        """

        for name in self._renderlist:
            # Create image file
            filename = os.path.join(self._tmpdir.name, f"{name}.png")

            # Render image
            img = GrassRenderer(filename=filename)
            if self.basemap:
                img.d_rast(map=self.basemap)
            if self._etype == "strds":
                img.d_rast(map=name)
            elif self._etype == "stvds":
                img.d_vect(map=name)
            if self.overlay:
                img.d_vect(map=self.overlay)
            # Add legend if called
            if self._legend:
                info = gs.parse_command("t.info", input="precip_sum", flags="g")
                min_min = info["min_min"]
                max_max = info["max_max"]
                img.d_legend(
                    raster=name, range=f"{min_min}, {max_max}", **self._legend_kwargs
                )

    def time_slider(self, slider_width="60%"):
        """
        Create interactive timeline slider.

        param str slider_width: width of datetime selection slider as a
                                percentage (%) or pixels (px)
        """
        # Lazy Imports
        import ipywidgets as widgets  # pylint: disable=import-outside-toplevel
        from IPython.display import Image  # pylint: disable=import-outside-toplevel

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
            # Look up raster name for date
            name = self._date_name_dict[date]
            filename = os.path.join(self._tmpdir.name, f"{name}.png")
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
        import PIL  # pylint: disable=import-outside-toplevel
        import IPython.display  # pylint: disable=import-outside-toplevel

        # filepath
        if not filename:
            filename = os.path.join(self._tmpdir.name, "image.gif")

        imgs = []
        for date in self._dates:
            name = self._date_name_dict[date]
            img_path = os.path.join(self._tmpdir.name, f"{name}.png")
            img = PIL.Image.open(img_path)
            draw = PIL.ImageDraw.Draw(img)
            if label:
                draw.text(
                    (0, 0),
                    date,
                    fill=text_color,
                    font=PIL.ImageFont.truetype(font, text_size),
                )
            imgs.append(img)

        imgs[0].save(
            fp=filename,
            format="GIF",
            append_images=imgs[1:],
            save_all=True,
            duration=duration,
            loop=0,
        )

        # Display the GIF
        return IPython.display.Image(filename)
