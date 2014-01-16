#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:       t.select
# AUTHOR(S):    Thomas Leppelt, Soeren Gebbert
#
# PURPOSE:      Select maps from space time datasets by topological relationships to
#               other space time datasets using temporal algebra.
# COPYRIGHT:    (C) 2011 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (version 2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Select maps from space time datasets by topological relationships to other space time datasets using temporal algebra.
#% keywords: temporal
#% keywords: metadata
#%end

#%option G_OPT_STDS_TYPE
#% guidependency: input
#% guisection: Required
#%end

#%option
#% key: expression
#% type: string
#% description: The temporal mapcalc expression
#% key_desc: expression
#% required : yes
#%end

#%flag
#% key: s
#% description: Activate spatial topology.
#%end


import grass.script as grass
import grass.temporal as tgis
import sys

def main():

    expression = options['expression']
    spatial = flags["s"]
    stdstype = options["type"]

    # Check for PLY istallation
    try:
        import ply.lex as lex
        import ply.yacc as yacc
    except:
        grass.fatal(_("Please install PLY (Lex and Yacc Python implementation) to use the temporal algebra modules."))

    tgis.init(True)
    p = tgis.TemporalAlgebraParser(run=True, debug=False, spatial = spatial)
    p.parse(expression, stdstype)

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())




