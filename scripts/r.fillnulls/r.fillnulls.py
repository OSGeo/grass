#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       r.fillnulls
# AUTHOR(S):    Markus Neteler
#               Updated to GRASS 5.7 by Michael Barton
#               Updated to GRASS 6.0 by Markus Neteler
#               Ring and zoom improvements by Hamish Bowman
#               Converted to Python by Glynn Clements
#               Added support for r.resamp.bspline by Luca Delucchi
#               Per hole filling with RST by Maris Nartiss
#               Speedup for per hole filling with RST by Stefan Blumentrath
# PURPOSE:      fills NULL (no data areas) in raster maps
#               The script respects a user mask if present.
#
# COPYRIGHT:    (C) 2001-2025 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


# %module
# % description: Fills no-data areas in raster maps using spline interpolation.
# % keyword: raster
# % keyword: surface
# % keyword: elevation
# % keyword: interpolation
# % keyword: splines
# % keyword: no-data filling
# %end
# %option G_OPT_R_INPUT
# %end
# %option G_OPT_R_OUTPUT
# %end
# %option
# % key: method
# % type: string
# % description: Interpolation method to use
# % required: yes
# % options: bilinear,bicubic,rst
# % answer: rst
# %end
# %option
# % key: tension
# % type: double
# % description: Spline tension parameter
# % required : no
# % answer : 40.
# % guisection: RST options
# %end
# %option
# % key: smooth
# % type: double
# % description: Spline smoothing parameter
# % required : no
# % answer : 0.1
# % guisection: RST options
# %end
# %option
# % key: edge
# % type: integer
# % description: Width of hole edge used for interpolation (in cells)
# % required : no
# % answer : 3
# % options : 2-100
# % guisection: RST options
# %end
# %option
# % key: npmin
# % type: integer
# % description: Minimum number of points for approximation in a segment (>segmax)
# % required : no
# % answer : 600
# % options : 2-10000
# % guisection: RST options
# %end
# %option
# % key: segmax
# % type: integer
# % description: Maximum number of points in a segment
# % required : no
# % answer : 300
# % options : 2-10000
# % guisection: RST options
# %end
# %option
# % key: lambda
# % type: double
# % required: no
# % multiple: no
# % label: Tykhonov regularization parameter (affects smoothing)
# % description: Used in bilinear and bicubic spline interpolation
# % answer: 0.01
# % guisection: Spline options
# %end
# %option G_OPT_MEMORYMB
# %end


import os
import atexit
import subprocess

import grass.script as gs
from grass.exceptions import CalledModuleError

tmp_rmaps = []
tmp_vmaps = []
usermask = None
mapset = None

# what to do in case of user break:


def cleanup():
    # delete internal any temporary files:
    if len(tmp_vmaps) > 0:
        gs.run_command(
            "g.remove", quiet=True, flags="fb", type="vector", name=tmp_vmaps
        )
    if len(tmp_rmaps) > 0:
        gs.run_command(
            "g.remove", quiet=True, flags="fb", type="raster", name=tmp_rmaps
        )


def main():
    global usermask, mapset, tmp_rmaps, tmp_vmaps

    input = options["input"]
    output = options["output"]
    tension = options["tension"]
    smooth = options["smooth"]
    method = options["method"]
    edge = int(options["edge"])
    segmax = int(options["segmax"])
    npmin = int(options["npmin"])
    lambda_ = float(options["lambda"])
    memory = options["memory"]
    quiet = True  # FIXME
    mapset = gs.gisenv()["MAPSET"]
    unique = str(os.getpid())  # Shouldn't we use temp name?
    prefix = "r_fillnulls_%s_" % unique
    failed_list = []  # a list of failed holes. Caused by issues with v.surf.rst. Connected with #1813

    # check if input file exists
    if not gs.find_file(input)["file"]:
        gs.fatal(_("Raster map <%s> not found") % input)

    # save original region
    reg_org = gs.region()

    # Check if a raster mask is present.
    # We need to handle the mask in a special way, so we don't fill the masked parts.
    mask_status = gs.parse_command("r.mask.status", format="json")
    if mask_status["present"]:
        usermask = mask_status["name"]

    # check if method is rst to use v.surf.rst
    if method == "rst":
        # idea: filter all NULLS and grow that area(s) by 3 pixel, then
        # interpolate from these surrounding 3 pixel edge
        filling = prefix + "filled"

        gs.use_temp_region()
        gs.run_command("g.region", align=input, quiet=quiet)
        region = gs.region()
        ns_res = region["nsres"]
        ew_res = region["ewres"]

        gs.message(_("Using RST interpolation..."))
        gs.message(_("Locating and isolating NULL areas..."))

        # creating binary (0/1) map
        if usermask:
            # Disable masking to not interfere with NULL lookup part.
            gs.message(_("Skipping masked raster parts"))
            with gs.MaskManager():
                gs.mapcalc(
                    (
                        '$tmp1 = if(isnull("$input") && !($mask == 0 || isnull($mask)),'
                        "1,null())"
                    ),
                    tmp1=prefix + "nulls",
                    input=input,
                    mask=usermask,
                )
        else:
            gs.mapcalc(
                '$tmp1 = if(isnull("$input"),1,null())',
                tmp1=prefix + "nulls",
                input=input,
            )
        tmp_rmaps.append(prefix + "nulls")

        # grow identified holes by X pixels
        gs.message(_("Growing NULL areas"))
        tmp_rmaps.append(prefix + "grown")
        try:
            gs.run_command(
                "r.grow",
                input=prefix + "nulls",
                radius=edge + 0.01,
                old=1,
                new=1,
                out=prefix + "grown",
                quiet=quiet,
            )
        except CalledModuleError:
            gs.fatal(
                _("abandoned. Removing temporary map, restoring user mask if needed:")
            )

        # assign unique IDs to each hole or hole system (holes closer than edge
        # distance)
        gs.message(_("Assigning IDs to NULL areas"))
        tmp_rmaps.append(prefix + "clumped")
        try:
            gs.run_command(
                "r.clump",
                input=prefix + "grown",
                output=prefix + "clumped",
                quiet=quiet,
            )
        except CalledModuleError:
            gs.fatal(
                _("abandoned. Removing temporary map, restoring user mask if needed:")
            )

        # get a list of unique hole cat's
        gs.mapcalc(
            "$out = if(isnull($inp), null(), $clumped)",
            out=prefix + "holes",
            inp=prefix + "nulls",
            clumped=prefix + "clumped",
        )
        tmp_rmaps.append(prefix + "holes")

        # use new IDs to identify holes
        try:
            gs.run_command(
                "r.to.vect",
                flags="v",
                input=prefix + "holes",
                output=prefix + "holes",
                type="area",
                quiet=quiet,
            )
        except CalledModuleError:
            gs.fatal(
                _("abandoned. Removing temporary maps, restoring user mask if needed:")
            )
        tmp_vmaps.append(prefix + "holes")

        # get a list of unique hole cat's
        cats_file_name = gs.tempfile(False)
        gs.run_command(
            "v.db.select",
            flags="c",
            map=prefix + "holes",
            columns="cat",
            file=cats_file_name,
            quiet=quiet,
        )
        cat_list = []
        with open(cats_file_name) as cats_file:
            for line in cats_file:
                cat_list.append(line.rstrip("\n"))
        os.remove(cats_file_name)

        if len(cat_list) < 1:
            # no holes found in current region
            gs.run_command(
                "g.copy", raster="%s,%sfilled" % (input, prefix), overwrite=True
            )
            gs.warning(
                _(
                    "Input map <%s> has no holes. Copying to output without "
                    "modification."
                )
                % (input,)
            )

        # GTC Hole is NULL area in a raster map
        gs.message(_("Processing %d map holes") % len(cat_list))
        first = True
        hole_n = 1
        for cat in cat_list:
            holename = prefix + "hole_" + cat
            # GTC Hole is a NULL area in a raster map
            gs.message(_("Filling hole %s of %s") % (hole_n, len(cat_list)))
            hole_n += 1
            # cut out only CAT hole for processing
            try:
                gs.run_command(
                    "v.extract",
                    input=prefix + "holes",
                    output=holename + "_pol",
                    cats=cat,
                    quiet=quiet,
                )
            except CalledModuleError:
                gs.fatal(
                    _(
                        "abandoned. Removing temporary maps, restoring "
                        "user mask if needed:"
                    )
                )
            tmp_vmaps.append(holename + "_pol")

            # zoom to specific hole with a buffer of two cells around the hole to
            # remove rest of data
            try:
                gs.run_command(
                    "g.region",
                    vector=holename + "_pol",
                    align=input,
                    w="w-%d" % (edge * 2 * ew_res),
                    e="e+%d" % (edge * 2 * ew_res),
                    n="n+%d" % (edge * 2 * ns_res),
                    s="s-%d" % (edge * 2 * ns_res),
                    quiet=quiet,
                )
            except CalledModuleError:
                gs.fatal(
                    _(
                        "abandoned. Removing temporary maps, restoring "
                        "user mask if needed:"
                    )
                )

            # remove temporary map to not overfill disk
            try:
                gs.run_command(
                    "g.remove",
                    flags="fb",
                    type="vector",
                    name=holename + "_pol",
                    quiet=quiet,
                )
            except CalledModuleError:
                gs.fatal(
                    _(
                        "abandoned. Removing temporary maps, restoring "
                        "user mask if needed:"
                    )
                )
            tmp_vmaps.remove(holename + "_pol")

            # copy only data around hole
            gs.mapcalc(
                "$out = if($inp == $catn, $inp, null())",
                out=holename,
                inp=prefix + "holes",
                catn=cat,
            )
            tmp_rmaps.append(holename)

            # If here loop is split into two, next part of loop can be run in parallel
            # (except final result patching)
            # Downside - on large maps such approach causes large disk usage

            # grow hole border to get it's edge area
            tmp_rmaps.append(holename + "_grown")
            try:
                gs.run_command(
                    "r.grow",
                    input=holename,
                    radius=edge + 0.01,
                    old=-1,
                    out=holename + "_grown",
                    quiet=quiet,
                )
            except CalledModuleError:
                gs.fatal(
                    _(
                        "abandoned. Removing temporary map, restoring "
                        "user mask if needed:"
                    )
                )

            # no idea why r.grow old=-1 doesn't replace existing values with NULL
            gs.mapcalc(
                '$out = if($inp == -1, null(), "$dem")',
                out=holename + "_edges",
                inp=holename + "_grown",
                dem=input,
            )
            tmp_rmaps.append(holename + "_edges")

            # convert to points for interpolation
            tmp_vmaps.append(holename)
            try:
                gs.run_command(
                    "r.to.vect",
                    input=holename + "_edges",
                    output=holename,
                    type="point",
                    flags="z",
                    quiet=quiet,
                )
            except CalledModuleError:
                gs.fatal(
                    _(
                        "abandoned. Removing temporary maps, restoring "
                        "user mask if needed:"
                    )
                )

            # count number of points to control segmax parameter for interpolation:
            pointsnumber = gs.vector_info_topo(map=holename)["points"]
            gs.verbose(_("Interpolating %d points") % pointsnumber)

            if pointsnumber < 2:
                gs.verbose(_("No points to interpolate"))
                failed_list.append(holename)
                continue

            # Avoid v.surf.rst warnings
            if pointsnumber < segmax:
                use_npmin = pointsnumber
                use_segmax = pointsnumber * 2
            else:
                use_npmin = npmin
                use_segmax = segmax

            # launch v.surf.rst
            tmp_rmaps.append(holename + "_dem")
            try:
                gs.run_command(
                    "v.surf.rst",
                    quiet=quiet,
                    input=holename,
                    elev=holename + "_dem",
                    tension=tension,
                    smooth=smooth,
                    segmax=use_segmax,
                    npmin=use_npmin,
                )
            except CalledModuleError:
                # GTC Hole is NULL area in a raster map
                gs.fatal(_("Failed to fill hole %s") % cat)

            # v.surf.rst sometimes fails with exit code 0
            # related bug #1813
            if not gs.find_file(holename + "_dem")["file"]:
                try:
                    tmp_rmaps.remove(holename)
                    tmp_rmaps.remove(holename + "_grown")
                    tmp_rmaps.remove(holename + "_edges")
                    tmp_rmaps.remove(holename + "_dem")
                    tmp_vmaps.remove(holename)
                except ValueError:
                    pass
                gs.warning(
                    _(
                        "Filling has failed silently. Leaving temporary maps "
                        "with prefix <%s> for debugging."
                    )
                    % holename
                )
                failed_list.append(holename)
                continue

            # append hole result to interpolated version later used to patch into
            # original DEM
            if first:
                tmp_rmaps.append(filling)
                gs.run_command(
                    "g.region", align=input, raster=holename + "_dem", quiet=quiet
                )
                gs.mapcalc(
                    "$out = if(isnull($inp), null(), $dem)",
                    out=filling,
                    inp=holename,
                    dem=holename + "_dem",
                )
                first = False
            else:
                tmp_rmaps.append(filling + "_tmp")
                gs.run_command(
                    "g.region",
                    align=input,
                    raster=(filling, holename + "_dem"),
                    quiet=quiet,
                )
                gs.mapcalc(
                    "$out = if(isnull($inp), if(isnull($fill), null(), $fill), $dem)",
                    out=filling + "_tmp",
                    inp=holename,
                    dem=holename + "_dem",
                    fill=filling,
                )
                try:
                    gs.run_command(
                        "g.rename",
                        raster=(filling + "_tmp", filling),
                        overwrite=True,
                        quiet=quiet,
                    )
                except CalledModuleError:
                    gs.fatal(
                        _(
                            "abandoned. Removing temporary maps, restoring user "
                            "mask if needed:"
                        )
                    )
                # this map has been removed. No need for later cleanup.
                tmp_rmaps.remove(filling + "_tmp")

            # remove temporary maps to not overfill disk
            try:
                tmp_rmaps.remove(holename)
                tmp_rmaps.remove(holename + "_grown")
                tmp_rmaps.remove(holename + "_edges")
                tmp_rmaps.remove(holename + "_dem")
            except ValueError:
                pass
            try:
                gs.run_command(
                    "g.remove",
                    quiet=quiet,
                    flags="fb",
                    type="raster",
                    name=(
                        holename,
                        holename + "_grown",
                        holename + "_edges",
                        holename + "_dem",
                    ),
                )
            except CalledModuleError:
                gs.fatal(
                    _(
                        "abandoned. Removing temporary maps, restoring "
                        "user mask if needed:"
                    )
                )
            try:
                tmp_vmaps.remove(holename)
            except ValueError:
                pass
            try:
                gs.run_command(
                    "g.remove", quiet=quiet, flags="fb", type="vector", name=holename
                )
            except CalledModuleError:
                gs.fatal(
                    _(
                        "abandoned. Removing temporary maps, restoring user mask if "
                        "needed:"
                    )
                )

    # check if method is different from rst to use r.resamp.bspline
    if method != "rst":
        gs.message(_("Using %s bspline interpolation") % method)

        # clone current region
        gs.use_temp_region()
        gs.run_command("g.region", align=input)

        reg = gs.region()
        # launch r.resamp.bspline
        tmp_rmaps.append(prefix + "filled")
        # If there are no NULL cells, r.resamp.bslpine call
        # will end with an error although for our needs it's fine
        # Only problem - this state must be read from stderr
        new_env = dict(os.environ)
        new_env["LC_ALL"] = "C"
        if usermask:
            try:
                with gs.MaskManager():
                    p = gs.core.start_command(
                        "r.resamp.bspline",
                        input=input,
                        mask=usermask,
                        output=prefix + "filled",
                        method=method,
                        ew_step=3 * reg["ewres"],
                        ns_step=3 * reg["nsres"],
                        lambda_=lambda_,
                        memory=memory,
                        flags="n",
                        stderr=subprocess.PIPE,
                        env=new_env,
                    )
                    stderr = gs.decode(p.communicate()[1])
                if "No NULL cells found" in stderr:
                    gs.run_command(
                        "g.copy", raster="%s,%sfilled" % (input, prefix), overwrite=True
                    )
                    p.returncode = 0
                    gs.warning(
                        _(
                            "Input map <%s> has no holes. Copying to output without "
                            "modification."
                        )
                        % (input,)
                    )
            except CalledModuleError:
                gs.fatal(
                    _("Failure during bspline interpolation. Error message: %s")
                    % stderr
                )
        else:
            try:
                p = gs.core.start_command(
                    "r.resamp.bspline",
                    input=input,
                    output=prefix + "filled",
                    method=method,
                    ew_step=3 * reg["ewres"],
                    ns_step=3 * reg["nsres"],
                    lambda_=lambda_,
                    memory=memory,
                    flags="n",
                    stderr=subprocess.PIPE,
                    env=new_env,
                )
                stderr = gs.decode(p.communicate()[1])
                if "No NULL cells found" in stderr:
                    gs.run_command(
                        "g.copy", raster="%s,%sfilled" % (input, prefix), overwrite=True
                    )
                    p.returncode = 0
                    gs.warning(
                        _(
                            "Input map <%s> has no holes. Copying to output without "
                            "modification."
                        )
                        % (input,)
                    )
            except CalledModuleError:
                gs.fatal(
                    _("Failure during bspline interpolation. Error message: %s")
                    % stderr
                )

    # set region to original extents, align to input
    gs.run_command(
        "g.region",
        n=reg_org["n"],
        s=reg_org["s"],
        e=reg_org["e"],
        w=reg_org["w"],
        align=input,
    )

    # patch orig and fill map
    gs.message(_("Patching fill data into NULL areas..."))
    # we can use --o here as g.parser already checks on startup
    gs.run_command(
        "r.patch", input=(input, prefix + "filled"), output=output, overwrite=True
    )

    # restore the real region
    gs.del_temp_region()

    gs.message(_("Filled raster map is: %s") % output)

    # write cmd history:
    gs.raster_history(output)

    if len(failed_list) > 0:
        gs.warning(
            _(
                "Following holes where not filled. Temporary maps with are left "
                "in place to allow examination of unfilled holes"
            )
        )
        outlist = failed_list[0]
        for hole in failed_list[1:]:
            outlist = ", " + outlist
        gs.message(outlist)

    gs.message(_("Done."))


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    main()
