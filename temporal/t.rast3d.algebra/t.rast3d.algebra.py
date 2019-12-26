#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.rast3d.algebra
# AUTHOR(S):    Thomas Leppelt, Soeren Gebbert
#
# PURPOSE:      Provide temporal 3D raster algebra to perform spatial an temporal operations
#               for space time datasets by topological relationships to other space time
#               datasets.
# COPYRIGHT:    (C) 2017 by the GRASS Development Team
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
#% description: Apply temporal and spatial operations on space time 3D raster datasets using temporal 3D raster algebra.
#% keyword: temporal
#% keyword: algebra
#% keyword: raster3d
#% keyword: voxel
#% keyword: time
#%end

#%option
#% key: expression
#% type: string
#% description: Algebraic expression for temporal and spatial analysis of space time 3D raster datasets
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
#% description: Check the spatial topology of temporally related maps and process only spatially related maps
#%end

#%flag
#% key: n
#% description: Register Null maps
#%end

#%flag
#% key: g
#% description: Use granularity sampling instead of the temporal topology approach
#%end


import grass.script
import sys


def main():
    # lazy imports
    import grass.temporal as tgis

    expression = options['expression']
    basename = options['basename']
    nprocs = options["nprocs"]
    spatial = flags["s"]
    register_null = flags["n"]
    granularity = flags["g"]

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

    if granularity:
        if not p.setup_common_granularity(expression=expression,  stdstype = 'str3ds',  lexer = tgis.TemporalRasterAlgebraLexer()):
            grass.script.fatal(_("Unable to process the expression in granularity algebra mode"))

    p.parse(expression, basename, grass.script.overwrite())

if __name__ == "__main__":
    options, flags = grass.script.parser()
    sys.exit(main())

