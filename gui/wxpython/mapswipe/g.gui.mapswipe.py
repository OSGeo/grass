#!/usr/bin/env python
############################################################################
#
# MODULE:    Map Swipe
# AUTHOR(S): Anna Kratochvilova
# PURPOSE:   The Map Swipe is a wxGUI component which allows the user to
#            interactively compare two maps  
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
#% description: Allows to interactively compare two maps by swiping.
#% keywords: general
#% keywords: gui
#% keywords: display
#%end
#%option G_OPT_R_INPUT
#% key: first
#% description: First (top/right) raster map
#% required: no
#%end
#%option G_OPT_R_INPUT
#% key: second
#% description: Second (bottom/left) raster map
#% required: no
#%end
#%option
#% key: mode
#% description: View mode
#% options: swipe,mirror
#% descriptions:swipe;swiping the upper map layer to show the map layer below ;mirror;synchronized maps side by side
#% answer: swipe
#% required: no
#%end


import os
import sys

import  wx
import gettext

import grass.script as grass

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.settings import UserSettings
from core.giface import StandaloneGrassInterface
from mapswipe.frame import SwipeMapFrame


def main():
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

    driver = UserSettings.Get(group = 'display', key = 'driver', subkey = 'type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    options, flags = grass.parser()

    first = options['first']
    second = options['second']
    mode = options['mode']

    for mapName in [first, second]:    
        if mapName:
            gfile = grass.find_file(name = mapName)
            if not gfile['name']:
                grass.fatal(_("Raster map <%s> not found") % mapName)

    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()

    frame = SwipeMapFrame(parent = None, giface = StandaloneGrassInterface())
    
    if first:
        frame.SetFirstRaster(first)
    if second:
        frame.SetSecondRaster(second)
    if first or second:
        frame.SetRasterNames()

    frame.SetViewMode(mode)
    frame.ZoomToMap()
    frame.Show()

    app.MainLoop()


if __name__ == '__main__':
    main()

