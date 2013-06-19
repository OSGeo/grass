#!/usr/bin/env python

############################################################################
#
# MODULE:	v.rast.stats
# AUTHOR(S):	Markus Neteler
#		converted to Python by Glynn Clements
#		speed up by Markus Metz
# PURPOSE:	Calculates univariate statistics from a GRASS raster map
#		only for areas covered by vector objects on a per-category base
# COPYRIGHT:	(C) 2005-2010 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Calculates univariate statistics from a raster map based on vector polygon map and uploads statistics to new attribute columns.
#% keywords: vector
#% keywords: statistics
#% keywords: raster
#% keywords: univariate statistics
#% keywords: zonal statistics
#%end
#%flag
#% key: c
#% description: Continue if upload column(s) already exist
#%end
#%flag
#% key: e
#% description: Calculate extended statistics
#%end
#%option G_OPT_V_MAP
#% key: vector
#%end
#%option G_OPT_V_FIELD
#%end
#%option G_OPT_R_INPUT
#% key: raster
#% description: Name of input raster map to calculate statistics from
#%end
#%option
#% key: column_prefix
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
    if rastertmp:
	grass.run_command('g.remove', rast = rastertmp, quiet = True)
    grass.run_command('g.remove', rast = 'MASK', quiet = True, stderr = nuldev)
    if mask_found:
	grass.message(_("Restoring previous MASK..."))
	grass.run_command('g.rename', rast = (tmpname + "_origmask", 'MASK'), quiet = True)
#    for f in [tmp, tmpname, sqltmp]:
#	grass.try_remove(f)

def main():
    global tmp, sqltmp, tmpname, nuldev, vector, mask_found, rastertmp
    mask_found = False
    rastertmp = False
    #### setup temporary files
    tmp = grass.tempfile()
    sqltmp = tmp + ".sql"
    # we need a random name
    tmpname = grass.basename(tmp)

    nuldev = file(os.devnull, 'w')

    raster = options['raster']
    colprefix = options['column_prefix']
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

    rastertmp = "%s_%s" % (vector, tmpname)

    # check the input raster map
    if not grass.find_file(raster, 'cell')['file']:
	grass.fatal(_("Raster map <%s> not found") % raster)

    # check presence of raster MASK, put it aside
    mask_found = bool(grass.find_file('MASK', 'cell')['file'])
    if mask_found:
	grass.message(_("Raster MASK found, temporarily disabled"))
	grass.run_command('g.rename', rast = ('MASK', tmpname + "_origmask"), quiet = True)

    # save current settings:
    grass.use_temp_region()

    # Temporarily aligning region resolution to $RASTER resolution
    # keep boundary settings
    grass.run_command('g.region', align = raster)

    # prepare raster MASK
    if grass.run_command('v.to.rast', input = vector, output = rastertmp,
			 use = 'cat', quiet = True) != 0:
	grass.fatal(_("An error occurred while converting vector to raster"))

    # dump cats to file to avoid "too many argument" problem:
    p = grass.pipe_command('r.category', map = rastertmp, sep = ';', quiet = True)
    cats = []
    for line in p.stdout:
	cats.append(line.rstrip('\r\n').split(';')[0])
    p.wait()

    number = len(cats)
    if number < 1:
	grass.fatal(_("No categories found in raster map"))

    # check if DBF driver used, in this case cut to 10 chars col names:
    try:
        fi = grass.vector_db(map = vector)[int(layer)]
    except KeyError:
	grass.fatal(_('There is no table connected to this map. Run v.db.connect or v.db.addtable first.'))
    # we need this for non-DBF driver:
    dbfdriver = fi['driver'] == 'dbf'

    # Find out which table is linked to the vector map on the given layer
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
	# check if column already present
	currcolumn = ("%s_%s" % (colprefix, i))
	if dbfdriver:
	    currcolumn = currcolumn[:10]

	if currcolumn in grass.vector_columns(vector, layer).keys():
	    if not flags['c']:
		grass.fatal((_("Cannot create column <%s> (already present). ") % currcolumn) +
			    _("Use -c flag to update values in this column."))
	else:
	    if i == "n":
		coltype = "INTEGER"
	    else:
		coltype = "DOUBLE PRECISION"
	    addcols.append(currcolumn + ' ' + coltype)

    if addcols:
	grass.verbose(_("Adding columns '%s'") % addcols)
	if grass.run_command('v.db.addcolumn', map = vector, columns = addcols, layer=layer) != 0:
	    grass.fatal(_("Adding columns failed. Exiting."))

    # calculate statistics:
    grass.message(_("Processing data (%d categories)...") % number)

    # get rid of any earlier attempts
    grass.try_remove(sqltmp)

    colnames = []
    for var in basecols + extracols:
	colname = '%s_%s' % (colprefix, var)
	if dbfdriver:
	    colname = colname[:10]
	colnames.append(colname)

    ntabcols = len(colnames)

    # do extended stats?
    if flags['e']:
	extstat = 'e'
    else:
	extstat = ""
	
    f = file(sqltmp, 'w')

    # do the stats
    p = grass.pipe_command('r.univar', flags = 't' + 'g' + extstat, map = raster, 
                      zones = rastertmp, percentile = percentile, sep = ';')

    first_line = 1
    for line in p.stdout:
	if first_line:
	    first_line = 0
	    continue

	vars = line.rstrip('\r\n').split(';')

	f.write("UPDATE %s SET" % fi['table'])
	i = 2
	first_var = 1
	for colname in colnames:
	    value = vars[i]
	    # convert nan, +nan, -nan to NULL
	    if value.lower().endswith('nan'):
		value = 'NULL'
	    if not first_var:
		f.write(" , ")
	    else:
		first_var = 0
	    f.write(" %s=%s" % (colname, value))
	    i += 1
	    # skip n_null_cells, mean_of_abs, sum_of_abs
	    if i == 3 or i == 8 or i == 13:
		i += 1

	f.write(" WHERE %s=%s;\n" % (fi['key'], vars[0]))

    p.wait()
    f.close()

    grass.message(_("Updating the database ..."))
    exitcode = grass.run_command('db.execute', input = sqltmp,
				 database = fi['database'], driver = fi['driver'])

    grass.run_command('g.remove', rast = 'MASK', quiet = True, stderr = nuldev)

    if exitcode == 0:
	grass.message((_("Statistics calculated from raster map <%s>") % raster) +
		      (_(" and uploaded to attribute table of vector map <%s>.") % vector))
    else:
	grass.warning(_("Failed to upload statistics to attribute table of vector map <%s>.") % vector)
    
    
    sys.exit(exitcode)

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
