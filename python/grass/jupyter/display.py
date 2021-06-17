
# MODULE:    grass.jupyter.display
#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for non-interactive display
#            in Jupyter Notebooks
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#           This program is free software under the GNU General Public
#           License (>=v2). Read the file COPYING that comes with GRASS
#           for details.

import os
import folium
import grass.script as gs

class grassRenderer:
    """
    The grassRenderer class creates and displays GRASS maps in Jupyter
    Notebooks. 
    """

    def __init__(self, width=600, height=400, text_size=12):
        self.height = height
        self.width = width
        os.environ["GRASS_RENDER_WIDTH"] = width
        os.environ["GRASS_RENDER_HEIGHT"] = height
        os.environ["GRASS_TEXT_SIZE"] = text_size
        gs.run_command("d.erase")


    def d_rast(self, raster, color="greys"):
        """
        Adds a raster to the display
        """
        gs.run_command("r.colors", map=raster, color=color)
        gs.run_command("d.rast", map=raster)


    def d_vect(self, vector, fill="grey", border_color="black", width=2):
        """
        Adds a vector to the display
        """
        gs.run_command('d.vect', map=vector, fill_color='none', border_color="black", width=2)


    def show(self):
        """
        Displays a png image of the map (non-interactive)
        """
        from IPython.display import Image
        return Image("map.png")


    # def show_interactively(self, opacity=0.8):
    #     # Get extent and center of GRASS region
    #     extent = gs.read_command("g.region", flags="e")
    #     center = gs.read_command("g.region", flags="c")
        
    #     # Get proj of current GRASS region
    #     proj = gs.read_command("g.proj", flags="jf")

    #     # Create target proj (EPSG:3857)
    #     proj_merc = "EPSG:3857"

    #     # Convert center and extent to EPSG:3857 for Folium
    #     extent_merc = gs.parse_command(
    #         "m.proj",
    #         input=extent,
    #         proj_in=proj,
    #         proj_out=proj_merc
    #     )
        
    #     center_merc = gs.parse_command(
    #         "m.proj",
    #         coordinates=center,
    #         proj_in=proj,
    #         proj_out=proj_merc
    #     )
        
    #     # Create Folium Map
    #     m = folium.Map(
    #         width = self.width,   # not sure this will work to set height
    #         height = self.height, # https://www.analyticsvidhya.com/blog/2020/06/guide-geospatial-analysis-folium-python/
    #         location = [center_merc[0], center_merc[1]],
    #         tiles="cartodbpositron"
    #     )

    #     # Overlay map.png on folium
    #     img = folium.raster_layers.ImageOverlay(
    #             image='map.png',
    #             bounds=[[extent_merc[0], extent_merc[1]], [extent_merc[2], extent_merc[3]]],
    #             opacity=opacity,
    #             interactive=True,
    #             cross_origin=False,
    #         )
    #     # Add img layer to m
    #     img.add_to(m)

    #     return m
