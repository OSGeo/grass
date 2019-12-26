#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.to.rast3
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Convert a space time raster dataset into a 3D raster map
# COPYRIGHT:    (C) 2011-2017 by the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

#%module
#% description: Converts a space time raster dataset into a 3D raster map.
#% keyword: temporal
#% keyword: conversion
#% keyword: raster
#% keyword: raster3d
#% keyword: voxel
#% keyword: time
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_R3_OUTPUT
#%end
from __future__ import print_function

import os
import grass.script as grass
from datetime import datetime
from grass.exceptions import CalledModuleError

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]

    # Make sure the temporal database exists
    tgis.init()

    mapset = grass.gisenv()["MAPSET"]

    sp = tgis.open_old_stds(input, "strds")

    grass.use_temp_region()

    maps = sp.get_registered_maps_as_objects_by_granularity()
    num_maps = len(maps)
    # get datatype of the first map
    if maps:
        maps[0][0].select()
        datatype = maps[0][0].metadata.get_datatype()
    else:
        datatype = None

    # Get the granularity and set bottom, top and top-bottom resolution
    granularity = sp.get_granularity()

    # This is the reference time to scale the z coordinate
    reftime = datetime(1900, 1, 1)

    # We set top and bottom according to the start time in relation
    # to the date 1900-01-01 00:00:00
    # In case of days, hours, minutes and seconds, a double number
    # is used to represent days and fracs of a day

    # Space time voxel cubes with montly or yearly granularity can not be
    # mixed with other temporal units

    # Compatible temporal units are : days, hours, minutes and seconds
    # Incompatible are years and moths
    start, end = sp.get_temporal_extent_as_tuple()

    if sp.is_time_absolute():
        unit = granularity.split(" ")[1]
        granularity = float(granularity.split(" ")[0])

        print("Gran from stds %0.15f"%(granularity))

        if unit == "years" or unit == "year":
            bottom = float(start.year - 1900)
            top = float(granularity * num_maps)
        elif unit == "months" or unit == "month":
            bottom = float((start.year - 1900) * 12 + start.month)
            top = float(granularity * num_maps)
        else:
            bottom = float(tgis.time_delta_to_relative_time(start - reftime))
            days = 0.0
            hours = 0.0
            minutes = 0.0
            seconds = 0.0
            if unit == "days" or unit == "day":
                days = float(granularity)
            if unit == "hours" or unit == "hour":
                hours = float(granularity)
            if unit == "minutes" or unit == "minute":
                minutes = float(granularity)
            if unit == "seconds" or unit == "second":
                seconds = float(granularity)

            granularity = float(days + hours / 24.0 + minutes / \
                1440.0 + seconds / 86400.0)
    else:
        unit = sp.get_relative_time_unit()
        bottom = start

    top = float(bottom + granularity * float(num_maps))
    try:
        grass.run_command("g.region", t=top, b=bottom, tbres=granularity)
    except CalledModuleError:
        grass.fatal(_("Unable to set 3D region"))

    # Create a NULL map to fill the gaps
    null_map = "temporary_null_map_%i" % os.getpid()
    if datatype == 'DCELL':
        grass.mapcalc("%s = double(null())" % (null_map))
    elif datatype == 'FCELL':
        grass.mapcalc("%s = float(null())" % (null_map))
    else:
        grass.mapcalc("%s = null()" % (null_map))

    if maps:
        count = 0
        map_names = ""
        for map in maps:
            # Use the first map
            id = map[0].get_id()
            # None ids will be replaced by NULL maps
            if id is None:
                id = null_map

            if count == 0:
                map_names = id
            else:
                map_names += ",%s" % id

            count += 1

        try:
            grass.run_command("r.to.rast3", input=map_names,
                              output=output, overwrite=grass.overwrite())
        except CalledModuleError:
            grass.fatal(_("Unable to create 3D raster map <%s>" % output))

    grass.run_command("g.remove", flags='f', type='raster', name=null_map)

    title = _("Space time voxel cube")
    descr = _("This space time voxel cube was created with t.rast.to.rast3")

    # Set the unit
    try:
        grass.run_command("r3.support", map=output, vunit=unit,
                          title=title, description=descr,
                          overwrite=grass.overwrite())
    except CalledModuleError:
        grass.warning(_("%s failed to set units.") % 'r3.support')

    # Register the space time voxel cube in the temporal GIS
    if output.find("@") >= 0:
        id = output
    else:
        id = output + "@" + mapset

    start, end = sp.get_temporal_extent_as_tuple()
    r3ds = tgis.Raster3DDataset(id)

    if r3ds.is_in_db():
        r3ds.select()
        r3ds.delete()
        r3ds = tgis.Raster3DDataset(id)

    r3ds.load()

    if sp.is_time_absolute():
        r3ds.set_absolute_time(start, end)
    else:
        r3ds.set_relative_time(start, end, sp.get_relative_time_unit())

    r3ds.insert()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
