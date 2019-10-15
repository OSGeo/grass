#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.gmodeler
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   Graphical Modeler to create, edit, and manage models
# COPYRIGHT: (C) 2010-2012 by Martin Landa, and the GRASS Development Team
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
#% label: Graphical Modeler.
#% description: Allows interactively creating, editing and managing models.
#% keyword: general
#% keyword: GUI
#% keyword: graphical modeler
#% keyword: workflow
#%end
#%option G_OPT_F_INPUT
#% key: file
#% description: Name of model file to be loaded
#% key_desc: name.gxm
#% required: no
#% guisection: Model
#%end

import grass.script as gscript


def main():
    options, flags = gscript.parser()

    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from core.giface import StandaloneGrassInterface
    from gmodeler.frame import ModelFrame

    app = wx.App()
    frame = ModelFrame(parent=None, giface=StandaloneGrassInterface())
    if options['file']:
        frame.LoadModelFile(options['file'])
    frame.Show()

    app.MainLoop()

if __name__ == "__main__":
    main()
