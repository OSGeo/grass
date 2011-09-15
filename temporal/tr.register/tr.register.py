#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.register
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Register a raster map in a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Register raster maps in a space time raster dataset
#% keywords: spacetime raster dataset
#% keywords: raster
#%end

#%option
#% key: dataset
#% type: string
#% description: Name of an existing space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing raster map(s), multiple maps must be provided in temporal order in case datetime should be attached
#% required: yes
#% multiple: yes
#%end

#%option
#% key: start
#% type: string
#% description: The start date and time of the first raster map, in case the map has no date (format absolute: "yyyy-mm-dd HH:MM:SS", format relative 5.0)
#% required: no
#% multiple: no
#%end

#%option
#% key: increment
#% type: string
#% description: Time increment between maps for time stamp creation (format absolute: NNN seconds, minutes, hours, days, weeks, months, years; format relative: 1.0)
#% required: no
#% multiple: no
#%end

import datetime
import grass.script as grass
from copy import *
#######################
#####################################################

def main():

    # Get the options
    name = options["dataset"]
    maps = options["maps"]
    start = options["start"]
    increment = options["increment"]

    # Make sure the temporal database exists
    grass.create_temporal_database()

    mapset =  grass.gisenv()["MAPSET"]
    id = name + "@" + mapset

    sp = grass.space_time_raster_dataset(id)
    # Insert content from db
    sp.select()
    
    if sp.is_in_db() == False:
        grass.fatal("Space time " + sp.get_new_map_instance(None).get_type() + " dataset <" + name + "> not found")

    if maps.find(",") == -1:
        maplist = (maps,)
    else:
        maplist = tuple(maps.split(","))
                    
    count = 0
    for mapname in maplist:
        mapid = mapname + "@" + mapset
        map = grass.raster_dataset(mapid)

        # In case the map is already registered print a message and continue to the next map
        
        # Put the map into the database
        if map.is_in_db() == False:
            map.load()
            map.insert()
        else:
            map.select()
            if map.get_temporal_type() != sp.get_temporal_type():
                grass.fatal("Unable to register map <" + map.get_id() + ">. The temporal types are different.")

        # Set the time interval
        if start:
            
            if sp.is_time_absolute():
                # Create the start time object
                if start.find(":") > 0:
                    time_format = "%Y-%m-%d %H:%M:%S"
                else:
                    time_format = "%Y-%m-%d"
                
                start_time = datetime.datetime.strptime(start, time_format)
                end_time = None

                # Add the increment
                if increment:
                    start_time = grass.increment_datetime_by_string(start_time, increment, count)
                    end_time = grass.increment_datetime_by_string(start_time, increment, 1)

                grass.verbose("Set absolute time interval for map <" + mapid + "> to " + str(start_time) + " - " + str(end_time))
                map.update_absolute_time(start_time, end_time)
            else:
                if increment:
                    interval = float(start) + count * float(increment)
                else:
                    interval = float(start)
                grass.verbose("Set relative time interval for map <" + mapid + "> to " + str(interval))
                map.update_relative_time(interval)
            
        # Register map
        sp.register_map(map)
        count += 1

if __name__ == "__main__":
    options, flags = grass.core.parser()
    main()

