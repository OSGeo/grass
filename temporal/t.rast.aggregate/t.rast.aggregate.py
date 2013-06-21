#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.aggregate
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Temporally aggregates the maps of a space time raster dataset by a user defined granularity.
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Temporally aggregates the maps of a space time raster dataset by a user defined granularity.
#% keywords: temporal
#% keywords: aggregation
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_STRDS_OUTPUT
#%end

#%option G_OPT_R_BASE
#%end

#%option
#% key: granularity
#% type: string
#% description: Aggregation granularity, format absolute time "x years, x months, x weeks, x days, x hours, x minutes, x seconds" or an integer value for relative time
#% required: yes
#% multiple: no
#%end

#%option
#% key: method
#% type: string
#% description: Aggregate operation to be performed on the raster maps
#% required: yes
#% multiple: no
#% options: average,count,median,mode,minimum,min_raster,maximum,max_raster,stddev,range,sum,variance,diversity,slope,offset,detcoeff,quart1,quart3,perc90,quantile,skewness,kurtosis
#% answer: average
#%end

#%option G_OPT_T_SAMPLE
#%end

#%option G_OPT_T_WHERE
#%end

#%flag
#% key: n
#% description: Register Null maps
#%end

from multiprocessing import Process
import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    gran = options["granularity"]
    base = options["base"]
    register_null = flags["n"]
    method = options["method"]
    sampling = options["sampling"]

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = tgis.SpaceTimeRasterDataset(id)

    if sp.is_in_db() == False:
        dbif.close()
        grass.fatal(_("Space time %s dataset <%s> not found") % (
            sp.get_new_map_instance(None).get_type(), id))

    sp.select(dbif)

    if output.find("@") >= 0:
        out_id = output
    else:
        out_id = output + "@" + mapset

    # The new space time raster dataset
    new_sp = tgis.SpaceTimeRasterDataset(out_id)
    if new_sp.is_in_db(dbif):
        if grass.overwrite() == True:
            new_sp.delete(dbif)
            new_sp = tgis.SpaceTimeRasterDataset(out_id)
        else:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> is already in the "
                          "database, use overwrite flag to overwrite") % out_id)

    temporal_type, semantic_type, title, description = sp.get_initial_values()
    new_sp.set_initial_values(temporal_type, semantic_type, title, description)
    new_sp.insert(dbif)

    rows = sp.get_registered_maps("id,start_time", where, "start_time", dbif)

    if not rows:
        dbif.close()
        grass.fatal(_("Space time raster dataset <%s> is empty") % out_id)

    # Modify the start time to fit the granularity

    if sp.is_time_absolute():
        first_start_time = tgis.adjust_datetime_to_granularity(
            rows[0]["start_time"], gran)
    else:
        first_start_time = rows[0]["start_time"]

    last_start_time = rows[len(rows) - 1]["start_time"]
    next_start_time = first_start_time

    count = 0

    while next_start_time <= last_start_time:
        start = next_start_time
        if sp.is_time_absolute():
            end = tgis.increment_datetime_by_string(next_start_time, gran)
        else:
            end = next_start_time + int(gran)
        next_start_time = end

        input_map_names = tgis.collect_map_names(
            sp, dbif, start, end, sampling)

        if input_map_names:
            new_map = tgis.aggregate_raster_maps(
                input_map_names, base, start, end,
                count, method, register_null, dbif)

            if new_map:
                # Set the time stamp and write it to the raster map
                if sp.is_time_absolute():
                    new_map.set_absolute_time(start, end, None)
                else:
                    new_map.set_relative_time(start,
                                              end, sp.get_relative_time_unit())

                # Insert map in temporal database
                new_map.insert(dbif)
                new_sp.register_map(new_map, dbif)

                count += 1

    # Update the spatio-temporal extent and the raster metadata table entries
    new_sp.update_from_registered_maps(dbif)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
