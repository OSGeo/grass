#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:   t.remove
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:  Remove space time datasets from the temporal database
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
#% description: Removes space time datasets from temporal database.
#% keyword: temporal
#% keyword: map management
#% keyword: remove
#% keyword: time
#%end

#%option G_OPT_STDS_INPUTS
#% guisection: Input
#% required: no
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset, default is strds
#% guidependency: inputs
#% guisection: Input
#% required: no
#% options: strds, str3ds, stvds
#% answer: strds
#%end

#%option G_OPT_F_INPUT
#% key: file
#% description: Input file with dataset names, one per line
#% guisection: Input
#% required: no
#%end

#%flag
#% key: r
#% description: Remove all registered maps from the temporal and also from the spatial database
#%end

#%flag
#% key: f
#% description: Force recursive removing
#%end

import grass.script as grass


# lazy imports at the end of the file

############################################################################

def main():
    # Get the options
    datasets = options["inputs"]
    file = options["file"]
    type = options["type"]
    recursive = flags["r"]
    force = flags["f"]

    if recursive and not force:
        grass.fatal(_("The recursive flag works only in conjunction with the force flag: use -rf"))

    if datasets and file:
        grass.fatal(_("%s= and %s= are mutually exclusive") % ("input", "file"))

    # Make sure the temporal database exists
    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    dataset_list = []

    # Dataset names as comma separated string
    if datasets:
        if datasets.find(",") == -1:
            dataset_list = (datasets,)
        else:
            dataset_list = tuple(datasets.split(","))

    # Read the dataset list from file
    if file:
        fd = open(file, "r")

        line = True
        while True:
            line = fd.readline()
            if not line:
                break

            line_list = line.split("\n")
            dataset_name = line_list[0]
            dataset_list.append(dataset_name)

    statement = ""

    # Create the pygrass Module object for g.remove
    remove = pyg.Module("g.remove", quiet=True, flags='f', run_=False)

    for name in dataset_list:
        name = name.strip()
        sp = tgis.open_old_stds(name, type, dbif)

        if recursive and force:
            grass.message(_("Removing registered maps and %s" % type))
            maps = sp.get_registered_maps_as_objects(dbif=dbif)
            map_statement = ""
            count = 1
            name_list = []
            for map in maps:
                map.select(dbif)
                # We may have multiple layer for a single map, hence we need
                # to avoid multiple deletation of the same map,
                # but the database entries are still present and must be removed
                if map.get_name() not in name_list:
                    name_list.append(str(map.get_name()))
                map_statement += map.delete(dbif=dbif, execute=False)

                count += 1
                # Delete every 100 maps
                if count%100 == 0:
                    dbif.execute_transaction(map_statement)
                    if type == "strds":
                        remove(type="raster", name=name_list, run_=True)
                    if type == "stvds":
                        remove(type="vector", name=name_list, run_=True)
                    if type == "str3ds":
                        remove(type="raster_3d", name=name_list, run_=True)
                    map_statement = ""
                    name_list = []

            if map_statement:
                dbif.execute_transaction(map_statement)
            if name_list:
                if type == "strds":
                    remove(type="raster", name=name_list, run_=True)
                if type == "stvds":
                    remove(type="vector", name=name_list, run_=True)
                if type == "str3ds":
                    remove(type="raster_3d", name=name_list, run_=True)
        else:
            grass.message(_("Note: registered maps themselves have not been removed, only the %s" % type))

        statement += sp.delete(dbif=dbif, execute=False)

    # Execute the collected SQL statenents
    dbif.execute_transaction(statement)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()

    # lazy imports
    import grass.temporal as tgis
    import grass.pygrass.modules as pyg

    tgis.profile_function(main)
