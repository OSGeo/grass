#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.remove
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Remove a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Remove a space time and map datasets from temporal database
#% keywords: spacetime
#% keywords: dataset
#% keywords: remove
#%end

#%option
#% key: dataset
#% type: string
#% description: Name(s) of the space time or map dataset to be removed from the temporal database
#% required: yes
#% multiple: no
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset, default is strds
#% required: no
#% options: strds, str3ds, stvds, raster, raster3d, vector
#% answer: strds
#%end

import grass.script as grass

############################################################################

def main():
    
    # Get the options
    names = options["dataset"]
    type = options["type"]

    # Make sure the temporal database exists
    grass.create_temporal_database()
    
    mapset =  grass.gisenv()["MAPSET"]
    
    for name in names.split(","):
        name = name.strip()
        # Check for the mapset in name
        if name.find("@") < 0:
            id = name + "@" + mapset
        else:
            id = name

        if type == "strds":
            sp = grass.space_time_raster_dataset(id)
        if type == "str3ds":
            sp = grass.space_time_raster3d_dataset(id)
        if type == "stvds":
            sp = grass.space_time_vector_dataset(id)
        if type == "raster":
            sp = grass.raster_dataset(id)
        if type == "raster3d":
            sp = grass.raster3d_dataset(id)
        if type == "vector":
            sp = grass.vector_dataset(id)

        if sp.is_in_db() == False:
            grass.fatal("Dataset <" + name + "> not found in temporal database")

        # We need to read some data from the temporal database
        sp.select()
        sp.delete()

if __name__ == "__main__":
    options, flags = grass.core.parser()
    main()

