#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast3d.mapcalc2
# AUTHOR(S):    Thomas Leppelt, Soeren Gebbert
#
# PURPOSE:      Provide temporal 3D raster algebra to perform spatial an temporal operations
#               for space time datasets by topological relationships to other space time
#               datasets.
# COPYRIGHT:    (C) 2014 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Apply temporal and spatial operations on space time 3D raster datasets using temporal raster algebra.
#% keywords: temporal
#% keywords: algebra
#%end

#%option
#% key: expression
#% type: string
#% description: r3.mapcalc expression for temporal and spatial analysis of space time 3D raster datasets
#% required : yes
#%end

#%option
#% key: basename
#% type: string
#% label: Basename of the new generated output maps
#% description: A numerical suffix separated by an underscore will be attached to create a unique identifier
#% required: yes
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
#% key: s
#% description: Activate spatial topology
#%end

#%flag
#% key: n
#% description: Register Null maps
#%end


import grass.script
import grass.temporal as tgis
import sys

def main():
    expression = options['expression']
    basename = options['basename']
    nprocs = options["nprocs"]
    spatial = flags["s"]
    register_null = flags["n"]

    # Check for PLY istallation
    try:
        import ply.lex as lex
        import ply.yacc as yacc
    except:
        grass.script.fatal(_("Please install PLY (Lex and Yacc Python implementation) to use the temporal algebra modules. "
                             "You can use t.rast3d.mapcalc that provides a limited but useful alternative to "
                             "t.rast3d.mapcalc2 without PLY requirement."))

    tgis.init(True)
    p = tgis.TemporalRaster3DAlgebraParser(run = True, debug=False, spatial = spatial, nprocs = nprocs, register_null = register_null)
    p.parse(expression, basename, grass.script.overwrite())

if __name__ == "__main__":
    options, flags = grass.script.parser()
    sys.exit(main())

