#!/usr/bin/env python
# -*- coding: utf-8 -*-
############################################################################
#
# MODULE:	tr.unregister
# AUTHOR(S):	Soeren Gebbert
#               
# PURPOSE:	Unregister raster map a space time raster dataset
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (version 2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Register raster maps in a space time raster dataset
#% keywords: spacetime raster dataset
#% keywords: raster
#%end

#%option
#% key: input
#% type: string
#% description: Name of an existing space time raster dataset
#% required: yes
#% multiple: no
#%end

#%option
#% key: maps
#% type: string
#% description: Name(s) of existing raster map(s) to unregister
#% required: yes
#% multiple: yes
#%end

import grass.script as grass
############################################################################

def main():

    # Get the options
    input = options["input"]
    maps = options["maps"]

if __name__ == "__main__":
    options, flags = grass.core.parser()
    main()

