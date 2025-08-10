#!/usr/bin/env python3

############################################################################
#
# MODULE:       r.rgb
# AUTHOR(S):	Glynn Clements
# PURPOSE:	Split a raster map into red, green and blue maps
# COPYRIGHT:	(C) 2009 Glynn Clements and the GRASS Development Team
#
# 		This program is free software under the GNU General Public
# 		License (>=v2). Read the file COPYING that comes with GRASS
# 		for details.
#
#############################################################################

# %module
# % description: Splits a raster map into red, green and blue maps.
# % keyword: raster
# % keyword: RGB
# % keyword: separate
# % keyword: split
# %end
# %option G_OPT_R_INPUT
# %end
# %option G_OPT_R_OUTPUT
# % key: red
# % description: Red channel raster map name
# % required: no
# %end
# %option G_OPT_R_OUTPUT
# % key: green
# % description: Green channel raster map name
# % required: no
# %end
# %option G_OPT_R_OUTPUT
# % key: blue
# % description: Blue channel raster map name
# % required: no
# %end
# %rules
# % required: red, green, blue
# %end

import grass.script as gs


def main():
    options, unused = gs.parser()
    input = options["input"]
    red = options["red"]
    green = options["green"]
    blue = options["blue"]

    if not gs.find_file(input)["file"]:
        gs.fatal(_("Raster map <%s> not found") % input)

    expressions = []
    maps = []
    if red:
        expressions.append("%s = r#${input}" % red)  # noqa: RUF027
        maps.append(red)
    if green:
        expressions.append("%s = g#${input}" % green)  # noqa: RUF027
        maps.append(green)
    if blue:
        expressions.append("%s = b#${input}" % blue)  # noqa: RUF027
        maps.append(blue)
    expr = ";".join(expressions)
    gs.mapcalc(expr, input=input)

    for name in maps:
        gs.run_command("r.colors", map=name, color="grey255")
        gs.raster_history(name)


if __name__ == "__main__":
    main()
