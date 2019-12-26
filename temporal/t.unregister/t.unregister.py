#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.unregister
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Unregister raster, vector and raster3d maps from the temporal database or a specific space time dataset
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
#% description: Unregisters raster, vector and raster3d maps from the temporal database or a specific space time dataset.
#% keyword: temporal
#% keyword: map management
#% keyword: unregister
#% keyword: time
#%end

#%option G_OPT_STDS_INPUT
#% required: no
#%end

#%option G_OPT_F_INPUT
#% key: file
#% description: Input file with map names, one per line
#% required: no
#%end

#%option G_OPT_MAP_TYPE
#% guidependency: input,maps
#%end


#%option G_OPT_MAP_INPUTS
#% description: Name(s) of existing raster, vector or raster3d map(s) to unregister
#% required: no
#%end

import grass.script as grass


# lazy imports at the end of the file

############################################################################


def main():
    # Get the options
    file = options["file"]
    input = options["input"]
    maps = options["maps"]
    type = options["type"]

    # Make sure the temporal database exists
    tgis.init()

    if maps and file:
        grass.fatal(_(
            "%s= and %s= are mutually exclusive") % ("input", "file"))

    if not maps and not file:
        grass.fatal(_("%s= or %s= must be specified") % ("input", "file"))

    mapset = grass.gisenv()["MAPSET"]

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    # In case a space time dataset is specified
    if input:
        sp = tgis.open_old_stds(input, type, dbif)

    maplist = []

    dummy = tgis.RasterDataset(None)

    # Map names as comma separated string
    if maps is not None and maps != "":
        if maps.find(",") == -1:
            maplist = [maps, ]
        else:
            maplist = maps.split(",")

        # Build the maplist
        for count in range(len(maplist)):
            mapname = maplist[count]
            mapid = dummy.build_id(mapname, mapset)
            maplist[count] = mapid

    # Read the map list from file
    if file:
        fd = open(file, "r")

        line = True
        while True:
            line = fd.readline()
            if not line:
                break

            mapname = line.strip()
            mapid = dummy.build_id(mapname, mapset)
            maplist.append(mapid)

    num_maps = len(maplist)
    update_dict = {}
    count = 0

    statement = ""

    # Unregister already registered maps
    grass.message(_("Unregister maps"))
    for mapid in maplist:
        if count%10 == 0:
            grass.percent(count, num_maps, 1)

        map = tgis.dataset_factory(type, mapid)

        # Unregister map if in database
        if map.is_in_db(dbif) == True:
            # Unregister from a single dataset
            if input:
                # Collect SQL statements
                statement += sp.unregister_map(
                    map=map, dbif=dbif, execute=False)

            # Unregister from temporal database
            else:
                # We need to update all datasets after the removement of maps
                map.metadata.select(dbif)
                datasets = map.get_registered_stds(dbif)
                # Store all unique dataset ids in a dictionary
                if datasets:
                    for dataset in datasets:
                        update_dict[dataset] = dataset
                # Collect SQL statements
                statement += map.delete(dbif=dbif, update=False, execute=False)
        else:
            grass.warning(_("Unable to find %s map <%s> in temporal database" %
                            (map.get_type(), map.get_id())))

        count += 1

    # Execute the collected SQL statenents
    if statement:
        dbif.execute_transaction(statement)

    grass.percent(num_maps, num_maps, 1)

    # Update space time datasets
    if input:
        grass.message(_("Unregister maps from space time dataset <%s>"%(input)))
    else:
        grass.message(_("Unregister maps from the temporal database"))

    if input:
        sp.update_from_registered_maps(dbif)
        sp.update_command_string(dbif=dbif)
    elif len(update_dict) > 0:
        count = 0
        for key in update_dict.keys():
            id = update_dict[key]
            sp = tgis.open_old_stds(id, type, dbif)
            sp.update_from_registered_maps(dbif)
            grass.percent(count, len(update_dict), 1)
            count += 1

    dbif.close()

###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()

    # lazy imports
    import grass.temporal as tgis

    tgis.profile_function(main)
