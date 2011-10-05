#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.series
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Performe different aggregation algorithms from r.series on all raster maps of a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Performe different aggregation algorithms from r.series on all raster maps of a space time raster dataset
#% keywords: spacetime raster dataset
#% keywords: raster
#% keywords: extract
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time raster dataset
#% required: yes
#% multiple: yes
#%end

#%option
#% key: method
#% type: string
#% description: Aggregate operation on all raster maps of a space time raster dataset
#% required: yes
#% multiple: no
#% options: average,count,median,mode,minimum,min_raster,maximum,max_raster,stddev,range,sum,variance,diversity,slope,offset,detcoeff,quart1,quart3,perc90,quantile,skewness,kurtosis
#% answer: average
#%end

#%option
#% key: sort
#% type: string
#% description: Sort the maps by category.
#% required: no
#% multiple: yes
#% options: id, name, creator, mapset, creation_time, modification_time, start_time, end_time, north, south, west, east, min, max
#% answer: id
#%end

#%option G_OPT_DB_WHERE
#%end

#%option G_OPT_R_OUTPUT
#%end

#%flag
#% key: t
#% description: Assign the space time raster dataset time interval to the output map
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    method = options["method"]
    sort = options["sort"]
    where = options["where"]
    add_time = flags["t"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    if input.find("@") >= 0:
        id = input
    else:
        mapset =  grass.gisenv()["MAPSET"]
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)

    if sp.is_in_db() == False:
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select()
    
    if add_time:
	if sp.is_time_absolute():
	    start_time, end_time, tz = sp.get_absolute_time()
	else:
	    start_time, end_time = sp.get_relative_time()

    rows = sp.get_registered_maps(None, where, sort)

    if rows:
        filename = grass.tempfile(True)
        file = open(filename, 'w')

        for row in rows:
            string = "%s\n" % (row["id"])
            file.write(string)
        
        file.close()

        if grass.overwrite() == True:
            grass.run_command("r.series", file=filename, output=output, overwrite=True, method=method)
        else:
            grass.run_command("r.series", file=filename, output=output, overwrite=False, method=method)

    if add_time:
	# Create the time range for the output map
	if output.find("@") >= 0:
	    id = output
	else:
	    mapset =  grass.gisenv()["MAPSET"]
	    id = output + "@" + mapset

	map = sp.get_new_map_instance(id)

	map.load()
	if sp.is_time_absolute():
	    map.set_absolute_time(start_time, end_time, tz)
	else:
	    map.set_relative_time(start_time, end_time)

	# Register the map in the temporal database
	if map.is_in_db():
	    map.update()
	else:
	    map.insert()    

if __name__ == "__main__":
    options, flags = grass.parser()
    main()

