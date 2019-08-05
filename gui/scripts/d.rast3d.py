#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       d.rast3d
#
# AUTHOR(S):    Martin Landa <landa.martin gmail.com>
#
# PURPOSE:	Wrapper for wxGUI to add 3D raster map into layer tree
#
# COPYRIGHT:	(C) 2008, 2010 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Displays a 3D raster map layer.
#% keyword: display
#% keyword: raster3d
#%end

#%option
#% key: map
#% type: string
#% gisprompt: old,grid3,3d-raster
#% description: 3D raster map to be displayed
#% required : yes
#%end

from grass.script import core as grass

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
