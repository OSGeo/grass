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
    get_location_proj_string,
    get_map_name_from_d_command,
    get_region,
    reproject_region,
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
        self._bbox = [[90, 180], [-90, -180]]

    @property
    def bbox(self):
        """Bbox property for accessing maximum bounding box of all rendered layers."""
        return self._bbox

    def set_region_from_raster(self, raster):
        """Sets computational region for rendering.

        This functions sets computational region based on
        a raster map in the target environment.

        If user specified the name of saved region during object's initialization,
        the provided region is used. If it's not specified
        and use_region=True, current region is used.

        Also enlarges bounding box based on the raster.
        """
        if self._saved_region:
            self._src_env["GRASS_REGION"] = gs.region_env(
                region=self._saved_region, env=self._src_env
            )
        elif self._use_region:
            # use current
            self._set_bbox(self._src_env)
            return
        else:
            self._src_env["GRASS_REGION"] = gs.region_env(
                raster=raster, env=self._src_env
            )
        region = get_region(env=self._src_env)
        from_proj = get_location_proj_string(self._src_env)
        to_proj = get_location_proj_string(env=self._tgt_env)
        new_region = reproject_region(region, from_proj, to_proj)
        gs.run_command(
            "g.region",
            n=new_region["north"],
            s=new_region["south"],
            e=new_region["east"],
            w=new_region["west"],
            env=self._tgt_env,
        )
        self._set_bbox(self._src_env)

    def set_bbox_vector(self, vector):
        """Enlarge bounding box based on vector"""
        env = self._src_env.copy()
        env["GRASS_REGION"] = gs.region_env(vector=vector, env=env)
        self._set_bbox(env)

    def _set_bbox(self, env):
        bbox = gs.parse_command("g.region", flags="bg", env=env)
        south = float(bbox["ll_s"])
        west = float(bbox["ll_w"])
        north = float(bbox["ll_n"])
        east = float(bbox["ll_e"])
        if self._bbox[0][0] > south:
            self._bbox[0][0] = south
        if self._bbox[0][1] > west:
            self._bbox[0][1] = west
        if self._bbox[1][0] < north:
            self._bbox[1][0] = north
        if self._bbox[1][1] < east:
            self._bbox[1][1] = east


class RegionManagerFor2D:
    """Region manager for 2D displays (gets region from display commands)"""

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
        self._extent_set = False
        self._resolution_set = False

    def set_region_from_env(self, env):
        """Copies GRASS_REGION from provided environment
        to local environment to set the computational region"""
        if "GRASS_REGION" in env:
            self._env["GRASS_REGION"] = env["GRASS_REGION"]

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
            else:
                if not self._resolution_set and not self._extent_set:
                    self._env["GRASS_REGION"] = gs.region_env(
                        raster=name, env=self._env
                    )
                    self._extent_set = True
                    self._resolution_set = True
                elif not self._resolution_set:
                    self._env["GRASS_REGION"] = gs.region_env(align=name, env=self._env)
                    self._resolution_set = True
        except CalledModuleError:
            return


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
