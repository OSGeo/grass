#!/usr/bin/env python
############################################################################
#
# MODULE:    Animation
# AUTHOR(S): Anna Kratochvilova
# PURPOSE:   Tool for animating a series of GRASS raster maps
#            or a space time raster dataset
# COPYRIGHT: (C) 2012 by Anna Kratochvilova, and the GRASS Development Team
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
############################################################################

#%module
#% description: Tool for animating a series of raster maps or a space time raster dataset.
#% keywords: general
#% keywords: gui
#% keywords: display
#%end
#%option G_OPT_R_INPUTS
#% key: rast
#% description: Raster maps to animate
#% required: no
#% guisection: Input
#%end
#%option G_OPT_STRDS_INPUT
#% key: strds
#% description: Space time raster dataset to animate
#% required: no
#% guisection: Input
#%end


import os
import sys

import wx
import gettext

import grass.script as grass

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.settings import UserSettings
from core.giface import StandaloneGrassInterface
from animation.frame import AnimationFrame, MAX_COUNT

def main():
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

    options, flags = grass.parser()

    rast = options['rast']
    strds = options['strds']

    if rast and strds:
        grass.fatal(_("Options 'rast' and 'strds' are mutually exclusive."))

    if rast:
        rast = [rast.split(',')] + [None] * (MAX_COUNT - 1)
    else:
        rast = None

    if strds:
        strds = [strds] + [None] * (MAX_COUNT - 1)
    else:
        strds = None

    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()

    frame = AnimationFrame(parent = None)
    frame.CentreOnScreen()
    frame.Show()
    frame.SetAnimations(raster = rast, strds = strds)
    app.MainLoop()


if __name__ == '__main__':
    main()

