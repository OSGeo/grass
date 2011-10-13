#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.topology
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	List and modify temporal topology of a space time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: List and modify temporal topology of a space time dataset
#% keywords: spacetime dataset
#% keywords: remove
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset, default is space time raster dataset (strds)
#% required: no
#% options: strds, str3ds, stvds
#% answer: strds
#%end

#%flag
#% key: t
#% description: Print temporal relation matrix and exit
#%end

#%flag
#% key: r
#% description: Print temporal relation count
#%end

#%flag
#% key: m
#% description: Print temporal map type count
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    name = options["input"]
    type = options["type"]
    tmatrix = flags['t']
    relation = flags['r']
    map_types = flags['m']

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    #Get the current mapset to create the id of the space time dataset
    if name.find("@") >= 0:
        id = name
    else:
        mapset =  grass.gisenv()["MAPSET"]
        id = name + "@" + mapset

    if type == "strds":
        sp = tgis.space_time_raster_dataset(id)
    if type == "str3ds":
        sp = tgis.space_time_raster3d_dataset(id)
    if type == "stvds":
        sp = tgis.space_time_vector_dataset(id)

    if sp.is_in_db() == False:
        grass.fatal("Dataset <" + name + "> not found in temporal database")
        
    # Insert content from db
    sp.select()

    maps = sp.get_registered_maps_as_objects()

    if tmatrix:
        matrix = sp.get_temporal_relation_matrix(maps)

        for row in matrix:
            for col in row:
                print col,
            print " "
        print " "
        return

    if relation:
        dict = sp.get_temporal_relations_count(maps)

        for key in dict.keys():
            print key, dict[key]

    if map_types:
        dict = sp.get_temporal_map_type_count(maps)
        
        for key in dict.keys():
            print key, dict[key]

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

