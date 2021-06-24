#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for interactive display
#            in Jupyter Notebooks.
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU Gernal Public
#            License (>=v2). Read teh file COPYING that comes with GRASS
#            for details.

import folium
import grass.script as gs

class GrassRenderer2:
    """This class will be merged with display.py eventually?"""

    def __init__(self, width=400, height=400):
        """This initiates an instance of GrassRenderer"""
        self.height = height
        self.width = width
        # We still need d.erase because folium uses png overlay
        # for rasters
        gs.run_command("d.erase")
        
    def d_rast(self, raster):
        """This is a wrapper for d.rast that adds a raster to map.png"""
        gs.run_command("d.rast", map=raster)

    def _convert_coordinates(self, coordinates, proj_in):
        """This function reprojects coordinates to WGS84, the required
           projection for folium.
           
           Coordinates should be a string with x and y values separated
           by a comma.
           
           proj_in is a proj4 string, for example, the output of g.region
           with the `g` flag."""
        
        coords_folium = gs.read_command(
                "m.proj",
                coordinates = coordinates,
                proj_in = proj_in,
                separator = "comma",
                flags = "do"
            )
        
        # Reformat from string to array
        coords_folium = coords_folium.strip() # Remove '\n' at end of string
        coords_folium = coords_folium.split(',') #Split on comma
        coords_folium = [float(value) for value in coords_folium] #Convert to floats
        
        return coords_folium[1], coords_folium[0] # Return Lat and Lon
    
    def _get_folium_bounding_box(self):
        """This function returns the bounding box of the current region
        in WGS84, the required projection for folium"""
        
        # Get proj of current GRASS region
        proj = gs.read_command("g.proj", flags="jf")
        
        # Get extent
        extent = gs.parse_command("g.region", flags="g")
        extent_ne = "{}, {}".format(extent['e'], extent['n'])
        extent_sw = "{}, {}".format(extent['w'], extent['s'])
        
        # Convert extent to EPSG:3857, required projection for Folium
        bb_n, bb_e = self._convert_coordinates(extent_ne, proj)
        bb_s, bb_w = self._convert_coordinates(extent_sw, proj)
        
        bounding_box = [[bb_n, bb_w], [bb_s, bb_e]]
        
        return bounding_box
    
    def show_interactively(self, opacity=0.8):
        """This function creates a folium map with a GRASS raster
        overlayed on a basemap"""
        # Get extent and center of GRASS region
        center = gs.parse_command("g.region", flags="cg")
        center = "{}, {}".format(center['center_easting'], center['center_northing'])

        # Get proj of current GRASS region
        proj = gs.read_command("g.proj", flags="jf")

        # Convert center and extent to EPSG:3857 for Folium
        center_folium = self._convert_coordinates(center, proj)
        
        bounding_box = self._get_folium_bounding_box()
        
        # Create Folium Map
        fig = folium.Figure(width = self.width, height = self.height)
        m = folium.Map(
                width = self.width,   # not sure this work work
                height = self.height, 
                location = center_folium,
                tiles = "cartodbpositron"
            )

        # Overlay map.png on folium
        img = folium.raster_layers.ImageOverlay(
                image = 'map.png',
                bounds = bounding_box,
                opacity = opacity,
                interactive = True,
                cross_origin = False,
            )

        # Add img layer to m
        img.add_to(m)
        
        # Add map to figure
        fig.add_child(m)
        
        return fig
