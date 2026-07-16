#!/usr/bin/env python3

############################################################################
#
# MODULE:    d.background
# AUTHOR(S): Vaclav Petras
# PURPOSE:   Uses d.his to drape a color raster over a shaded relief map
# SPDX-FileCopyrightText: 2022 Vaclav Petras
# SPDX-FileCopyrightText: Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#############################################################################

# %module
# % description: Fills the graphics display frame with user defined color.
# % keyword: display
# % keyword: graphics
# % keyword: background
# % keyword: fill
# % keyword: erase
# %end
# %option
# % key: color
# % type: string
# % required: yes
# % key_desc: name
# % label: Background color
# % description: Either a standard color name or R:G:B triplet
# % gisprompt: old,color,color
# %end


import grass.script as gs


def main():
    options, unused = gs.parser()
    color = options["color"]
    gs.run_command("d.erase", bgcolor=color, errors="fatal")


if __name__ == "__main__":
    main()

