#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast3d.univar
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Calculates univariate statistics from the non-null cells for each registered 3D 
#               raster map of a space time 3D raster dataset
# COPYRIGHT:	(C) 2011-2014, Soeren Gebbert and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Calculates univariate statistics from the non-null cells for each registered 3D raster map of a space time 3D raster dataset.
#% keywords: temporal
#% keywords: statistics
#% keywords: raster3d
#% keywords: voxel
#%end

#%option G_OPT_STR3DS_INPUT
#%end

#%option G_OPT_T_WHERE
#% guisection: Selection
#%end

#%option G_OPT_F_SEP
#% description: Field separator character between the output columns
#% guisection: Formatting
#%end

#%flag
#% key: e
#% description: Calculate extended statistics
#%end

#%flag
#% key: s
#% description: Suppress printing of column names
#% guisection: Formatting
#%end

import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    input = options["input"]
    where = options["where"]
    extended = flags["e"]
    no_header = flags["s"]
    separator = grass.separator(options["separator"])

    # Make sure the temporal database exists
    tgis.init()

    tgis.print_gridded_dataset_univar_statistics(
        "str3ds", input, where, extended, no_header, separator)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
