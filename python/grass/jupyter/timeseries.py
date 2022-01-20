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
#from grass.animation import temporal_manager as tm
#import grass.temporal as tgis

from .region import RegionManagerFor2D, RegionManagerFor3D
import ipywidgets as widgets


class timeseries:
    """timeseries creates visualization of time-space raster and
    vector dataset in Jupyter Notebooks"""

    def __init__(self, timeseries, type="strds", basemap=None, overlay=None):
        self.timeseries = timeseries
        self.basemap = basemap
        self.overlay = overlay
        self.etype = type # element type - convention from temporal manager

        # Check that map is time space dataset
        #self.validateTimeseriesName()
        test = gs.read_command("t.list", where=f"name LIKE '{timeseries}'")
        if not test:
             raise AttributeError(_(f"Could not find space time raster or vector dataset named {timeseries}"))

        # Create a temporary directory for our PNG images
        self._tmpdir = tempfile.TemporaryDirectory()
        print(self._tmpdir.name)

    #def d_legend(self, **kwargs):
        # this function should construct a legend command that can
        # be called in the render_layers loop below

    # THIS IS FROM gui/wxpython/animation/util.py but I couldn't figure out how
    # to import that...
    # CANNOT IMPORT grass.temporal
    # def validateTimeseriesName(self):
    #     """Check if space time dataset exists and completes missing mapset.
    #
    #     Raises GException if dataset doesn't exist."""
    #     trastDict = tgis.tlist_grouped(self.etype)
    #     if self.timeseries.find("@") >= 0:
    #         nameShort, mapset = self.timeseries.split("@", 1)
    #         if nameShort in trastDict[mapset]:
    #             return self.timeseries
    #         else:
    #             raise GException(_(f"Space time dataset {self.timeseries} not found."))
    #
    #     mapsets = tgis.get_tgis_c_library_interface().available_mapsets()
    #     for mapset in mapsets:
    #         if mapset in trastDict.keys():
    #             if self.timeseries in trastDict[mapset]:
    #                 return self.timeseries + "@" + mapset
    #     raise GException(_(f"Space time dataset {self.timeseries} not found."))

    def render_layers(self):
        # NOT SURE IF THIS WILL ALWAYS WORK
        # The maps key is found in the command history of the t.info output
        # If, somehow, there was no "t.register" with map list in the command history,
        # I think this would fail.
        renderlist = gs.parse_command("t.info", input=self.timeseries, flags="h")["maps"][1:-1].split(",")

        filenames = []
        for name in renderlist:
            filename= os.path.join(self._tmpdir.name, "{}.png".format(name))
            filenames.append(filename)
            img = GrassRenderer(filename=filename)
            if self.basemap:
                img.d_rast(map=self.basemap)
            # THIS IS A BAD WAY OF DOING THIS
            try:
                img.d_rast(map=name)
            except CalledModuleError:
                img.d._vect(map=name)
            if self.overlay:
                img.d_vect(map=self.overlay)
            # THIS SHOULD CHANGE WITH d_legend completion
            info = gs.parse_command("t.info", input="precip_sum", flags="g")
            min_min = info["min_min"]
            max_max = info["max_max"]
            img.d_legend(raster=name, range=f"{min_min}, {max_max}")

    def timeslider(self):

        # create list of date/times for labels
        dates = []

        # Create slider
        slider = widgets.SelectionSlider(
            options=dates,
            value=dates,
            continuous_update=False,
            disabled=False
        )

        output_map = widgets.Output()

        def react_with_slider(change):
            output_map.clear_output(wait=True)
            with output_map:
                draw_map(change.new)

        slider.observe(react_with_slider, names='value')

        display(output_map)
        display(slider)


