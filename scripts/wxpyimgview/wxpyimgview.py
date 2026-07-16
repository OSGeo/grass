#!/usr/bin/env python3

############################################################################
#
# MODULE:       wxpyimgview
# AUTHOR(S):    Glynn Clements <glynn@gclements.plus.com>
# SPDX-FileCopyrightText: 2010 Glynn Clements
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
# /

# %module
# % description: Views BMP images from the PNG driver.
# % keyword: display
# % keyword: graphics
# % keyword: raster
# %end
# %option G_OPT_F_INPUT
# % key: image
# % description: Name of input image file
# %end
# %option
# % key: percent
# % type: integer
# % required: no
# % multiple: no
# % description: Percentage of CPU time to use
# % answer: 10
# %end

import os
import grass.script as gs

if __name__ == "__main__":
    options, flags = gs.parser()
    image = options["image"]
    percent = options["percent"]
    python = os.getenv("GRASS_PYTHON", "python")
    gisbase = os.environ["GISBASE"]
    script = os.path.join(gisbase, "etc", "wxpyimgview_gui.py")
    os.execlp(python, script, script, image, percent)

