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
#% description: Aggregates temporally the maps of a space time raster dataset by a user defined granularity.
#% keywords: temporal
#% keywords: aggregation
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_STRDS_OUTPUT
#%end

#%option
#% key: basename
#% type: string
#% label: Basename of the new generated output maps
#% description: A numerical suffix separated by an underscore will be attached to create a unique identifier
#% required: yes
#% multiple: no
#% gisprompt:
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

#%option
#% key: offset
#% type: integer
#% description: Offset that is used to create the output map ids, output map id is generated as: basename_ (count + offset)
#% required: no
#% multiple: no
#% answer: 0
#%end

#%option G_OPT_T_SAMPLE
#%end

#%option G_OPT_T_WHERE
#%end

#%flag
#% key: n
#% description: Register Null maps
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    gran = options["granularity"]
    base = options["basename"]
    register_null = flags["n"]
    method = options["method"]
    sampling = options["sampling"]
    offset = options["offset"]

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    sp = tgis.open_old_space_time_dataset(input, "strds", dbif)
    temporal_type, semantic_type, title, description = sp.get_initial_values()
    new_sp = tgis.open_new_space_time_dataset(output, "strds", temporal_type,
                                              title, description, semantic_type,
                                              dbif, grass.overwrite())

    rows = sp.get_registered_maps("id,start_time,end_time", where, "start_time", dbif)

    if not rows:
        dbif.close()
        grass.fatal(_("Space time raster dataset <%s> is empty") % input)

    # Modify the start time to fit the granularity

    if sp.is_time_absolute():
        first_start_time = tgis.adjust_datetime_to_granularity(
            rows[0]["start_time"], gran)
    else:
        first_start_time = rows[0]["start_time"]

    # We use the end time first
    last_start_time = rows[len(rows) - 1]["end_time"]
    is_end_time = True

    # In case no end time is available, then we use the start time
    if last_start_time is None:
        last_start_time = rows[len(rows) - 1]["start_time"]
        is_end_time = False

    next_start_time = first_start_time

    count = 0

    while True:
        if is_end_time is True:
            if next_start_time >= last_start_time:
                break
        else:
            if next_start_time > last_start_time:
                break

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
                count, method, register_null, dbif,  offset)

            if new_map:
                # Set the time stamp and write it to the raster map
                if sp.is_time_absolute():
                    new_map.set_absolute_time(start, end)
                else:
                    new_map.set_relative_time(start,
                                              end, sp.get_relative_time_unit())

                # Insert map in temporal database
                new_map.insert(dbif)
                new_sp.register_map(new_map, dbif)

                count += 1

    # Update the spatio-temporal extent and the raster metadata table entries
    new_sp.set_aggregation_type(method)
    new_sp.metadata.update(dbif)
    new_sp.update_from_registered_maps(dbif)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
