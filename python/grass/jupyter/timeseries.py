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


# Probably needs a better name
def parse_csv_str(string):
    return string.strip().splitlines()


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
        :param str etype: element type, strds (space-time raster dataset) or stvds (space-time vector dataset)
        :param str basemap: name of raster to use as basemap/background for visuals
        :param str overlay: name of vector to add to visuals
        """
        self.timeseries = timeseries
        self._legend = False
        self._etype = etype  # element type, borrowing convention from tgis
        self._legend_kwargs = None
        self._filenames = []
        self._file_date_dict = {}
        self._date_file_dict = {}

        # TODO: Improve basemap and overlay method
        # Currently does not support multiple basemaps or overlays
        # (i.e. if you wanted to have two vectors rendered, like
        # roads and streams, this isn't possible - you can only have one
        self.basemap = basemap
        self.overlay = overlay

        # Check that map is time space dataset
        test = gs.read_command("t.list", where=f"name LIKE '{timeseries}'")
        if not test:
            raise NameError(
                _(
                    f"Could not find space time raster or vector"
                    f"dataset named {timeseries}"
                )
            )

        # Create a temporary directory for our PNG images
        self._tmpdir = tempfile.TemporaryDirectory()

        # create list of date/times
        if self._etype == "strds":
            self._dates = parse_csv_str(
                gs.read_command(
                    "t.rast.list",
                    input=self.timeseries,
                    columns="start_time",
                    flags="u",
                )
            )
        elif self._etype == "stvds":
            self._dates = parse_csv_str(
                gs.read_command(
                    "t.vect.list",
                    input=self.timeseries,
                    columns="start_time",
                    flags="u",
                )
            )

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
        # Create List of layers to Render
        if self._etype == "strds":
            renderlist = parse_csv_str(
                gs.read_command(
                    "t.rast.list", input=self.timeseries, columns="name", flags="u"
                )
            )
        elif self._etype == "stvds":
            renderlist = parse_csv_str(
                gs.read_command(
                    "t.vect.list", input=self.timeseries, columns="name", flags="u"
                )
            )
        else:
            raise NameError(
                _(f"Dataset {self.timeseries} is not element type 'strds' or 'stvds'")
            )

        # Start counter for matching datetime stamp with filename
        i = 0
        for name in renderlist:
            # Create image file
            filename = os.path.join(self._tmpdir.name, "{}.png".format(name))
            self._filenames.append(filename)
            self._file_date_dict[self._dates[i]] = filename
            self._date_file_dict[filename] = self._dates[i]
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
            # Add 1 to counter
            i = i + 1

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
            return Image(self._file_date_dict[date])

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
        for f in self._filenames:
            date = self._date_file_dict[f]
            img = Image.open(f)
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
