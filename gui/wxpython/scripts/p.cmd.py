#!/usr/bin/env python
############################################################################
#
# MODULE:       p.cmd
# AUTHOR(S):    Martin Landa, Hamish Bowman
#               Converted to Python by Huidae Cho
# PURPOSE:      Wrapper for display commands and pX monitors
# COPYRIGHT:    (C) 2009 by The GRASS Development Team
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
#% description: Wrapper for display commands and pX monitors
#% keywords: display
#%End

#%Option
#% key: cmd
#% type: string
#% required: yes
#% multiple: no
#% label: Command to be performed
#% description: Example: "d.rast map=elevation.dem@PERMANENT catlist=1300-1400 -i"
#%End

#%Option
#% key: opacity
#% type: string
#% required: no
#% multiple: no
#% key_desc: val
#% description: Opacity level in percentage
#% answer: 100
#%End

import os
import grass.script as grass

def main():
    cmd_file = grass.gisenv()["GRASS_PYCMDFILE"]

    if cmd_file == "" or os.path.exists(cmd_file) == False:
        grass.message(_("GRASS_PYCMDFILE - File not found. Run p.mon."), "e")
        return

    cmd = options["cmd"]
    opacity = options["opacity"]

    fp = open(cmd_file, "a")
    fp.write("%s opacity=%s\n" % (cmd, opacity))
    fp.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
