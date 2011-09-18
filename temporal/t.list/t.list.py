#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.list
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	List space time and map datasets
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: List space time and map datasets
#% keywords: dataset
#% keywords: spacetime
#% keywords: list
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset, default is strds
#% required: no
#% options: strds, str3ds, stvds, raster, raster3d, vector
#% answer: strds
#%end

#%option
#% key: sort
#% type: string
#% description: Sort the space time dataset by category. Columns number_of_maps and granularity only available fpr space time datasets
#% required: no
#% multiple: yes
#% options: id, name, creator, mapset, number_of_maps, creation_time, modification_time, start_time, end_time, interval, north, south, west, east, granularity
#% answer: id
#%end

#%option
#% key: columns
#% type: string
#% description: Which columns should be printed to stdout. Columns number_of_maps and granularity only available fpr space time datasets
#% required: no
#% multiple: yes
#% options: id, name, creator, mapset, number_of_maps, creation_time, modification_time, revision, start_time, end_time, interval, north, south, west, east, granularity, all
#% answer: id
#%end

#%option
#% key: where
#% type: string
#% description: A where statement for selected listing e.g: start_time < "2001-01-01" and end_time > "2001-01-01"
#% required: no
#% multiple: no
#%end

#%option
#% key: temporaltype
#% type: string
#% description: The temporal type of the space time dataset, default is absolute
#% required: no
#% options: absolute,relative
#% answer: absolute
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

############################################################################

def main():

    # Get the options
    type = options["type"]
    temporaltype = options["temporaltype"]
    columns = options["columns"]
    sort = options["sort"]
    where = options["where"]
    separator = options["fs"]
    colhead = flags['c']

    # Make sure the temporal database exists
    grass.create_temporal_database()

    id = None

    if type == "strds":
        sp = grass.space_time_raster_dataset(id)
    if type == "str3ds":
        sp = grass.space_time_raster3d_dataset(id)
    if type == "stvds":
        sp = grass.space_time_vector_dataset(id)
    if type == "raster":
        sp = grass.raster_dataset(id)
    if type == "raster3d":
        sp = grass.raster3d_dataset(id)
    if type == "vector":
        sp = grass.vector_dataset(id)

    dbif = grass.sql_database_interface()
    dbif.connect()

    # Insert content from db
    sp.select(dbif)

    # Create the sql selection statement
    # Table name
    if temporaltype == "absolute":
        table = sp.get_type() + "_view_abs_time"
    else:
        table = sp.get_type() + "_view_rel_time"

    if columns.find("all") == -1:
        sql = "SELECT " + str(columns) + " FROM " + table
    else:
        sql = "SELECT * FROM " + table

    if sort:
        sql += " ORDER BY " + sort

    if where:
        sql += " WHERE " + where

    dbif.cursor.execute(sql)
    rows = dbif.cursor.fetchall()
    dbif.close()

    # Print the query result to stout
    if rows:
        if separator == None or separator == "":
            separator = "\t"

        # Print the column names if requested
        if colhead == True:
            output = ""
            count = 0
            for key in rows[0].keys():
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
    options, flags = grass.core.parser()
    main()
