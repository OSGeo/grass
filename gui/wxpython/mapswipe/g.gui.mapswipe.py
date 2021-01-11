#!/usr/bin/env python3
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
#% description: Interactively compares two maps by swiping a visibility bar.
#% keyword: general
#% keyword: GUI
#% keyword: display
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
import grass.script as gscript


def main():
    options, flags = gscript.parser()

    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from core.settings import UserSettings
    from core.giface import StandaloneGrassInterface
    from mapswipe.frame import SwipeMapFrame

    driver = UserSettings.Get(group='display', key='driver', subkey='type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    first = options['first']
    second = options['second']
    mode = options['mode']

    for mapName in [first, second]:
        if mapName:
            gfile = gscript.find_file(name=mapName)
            if not gfile['name']:
                gscript.fatal(_("Raster map <%s> not found") % mapName)

    app = wx.App()

    frame = SwipeMapFrame(
        parent=None,
        giface=StandaloneGrassInterface(),
        title=_("Temporal Plot Tool - GRASS GIS"),
    )

    if first:
        frame.SetFirstRaster(first)
    if second:
        frame.SetSecondRaster(second)
    if first or second:
        frame.SetRasterNames()

    frame.SetViewMode(mode)
    frame.Show()

    app.MainLoop()


if __name__ == '__main__':
    main()
