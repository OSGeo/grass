#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.info
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Print information about a space-time dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: List informtion about space time and map datasets
#% keywords: spacetime dataset
#% keywords: remove
#%end

#%option
#% key: dataset
#% type: string
#% description: Name of an existing space time or map dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: type
#% type: string
#% description: Type of the dataset, default is strds (space time raster dataset)
#% required: no
#% options: strds, str3ds, stvds, raster, raster3d, vector
#% answer: strds
#%end

#%flag
#% key: g
#% description: Print information in shell style
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    name = options["dataset"]
    type = options["type"]
    shellstyle = flags['g']

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
    if type == "raster":
        sp = tgis.raster_dataset(id)
    if type == "raster3d":
        sp = tgis.raster3d_dataset(id)
    if type == "vector":
        sp = tgis.vector_dataset(id)

    if sp.is_in_db() == False:
        grass.fatal("Dataset <" + name + "> not found in temporal database")
        
    # Insert content from db
    sp.select()

    if shellstyle == True:
        sp.print_shell_info()
    else:
        sp.print_info()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

