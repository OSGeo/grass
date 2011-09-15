#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.unregister
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Unregister raster maps from space time raster datasets
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Unregister raster map(s) from a specific or all space time raster dataset in which it is registered
#% keywords: spacetime raster dataset
#% keywords: raster
#%end

#%option
#% key: dataset
#% type: string
#% description: Name of an existing space time raster dataset. If no name is provided the raster map(s) are unregistered from all space time datasets in which they are registered.
#% required: no
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing raster map(s) to unregister
#% required: yes
#% multiple: yes
#%end

import grass.script as grass
############################################################################

def main():

    # Get the options
    name = options["dataset"]
    maps = options["maps"]

    # Make sure the temporal database exists
    grass.create_temporal_database()

    mapset =  grass.gisenv()["MAPSET"]
    
    # In case a space time raster dataset is specified
    if name:
    
        id = name + "@" + mapset

        sp = grass.space_time_raster_dataset(id)
        # Read content from db
        sp.select()

        if sp.is_in_db() == False:
            grass.fatal("Space time " + sp.get_new_map_instance(None).get_type() + " dataset <" + name + "> not found")

    # Build the list of maps
    if maps.find(",") == -1:
        maplist = (maps,)
    else:
        maplist = tuple(maps.split(","))

    for mapname in maplist:
        mapid = mapname + "@" + mapset
        map = grass.raster_dataset(mapid)

        # Unregister map if in database
        if map.is_in_db() == True:
            if name:
                sp.unregister_map(map)
            else:
                map.select()
                map.unregister()

if __name__ == "__main__":
    options, flags = grass.core.parser()
    main()

