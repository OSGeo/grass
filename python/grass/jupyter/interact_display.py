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
from .display import GrassRenderer
from .utils import (
    convert_coordinates_to_latlon,
    get_region,
    get_location_proj_string,
    reproject_region,
    estimate_resolution,
)
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

        # Remember original environment
        self._src_env = os.environ.copy()

        # Set up temporary locations for vectors and rasters
        # They must be different since folium takes vectors
        # in WGS84 and rasters in Pseudo-mercator
        self._setup_vector_location()
        self._setup_raster_location()

        # Get Center of tmp GRASS region
        center = gs.parse_command("g.region", flags="cg", env=self._wgs84_env)
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

    def _setup_vector_location(self):
        # Create new vector environment for tmp WGS84 location
        rcfile, self._wgs84_env = gs.create_environment(
            self.tmp_dir, "temp_folium_WGS84", "PERMANENT"
        )
        # Location and mapset and region for Vectors
        gs.create_location(
            self.tmp_dir, "temp_folium_WGS84", epsg="4326", overwrite=True
        )
        # Reproject region
        region = get_region(env=self._src_env)
        from_proj = get_location_proj_string(env=self._src_env)
        to_proj = get_location_proj_string(env=self._wgs84_env)
        new_region = reproject_region(region, from_proj, to_proj)
        # self._folium_region
        # Set vector region to match original region extent
        gs.run_command(
            "g.region",
            n=new_region["north"],
            s=new_region["south"],
            e=new_region["east"],
            w=new_region["west"],
            env=self._wgs84_env,
        )

    def _setup_raster_location(self):
        # Create new raster environment in PseudoMercator
        rcfile, self._psmerc_env = gs.create_environment(
            self.tmp_dir, "temp_folium_WGS84_pmerc", "PERMANENT"
        )
        # Location and mapset for Rasters
        gs.create_location(
            self.tmp_dir, "temp_folium_WGS84_pmerc", epsg="3857", overwrite=True
        )
        # Reproject region
        region = get_region(env=self._src_env)
        from_proj = get_location_proj_string(env=self._src_env)
        to_proj = get_location_proj_string(env=self._psmerc_env)
        new_region = reproject_region(region, from_proj, to_proj)
        # Set raster region to match original region extent
        gs.run_command(
            "g.region",
            n=new_region["north"],
            s=new_region["south"],
            e=new_region["east"],
            w=new_region["west"],
            env=self._psmerc_env,
        )

    def add_vector(self, name):
        """Imports vector into temporary WGS84 location,
        re-formats to a GeoJSON and adds to folium map.

        :param str name: name of vector to be added to map;
                         positional-only parameter
        """

        # Find full name of vector
        file_info = gs.find_file(name, element="vector")
        full_name = file_info["fullname"]
        name = file_info["name"]
        # Reproject vector into WGS84 Location
        env_info = gs.gisenv(env=self._src_env)
        gs.run_command(
            "v.proj",
            input=full_name,
            location=env_info["LOCATION_NAME"],
            dbase=env_info["GISDBASE"],
            env=self._wgs84_env,
        )
        # Convert to GeoJSON
        json_file = self.tmp_dir / f"tmp_{name}.json"
        gs.run_command(
            "v.out.ogr",
            input=name,
            output=json_file,
            format="GeoJSON",
            env=self._wgs84_env,
        )
        # Import GeoJSON to folium and add to map
        folium.GeoJson(str(json_file), name=name).add_to(self.map)

    def add_raster(self, name, opacity=0.8):
        """Imports raster into temporary WGS84 location,
        exports as png and overlays on folium map

        :param str name: name of raster to add to display
        :param float opacity: raster opacity, number between
                              0 (transparent) and 1 (opaque)
        """

        # Find full name of raster
        file_info = gs.find_file(name, element="raster")
        full_name = file_info["fullname"]
        name = file_info["name"]

        # Reproject raster into WGS84/epsg3857 location
        env_info = gs.gisenv(env=self._src_env)
        resolution = estimate_resolution(
            full_name, env_info["GISDBASE"], env_info["LOCATION_NAME"], self._psmerc_env
        )
        gs.run_command(
            "r.proj",
            input=full_name,
            output=name,
            location=env_info["LOCATION_NAME"],
            dbase=env_info["GISDBASE"],
            resolution=resolution,
            env=self._psmerc_env,
        )

        # Reprojects bounds of raster for overlaying png (THIS HAS TO BE IN WGS84)
        bounds = gs.read_command("g.region", flags="g", env=self._src_env)
        bounds = gs.parse_key_val(bounds, sep="=", vsep="\n")
        proj = gs.read_command("g.proj", flags="jf", env=self._src_env)
        north, east = convert_coordinates_to_latlon(bounds["e"], bounds["n"], proj)
        south, west = convert_coordinates_to_latlon(bounds["w"], bounds["s"], proj)
        new_bounds = [[north, west], [south, east]]

        # Write raster to png file with GrassRenderer
        filename = os.path.join(self.tmp_dir, "map.png")
        m = GrassRenderer(
            width=bounds["cols"],
            height=bounds["rows"],
            env=self._psmerc_env,
            filename=filename,
        )
        m.run("d.rast", map=name)

        # Overlay image on folium map
        img = folium.raster_layers.ImageOverlay(
            image=filename,
            name=name,
            bounds=new_bounds,
            opacity=opacity,
            interactive=True,
            cross_origin=False,
        )

        # Add image to map
        img.add_to(self.map)

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
