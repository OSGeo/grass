#!/usr/bin/env python3

############################################################################
#
# MODULE:       wxpyimgview
# AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
# COPYRIGHT:    (C) 2010 Glynn Clements
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
# /

#%module
#% description: Views BMP images from the PNG driver.
#% keyword: display
#% keyword: graphics
#% keyword: raster
#%end
#%option G_OPT_F_INPUT
#% key: image
#% description: Name of input image file
#%end
#%option
#% key: percent
#% type: integer
#% required: no
#% multiple: no
#% description: Percentage of CPU time to use
#% answer: 10
#%end

import sys
import os
import grass.script as grass

if __name__ == "__main__":
    options, flags = grass.parser()
    image = options['image']
    percent = options['percent']
    python = os.getenv('GRASS_PYTHON', 'python')
    gisbase = os.environ['GISBASE']
    script = os.path.join(gisbase, "etc", "wxpyimgview_gui.py")
    os.execlp(python, script, script, image, percent)
