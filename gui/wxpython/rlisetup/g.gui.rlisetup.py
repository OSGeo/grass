#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.rlisetup
# AUTHOR(S): Luca Delucchi <lucadeluge gmail.com>
# PURPOSE:   RLi Setup to create configuration file for r.li modules
# SPDX-FileCopyrightText: 2012 Luca Delucchi
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
############################################################################

# %module
# % description: Configuration tool for r.li modules.
# % keyword: general
# % keyword: GUI
# % keyword: raster
# % keyword: landscape structure analysis
# %end

import grass.script as gs


def main():
    gs.parser()

    import wx

    from grass.script.setup import set_gui_path

    set_gui_path()

    from core.giface import StandaloneGrassInterface
    from rlisetup.frame import RLiSetupFrame

    app = wx.App()
    frame = RLiSetupFrame(
        parent=None,
        giface=StandaloneGrassInterface(),
        title=_("Setup for r.li modules - GRASS"),
    )
    frame.Show()
    frame.CenterOnScreen()

    app.MainLoop()


if __name__ == "__main__":
    main()
