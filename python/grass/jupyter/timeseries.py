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

import tempfile
import os
import grass.script as gs
from .display import GrassRenderer


def collect_lyr_dates(timeseries, etype):
    """Create lists of layer names and start_times for a
    space-time raster or vector dataset.

    For datasets with variable timesteps, makes step regular with
    "gran" method for t.rast.list or t.vect.list then fills in
    missing layers with previous timestep layer. If first time step
    is missing, uses the first available layer.
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
    new_rows = [row.split("|") for row in rows]
    new_array = [list(row) for row in zip(*new_rows)]

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

    print(names, dates)
    return names, dates


class TimeSeries:
    """Creates visualizations of time-space raster and
    vector datasets in Jupyter Notebooks

    Basic usage::
    >>> img = TimeSeries("series_name")
    >>> img.d_legend() #Add legend
    >>> img.render_layers() #Render Layers
    >>> img.time_slider() #Create TimeSlider
    >>> img.animate()

    This class of grass.jupyter is experimental and under development.
    The API can change at anytime.
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

        # TODO: Improve basemap and overlay method
        # Currently does not support multiple basemaps or overlays
        # (i.e. if you wanted to have two vectors rendered, like
        # roads and streams, this isn't possible - you can only have one
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
        self._tmpdir = tempfile.TemporaryDirectory()

        # create list of layers to render and date/times
        self._renderlist, self._dates = collect_lyr_dates(self.timeseries, self._etype)
        self._date_name_dict = {
            self._dates[i]: self._renderlist[i] for i in range(len(self._dates))
        }

    def d_legend(self, **kwargs):
        """Wrapper for d.legend. Passes keyword arguments to d.legend in
        render_layers method.
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
            filename = os.path.join(self._tmpdir.name, "{}.png".format(name))
            # self._filenames.append(filename)

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
        import ipywidgets as widgets
        from IPython.display import Image

        # Datetime selection slider
        slider = widgets.SelectionSlider(
            options=self._dates,
            value=self._dates[0],
            description="Date/Time",
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
            filename = os.path.join(self._tmpdir.name, "{}.png".format(name))
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
    ):
        """
        param int duration: time to display each frame; milliseconds
        param bool label: include date/time stamp on each frame
        param str font: font file
        param int text_size: size of date/time text
        param str text_color: color to use for the text. See
                              https://pillow.readthedocs.io/en/stable/reference/ImageColor.html#color-names
                              for list of available color formats
        """
        # Create a GIF from the PNG images
        from PIL import Image
        from PIL import ImageFont
        from PIL import ImageDraw
        from IPython.display import Image as ipyImage

        # filepath
        fp_out = os.path.join(self._tmpdir.name, "image.gif")

        imgs = []
        for date in self._dates:
            name = self._date_name_dict[date]
            filename = os.path.join(self._tmpdir.name, "{}.png".format(name))
            img = Image.open(filename)
            draw = ImageDraw.Draw(img)
            if label:
                font_settings = ImageFont.truetype(font, text_size)
                draw.text((0, 0), date, fill=text_color, font=font_settings)
            imgs.append(img)

        img.save(
            fp=fp_out,
            format="GIF",
            append_images=imgs[:-1],
            save_all=True,
            duration=duration,
            loop=0,
        )

        # Display the GIF
        return ipyImage(fp_out)
