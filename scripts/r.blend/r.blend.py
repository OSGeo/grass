#!/usr/bin/env python3

############################################################################
#
# MODULE:	r.blend for GRASS 5.7; based on blend.sh for GRASS 5
# AUTHOR(S):	CERL?; updated to GRASS 5.7 by Michael Barton
#               Converted to Python by Glynn Clements
# PURPOSE:	To redraw current displayed maps to 24 bit PNG output
# COPYRIGHT:	(C) 2004-2012 by the GRASS Development Team
#
# 		This program is free software under the GNU General Public
# 		License (>=v2). Read the file COPYING that comes with GRASS
# 		for details.
#
#############################################################################

# %module
# % description: Blends color components of two raster maps by a given ratio.
# % keyword: raster
# % keyword: composite
# %end
# %option G_OPT_R_INPUT
# % key: first
# % description: Name of first raster map for blending
# %end
# %option G_OPT_R_INPUT
# % key: second
# % description: Name of second raster map for blending
# %end
# %option G_OPT_R_BASENAME_OUTPUT
# % description: Basename for red, green and blue output raster maps
# %end
# %option
# % key: percent
# % type: double
# % answer: 50
# % options : 0-100
# % description: Percentage weight of first map for color blending
# % required : no
# %end
# % flag
# % key: c
# % description: Combine resulting R,G,B layers into single output map
# %end

import os
import string
import grass.script as gs


def main():
    first = options["first"]
    second = options["second"]
    output = options["output"]
    percent = options["percent"]

    mapset = gs.gisenv()["MAPSET"]

    if not gs.overwrite():
        for ch in ["r", "g", "b"]:
            map = "%s.%s" % (output, ch)
            if gs.find_file(map, element="cell", mapset=mapset)["file"]:
                gs.fatal(_("Raster map <%s> already exists.") % map)

    percent = float(percent)
    perc_inv = 100.0 - percent

    frac1 = percent / 100.0
    frac2 = perc_inv / 100.0

    gs.message(_("Calculating the three component maps..."))

    template = string.Template(
        "$$output.$ch = "
        'if(isnull("$$first"), $ch#"$$second", '
        'if(isnull("$$second"), $ch#"$$first", '
        '$$frac1 * $ch#"$$first" + '
        '$$frac2 * $ch#"$$second"))'
    )
    cmd = [template.substitute(ch=ch) for ch in ["r", "g", "b"]]
    cmd = ";".join(cmd)

    gs.mapcalc(cmd, output=output, first=first, second=second, frac1=frac1, frac2=frac2)

    for ch in ["r", "g", "b"]:
        map = "%s.%s" % (output, ch)
        gs.run_command("r.colors", map=map, color="grey255")
        gs.run_command(
            "r.support",
            map=map,
            history="",
            title="Color blend of %s and %s" % (first, second),
            description="generated by r.blend",
        )
        gs.run_command("r.support", map=map, history="r.blend %s channel." % ch)
        gs.run_command(
            "r.support",
            map=map,
            history="  %d%% of %s, %d%% of %s" % (percent, first, perc_inv, second),
        )
        gs.run_command("r.support", map=map, history="")
        gs.run_command("r.support", map=map, history=os.environ["CMDLINE"])

    if flags["c"]:
        gs.run_command(
            "r.composite",
            r="%s.r" % output,
            g="%s.g" % output,
            b="%s.b" % output,
            output=output,
        )

        gs.run_command(
            "r.support",
            map=output,
            history="",
            title="Color blend of %s and %s" % (first, second),
            description="generated by r.blend",
        )
        gs.run_command(
            "r.support",
            map=output,
            history="  %d%% of %s, %d%% of %s" % (percent, first, perc_inv, second),
        )
        gs.run_command("r.support", map=output, history="")
        gs.run_command("r.support", map=output, history=os.environ["CMDLINE"])
    else:
        gs.message(_("Done. Use the following command to visualize the result:"))
        gs.message(_("d.rgb r=%s.r g=%s.g b=%s.b") % (output, output, output))


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
