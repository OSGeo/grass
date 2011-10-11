#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.list
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	List registered maps of a spae time raster dataset 
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: List registered maps of a spae time raster dataset 
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
#% key: sort
#% type: string
#% description: Sort the space time dataset by category. Columns number_of_maps and granularity only available fpr space time datasets
#% required: no
#% multiple: yes
#% options: id, name, creator, mapset, temporal_type, creation_time, start_time, end_time, north, south, west, east, nsres, ewres, cols, rows, number_of_cells, min, max 
#% answer: start_time
#%end

#%option
#% key: columns
#% type: string
#% description: Which columns should be printed to stdout. Columns number_of_maps and granularity only available fpr space time datasets
#% required: no
#% multiple: yes
#% options: id, name, creator, mapset, temporal_type, creation_time, start_time, end_time, north, south, west, east, nsres, ewres, cols, rows, number_of_cells, min, max 
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
#% key: fs
#% type: string
#% description: The field separator character between the columns, default is tabular "\t"
#% required: no
#%end

#%flag
#% key: c
#% description: Print the column names as first row
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################

def main():

    # Get the options
    input = options["input"]
    columns = options["columns"]
    sort = options["sort"]
    where = options["where"]
    separator = options["fs"]
    colhead = flags['c']

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    mapset =  grass.gisenv()["MAPSET"]

    if input.find("@") >= 0:
        id = input
    else:
        id = input + "@" + mapset

    sp = tgis.space_time_raster_dataset(id)
    
    if sp.is_in_db() == False:
        dbif.close()
        grass.fatal(_("Dataset <%s> not found in temporal database") % (id))

    sp.select()

    rows = sp.get_registered_maps(columns, where, sort, None)

    # Print the query result to stout
    if rows:
        if separator == None or separator == "":
            separator = "\t"

        # Print the column names if requested
        if colhead == True:
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
