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
        self._env=os.environ.copy()
        
        # Set up temporary locations for vectors and rasters
        # They must be different since folium takes vectors
        # in WGS84 and rasters in Pseudo-mercator
        self.setup_vector_location()
        self.setup_raster_location()
        
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
        
    def get_region(self, env=None):
        """Returns current computational region as dictionary.
        Adds long key names.
        """
        region = gs.region(env=env)
        region['east'] = region['e']
        region['west'] = region['w']
        region['north'] = region['n']
        region['south'] = region['s']
        return region
    
    def get_location_proj_string(self, env=None):
        out = gs.read_command('g.proj', flags='jf', env=env)
        return out.strip()

    def reproject_region(self, region, from_proj, to_proj):
        region = region.copy()
        proj_input = '{east} {north}\n{west} {south}'.format(**region)
        proc = gs.start_command('m.proj', input='-', separator=' , ',
	                        proj_in=from_proj, proj_out=to_proj,
	                        flags="d",
	                        stdin=gs.PIPE, stdout=gs.PIPE, stderr=gs.PIPE)
        proc.stdin.write(gs.encode(proj_input))
        proc.stdin.close()
        proc.stdin = None
        proj_output, stderr = proc.communicate()
        if proc.returncode:
            raise RuntimeError("reprojecting region: m.proj error: " + stderr)
        enws = gs.decode(proj_output).split(os.linesep)
        elon, nlat, unused = enws[0].split(' ')
        wlon, slat, unused = enws[1].split(' ')
        region['east'] = elon
        region['north'] = nlat
        region['west'] = wlon
        region['south'] = slat
        return region
    
    def setup_vector_location(self):
        # Create new vector environment for tmp WGS84 location
        rcfile, self._vector_env = gs.create_environment(
            self.tmp_dir, "temp_folium_WGS84", "PERMANENT"
        )
        # Location and mapset and region for Vectors
        gs.create_location(
            self.tmp_dir, "temp_folium_WGS84", epsg="4326", overwrite=True
        )
        # Reproject region
        region = self.get_region(env=self._env)
        from_proj = self.get_location_proj_string(env=self._env)
        to_proj = self.get_location_proj_string(env=self._vector_env)
        new_region = self.reproject_region(region, from_proj, to_proj)
        # Set vector region to match original region extent
        gs.run_command(
            "g.region",
            n=new_region["north"],
            s=new_region["south"],
            e=new_region["east"],
            w=new_region["west"],
            env=self._vector_env
        )
        
    def setup_raster_location(self):
        # Create new raster environment in PseudoMercator
        rcfile, self._raster_env = gs.create_environment(
            self.tmp_dir, "temp_folium_WGS84_pmerc", "PERMANENT"
        )
        # Location and mapset for Rasters
        gs.create_location(
            self.tmp_dir, "temp_folium_WGS84_pmerc", epsg="3857", overwrite=True
        )
        # Reproject region
        region = self.get_region(env=self._env)
        from_proj = self.get_location_proj_string(env=self._env)
        to_proj = self.get_location_proj_string(env=self._raster_env)
        new_region = self.reproject_region(region, from_proj, to_proj)
        # Set raster region to match original region extent
        gs.run_command(
            "g.region",
            n=new_region["north"],
            s=new_region["south"],
            e=new_region["east"],
            w=new_region["west"],
            env=self._raster_env
        )
        
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

#    def _convert_extent(self, env=None):
#        """This function returns the bounding box of the current region
#        in WGS84, the required projection for folium"""
#
#        # Get proj of current GRASS region
#        proj = gs.read_command("g.proj", flags="jf", env=env)
#        # Get extent
#        extent = gs.parse_command("g.region", flags="g", env=env)
#        # Convert extent to EPSG:3857, required projection for Folium
#        north, east = self._convert_coordinates(extent["e"], extent["n"], proj)
#        south, west = self._convert_coordinates(extent["w"], extent["s"], proj)
#        extent = {"north": north, "south": south, "east": east, "west": west}
#        return extent

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
               
    def add_raster(self, name, opacity=0.8):
        """Imports raster into temporary WGS84 location,
            exports as png and overlays on folium map"""
        
        # Find full name of raster
        file_info = gs.find_file(name, element="raster")
        full_name = file_info["fullname"]
        name = file_info["name"]
        # Reproject raster into WGS84/epsg3857 location
        env_info = gs.gisenv(env=os.environ)
        gs.run_command("r.proj",
                      input=full_name,
                      output=name,
                      location=env_info["LOCATION_NAME"],
                      dbase=env_info["GISDBASE"],
                      env=self._raster_env
        )
        
        # Write raster to file with GrassRenderer
        tmp_file = tempfile.NamedTemporaryFile(suffix=".png")
        m=GrassRenderer(env=self._raster_env)
        m.run("d.rast", map=name, filename=tmp_file.name)
        
        # Get bounds of map for overlaying png
        raster_env_info = gs.gisenv(env=self._raster_env)
        bounds = gs.read_command("r.proj",
                                input=name,
                                location=raster_env_info["LOCATION_NAME"],
                                flags="g",
                                env=self._raster_env
        )
        
        print(bounds)
        # Reformat for folium
        bounds = self._folium_bounding_box(bounds)
        
        # Overlay image on folium map
        img = folium.raster_layers.ImageOverlay(
           image=tmp_file.name,
           bounds= bounds,
           opacity=opacity,
           interactive=True,
           cross_origin=False
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
