#!/usr/bin/env python3
############################################################################
#
# MODULE:    Data catalog
# AUTHOR(S): Tereza Fiedlerova
# PURPOSE:   GRASS data catalog for browsing, modifying and managing GRASS maps
# SPDX-FileCopyrightText: 2014-2015 Tereza Fiedlerova
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
############################################################################

# %module
# % description: Tool for browsing, modifying and managing GRASS maps.
# % keyword: general
# % keyword: GUI
# % keyword: map management
# %end

import grass.script as gs


def main():
    options, flags = gs.parser()

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
        title=_("Data Catalog - GRASS"),
    )
    frame.CentreOnScreen()
    frame.Show()
    app.MainLoop()


if __name__ == "__main__":
    main()

