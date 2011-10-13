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
#% keywords: spacetime raster3d dataset
#% keywords: raster3d
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time raster3d dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing raster3d map(s)
#% required: no
#% multiple: yes
#%end

#%option
#% key: file
#% type: string
#% description: Input file with raster3d map names, one per line. Additionally the start time and the end time can be specified per line
#% required: no
#% multiple: no
#%end

#%option
#% key: start
#% type: string
#% description: The valid start date and time of the first raster3d map, in case the map has no valid time (format absolute: "yyyy-mm-dd HH:MM:SS", format relative 5.0). Use "file" as identifier in case the start time is located in an input file
#% required: no
#% multiple: no
#%end

#%option
#% key: end
#% type: string
#% description: The valid end date and time of the first raster3d map. Absolute time format is "yyyy-mm-dd HH:MM:SS" and "yyyy-mm-dd", relative time format id double. Use "file" as identifier in case the end time is located in the input file 
#% required: no
#% multiple: no
#%end

#%option
#% key: increment
#% type: string
#% description: Time increment between maps for valid time interval creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0), or "file" in case the increment is located in an input file
#% required: no
#% multiple: no
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
    name = options["input"]
    maps = options["maps"]
    file = options["file"]
    fs = options["fs"]
    start = options["start"]
    end = options["end"]
    increment = options["increment"]
    interval = flags["i"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Register maps
    tgis.register_maps_in_space_time_dataset(type="rast3d", name=name, maps=maps, file=file, start=start, end=end, \
                                             increment=increment, dbif=None, interval=interval, fs=fs)
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()

