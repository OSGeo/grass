#!/usr/bin/env python3

############################################################################
#
# MODULE:       r.reclass.area
# AUTHOR(S):    NRCS
#               Converted to Python by Glynn Clements
#               Added rmarea method by Luca Delucchi
# PURPOSE:      Reclasses a raster map greater or less than user specified area size (in hectares)
# COPYRIGHT:    (C) 1999,2008,2014 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
# 04/2026: rewrite with Tools API and new options (Stefan Blumentrath)
# 10/2013: added option to use a pre-clumped input map (Eric Goddard)
# 8/2012: added fp maps support, cleanup, removed tabs AK
# 3/2007: added label support MN
# 3/2004: added parser support MN
# 11/2001 added mapset support markus
# 2/2001 fixes markus
# 2000: updated to GRASS 5
# 1998 from NRCS, slightly modified for GRASS 4.2.1

# %module
# % description: Reclasses a raster map greater or less than user specified area size (in hectares).
# % keyword: raster
# % keyword: statistics
# % keyword: aggregation
# %end

# %option G_OPT_R_INPUT
# %end

# %option G_OPT_R_OUTPUT
# %end

# %option
# % key: value
# % type: double
# % description: Value option that sets the area size limit (in hectares) (deprecated)
# % required: no
# % guisection: Area
# %end

# %option
# % key: mode
# % type: string
# % description: Keep only areas lesser or greater than specified value (deprecated)
# % options: lesser,greater
# % required: no
# % guisection: Area
# %end

# %option
# % key: lower
# % type: double
# % description: Value for the lower area size limit (in hectares). Pixel clumps with areas below this value are removed.
# % required: no
# % guisection: Area
# %end

# %option
# % key: upper
# % type: double
# % description: Value for the upper area size limit (in hectares). Pixel clumps with areas above this value are removed.
# % required: no
# % guisection: Area
# %end

# %option
# % key: method
# % type: string
# % description: Method used for area size filtering
# % options: reclass,rmarea
# % answer: reclass
# % guisection: Area
# %end

# %flag
# % key: c
# % description: Input map is clumped
# %end

# %flag
# % key: d
# % description: Generate clumps including diagonal neighbors
# %end

# %flag
# % key: i
# % description: Invert filter (remove clumps larger than lower and smaller than upper)
# %end

# %flag
# % key: m
# % description: Apply lower area threshold as "minsize" during clumping
# %end

# %flag
# % key: v
# % description: Output vector map, do not convert back to raster (only for method 'rmarea')
# %end

# %rules
# % required: lower, upper, mode
# % requires: mode, value
# % requires: -m, lower
# % excludes: -c, -d, -m
# %end


import atexit
import io
import sys

from functools import partial
from math import ceil, inf, prod
from typing import Literal

import grass.script as gs
from grass.tools import Tools

TEMP_PREFIX = gs.tempname(12)


def cleanup() -> None:
    """Delete temporary maps."""
    tools = Tools(capture_output=False)
    tools.g_remove(
        flags="fb",
        type=["raster", "vector"] if options["method"] == "rmarea" else "raster",
        pattern=f"{TEMP_PREFIX}_*",
        superquiet=True,
    )


def check_projection() -> tuple[float, float]:
    """Check if current projection is supported.

    Issues a fatal message if the tool does not reliably
    work in the current type of projection (XY, latlon)
    """
    tools = Tools(capture_output=True)
    proj = tools.g_region(flags="p", format="json").json
    if proj["crs"]["type_code"] == "0":
        gs.fatal(
            _(
                "xy-locations are not supported\n"
                "Need projected data with grids in meters",
            ),
        )
    in_proj = gs.parse_key_val(tools.g_proj(flags="p", format="shell").text)
    if in_proj["unit"].lower() == "degree":
        gs.fatal(_("Latitude-longitude locations are not supported"))
    if in_proj["name"].lower() == "xy_location_unprojected":
        gs.fatal(_("xy-locations are not supported"))
    return proj["ewres"], proj["nsres"]


def get_clumpfile(
    input_map: str,
    resolution: tuple[float, float],
    minsize: float | None = None,
    *,
    clump_flags: Literal["d"] | None = None,
) -> str:
    """Generate a clump-raster map."""
    tools = Tools(capture_output=False)
    clump_map = f"{TEMP_PREFIX}_clump"

    if minsize:
        minsize = ceil((minsize * 10000.0) / prod(resolution))

    msg = _("Generating a clumped raster file")
    if clump_flags:
        msg += _(" including diagonal neighbors")
    if minsize:
        msg += _(" with minimum %s pixels") % minsize
    gs.verbose(msg)
    tools.r_clump(
        flags=clump_flags,
        input=input_map,
        minsize=minsize,
        output=clump_map,
        quiet=True,
    )
    return clump_map


def reclass(
    clump_map: str,
    output_map: str | None,
    lower: float | None = None,
    upper: float | None = None,
    input_map: str | None = None,
) -> None:
    """Perform raster based filtering."""
    tools = Tools(capture_output=True)
    stats_input = clump_map
    expected_fields_number = 4
    verbose_message = _("Generating a reclass map with area size")
    range_filter_message = ""
    if lower:
        range_filter_message += _(" larger than or equal to %f hectares...") % lower
    else:
        lower = -inf

    if upper:
        if range_filter_message:
            range_filter_message += " and"
        range_filter_message += _(" smaller than or equal to %f hectares...") % upper
    else:
        upper = inf
    gs.verbose(verbose_message + range_filter_message)

    recfile = f"{TEMP_PREFIX}_recl"
    sflags = "aln"
    if input_map:
        expected_fields_number = 5
        stats_input = (clump_map, input_map)
        if gs.raster_info(input_map)["datatype"] in {"FCELL", "DCELL"}:
            sflags += "i"
    clump_areas = (
        tools.r_stats(
            flags=sflags,
            input=stats_input,
            sep=";",
            quiet=True,
        )
        .text.rstrip()
        .split("\n")
    )

    rules = ""
    for line in clump_areas:
        f = line.rstrip().split(";")
        if len(f) < expected_fields_number:
            continue
        hectares = float(f[-1]) * 0.0001
        if lower <= hectares <= upper:
            rules += "{} = {} {}\n".format(*[f[i] for i in [0, 2, -2]])
    if rules:
        tools.r_reclass(
            input=clump_map,
            output=recfile,
            rules=io.StringIO(rules),
            quiet=True,
        )
    else:
        fatal_message = _("No areas of size ")
        gs.fatal(fatal_message + range_filter_message)
    tools.r_mapcalc(expression=f"{output_map} = {recfile}")


def rmarea(
    input_map: str,
    output_map: str,
    lower: float | None = None,
    upper: float | None = None,
    *,
    return_vector: bool = False,
) -> None:
    """Perform vector based filtering."""
    # transform user input from hectares to map units (kept this for future)
    # thresh = thresh * 10000.0 / (float(coef)**2)
    # grass.debug("Threshold: %d, coeff linear: %s, coef squared: %d" %
    # (thresh, coef, (float(coef)**2)), 0)

    # transform user input from hectares to meters because currently v.clean
    # rmarea accept only meters as threshold
    tools = Tools(capture_output=False)

    vectfile = f"{TEMP_PREFIX}_vect"
    cleanfile = f"{TEMP_PREFIX}_clean"
    edit_vector = vectfile

    tools.r_to_vect(input=input_map, output=vectfile, type="area", quiet=True)

    if lower:
        lower *= 10000.0
        tools.v_clean(
            input=vectfile,
            output=cleanfile,
            tool="rmarea",
            threshold=lower,
            quiet=True,
        )
        edit_vector = cleanfile

    if upper:
        tools.v_to_db(
            map=edit_vector,
            type="centroid",
            option="area",
            columns="area_ha",
            units="hectares",
            quiet=True,
        )
        tools.v_edit(
            map=edit_vector, tool="delete", where=f"area_ha >= {upper}", superquiet=True
        )

    if not return_vector:
        tools.v_to_rast(
            input=edit_vector,
            output=output_map,
            use="attr",
            attrcolumn="value",
            quiet=True,
        )
    else:
        tools.v_clean(
            flags="c",
            input=edit_vector,
            output=output_map,
            tool=["rmdangle", "rmbridge"],
            threshold=-1,
            quiet=True,
        )


def main() -> None:
    """Run area filtering."""
    if not gs.find_file(options["input"])["name"]:
        gs.fatal(_("Raster map <%s> not found") % options["input"])

    # check for unsupported locations
    resolution = check_projection()

    # Give deprecation warnings
    if options["mode"]:
        gs.warning(
            _(
                "Options <mode> and <value> are deprecated. "
                "Use 'lower' and/or 'upper'.",
            ),
        )
        options["lower" if options["mode"] == "greater" else "upper"] = options["value"]

    lower = float(options["lower"]) if options["lower"] else None
    upper = float(options["upper"]) if options["upper"] else None
    # Compute clump map if neede
    clump_map = (
        options["input_map"]
        if flags["c"]
        else get_clumpfile(
            options["input"],
            resolution=resolution,
            minsize=lower if flags["m"] else None,
            clump_flags="d" if flags["d"] else None,
        )
    )

    # Filter clumped map
    filter_method = (
        partial(reclass, input_map=options["input"] if not flags["c"] else None)
        if options["method"] == "reclass"
        else partial(rmarea, return_vector=flags["v"])
    )
    filter_method(clump_map, options["output"], lower=lower, upper=upper)

    gs.message(_("Generating output raster map <%s>...") % options["output"])


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    sys.exit(main())
