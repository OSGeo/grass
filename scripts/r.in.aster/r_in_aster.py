#!/usr/bin/python
############################################################################
#
# MODULE:       r_in_aster.py
# AUTHOR(S):    Michael Barton (michael.barton@asu.edu)
#                   Based on r.in.aster bash script for GRASS 
#                   by Michael Barton and Paul Kelly
# PURPOSE:      Rectifies, georeferences, & imports Terra-ASTER imagery 
#                   using gdalwarp
# COPYRIGHT:    (C) 2008 by the GRASS Development Team
#
#       This program is free software under the GNU General Public
#       License (>=v2). Read the file COPYING that comes with GRASS
#       for details.
#
#############################################################################
#
# Requires:
#   gdalwarp

#%Module
#%  description: Georeference, rectify, and import Terra-ASTER imagery and relative DEM's using gdalwarp.
#%  keywords: raster, imagery, import
#%End
#%option
#%  key: input
#%  type: string
#%  gisprompt: old_file,file,input
#%  description: Input ASTER image to be georeferenced & rectified
#%  required: yes
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
#%option
#%  key: output
#%  type: string
#%  description: Base name for output raster map (band number will be appended to base name)
#%  gisprompt: old,cell,raster
#%  required: yes
#%end
#%flag
#%  key: o
#%  description: Overwrite existing file
#%END

import sys
import os
import subprocess
import platform

def main():

    #if ( os.getenv("GIS_FLAG_OVERWRITE") == "1" ):
    #    print "Flag -f set"
    #else:
    #    print "Flag -f not set"
         
    #check whether gdalwarp is in path and executable
    p = None
    try:
        p = subprocess.call(['gdalwarp', '--version'])
    except:
        pass
    if p == None or p != 0:
        subprocess.call(["g.message", "-e", "gdalwarp is not in the path and executable"])
        return

    #initialize variables
    dataset = ''
    srcfile = ''
    proj = ''
    band = ''
    outfile = ''
    bandlist = []
    
    #create temporary file to hold gdalwarp output before importing to GRASS
    tempfileCmd = subprocess.Popen(["g.tempfile","pid=%d" % os.getpid()],stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    tempfile = tempfileCmd.stdout.read().strip()
    tempfile=tempfile + '.tif'

    #get projection information for current GRASS location
    p = subprocess.Popen(['g.proj', '-jf'], stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)
    proj = p.stdout.read()

    #currently only runs in projected location
    if "XY location" in proj:
      subprocess.call(["g.message", "-e", "This module needs to be run in a projected location (found: %s)" % proj])
      return

    
    #process list of bands
    if os.getenv("GIS_OPT_BAND").strip() == 'all':
        bandlist = ['1','2','3n','3b','4','5','6','7','8','9','10','11','12','13','14']
    else:
        bandlist = os.getenv("GIS_OPT_BAND").split(',')
        
    print 'bandlist =',bandlist

    #initialize datasets for L1A and L1B
    #
    
    if os.getenv("GIS_OPT_PROCTYPE") == "L1A":
        for band in bandlist:
            if band == '1':
                dataset="VNIR_Band1:ImageData"
            elif band == '2':
                dataset="VNIR_Band2:ImageData"
            elif band == '3n':
                dataset="VNIR_Band3N:ImageData"
            elif band == '3b':
                dataset="VNIR_Band3B:ImageData"
            elif band == '4':
                dataset="SWIR_Band4:ImageData"
            elif band == '5':
                dataset="SWIR_Band5:ImageData"
            elif band == '6':
                dataset="SWIR_Band6:ImageData"
            elif band == '7':
                dataset="SWIR_Band7:ImageData"
            elif band == '8':
                dataset="SWIR_Band8:ImageData"
            elif band == '9':
                dataset="SWIR_Band9:ImageData"
            elif band == '10':
                dataset="TIR_Band10:ImageData"
            elif band == '11':
                dataset="TIR_Band11:ImageData"
            elif band == '12':
                dataset="TIR_Band12:ImageData"
            elif band == '13':
                dataset="TIR_Band13:ImageData"
            elif band == '14':
                dataset="TIR_Band14:ImageData"        
            srcfile = "HDF4_EOS:EOS_SWATH:%s:%s" % (os.getenv("GIS_OPT_INPUT"), dataset)
            import_aster(proj, srcfile, tempfile, band)
            
    elif os.getenv("GIS_OPT_PROCTYPE") == "L1B":
        for band in bandlist:
            if band == '1':
                dataset="VNIR_Swath:ImageData1"
            elif band == '2':
                dataset="VNIR_Swath:ImageData2"
            elif band == '3n':
                dataset="VNIR_Swath:ImageData3N"
            elif band == '3b':
                dataset="VNIR_Swath:ImageData3B"
            elif band == '4':
                dataset="SWIR_Swath:ImageData4"
            elif band == '5':
                dataset="SWIR_Swath:ImageData5"
            elif band == '6':
                dataset="SWIR_Swath:ImageData6"
            elif band == '7':
                dataset="SWIR_Swath:ImageData7"
            elif band == '8':
                dataset="SWIR_Swath:ImageData8"
            elif band == '9':
                dataset="SWIR_Swath:ImageData9"
            elif band == '10':
                dataset="TIR_Swath:ImageData10"
            elif band == '11':
                dataset="TIR_Swath:ImageData11"
            elif band == '12':
                dataset="TIR_Swath:ImageData12"
            elif band == '13':
                dataset="TIR_Swath:ImageData13"
            elif band == '14':
                dataset="TIR_Swath:ImageData14"        
            srcfile = "HDF4_EOS:EOS_SWATH:%s:%s" % (os.getenv("GIS_OPT_INPUT"), dataset)
            import_aster(proj, srcfile, tempfile, band)

    elif os.getenv("GIS_OPT_PROCTYPE") == "DEM": 
        srcfile=os.getenv("GIS_OPT_INPUT")
        import_aster(proj, srcfile, tempfile, "DEM")

    #for debugging
    #print 'source file=',srcfile
    #print 'tempfile=',tempfile
    #print 'proj =',proj
           
    # write cmd history: Not sure how to replicate this in Python yet
    #r.support "$GIS_OPT_OUTPUT" history="${CMDLINE}"

    #cleanup
    subprocess.call(["g.message", "Cleaning up ..."])
    os.remove(tempfile)
    subprocess.call(["g.message", "Done."])

    return

def import_aster(proj, srcfile, tempfile, band):
    #run gdalwarp with selected options (must be in $PATH)
    #to translate aster image to geotiff
    subprocess.call(["g.message", "Georeferencing aster image ..."])    
    subprocess.call(["g.message", "-d", "message=gdalwarp -t_srs %s %s %s" % (proj, srcfile, tempfile)])

    if platform.system() == "Darwin":
        cmd = ["arch", "-i386", "gdalwarp", "-t_srs", proj, srcfile, tempfile ]
    else:
        cmd = ["gdalwarp", "-t_srs", proj, srcfile, tempfile ]
    p = subprocess.call(cmd)
    if p != 0:
        #check to see if gdalwarp executed properly
        return
    
    #p = subprocess.call(["gdal_translate", srcfile, tempfile])

    #import geotiff to GRASS
    subprocess.call(["g.message", "Importing into GRASS ..."])
    outfile = os.getenv("GIS_OPT_OUTPUT").strip()+'.'+band
    cmd = ["r.in.gdal", "input=%s" % tempfile, "output=%s" % outfile]
    if ( os.getenv("GIS_FLAG_O") == "1" ):
        cmd.append("--o")
    #msgcmd = ['g.message']
    #msgcmd = msgcmd.extend(cmd)
    #subprocess.call(msgcmd)
    subprocess.call(cmd)
    
    return

if __name__ == "__main__":
    if ( len(sys.argv) <= 1 or sys.argv[1] != "@ARGS_PARSED@" ):
        os.execvp("g.parser", [sys.argv[0]] + sys.argv)
    else:
        main();

