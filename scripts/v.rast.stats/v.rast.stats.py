#!/usr/bin/env python

############################################################################
#
# MODULE:	v.rast.stats
# AUTHOR(S):	Markus Neteler, converted to Python by Glynn Clements
# PURPOSE:	Calculates univariate statistics from a GRASS raster map
#		only for areas covered by vector objects on a per-category base
# COPYRIGHT:	(C) 2005-2010 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#%  description: Calculates univariate statistics from a GRASS raster map based on vector polygons and uploads statistics to new attribute columns.
#%  keywords: vector
#%  keywords: raster
#%  keywords: statistics
#%End
#%flag
#%  key: c
#%  description: Continue if upload column(s) already exist
#%END
#%flag
#%  key: e
#%  description: Calculate extended statistics
#%END
#%option
#% key: vector
#% type: string
#% key_desc: name
#% gisprompt: old,vector,vector
#% label: Name of input vector polygon map
#% description: Data source for OGR access
#% required : yes
#%End
#%option
#% key: layer
#% type: integer
#% label: Layer number or name
#% description: A single vector map can be connected to multiple database tables. This number determines which table to use. Layer name for OGR access.
#% gisprompt: old_layer,layer,layer
#% answer: 1
#% required : no
#%end
#%option
#% key: raster
#% type: string
#% key_desc: name
#% gisprompt: old,cell,raster
#% description: Name of input raster map to calculate statistics from
#% required : yes
#%END
#%option
#% key: colprefix
#% type: string
#% description: Column prefix for new attribute columns
#% required : yes
#%end
#%option
#% key: percentile
#% type: integer
#% description: Percentile to calculate (requires extended statistics flag)
#% options: 0-100
#% answer: 90
#% required : no
#%end

import sys
import os
import atexit
import grass.script as grass

def has_column(vector, col):
    return 

def cleanup():
    grass.run_command('g.remove', rast = '%s_%s' % (vector, tmpname), quiet = True)
    grass.run_command('g.remove', rast = 'MASK', quiet = True, stderr = nuldev)
    if mask_found:
	grass.message(_("Restoring previous MASK..."))
	grass.run_command('g.rename', rast = (tmpname + "_origmask", 'MASK'), quiet = True)
#    for f in [tmp, tmpname, sqltmp]:
#	grass.try_remove(f)

def main():
    global tmp, sqltmp, tmpname, nuldev, vector, mask_found
    mask_found = False
    #### setup temporary files
    tmp = grass.tempfile()
    sqltmp = tmp + ".sql"
    # we need a random name
    tmpname = grass.basename(tmp)

    nuldev = file(os.devnull, 'w')

    raster = options['raster']
    colprefix = options['colprefix']
    vector = options['vector']
    layer = options['layer']
    percentile = options['percentile']

    ### setup enviro vars ###
    env = grass.gisenv()
    mapset = env['MAPSET']

    vs = vector.split('@')
    if len(vs) > 1:
	vect_mapset = vs[1]
    else:
	vect_mapset = mapset

    # does map exist in CURRENT mapset?
    if vect_mapset != mapset or not grass.find_file(vector, 'vector', mapset)['file']:
	grass.fatal(_("Vector map <%s> not found in current mapset") % vector)

    vector = vs[0]

    #check the input raster map
    if not grass.find_file(raster, 'cell')['file']:
	grass.fatal(_("Raster map <%s> not found") % raster)

    #check presence of raster MASK, put it aside
    mask_found = bool(grass.find_file('MASK', 'cell')['file'])
    if mask_found:
	grass.message(_("Raster MASK found, temporarily disabled"))
	grass.run_command('g.rename', rast = ('MASK', tmpname + "_origmask"), quiet = True)

    #get RASTER resolution of map which we want to query:
    #fetch separated to permit for non-square cells (latlong etc)
    kv = grass.raster_info(map = raster)
    nsres = kv['nsres']
    ewres = kv['ewres']

    #save current settings:
    grass.use_temp_region()

    #Temporarily setting raster resolution to $RASTER resolution
    #keep boundary settings
    grass.run_command('g.region', flags = 'a', nsres = nsres, ewres = ewres)

    #prepare raster MASK
    if grass.run_command('v.to.rast', input = vector, output = "%s_%s" % (vector, tmpname),
			 use = 'cat', quiet = True) != 0:
	grass.fatal(_("An error occurred while converting vector to raster"))

    #dump cats to file to avoid "too many argument" problem:
    p = grass.pipe_command('r.category', map = '%s_%s' % (vector, tmpname), fs = ';', quiet = True)
    cats = []
    for line in p.stdout:
	cats.append(line.rstrip('\r\n').split(';')[0])
    p.wait()

    #echo "List of categories found: $CATSLIST"
    number = len(cats)
    if number < 1:
	grass.fatal(_("No categories found in raster map"))

    #check if DBF driver used, in this case cut to 10 chars col names:
    try:
        fi = grass.vector_db(map = vector)[int(layer)]
    except KeyError:
	grass.fatal(_('There is no table connected to this map. Run v.db.connect or v.db.addtable first.'))
    # we need this for non-DBF driver:
    dbfdriver = fi['driver'] == 'dbf'

    #Find out which table is linked to the vector map on the given layer
    if not fi['table']:
	grass.fatal(_('There is no table connected to this map. Run v.db.connect or v.db.addtable first.'))

    basecols = ['n', 'min', 'max', 'range', 'mean', 'stddev', 'variance', 'cf_var', 'sum']

    # we need at least three chars to distinguish [mea]n from [med]ian
    # so colprefix can't be longer than 6 chars with DBF driver
    if dbfdriver:
	colprefix = colprefix[:6]

    # do extended stats?
    if flags['e']:
	# namespace is limited in DBF but the % value is important
	if dbfdriver:
	    perccol = "per" + percentile
	else:
	    perccol = "percentile_" + percentile
	extracols = ['first_quartile', 'median', 'third_quartile'] + [perccol]
    else:
	extracols = []

    addcols = []
    for i in basecols + extracols:
	#check if column already present
	currcolumn = ("%s_%s" % (colprefix, i))
	if dbfdriver:
	    currcolumn = currcolumn[:10]

	if currcolumn in grass.vector_columns(vector, layer).keys():
	    if not flags['c']:
		grass.fatal(("Cannot create column <%s> (already present). " % currcolumn) +
			    "Use -c flag to update values in this column.")
	else:
	    if i == "n":
		coltype = "INTEGER"
	    else:
		coltype = "DOUBLE PRECISION"
	    addcols.append(currcolumn + ' ' + coltype)

    if addcols:
	grass.verbose(_("Adding columns '%s'") % addcols)
	if grass.run_command('v.db.addcolumn', map = vector, columns = addcols) != 0:
	    grass.fatal(_("Adding columns failed. Exiting."))

    #loop over cats and calculate statistics:
    grass.message(_("Processing data (%d categories)...") % number)

    # get rid of any earlier attempts
    grass.try_remove(sqltmp)

    # do extended stats?
    if flags['e']:
	extstat = 'e'
    else:
	extstat = ""

    f = file(sqltmp, 'w')
    currnum = 1

    for i in cats:
	grass.message(_("Processing category %s (%d/%d)...") % (i, currnum, number))
	grass.run_command('g.remove', rast = 'MASK', quiet = True, stderr = nuldev)
	grass.mapcalc("MASK = if($name == $i, 1, null())",
		      name = "%s_%s" % (vector, tmpname), i = i)

        #n, min, max, range, mean, stddev, variance, coeff_var, sum
	# How to test r.univar $? exit status? using -e creates the real possibility of out-of-memory errors
	s = grass.read_command('r.univar', flags = 'g' + extstat, map = raster, percentile = percentile)
	vars = grass.parse_key_val(s)

	vars['cf_var'] = vars['coeff_var']

	if flags['e'] and dbfdriver:
	    percvar = 'percentile_' + percentile
	    vars[perccol] = vars[percvar]

	for var in basecols + extracols:
	    value = vars[var]
	    if value.lower() == 'nan':
		value = 'NULL'
	    colname = '%s_%s' % (colprefix, var)
	    if dbfdriver:
		colname = colname[:10]
	    f.write("UPDATE %s SET %s=%s WHERE cat=%s;\n" % (fi['table'], colname, value, i))

	currnum += 1

    f.close()

    grass.message(_("Updating the database ..."))
    exitcode = grass.run_command('db.execute', input = sqltmp,
				 database = fi['database'], driver = fi['driver'])
    
    grass.run_command('g.remove', rast = 'MASK', quiet = True, stderr = nuldev)

    if exitcode == 0:
	grass.message(("Statistics calculated from raster map <%s>" % raster) +
		      (" and uploaded to attribute table of vector map <%s>." % vector))
    
    sys.exit(exitcode)

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
