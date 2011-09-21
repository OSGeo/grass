#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr3.register
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Register raster3d maps in a space time raster3d dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Register raster3d maps in a space time raster3d dataset
#% keywords: spacetime raster dataset
#% keywords: raster
#%end

#%option
#% key: dataset
#% type: string
#% description: Name of an existing space time raster3d dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing raster3d map(s), multiple maps must be provided in temporal order in case datetime should be attached
#% required: yes
#% multiple: yes
#%end

#%option
#% key: start
#% type: string
#% description: The valid start date and time of the first raster3d map, in case the map has no valid time (format absolute: "yyyy-mm-dd HH:MM:SS", format relative 5.0)
#% required: no
#% multiple: no
#%end

#%option
#% key: increment
#% type: string
#% description: Time increment between maps for valid time interval creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
#% required: no
#% multiple: no
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    name = options["dataset"]
    maps = options["maps"]
    start = options["start"]
    increment = options["increment"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Register maps
    tgis.register_maps_in_space_time_dataset("raster3d", name, maps, start, increment)
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()

