#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.vect.extract
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:	Extract a subset of a space time vector dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Extracts a subset of a space time vector dataset.
#% keywords: temporal
#% keywords: extract
#%end

#%option G_OPT_STVDS_INPUT
#%end

#%option G_OPT_T_WHERE
#%end

#%option G_OPT_DB_WHERE
#% key: expression
#%end

#%option G_OPT_STVDS_OUTPUT
#%end

#%option G_OPT_V_FIELD
#%end

#%option G_OPT_V_TYPE
#%end

#%option
#% key: base
#% type: string
#% description: Base name of the new created vector maps
#% required: no
#% multiple: no
#%end

#%option
#% key: nprocs
#% type: integer
#% description: The number of v.extract processes to run in parallel. Use only if database backend is used which supports concurrent writing
#% required: no
#% multiple: no
#% answer: 1
#%end

#%flag
#% key: n
#% description: Register empty maps
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
    layer = options["layer"]
    type = options["type"]
    base = options["base"]
    nprocs = int(options["nprocs"])
    register_null = flags["n"]

    # Make sure the temporal database exists
    tgis.init()

    tgis.extract_dataset(input, output, "vector", where, expression,
                         base, nprocs, register_null, layer, type)

###############################################################################

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
