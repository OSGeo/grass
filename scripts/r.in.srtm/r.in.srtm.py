#!/usr/bin/env python3
#
############################################################################
#
# MODULE:    r_in_aster.py
# AUTHOR(S): Markus Neteler 11/2003 neteler AT itc it
#            Hamish Bowman
#            Glynn Clements
#            Luca Delucchi
# PURPOSE:   import of SRTM hgt files into GRASS
#
# COPYRIGHT:	(C) 2004, 2006 by the GRASS Development Team
#
# 		This program is free software under the GNU General Public
# 		License (>=v2). Read the file COPYING that comes with GRASS
# 		for details.
#
# Dec 2004: merged with srtm_generate_hdr.sh (M. Neteler)
#           corrections and refinement (W. Kyngesburye)
# Aug 2004: modified to accept files from other directories
#           (by H. Bowman)
# June 2005: added flag to read in US 1-arcsec tiles (H. Bowman)
# April 2006: links updated from ftp://e0dps01u.ecs.nasa.gov/srtm/
#             to current links below
# October 2008: Converted to Python by Glynn Clements
# March 2018: Added capabilities to import SRTM SWBD
#             Removed unzip dependencies, now it use Python zipfile library
#             by Luca Delucchi
#########################
# Derived from:
# ftp://e0srp01u.ecs.nasa.gov/srtm/version1/Documentation/Notes_for_ARCInfo_users.txt
#     (note: document was updated silently end of 2003)
#
# ftp://e0srp01u.ecs.nasa.gov/srtm/version1/Documentation/SRTM_Topo.txt
#  "3.0 Data Formats
#  [...]
#  To be more exact, these coordinates refer to the geometric center of
#  the lower left pixel, which in the case of SRTM-1 data will be about
#  30 meters in extent."
#
# - SRTM 90 Tiles are 1 degree by 1 degree
# - SRTM filename coordinates are said to be the *center* of the LL pixel.
#       N51E10 -> lower left cell center
#
# - BIL uses *center* of the UL (!) pixel:
#      http://downloads.esri.com/support/whitepapers/other_/eximgav.pdf
#
# - GDAL uses *corners* of pixels for its coordinates.
#
# NOTE: Even, if small difference: SRTM is referenced to EGM96, not WGS84 ellps
# http://earth-info.nga.mil/GandG/wgs84/gravitymod/egm96/intpt.html
#
#########################

# %Module
# % description: Imports SRTM HGT files into raster map.
# % keyword: raster
# % keyword: import
# % keyword: SRTM
# %End
# %option G_OPT_F_INPUT
# % description: Name of SRTM input tile (file without .hgt.zip extension)
# %end
# %option G_OPT_R_OUTPUT
# % description: Name for output raster map (default: input tile)
# % required : no
# %end
# %flag
# % key: 1
# % description: Input is a 1-arcsec tile (default: 3-arcsec)
# %end


import os
import shutil
import atexit
import grass.script as gs
import zipfile as zfile
from pathlib import Path
from grass.exceptions import CalledModuleError


tmpl1sec = """BYTEORDER M
LAYOUT BIL
NROWS 3601
NCOLS 3601
NBANDS 1
NBITS 16
BANDROWBYTES 7202
TOTALROWBYTES 7202
BANDGAPBYTES 0
PIXELTYPE SIGNEDINT
NODATA -32768
ULXMAP %s
ULYMAP %s
XDIM 0.000277777777777778
YDIM 0.000277777777777778
"""

swbd1sec = """BYTEORDER M
LAYOUT BIL
NROWS 3601
NCOLS 3601
NBANDS 1
NBITS 8
BANDROWBYTES 7202
TOTALROWBYTES 7202
BANDGAPBYTES 0
PIXELTYPE SIGNEDINT
NODATA -32768
ULXMAP %s
ULYMAP %s
XDIM 0.000277777777777778
YDIM 0.000277777777777778
"""

tmpl3sec = """BYTEORDER M
LAYOUT BIL
NROWS 1201
NCOLS 1201
NBANDS 1
NBITS 16
BANDROWBYTES 2402
TOTALROWBYTES 2402
BANDGAPBYTES 0
PIXELTYPE SIGNEDINT
NODATA -32768
ULXMAP %s
ULYMAP %s
XDIM 0.000833333333333
YDIM 0.000833333333333
"""

proj = 'GEOGCS["wgs84",DATUM["WGS_1984",SPHEROID["wgs84",6378137,298.257223563],TOWGS84[0.000000,0.000000,0.000000]],PRIMEM["Greenwich",0],UNIT["degree",0.0174532925199433]]'


def cleanup():
    if not in_temp:
        return
    for ext in [".bil", ".hdr", ".prj", ".hgt.zip"]:
        gs.try_remove(tile + ext)
    os.chdir("..")
    gs.try_rmdir(tmpdir)


def main():
    global tile, tmpdir, in_temp

    in_temp = False

    # to support SRTM water body
    swbd = False

    input = options["input"]
    output = options["output"]
    one = flags["1"]

    # are we in LatLong location?
    s = gs.read_command("g.proj", flags="j")
    kv = gs.parse_key_val(s)
    if "+proj" not in kv.keys() or kv["+proj"] != "longlat":
        gs.fatal(_("This module only operates in LatLong locations"))

    # use these from now on:
    infile = input
    while infile[-4:].lower() in {".hgt", ".zip", ".raw"}:
        infile = infile[:-4]
    (fdir, tile) = os.path.split(infile)

    tileout = output or tile

    if ".hgt" in input:
        suff = ".hgt"
    else:
        suff = ".raw"
        swbd = True

    zipfile = f"{infile}{suff}.zip"
    hgtfile = f"{infile}{suff}"

    if os.path.isfile(zipfile):
        # really a ZIP file?
        if not zfile.is_zipfile(zipfile):
            gs.fatal(_("'%s' does not appear to be a valid zip file.") % zipfile)

        is_zip = True
    elif os.path.isfile(hgtfile):
        # try and see if it's already unzipped
        is_zip = False
    else:
        gs.fatal(_("File '%s' or '%s' not found") % (zipfile, hgtfile))

    # make a temporary directory
    tmpdir = gs.tempfile()
    gs.try_remove(tmpdir)
    os.mkdir(tmpdir)
    if is_zip:
        shutil.copyfile(zipfile, os.path.join(tmpdir, f"{tile}{suff}.zip"))
    else:
        shutil.copyfile(hgtfile, os.path.join(tmpdir, f"{tile[:7]}{suff}"))
    # change to temporary directory
    os.chdir(tmpdir)
    in_temp = True

    zipfile = f"{tile}{suff}.zip"
    hgtfile = f"{tile[:7]}{suff}"

    bilfile = tile + ".bil"

    if is_zip:
        # unzip & rename data file:
        gs.message(_("Extracting '%s'...") % infile)
        try:
            with zfile.ZipFile(zipfile) as zf:
                zf.extractall()
        except (zfile.BadZipfile, zfile.LargeZipFile, PermissionError):
            gs.fatal(_("Unable to unzip file."))

    gs.message(_("Converting input file to BIL..."))
    os.rename(hgtfile, bilfile)

    north = tile[0]
    ll_latitude = int(tile[1:3])
    east = tile[3]
    ll_longitude = int(tile[4:7])

    # are we on the southern hemisphere? If yes, make LATITUDE negative.
    if north == "S":
        ll_latitude *= -1

    # are we west of Greenwich? If yes, make LONGITUDE negative.
    if east == "W":
        ll_longitude *= -1

    # Calculate Upper Left from Lower Left
    ulxmap = "%.1f" % ll_longitude
    # SRTM90 tile size is 1 deg:
    ulymap = "%.1f" % (ll_latitude + 1)

    if not one:
        tmpl = tmpl3sec
    elif swbd:
        gs.message(_("Attempting to import 1-arcsec SWBD data"))
        tmpl = swbd1sec
    else:
        gs.message(_("Attempting to import 1-arcsec data"))
        tmpl = tmpl1sec

    header = tmpl % (ulxmap, ulymap)
    hdrfile = tile + ".hdr"
    Path(hdrfile).write_text(header)

    # create prj file: To be precise, we would need EGS96! But who really cares...
    prjfile = tile + ".prj"
    Path(prjfile).write_text(proj)

    try:
        gs.run_command("r.in.gdal", input=bilfile, out=tileout)
    except CalledModuleError:
        gs.fatal(_("Unable to import data"))

    # nice color table
    if not swbd:
        gs.run_command("r.colors", map=tileout, color="srtm")

    # write cmd history:
    gs.raster_history(tileout)

    gs.message(_("Done: generated map ") + tileout)
    gs.message(
        _("(Note: Holes in the data can be closed with 'r.fillnulls' using splines)")
    )


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    main()
