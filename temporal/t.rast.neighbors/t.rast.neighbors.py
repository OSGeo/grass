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

import copy
import grass.script as grass
import grass.temporal as tgis
import grass.pygrass.modules as pymod


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
        grass.warning(_("Space time raster dataset <%s> is empty") % sp.get_id())
        return

    new_sp = tgis.check_new_space_time_dataset(input, "strds", dbif=dbif,
                                               overwrite=overwrite)
    # Configure the r.neighbor module
    neighbor_module = pymod.Module("r.neighbors", input="dummy",
                                   output="dummy", run_=False,
                                   finish_=False, size=int(size),
                                   method=method, overwrite=overwrite,
                                   quiet=True)
    # The module queue for parallel execution
    process_queue = pymod.ParallelModuleQueue(int(nprocs))

    count = 0
    num_maps = len(maps)
    new_maps = []

    # run r.neighbors all selected maps
    for map in maps:
        count += 1
        map_name = "%s_%i" % (base, count)
        new_map = tgis.open_new_map_dataset(map_name, None, mapset,
                                            type="raster",
                                            temporal_extent=map.get_temporal_extent(),
                                            overwrite=overwrite, dbif=dbif)
        new_maps.append(new_map)

        mod = copy.deepcopy(neighbor_module)
        mod(input=map.get_id(), output=new_map.get_id())
        print(mod.get_bash())
        process_queue.put(mod)

    # Wait for unfinished processes
    process_queue.wait()

    # Open the new space time raster dataset
    ttype, stype, title, descr = sp.get_initial_values()
    new_sp = tgis.open_new_space_time_dataset(output, "strds", ttype, title,
                                              descr, stype, dbif, overwrite)
    num_maps = len(new_maps)
    # collect empty maps to remove them
    empty_maps = []

    # Register the maps in the database
    count = 0
    for map in new_maps:
        count += 1

        if count%10 == 0:
            grass.percent(count, num_maps, 1)

        # Do not register empty maps
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
                count += 1
                names += "%s" % (map.get_name())
            else:
                names += ",%s" % (map.get_name())

        grass.run_command("g.remove", rast=names, quiet=True)

    dbif.close()

############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
