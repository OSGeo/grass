#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr3.unregister
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Unregister raster3d maps from space time raster3d datasets
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Unregister raster3d map(s) from a specific or from all space time raster3d dataset in which it is registered
#% keywords: spacetime raster3d dataset
#% keywords: raster3d
#%end

#%option
#% key: dataset
#% type: string
#% description: Name of an existing space time raster3d dataset. If no name is provided the raster3d map(s) are unregistered from all space time datasets in which they are registered.
#% required: no
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing raster3d map(s) to unregister
#% required: yes
#% multiple: yes
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    name = options["dataset"]
    maps = options["maps"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Unregister maps
    tgis.unregister_maps_from_space_time_datasets("raster3d", name, maps)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

