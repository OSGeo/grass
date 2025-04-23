#
# AUTHOR(S): Caitlin Haedrich <caitlin DOT haedrich AT gmail>
#
# PURPOSE:   ReprojectionRenderer writes PNGs and geoJSONs that are importable by
#            folium. Reprojects rasters to Pseudo-Mercator and vectors to WGS84.
#            Exports reprojected rasters and vectors to PNGs and geoJSONs, respectively.
#
# COPYRIGHT: (C) 2022 Caitlin Haedrich, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Reprojects rasters to Pseudo-Mercator and vectors to WGS84. Exports reprojected
rasters and vectors to PNGs and geoJSONs, respectively."""

import os
import tempfile
import weakref
from pathlib import Path
import grass.script as gs
from .map import Map
from .utils import (
    get_location_proj_string,
    get_region,
    reproject_region,
    setup_location,
)
from .region import RegionManagerForInteractiveMap


class ReprojectionRenderer:
    """This class reprojects rasters and vectors to folium-compatible temporary location
     and projection.

    In preparation to displaying with folium, it saves vectors to geoJSON and rasters to
    PNG images.
    """

    def __init__(self, use_region=False, saved_region=None, work_dir=None):
        """Creates Pseudo-Mercator and WGS84 locations. If no work_dir provided, also
        creates temporary working directory to contain locations.

        param bool use_region: use computational region of current mapset
        param str saved_region: name of saved computation region to use
        param work_dir: path to directory where locations, files should be written
        """
        # Temporary folder for all our files
        if not work_dir:
            # Resource managed by weakref.finalize.
            self._tmp_dir = (
                # pylint: disable=consider-using-with
                tempfile.TemporaryDirectory()
            )

            def cleanup(tmpdir):
                tmpdir.cleanup()

            weakref.finalize(self, cleanup, self._tmp_dir)
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

    def get_bbox(self):
        """Return bounding box of computation region in WGS84"""
        return self._region_manager.bbox

    def render_raster(self, name):
        """Reprojects raster to Pseudo-Mercator and saves PNG in working directory.
        Return PNG filename and bounding box of WGS84.

        param str name: name of raster
        """
        # Find full name of raster
        file_info = gs.find_file(name, element="cell", env=self._src_env)
        full_name = file_info["fullname"]
        name = file_info["name"]
        mapset = file_info["mapset"]

        self._region_manager.set_region_from_raster(full_name)
        # Reproject raster into WGS84/epsg3857 location
        env_info = gs.gisenv(env=self._src_env)
        tgt_name = full_name.replace("@", "_")
        gs.run_command(
            "r.proj",
            input=full_name,
            output=tgt_name,
            mapset=mapset,
            project=env_info["LOCATION_NAME"],
            dbase=env_info["GISDBASE"],
            resolution=self._region_manager.resolution,
            env=self._psmerc_env,
        )
        # Write raster to png file with Map
        raster_info = gs.raster_info(tgt_name, env=self._psmerc_env)
        filename = os.path.join(self._tmp_dir.name, f"{tgt_name}.png")
        img = Map(
            width=int(raster_info["cols"]),
            height=int(raster_info["rows"]),
            env=self._psmerc_env,
            filename=filename,
            use_region=True,
        )
        img.run("d.rast", map=tgt_name)
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

        return filename, new_bounds

    def render_vector(self, name):
        """Reproject vector to WGS84 and save geoJSON in working directory. Return
        geoJSON filename.

        param str name: name of vector"""
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
            project=env_info["LOCATION_NAME"],
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

        return json_file
