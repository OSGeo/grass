#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.list
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	List registered maps of a space time raster dataset 
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: List registered maps of a space time raster dataset 
#% keywords: dataset
#% keywords: spacetime
#% keywords: raster
#% keywords: list
#%end

#%option
#% key: input
#% type: string
#% description: Name of a space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: order
#% type: string
#% description: Order the space time dataset by category. 
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,nsres,ewres,cols,rows,number_of_cells,min,max 
#% answer: start_time
#%end

#%option
#% key: columns
#% type: string
#% description: Select columns to be printed to stdout 
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,nsres,ewres,cols,rows,number_of_cells,min,max 
#% answer: name,mapset,start_time,end_time
#%end

#%option
#% key: where
#% type: string
#% description: A where statement for selected listing e.g: start_time < "2001-01-01" and end_time > "2001-01-01"
#% required: no
#% multiple: no
#%end

#%option
#% key: method
#% type: string
#% description: Which method should be used fot data listing
#% required: no
#% multiple: no
#% options: cols,comma,delta,deltagaps
#% answer: cols
#%end

#%option
#% key: fs
#% type: string
#% description: The field separator character between the columns, default is tabular "\t"
#% required: no
#%end

#%flag
#% key: h
#% description: Print column names 

import grass.script as grass
import grass.temporal as tgis
############################################################################

def main():

    # Get the options
    input = options["input"]
    columns = options["columns"]
    order = options["order"]
    where = options["where"]
    separator = options["fs"]
    method = options["method"]
    header = flags["h"]

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

    if separator == None or separator == "":
        separator = "\t"
           
    # This method expects a list of objects for gap detection
    if method == "delta" or method == "deltagaps":
        columns = "id,start_time,end_time"
        if method == "deltagaps":
            maps = sp.get_registered_maps_as_objects_with_gaps(where, None)
        else:
            maps = sp.get_registered_maps_as_objects(where, "start_time", None)

        if header:
            string = ""
            string += "%s%s" % ("id", separator)
            string += "%s%s" % ("start_time", separator)
            string += "%s%s" % ("end_time", separator)
            string += "%s%s" % ("interval_length", separator)
            string += "%s"   % ("distance_from_begin")
            print string

        if maps and len(maps) > 0:

            first_time, dummy = maps[0].get_valid_time()

            for map in maps:
                start, end = map.get_valid_time()
                if end:
                    delta = end -start
                else:
                    delta = None
                delta_first = start - first_time

                if map.is_time_absolute():
                    if end:
                        delta = tgis.time_delta_to_relative_time(delta)
                    delta_first = tgis.time_delta_to_relative_time(delta_first)

                string = ""
                string += "%s%s" % (map.get_id(), separator)
                string += "%s%s" % (start, separator)
                string += "%s%s" % (end, separator)
                string += "%s%s" % (delta, separator)
                string += "%s"   % (delta_first)
                print string

    else:
        # In comma separated mode only map ids are needed
        if method == "comma":
            columns = "id"

        rows = sp.get_registered_maps(columns, where, order, None)

        if rows:
            if method == "comma":
                string = ""
                count = 0
                for row in rows:
                    if count == 0:
                        string += row["id"]
                    else:
                        string += ",%s" % row["id"]
                    count += 1
                print string

            elif method == "cols":
                # Print the column names if requested
                if header:
                    output = ""
                    count = 0

                    collist = columns.split(",")

                    for key in collist:
                        if count > 0:
                            output += separator + str(key)
                        else:
                            output += str(key)
                        count += 1
                    print output

                for row in rows:
                    output = ""
                    count = 0
                    for col in row:
                        if count > 0:
                            output += separator + str(col)
                        else:
                            output += str(col)
                        count += 1
                        
                    print output

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
