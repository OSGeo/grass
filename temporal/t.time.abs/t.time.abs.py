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
#% key: input
#% type: string
#% description: Name(s) of existing raster, raster3d or vector map(s)
#% required: no
#% multiple: yes
#%end

#%option
#% key: start
#% type: string
#% description: The valid start date and time of the first raster map. Time format is "yyyy-mm-dd HH:MM:SS" or only "yyyy-mm-dd", or file in case the start time is located in the input file
#% required: no
#% multiple: no
#%end

#%option
#% key: end
#% type: string
#% description: The valid end date and time of the first raster map. Time format is "yyyy-mm-dd HH:MM:SS" or only "yyyy-mm-dd", or file in case the start time is located in the input file 
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

#%option
#% key: file
#% type: string
#% description: Input file with map names, one per line
#% required: no
#% multiple: no
#%end

#%option
#% key: type
#% type: string
#% description: Input map type
#% required: no
#% options: rast, rast3d, vect
#% answer: rast
#%end

#%option
#% key: fs
#% type: string
#% description: The field separator character of the input file
#% required: no
#% answer: |
#%end

#%flag
#% key: i
#% description: Create an interval (start and end time) in case an increment is provided
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    maps = options["input"]
    file = options["file"]
    start = options["start"]
    end = options["end"]
    increment = options["increment"]
    fs = options["fs"]
    type = options["type"]
    interval = flags["i"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Set valid absolute time to maps
    tgis.assign_valid_time_to_maps(type=type, maps=maps, ttype="absolute", \
                                   start=start, end=end, file=file, increment=increment, \
                                   dbif=None, interval=interval, fs=fs)
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()

