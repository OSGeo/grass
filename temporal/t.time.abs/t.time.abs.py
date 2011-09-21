#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.time.abs
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Set the absolute valid time interval for raster maps
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Set the absolute valid time interval for maps of type raster, vector and raster3d
#% keywords: time
#% keywords: absolute
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
#% type: string
#% description: The valid start date and time of the first raster map. Time format is "yyyy-mm-dd HH:MM:SS" or only "yyyy-mm-dd"
#% required: no
#% multiple: no
#%end

#%option
#% key: end
#% type: string
#% description: The valid end date and time of the first raster map. Time format is "yyyy-mm-dd HH:MM:SS" or only "yyyy-mm-dd". End time and increment are mutual exclusive.
#% required: no
#% multiple: no
#%end

#%option
#% key: increment
#% type: string
#% description: Time increment between maps for valid time interval creation. Interval format: NNN seconds, minutes, hours, days, weeks, months, years
#% required: no
#% multiple: no
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    maps = options["maps"]
    start = options["start"]
    end = options["end"]
    increment = options["increment"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Set valid absolute time to maps
    tgis.assign_valid_time_to_maps(type="raster", maps=maps, ttype="absolute", \
                                   start=start, end=end, increment=increment, dbif=None)
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()

