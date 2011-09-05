#!/usr/bin/env python
#
############################################################################
#
# MODULE:	v.in.gps
#
# PURPOSE:	Import GPS data from a GPS receiver or file into a GRASS
#	       vector map using gpsbabel
#
# COPYRIGHT:	(c) 2011 Hamish Bowman, and the GRASS Development Team
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
# AUTHOR:	Hamish Bowman, Dunedin, New Zealand
#		Python version based on v.out.gps.py by Glynn Clements
#		Work-alike of the v.in.gpsbabel shell script from GRASS 6
#
#############################################################################
#
# REQUIREMENTS:
#      -  GPSBabel from 	http://gpsbabel.sourceforge.net
#      -  cs2cs from PROJ.4 (for m.proj)	http://proj.osgeo.org
#
#      - report supported GPSBabel formats:
#	 gpsbabel -^2 | tr '\t' ';' | sort -t';' -k3
#
#############################################################################
#
# How to do it
#   http://www.gdal.org/ogr/drv_gpx.html
#   gpsbabel [options] -i INTYPE -f INFILE -o OUTTYPE -F OUTFILE
#
#############################################################################

#%Module
#%  description: Import waypoints, routes, and tracks from a GPS receiver or GPS download file into a vector map.
#%  keywords: vector
#%  keywords: import
#%  keywords: GPS
#%End
#%flag
#%  key: w
#%  description: Import as waypoints
#%end
#%flag
#%  key: r
#%  description: Import as routes
#%end
#%flag
#%  key: t
#%  description: Import as tracks
#%end
#%flag
#%  key: p
#%  description: Force vertices of track or route data as points
#%end
#%flag
#%  key: k
#%  description: Do not attempt projection transform from WGS84
#%end
#%option G_OPT_F_INPUT
#% description: Device or file used to import data
#%end
#%option G_OPT_V_OUTPUT
#%end
#%option
#% key: format
#% type: string
#% description: GPSBabel supported output format
#% answer: gpx
#%end
#%option
#% key: proj
#% type: string
#% description: Projection of input data (PROJ.4 style), if not set Lat/Lon WGS84 is assumed
#% required: no
#%end



import sys
import os
import atexit
import string
import re

import grass.script as grass

#.... todo ....

def main():
    format = options['format']
    input = options['input']
    output = options['output']
    proj_terms = options['proj']
    wpt = flags['w']
    rte = flags['r']
    trk = flags['t']
    points_mode = flags['p']
    no_reproj = flags['k']

    nflags = len(filter(None, [wpt, rte, trk]))
    if nflags > 1:
	grass.fatal(_("One feature at a time please."))
    if nflags < 1:
	grass.fatal(_("No features requested for import."))


    #### check for gpsbabel
    ### FIXME: may need --help or similar?
    if not grass.find_program("gpsbabel"):
	grass.fatal(_("The gpsbabel program was not found, please install it first.\n") +
		    "http://gpsbabel.sourceforge.net")

    #### check for cs2cs
    if not grass.find_program("cs2cs"):
	grass.fatal(_("The cs2cs program was not found, please install it first.\n") +
		    "http://proj.osgeo.org")

#todo
#    # check if we will overwrite data
#    if grass.findfile(output) and not grass.overwrite():
#	grass.fatal(_("Output file already exists."))

    #### set temporary files
    tmp = grass.tempfile()

    # import as GPX using v.in.ogr
#     if trk:
# 	linetype = "FORCE_GPX_TRACK=YES"
#     elif rte:
# 	linetype = "FORCE_GPX_TRACK=YES"
#     else:
# 	linetype = None

    if format == 'gpx':
	# short circuit, we have what we came for.
#todo
#	grass.try_remove(output)
#	os.rename(tmp_gpx, output)
	grass.verbose("Fast exit.")
	sys.exit()

    # run gpsbabel
    if wpt:
	gtype = '-w'
    elif trk:
	gtype = '-t'
    elif rte:
	gtype = '-r'
    else:
	gtype = ''

    grass.verbose("Running GPSBabel ...")

    ret = grass.call(['gpsbabel',
		      gtype,
		      '-i', format,
		      '-f', output,
		      '-o', 'gpx',
		      '-F', tmp + '.gpx')

    if ret != 0:
	grass.fatal(_("Error running GPSBabel"))


    grass.verbose("Importing data ...")

    tmp_gpx = tmp + ".gpx"
    ret = grass.run_command('v.in.ogr', dsn = tmp_gpx, output = output,
			    type = type, format = 'GPX', lco = linetype,
			    dsco = "GPX_USE_EXTENSIONS=YES", quiet = True)
    if ret != 0:
	grass.fatal(_("Error importing data"))


    #### set up projection info
    # TODO: check if we are already in ll/WGS84.  If so skip m.proj step.

    # TODO: multi layer will probably fail badly due to sed 's/^ 1   /'
    #   output as old GRASS 4 vector ascii and fight with dig_ascii/?
    #   Change to s/^ \([0-9]   .*\)    /# \1/' ??? mmph.

#todo (taken from Glynn's v.out.gps)
    # reproject to lat/lon WGS84
#     grass.verbose("Reprojecting data ...")
# 
#     re1 = re.compile(r'^\([PLBCFKA]\)')
#     re2 = re.compile(r'^ 1     ')
# 
#     re3 = re.compile(r'\t\([-\.0-9]*\) .*')
#     re4 = re.compile(r'^\([-\.0-9]\)')
#     re5 = re.compile(r'^#')
# 
#     tmp_proj = tmp + ".proj"
#     tf = open(tmp_proj, 'w')
#     p1 = grass.pipe_command('v.out.ascii', input = inmap, format = 'standard')
#     p2 = grass.feed_command('m.proj', input = '-', flags = 'od', quiet = True, stdout = tf)
#     tf.close()
# 
#     lineno = 0
#     for line in p1.stdout:
# 	lineno += 1
# 	if lineno < 11:
# 	    continue
# 	line = re1.sub(r'#\1', line)
# 	line = re2.sub(r'# 1  ', line)
# 	p2.stdin.write(line)
# 
#     p2.stdin.close()
#     p1.wait()
#     p2.wait()
# 
#     if p1.returncode != 0 or p2.returncode != 0:
# 	grass.fatal(_("Error reprojecting data"))
# 
#     tmp_vogb = "tmp_vogb_epsg4326_%d" % os.getpid()
#     p3 = grass.feed_command('v.in.ascii', out = tmp_vogb, format = 'standard', flags = 'n', quiet = True)
#     tf = open(tmp_proj, 'r')
# 
#     for line in tf:
# 	line = re3.sub(r' \1', line)
# 	line = re4.sub(r' \1', line)
# 	line = re5.sub('', line)
# 	p3.stdin.write(line)
# 
#     p3.stdin.close()
#     tf.close()
#     p3.wait()
# 
#     if p3.returncode != 0:
# 	grass.fatal(_("Error reprojecting data"))



    grass.verbose("Done.")

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
