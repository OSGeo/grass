#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tv.list
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	List registered maps of a space time vector dataset 
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: List registered maps of a space time vector dataset 
#% keywords: dataset
#% keywords: spacetime
#% keywords: vector
#% keywords: list
#%end

#%option
#% key: input
#% type: string
#% description: Name of a space time vector dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: order
#% type: string
#% description: Order the space time dataset by category. 
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east
#% answer: start_time
#%end

#%option
#% key: columns
#% type: string
#% description: Select columns to be printed to stdout 
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east
#% answer: name,mapset,start_time,end_time
#%end

#%option
#% key: where
#% type: string
#% description: A where statement for selected listing without "WHERE" e.g: "start_time < '2001-01-01' AND end_time > '2001-01-01'"
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
#%end

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

    tgis.list_maps_of_stds("stvds", input, columns, order, where, separator, method, header)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
