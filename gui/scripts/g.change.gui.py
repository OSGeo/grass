#!/usr/bin/env python

############################################################################
#
# MODULE:       g.change.gui
# AUTHOR(S):    Hamish Bowman
# PURPOSE:      
# COPYRIGHT:    (C) 2009 GRASS Development Team
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
#%Module
#% description: Changes the default GRASS graphical user interface (GUI) setting.
#% keywords: general, gui
#%End
#%Option
#% key: gui
#% type: string
#% required: yes
#% multiple: no
#% options: wxpython,text
#% label: GUI type
#% description: Default value: GRASS_GUI if defined otherwise tcltk
#% descriptions: wxpython;wxPython based next generation GUI;text;command line interface only
#%End

# simple front end to g.gui to be used from within the GUI.

import grass
import sys

def main():
    grass.exec_command("g.gui", flags = 'nu', gui = options['gui'])
    sys.exit(ret)

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
