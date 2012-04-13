#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.support
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Modify the metadata of a space time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Modify the metadata of a space time dataset
#% keywords: temporal
#% keywords: support
#%end

#%option G_OPT_STDS_INPUT
#%end

#%option
#% key: semantictype
#% type: string
#% description: The semantic type of the space time dataset
#% required: no
#% multiple: no
#% options: min,max,sum,mean
#% answer: mean
#%end

#%option G_OPT_STDS_TYPE
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

#%flag
#% key: m
#% description: Update the metadata information and spatial extent of registered maps from the grass spatial database
#%end

#%flag
#% key: u
#% description: Update metadata information, temporal and spatial extent from registered maps
#%end

import grass.temporal as tgis
import grass.script as grass

############################################################################

def main():

    # Get the options
    name = options["input"]
    type = options["type"]
    title = options["title"]
    descr = options["description"]
    semantic = options["semantictype"]
    update = flags["u"]
    map_update = flags["m"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    #Get the current mapset to create the id of the space time dataset
    mapset =  grass.gisenv()["MAPSET"]

    if name.find("@") >= 0:
        id = name
    else:
        id = name + "@" + mapset
        
    dbif = tgis.sql_database_interface_connection()
    dbif.connect()

    sp = tgis.dataset_factory(type, id)

    if sp.is_in_db(dbif=dbif) == False:
        dbif.close()
        grass.fatal(_("Space time %s dataset <%s> not found") % (sp.get_new_map_instance(None).get_type(), id))

    sp.select(dbif=dbif)
    # Temporal type can not be changed
    ttype= sp.get_temporal_type()
    sp.set_initial_values(temporal_type=ttype, semantic_type=semantic, title=title, description=descr)
    # Update only non-null entries
    sp.update(dbif=dbif)

    if map_update:
        #Update the registered maps from the grass spatial database
        statement = ""

        count = 0
        maps = sp.get_registered_maps_as_objects(dbif=dbif)
        for map in maps:
            map.select(dbif=dbif)
            map.load()
            statement += map.update(dbif=dbif, execute=False)
            grass.percent(count, len(maps), 1)
	    count += 1

        # Execute the collected SQL statenents
        dbif.execute_transaction(statement)

    if map_update or update:
        sp.update_from_registered_maps(dbif=dbif)

    dbif.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

