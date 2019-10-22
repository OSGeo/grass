#!/usr/bin/env python3
############################################################################
#
# MODULE:    g.gui.psmap
# AUTHOR(S): Anna Kratochvilova <kratochanna gmail.com>
# PURPOSE:   Cartographic Composer
# COPYRIGHT: (C) 2011-2012 by Anna Kratochvilova, and the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or
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
#% description: Tool for creating hardcopy map outputs.
#% keyword: general
#% keyword: GUI
#% keyword: printing
#%end
#%option G_OPT_F_INPUT
#% key: file
#% label: File containing mapping instructions to load
#% description: See ps.map manual for details
#% key_desc: name
#% required: no
#%end

import grass.script as gscript


def main():
    options, flags = gscript.parser()

    import wx

    from grass.script.setup import set_gui_path
    set_gui_path()

    from psmap.frame import PsMapFrame

    app = wx.App()
    frame = PsMapFrame(parent=None)
    frame.Show()

    if options['file']:
        frame.LoadFile(options['file'])

    app.MainLoop()

if __name__ == "__main__":
    main()
