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
#% description: The start date and time of the first raster map, in case the map has no date (format absolute: "yyyy-mm-dd HH:MM:SS", relative 5.0)
#% required: no
#% multiple: no
#%end

#%option
#% key: increment
#% type: string
#% description: Time increment between maps for time stamp creation (NNN seconds, minutes, hours, days, weeks)
#% required: no
#% multiple: no
#%end

import datetime
from datetime import timedelta
import time
import grass.script as grass
#######################
#####################################################

def main():

    # Get the options
    name = options["dataset"]
    maps = options["maps"]
    start = options["start"]
    increment = options["increment"]

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

    if increment:
        if sp.is_time_absolute():
            inc_value = float(increment.split(" ")[0])
            inc_unit = increment.split(" ")[1]
        else:
            inc_value = float(increment)

    count = 0
    for mapname in maplist:
        mapid = mapname + "@" + mapset
        map = grass.raster_dataset(mapid)
        map.load()

        # In case the map is already registered print a message and continue to the next map

        
        # Put the map into the database
        if map.is_in_db() == False:

            # Set the time interval
            if start:
                grass.info("Set time interval for map " + mapname)
                if sp.is_time_absolute():
                    
                    if start.find(":") > 0:
                        time_format = "%Y-%m-%d %H:%M:%S"
                    else:
                        time_format = "%Y-%m-%d"

                    start_time = datetime.datetime.fromtimestamp(time.mktime(time.strptime(start, time_format)))
                    end_time = None

                    if increment:
                        if inc_unit.find("seconds") >= 0:
                            tdelta = timedelta(seconds=inc_value)
                        elif inc_unit.find("minutes") >= 0:
                            tdelta = timedelta(minutes=inc_value)
                        elif inc_unit.find("hours") >= 0:
                            tdelta = timedelta(hours=inc_value)
                        elif inc_unit.find("days") >= 0:
                            tdelta = timedelta(days=inc_value)
                        elif inc_unit.find("weeks") >= 0:
                            tdelta = timedelta(weeks=inc_value)
                        else:
                            grass.fatal("Wrong increment format: " + increment)

                        start_time += count * tdelta
                        end_time = start_time + tdelta

                    map.set_absolute_time(start_time, end_time)
                else:
                    interval = float(start) + count * inc_value
                    map.set_relative_time(interval)
            # Put map with time interval in the database
            map.insert()
            
        # Register map
        sp.register_map(map)
        count += 1

if __name__ == "__main__":
    options, flags = grass.core.parser()
    main()

