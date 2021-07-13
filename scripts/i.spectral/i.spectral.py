#!/usr/bin/env python3

############################################################################
#
# MODULE:       i.spectral
# AUTHOR(S):    Markus Neteler, 18. August 1998
#               Converted to Python by Glynn Clements
# PURPOSE:      Displays spectral response at user specified locations in
#               group or raster images
# COPYRIGHT:    (C) 1999-2013 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
#
#  this script needs gnuplot for pretty rendering
#  TODO: use PyPlot like the wxGUI Profiling tool
#
# written by Markus Neteler 18. August 1998
#            neteler geog.uni-hannover.de
#
# bugfix: 25. Nov.98/20. Jan. 1999
# 3 March 2006: Added multiple images and group support by Francesco Pirotti - CIRGEO
#

# %Module
# % description: Displays spectral response at user specified locations in group or images.
# % keyword: imagery
# % keyword: querying
# % keyword: raster
# % keyword: multispectral
# %End
# %option G_OPT_I_GROUP
# % required : no
# % guisection: Input
# %end
# %option G_OPT_I_SUBGROUP
# % required : no
# % guisection: Input
# %end
# %option G_OPT_R_INPUTS
# % key: raster
# % required : no
# % guisection: Input
# %end
# %option G_OPT_M_COORDS
# % multiple: yes
# % required: yes
# % guisection: Input
# %end
# %option G_OPT_F_OUTPUT
# % key: output
# % description: Name for output image (or text file for -t)
# % guisection: Output
# % required : no
# %end
# %Option
# % key: format
# % type: string
# % description: Graphics format for output file
# % options: png,eps,svg
# % answer: png
# % multiple: no
# % guisection: Output
# %End
# %flag
# % key: c
# % description: Show sampling coordinates instead of numbering in the legend
# %end
# % flag
# % key: g
# % description: Use gnuplot for display
# %end
# % flag
# % key: t
# % description: output to text file
# %end

import os
import atexit
from grass.script.utils import try_rmdir
from grass.script import core as gcore


def cleanup():
    try_rmdir(tmp_dir)


def write2textf(what, output):
    outf = open(output, "w")
    i = 0
    for row in enumerate(what):
        i = i + 1
        outf.write("%d, %s\n" % (i, row))
    outf.close()


def draw_gnuplot(what, xlabels, output, img_format, coord_legend):
    xrange = 0

    for i, row in enumerate(what):
        outfile = os.path.join(tmp_dir, "data_%d" % i)
        outf = open(outfile, "w")
        xrange = max(xrange, len(row) - 2)
        for j, val in enumerate(row[3:]):
            outf.write("%d %s\n" % (j + 1, val))
        outf.close()

    # build gnuplot script
    lines = []
    if output:
        if img_format == "png":
            term_opts = "png truecolor large size 825,550"
        elif img_format == "eps":
            term_opts = "postscript eps color solid size 6,4"
        elif img_format == "svg":
            term_opts = "svg size 825,550 dynamic solid"
        else:
            gcore.fatal(_("Programmer error (%s)") % img_format)

        lines += ["set term " + term_opts, "set output '%s'" % output]

    lines += [
        "set xtics (%s)" % xlabels,
        "set grid",
        "set title 'Spectral signatures'",
        "set xrange [0.5 : %d - 0.5]" % xrange,
        "set noclabel",
        "set xlabel 'Bands'",
        "set xtics rotate by -40",
        "set ylabel 'DN Value'",
        "set style data lines",
    ]

    cmd = []
    for i, row in enumerate(what):
        if not coord_legend:
            title = "Pick " + str(i + 1)
        else:
            title = str(tuple(row[0:2]))

        x_datafile = os.path.join(tmp_dir, "data_%d" % i)
        cmd.append(" '%s' title '%s'" % (x_datafile, title))

    cmd = ",".join(cmd)
    cmd = " ".join(["plot", cmd, "with linespoints pt 779"])
    lines.append(cmd)

    plotfile = os.path.join(tmp_dir, "spectrum.gnuplot")
    plotf = open(plotfile, "w")
    for line in lines:
        plotf.write(line + "\n")
    plotf.close()

    if output:
        gcore.call(["gnuplot", plotfile])
    else:
        gcore.call(["gnuplot", "-persist", plotfile])


def draw_linegraph(what):
    yfiles = []

    xfile = os.path.join(tmp_dir, "data_x")

    xf = open(xfile, "w")
    for j, val in enumerate(what[0][3:]):
        xf.write("%d\n" % (j + 1))
    xf.close()

    for i, row in enumerate(what):
        yfile = os.path.join(tmp_dir, "data_y_%d" % i)
        yf = open(yfile, "w")
        for j, val in enumerate(row[3:]):
            yf.write("%s\n" % val)
        yf.close()
        yfiles.append(yfile)

    sienna = "#%02x%02x%02x" % (160, 82, 45)
    coral = "#%02x%02x%02x" % (255, 127, 80)
    gp_colors = ["red", "green", "blue", "magenta", "cyan", sienna, "orange", coral]

    colors = gp_colors
    while len(what) > len(colors):
        colors += gp_colors
    colors = colors[0 : len(what)]

    gcore.run_command(
        "d.linegraph",
        x_file=xfile,
        y_file=yfiles,
        y_color=colors,
        title=_("Spectral signatures"),
        x_title=_("Bands"),
        y_title=_("DN Value"),
    )


def main():
    group = options["group"]
    subgroup = options["subgroup"]
    raster = options["raster"]
    output = options["output"]
    coords = options["coordinates"]
    img_fmt = options["format"]
    coord_legend = flags["c"]
    gnuplot = flags["g"]
    textfile = flags["t"]

    global tmp_dir
    tmp_dir = gcore.tempdir()

    if not group and not raster:
        gcore.fatal(_("Either group= or raster= is required"))

    if group and raster:
        gcore.fatal(_("group= and raster= are mutually exclusive"))

    # -t needs an output filename
    if textfile and not output:
        gcore.fatal(_("Writing to text file requires output=filename"))

    # check if gnuplot is present
    if gnuplot and not gcore.find_program("gnuplot", "-V"):
        gcore.fatal(_("gnuplot required, please install first"))

    # get data from group listing and set the x-axis labels
    if group:
        # Parse the group list output
        if subgroup:
            s = gcore.read_command(
                "i.group", flags="g", group=group, subgroup=subgroup, quiet=True
            )
        else:
            s = gcore.read_command("i.group", flags="g", group=group, quiet=True)
        rastermaps = s.splitlines()
    else:
        # get data from list of files and set the x-axis labels
        rastermaps = raster.split(",")

    xlabels = ["'%s' %d" % (n, i + 1) for i, n in enumerate(rastermaps)]
    xlabels = ",".join(xlabels)

    # get y-data for gnuplot-data file
    what = []
    s = gcore.read_command(
        "r.what", map=rastermaps, coordinates=coords, null="0", quiet=True
    )
    if len(s) == 0:
        gcore.fatal(_("No data returned from query"))

    for line in s.splitlines():
        f = line.split("|")
        for i, v in enumerate(f):
            if v in ["", "*"]:
                f[i] = 0
            else:
                f[i] = float(v)
        what.append(f)

    # build data files
    if gnuplot:
        draw_gnuplot(what, xlabels, output, img_fmt, coord_legend)
    elif textfile:
        write2textf(what, output)
    else:
        draw_linegraph(what)


if __name__ == "__main__":
    options, flags = gcore.parser()
    atexit.register(cleanup)
    main()
