#!/usr/bin/env python3

############################################################################
#
# MODULE:       i.in.spot
#
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
#
# PURPOSE:      Import SPOT VEGETATION data into a GRASS raster map
#               SPOT Vegetation (1km, global) data:
#               http://free.vgt.vito.be/
#
# COPYRIGHT:    (c) 2004-2011 GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################
#
# REQUIREMENTS:
#      -  gdal: https://gdal.org
#
# Notes:
# * According to the faq (http://www.vgt.vito.be/faq/faq.html), SPOT vegetation
#   coordinates refer to the center of a pixel.
# * GDAL coordinates refer to the corner of a pixel
#   -> correction of 0001/0001_LOG.TXT coordinates by 0.5 pixel
#############################################################################

# %module
# % description: Imports SPOT VGT NDVI data into a raster map.
# % keyword: imagery
# % keyword: import
# % keyword: NDVI
# % keyword: SPOT
# %end
# %flag
# % key: a
# % description: Also import quality map (SM status map layer) and filter NDVI map
# %end
# %option G_OPT_F_INPUT
# % description: Name of input SPOT VGT NDVI HDF file
# %end
# % option G_OPT_R_OUTPUT
# % required : no
# %end

import os
import atexit
import string
import grass.script as gs
from grass.exceptions import CalledModuleError
from pathlib import Path


vrt = """<VRTDataset rasterXSize="$XSIZE" rasterYSize="$YSIZE">
 <SRS>GEOGCS[&quot;wgs84&quot;,DATUM[&quot;WGS_1984&quot;,SPHEROID[&quot;wgs84&quot;,6378137,298.257223563],TOWGS84[0.000,0.000,0.000]],PRIMEM[&quot;Greenwich&quot;,0],UNIT[&quot;degree&quot;,0.0174532925199433]]</SRS>
   <GeoTransform>$WESTCORNER, $RESOLUTION, 0.0000000000000000e+00, $NORTHCORNER, 0.0000000000000e+00, -$RESOLUTION</GeoTransform>
   <VRTRasterBand dataType="Byte" band="1">
    <NoDataValue>0.00000000000000E+00</NoDataValue>
    <ColorInterp>Gray</ColorInterp>
    <SimpleSource>
     <SourceFilename>$FILENAME</SourceFilename>
      <SourceBand>1</SourceBand>
      <SrcRect xOff="0" yOff="0" xSize="$XSIZE" ySize="$YSIZE"/>
      <DstRect xOff="0" yOff="0" xSize="$XSIZE" ySize="$YSIZE"/>
    </SimpleSource>
 </VRTRasterBand>
</VRTDataset>"""  # noqa: E501

# a function for writing VRT files


def create_VRT_file(projfile, vrtfile, infile):
    kv = {}
    with open(projfile) as fh:
        for line in fh:
            f = line.rstrip("\r\n").split()
            if f < 2:
                continue
            kv[f[0]] = f[1]

    north_center = kv["CARTO_UPPER_LEFT_Y"]
    # south_center = kv['CARTO_LOWER_LEFT_Y']
    # east_center = kv['CARTO_UPPER_RIGHT_X']
    west_center = kv["CARTO_UPPER_LEFT_X"]
    map_proj_res = kv["MAP_PROJ_RESOLUTION"]
    xsize = kv["IMAGE_UPPER_RIGHT_COL"]
    ysize = kv["IMAGE_LOWER_RIGHT_ROW"]

    resolution = float(map_proj_res)
    north_corner = float(north_center) + resolution / 2
    # south_corner = float(south_center) - resolution / 2
    # east_corner = float(east_center) + resolution / 2
    west_corner = float(west_center) - resolution / 2

    t = string.Template(vrt)
    s = t.substitute(
        NORTHCORNER=north_corner,
        WESTCORNER=west_corner,
        XSIZE=xsize,
        YSIZE=ysize,
        RESOLUTION=map_proj_res,
        FILENAME=infile,
    )
    Path(vrtfile).write_text(s)


def cleanup():
    # clean up the mess
    gs.try_remove(vrtfile)
    gs.try_remove(tmpfile)


def main():
    global vrtfile, tmpfile

    infile = options["input"]
    rast = options["output"]
    also = flags["a"]

    # check for gdalinfo (just to check if installation is complete)
    if not gs.find_program("gdalinfo", "--help"):
        gs.fatal(_("'gdalinfo' not found, install GDAL tools first (https://gdal.org)"))

    pid = str(os.getpid())
    tmpfile = gs.tempfile()

    # let's go

    spotdir = os.path.dirname(infile)
    spotname = gs.basename(infile, "hdf")
    name = rast or spotname

    if not gs.overwrite() and gs.find_file(name)["file"]:
        gs.fatal(_("<%s> already exists. Aborting.") % name)

    # still a ZIP file?  (is this portable?? see the r.in.srtm script for
    # ideas)
    if infile.lower().endswith(".zip"):
        gs.fatal(_("Please extract %s before import.") % infile)

    try:
        p = gs.Popen(["file", "-ib", infile], stdout=gs.PIPE)
        s = p.communicate()[0]
        if s == "application/x-zip":
            gs.fatal(_("Please extract %s before import.") % infile)
    except Exception:
        pass

    # create VRT header for NDVI

    projfile = os.path.join(spotdir, "0001_LOG.TXT")
    vrtfile = tmpfile + ".vrt"

    # first process the NDVI:
    gs.try_remove(vrtfile)
    create_VRT_file(projfile, vrtfile, infile)

    # let's import the NDVI map...
    gs.message(_("Importing SPOT VGT NDVI map..."))
    try:
        gs.run_command("r.in.gdal", input=vrtfile, output=name)
    except CalledModuleError:
        gs.fatal(_("An error occurred. Stop."))

    gs.message(_("Imported SPOT VEGETATION NDVI map <%s>.") % name)

    #################
    # http://www.vgt.vito.be/faq/FAQS/faq19.html
    # What is the relation between the digital number and the real NDVI ?
    # Real NDVI =coefficient a * Digital Number + coefficient b
    #           = a * DN +b
    #
    # Coefficient a = 0.004
    # Coefficient b = -0.1

    # clone current region
    # switch to a temporary region
    gs.use_temp_region()

    gs.run_command("g.region", raster=name, quiet=True)

    gs.message(_("Remapping digital numbers to NDVI..."))
    tmpname = "%s_%s" % (name, pid)
    gs.mapcalc("$tmpname = 0.004 * $name - 0.1", tmpname=tmpname, name=name)
    gs.run_command("g.remove", type="raster", name=name, quiet=True, flags="f")
    gs.run_command("g.rename", raster=(tmpname, name), quiet=True)

    # write cmd history:
    gs.raster_history(name)

    # apply color table:
    gs.run_command("r.colors", map=name, color="ndvi", quiet=True)

    ##########################
    # second, optionally process the SM quality map:

    # SM Status Map
    # http://nieuw.vgt.vito.be/faq/FAQS/faq22.html
    # Data about
    # Bit NR 7: Radiometric quality for B0 coded as 0 if bad and 1 if good
    # Bit NR 6: Radiometric quality for B2 coded as 0 if bad and 1 if good
    # Bit NR 5: Radiometric quality for B3 coded as 0 if bad and 1 if good
    # Bit NR 4: Radiometric quality for MIR coded as 0 if bad and 1 if good
    # Bit NR 3: land code 1 or water code 0
    # Bit NR 2: ice/snow code 1 , code 0 if there is no ice/snow
    # Bit NR 1:	0	0	1		1
    # Bit NR 0:	0	1	0		1
    # 		clear	shadow	uncertain	cloud
    #
    # Note:
    # pos 7     6    5    4    3    2   1   0 (bit position)
    #   128    64   32   16    8    4   2   1 (values for 8 bit)
    #
    #
    # Bit 4-7 should be 1: their sum is 240
    # Bit 3   land code, should be 1, sum up to 248 along with higher bits
    # Bit 2   ice/snow code
    # Bit 0-1 should be 0
    #
    # A good map threshold: >= 248

    if also:
        gs.message(_("Importing SPOT VGT NDVI quality map..."))
        gs.try_remove(vrtfile)
        qname = spotname.replace("NDV", "SM")
        qfile = os.path.join(spotdir, qname)
        create_VRT_file(projfile, vrtfile, qfile)

        # let's import the SM quality map...
        smfile = name + ".sm"
        try:
            gs.run_command("r.in.gdal", input=vrtfile, output=smfile)
        except CalledModuleError:
            gs.fatal(_("An error occurred. Stop."))

        # some of the possible values:
        rules = [
            r + "\n"
            for r in [
                "8 50 50 50",
                "11 70 70 70",
                "12 90 90 90",
                "60 grey",
                "155 blue",
                "232 violet",
                "235 red",
                "236 brown",
                "248 orange",
                "251 yellow",
                "252 green",
            ]
        ]
        gs.write_command("r.colors", map=smfile, rules="-", stdin=rules)

        gs.message(_("Imported SPOT VEGETATION SM quality map <%s>.") % smfile)
        gs.message(
            _("Note: A snow map can be extracted by category 252 (d.rast %s cat=252)")
            % smfile
        )
        gs.message(_("Filtering NDVI map by Status Map quality layer..."))

        filtfile = "%s_filt" % name
        gs.mapcalc(
            "$filtfile = if($smfile % 4 == 3 || "
            "($smfile / 16) % 16 == 0, null(), $name)",
            filtfile=filtfile,
            smfile=smfile,
            name=name,
        )
        gs.run_command("r.colors", map=filtfile, color="ndvi", quiet=True)
        gs.message(_("Filtered SPOT VEGETATION NDVI map <%s>.") % filtfile)

        # write cmd history:
        gs.raster_history(smfile)
        gs.raster_history(filtfile)

    gs.message(_("Done."))


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    main()
