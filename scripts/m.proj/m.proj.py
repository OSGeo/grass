#!/usr/bin/env python3

"""
MODULE:    m.proj

AUTHOR(S): M. Hamish Bowman, Dept. Marine Science, Otago University,
           New Zealand
           Converted to Python by Glynn Clements

PURPOSE:   cs2cs reprojection frontend for a list of coordinates.
           Replacement for m.proj2 from GRASS 5

SPDX-FileCopyrightText: 2006-2019 Hamish Bowman
SPDX-FileCopyrightText: GRASS Development Team
SPDX-License-Identifier: GPL-2.0-or-later
"""

# notes:
#  - cs2cs expects "x y" data so be sure to send it "lon lat" not "lat lon"
#  - if you send cs2cs a third data column, beware it might be treated as "z"
# TODO:
#  - `cut` away x,y columns into a temp file, feed to cs2cs, then `paste`
#    back to input file. see method in v.in.garmin.sh. that way additional
#    numeric and string columns would survive the trip, and 3rd column would
#    not be modified as z.

# %module
# % description: Converts coordinates from one coordinate reference system to another (cs2cs frontend).
# % keyword: miscellaneous
# % keyword: projection
# % keyword: transformation
# %end
# %option G_OPT_M_COORDS
# % description: Input coordinates to reproject
# % guisection: Input coordinates
# %end
# %option G_OPT_F_INPUT
# % label: Name of input coordinate file
# % description: '-' for standard input
# % required: no
# % guisection: Input coordinates
# %end
# %option G_OPT_F_OUTPUT
# % description: Name for output coordinate file (omit to send to stdout)
# % required : no
# % guisection: Output
# %end
# %option G_OPT_F_SEP
# % label: Field separator (format: input[,output])
# % required : no
# % guisection: Input coordinates
# %end
# %option
# % key: proj_in
# % type: string
# % label: Input coordinate reference system
# % description: PROJ string, WKT, PROJJSON, or authority code such as EPSG:4326 (default: current project)
# % required : no
# % guisection: Projections
# %end
# %option
# % key: proj_out
# % type: string
# % label: Output coordinate reference system
# % description: PROJ string, WKT, PROJJSON, or authority code such as EPSG:4326
# % required : no
# % guisection: Projections
# %end
# %flag
# % key: i
# % description: Use LL WGS84 as input and current project as output CRS
# % guisection: Projections
# %end
# %flag
# % key: o
# % description: Use current project as input and LL WGS84 as output CRS
# % guisection: Projections
# %end
# %flag
# % key: d
# % description: Output long/lat in decimal degrees, or other projections with many decimal places
# % guisection: Output
# %end
# %flag
# % key: e
# % description: Include input coordinates in output file
# % guisection: Output
# %end
# %flag
# % key: c
# % description: Include column names in output file
# % guisection: Output
# %end
# %rules
# % required: coordinates, input
# % exclusive: coordinates, input
# % exclusive: proj_in, -i
# % exclusive: proj_out, -o
# % exclusive: -i, -o
# %end

import json
import os
import sys
import threading
from pathlib import Path
from grass.script.utils import separator, parse_key_val, encode, decode
from grass.script import core as gcore

# Longitude/latitude on WGS 84 with longitude first.
LL_WGS84 = "OGC:CRS84"


class TrThread(threading.Thread):
    def __init__(self, ifs, inf, outf):
        threading.Thread.__init__(self)
        self.ifs = ifs
        self.inf = inf
        self.outf = outf

    def run(self):
        while True:
            line = self.inf.readline()
            if not line:
                break
            line = line.replace(self.ifs, " ")
            line = encode(line)
            self.outf.write(line)
            self.outf.flush()

        self.outf.close()


def is_proj_string(crs):
    """Return True when the CRS is given as a PROJ string (starts with +)"""
    return crs.lstrip().startswith("+")


def run_projinfo(args, crs):
    """Run projinfo on a CRS definition and return its standard output"""
    if not gcore.find_program("projinfo"):
        gcore.fatal(
            _("projinfo program not found, install PROJ first: https://proj.org")
        )
    env = os.environ.copy()
    # Interpret +init=epsg:XXXX in PROJ strings the way PROJ 4 did.
    env["PROJ_USE_PROJ4_INIT_RULES"] = "YES"
    process = gcore.Popen(
        ["projinfo", "-q", *args, crs],
        stdout=gcore.PIPE,
        stderr=gcore.PIPE,
        env=env,
    )
    stdout, stderr = process.communicate()
    stdout = decode(stdout).strip()
    if process.returncode != 0 or not stdout:
        gcore.fatal(
            _("Unable to interpret coordinate reference system '{crs}': {msg}").format(
                crs=crs, msg=decode(stderr).strip()
            )
        )
    return stdout


def proj_string_to_wkt(crs):
    """Convert a PROJ string to a single-line WKT2 string"""
    if "+type=crs" not in crs:
        crs += " +type=crs"
    return run_projinfo(["-o", "WKT2:2019", "--single-line"], crs)


def first_axis_is_northing(crs):
    """Return True when the first axis of the CRS is latitude or northing

    cs2cs honors the axis order of a CRS given as WKT, PROJJSON, or
    authority code, while m.proj always reads and writes x y, i.e.,
    longitude latitude or easting northing.
    """
    definition = json.loads(run_projinfo(["-o", "PROJJSON"], crs))
    # Compound and bound CRSs carry the coordinate system in a nested CRS.
    while "coordinate_system" not in definition:
        if "components" in definition:
            definition = definition["components"][0]
        elif "source_crs" in definition:
            definition = definition["source_crs"]
        elif "base_crs" in definition:
            definition = definition["base_crs"]
        else:
            return False
    axes = definition["coordinate_system"].get("axis", [])
    return bool(axes) and axes[0].get("direction") in {"north", "south"}


def current_project_crs():
    """Return the CRS of the current project as a single-line WKT string"""
    return gcore.read_command("g.proj", flags="pf", format="wkt").strip()


def main():
    coords = options["coordinates"]
    input = options["input"]
    output = options["output"]
    fs = options["separator"]
    proj_in = options["proj_in"]
    proj_out = options["proj_out"]
    ll_in = flags["i"]
    ll_out = flags["o"]
    decimal = flags["d"]
    copy_input = flags["e"]
    include_header = flags["c"]

    # check for cs2cs
    if not gcore.find_program("cs2cs"):
        gcore.fatal(
            _(
                "cs2cs program not found, install PROJ first: \
            https://proj.org"
            )
        )

    # parse field separator
    # FIXME: input_x,y needs to split on multiple whitespace between them
    if fs == ",":
        ifs = ofs = ","
    else:
        try:
            ifs, ofs = fs.split(",")
        except ValueError:
            ifs = ofs = fs

    ifs = separator(ifs)
    ofs = separator(ofs)

    # set up projection params
    s = gcore.read_command("g.proj", flags="p", format="proj4")
    kv = parse_key_val(s)
    if "XY location" in kv:
        gcore.fatal(_("Unable to project to or from a XY location"))

    in_crs = None

    if ll_in:
        in_crs = LL_WGS84
        gcore.verbose("Assuming LL WGS84 as input, current project CRS as output")

    if ll_out:
        in_crs = current_project_crs()

    if proj_in:
        in_crs = proj_in

    if not in_crs:
        gcore.verbose("Assuming current project CRS as input")
        in_crs = current_project_crs()

    in_crs = in_crs.strip()
    gcore.verbose("Input CRS: '%s'" % in_crs)

    out_crs = None

    if ll_out:
        out_crs = LL_WGS84
        gcore.verbose("Assuming current project CRS as input, LL WGS84 as output")

    if ll_in:
        out_crs = current_project_crs()

    if proj_out:
        out_crs = proj_out

    if not out_crs:
        gcore.fatal(_("Missing output coordinate reference system"))
    out_crs = out_crs.strip()
    gcore.verbose("Output CRS: '%s'" % out_crs)

    # set up input file
    if coords:
        x, y = coords.split(",")
        tmpfile = gcore.tempfile()
        fd = open(tmpfile, "w")
        fd.write("%s%s%s\n" % (x, ifs, y))
        fd.close()
        inf = open(tmpfile)
    elif input == "-":
        infile = None
        inf = sys.stdin
    else:
        infile = input
        if not Path(infile).exists():
            gcore.fatal(_("Unable to read input data"))
        inf = open(infile)
        gcore.debug("input file=[%s]" % infile)

    # set up output file
    if not output:
        outfile = None
        outf = sys.stdout
    else:
        outfile = output
        outf = open(outfile, "w")
        gcore.debug("output file=[%s]" % outfile)

    # set up output style
    outfmt = ["-w5"] if not decimal else ["-f", "%.8f"]
    copyinp = [] if not copy_input else ["-E"]

    # do the conversion
    # Convert cs2cs DMS format to GRASS DMS format:
    #   cs2cs | sed -e 's/d/:/g' -e "s/'/:/g"  -e 's/"//g'

    if is_proj_string(in_crs) and is_proj_string(out_crs):
        # Two PROJ strings use the cs2cs "+to" syntax, which always reads
        # and writes longitude latitude or easting northing.
        cmd = ["cs2cs"] + copyinp + outfmt + in_crs.split() + ["+to"] + out_crs.split()
    else:
        # Anything else goes through the cs2cs "source_crs target_crs"
        # syntax. cs2cs does not mix the two syntaxes, so a PROJ string
        # is converted to WKT. The axis order of the CRS is honored by
        # cs2cs, so swap input (-r) and output (-s) when needed to keep
        # x y, i.e., longitude latitude or easting northing.
        if is_proj_string(in_crs):
            in_crs = proj_string_to_wkt(in_crs)
        if is_proj_string(out_crs):
            out_crs = proj_string_to_wkt(out_crs)
        axis_order = []
        if first_axis_is_northing(in_crs):
            axis_order.append("-r")
        if first_axis_is_northing(out_crs):
            axis_order.append("-s")
        cmd = ["cs2cs"] + copyinp + outfmt + axis_order + [in_crs, out_crs]

    gcore.debug("cs2cs command: %s" % cmd)
    p = gcore.Popen(cmd, stdin=gcore.PIPE, stdout=gcore.PIPE, text=False)

    tr = TrThread(ifs, inf, p.stdin)
    tr.start()

    if not copy_input:
        if include_header:
            outf.write("x%sy%sz\n" % (ofs, ofs))
        for line in p.stdout:
            try:
                xy, z = decode(line).split(" ", 1)
                x, y = xy.split("\t")
            except ValueError:
                gcore.fatal(line)

            outf.write("%s%s%s%s%s\n" % (x.strip(), ofs, y.strip(), ofs, z.strip()))
    else:
        if include_header:
            outf.write("input_x%sinput_y%sx%sy%sz\n" % (ofs, ofs, ofs, ofs))
        for line in p.stdout:
            inXYZ, x, rest = decode(line).split("\t")
            inX, inY = inXYZ.split(" ")[:2]
            y, z = rest.split(" ", 1)
            outf.write(
                "%s%s%s%s%s%s%s%s%s\n"
                % (
                    inX.strip(),
                    ofs,
                    inY.strip(),
                    ofs,
                    x.strip(),
                    ofs,
                    y.strip(),
                    ofs,
                    z.strip(),
                )
            )

    p.wait()

    if p.returncode != 0:
        gcore.fatal(
            _("Coordinate transformation failed (cs2cs exit code {})").format(
                p.returncode
            )
        )


if __name__ == "__main__":
    options, flags = gcore.parser()
    main()
