#!/usr/bin/env python3
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
#% description: Performs a neighborhood analysis for each map in a space time raster dataset.
#% keyword: temporal
#% keyword: aggregation
#% keyword: raster
#% keyword: time
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
#% key: suffix
#% type: string
#% description: Suffix to add at basename: set 'gran' for granularity, 'time' for the full time format, 'num' for numerical suffix with a specific number of digits (default %05)
#% answer: gran
#% required: no
#% multiple: no
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

#%flag
#% key: r
#% description: Ignore the current region settings and use the raster map regions
#%end

from __future__ import print_function

import copy
import grass.script as grass


############################################################################

def main():
    # lazy imports
    import grass.temporal as tgis
    import grass.pygrass.modules as pymod

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    size = options["size"]
    base = options["basename"]
    register_null = flags["n"]
    use_raster_region = flags["r"]
    method = options["method"]
    nprocs = options["nprocs"]
    time_suffix = options["suffix"]

    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    overwrite = grass.overwrite()

    sp = tgis.open_old_stds(input, "strds", dbif)
    maps = sp.get_registered_maps_as_objects(where=where, dbif=dbif)

    if not maps:
        dbif.close()
        grass.warning(_("Space time raster dataset <%s> is empty") % sp.get_id())
        return

    new_sp = tgis.check_new_stds(output, "strds", dbif=dbif,
                                               overwrite=overwrite)
    # Configure the r.neighbor module
    neighbor_module = pymod.Module("r.neighbors", input="dummy",
                                   output="dummy", run_=False,
                                   finish_=False, size=int(size),
                                   method=method, overwrite=overwrite,
                                   quiet=True)

    gregion_module =  pymod.Module("g.region", raster="dummy", run_=False,
                                   finish_=False,)

    # The module queue for parallel execution
    process_queue = pymod.ParallelModuleQueue(int(nprocs))

    count = 0
    num_maps = len(maps)
    new_maps = []

    # run r.neighbors all selected maps
    for map in maps:
        count += 1
        if sp.get_temporal_type() == 'absolute' and time_suffix == 'gran':
            suffix = tgis.create_suffix_from_datetime(map.temporal_extent.get_start_time(),
                                                      sp.get_granularity())
            map_name = "{ba}_{su}".format(ba=base, su=suffix)
        elif sp.get_temporal_type() == 'absolute' and time_suffix == 'time':
            suffix = tgis.create_time_suffix(map)
            map_name = "{ba}_{su}".format(ba=base, su=suffix)
        else:
            map_name = tgis.create_numeric_suffix(base, count, time_suffix)

        new_map = tgis.open_new_map_dataset(map_name, None, type="raster",
                                            temporal_extent=map.get_temporal_extent(),
                                            overwrite=overwrite, dbif=dbif)
        new_maps.append(new_map)

        mod = copy.deepcopy(neighbor_module)
        mod(input=map.get_id(), output=new_map.get_id())

        if use_raster_region is True:
            reg = copy.deepcopy(gregion_module)
            reg(raster=map.get_id())
            print(reg.get_bash())
            print(mod.get_bash())
            mm = pymod.MultiModule([reg, mod], sync=False, set_temp_region=True)
            process_queue.put(mm)
        else:
            print(mod.get_bash())
            process_queue.put(mod)

    # Wait for unfinished processes
    process_queue.wait()
    proc_list = process_queue.get_finished_modules()

    # Check return status of all finished modules
    error = 0
    for proc in proc_list:
        if proc.popen.returncode != 0:
            grass.error(_("Error running module: %\n    stderr: %s") %(proc.get_bash(), proc.outputs.stderr))
            error += 1

    if error > 0:
        grass.fatal(_("Error running modules."))

    # Open the new space time raster dataset
    ttype, stype, title, descr = sp.get_initial_values()
    new_sp = tgis.open_new_stds(output, "strds", ttype, title,
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

        grass.run_command("g.remove", flags='f', type='raster', name=names, quiet=True)

    dbif.close()

############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
