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
import grass.script as gs

from .region import RegionManagerFor2D, RegionManagerFor3D
import ipywidgets as widgets


class timeseries:
    """timeseries creates visualization of time-space raster and
    vector dataset in Jupyter Notebooks"""

    def __init__(self, map, basemap, overlay):
        self.map = map
        self.basemap = basemap
        self.overlay = overlay
        # Check that map ends with tsrds or tsvds

        # self.type == type #tsrds or tsvds

    def render_layers(self):
        if self.type == "tsrds":
            lyrList = gs.read_command("t.rast.list", input=self.map)
        if self.type == "tsvds":
            lyrList = gs.read_command("t.vect.list", input=self.map)

        lyrList = str(lyrList)
        lyrList = lyrList.split("\r\n")

        names = []
        for line in lyrList[1:-1]:
            line = line.split("|")
            names.append(line[0])

        # TODO: add tempdirectory here for filenames
        filenames = []
        for name in names:
            filename = "{}.png".format(name)
            filenames.append(filename)
            img = gj.GrassRenderer(filename=filename)
            if self.background:
                img.d_rast(map=self.background)
            if self.type == "tsrds":
                img.d_rast(map=name)
            if self.type == "tsvds":
                img.d._vect(map=name)
            if self.overlay:
                img.d_vect(map=self.overlay)

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


