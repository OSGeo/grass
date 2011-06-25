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
# COPYRIGHT:    (c) 2008, 2010-2011 Hamish Bowman, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Sets color rules for features in a vector map using a numeric attribute column.
#% keywords: vector
#% keywords: color table
#%end
#% option
#% key: map
#% type: string
#% gisprompt: old,vector,vector
#% key_desc: name
#% description: Name of vector map 
#% required: yes
#%end
#%option
#% key: layer
#% type: integer
#% description: Layer number
#% gisprompt: old_layer,layer,layer
#% answer: 1
#% required: no
#%end
#%option
#% key: column
#% type: string
#% description: Name of column containing numeric data
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% required : yes
#%end
#%option
#% key: rgb_column
#% type: string
#% required: no
#% description: Name of color column to populate with RGB values
#% gisprompt: new_dbcolumn,dbcolumn,dbcolumn
#% answer: GRASSRGB
#% guisection: Colors
#%end
#%option
#% key: range
#% type: double
#% required: no
#% multiple: no
#% key_desc: min,max
#% description: Manually set range (min,max)
#%end
#%option
#% key: color
#% type: string
#% key_desc: string
#% options: aspect,aspectcolr,bcyr,bgyr,byg,byr,celsius,corine,curvature,differences,elevation,etopo2,evi,gdd,grey,grey1.0,grey255,grey.eq,grey.log,gyr,ndvi,population,precipitation,rainbow,ramp,random,ryb,ryg,sepia,slope,srtm,terrain,wave
#% description: Type of color table
#% required: no
#% guisection: Colors
#%end
#%option
#% key: raster
#% type: string
#% key_desc: name
#% required: no
#% description: Name of raster map from which to copy color table
#% gisprompt: old,cell,raster
#% guisection: Colors
#%end
#%option
#% key: rules
#% type: string
#% required: no
#% description: Name of file containing rules
#% gisprompt: old_file,file,input
#% guisection: Colors
#%end
#%flag
#% key: s
#% description: Save placeholder raster map for use with d.legend
#%end
#%flag
#% key: n
#% description: Invert colors
#% guisection: Colors
#%end

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
    color  = options['color']
    column = options['column']
    layer  = options['layer']
    map    = options['map']
    range  = options['range']
    raster = options['raster']
    rgb_column = options['rgb_column']
    rules  = options['rules']
    flip   = flags['n']
    
    global tmp, tmp_colr, tmp_vcol
    pid = os.getpid()
    tmp = tmp_colr = tmp_vcol = None
    
    mapset = grass.gisenv()['MAPSET']
    gisbase = os.getenv('GISBASE')
    
    # does map exist in CURRENT mapset?
    kv = grass.find_file(map, element = 'vector', mapset = mapset)
    if not kv['file']:
	grass.fatal(_("Vector map <%s> not found in current mapset") % map)
    
    vector = map.split('@', 1)
    
    # sanity check mutually exclusive color options
    if not options['color'] and not options['raster'] and not options['rules']:
	grass.fatal(_("Pick one of color, rules, or raster options"))
    
    if color:
	#### check the color rule is valid
	color_opts = os.listdir(os.path.join(gisbase, 'etc', 'colors'))
	color_opts += ['random', 'grey.eq', 'grey.log', 'rules']
	if color not in color_opts:
	    grass.fatal(_("Invalid color rule <%s>\n") % color +
			_("Valid options are: %s") % ' '.join(color_opts))
    elif raster:
	if not grass.find_file(raster)['name']:
	    grass.fatal(_("Raster raster map <%s> not found") % raster)
    elif rules:
	if not os.access(rules, os.R_OK):
	    grass.fatal(_("Unable to read color rules file <%s>") % rules)
    
    # column checks
    # check input data column
    cols = grass.vector_columns(map, layer = layer)
    if column not in cols:
	grass.fatal(_("Column <%s> not found") % column)
    ncolumn_type = cols[column]['type']
    if ncolumn_type not in ["INTEGER", "DOUBLE PRECISION"]:
	grass.fatal(_("Column <%s> is not numeric but %s") % (column, ncolumn_type))
    
    # check if GRASSRGB column exists, make it if it doesn't
    table = grass.vector_db(map)[int(layer)]['table']
    if rgb_column not in cols:
        # RGB Column not found, create it
	grass.message(_("Creating column <%s>...") % rgb_column)
	if 0 != grass.run_command('v.db.addcolumn', map = map, layer = layer, column = "%s varchar(11)" % rgb_column):
	    grass.fatal(_("Creating color column"))
    else:
	column_type = cols[rgb_column]['type']
	if column_type not in ["CHARACTER", "TEXT"]:
	    grass.fatal(_("Column <%s> is not of compatible type (found %s)") % (rgb_column, column_type))
	else:
	    num_chars = dict([(v[0], int(v[2])) for v in grass.db_describe(table)['cols']])[rgb_column]
	    if num_chars < 11:
		grass.fatal(_("Color column <%s> is not wide enough (needs 11 characters)"), rgb_column)
    
    cvals = grass.vector_db_select(map, layer = int(layer), columns = column)['values'].values()
    
    # find data range
    if range:
	# order doesn't matter
	vals = range.split(',')
    else:
	grass.message(_("Scanning values..."))
	vals = [float(x[0]) for x in cvals]
    
    minval = min(vals)
    maxval = max(vals)

    grass.verbose(_("Range: [%s, %s]") % (minval, maxval))
    if not minval or not maxval:
	grass.fatal(_("Scanning data range"))

    # setup internal region
    grass.use_temp_region()
    grass.run_command('g.region', rows = 2, cols = 2)
    
    tmp_colr = "tmp_colr_%d" % pid
    
    # create dummy raster map
    if ncolumn_type == "INTEGER":
	grass.mapcalc("$tmp_colr = int(if(row() == 1, $minval, $maxval))",
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
    
    grass.run_command('r.colors', map = tmp_colr, flags = flip_flag, quiet = True, **color_cmd)
    
    tmp = grass.tempfile()
    
    # calculate colors and write SQL command file
    grass.message(_("Looking up colors..."))
    
    f = open(tmp, 'w')
    p = grass.feed_command('r.what.color', flags = 'i', input = tmp_colr, stdout = f)
    lastval = None
    for v in sorted(vals):
	if v == lastval:
	    continue
	p.stdin.write('%f\n' % v)
    p.stdin.close()
    p.wait()
    f.close()
    
    tmp_vcol = "%s_vcol.sql" % tmp
    fi = open(tmp, 'r')
    fo = open(tmp_vcol, 'w')
    t = string.Template("UPDATE $table SET $rgb_column = '$colr' WHERE $column = $value;\n")
    found = 0
    for line in fi:
	[value, colr] = line.split(': ')
	colr = colr.strip()
	if len(colr.split(':')) != 3:
	    continue
	fo.write(t.substitute(table = table, rgb_column = rgb_column, colr = colr, column = column, value = value))
	found += 1
    fi.close()
    fo.close()
    
    if not found:
	grass.fatal(_("No values found in color range"))
    
    # apply SQL commands to update the table with values
    grass.message(_("Writing %s colors...") % found)
    
    if 0 != grass.run_command('db.execute', input = tmp_vcol):
	grass.fatal(_("Processing SQL transaction"))
    
    if flags['s']:
	vcolors = "vcolors_%d" % pid
	grass.run_command('g.rename', rast = (tmp_colr, vcolors), quiet = True)
	grass.message(_("Raster map containing color rules saved to <%s>") % vcolors)
	# TODO save full v.colors command line history
	grass.run_command('r.support', map = vcolors,
			  history = "",
			  source1 = "vector map = %s" % map,
			  source2 = "column = %s" % column,
			  title = _("Dummy raster to use as thematic vector legend"),
			  description = "generated by v.colors using r.mapcalc")
	grass.run_command('r.support', map = vcolors,
			  history = _("RGB saved into <%s> using <%s%s%s>") % (rgb_column, color, raster, rules))
    
if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
