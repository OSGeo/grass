#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast.contour
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Produces a space time vector dataset of specified contours from a space time raster dataset.
# COPYRIGHT:    (C) 2015-2017 by the GRASS Development Team
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
#% description: Produces a space time vector dataset of specified contours from a space time raster dataset.
#% keyword: temporal
#% keyword: contour
#% keyword: raster
#% keyword: vector
#% keyword: time
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_STVDS_OUTPUT
#%end

#%option G_OPT_T_WHERE
#%end

#%option
#% key: basename
#% type: string
#% label: Basename of the new generated output maps
#% description: A numerical suffix separated by an underscore will be attached to create a unique identifier
#% required: yes
#% multiple: no
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
#% key: step
#% type: double
#% description: Increment between contour levels
#% required: no
#% multiple: no
#%end

#%option
#% key: levels
#% type: double
#% description: List of contour levels
#% required: no
#% multiple: yes
#%end

#%option
#% key: minlevel
#% type:  double
#% description: Minimum contour level
#% required: no
#% multiple: no
#%end

#%option
#% key: maxlevel
#% type: double
#% description: Maximum contour level
#% required: no
#% multiple: no
#%end

#%option
#% key: cut
#% type: integer
#% description: Minimum number of points for a contour line (0 -> no limit)
#% required: no
#% multiple: no
#% answer: 0
#%end

#%option
#% key: nprocs
#% type: integer
#% description: Number of r.contour processes to run in parallel, more than 1 process works only in conjunction with flag -t
#% required: no
#% multiple: no
#% answer: 1
#%end

#%flag
#% key: n
#% description: Register empty vector maps
#%end

#%flag
#% key: t
#% description: Do not create attribute tables
#%end

import sys
import copy
import grass.script as gscript
from grass.exceptions import FatalError


############################################################################

def main(options, flags):
    # lazy imports
    import grass.temporal as tgis
    import grass.pygrass.modules as pymod

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    base = options["basename"]
    nprocs = int(options["nprocs"])
    step = options["step"]
    levels = options["levels"]
    minlevel = options["minlevel"]
    maxlevel = options["maxlevel"]
    cut = options["cut"]
    time_suffix = options["suffix"]

    register_null = flags["n"]
    t_flag = flags["t"]


    # Make sure the temporal database exists
    tgis.init()
    # We need a database interface
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    overwrite = gscript.overwrite()

    sp = tgis.open_old_stds(input, "strds", dbif)
    maps = sp.get_registered_maps_as_objects(where=where, dbif=dbif)

    if not maps:
        dbif.close()
        gscript.warning(_("Space time raster dataset <%s> is empty") % sp.get_id())
        return

    # Check the new stvds
    new_sp = tgis.check_new_stds(output, "stvds", dbif=dbif,
                                 overwrite=overwrite)

    # Setup the flags
    flags = ""
    if t_flag is True:
        flags += "t"

    # Configure the r.to.vect module
    contour_module = pymod.Module("r.contour", input="dummy",
                                   output="dummy", run_=False,
                                   finish_=False, flags=flags,
                                   overwrite=overwrite,
                                   quiet=True)

    if step:
        contour_module.inputs.step = float(step)
    if minlevel:
        contour_module.inputs.minlevel = float(minlevel)
    if maxlevel:
        contour_module.inputs.maxlevel = float(maxlevel)
    if levels:
        contour_module.inputs.levels = levels.split(",")
    if cut:
        contour_module.inputs.cut = int(cut)

    # The module queue for parallel execution, except if attribute tables should
    # be created. Then force single process use
    if t_flag is False:
        if nprocs > 1:
            nprocs = 1
            gscript.warning(_("The number of parellel r.contour processes was "\
                              "reduced to 1 because of the table attribute "\
                              "creation"))
    process_queue = pymod.ParallelModuleQueue(int(nprocs))

    count = 0
    num_maps = len(maps)
    new_maps = []

    # run r.to.vect all selected maps
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
        new_map = tgis.open_new_map_dataset(map_name, None, type="vector",
                                            temporal_extent=map.get_temporal_extent(),
                                            overwrite=overwrite, dbif=dbif)
        new_maps.append(new_map)

        mod = copy.deepcopy(contour_module)
        mod(input=map.get_id(), output=new_map.get_id())
        sys.stderr.write(mod.get_bash() + "\n")
        process_queue.put(mod)

        if count%10 == 0:
            gscript.percent(count, num_maps, 1)

    # Wait for unfinished processes
    process_queue.wait()

    # Open the new space time vector dataset
    ttype, stype, title, descr = sp.get_initial_values()
    new_sp = tgis.open_new_stds(output, "stvds", ttype, title,
                                descr, stype, dbif, overwrite)
    # collect empty maps to remove them
    num_maps = len(new_maps)
    empty_maps = []

    # Register the maps in the database
    count = 0
    for map in new_maps:
        count += 1

        if count%10 == 0:
            gscript.percent(count, num_maps, 1)

        # Do not register empty maps
        try:
            if map.load() is not True:
                continue
        except FatalError:
            continue
        if map.metadata.get_number_of_primitives() == 0:
            if not register_null:
                empty_maps.append(map)
                continue

        # Insert map in temporal database
        map.insert(dbif)
        new_sp.register_map(map, dbif)

    # Update the spatio-temporal extent and the metadata table entries
    new_sp.update_from_registered_maps(dbif)
    gscript.percent(1, 1, 1)

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

        gscript.run_command("g.remove", flags='f', type='vector', name=names,
                            quiet=True)

    dbif.close()

############################################################################

if __name__ == "__main__":
    options, flags = gscript.parser()
    main(options, flags)
