#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   This module contains functions for interactive display
#            in Jupyter Notebooks.
#
# COPYRIGHT: (C) 2021 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

import os
import sys
import tempfile
import weakref
from pathlib import Path
import grass.script as gs
from .display import GrassRenderer
from .utils import (
    estimate_resolution,
    get_location_proj_string,
    get_region,
    reproject_region,
    setup_location,
)
from .region import RegionManagerForInteractiveMap


class ReprojectionRenderer:
    """This class reprojects rasters and vectors to folium-compatible
    temporary location and projection.

    In preparation to displaying with folium, it saves vectors to geoJSON and rasters
    to PNG images.
    """

    def __init__(self, use_region=False, saved_region=None, work_dir=None):
        # Temporary folder for all our files
        if not work_dir:
            # Make temporary folder for all our files
            self._tmp_dir = tempfile.TemporaryDirectory()
        else:
            self._tmp_dir = work_dir

        # Remember original environment; all environments used
        # in this class are derived from this one
        self._src_env = os.environ.copy()

        # Set up temporary locations  in WGS84 and Pseudo-Mercator
        # We need two because folium uses WGS84 for vectors and coordinates
        # and Pseudo-Mercator for raster overlays
        self._rcfile_psmerc, self._psmerc_env = setup_location(
            "psmerc", self._tmp_dir.name, "3857", self._src_env
        )
        self._rcfile_wgs84, self._wgs84_env = setup_location(
            "wgs84", self._tmp_dir.name, "4326", self._src_env
        )

        # region handling
        self._region_manager = RegionManagerForInteractiveMap(
            use_region, saved_region, self._src_env, self._psmerc_env
        )

        # Cleanup rcfiles with finalizer
        def remove_if_exists(path):
            if sys.version_info < (3, 8):
                try:
                    os.remove(path)
                except FileNotFoundError:
                    pass
            else:
                path.unlink(missing_ok=True)

        def clean_up(paths):
            for path in paths:
                remove_if_exists(path)

        self._finalizer = weakref.finalize(
            self, clean_up, [Path(self._rcfile_psmerc), Path(self._rcfile_wgs84)]
        )

    def render_raster(self, name):
        # Find full name of raster
        file_info = gs.find_file(name, element="cell", env=self._src_env)
        full_name = file_info["fullname"]
        name = file_info["name"]
        mapset = file_info["mapset"]

        self._region_manager.set_region_from_raster(full_name)
        # Reproject raster into WGS84/epsg3857 location
        env_info = gs.gisenv(env=self._src_env)
        resolution = estimate_resolution(
            raster=name,
            mapset=mapset,
            location=env_info["LOCATION_NAME"],
            dbase=env_info["GISDBASE"],
            env=self._psmerc_env,
        )
        tgt_name = full_name.replace("@", "_")
        gs.run_command(
            "r.proj",
            input=full_name,
            output=tgt_name,
            mapset=mapset,
            location=env_info["LOCATION_NAME"],
            dbase=env_info["GISDBASE"],
            resolution=resolution,
            env=self._psmerc_env,
        )
        # Write raster to png file with GrassRenderer
        region_info = gs.region(env=self._src_env)
        png_width = region_info["cols"]
        png_height = region_info["rows"]
        filename = os.path.join(self._tmp_dir.name, f"{tgt_name}.png")
        m = GrassRenderer(
            width=png_width,
            height=png_height,
            env=self._psmerc_env,
            filename=filename,
            use_region=True,
        )
        m.run("d.rast", map=tgt_name)
        # Reproject bounds of raster for overlaying png
        # Bounds need to be in WGS84
        old_bounds = get_region(self._src_env)
        from_proj = get_location_proj_string(env=self._src_env)
        to_proj = get_location_proj_string(env=self._wgs84_env)
        bounds = reproject_region(old_bounds, from_proj, to_proj)
        new_bounds = [
            [bounds["north"], bounds["west"]],
            [bounds["south"], bounds["east"]],
        ]

        return filename, new_bounds, self._tmp_dir

    def render_vector(self, name):
        # Find full name of vector
        file_info = gs.find_file(name, element="vector")
        full_name = file_info["fullname"]
        name = file_info["name"]
        mapset = file_info["mapset"]
        new_name = full_name.replace("@", "_")
        # set bbox
        self._region_manager.set_bbox_vector(full_name)
        # Reproject vector into WGS84 Location
        env_info = gs.gisenv(env=self._src_env)
        gs.run_command(
            "v.proj",
            input=name,
            output=new_name,
            mapset=mapset,
            location=env_info["LOCATION_NAME"],
            dbase=env_info["GISDBASE"],
            env=self._wgs84_env,
        )
        # Convert to GeoJSON
        json_file = Path(self._tmp_dir.name) / f"{new_name}.json"
        gs.run_command(
            "v.out.ogr",
            input=new_name,
            output=json_file,
            format="GeoJSON",
            env=self._wgs84_env,
        )

        return json_file, self._tmp_dir


class Raster:
    """Similar to overlay object in folium"""

    def __init__(self, name, title=None, use_region=False, saved_region=None, **kwargs):
        """Not sure"""
        import folium

        self._folium = folium

        # Make temporary folder for all our files
        self._tmp_dir = tempfile.TemporaryDirectory()

        self._name = name
        self._overlay_kwargs = kwargs
        self._filename, self._bounds, tmp_dir = ReprojectionRenderer(
            use_region=use_region, saved_region=saved_region, work_dir=self._tmp_dir
        ).render_raster(name)
        self._title = title
        if not self._title:
            self._title = self._name

    def add_to(self, map):
        # Overlay image on folium map
        img = self._folium.raster_layers.ImageOverlay(
            image=self._filename,
            bounds=self._bounds,
            name=self._title,
            **self._overlay_kwargs,
        )

        # Add image to map
        img.add_to(map)


class Vector:
    """For adding geoJSON to folium map"""

    def __init__(self, name, title=None, use_region=False, saved_region=None, **kwargs):
        """Not sure"""
        import folium

        self._folium = folium

        # Make temporary folder for all our files
        self._tmp_dir = tempfile.TemporaryDirectory()

        self._name = name
        self._geojson_kwargs = kwargs
        self._filename, tmp_dir = ReprojectionRenderer(
            use_region=use_region, saved_region=saved_region, work_dir=self._tmp_dir
        ).render_vector(name)
        self._title = title
        if not self._title:
            self._title = self._name

    def add_to(self, map):
        self._folium.GeoJson(
            str(self._filename), name=self._title, **self._geojson_kwargs
        ).add_to(map)


class InteractiveMap:
    """This class creates interative GRASS maps with folium.

    Basic Usage:
    >>> m = InteractiveMap()
    >>> m.add_vector("streams")
    >>> m.add_raster("elevation")
    >>> m.add_layer_control()
    >>> m.show()
    """

    def __init__(self, width=400, height=400, use_region=False, saved_region=None):
        """Creates a blank folium map centered on g.region.

        :param int height: height in pixels of figure (default 400)
        :param int width: width in pixels of figure (default 400)
        """

        import folium

        self._folium = folium

        # Store region settings
        self._use_region = use_region
        self._saved_region = saved_region

        # Store height and width
        self.width = width
        self.height = height

        # Create Folium Map
        self.map = self._folium.Map(
            width=self.width,
            height=self.height,
            tiles="cartodbpositron",
        )
        # Set LayerControl default
        self.layer_control = False

        self.renderer = ReprojectionRenderer(
            use_region=use_region, saved_region=saved_region
        )

    def add_vector(self, name, title=None, **kwargs):
        """Imports vector into temporary WGS84 location,
        re-formats to a GeoJSON and adds to folium map.

        :param str name: name of vector to be added to map;
                         positional-only parameter
        :param str title: vector name for layer control
        """

        # Reproject vector and write to Geojson
        filename, tmp_dir = self.renderer.render_vector(name)

        if not title:
            title = name

        # Import GeoJSON to folium and add to map
        self._folium.GeoJson(str(filename), name=title, **kwargs).add_to(self.map)

        return self.map

    def add_raster(self, name, title=None, **kwargs):
        """Imports raster into temporary WGS84 location,
        exports as png and overlays on folium map.

        Color table for the raster can be modified with `r.colors` before calling
        this function.

        .. note:: This will only work if the raster is located in the current mapset.
        To change the color table of a raster located outside the current mapset,
        switch to that mapset with `g.mapset`, modify the color table with `r.color`
        then switch back to the initial mapset and run this function.

        :param str name: name of raster to add to display; positional-only parameter
        :param str title: raster name for layer control
        :param float opacity: raster opacity, number between
                              0 (transparent) and 1 (opaque)
        """

        # Reproject vector and write to Geojson
        filename, bounds, tmp_dir = self.renderer.render_raster(name)

        if not title:
            title = name

        img = self._folium.raster_layers.ImageOverlay(
            image=filename,
            bounds=bounds,
            name=title,
            **kwargs,
        )

        # Add image to map
        img.add_to(self.map)

        return self.map

    def add_layer_control(self, **kwargs):
        """Add layer control to display"

        :param `**kwargs`: named arguments to be passed to folium.LayerControl()"""

        self.layer_control = True
        self.layer_control_object = self._folium.LayerControl(**kwargs)

    def show(self):
        """This function returns a folium figure object with a GRASS raster
        overlayed on a basemap.

        If map has layer control enabled, additional layers cannot be
        added after calling show()."""

        if self.layer_control:
            self.map.add_child(self.layer_control_object)
        # Create Figure
        fig = self._folium.Figure(width=self.width, height=self.height)
        # Add map to figure
        fig.add_child(self.map)
        self.map.fit_bounds(self.renderer._region_manager.bbox)

        return fig

    def save(self, filename):
        """Save map as an html map.

        :param str filename: name of html file
        """
        self.map.save(filename)
