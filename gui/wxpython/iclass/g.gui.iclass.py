#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.iclass
# AUTHOR(S): Anna Kratochvilova, Vaclav Petras
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
#% label: Tool for supervised classification of imagery data.
#% description: Generates spectral signatures for an image by allowing the user to outline regions of interest.
#%end

import os
import sys

import  wx
import gettext

import grass.script as grass

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.settings  import UserSettings
from core.giface    import StandaloneGrassInterface
from iclass.frame   import IClassMapFrame

def main():
    # define display driver
    driver = UserSettings.Get(group = 'display', key = 'driver', subkey = 'type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'
    
    # launch application
    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()
    
    # show main frame
    frame = IClassMapFrame(parent = None, giface = StandaloneGrassInterface())
    frame.Show()
    
    app.MainLoop()
    
if __name__ == '__main__':
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    options, flags = grass.parser()
    main()
