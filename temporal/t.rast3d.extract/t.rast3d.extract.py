#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.rast3d.extract
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Extract a subset of a space time 3D raster dataset
# COPYRIGHT:	(C) 2011-2014 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Extracts a subset of a space time 3D raster dataset.
#% keyword: temporal
#% keyword: extract
#% keyword: raster3d
#% keyword: voxel
#% keyword: time
#%end

#%option G_OPT_STR3DS_INPUT
#%end

#%option G_OPT_T_WHERE
#%end

#%option
#% key: expression
#% type: string
#% description: The r3.mapcalc expression assigned to all extracted 3D raster maps
#% required: no
#% multiple: no
#%end

#%option G_OPT_STR3DS_OUTPUT
#%end

#%option
#% key: basename
#% type: string
#% description: Basename of the new generated 3D raster maps
#% required: no
#% multiple: no
#%end

#%option
#% key: suffix
#% type: string
#% description: Suffix to add at basename: set 'gran' for granularity, 'time' for the full time format, 'num' for numerical suffix with a specific number of digits (default %05)
#% answer: gran
#% required: no
#% multiple: no
#%end

#%option
#% key: nprocs
#% type: integer
#% description: Number of r3.mapcalc processes to run in parallel
#% required: no
#% multiple: no
#% answer: 1
#%end

#%flag
#% key: n
#% description: Register Null maps
#%end

import grass.script as grass


############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    output = options["output"]
    where = options["where"]
    expression = options["expression"]
    base = options["basename"]
    nprocs = int(options["nprocs"])
    register_null = flags["n"]
    time_suffix = options["suffix"]

    # Make sure the temporal database exists
    tgis.init()

    tgis.extract_dataset(input, output, "raster3d", where, expression,
                         base, time_suffix, nprocs, register_null)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
