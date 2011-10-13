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
#% description: Type of the space time dataset, default is strds (space time raster dataset)
#% required: no
#% options: strds, str3ds, stvds
#% answer: strds
#%end

#%flag
#% key: t
#% description: Print temporal relation matrix for space time datasets
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    name = options["input"]
    type = options["type"]
    tmatrix = flags['t']

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
    if type == "rast":
        sp = tgis.raster_dataset(id)
        tmatrix = False
    if type == "rast3d":
        sp = tgis.raster3d_dataset(id)
        tmatrix = False
    if type == "vect":
        sp = tgis.vector_dataset(id)
        tmatrix = False

    if sp.is_in_db() == False:
        grass.fatal("Dataset <" + name + "> not found in temporal database")
        
    # Insert content from db
    sp.select()

    if tmatrix:
        matrix = sp.get_temporal_relation_matrix()

        for row in matrix:
            for col in row:
                print col,
            print " "
        print " "

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

