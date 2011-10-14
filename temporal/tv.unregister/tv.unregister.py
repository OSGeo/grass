#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tv.unregister
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Unregister vector maps from space time vector datasets
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Unregister vector map(s) from a specific or from all space time vector dataset in which it is registered
#% keywords: spacetime vector dataset
#% keywords: vector
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time vector dataset. If no name is provided the vector map(s) are unregistered from all space time datasets in which they are registered.
#% required: no
#% multiple: no
#%end

#%option
#% key: file
#% type: string
#% description: Input file with vector map names, one per line
#% required: no
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing vector map(s) to unregister
#% required: yes
#% multiple: yes
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    file = options["file"]
    name = options["input"]
    maps = options["maps"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Unregister maps
    tgis.unregister_maps_from_space_time_datasets(type="stvds", name=name, maps=maps, file=file, dbif=None)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

