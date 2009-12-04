#!/usr/bin/env python
############################################################################
#
# MODULE:       p.rast
# AUTHOR(S):    Jachym Cepicky, Martin Landa, Hamish Bowman
#               Converted to Python by Huidae Cho
# PURPOSE:      Displays raster map layer in the active map display window.
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

#%module
#% description: Displays raster map layer in the active map display window.
#% keywords: display, raster
#%end
#%flag
#% key: n
#% description: Make null cells opaque
#%end
#%flag
#% key: i
#% description: Invert catlist
#% guisection: Selection
#%end
#%option
#% key: map
#% type: string
#% required: yes
#% multiple: no
#% key_desc: name
#% description: Raster map to be displayed
#% gisprompt: old,cell,raster
#%end
#%option
#% key: catlist
#% type: string
#% required: no
#% multiple: yes
#% key_desc: cat[-cat]
#% description: List of categories to be displayed (INT maps)
#% guisection: Selection
#%end
#%option
#% key: vallist
#% type: string
#% required: no
#% multiple: yes
#% key_desc: val[-val]
#% description: List of values to be displayed (FP maps)
#% guisection: Selection
#%end
#%option
#% key: bg
#% type: string
#% required: no
#% multiple: no
#% key_desc: color
#% description: Background color (for null)
#% gisprompt: old_color,color,color
#%end
#%option
#% key: opacity
#% type: string
#% required: no
#% multiple: no
#% key_desc: val
#% answer: 100
#% description: Set opacity between 0-100%
#%end

import sys
import os
import grass.script as grass

def construct_command(cmd):
    line = cmd
    for key, val in options.iteritems():
        if val != "":
            line += " %s=%s" % (key, val)
    for key, val in flags.iteritems():
        if val == True:
            line += " -%s" % key
    return line

def main():
    cmd_file = grass.gisenv()["GRASS_PYCMDFILE"]

    if cmd_file == "" or os.path.exists(cmd_file) == False:
        grass.message(_("GRASS_PYCMDFILE - File not found. Run p.mon."), "e")
        return

    cmd = construct_command("d"+os.path.basename(sys.argv[0])[1:-3])

    fp = open(cmd_file, "a")
    fp.write("%s\n" % cmd)
    fp.close()

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
