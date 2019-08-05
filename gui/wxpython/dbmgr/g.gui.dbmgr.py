#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.dbmgr
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   Attribute Table Manager
# COPYRIGHT: (C) 2012-2013 by Martin Landa, and the GRASS Development Team
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
#% description: Launches graphical attribute table manager.
#% keyword: general
#% keyword: GUI
#% keyword: attribute table
#% keyword: database
#%end
#%option G_OPT_V_MAP
#%end

import grass.script as gscript


def main():
    options, flags = gscript.parser()

    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from dbmgr.manager import AttributeManager

    mapName = gscript.find_file(options['map'], element='vector')['fullname']
    if not mapName:
        gscript.set_raise_on_error(False)
        gscript.fatal(_("Vector map <%s> not found") % options['map'])

    app = wx.App()
    gscript.message(
        _("Loading attribute data for vector map <%s>...") %
        mapName)
    f = AttributeManager(
        parent=None, id=wx.ID_ANY, title="%s - <%s>" %
        (_("GRASS GIS Attribute Table Manager"), mapName), size=(
            900, 600), vectorName=mapName)
    f.Show()

    app.MainLoop()

if __name__ == "__main__":
    main()
