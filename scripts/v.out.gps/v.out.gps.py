#!/usr/bin/env python
#
############################################################################
#
# MODULE:	v.out.gps
#
# PURPOSE:	Exports a GRASS vector map to a GPS receiver
#		or data file using GPSBabel
#
# COPYRIGHT:	(c) 2008-2009 Hamish Bowman, and the GRASS Development Team
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
# AUTHOR:	Hamish Bowman, Dunedin, New Zealand
#		Converted to Python by Glynn Clements
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
#%  description: Exports a vector map to a GPS receiver or file format supported by GPSBabel.
#%  keywords: vector
#%  keywords: export
#%  keywords: GPS
#%End
#%flag
#%  key: w
#%  description: Export as waypoints
#%end
#%flag
#%  key: r
#%  description: Export as routes
#%end
#%flag
#%  key: t
#%  description: Export as tracks
#%end
############ TODO:
##%flag
##%  key: z
##%  description: Export altitude from 3D vector's z-coordinate
##%end
############
#%option G_OPT_V_INPUT
#%end
#%option G_OPT_V_TYPE
#% options: point,centroid,line,boundary
#% answer: point,centroid,line,boundary
#%end
#%option G_OPT_F_OUTPUT
#% description: Name for output file or GPS device
#%end
#%option
#% key: format
#% type: string
#% description: GPSBabel supported output format
#% answer: gpx
#%end
#%option G_OPT_V_FIELD
#% required: no
#% guisection: Subset
#%end
#%option G_OPT_DB_WHERE
#% guisection: Subset
#%end

import sys
import os
import atexit
import string
import re

import grass.script as grass

def cleanup():
    grass.verbose("Cleaning up ...")
    if tmp:
	grass.try_remove(tmp)
    if tmp_proj:
	grass.try_remove(tmp_proj)
    if tmp_gpx:
	grass.try_remove(tmp_gpx)

    # only try to remove map if it exists to avoid ugly warnings
    if tmp_vogb:
	if grass.find_file(tmp_vogb, element = 'vector')['name']:
	    grass.run_command('g.remove', vect = tmp_vogb, quiet = True)
    if tmp_extr:
	if grass.find_file(tmp_extr, element = 'vector')['name']:
	    grass.run_command('g.remove', vect = tmp_vogb, quiet = True)

tmp = None
tmp_proj = None
tmp_gpx = None
tmp_extr = None
tmp_vogb = None

def main():
    global tmp, tmp_proj, tmp_gpx, tmp_extr, tmp_vogb

    format = options['format']
    input = options['input']
    layer = options['layer']
    output = options['output']
    type = options['type']
    where = options['where']
    wpt = flags['w']
    rte = flags['r']
    trk = flags['t']

    nflags = len(filter(None, [wpt, rte, trk]))
    if nflags > 1:
	grass.fatal(_("One feature at a time please."))
    if nflags < 1:
	grass.fatal(_("No features requested for export."))

    # set some reasonable defaults
    if not type:
	if wpt:
	    type = 'point'
	else:
	    type = 'line'

    #### check for gpsbabel
    ### FIXME: may need --help or similar?
    if not grass.find_program("gpsbabel"):
	grass.fatal(_("The gpsbabel program was not found, please install it first.\n") +
		    "http://gpsbabel.sourceforge.net")

    #### check for cs2cs
    if not grass.find_program("cs2cs"):
	grass.fatal(_("The cs2cs program was not found, please install it first.\n") +
		    "http://proj.osgeo.org")

    # check if we will overwrite data
    if os.path.exists(output) and not grass.overwrite():
	grass.fatal(_("Output file already exists."))

    #### set temporary files
    tmp = grass.tempfile()

    # SQL extract if needed
    if where:
	grass.verbose("Extracting data ...")
	tmp_extr = "tmp_vogb_extr_%d" % os.getpid()
	ret = grass.run_command('v.extract', input = "$GIS_OPT_INPUT",
				output = tmp_extr, type = type, layer = layer,
				where = where, quiet = True)
	if ret != 0:
	    grass.fatal(_("Error executing SQL query"))

	kv = grass.vector_info_topo(tmp_extr)
	if kv['primitives'] == 0:
	    grass.fatal(_("SQL query returned an empty map (no %s features?)") % type)

	inmap = tmp_extr
    else:
	#   g.copy "$GIS_OPT_INPUT,tmp_vogb_extr_$$"   # to get a copy of DB into local mapset
	#   INMAP="tmp_vogb_extr_$$"
	inmap = input

    #### set up projection info
    # TODO: check if we are already in ll/WGS84.  If so skip m.proj step.

    # TODO: multi layer will probably fail badly due to sed 's/^ 1   /'
    #   output as old GRASS 4 vector ascii and fight with dig_ascii/?
    #   Change to s/^ \([0-9]   .*\)    /# \1/' ??? mmph.

    # reproject to lat/lon WGS84
    grass.verbose("Reprojecting data ...")

    re1 = re.compile(r'^\([PLBCFKA]\)')
    re2 = re.compile(r'^ 1     ')

    re3 = re.compile(r'\t\([-\.0-9]*\) .*')
    re4 = re.compile(r'^\([-\.0-9]\)')
    re5 = re.compile(r'^#')

    tmp_proj = tmp + ".proj"
    tf = open(tmp_proj, 'w')
    p1 = grass.pipe_command('v.out.ascii', input = inmap, format = 'standard')
    p2 = grass.feed_command('m.proj', input = '-', flags = 'od', quiet = True, stdout = tf)
    tf.close()

    lineno = 0
    for line in p1.stdout:
	lineno += 1
	if lineno < 11:
	    continue
	line = re1.sub(r'#\1', line)
	line = re2.sub(r'# 1  ', line)
	p2.stdin.write(line)

    p2.stdin.close()
    p1.wait()
    p2.wait()

    if p1.returncode != 0 or p2.returncode != 0:
	grass.fatal(_("Error reprojecting data"))

    tmp_vogb = "tmp_vogb_epsg4326_%d" % os.getpid()
    p3 = grass.feed_command('v.in.ascii', out = tmp_vogb, format = 'standard', flags = 'n', quiet = True)
    tf = open(tmp_proj, 'r')

    for line in tf:
	line = re3.sub(r' \1', line)
	line = re4.sub(r' \1', line)
	line = re5.sub('', line)
	p3.stdin.write(line)

    p3.stdin.close()
    tf.close()
    p3.wait()

    if p3.returncode != 0:
	grass.fatal(_("Error reprojecting data"))

    # don't v.db.connect directly as source table will be removed with
    # temporary map in that case. So we make a temp copy of it to work with.
    kv = vector_db(inmap)
    if layer in kv:
	db_params = kv[layer]

	db_table = db_params['table']
	db_key = db_params['key']
	db_database = db_params['database']
	db_driver = db_params['driver']

	ret = grass.run_command('db.copy',
				from_driver = db_driver,
				from_database = db_database,
				from_table = db_table,
				to_table = tmp_vogb)
	if ret != 0:
	    grass.fatal(_("Error copying temporary DB"))

	ret = grass.run_command('v.db.connect', map = tmp_vogb, table = tmp_vogb, quiet = True)
	if ret != 0:
	    grass.fatal(_("Error reconnecting temporary DB"))

    # export as GPX using v.out.ogr
    if trk:
	linetype = "FORCE_GPX_TRACK=YES"
    elif rte:
	linetype = "FORCE_GPX_TRACK=YES"
    else:
	linetype = None

    # BUG: cat is being reported as evelation and attribute output is skipped.
    #   (v.out.ogr DB reading or ->OGR GPX driver bug<-
    #     resolved: see new Create opts at http://www.gdal.org/ogr/drv_gpx.html)
    #   v.out.ogr -> shapefile -> GPX works, but we try to avoid that as it's
    #     lossy. Also that would allow ogr2ogr -a_srs $IN_PROJ -t_srs EPSG:4326
    #     so skip m.proj pains.. if that is done ogr2ogr -s_srs MUST HAVE +wktext
    #     with PROJ.4 terms or else the +nadgrids will be ignored! best to feed
    #     it  IN_PROJ="`g.proj -jf` +wktext"  in that case.

    grass.verbose("Exporting data ...")

    tmp_gpx = tmp + ".gpx"
    ret = grass.run_command('v.out.ogr', input = tmp_vogb, dsn = tmp_gpx,
			    type = type, format = 'GPX', lco = linetype,
			    dsco = "GPX_USE_EXTENSIONS=YES", quiet = True)
    if ret != 0:
	grass.fatal(_("Error exporting data"))

    if format == 'gpx':
	# short circuit, we have what we came for.
	grass.try_remove(output)
	os.rename(tmp_gpx, output)
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
		      '-i', 'gpx',
		      '-f', tmp + '.gpx',
		      '-o', format,
		      '-F', output])

    if ret != 0:
	grass.fatal(_("Error running GPSBabel"))

    grass.verbose("Done.")

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
