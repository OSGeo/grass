#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast3d.univar
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Calculates univariate statistics from the non-null cells for each registered raster3d map of a space time raster3d dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Calculates univariate statistics from the non-null cells for each registered raster3d map of a space time raster3d dataset.
#% keywords: temporal
#% keywords: statistics
#% keywords: raster
#%end

#%option G_OPT_STR3DS_INPUT
#%end

#%option G_OPT_T_WHERE
#%end

#%option G_OPT_F_SEP
#% description: Field separator character between the output columns
#%end

#%flag
#% key: e
#% description: Calculate extended statistics
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
    where = options["where"]
    extended = flags["e"]
    header = flags["h"]
    fs = options["separator"]

    # Make sure the temporal database exists
    tgis.init()

    tgis.print_gridded_dataset_univar_statistics(
        "str3ds", input, where, extended, header, fs)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
