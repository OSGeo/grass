#!/usr/bin/env python3
import sys
import re
import os


def match(pattern, string):
    m = re.match(pattern, string, re.IGNORECASE)
    if m:
        match.value = m.group(1)
        return True
    match.value = None
    return False


if len(sys.argv) != 3 or re.match(r"^-*help", sys.argv[1]):
    print("Usage: arc.to.gridatb.py arc_file gridatb_file")
    sys.exit()

infname = sys.argv[1]
outfname = sys.argv[2]

if not os.path.isfile(infname):
    print(f"{infname}: File not found")
    sys.exit()

if os.path.isfile(outfname):
    print(f"{outfname}: File already exists")
    sys.exit()

inf = open(infname)

head = 0
for inline in inf:
    if match("^ncols[ \t]+([0-9.]+)[ \t]*$", inline):
        ncols = match.value
        head |= 0x1
    elif match("^nrows[ \t]+([0-9.]+)[ \t]*$", inline):
        nrows = match.value
        head |= 0x2
    elif match("^xllcorner[ \t]+([0-9.]+)[ \t]*$", inline):
        xllcorner = match.value
        head |= 0x4
    elif match("^yllcorner[ \t]+([0-9.]+)[ \t]*$", inline):
        yllcorner = match.value
        head |= 0x8
    elif match("^cellsize[ \t]+([0-9.]+)[ \t]*$", inline):
        cellsize = match.value
        head |= 0x10
    elif match("^nodata_value[ \t]+([0-9.]+)[ \t]*$", inline):
        nodata_value = match.value
        head |= 0x20
    else:
        print(f"{infname}: Invalid input file format")
        inf.close()
        sys.exit()
    if head == 0x3F:
        break

if head != 0x3F:
    print(f"{infname}: Invalid input file format")
    inf.close()
    sys.exit()

outf = open(outfname, "w")
outf.write(
    f"""\
arc.to.gridatb.py {infname} {outfname}
{ncols} {nrows} {cellsize}
"""
)
for inline in inf:
    outline = re.sub(
        f"(?=^|[ \t]*){nodata_value}(\\.0*)?(?=([ \t]*|$))", "9999.00", inline
    )
    outf.write(outline)

inf.close()
outf.close()
