#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast.extract
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Extract a subset of a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Extracts a subset of a space time raster datasets.
#% keywords: temporal
#% keywords: extract
#%end

#%option G_OPT_STRDS_INPUT
#%end

#%option G_OPT_T_WHERE
#%end

#%option
#% key: expression
#% type: string
#% description: r.mapcalc expression assigned to all extracted raster maps
#% required: no
#% multiple: no
#%end

#%option G_OPT_STRDS_OUTPUT
#%end

#%option
#% key: base
#% type: string
#% description: The base name of the new created raster maps. This name will be extended with a numerical prefix
#% required: no
#% multiple: no
#% gisprompt:
#%end

#%option
#% key: nprocs
#% type: integer
#% description: Number of r.mapcalc processes to run in parallel
#% required: no
#% multiple: no
#% answer: 1
#%end

#%flag
#% key: n
#% description: Register Null maps
#%end

import grass.script as grass
import grass.temporal as tgis
from multiprocessing import Process

############################################################################


def main():

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    expression = options["expression"]
    base = options["base"]
    nprocs = int(options["nprocs"])
    register_null = flags["n"]

    # Make sure the temporal database exists
    tgis.create_temporal_database()

    tgis.extract_dataset(input, output, "raster", where, expression,
                         base, nprocs, register_null)

###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
