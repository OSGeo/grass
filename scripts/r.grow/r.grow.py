#!/usr/bin/env python3
#
############################################################################
#
# MODULE:    r.grow
# AUTHOR(S): Glynn Clements
# PURPOSE:   Fast replacement for r.grow using r.grow.distance
#
# COPYRIGHT: (C) 2008 by Glynn Clements
#
#   This program is free software under the GNU General Public
#   License (>=v2). Read the file COPYING that comes with GRASS
#   for details.
#
#############################################################################

# %Module
# % description: Generates a raster map layer with contiguous areas grown by one cell.
# % keyword: raster
# % keyword: distance
# % keyword: proximity
# %end
# %flag
# % key: m
# % description: Radius is in map units rather than cells
# %end
# %option G_OPT_R_INPUT
# %end
# %option G_OPT_R_OUTPUT
# %end
# %option
# % key: radius
# % type: double
# % required: no
# % multiple: no
# % description: Radius of buffer in raster cells
# % answer: 1.01
# %end
# %option
# % key: metric
# % type: string
# % required: no
# % multiple: no
# % options: euclidean,maximum,manhattan
# % description: Metric
# % answer: euclidean
# %end
# %option
# % key: old
# % type: integer
# % required: no
# % multiple: no
# % description: Value to write for input cells which are non-NULL (-1 => NULL)
# %end
# %option
# % key: new
# % type: integer
# % required: no
# % multiple: no
# % description: Value to write for "grown" cells
# %end
# %option G_OPT_M_NPROCS
# %end

import os
import atexit
import math

import grass.script as gs
from grass.exceptions import CalledModuleError


# what to do in case of user break:
def cleanup():
    for map in [temp_dist, temp_val]:
        if map:
            gs.run_command("g.remove", flags="fb", quiet=True, type="rast", name=map)


def main():
    global temp_dist, temp_val

    input = options["input"]
    radius = float(options["radius"])
    metric = options["metric"]
    old = options["old"]
    new = options["new"]
    nprocs = options["nprocs"]
    mapunits = flags["m"]

    tmp = str(os.getpid())

    temp_dist = "r.grow.tmp.%s.dist" % tmp

    shrink = False
    if radius < 0.0:
        shrink = True
        radius = -radius

    if new == "" and not shrink:
        temp_val = "r.grow.tmp.%s.val" % tmp
        new = '"%s"' % temp_val
    else:
        temp_val = None

    if old == "":
        old = '"%s"' % input

    if not mapunits:
        kv = gs.region()
        scale = math.sqrt(float(kv["nsres"]) * float(kv["ewres"]))
        radius *= scale

    if metric == "euclidean":
        metric = "squared"
        radius *= radius

    radius_str = str(radius)
    if "e" in radius_str.lower():
        radius_str = f"{radius:.20f}".rstrip("0").rstrip(".")

    # check if input file exists
    if not gs.find_file(input)["file"]:
        gs.fatal(_("Raster map <%s> not found") % input)

    # Workaround for r.mapcalc bug #3475
    # Mapcalc will fail if output is a fully qualified map name
    out_name = options["output"].split("@")
    if len(out_name) == 2:
        if out_name[1] != gs.gisenv()["MAPSET"]:
            gs.fatal(_("Output can be written only to the current mapset"))
        output = out_name[0]
    else:
        output = out_name[0]

    if not shrink:
        try:
            gs.run_command(
                "r.grow.distance",
                input=input,
                metric=metric,
                distance=temp_dist,
                value=temp_val,
            )
        except CalledModuleError:
            gs.fatal(_("Growing failed. Removing temporary maps."))

        gs.mapcalc(
            '$output = if(!isnull("$input"),$old,if($dist < $radius,$new,null()))',
            output=output,
            input=input,
            radius=radius_str,
            old=old,
            new=new,
            dist=temp_dist,
            nprocs=nprocs,
        )
    else:
        # shrink
        try:
            gs.run_command(
                "r.grow.distance",
                input=input,
                metric=metric,
                distance=temp_dist,
                value=temp_val,
                flags="n",
            )
        except CalledModuleError:
            gs.fatal(_("Shrinking failed. Removing temporary maps."))

        gs.mapcalc(
            "$output = if(isnull($dist), $old, if($dist < $radius,null(),$old))",
            output=output,
            radius=radius_str,
            old=old,
            dist=temp_dist,
            nprocs=nprocs,
        )

    gs.run_command("r.colors", map=output, raster=input)

    # write cmd history:
    gs.raster_history(output)


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    main()
