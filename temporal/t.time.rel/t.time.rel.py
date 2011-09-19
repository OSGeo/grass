#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.time.rel
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Set the relative valid time interval for raster maps
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Set the relative valid time interval for maps of type raster, vector and raster3d
#% keywords: time
#% keywords: relative
#% keywords: raster
#% keywords: vector
#% keywords: raster3d
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing raster map(s)
#% required: yes
#% multiple: yes
#%end

#%option
#% key: start
#% type: double
#% description: The valid time value in [days]
#% required: no
#% multiple: no
#%end

#%option
#% key: increment
#% type: double
#% description: Time increment between maps for valid time interval creation [days]
#% required: no
#% multiple: no
#%end

import grass.script as grass

############################################################################

def main():

    # Get the options
    maps = options["maps"]
    start = options["start"]
    increment = options["increment"]

    # Make sure the temporal database exists
    grass.create_temporal_database()
    # Set valid relative time to maps
    grass.assign_valid_time_to_maps(type="raster", maps=maps, ttype="relative", \
                                    start=start, end=None, increment=increment, dbif=None)

if __name__ == "__main__":
    options, flags = grass.core.parser()
    main()

