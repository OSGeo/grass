#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.to.rast3
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Convert a space time raster dataset into a rast3d map
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Convert a space time raster dataset into a rast3d map
#% keywords: dataset
#% keywords: spacetime
#% keywords: raster
#% keywords: raster3d
#%end

#%option
#% key: input
#% type: string
#% description: Name of a space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option G_OPT_R3_OUTPUT
#%end

import os
import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    input = options["input"]
    output = options["output"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    mapset =  grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)

    if sp.is_in_db() == False:
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select()

    # Compute relative granularity and set bottom, top and resolution
    granularity = sp.get_granularity()

    if sp.is_time_absolute():
        start, end = sp.get_valid_time()
        end = tgis.increment_datetime_by_string(start, granularity)
        granularity = tgis.time_delta_to_relative_time(end - start)

    maps = sp.get_registered_maps_as_objects_by_granularity()

    num_maps = len(maps)
    bottom = 0
    top = granularity * num_maps

    ret = grass.run_command("g.region", t=top, b=bottom, tbres=granularity)
    
    if ret != 0:
        grass.fatal(_("Unable to set 3d region"))

    # Create a NULL map in case of granularity support
    null_map = "temporary_null_map_%i" % os.getpid()
    grass.mapcalc("%s = null()" % (null_map))

    if maps:
    	count = 0
	map_names = ""
        for map in maps:
	    # Use the first map
            id = map[0].get_id()
            # None ids will be replaced by NULL maps
            if id == None:
                id = null_map

	    if count == 0:
	        map_names = id
            else:
                map_names += ",%s" % id

            count += 1

        ret = grass.run_command("r.to.rast3", input=map_names, output=output, overwrite=grass.overwrite())
     
        if ret != 0:
            grass.fatal(_("Unable to create raster3d map <%s>" % output))

    grass.run_command("g.remove", rast=null_map)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
