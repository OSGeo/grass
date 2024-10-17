#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       r.mask
# AUTHOR(S):    Michael Barton, Arizona State University
#               Markus Neteler
#               Converted to Python by Glynn Clements
#               Markus Metz
# PURPOSE:      Facilitates creation of raster MASK
# COPYRIGHT:    (C) 2005-2013 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# %module
# % description: Creates a MASK for limiting raster operation.
# % keyword: raster
# % keyword: mask
# % keyword: null data
# % keyword: no-data
# % overwrite: yes
# %end
# %option G_OPT_R_INPUT
# % key: raster
# % description: Name of raster map to use as mask
# % required: NO
# % guisection: Raster
# %end
# %option
# % key: maskcats
# % type: string
# % label: Raster values to use for mask
# % description: Format: 1 2 3 thru 7 *
# % answer: *
# % guisection: Raster
# %end
# %option G_OPT_V_INPUT
# % key: vector
# % label: Name of vector map to use as mask
# % required: NO
# % guisection: Vector
# %end
# %option G_OPT_V_FIELD
# % label: Layer number or name (vector)
# % required: NO
# % guisection: Vector
# %end
# %option G_OPT_V_CATS
# % label: Category values (vector)
# % guisection: Vector
# %end
# %option G_OPT_DB_WHERE
# % label: WHERE conditions of SQL statement without 'where' keyword (vector)
# % guisection: Vector
# %end
# %flag
# % key: i
# % description: Create inverse mask
# % guisection: Create
# %end
# %flag
# % key: r
# % description: Remove existing mask (overrides other options)
# % guisection: Remove
# %end

import os
import sys
import atexit

import grass.script as gs
from grass.script.utils import encode
from grass.exceptions import CalledModuleError


def cleanup():
    if tmp:
        gs.run_command("g.remove", flags="f", type="raster", name=tmp, quiet=True)
    if tmp_hull:
        gs.run_command("g.remove", flags="f", type="vector", name=tmp_hull, quiet=True)


def main():
    raster = options["raster"]
    maskcats = options["maskcats"]
    vector = options["vector"]
    layer = options["layer"]
    cats = options["cats"]
    where = options["where"]
    remove = flags["r"]
    invert = flags["i"]

    if not remove and not raster and not vector:
        gs.fatal(_("Either parameter <raster> or parameter <vector> is required"))

    mapset = gs.gisenv()["MAPSET"]
    exists = bool(gs.find_file("MASK", element="cell", mapset=mapset)["file"])

    if remove:
        # -> remove
        if exists:
            if sys.platform == "win32":
                gs.run_command(
                    "g.remove", flags="if", quiet=True, type="raster", name="MASK"
                )
            else:
                gs.run_command(
                    "g.remove", flags="f", quiet=True, type="raster", name="MASK"
                )
            gs.message(_("Raster MASK removed"))
        else:
            gs.fatal(_("No existing MASK to remove"))
    else:
        # -> create
        if exists:
            if not gs.overwrite():
                gs.fatal(
                    _(
                        "MASK already found in current mapset. Delete first or "
                        "overwrite."
                    )
                )
            else:
                gs.warning(_("MASK already exists and will be overwritten"))
                gs.run_command(
                    "g.remove", flags="f", quiet=True, type="raster", name="MASK"
                )

        if raster:
            # check if input raster exists
            if not gs.find_file(raster)["file"]:
                gs.fatal(_("Raster map <%s> not found") % raster)

            if maskcats != "*" and not remove:
                if gs.raster_info(raster)["datatype"] != "CELL":
                    gs.fatal(
                        _(
                            "The raster map <%s> must be integer (CELL type) "
                            " in order to use the 'maskcats' parameter"
                        )
                        % raster
                    )

            p = gs.feed_command(
                "r.reclass", input=raster, output="MASK", overwrite=True, rules="-"
            )
            res = "%s = 1" % maskcats
            p.stdin.write(encode(res))
            p.stdin.close()
            p.wait()
        elif vector:
            vector_name = gs.find_file(vector, "vector")["fullname"]
            if not vector_name:
                gs.fatal(_("Vector map <%s> not found") % vector)

            # parser bug?
            if len(cats) == 0:
                cats = None
            if len(where) == 0:
                where = None

            if gs.vector_info_topo(vector_name)["areas"] < 1:
                gs.warning(
                    _(
                        "No area found in vector map <%s>. "
                        "Creating a convex hull for MASK."
                    )
                    % vector_name
                )
                global tmp_hull
                tmp_hull = "tmp_hull_%d" % os.getpid()
                to_rast_input = tmp_hull
                # force 'flat' convex hull for 3D vector maps
                try:
                    gs.run_command(
                        "v.hull",
                        flags="f",
                        quiet=True,
                        input=vector_name,
                        output=tmp_hull,
                        layer=layer,
                        cats=cats,
                        where=where,
                    )
                except CalledModuleError:
                    gs.fatal(
                        _("Unable to create a convex hull for vector map <%s>")
                        % vector_name
                    )
            else:
                to_rast_input = vector_name

            env = os.environ.copy()
            if gs.verbosity() > 1:
                env["GRASS_VERBOSE"] = "1"
            gs.run_command(
                "v.to.rast",
                input=to_rast_input,
                layer=layer,
                output="MASK",
                use="value",
                value="1",
                type="area",
                cats=cats,
                where=where,
                env=env,
            )

        if invert:
            global tmp
            tmp = "r_mask_%d" % os.getpid()
            gs.run_command("g.rename", raster=("MASK", tmp), quiet=True)
            gs.message(_("Creating inverted raster MASK..."))
            gs.mapcalc("MASK = if(isnull($tmp), 1, null())", tmp=tmp)
            gs.verbose(_("Inverted raster MASK created"))
        else:
            gs.verbose(_("Raster MASK created"))

        gs.message(
            _(
                "All subsequent raster operations will be limited to "
                "the MASK area. Removing or renaming raster map named "
                "'MASK' will restore raster operations to normal."
            )
        )


if __name__ == "__main__":
    options, flags = gs.parser()
    tmp = tmp_hull = None
    atexit.register(cleanup)
    main()
