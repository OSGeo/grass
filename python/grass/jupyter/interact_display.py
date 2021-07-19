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
from pathlib import Path

import folium

import grass.script as gs


class InteractiveMap:
    """This class creates interative GRASS maps with folium"""

    def __init__(self, width=400, height=400):
        """This initiates a folium map centered on g.region.

        Keyword arguments:
            height -- height in pixels of figure (default 400)
            width -- width in pixels of figure (default 400)"""

        # Store height and width
        self.width = width
        self.height = height
        # Make temporary folder for all our files
        self.tmp_dir = Path("./tmp/")
        try:
            os.mkdir(self.tmp_dir)
        except FileExistsError:
            pass
        # Create new environment for tmp WGS84 location
        rcfile, self._vector_env = gs.create_environment(
            self.tmp_dir, "temp_folium_WGS84", "PERMANENT"
        )
        # Location and mapset and region
        gs.create_location(
            self.tmp_dir, "temp_folium_WGS84", epsg="4326", overwrite=True
        )
        self._extent = self._convert_extent(
            env=os.environ
        )  # Get the extent of the original area in WGS84
        # Set region to match original region extent
        gs.run_command(
            "g.region",
            n=self._extent["north"],
            s=self._extent["south"],
            e=self._extent["east"],
            w=self._extent["west"],
            env=self._vector_env,
        )
        # Get Center of tmp GRASS region
        center = gs.parse_command("g.region", flags="cg", env=self._vector_env)
        center = (float(center["center_northing"]), float(center["center_easting"]))
        # Create Folium Map
        self.map = folium.Map(
            width=self.width,
            height=self.height,
            location=center,
            tiles="cartodbpositron",
        )
        # Create LayerControl default
        self.layer_control = False

    def _convert_coordinates(self, x, y, proj_in):
        """This function reprojects coordinates to WGS84, the required
        projection for vectors in folium.

        Arguments:
            x -- x coordinate (string)
            y -- y coordinate (string)
            proj_in -- proj4 string of location (for example, the output
            of g.region run with the `g` flag."""

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
        extent = {"north": north, "south": south, "east": east, "west": west}
        return extent

    def _folium_bounding_box(self, extent):
        """Reformats extent into bounding box to pass to folium"""
        return [[extent["north"], extent["west"]], [extent["south"], extent["east"]]]

    def add_vector(self, name):
        """Imports vector into temporary WGS84 location,
        re-formats to a GeoJSON and adds to folium map.

        Arguments:
            name -- a positional-only parameter; name of vector to be added
            to map as a string"""

        # Find full name of vector
        file_info = gs.find_file(name, element="vector")
        full_name = file_info["fullname"]
        name = file_info["name"]
        # Reproject vector into WGS84 Location
        env_info = gs.gisenv(env=os.environ)
        gs.run_command(
            "v.proj",
            input=full_name,
            location=env_info["LOCATION_NAME"],
            dbase=env_info["GISDBASE"],
            env=self._vector_env,
        )
        # Convert to GeoJSON
        json_file = self.tmp_dir / f"tmp_{name}.json"
        gs.run_command(
            "v.out.ogr",
            input=name,
            output=json_file,
            format="GeoJSON",
            env=self._vector_env,
        )
        # Import GeoJSON to folium and add to map
        folium.GeoJson(str(json_file), name=name).add_to(self.map)

    def add_layer_control(self, **kwargs):
        self.layer_control = True
        self.layer_control_object = folium.LayerControl(**kwargs)

    def show(self):
        """This function creates a folium map with a GRASS raster
        overlayed on a basemap.

        If map has layer control enabled, additional layers cannot be
        added after calling show()."""

        if self.layer_control:
            self.map.add_child(self.layer_control_object)
        # Create Figure
        fig = folium.Figure(width=self.width, height=self.height)
        # Add map to figure
        fig.add_child(self.map)

        return fig
