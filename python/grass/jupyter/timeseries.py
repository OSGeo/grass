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


class TimeSeries:
    """timeseries creates visualization of time-space raster and
    vector dataset in Jupyter Notebooks"""

    def __init__(self, timeseries, etype="strds", basemap=None, overlay=None):
        self.timeseries = timeseries
        self.basemap = basemap
        self.overlay = overlay
        self._legend = False
        self._etype = etype  # element type, borrowing convention from tgis
        self._legend_kwargs = None
        self._filenames = []
        self._file_date_dict = {}
        self._date_file_dict = {}

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
            self._dates = (
                gs.read_command(
                    "t.rast.list",
                    input=self.timeseries,
                    columns="start_time",
                    flags="u",
                )
                .strip()
                .split("\n")
            )
        elif self._etype == "stvds":
            self._dates = (
                gs.read_command(
                    "t.vect.list",
                    input=self.timeseries,
                    columns="start_time",
                    flags="u",
                )
                .strip()
                .split("\n")
            )

    def d_legend(self, **kwargs):
        self._legend = True
        self._legend_kwargs = kwargs

    def render_layers(self):
        # Create List of layers to Render
        if self._etype == "strds":
            renderlist = (
                gs.read_command(
                    "t.rast.list", input=self.timeseries, columns="name", flags="u"
                )
                .strip()
                .split("\n")
            )
        elif self._etype == "stvds":
            renderlist = (
                gs.read_command(
                    "t.vect.list", input=self.timeseries, columns="name", flags="u"
                )
                .strip()
                .split("\n")
            )
        else:
            raise NameError(
                _(f"Dataset {self.timeseries} is not element type 'strds' or 'stvds'")
            )

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
            i = i + 1

    def time_slider(self):
        # Lazy Imports
        import ipywidgets as widgets
        from IPython.display import Image

        slider = widgets.SelectionSlider(
            options=self._dates,
            value=self._dates[0],
            description="Date/Time",
            disabled=False,
            continuous_update=True,
            orientation="horizontal",
            readout=True,
            layout=widgets.Layout(width="80%"),
        )

        def view_image(date):
            return Image(self._file_date_dict[date])

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
