#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.to.rast3
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Convert a space time raster dataset into a rast3d map
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Converts a space time raster dataset into a raster3d map.
#% keywords: temporal
#% keywords: conversion
#% keywords: raster3d
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_R3_OUTPUT
#%end

import os
import grass.script as grass
import grass.temporal as tgis
from datetime import datetime

############################################################################


def main():

    # Get the options
    input = options["input"]
    output = options["output"]

    # Make sure the temporal database exists
    tgis.init()

    mapset = grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = tgis.SpaceTimeRasterDataset(id)

    if sp.is_in_db() == False:
        grass.fatal(_("Space time %s dataset <%s> not found") % (
            sp.get_new_map_instance(None).get_type(), id))

    sp.select()

    grass.use_temp_region()

    maps = sp.get_registered_maps_as_objects_by_granularity()
    num_maps = len(maps)

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
        
        print "Gran from stds %0.15f"%(granularity)

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
    ret = grass.run_command("g.region", t=top, b=bottom, tbres=granularity)

    if ret != 0:
        grass.fatal(_("Unable to set 3d region"))

    # Create a NULL map to fill the gaps
    null_map = "temporary_null_map_%i" % os.getpid()
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

        ret = grass.run_command("r.to.rast3", input=map_names,
                                output=output, overwrite=grass.overwrite())

        if ret != 0:
            grass.fatal(_("Unable to create raster3d map <%s>" % output))

    grass.run_command("g.remove", rast=null_map)

    title = _("Space time voxel cube")
    descr = _("This space time voxel cube was created with t.rast.to.rast3")

    # Set the unit
    ret = grass.run_command("r3.support", map=output, vunit=unit,
                            title=title, description=descr, 
                            overwrite=grass.overwrite())

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
