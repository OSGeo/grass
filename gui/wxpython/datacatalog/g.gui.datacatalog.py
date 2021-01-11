#!/usr/bin/env python3
############################################################################
#
# MODULE:    Data catalog
# AUTHOR(S): Tereza Fiedlerova
# PURPOSE:   GRASS data catalog for browsing, modifying and managing GRASS maps
# COPYRIGHT: (C) 2014-2015 by Tereza Fiedlerova, and the GRASS Development Team
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
#% description: Tool for browsing, modifying and managing GRASS maps.
#% keyword: general
#% keyword: GUI
#% keyword: map management
#%end

import grass.script as gscript


def main():
    options, flags = gscript.parser()

    # import wx only after running parser
    # to avoid issues when only interface is needed
    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from core.giface import StandaloneGrassInterface
    from datacatalog.frame import DataCatalogFrame

    app = wx.App()

    frame = DataCatalogFrame(
        parent=None,
        giface=StandaloneGrassInterface(),
        title=_("Data Catalog - GRASS GIS"),
    )
    frame.CentreOnScreen()
    frame.Show()
    app.MainLoop()

if __name__ == '__main__':
    main()
