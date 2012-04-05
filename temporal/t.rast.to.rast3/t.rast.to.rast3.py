#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast.to.rast3
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
#% keywords: temporal
#% keywords: raster3d
#% keywords: convert
#%end

#%option G_OPT_STRDS_INPUT
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
        grass.fatal(_("Space time %s dataset <%s> not found") % (sp.get_new_map_instance(None).get_type(), id))

    sp.select()

    maps = sp.get_registered_maps_as_objects_by_granularity()

    # Get the granularity and set bottom, top and top-bottom resolution
    granularity = sp.get_granularity()

    if sp.is_time_absolute():
        unit = granularity.split(" ")[1] 
        granularity = int(granularity.split(" ")[0])
    else:
        unit = sp.get_relative_time_unit()

    num_maps = len(maps)
    bottom = 0
    top = granularity * num_maps

    ret = grass.run_command("g.region", t=top, b=bottom, tbres=granularity)
    
    if ret != 0:
        grass.fatal(_("Unable to set 3d region"))

    # Create a NULL map to fill the gaps
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

    title = _("Space time voxel cube")
    descr = _("This space time voxel cube was created with t.rast.to.rast3")

    # Set the unit
    ret = grass.run_command("r3.support", map=output, vunit=unit, title=title, description=descr, overwrite=grass.overwrite())

    # Register the space time voxel cube in the temporal GIS
    if output.find("@") >= 0:
        id = output
    else:
        id = output + "@" + mapset

    start, end = sp.get_valid_time()
    r3ds = tgis.raster3d_dataset(id)

    if r3ds.is_in_db():
        r3ds.select()
        r3ds.delete()
        r3ds = tgis.raster3d_dataset(id)

    r3ds.load()

    if sp.is_time_absolute():
        r3ds.set_absolute_time(start, end)
        r3ds.write_absolute_time_to_file()
    else:
        r3ds.set_relative_time(start, end, sp.get_relative_time_unit())
        r3ds.write_relative_time_to_file()

    r3ds.insert()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
