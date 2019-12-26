#!/usr/bin/env python3
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.vect.algebra
# AUTHOR(S):    Thomas Leppelt, Soeren Gebbert
#
# PURPOSE:      Provide temporal vector algebra to perform spatial and temporal operations
#               for space time datasets by topological relationships to other space time
#               datasets.
# COPYRIGHT:    (C) 2014-2017 by the GRASS Development Team
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
#% description: Apply temporal and spatial operations on space time vector datasets using temporal vector algebra.
#% keyword: temporal
#% keyword: algebra
#% keyword: vector
#% keyword: time
#%end

#%option
#% key: expression
#% type: string
#% description: Spatio-temporal mapcalc expression
#% key_desc: expression
#% required : yes
#%end

#%option
#% key: basename
#% type: string
#% label: Basename of the new generated output maps
#% description: A numerical suffix separated by an underscore will be attached to create a unique identifier
#% key_desc: basename
#% required : yes
#%end

#%flag
#% key: s
#% description: Check the spatial topology of temporally related maps and process only spatially related maps
#%end

import grass.script
import sys

def main():
    # lazy imports
    import grass.temporal as tgis

    expression = options['expression']
    basename = options['basename']
    spatial = flags["s"]
    stdstype = "stvds"

    # Check for PLY istallation
    try:
        import ply.lex as lex
        import ply.yacc as yacc
    except:
        grass.script.fatal(_("Please install PLY (Lex and Yacc Python implementation) to use the temporal algebra modules."))

    tgis.init(True)
    p = tgis.TemporalVectorAlgebraParser(run = True, debug=False, spatial = spatial)
    p.parse(expression, basename, grass.script.overwrite())

if __name__ == "__main__":
    options, flags = grass.script.parser()
    sys.exit(main())

