#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.neighbors
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Performs a neighborhood analysis for each map in a space time
#               raster dataset.
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

    overwrite = grass.overwrite()

    mapset = grass.gisenv()["MAPSET"]

    sp = tgis.open_old_space_time_dataset(input, "strds", dbif)
    maps = sp.get_registered_maps_as_objects(where=where, dbif=dbif)

    if not maps:
        dbif.close()
        grass.fatal(_("Space time raster dataset <%s> is empty") % sp.get_id())

    new_sp = tgis.check_new_space_time_dataset(input, "strds", dbif=dbif,
                                               overwrite=overwrite)

    count = 0
    proc_list = []

    num_maps = len(maps)
    new_maps = []

    for map in maps:
        count += 1

        if count%10 == 0:
            grass.percent(count, num_maps, 1)

        map_name = "%s_%i" % (base, count)
        new_map = tgis.open_new_map_dataset(map_name, None, mapset,
                                       type="raster",
                                       temporal_extent=map.get_temporal_extent(),
                                       overwrite=overwrite, dbif=dbif)

        proc_list.append(Process(target=run_neighbors,
                                     args=(map.get_id(),map_name,
                                           method,size, overwrite)))
        proc_list[-1].start()

        # Join processes if the maximum number of processes are
        # reached or the end of the loop is reached
        if len(proc_list) == nprocs:
            for proc in proc_list:
                proc.join()
                if proc.exitcode != 0:
                    dbif.close()
                    grass.fatal(_("Error while neighborhood computation"))

            # Empty process list
            proc_list = []

        # Initlialize and load the content of the map
        new_maps.append(new_map)

    for proc in proc_list:
        proc.join()
        if proc.exitcode != 0:
            dbif.close()
            grass.fatal(_("Error while computation"))

    grass.percent(1, 1, 1)

    # Open the new space time raster dataset
    temporal_type, semantic_type, title, description = sp.get_initial_values()
    new_sp = tgis.open_new_space_time_dataset(output, "strds",
                                              sp.get_temporal_type(),
                                              title, description,
                                              semantic_type,
                                              dbif, overwrite)

    # collect empty maps to remove them
    num_maps = len(new_maps)
    empty_maps = []

    # Register the maps in the database
    count = 0
    for map in new_maps:
        count += 1

        if count%10 == 0:
            grass.percent(count, num_maps, 1)

        # In case of a empty map continue, do not register empty maps
        map.load()
        if map.metadata.get_min() is None and \
            map.metadata.get_max() is None:
            if not register_null:
                empty_maps.append(map)
                continue

        # Insert map in temporal database
        map.insert(dbif)
        new_sp.register_map(map, dbif)

    # Update the spatio-temporal extent and the metadata table entries
    new_sp.update_from_registered_maps(dbif)
    grass.percent(1, 1, 1)

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

############################################################################

def run_neighbors(input, output, method, size, overwrite):
    """Helper function to run r.neighbors in parallel"""
    return grass.run_command("r.neighbors", input=input, output=output,
                            method=method, size=size,
                            overwrite=overwrite,
                            quiet=True)

############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
