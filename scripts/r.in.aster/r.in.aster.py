#!/usr/bin/env python
############################################################################
#
# MODULE:    r_in_aster.py
# AUTHOR(S): Michael Barton (michael.barton@asu.edu) and
#               Glynn Clements (glynn@gclements.plus.com)
#               Based on r.in.aster bash script for GRASS 
#               by Michael Barton and Paul Kelly
# PURPOSE:   Rectifies, georeferences, & imports Terra-ASTER imagery 
#               using gdalwarp
# COPYRIGHT: (C) 2008 by the GRASS Development Team
#
#   This program is free software under the GNU General Public
#   License (>=v2). Read the file COPYING that comes with GRASS
#   for details.
#
#############################################################################
#
# Requires:
#   gdalwarp
#   gdal compiled with HDF4 support

#%Module
#%  description: Georeference, rectify, and import Terra-ASTER imagery and relative DEMs using gdalwarp.
#%  keywords: raster
#%  keywords: import
#%  keywords: imagery
#%  keywords: Terra-ASTER
#%End
#%option G_OPT_F_INPUT
#%  description: Name of input ASTER image
#%end
#%option
#%  key: proctype
#%  type: string
#%  description: ASTER imagery processing type (Level 1A, Level 1B, or relative DEM)
#%  options: L1A,L1B,DEM
#%  answer: L1B
#%  required: yes
#%end
#%option
#%  key: band
#%  type: string
#%  description: List L1A or L1B band to translate (1,2,3n,...), or enter 'all' to translate all bands
#%  answer: all
#%  required: yes
#%end
#%option G_OPT_R_OUTPUT
#%  description: Base name for output raster map (band number will be appended to base name)
#%end

import sys
import os
import platform
import grass.script as grass

bands = {
    'L1A': {
        '1':  "VNIR_Band1:ImageData",
        '2':  "VNIR_Band2:ImageData",
        '3n': "VNIR_Band3N:ImageData",
        '3b': "VNIR_Band3B:ImageData",
        '4':  "SWIR_Band4:ImageData",
        '5':  "SWIR_Band5:ImageData",
        '6':  "SWIR_Band6:ImageData",
        '7':  "SWIR_Band7:ImageData",
        '8':  "SWIR_Band8:ImageData",
        '9':  "SWIR_Band9:ImageData",
        '10': "TIR_Band10:ImageData",
        '11': "TIR_Band11:ImageData",
        '12': "TIR_Band12:ImageData",
        '13': "TIR_Band13:ImageData",
        '14': "TIR_Band14:ImageData"
    },
    'L1B': {
        '1':  "VNIR_Swath:ImageData1",
        '2':  "VNIR_Swath:ImageData2",
        '3n': "VNIR_Swath:ImageData3N",
        '3b': "VNIR_Swath:ImageData3B",
        '4':  "SWIR_Swath:ImageData4",
        '5':  "SWIR_Swath:ImageData5",
        '6':  "SWIR_Swath:ImageData6",
        '7':  "SWIR_Swath:ImageData7",
        '8':  "SWIR_Swath:ImageData8",
        '9':  "SWIR_Swath:ImageData9",
        '10': "TIR_Swath:ImageData10",
        '11': "TIR_Swath:ImageData11",
        '12': "TIR_Swath:ImageData12",
        '13': "TIR_Swath:ImageData13",
        '14': "TIR_Swath:ImageData14"
    }
}

def main():
    input = options['input']
    proctype = options['proctype']
    output = options['output']
    band = options['band']

    #check whether gdalwarp is in path and executable
    if not grass.find_program('gdalwarp'):
        grass.fatal(_("gdalwarp is not in the path and executable"))

    #create temporary file to hold gdalwarp output before importing to GRASS
    tempfile = grass.read_command("g.tempfile", pid = os.getpid()).strip() + '.tif'

    #get projection information for current GRASS location
    proj = grass.read_command('g.proj', flags = 'jf').strip()

    #currently only runs in projected location
    if "XY location" in proj:
      grass.fatal(_("This module needs to be run in a projected location (found: %s)") % proj)


    #process list of bands
    allbands = ['1','2','3n','3b','4','5','6','7','8','9','10','11','12','13','14']
    if band == 'all':
        bandlist = allbands
    else:
        bandlist = band.split(',')

    #initialize datasets for L1A and L1B
    if proctype in ["L1A", "L1B"]:
        for band in bandlist:
            if band in allbands:
                dataset = bands[proctype][band]
                srcfile = "HDF4_EOS:EOS_SWATH:%s:%s" % (input, dataset)
                import_aster(proj, srcfile, tempfile, band)
            else:
                grass.fatal(_('band %s is not an available Terra/ASTER band') % band)
    elif proctype == "DEM": 
        srcfile = input
        import_aster(proj, srcfile, tempfile, "DEM")

    #cleanup
    grass.message(_("Cleaning up ..."))
    grass.try_remove(tempfile)
    grass.message(_("Done."))

    return

def import_aster(proj, srcfile, tempfile, band):
    #run gdalwarp with selected options (must be in $PATH)
    #to translate aster image to geotiff
    grass.message(_("Georeferencing aster image ..."))
    grass.debug("gdalwarp -t_srs %s %s %s" % (proj, srcfile, tempfile))

    if platform.system() == "Darwin":
        cmd = ["arch", "-i386", "gdalwarp", "-t_srs", proj, srcfile, tempfile ]
    else:
        cmd = ["gdalwarp", "-t_srs", proj, srcfile, tempfile ]
    p = grass.call(cmd)
    if p != 0:
        #check to see if gdalwarp executed properly
        return

    #import geotiff to GRASS
    grass.message(_("Importing into GRASS ..."))
    outfile = "%s.%s" % (output, band)
    grass.run_command("r.in.gdal", input = tempfile, output = outfile)

    # write cmd history
    grass.raster_history(outfile)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
