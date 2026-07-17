#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.dbmgr
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   Attribute Table Manager
# SPDX-FileCopyrightText: 2012-2013 Martin Landa
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
############################################################################

# %module
# % description: Launches graphical attribute table manager.
# % keyword: general
# % keyword: GUI
# % keyword: attribute table
# % keyword: database
# %end
# %option G_OPT_V_MAP
# %end

import grass.script as gs


def main():
    options, flags = gs.parser()

    import wx

    from grass.script.setup import set_gui_path

    set_gui_path()

    from dbmgr.manager import AttributeManager

    mapName = gs.find_file(options["map"], element="vector")["fullname"]
    if not mapName:
        gs.set_raise_on_error(False)
        gs.fatal(_("Vector map <%s> not found") % options["map"])

    app = wx.App()
    gs.message(_("Loading attribute data for vector map <%s>...") % mapName)
    f = AttributeManager(
        parent=None,
        id=wx.ID_ANY,
        base_title=_("Attribute Table Manager - GRASS"),
        size=(900, 600),
        vectorName=mapName,
    )
    f.Show()

    app.MainLoop()


if __name__ == "__main__":
    main()
