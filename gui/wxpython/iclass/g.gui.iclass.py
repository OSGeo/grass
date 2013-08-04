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
#% keywords: general
#% keywords: GUI
#% keywords: imagery
#% keywords: classification
#% keywords: Maximum Likelihood Classification
#% keywords: signatures
#%end
#%flag
#% key: m
#% description: Maximize window
#%end
#%option G_OPT_I_GROUP
#% required: no
#%end
#%option G_OPT_R_MAP
#% description: Name of raster map to load
#% required: no
#%end
#%option G_OPT_V_MAP
#% key: trainingmap
#% label: Ground truth training map to load
#% description:
#% required: no
#%end

import os
import sys

import  wx

import grass.script as grass

if __name__ == '__main__':
    wxbase = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if wxbase not in sys.path:
        sys.path.append(wxbase)

from core.settings  import UserSettings
from core.globalvar import CheckWxVersion
from core.giface    import StandaloneGrassInterface
from core.utils     import _, GuiModuleMain
from iclass.frame   import IClassMapFrame

def main():
    group_name = map_name = trainingmap_name = None
    if options['group']:
        group_name = grass.find_file(name = options['group'], element = 'group')['name']
        if not group_name:
            grass.fatal(_("Group <%s> not found") % options['group'])
    if options['map']:
        map_name = grass.find_file(name = options['map'], element = 'cell')['fullname']
        if not map_name:
            grass.fatal(_("Raster map <%s> not found") % options['map'])
    if options['trainingmap']:
        trainingmap_name = grass.find_file(name = options['trainingmap'], element = 'vector')['fullname']
        if not trainingmap_name:
            grass.fatal(_("Vector map <%s> not found") % options['trainingmap'])
    
    # define display driver
    driver = UserSettings.Get(group = 'display', key = 'driver', subkey = 'type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'
    
    # launch application
    app = wx.PySimpleApp()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()
    
    # show main frame
    giface = StandaloneGrassInterface()
    frame = IClassMapFrame(parent = None, giface = giface)
    if not flags['m']:
        frame.CenterOnScreen()
    if group_name:
        frame.SetGroup(group_name) 
    if map_name:
        giface.WriteLog(_("Loading raster map <%s>...") % map_name)
        frame.trainingMapManager.AddLayer(map_name)
    if trainingmap_name:
        giface.WriteLog(_("Loading training map <%s>...") % trainingmap_name)
        frame.ImportAreas(trainingmap_name)

    frame.Show()    
    if flags['m']:
        frame.Maximize()
    
    app.MainLoop()
    
if __name__ == '__main__':
    grass.set_raise_on_error(False)
    options, flags = grass.parser()
    
    GuiModuleMain(main)
