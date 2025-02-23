#!/usr/bin/env python3

############################################################################
#
# MODULE:	t.rast.colors
# AUTHOR(S):	Soeren Gebbert
#
# PURPOSE:  Creates/modifies the color table associated with each raster map of the space time raster dataset.
# COPYRIGHT:	(C) 2011-2017 by the GRASS Development Team
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
#############################################################################

# %module
# % description: Creates/modifies the color table associated with each raster map of the space time raster dataset.
# % keyword: temporal
# % keyword: color table
# % keyword: raster
# % keyword: time
# %end

# %option G_OPT_STRDS_INPUT
# %end

# %option G_OPT_M_COLR
# % key: color
# % type: string
# % description: Name of color table (see r.color help)
# % required: no
# % multiple: no
# % guisection: Define
# %end

# %option G_OPT_R_INPUT
# % key: raster
# % description: Raster map from which to copy color table
# % required: no
# % guisection: Define
# %end

# %option G_OPT_R3_INPUT
# % key: raster_3d
# % description: 3D raster map from which to copy color table
# % required: no
# % guisection: Define
# %end

# %option G_OPT_F_INPUT
# % key: rules
# % description: Path to rules file
# % required: no
# % guisection: Define
# %end

# %flag
# % key: r
# % description: Remove existing color table
# % guisection: Remove
# %end

# %flag
# % key: w
# % description: Only write new color table if it does not already exist
# %end

# %flag
# % key: l
# % description: List available rules then exit
# % guisection: Print
# %end

# %flag
# % key: n
# % description: Invert colors
# % guisection: Define
# %end

# %flag
# % key: g
# % description: Logarithmic scaling
# % guisection: Define
# %end

# %flag
# % key: a
# % description: Logarithmic-absolute scaling
# % guisection: Define
# %end

# %flag
# % key: e
# % description: Histogram equalization
# % guisection: Define
# %end

import grass.script as gs
from grass.exceptions import CalledModuleError
from pathlib import Path
############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    # Get the options
    input = options["input"]
    color = options["color"]
    raster = options["raster"]
    volume = options["raster_3d"]
    rules = options["rules"]
    remove = flags["r"]
    write = flags["w"]
    list = flags["l"]
    invert = flags["n"]
    log = flags["g"]
    abslog = flags["a"]
    equi = flags["e"]

    if raster == "":
        raster = None

    if volume == "":
        volume = None

    if rules == "":
        rules = None

    if color == "":
        color = None

    # Make sure the temporal database exists
    tgis.init()

    sp = tgis.open_old_stds(input, "strds")

    rows = sp.get_registered_maps("id", None, None, None)

    if rows:
        flags_ = ""
        if remove:
            flags_ += "r"
        if write:
            flags_ += "w"
        if list:
            flags_ += "l"
        if invert:
            flags_ += "n"
        if log:
            flags_ += "g"
        if abslog:
            flags_ += "a"
        if equi:
            flags_ += "e"

        # Create the r.colors input file
        filename = gs.tempfile(True)
        Path(filename).write_text("\n".join(str(row["id"]) for row in rows))

        try:
            gs.run_command(
                "r.colors",
                flags=flags_,
                file=filename,
                color=color,
                raster=raster,
                volume=volume,
                rules=rules,
                overwrite=gs.overwrite(),
            )
        except CalledModuleError:
            gs.fatal(_("Error in r.colors call"))


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
