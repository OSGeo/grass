#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.unregister
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Unregister raster, vector and raster3d maps from the temporal database or a specific space time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Unregister raster, vector and raster3d maps from the temporal database or a specific space time dataset
#% keywords: temporal
#% keywords: unregister
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
#%end


#%option G_OPT_MAP_INPUTS
#% description: Name(s) of existing raster, vector or raster3d map(s) to unregister
#% required: no
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    file = options["file"]
    name = options["input"]
    maps = options["maps"]
    type = options["type"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Unregister maps
    unregister_maps_from_space_time_datasets(type=type, name=name, maps=maps, file=file, dbif=None)

############################################################################

def unregister_maps_from_space_time_datasets(type, name, maps, file=None, dbif=None):
    """Unregister maps from a single space time dataset or, in case no dataset name is provided,
       unregister from all datasets within the maps are registered.

       @param type: The type of the maps raster, vector or raster3d
       @param name: Name of an existing space time raster dataset. If no name is provided the raster map(s) are unregistered from all space time datasets in which they are registered.
       @param maps: A comma separated list of map names
       @param file: Input file one map per line
       @param dbif: The database interface to be used
    """

    if maps and file:
        grass.fatal(_("%s= and %s= are mutually exclusive") % ("input","file"))

    if not maps and not file:
        grass.fatal(_("%s= or %s= must be specified") % ("input","file"))


    mapset =  grass.gisenv()["MAPSET"]

    if dbif == None:
        dbif = tgis.sql_database_interface()
        dbif.connect()
        connect = True

    # In case a space time dataset is specified
    if name:
        # Check if the dataset name contains the mapset as well
        if name.find("@") < 0:
            id = name + "@" + mapset
        else:
            id = name

        if type == "rast":
            sp = tgis.dataset_factory("strds", id)
        if type == "rast3d":
            sp = tgis.dataset_factory("str3ds", id)
        if type == "vect":
            sp = tgis.dataset_factory("stvds", id)

        if sp.is_in_db(dbif) == False:
            dbif.close()
            grass.fatal("Space time " + sp.get_new_map_instance(None).get_type() + " dataset <" + name + "> not found")

    maplist = []

    dummy = tgis.raster_dataset(None)

    # Map names as comma separated string
    if maps != None:
	if maps.find(",") == -1:
	    maplist = [maps,]
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
    for mapid in maplist:
	grass.percent(count, num_maps, 1)
            
        map = tgis.dataset_factory(type, mapid)

        # Unregister map if in database
        if map.is_in_db(dbif) == True:
            # Unregister from a single dataset
            if name:
                sp.metadata.select(dbif)
                # Collect SQL statements
                statement += sp.unregister_map(map=map, dbif=dbif, execute=False)
 
            # Unregister from temporal database
            else:
                # We need to update all datasets after the removement of maps
                map.metadata.select(dbif)
                datasets = map.get_registered_datasets(dbif)
                # Store all unique dataset ids in a dictionary
                if datasets:
                    for dataset in datasets:
                        update_dict[dataset["id"]] = dataset["id"]
                # Collect SQL statements
                statement += map.delete(dbif=dbif, update=False, execute=False)
		
	count += 1

    # Execute the collected SQL statenents
    sql_script = ""
    sql_script += "BEGIN TRANSACTION;\n"
    sql_script += statement
    sql_script += "END TRANSACTION;"
    # print sql_script

    if tgis.dbmi.__name__ == "sqlite3":
            dbif.cursor.executescript(statement)
    else:
            dbif.cursor.execute(statement)

    # Update space time datasets
    if name:
        sp.update_from_registered_maps(dbif)
    elif len(update_dict) > 0:
        for key in update_dict.keys():
            id = update_dict[key]
            if type == "rast":
                sp = tgis.dataset_factory("strds", id)
            elif type == "rast3d":
                sp = tgis.dataset_factory("str3ds", id)
            elif type == "vect":
                sp = tgis.dataset_factory("stvds", id)
            else:
                break

            sp.metadata.select(dbif)
            sp.update_from_registered_maps(dbif)

    if connect == True:
        dbif.close()
	
    grass.percent(num_maps, num_maps, 1)

###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
