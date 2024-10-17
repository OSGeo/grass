#
# AUTHOR(S): Anna Petrasova <kratochanna AT gmail>
#
# PURPOSE:   This module contains functionality for managing region
#            during rendering.
#
# COPYRIGHT: (C) 2021 Anna Petrasova, and by the GRASS Development Team
#
#            This program is free software under the GNU General Public
#            License (>=v2). Read the file COPYING that comes with GRASS
#            for details.

"""Manage computational or display region settings for display (render) classes."""

import grass.script as gs
from grass.exceptions import CalledModuleError

from .utils import (
    get_map_name_from_d_command,
    estimate_resolution,
    set_target_region,
    get_rendering_size,
)


class RegionManagerForInteractiveMap:
    """Region manager for an interactive map (gets region from raster and vector)"""

    def __init__(self, use_region, saved_region, src_env, tgt_env):
        """Manages region during rendering for interactive map.

        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        :param src_env: source environment (original projection)
        :param tgt_env: target environment (pseudomercator)
        """
        self._use_region = use_region
        self._saved_region = saved_region
        self._src_env = src_env
        self._tgt_env = tgt_env
        self._resolution = None
        # [SW, NE]: inverted to easily expand based on data, see _set_bbox
        self._bbox = [[90, 180], [-90, -180]]
        if self._use_region:
            # tgt region already set, set resolution
            self._resolution = self._get_psmerc_region_resolution()
            self._set_bbox(self._src_env)
        if self._saved_region:
            self._src_env["GRASS_REGION"] = gs.region_env(
                region=self._saved_region, env=self._src_env
            )
            set_target_region(src_env=self._src_env, tgt_env=self._tgt_env)
            self._resolution = self._get_psmerc_region_resolution()
            self._set_bbox(self._src_env)

    @property
    def bbox(self):
        """Bbox property for accessing maximum bounding box of all rendered layers."""
        return self._bbox

    @property
    def resolution(self):
        """Resolution to be used for reprojection."""
        return self._resolution

    def _get_psmerc_region_resolution(self):
        """Get region resolution (average ns and ew) of psmerc mapset"""
        reg = gs.region(env=self._tgt_env)
        return (reg["nsres"] + reg["ewres"]) / 2

    def set_region_from_raster(self, raster):
        """Sets computational region for rendering.

        This functions sets computational region based on
        a raster map in the target environment.

        If user specified the name of saved region during object's initialization,
        the provided region is used. If it's not specified
        and use_region=True, current region is used.

        Also enlarges bounding box based on the raster.
        """
        if self._use_region or self._saved_region:
            # target region and bbox already set
            return
        # set target location region extent
        self._src_env["GRASS_REGION"] = gs.region_env(raster=raster, env=self._src_env)
        set_target_region(src_env=self._src_env, tgt_env=self._tgt_env)
        # set resolution based on r.proj estimate
        env_info = gs.gisenv(env=self._src_env)
        name, mapset = raster.split("@")
        self._resolution = estimate_resolution(
            raster=name,
            mapset=mapset,
            location=env_info["LOCATION_NAME"],
            dbase=env_info["GISDBASE"],
            env=self._tgt_env,
        )
        self._set_bbox(self._src_env)

    def set_bbox_vector(self, vector):
        """Enlarge bounding box based on vector"""
        if self._saved_region or self._use_region:
            return
        env = self._src_env.copy()
        env["GRASS_REGION"] = gs.region_env(vector=vector, env=env)
        self._set_bbox(env)

    def _set_bbox(self, env):
        bbox = gs.parse_command("g.region", flags="bg", env=env)
        south = float(bbox["ll_s"])
        west = float(bbox["ll_w"])
        north = float(bbox["ll_n"])
        east = float(bbox["ll_e"])
        self._bbox[0][0] = min(self._bbox[0][0], south)
        self._bbox[0][1] = min(self._bbox[0][1], west)
        self._bbox[1][0] = max(self._bbox[1][0], north)
        self._bbox[1][1] = max(self._bbox[1][1], east)


class RegionManagerFor2D:
    """Region manager for 2D displays (gets region from display commands)"""

    def __init__(self, use_region, saved_region, width, height, env):
        """Manages region during rendering.

        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        :param width: rendering width
        :param height: rendering height
        :param env: environment for rendering
        """
        self._env = env
        self._width = width
        self._height = height
        self._use_region = use_region
        self._saved_region = saved_region
        self._extent_set = False
        self._resolution_set = False
        self._size_set = False

    def set_region_from_env(self, env):
        """Copies GRASS_REGION from provided environment
        to local environment to set the computational region"""
        if "GRASS_REGION" in env:
            self._env["GRASS_REGION"] = env["GRASS_REGION"]

    def adjust_rendering_size_from_region(self):
        """Sets the environmental render width and height variables
        based on the region dimensions. Only first call of this
        method sets the variables, subsequent calls do not adjust them.
        """
        if not self._size_set:
            region = gs.region(env=self._env)
            width, height = get_rendering_size(region, self._width, self._height)
            self._env["GRASS_RENDER_WIDTH"] = str(round(width))
            self._env["GRASS_RENDER_HEIGHT"] = str(round(height))
            # only when extent is set you can disable future size setting
            if self._extent_set:
                self._size_set = True

    def set_region_from_command(self, module, **kwargs):
        """Sets computational region for rendering.

        This functions identifies a raster/vector map from command
        and tries to set computational region based on that.
        It takes the extent from the first layer (raster or vector)
        and resolution and alignment from first raster layer.

        If user specified the name of saved region during object's initialization,
        the provided region is used. If it's not specified
        and use_region=True, current region is used.
        """
        if self._saved_region:
            self._env["GRASS_REGION"] = gs.region_env(
                region=self._saved_region, env=self._env
            )
            return
        if self._use_region:
            # use current
            return
        if self._resolution_set and self._extent_set:
            return
        name = get_map_name_from_d_command(module, **kwargs)
        if not name:
            return
        if len(name.split()) > 1:
            name = name.split()[0]
        try:
            if module.startswith("d.vect"):
                if not self._resolution_set and not self._extent_set:
                    self._env["GRASS_REGION"] = gs.region_env(
                        vector=name, env=self._env
                    )
                    self._extent_set = True
            elif not self._resolution_set and not self._extent_set:
                self._env["GRASS_REGION"] = gs.region_env(raster=name, env=self._env)
                self._extent_set = True
                self._resolution_set = True
            elif not self._resolution_set:
                self._env["GRASS_REGION"] = gs.region_env(align=name, env=self._env)
                self._resolution_set = True
        except CalledModuleError:
            return


class RegionManagerForSeries:
    """Region manager for SeriesMap"""

    def __init__(self, use_region, saved_region, width, height, env):
        """Manages region during rendering.

        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        :param width: rendering width
        :param height: rendering height
        :param env: environment for rendering
        """
        self._env = env
        self._width = width
        self._height = height
        self._use_region = use_region
        self._saved_region = saved_region
        self._extent_set = False
        self._resolution_set = False

    def set_region_from_rasters(self, rasters):
        """Sets computational region for rendering from a series of rasters.

        This function sets the region from a series of rasters. If the extent or
        resolution has already been set by calling this function previously or by the
        set_region_from vectors() function, this function will not modify it.

        If user specified the name of saved region during object's initialization,
        the provided region is used. If it's not specified
        and use_region=True, current region is used.
        """
        if self._saved_region:
            self._env["GRASS_REGION"] = gs.region_env(
                region=self._saved_region, env=self._env
            )
            return
        if self._use_region:
            # use current
            return
        if self._resolution_set and self._extent_set:
            return
        if not self._resolution_set and not self._extent_set:
            self._env["GRASS_REGION"] = gs.region_env(raster=rasters, env=self._env)
            self._extent_set = True
            self._resolution_set = True
        elif not self._resolution_set:
            self._env["GRASS_REGION"] = gs.region_env(align=rasters[0], env=self._env)
            self._resolution_set = True

    def set_region_from_vectors(self, vectors):
        """Sets computational region extent for rendering from a series of vectors

        If the extent and resolution has already been set by set_region_from_rasters,
        or by using the saved_region or use_region arguments, the region is not modified
        """
        if self._saved_region:
            self._env["GRASS_REGION"] = gs.region_env(
                region=self._saved_region, env=self._env
            )
            return
        if self._use_region:
            # use current
            return
        if self._resolution_set and self._extent_set:
            return
        if not self._resolution_set and not self._extent_set:
            self._env["GRASS_REGION"] = gs.region_env(vector=vectors, env=self._env)
            self._extent_set = True


class RegionManagerFor3D:
    """Region manager for 3D displays (gets region from m.nviz.image command)"""

    def __init__(self, use_region, saved_region):
        """Manages region during rendering.

        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        """
        self._use_region = use_region
        self._saved_region = saved_region

    def set_region_from_command(self, env, **kwargs):
        """Sets computational region for rendering.

        This functions identifies a raster map from m.nviz.image command
        and tries to set computational region based on that.

        If user specified the name of saved region during object's initialization,
        the provided region is used. If it's not specified
        and use_region=True, current region is used.
        """
        if self._saved_region:
            env["GRASS_REGION"] = gs.region_env(region=self._saved_region, env=env)
            return
        if self._use_region:
            # use current
            return
        if "elevation_map" in kwargs:
            elev = kwargs["elevation_map"].split(",")[0]
            try:
                env["GRASS_REGION"] = gs.region_env(raster=elev, env=env)
            except CalledModuleError:
                return


class RegionManagerForTimeSeries:
    """Region manager for TimeSeries visualizations."""

    def __init__(self, use_region, saved_region, env):
        """Manages region during rendering.

        :param use_region: if True, use either current or provided saved region,
                          else derive region from rendered layers
        :param saved_region: if name of saved_region is provided,
                            this region is then used for rendering
        :param env: environment for rendering
        """
        self._env = env
        self._use_region = use_region
        self._saved_region = saved_region

    def set_region_from_timeseries(self, timeseries):
        """Sets computational region for rendering.

        This function sets the computation region from the extent of
        a space-time dataset by using its bounding box and resolution.

        If user specified the name of saved region during object's initialization,
        the provided region is used. If it's not specified
        and use_region=True, current region is used.
        """
        if self._saved_region:
            self._env["GRASS_REGION"] = gs.region_env(
                region=self._saved_region, env=self._env
            )
            return
        if self._use_region:
            # use current
            return
        # Get extent, resolution from space time dataset
        info = gs.parse_command("t.info", input=timeseries, flags="g", env=self._env)
        # Set grass region from extent
        self._env["GRASS_REGION"] = gs.region_env(
            n=info["north"],
            s=info["south"],
            e=info["east"],
            w=info["west"],
            nsres=info["nsres_min"],
            ewres=info["ewres_min"],
        )
