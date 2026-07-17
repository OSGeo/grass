#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.gmodeler
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   Graphical Modeler to create, edit, and manage models
# SPDX-FileCopyrightText: 2010-2023 Martin Landa
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#
############################################################################

# %module
# % label: Graphical Modeler.
# % description: Allows interactively creating, editing and managing models.
# % keyword: general
# % keyword: GUI
# % keyword: graphical modeler
# % keyword: workflow
# %end
# %option G_OPT_F_INPUT
# % key: file
# % description: Name of model file to be loaded
# % key_desc: name.gxm
# % required: no
# % guisection: Model
# %end

import grass.script as gs


def main():
    options, flags = gs.parser()

    import wx

    from grass.script.setup import set_gui_path

    set_gui_path()

    from core.giface import StandaloneGrassInterface
    from gmodeler.frame import ModelerFrame

    app = wx.App()
    frame = ModelerFrame(
        parent=None,
        giface=StandaloneGrassInterface(),
        title=_("Graphical Modeler - GRASS"),
    )
    if options["file"]:
        frame.panel.LoadModelFile(options["file"])
    frame.Show()

    app.MainLoop()


if __name__ == "__main__":
    main()
