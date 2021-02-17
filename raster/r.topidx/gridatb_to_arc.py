#!/usr/bin/env python3
import sys
import re
import os

if (
    len(sys.argv) == 1
    or len(sys.argv) == 4
    or len(sys.argv) > 5
    or re.match("^-*help", sys.argv[1])
):
    print("Usage: gridatb.to.arc.py gridatb_file arc_file [xllcorner yllcorner]")
    exit()

xllcorner = 0
yllcorner = 0
if len(sys.argv) == 5:
    xllcorner = sys.argv[3]
    yllcorner = sys.argv[4]

infname = sys.argv[1]
outfname = sys.argv[2]

if not os.path.isfile(infname):
    print(f"{infname}: File not found")
    exit()

if os.path.isfile(outfname):
    print(f"{outfname}: File already exists")
    exit()

inf = open(infname)

title = inf.readline()
inline = inf.readline()
m = re.match("^[ \t]*([0-9.]+)[ \t]+([0-9.]+)[ \t]+([0-9.]+)[ \t]*$", inline)
if not m:
    print(f"{infname}: Invalid input file format")
    inf.close()
    exit()

ncols = m.group(1)
nrows = m.group(2)
cellsize = m.group(3)

outf = open(outfname, "w")
outf.write(
    f"""\
ncols         {ncols}
nrows         {nrows}
xllcorner     {xllcorner}
yllcorner     {yllcorner}
cellsize      {cellsize}
NODATA_value  9999
"""
)
for inline in inf:
    outf.write(inline)

inf.close()
outf.close()
