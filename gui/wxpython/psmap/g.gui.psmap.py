#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.psmap
# AUTHOR(S): Anna Kratochvilova <kratochanna gmail.com>
# PURPOSE:   Cartographic Composer
# SPDX-FileCopyrightText: 2011-2012 Anna Kratochvilova
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
############################################################################

# %module
# % description: Tool for creating hardcopy map outputs.
# % keyword: general
# % keyword: GUI
# % keyword: printing
# %end
# %option G_OPT_F_INPUT
# % key: file
# % label: File containing mapping instructions to load
# % description: See ps.map manual for details
# % key_desc: name
# % required: no
# %end

import grass.script as gs


def main():
    options, flags = gs.parser()

    import wx

    from grass.script.setup import set_gui_path

    set_gui_path()

    from psmap.frame import PsMapFrame

    app = wx.App()
    frame = PsMapFrame(
        parent=None,
        title=_("Cartographic Composer - GRASS"),
    )
    frame.Show()

    if options["file"]:
        frame.LoadFile(options["file"])

    app.MainLoop()


if __name__ == "__main__":
    main()

