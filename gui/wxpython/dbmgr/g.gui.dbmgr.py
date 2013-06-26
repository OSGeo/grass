#!/usr/bin/env python
############################################################################
#
# MODULE:    g.gui.dbmgr
# AUTHOR(S): Martin Landa <landa.martin gmail.com>
# PURPOSE:   Attribute Table Manager
# COPYRIGHT: (C) 2012 by Martin Landa, and the GRASS Development Team
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
#% keywords: general
#% keywords: GUI
#% keywords: attribute table
#% keywords: database
#%end
#%option G_OPT_V_MAP
#%end

import os
import sys

import  wx
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

import grass.script as grass

gui_wx_path = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
if gui_wx_path not in sys.path:
    sys.path.append(gui_wx_path)

from dbmgr.manager import AttributeManager

def main():
    mapName = grass.find_file(options['map'], element = 'vector')['fullname']
    if not mapName:
        grass.set_raise_on_error(False)
        grass.fatal(_("Vector map <%s> not found") % options['map'])
    
    app = wx.PySimpleApp()
    grass.message(_("Loading attribute data for vector map <%s>...") % mapName)
    f = AttributeManager(parent = None, id = wx.ID_ANY,
                         title = "%s - <%s>" % (_("GRASS GIS Attribute Table Manager"),
                                                mapName),
                         size = (900, 600), vectorName = mapName)
    f.Show()
    
    app.MainLoop()
    
if __name__ == "__main__":
    options, flags = grass.parser()
    main()
