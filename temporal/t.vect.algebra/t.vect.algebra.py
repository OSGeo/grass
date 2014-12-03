#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.vect.algebra
# AUTHOR(S):    Thomas Leppelt, Soeren Gebbert
#
# PURPOSE:      Provide temporal vector algebra to perform spatial an temporal operations
#               for space time datasets by topological relationships to other space time
#               datasets.
# COPYRIGHT:    (C) 2014 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Apply temporal and spatial oeprations on space time vector datasets using temporal vector algebra.
#% keywords: temporal
#% keywords: algebra
#% keywords: vector
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
#% description: Activate spatial topology
#%end

import grass.script
import grass.temporal as tgis
import sys

def main():
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

