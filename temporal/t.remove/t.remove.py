#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.remove
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Remove space time datasets from the temporal database
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Removes space time datasets from temporal database.
#% keywords: temporal
#% keywords: map management
#% keywords: remove
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
#% description: Remove all registered maps from the temporal and spatial database
#%end

#%flag
#% key: f
#% description: Force recursive removing
#%end
import grass.script as grass
import grass.temporal as tgis
import grass.pygrass.modules as pyg

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
    remove = pyg.Module("g.remove", quiet=True, run_=False)

    for name in dataset_list:
        name = name.strip()
        sp = tgis.open_old_space_time_dataset(name, type, dbif)

        if recursive and force:
            grass.message(_("Removing registered maps"))
            maps = sp.get_registered_maps_as_objects(dbif=dbif)
            map_statement = ""
            count = 1
            name_list = []
            for map in maps:
                map.select(dbif)
                name_list.append(map.get_name())
                map_statement += map.delete(dbif=dbif, execute=False)

                count += 1
                # Delete every 100 maps
                if count%100 == 0:
                    dbif.execute_transaction(map_statement)
                    remove(rast=name_list, run_=True)
                    map_statement = ""
                    name_list = []

            if map_statement:
                dbif.execute_transaction(map_statement)
            if name_list:
                remove(rast=name_list, run_=True)

        statement += sp.delete(dbif=dbif, execute=False)

    # Execute the collected SQL statenents
    dbif.execute_transaction(statement)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
