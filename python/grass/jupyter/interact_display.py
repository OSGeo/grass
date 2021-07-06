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

import os
import folium
import grass.script as gs


class InteractiveMap():
    """This class creates interative GRASS maps with folium"""

    def __init__(self, width=400, height=400):
        """This initiates a folium map centered on g.region"""
        # Store height and width
        self.width=width
        self.height=height
        
        # Get the parent location/mapset
        self._rc_original = self._get_rc(os.environ)
        
        # Create new environment for tmp WGS84 location
        rcfile, self._env = gs.create_environment(self._rc_original["GISDBASE"], "temp_folium_WGS84", "PERMANENT")       
        
        # Location and mapset and region
        gs.create_location(self._rc_original["GISDBASE"], "temp_folium_WGS84", epsg="4326", overwrite=True)
        self._rc_tmp = self._get_rc(self._env) # Get tmp location/mapset
        self._extent = self._convert_extent(env=os.environ) # Get the extent of the original area in WGS84
        
        # Set region to match original region extent
        gs.run_command("g.region",
                       n=self._extent["north"],
                       s=self._extent["south"],
                       e=self._extent["east"],
                       w=self._extent["west"],
                       env=self._env
                      )
        
        # Get Center of tmp GRASS region
        center = gs.parse_command("g.region", flags="cg", env=self._env)
        center = (float(center["center_northing"]), float(center["center_easting"]))

        # Create Folium Map 
        self.map = folium.Map(
            width=self.width,
            height=self.height,
            location=center,
            tiles="cartodbpositron",
        )

        
    def _get_rc(self, env=None):
        rc = {}
        
        if env:
            rcfile_location = env["GISRC"]
        else:
            rcfile_location = os.environ["GISRC"]
            
        with open(rcfile_location) as f:
            for line in f:
                entry = line.strip("\n").split(": ")
                rc[entry[0]] = entry[1]
        return rc
    
    

    def _convert_coordinates(self, x, y, proj_in):
        """This function reprojects coordinates to WGS84, the required
        projection for folium.

        Coordinates should be a string with x and y values separated
        by a comma.

        proj_in is a proj4 string, for example, the output of g.region
        with the `g` flag."""
        # Reformat input
        coordinates = f"{x}, {y}"
        
        # Reproject coordinates
        coords_folium = gs.read_command(
            "m.proj",
            coordinates=coordinates,
            proj_in=proj_in,
            separator="comma",
            flags="do",
        )

        # Reformat from string to array
        coords_folium = coords_folium.strip()  # Remove '\n' at end of string
        coords_folium = coords_folium.split(",")  # Split on comma
        coords_folium = [float(value) for value in coords_folium]  # Convert to floats

        return coords_folium[1], coords_folium[0]  # Return Lat and Lon

    def _convert_extent(self, env=None):
        """This function returns the bounding box of the current region
        in WGS84, the required projection for folium"""

        # Get proj of current GRASS region
        proj = gs.read_command("g.proj", flags="jf", env=env)

        # Get extent
        extent = gs.parse_command("g.region", flags="g", env=env)

        # Convert extent to EPSG:3857, required projection for Folium
        north, east = self._convert_coordinates(extent["e"], extent["n"], proj)
        south, west = self._convert_coordinates(extent["w"], extent["s"], proj)

        extent = {'north': north, 'south': south, 'east': east, 'west': west}

        return extent
    
    def _folium_bounding_box(self, extent):
        """Reformats extent into bounding box to pass to folium"""
        return [[extent['north'], extent['west']], [extent['south'], extent['east']]]
    
    def add_vector(self, vector):
        """Imports vector into temporary WGS84 location, 
        re-formats to a GeoJSON and adds to folium map"""
        
        # Import vector into new Location/Mapset
        name = gs.find_file(vector, element="vector")["fullname"]
        # Reproject vector into WGS84 Location
        gs.run_command("v.proj",
                       input=name,
                       location=self._rc_original["LOCATION_NAME"],
                       mapset=self._rc_original["MAPSET"],
                       env=self._env
                      )
        
        # Convert to GeoJSON
        json_file = f"tmp_{vector}.json"

        gs.run_command("v.out.ogr", input=name, output=json_file, format="GeoJSON", env=self._env)
        
        folium.GeoJson(json_file, name=vector).add_to(self.map)
    
    def show(self):
        """This function creates a folium map with a GRASS raster
        overlayed on a basemap"""
        fig = folium.Figure(width=self.width, height=self.height)
        
        # Add Layer Control
        folium.LayerControl().add_to(self.map)
        
        # Add map to figure
        fig.add_child(self.map)
        
        return fig
