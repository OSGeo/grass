#!/usr/bin/env python

#%module
#% description: 
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


import os
import sys

import  wx
import gettext

import grass.script as grass

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.settings import UserSettings
from frame import SwipeMapFrame


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

    for mapName in [first, second]:    
        if mapName:
            gfile = grass.find_file(name = mapName)
            if not gfile['name']:
                grass.fatal(_("Raster map <%s> not found") % mapName)

    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()

    frame = SwipeMapFrame()
    
    if first:
        frame.SetFirstRaster(first)
    if second:
        frame.SetSecondRaster(second)

    frame.Show()

    app.MainLoop()


if __name__ == '__main__':
    main()

