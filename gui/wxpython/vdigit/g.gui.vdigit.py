#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.vdigit
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   wxGUI Vector Digitizer
# COPYRIGHT: (C) 2007-2013 by Martin Landa, and the GRASS Development Team
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
#%flag
#% key: c
#% description: Create new vector map if doesn't exist
#%end
#%option G_OPT_V_MAP
#% label: Name of vector map to edit
#%end

import os
import sys

import grass.script as grass

import wx

if __name__ == '__main__':
    gui_wx_path = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if gui_wx_path not in sys.path:
        sys.path.append(gui_wx_path)

from core.globalvar import CheckWxVersion
from core.utils import _, GuiModuleMain
from mapdisp.frame import MapFrame
from core.giface import StandaloneGrassInterface
from core.settings import UserSettings
from vdigit.main import haveVDigit, errorMsg

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
    grass.set_raise_on_error(False)
    
    options, flags = grass.parser()
    
    if not haveVDigit:
        grass.fatal(_("Vector digitizer not available. %s") % errorMsg)
    
    if not grass.find_file(name = options['map'], element = 'vector',
                           mapset = grass.gisenv()['MAPSET'])['fullname']:
        if not flags['c']:
            grass.fatal(_("Vector map <%s> not found in current mapset. "
                          "New vector map can be created by providing '-c' flag.") % options['map'])
        else:
            grass.message(_("New vector map <%s> created") % options['map'])
            if 0 != grass.run_command('v.edit', map = options['map'], tool = 'create'):
                grass.fatal(_("Unable to create new vector map <%s>") % options['map'])
    
    GuiModuleMain(main)
