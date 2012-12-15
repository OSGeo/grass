#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.list
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	List space time datasets and maps registered in the temporal database
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: List space time datasets and maps registered in the temporal database.
#% keywords: temporal
#% keywords: map management
#% keywords: list
#%end

#%option
#% key: type
#% type: string
#% description: Type of the space time dataset or map, default is strds
#% guisection: Selection
#% required: no
#% options: strds, str3ds, stvds, rast, rast3d, vect
#% answer: strds
#%end

#%option G_OPT_T_TYPE
#% multiple: yes
#% answer: absolute,relative
#% guisection: Selection
#%end

#%option
#% key: order
#% type: string
#% description: Columns number_of_maps and granularity only available for space time datasets
#% label: Sort the space time dataset by category
#% guisection: Formatting
#% required: no
#% multiple: yes
#% options: id, name, creator, mapset, number_of_maps, creation_time, start_time, end_time, interval, north, south, west, east, granularity
#% answer: id
#%end

#%option
#% key: columns
#% type: string
#% description: Columns number_of_maps and granularity only available for space time datasets
#% label: Columns to be printed to stdout
#% guisection: Selection
#% required: no
#% multiple: yes
#% options: id, name, creator, mapset, number_of_maps, creation_time, start_time, end_time, north, south, west, east, granularity, all
#% answer: id
#%end

#%option G_OPT_T_WHERE
#% guisection: Selection
#%end

#%option
#% key: fs
#% type: string
#% description: The field separator character between the columns, default is tabular "\t"
#% guisection: Formatting
#% required: no
#%end

#%flag
#% key: c
#% description: Print the column names as first row
#% guisection: Formatting
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    type = options["type"]
    temporaltype = options["temporaltype"]
    columns = options["columns"]
    order = options["order"]
    where = options["where"]
    separator = options["fs"]
    colhead = flags['c']

    # Make sure the temporal database exists
    tgis.init()

    id = None
    sp = tgis.dataset_factory(type, id)

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()
    
    first = True
    
    for ttype in temporaltype.split(","):
    
        # Create the sql selection statement
        # Table name
        if ttype == "absolute":
            table = sp.get_type() + "_view_abs_time"
        else:
            table = sp.get_type() + "_view_rel_time"
    
        if columns.find("all") == -1:
            sql = "SELECT " + str(columns) + " FROM " + table
        else:
            sql = "SELECT * FROM " + table
    
        if where:
            sql += " WHERE " + where
    
        if order:
            sql += " ORDER BY " + order
    
        dbif.cursor.execute(sql)
        rows = dbif.cursor.fetchall()
    
        # Print the query result to stout
        if rows:
            if separator is None or separator == "":
                separator = "\t"
    
            # Print the column names if requested
            if colhead == True and first == True:
                output = ""
                count = 0
                for key in rows[0].keys():
                    if count > 0:
                        output += separator + str(key)
                    else:
                        output += str(key)
                    count += 1
                print output
                first = False
    
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

    dbif.close()
        
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
