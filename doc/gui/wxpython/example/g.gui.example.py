#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.iclass
# AUTHOR(S): Anna Petrasova
# PURPOSE:   Example GUI application
# COPYRIGHT: (C) 2012-2014 by the GRASS Development Team
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
#% description: Example GUI application which displays raster map and further information
#% keyword: example
#% keyword: GUI
#% keyword: raster
#%end
#%option G_OPT_R_INPUT
#% description: Name of raster map to load
#% required: no
#%end

import os
import sys
import wx


# i18n is taken care of in the grass library code.
# So we need to import it before any of the GUI code.
import grass.script.core as gcore

if __name__ == '__main__':
    wxbase = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if wxbase not in sys.path:
        sys.path.append(wxbase)

from core.globalvar import CheckWxVersion
from core.giface import StandaloneGrassInterface
from core.utils import GuiModuleMain
from core.settings import UserSettings
from example.frame import ExampleMapFrame


def main():
    options, flags = gcore.parser()
    if options['input']:
        map_name = gcore.find_file(name=options['input'], element='cell')['fullname']
        if not map_name:
            gcore.fatal(_("Raster map <{raster}> not found").format(raster=options['input']))

    # define display driver (avoid 'no graphics device selected' error at start up)
    driver = UserSettings.Get(group='display', key='driver', subkey='type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    # launch application
    app = wx.App()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()

    # show main frame
    giface = StandaloneGrassInterface()
    frame = ExampleMapFrame(parent=None, giface=giface)
    if options['input']:
        giface.WriteLog(_("Loading raster map <{raster}>...").format(raster=map_name))
        frame.SetLayer(map_name)

    frame.Show()
    app.MainLoop()

if __name__ == '__main__':
    GuiModuleMain(main)
