#!/usr/bin/env python3
#
# thumbnails.py: Create thumbnail images of the GRASS color tables
#
#  AUTHOR: Vaclav Petras (non-PPM version using r.mapcalc)
#          Glynn Clements (Python version)
#      Earlier Bourne script version by Hamish Bowman,
#      https://grasswiki.osgeo.org/wiki/Talk:Color_tables
#
#   (C) 2009-2017 by the GRASS Development Team
#       This program is free software under the GNU General Public
#       License (>=v2). Read the file COPYING that comes with GRASS
#       for details.
#

import os
import atexit
import sys
from pathlib import Path

import grass.script as gs


tmp_grad_abs = None
tmp_grad_rel = None


def cleanup():
    names = []
    if tmp_grad_rel:
        names.append(tmp_grad_rel)
    if tmp_grad_abs:
        names.append(tmp_grad_abs)
    if len(names) > 0:
        gs.run_command(
            "g.remove", flags="f", type="raster", name=",".join(names), quiet=True
        )


def make_gradient(path):
    text = Path(path).read_text()
    lines = text.splitlines()
    records = []
    for line in lines:
        if line.startswith("#"):
            # skip comments
            continue
        if len(line) == 0:
            # skip empty lines
            continue
        records.append(line.split())
    records = [
        record for record in records if record[0] != "nv" and record[0] != "default"
    ]
    relative = False
    absolute = False
    for record in records:
        if record[0].endswith("%"):
            relative = True
            record[0] = record[0].rstrip("%")
        else:
            absolute = True

    if absolute:
        if relative:
            minval = -0.04
            maxval = 0.04
        else:
            minval = float(records[0][0])
            # shift min up for floating point values so that
            # first color in color table is visible
            if "." in records[0][0]:
                # assumes that 1% of min does not go to the next value
                # and is still represented as float and does not make
                # too much difference in color
                # works better than 1% of the difference to the next value
                minval += abs(minval / 100)
            maxval = float(records[-1][0])
            maxval = min(maxval, 2500000)
        if os.path.basename(path) in {"ndvi", "ndwi", "ndwi2"}:
            minval = -1.0
            maxval = 1.0
        if os.path.basename(path) == "ndvi_MODIS":
            minval = -10000.0
            maxval = 10000.0
        if os.path.basename(path) == "population_dens":
            maxval = 1000.0
        if os.path.basename(path) == "precipitation":
            maxval = 2000.0
        if os.path.basename(path) in {"terrain", "srtm", "srtm_plus"}:
            minval = -500.0
            maxval = 3000.0
        grad = tmp_grad_abs
        gs.mapcalc(
            "$grad = "
            " float($min) + (col() - 1) * "
            "  (float($max) - float($min)) / ncols()",
            grad=tmp_grad_abs,
            min=minval,
            max=maxval,
            quiet=True,
        )
    else:
        grad = tmp_grad_rel

    return grad


def make_image(output_dir, table, grad, height, width):
    outfile = os.path.join(output_dir, "%s.png" % table)
    os.environ["GRASS_RENDER_FILE"] = outfile

    gs.run_command("r.colors", map=grad, color=table, quiet=True)
    os.environ["GRASS_RENDER_FRAME"] = "%f,%f,%f,%f" % (0, height, 2, width - 2)
    gs.run_command("d.rast", map=grad, quiet=True)
    if 1:
        os.environ["GRASS_RENDER_FRAME"] = "%f,%f,%f,%f" % (0, height, 0, width)
        gs.write_command(
            "d.graph",
            quiet=True,
            flags="m",
            stdin="""
        width 1
        color {outcolor}
        polyline
        {x1} {y1}
        {x2} {y1}
        {x2} {y2}
        {x1} {y2}
        {x1} {y1}
        color {incolor}
        polyline
        {x3} {y3}
        {x4} {y3}
        {x4} {y4}
        {x3} {y4}
        {x3} {y3}
        """.format(
                x1=1,
                x2=width,
                y1=0,
                y2=height - 1,
                x3=2,
                x4=width - 1,
                y3=1,
                y4=height - 2,
                outcolor="white",
                incolor="black",
            ),
        )


def main():
    global tmp_img, tmp_grad_abs, tmp_grad_rel

    height = 15
    width = 85

    os.environ["GRASS_OVERWRITE"] = "1"

    color_dir = os.path.join(os.environ["GISBASE"], "etc", "colors")
    output_dir = sys.argv[1]

    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    pid = os.getpid()
    tmp_grad_abs = "tmp_grad_abs_%d" % pid
    tmp_grad_rel = "tmp_grad_rel_%d" % pid

    os.environ["GRASS_RENDER_WIDTH"] = "%d" % width
    os.environ["GRASS_RENDER_HEIGHT"] = "%d" % height
    os.environ["GRASS_RENDER_TRUECOLOR"] = "TRUE"
    # for multiple d commands (requires to delete/move image each time)
    os.environ["GRASS_RENDER_FILE_READ"] = "TRUE"
    os.environ["GRASS_RENDER_FILE_MAPPED"] = "FALSE"
    os.environ["GRASS_RENDER_TRANSPARENT"] = "FALSE"
    os.environ["GRASS_RENDER_BACKGROUNDCOLOR"] = "ffffff"
    os.environ["GRASS_RENDER_IMMEDIATE"] = "cairo"
    # for one pixel wide lines
    os.environ["GRASS_RENDER_ANTIALIAS"] = "none"

    for var in ["GRASS_RENDER_LINE_WIDTH"]:
        if var in os.environ:
            del os.environ[var]

    os.environ["GRASS_ANTIALIAS"] = "none"

    gs.use_temp_region()
    gs.run_command(
        "g.region",
        s=0,
        w=0,
        n=height,
        e=width,
        rows=height,
        cols=width,
        res=1,
        flags="a",
    )

    gs.mapcalc("$grad = float(col())", grad=tmp_grad_rel, quiet=True)

    color_dir_path = Path(color_dir)
    for table_path in color_dir_path.iterdir():
        table = table_path.name
        grad = make_gradient(table_path)
        make_image(output_dir, table, grad, height=height, width=width)

    gs.mapcalc("$grad = col()", grad=tmp_grad_abs, quiet=True)
    for table in ["grey.eq", "grey.log", "random"]:
        make_image(output_dir, table, tmp_grad_abs, height=height, width=width)


if __name__ == "__main__":
    atexit.register(cleanup)
    main()
