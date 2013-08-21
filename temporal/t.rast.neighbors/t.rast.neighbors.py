#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.neighbors
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Performs a neighborhood analysis for each map in a space time raster dataset.
# COPYRIGHT:    (C) 2013 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Performs a neighborhood analysis for each map in a space time raster dataset.
#% keywords: temporal
#% keywords: aggregation
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_STRDS_OUTPUT
#%end

#%option G_OPT_T_WHERE
#%end

#%option
#% key: size
#% type: integer
#% description: Neighborhood size
#% required: no
#% multiple: no
#% answer: 3
#%end

#%option
#% key: method
#% type: string
#% description: Aggregate operation to be performed on the raster maps
#% required: yes
#% multiple: no
#% options: average,median,mode,minimum,maximum,range,stddev,sum,count,variance,diversity,interspersion,quart1,quart3,perc90,quantile
#% answer: average
#%end

#%option G_OPT_R_BASE
#%end

#%option
#% key: nprocs
#% type: integer
#% description: Number of r.neighbor processes to run in parallel
#% required: no
#% multiple: no
#% answer: 1
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
    size = options["size"]
    base = options["base"]
    register_null = flags["n"]
    method = options["method"]
    nprocs = options["nprocs"]

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    mapset = grass.gisenv()["MAPSET"]

    sp = tgis.open_old_space_time_dataset(input, "strds", dbif)
    dummy = sp.get_new_map_instance(None)

    temporal_type, semantic_type, title, description = sp.get_initial_values()
    new_sp = tgis.open_new_space_time_dataset(output, "strds", sp.get_temporal_type(),
                                              title, description, semantic_type,
                                              dbif, grass.overwrite(), dry=True)

    rows = sp.get_registered_maps("id,start_time", where, "start_time", dbif)

    if not rows:
        dbif.close()
        grass.fatal(_("Space time raster dataset <%s> is empty") % out_id)

    count = 0
    proc_count = 0
    proc_list = []

    num_rows = len(rows)
    new_maps = {}

    for row in rows:
        count += 1

        if count%10 == 0:
            grass.percent(count, num_rows, 1)

        map_name = "%s_%i" % (base, count)
        map_id = dummy.build_id(map_name, mapset)

        new_map = sp.get_new_map_instance(map_id)

        # Check if new map is in the temporal database
        if new_map.is_in_db(dbif):
            if grass.overwrite():
                # Remove the existing temporal database entry
                new_map.delete(dbif)
                new_map = sp.get_new_map_instance(map_id)
            else:
                grass.error(_("Map <%s> is already in temporal database,"
                             " use overwrite flag to overwrite") %
                            (new_map.get_map_id()))
                continue

        proc_list.append(Process(target=run_neighbors,
                                     args=(row["id"],map_name,method,size)))

        proc_list[proc_count].start()
        proc_count += 1

        # Join processes if the maximum number of processes are
        # reached or the end of the loop is reached
        if proc_count == nprocs or proc_count == num_rows:
            proc_count = 0
            exitcodes = 0
            for proc in proc_list:
                proc.join()
                exitcodes += proc.exitcode

            if exitcodes != 0:
                dbif.close()
                grass.fatal(_("Error while computation"))

            # Empty process list
            proc_list = []

        # Store the new maps
        new_maps[row["id"]] = new_map

    grass.percent(0, num_rows, 1)

    new_sp = tgis.open_new_space_time_dataset(output, "strds",
                                              sp.get_temporal_type(),
                                              title, description,
                                              semantic_type,
                                              dbif, grass.overwrite(),
                                              dry=False)

    # collect empty maps to remove them
    empty_maps = []

    # Register the maps in the database
    count = 0
    for row in rows:
        count += 1
        if count%10 == 0:
            grass.percent(count, num_rows, 1)
        # Register the new maps
        if row["id"] in new_maps:
            new_map = new_maps[row["id"]]
            # Read the raster map data
            new_map.load()

            # In case of a empty map continue, do not register empty maps
            if new_map.metadata.get_min() is None and \
                new_map.metadata.get_max() is None:
                if not register_null:
                    empty_maps.append(new_map)
                    continue

            old_map = sp.get_new_map_instance(row["id"])
            old_map.select(dbif)

            # Set the time stamp
            if old_map.is_time_absolute():
                start, end, tz = old_map.get_absolute_time()
                new_map.set_absolute_time(start, end, tz)
            else:
                start, end, unit = old_map.get_relative_time()
                new_map.set_relative_time(start, end, unit)

            # Insert map in temporal database
            new_map.insert(dbif)
            new_sp.register_map(new_map, dbif)

    # Update the spatio-temporal extent and the metadata table entries
    new_sp.update_from_registered_maps(dbif)
    grass.percent(num_rows, num_rows, 1)

    # Remove empty maps
    if len(empty_maps) > 0:
        names = ""
        count = 0
        for map in empty_maps:
            if count == 0:
                names += "%s" % (map.get_name())
            else:
                names += ",%s" % (map.get_name())
            count += 1

        grass.run_command("g.remove", rast=names, quiet=True)


    dbif.close()

def run_neighbors(input, output, method, size):
    """Helper function to run r.neighbors in parallel"""
    return grass.run_command("r.neighbors", input=input, output=output,
                            method=method, size=size, overwrite=grass.overwrite(),
                            quiet=True)


if __name__ == "__main__":
    options, flags = grass.parser()
    main()
