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
# SPDX-FileCopyrightText: 2008, 2010 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#############################################################################

# %module
# % description: Displays a 3D raster map layer.
# % keyword: display
# % keyword: raster3d
# %end

# %option
# % key: map
# % type: string
# % gisprompt: old,grid3,3d-raster
# % description: 3D raster map to be displayed
# % required : yes
# %end

import grass.script as gs

if __name__ == "__main__":
    gs.parser()

