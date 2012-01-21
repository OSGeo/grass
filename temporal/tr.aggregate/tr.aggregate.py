#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.aggregate
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Create a new space time raster dataset from the aggregated data of an existing space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Create a new space time raster dataset from the aggregated data of an existing space time raster dataset
#% keywords: spacetime raster dataset
#% keywords: raster
#% keywords: aggregation
#%end

#%option
#% key: input
#% type: string
#% description: Name of a space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option G_OPT_DB_WHERE
#%end

#%option
#% key: granularity
#% type: string
#% description: The aggregation granularity, format absolue time "x years, x months, x weeks, x days, x hours, x minutes, x seconds" or a double value for relative time
#% required: yes
#% multiple: no
#%end

#%option
#% key: output
#% type: string
#% description: Name of the output space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: method
#% type: string
#% description: Aggregate operation to be peformed on the raster maps
#% required: yes
#% multiple: no
#% options: average,count,median,mode,minimum,min_raster,maximum,max_raster,stddev,range,sum,variance,diversity,slope,offset,detcoeff,quart1,quart3,perc90,quantile,skewness,kurtosis
#% answer: average
#%end

#%option
#% key: sampling
#% type: string
#% description: The method to be used for sampling the input dataset
#% required: no
#% multiple: yes
#% options: start,during,overlap,contain,equal
#% answer: start
#%end

#%option
#% key: base
#% type: string
#% description: Base name of the new created raster maps
#% required: yes
#% multiple: no
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
    base = options["base"]
    register_null = flags["n"]
    method = options["method"]
    sampling = options["sampling"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # We need a database interface
    dbif = tgis.sql_database_interface()
    dbif.connect()
   
    mapset =  grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)
    
    if sp.is_in_db() == False:
        dbif.close()
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select(dbif)

    if output.find("@") >= 0:
        out_id = output
    else:
        out_id = output + "@" + mapset

    # The new space time raster dataset
    new_sp = tgis.space_time_raster_dataset(out_id)
    if new_sp.is_in_db(dbif):
        if grass.overwrite() == True:
            new_sp.delete(dbif)
            new_sp = tgis.space_time_raster_dataset(out_id)
        else:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> is already in database, use overwrite flag to overwrite") % out_id)

    temporal_type, semantic_type, title, description = sp.get_initial_values()
    new_sp.set_initial_values(temporal_type, semantic_type, title, description)
    new_sp.insert(dbif)

    rows = sp.get_registered_maps("id,start_time", where, "start_time", dbif)

    if not rows:
            dbif.close()
            grass.fatal(_("Space time raster dataset <%s> is empty") % out_id)

    # Modify the start time to fit the granularity

    if sp.is_time_absolute():
        first_start_time = tgis.adjust_datetime_to_granularity( rows[0]["start_time"], gran)
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

        input_map_names = tgis.collect_map_names(sp, dbif, start, end, sampling)

        if input_map_names:
            tgis.aggregate_raster_maps(sp, new_sp, mapset, input_map_names, base, start, end, count, method, register_null, dbif)

        count += 1

    # Update the spatio-temporal extent and the raster metadata table entries
    new_sp.update_from_registered_maps(dbif)
        
    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

