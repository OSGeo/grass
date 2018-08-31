#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.rlisetup
# AUTHOR(S): Luca Delucchi <lucadeluge gmail.com>
# PURPOSE:   RLi Setup to create configuration file for r.li modules
# COPYRIGHT: (C) 2012 by Luca Delucchi, and the GRASS Development Team
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
#% description: Configuration tool for r.li modules.
#% keyword: general
#% keyword: GUI
#% keyword: raster
#% keyword: landscape structure analysis
#%end

import grass.script as gscript


def main():
    gscript.parser()

    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from core.giface import StandaloneGrassInterface
    from core.globalvar import CheckWxVersion
    from rlisetup.frame import RLiSetupFrame

    app = wx.App()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()
    frame = RLiSetupFrame(parent=None, giface=StandaloneGrassInterface())
    frame.Show()
    frame.CenterOnScreen()

    app.MainLoop()

if __name__ == "__main__":
    main()
