#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tv.register
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Register vector maps in a space time vector dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Register vector maps in a space time vector dataset
#% keywords: spacetime vector dataset
#% keywords: vector
#%end

#%option
#% key: dataset
#% type: string
#% description: Name of an existing space time vector dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing vector map(s), multiple maps must be provided in temporal order in case datetime should be attached
#% required: yes
#% multiple: yes
#%end

#%option
#% key: start
#% type: string
#% description: The valid start date and time of the first vector map, in case the map has no valid time (format absolute: "yyyy-mm-dd HH:MM:SS", format relative 5.0)
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

#%flag
#% key: i
#% description: Create an interval (start and end time) in case an increment is provided
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
    interval = flags["i"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()
    # Register maps
    tgis.register_maps_in_space_time_dataset("vector", name, maps, start, increment, None, interval)
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()

