#!/usr/bin/env python
############################################################################
#
# MODULE:       v.colors
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago Univeristy,
#                 New Zealand
#               Converted to Python by Glynn Clements
# PURPOSE:      Populate a GRASSRGB column with a color map and data column
#		Helper script for thematic mapping tasks
#
# COPYRIGHT:    (c) 2008 Hamish Bowman, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%Module
#% description: Set color rules for features in a vector using a numeric attribute column.
#% keywords: vector, color table
#%End
#% option
#% key: map
#% type: string
#% gisprompt: old,vector,vector
#% key_desc: name
#% description: Name of input vector map 
#% required: yes
#%end
#%option
#% key: column
#% type: string
#% description: Name of column containg numeric data
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% required : yes
#%end
#%option
#% key: layer
#% type: integer
#% description: Layer number of data column
#% gisprompt: old_layer,layer,layer
#% answer: 1
#% required: no
#%end
#%Option
#% key: rgb_column
#% type: string
#% required: no
#% description: Name of color column to populate
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% answer: GRASSRGB
#% guisection: Colors
#%End
#% option
#% key: range
#% type: double
#% required: no
#% multiple: no
#% key_desc: min,max
#% description: Manually set range (min,max)
#%End
#% option
#% key: color
#% type: string
#% key_desc: style
#% options: aspect,aspectcolr,bcyr,bgyr,byg,byr,celsius,curvature,differences,elevation,etopo2,evi,grey,grey1.0,grey255,gyr,ndvi,population,precipitation,rainbow,ramp,ryb,ryg,sepia,slope,srtm,terrain,wave,random
#% description: Type of color table
#% required: no
#% guisection: Colors
#%end
#%Option
#% key: raster
#% type: string
#% required: no
#% description: Raster map name from which to copy color table
#% gisprompt: old,cell,raster
#% guisection: Colors
#%End
#%Option
#% key: rules
#% type: string
#% required: no
#% description: Path to rules file
#% gisprompt: old_file,file,input
#% guisection: Colors
#%End
#%Flag
#% key: s
#% description: Save placeholder raster map for use with d.legend
#%End
#%Flag
#% key: n
#% description: Invert colors
#% guisection: Colors
#%End


## TODO: implement -e (equalized) and -g (logarithmic) methods in r.colors
##   'v.db.select column= | wc -l' to set region size (1xLength)
##   then create r.in.ascii 1xLength matrix with data (WITHOUT uniq)
##   and run r.colors on that raster map.
##      or
##   v.to.rast, r.colors -g, then parse colr/ file. But that's resolution dependent


import sys
import os
import atexit
import string
import grass.script as grass

def cleanup():
    if tmp:
	grass.try_remove(tmp)
    if tmp_vcol:
	grass.try_remove(tmp_vcol)
    if tmp_colr:
	grass.run_command('g.remove', rast = tmp_colr, quiet = True)

def main():
    color = options['color']
    column = options['column']
    layer = options['layer']
    map = options['map']
    range = options['range']
    raster = options['raster']
    rgb_column = options['rgb_column']
    rules = options['rules']
    flip = flags['n']

    global tmp, tmp_colr, tmp_vcol
    pid = os.getpid()
    tmp = None
    tmp_colr = None
    tmp_vcol = None

    os.environ['GRASS_VERBOSE'] = '0'

    kv = grass.gisenv()
    mapset = kv['MAPSET']
    gisbase = os.getenv('GISBASE')

    #### does map exist in CURRENT mapset?
    kv = grass.find_file(map, element = 'vector', mapset = mapset)
    map_split = map.split('@')
    vect_mapset = map_split[1:]
    if vect_mapset == []:
	vect_mapset = mapset
    else:
	vect_mapset = vect_mapset[0]
    if not kv['file'] or vect_mapset != mapset:
	grass.fatal("Vector map <%s> not found in current mapset" % map)

    vector = map_split[0]

    # sanity check mutually exclusive color options
    ctest = 0
    for opt in ['color', 'raster', 'rules']:
	if options[opt]:
	    ctest += 1
    if ctest != 1:
	grass.fatal("Pick one of color, rules, or raster options")

    if color:
	#### check the color rule is valid
	color_opts = os.listdir(os.path.join(gisbase,'etc','colors'))
	color_opts += ['random', 'grey.eq', 'grey.log', 'rules']
	if color not in color_opts:
	    grass.fatal(("Invalid color rule <%s>\n" % color) +
			("Valid options are: %s" % ' '.join(color_opts)))
    elif raster:
	if not grass.find_file(raster)['name']:
	    grass.fatal("Unable to open raster map <%s>" % raster)
    elif rules:
	if not os.access(rules, os.R_OK):
	    grass.fatal("Unable to read color rules file <%s>" % rules)

    #### column checks
    # check input data column
    cols = grass.vector_columns(map, layer = layer)
    if column not in cols:
	grass.fatal("Column <%s> not found" % column)
    ncolumn_type = cols[column]
    if ncolumn_type not in ["INTEGER", "DOUBLE PRECISION"]:
	grass.fatal("Column <%s> is not numeric" % column)

    #g.message "column <$GIS_OPT_COLUMN> is type [$NCOLUMN_TYPE]"

    # check if GRASSRGB column exists, make it if it doesn't
    table = grass.vector_db(map)[layer]['table']
    if rgb_column not in cols:
        # RGB Column not found, create it
	grass.message("Creating column <%s> ..." % rgb_column)
	if 0 != grass.run_command('v.db.addcol', map = map, layer = layer, column = "%s varchar(11)" % rgb_column):
	    grass.fatal("Creating color column")
    else:
	column_type = cols[rgb_column]
	if column_type not in ["CHARACTER", "TEXT"]:
	    grass.fatal("Column <%s> is not of compatible type (found %s)" % (rgb_column, column_type))
	else:
	    num_chars = dict([(v[0], int(v[2])) for v in grass.db_describe(table)['cols']])[rgb_column]
	    if num_chars < 11:
		grass.fatal("Color column <%s> is not wide enough (needs 11 characters)", rgb_column)

    cvals = grass.vector_db_select(map, layer = layer, column = column)['values'].values()

    # find data range
    if range:
	#order doesn't matter
	vals = range.split(',')
    else:
	grass.message("Scanning values ...")
	vals = [float(x[0]) for x in cvals]

    minval = min(vals)
    maxval = max(vals)

    grass.message(" min=[%s]  max=[$%s]" % (minval, maxval))
    if not minval or not maxval:
	grass.fatal("Scanning data range")

    # setup internal region
    use_temp_region()
    grass.run_command('g.region', rows = 2, cols = 2)

    tmp_colr = "tmp_colr_%d" % pid

    # create dummy raster map
    if ncolumn_type == "INTEGER":
	grass.mapcalc("$tmp_colr = if(row() == 1, $minval, $maxval)",
		      tmp_colr = tmp_colr, minval = minval, maxval = maxval)
    else:
	grass.mapcalc("$tmp_colr = double(if(row() == 1, $minval, $maxval))",
		      tmp_colr = tmp_colr, minval = minval, maxval = maxval)

    if color:
	color_cmd = {'color': color}
    elif raster:
	color_cmd = {'raster': raster}
    elif rules:
	color_cmd = {'rules': rules}

    if flip:
	flip_flag = 'n'
    else:
	flip_flag = ''

    grass.run_command('r.colors', map = tmp_colr, flags = flip_flag, **color_cmd)

    tmp = grass.tempfile()

    # calculate colors and write SQL command file
    grass.message("Looking up colors ...")

    p = grass.start_command('r.what.color', flags = 'i', input = tmp_colr,
			    stdin = grass.PIPE, stdout = grass.PIPE)
    lastval = None
    for v in sorted(cvals):
	if v == lastval:
	    continue
	p.stdin.write('%f\n' % v)
    p.stdin.close()
    p.wait()

    tmp_vcol = "%s_vcol.sql" % tmp
    f = open(tmp_vcol, 'w')
    t = string.Template("UPDATE $table SET $rgb_column = '$colr' WHERE $column = $value;\n")
    found = 0
    for line in p.stdout:
	[value, colr] = line.split(':')
	colr = colr.strip()
	if len(colr.split(':')) != 3:
	    continue
	#g.message message="LINE=[$LINE]"
	f.write(t.substitute(table = table, rgb_column = rgb_column, colr = colr, value = value))
	found += 1
    f.close()
    
    if not found:
	grass.fatal("No values found in color range")

    # apply SQL commands to update the table with values
    grass.message("Writing %s colors ..." % found)
    # less "$TMP"
    if 0 != grass.run_command('db.execute', input = tmp_vcol):
	grass.fatal("Processing SQL transaction")

    if flags['s']:
	vcolors = "vcolors_%d" % pid
	grass.run_command('g.rename', rast = (tmp_colr, vcolors))
	grass.message("Raster map containing color rules saved to <%s>" % vcolors)
	# TODO save full v.colors command line history
	grass.run_command('r.support', map = vcolors,
			  history = "",
			  source1 = "vector map = %s" % map,
			  source2 = "column = %s" % column,
			  title = "Dummy raster to use as thematic vector legend",
			  description = "generated by v.colors using r.mapcalc")
	grass.run_command('r.support', map = vcolors,
			  history = "RGB saved into <%s> using <%s%s%s>" % (rgb_column, color, raster, rules))
    else:
	grass.run_command('g.remove', rast = tmp_colr)

    #v.db.dropcol map=vcol_test col=GRASSRGB
    #d.vect -a vcol_test icon=basic/circle color=none size=8

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
