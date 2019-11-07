#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast.list
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	List registered maps of a space time raster dataset
# COPYRIGHT:	(C) 2011-2017, Soeren Gebbert and the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

#%module
#% description: Lists registered maps of a space time raster dataset.
#% keyword: temporal
#% keyword: map management
#% keyword: raster
#% keyword: list
#% keyword: time
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option
#% key: order
#% type: string
#% description: Sort the space time dataset by category
#% guisection: Formatting
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,nsres,ewres,cols,rows,number_of_cells,min,max
#% answer: start_time
#%end

#%option
#% key: columns
#% type: string
#% description: Columns to be printed to stdout
#% guisection: Selection
#% required: no
#% multiple: yes
#% options: id,name,creator,mapset,temporal_type,creation_time,start_time,end_time,north,south,west,east,nsres,ewres,cols,rows,number_of_cells,min,max
#% answer: name,mapset,start_time,end_time
#%end

#%option G_OPT_T_WHERE
#% guisection: Selection
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
#% key: granule
#% type: string
#% description: The granule to be used for listing. The granule must be specified as string eg.: absolute time "1 months" or relative time "1"
#% required: no
#% multiple: no
#%end

#%option G_OPT_F_SEP
#% label: Field separator character between the output columns
#% guisection: Formatting
#%end

#%option G_OPT_F_OUTPUT
#% required: no
#%end

#%flag
#% key: u
#% description: Suppress printing of column names
#% guisection: Formatting
#%end

import grass.script as grass


############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    columns = options["columns"]
    order = options["order"]
    where = options["where"]
    separator = grass.separator(options["separator"])
    method = options["method"]
    granule = options["granule"]
    header = flags["u"]
    output = options["output"]

    # Make sure the temporal database exists
    tgis.init()

    tgis.list_maps_of_stds("strds", input, columns, order, where, separator,
                           method, header, granule, outpath=output)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
