#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast.mapcalc
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Perform spatio-temporal mapcalc expressions on temporal sampled maps of space time raster datasets
# COPYRIGHT:	(C) 2012 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Perform spatio-temporal mapcalc expressions on temporal sampled maps of space time raster datasets.
#% keywords: temporal
#% keywords: algebra
#%end

#%option G_OPT_STRDS_INPUTS
#%end

#%option
#% key: expression
#% type: string
#% description: Spatio-temporal mapcalc expression
#% required: yes
#% multiple: no
#%end

#%option G_OPT_T_SAMPLE
#% key: method
#% answer: equal
#%end

#%option G_OPT_STRDS_OUTPUT
#%end

#%option
#% key: base
#% type: string
#% description: Base name of the new created raster maps. This name will be extended with a numerical prefix
#% gisprompt: NULL
#% required: yes
#% multiple: no
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

#%flag
#% key: s
#% description: Check spatial overlap
#%end

from multiprocessing import Process
import copy
import grass.script as grass
import grass.temporal as tgis

############################################################################


def main():

    # Get the options
    inputs = options["inputs"]
    output = options["output"]
    expression = options["expression"]
    base = options["base"]
    method = options["method"]
    nprocs = int(options["nprocs"])
    register_null = flags["n"]
    spatial = flags["s"]

    # Create the method list
    method = method.split(",")

    # Make sure the temporal database exists
    tgis.init()

    tgis.dataset_mapcalculator(inputs, output, "raster", expression,
                               base, method, nprocs, register_null, spatial)

###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
