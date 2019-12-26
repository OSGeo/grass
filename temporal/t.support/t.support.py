#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.support
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Modify the metadata of a space time dataset
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
#% description: Modifies the metadata of a space time dataset.
#% keyword: temporal
#% keyword: metadata
#% keyword: time
#%end

#%option G_OPT_STDS_INPUT
#%end

#%option
#% key: semantictype
#% type: string
#% description: Semantic type of the space time dataset
#% required: no
#% multiple: no
#% options: min,max,sum,mean
#% answer: mean
#%end

#%option G_OPT_STDS_TYPE
#% guidependency: input
#% guisection: Required
#%end

#%option
#% key: title
#% type: string
#% description: Title of the space time dataset
#% required: no
#% multiple: no
#%end

#%option
#% key: description
#% type: string
#% description: Description of the space time dataset
#% required: no
#% multiple: no
#%end

#%option
#% key:  aggr_type
#% type: string
#% description: Aggregation type of the space time raster or 3D raster dataset
#% required: no
#% multiple: no
#%end

#%flag
#% key: m
#% label: Update the metadata information and spatial extent of registered maps from the GRASS spatial database
#% description: Check for removed maps and delete them from the temporal database and all effected space time datasets
#%end

#%flag
#% key: u
#% description: Update metadata information, temporal and spatial extent from registered maps based on database entries.
#%end


import grass.script as grass


############################################################################

def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    name = options["input"]
    type = options["type"]
    title = options["title"]
    aggr_type = options["aggr_type"]
    description = options["description"]
    semantic = options["semantictype"]
    update = flags["u"]
    map_update = flags["m"]

    # Make sure the temporal database exists
    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    stds = tgis.open_old_stds(name, type, dbif)

    update = False
    if aggr_type and type == "stvds":
        return()
    
    if aggr_type and type != "stvds":
        stds.metadata.set_aggregation_type(aggregation_type=aggr_type)
        update = True
    if title:
        stds.metadata.set_title(title=title)
        update = True
        # Update only non-null entries
    if description:
        stds.metadata.set_description(description=description)
        update = True
    if semantic:
        stds.base.set_semantic_type(semantic_type=semantic)
        update = True

    if update:
        stds.update(dbif=dbif)

    if map_update:
        #Update the registered maps from the grass spatial database
        statement = ""
        # This dict stores the datasets that must be updated
        dataset_dict = {}

        count = 0
        maps = stds.get_registered_maps_as_objects(dbif=dbif)

        # We collect the delete and update statements
        for map in maps:

            count += 1
            if count%10 == 0:
                grass.percent(count, len(maps), 1)

            map.select(dbif=dbif)

            # Check if the map is present in the grass spatial database
            # Update if present, delete if not present
            if map.map_exists():
                # Read new metadata from the spatial database
                map.load()
                statement += map.update(dbif=dbif, execute=False)
            else:
                # Delete the map from the temporal database
                # We need to update all effected space time datasets
                datasets = map.get_registered_stds(dbif)
                if datasets:
                    for dataset in datasets:
                        dataset_dict[dataset] = dataset
                # Collect the delete statements
                statement += map.delete(dbif=dbif, update=False, execute=False)

        # Execute the collected SQL statements
        dbif.execute_transaction(statement)

        # Update the effected space time datasets
        for id in dataset_dict:
            stds_new = stds.get_new_instance(id)
            stds_new.select(dbif=dbif)
            stds_new.update_from_registered_maps(dbif=dbif)

    if map_update or update:
        stds.update_from_registered_maps(dbif=dbif)

    stds.update_command_string(dbif=dbif)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
