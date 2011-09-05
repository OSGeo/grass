#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	t.remove
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Remove a space-time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Remove a space-time dataset
#% keywords: spacetime dataset
#% keywords: remove
#%end

#%option
#% key: name
#% type: string
#% description: Name of the new space-time dataset
#% required: yes
#% multiple: no
#%end
#%option
#% key: type
#% type: string
#% description: Type of the space time dataset, default is strds
#% required: no
#% options: strds
#% answer: strds
#%end

import sys
import os
import grass.script as grass

############################################################################

def main():

    # Get the options
    name = options["name"]
    type = options["type"]

if __name__ == "__main__":
    options, flags = grass.core.parser()
    main()

