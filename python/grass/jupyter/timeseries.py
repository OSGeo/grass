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
import datetime
import grass.script as gs
from .display import GrassRenderer
#from grass.animation import temporal_manager as tm
#import grass.temporal as tgis

from .region import RegionManagerFor2D, RegionManagerFor3D



class timeseries:
    """timeseries creates visualization of time-space raster and
    vector dataset in Jupyter Notebooks"""

    def __init__(self, timeseries, type="strds", basemap=None, overlay=None):
        self.timeseries = timeseries
        self.basemap = basemap
        self.overlay = overlay
        self._legend = False
        self.type = type
        self._legend_kwargs = None
        self._filenames = None

        # Check that map is time space dataset
        test = gs.read_command("t.list", where=f"name LIKE '{timeseries}'")
        if not test:
             raise NameError(_(f"Could not find space time raster or vector dataset named {timeseries}"))

        # Check datatype

        # Create a temporary directory for our PNG images
        self._tmpdir = tempfile.TemporaryDirectory()
        print(self._tmpdir.name)

    def d_legend(self, **kwargs):
        self._legend = True
        self._legend_kwargs = kwargs


    def render_layers(self):
        if self.type == "strds":
            renderlist = gs.read_command("t.rast.list", input=self.timeseries, columns="name", flags="u").strip().split("\n")
        elif self.type == "stvds":
            renderlist = gs.read_command("t.vect.list", input=self.timeseries, columns="name", flags="u").strip().split("\n")
        else:
            raise NameError(_(f"Dataset {self.timeseries} is not data type 'strds' or 'stvds'"))

        filenames = []
        for name in renderlist:
            filename= os.path.join(self._tmpdir.name, "{}.png".format(name))
            filenames.append(filename)
            img = GrassRenderer(filename=filename)
            if self.basemap:
                img.d_rast(map=self.basemap)
            if self.type == "strds":
                print(name)
                img.d_rast(map=name)
            elif self.type == "stvds":
                img.d_vect(map=name)
            if self.overlay:
                img.d_vect(map=self.overlay)
            # Add legend if called
            if self._legend:
                info = gs.parse_command("t.info", input="precip_sum", flags="g")
                min_min = info["min_min"]
                max_max = info["max_max"]
                img.d_legend(raster=name, range=f"{min_min}, {max_max}", **self._legend_kwargs)

        self._filenames = filenames

    def timeslider(self):
        # Lazy Imports
        import ipywidgets as widgets
        from IPython.display import Image

        # create list of date/times for labels
        if self.type == "strds":
            dates = gs.read_command("t.rast.list", input=self.timeseries, columns="start_time", flags="u").strip().split("\n")
        elif self.type == "stvds":
            dates = gs.read_command("t.vect.list", input=self.timeseries, columns="start_time", flags="u").strip().split("\n")

        value_dict = {dates[i]: self._filenames[i] for i in range(len(dates))}

        def view_image(date):
            return Image(value_dict[date])

        widgets.interact(view_image, date=dates)

