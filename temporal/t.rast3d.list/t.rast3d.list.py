#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast3d.list
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	List registered maps of a space time raster3d dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Lists registered maps of a space time raster3d dataset.
#% keywords: temporal
#% keywords: map management
#% keywords: raster3d
#% keywords: list
#%end

#%option G_OPT_STR3DS_INPUT
#%end

#%option
#% key: order
#% type: string
#% description: Order the space time dataset by category
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,nsres,tbres,ewres,cols,rows,depths,number_of_cells,min,max
#% answer: start_time
#%end

#%option
#% key: columns
#% type: string
#% description: Columns to be printed to stdout
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,nsres,tbres,ewres,cols,rows,depths,number_of_cells,min,max
#% answer: name,mapset,start_time,end_time
#%end

#%option G_OPT_T_WHERE
#%end

#%option
#% key: method
#% type: string
#% description: Method used for data listing
#% required: no
#% multiple: no
#% options: cols,comma,delta,deltagaps,gran
#% answer: cols
#%end

#%option
#% key: separator
#% type: string
#% description: Separator character between the output columns, default is tabular "\t"
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
    separator = options["separator"]
    method = options["method"]
    header = flags["h"]

    # Make sure the temporal database exists
    tgis.init()

    tgis.list_maps_of_stds(
        "str3ds", input, columns, order, where, separator, method, header)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
