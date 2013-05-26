#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.vdigit
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   wxGUI Vector Digitizer
# COPYRIGHT: (C) 2007-2012 by Martin Landa, and the GRASS Development Team
#
#  This program is free software; you can 1redistribute it and/or
#  modify it under the terms of the GNU General Public License as
#  published by the Free Software Foundation; either version 2 of the
#  License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
############################################################################

#%module
#% description: Interactive editing and digitization of vector maps.
#% keywords: general
#% keywords: GUI
#% keywords: vector
#% keywords: editing
#% keywords: digitization
#%end
#%option G_OPT_V_MAP
#%label: Name of vector map to load
#%end

import os
import sys
import gettext

import grass.script as grass

import wx

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.globalvar import CheckWxVersion
from mapdisp.frame import MapFrame
from core.giface   import StandaloneGrassInterface
from core.settings import UserSettings
from vdigit.main   import haveVDigit, errorMsg

class VDigitMapFrame(MapFrame):
    def __init__(self, vectorMap):
        MapFrame.__init__(self, parent = None, giface = StandaloneGrassInterface(),
                          title = _("GRASS GIS Vector Digitizer"), size = (850, 600))

        # load vector map
        mapLayer = self.GetMap().AddLayer(ltype = 'vector',
                                          command = ['d.vect', 'map=%s' % vectorMap],
                                          active = True, name = vectorMap, hidden = False, opacity = 1.0,
                                          render = True)
        
        # switch toolbar
        self.AddToolbar('vdigit', fixed = True)
        
        # start editing
        self.toolbars['vdigit'].StartEditing(mapLayer)

def main():
    if not haveVDigit:
        grass.fatal(_("Vector digitizer not available. %s") % errorMsg)
    
    if not grass.find_file(name = options['map'], element = 'vector',
                           mapset = grass.gisenv()['MAPSET'])['fullname']:
        grass.fatal(_("Vector map <%s> not found in current mapset") % options['map'])

    # allow immediate rendering
    driver = UserSettings.Get(group = 'display', key = 'driver', subkey = 'type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'
    
    app = wx.PySimpleApp()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()
    frame = VDigitMapFrame(options['map'])
    frame.Show()

    app.MainLoop()
    
if __name__ == "__main__":
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    grass.set_raise_on_error(False)
    options, flags = grass.parser()
    
    main()
