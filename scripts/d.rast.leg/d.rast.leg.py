#!/usr/bin/env python

##############################################################################
# d.rast.leg	(GRASS Shell Script)
#
# displays a raster map and its legend on a graphics window. 
#
# Usage: 	d.rast.leg
#	or	d.rast.leg help
#	or	d.rast.leg rast_map [num_of_lines]
#
# Description:	d.rast.leg clears the entire screen, divides it into a main
#		(left) and a minor (right) frames, and then display a raster 
#		map in the main frame and the map legend in the minor frame.
#		The user can run the program interactively or 
#		non-interactively.
#
# Parameters: 	rast_map 	A raster map to be displayed.
#
#	 	num_of_lines 	Number of lines to appear in the legend. 
#				If this number is not given, the legend frame 
#				will display as many lines as number of 
#				categories in the map, otherwise, it will
#				display the first num_of_lines minus 1  
#				categories with the rest being truncated. 
# 
# Note:		The user may adjust the num_of_lines parameter or the size of 
#		graphics window to get an appropriate result.
#
# See also:	d.rast, d.legend.
#
# Jianping Xu and Scott Madry, Rutgers University. October 19, 1993
# Markus Neteler 8/2002: added simple d.legend logic
# Markus Neteler 10/2003: added g.parser
# Michael Barton 12/2004: remove reference to (null)
# Glynn Clements 10/2008: converted to Python
# Michael Barton 9/2011: fix formatting in Python
##############################################################################

#%module
#% description: Displays a raster map and its legend on a graphics window
#% keywords: display
#%end
#%flag
#%  key: f
#%  description: Flip legend
#%end
#%flag
#%  key: n
#%  description: Omit entries with missing label
#%end
#%option G_OPT_R_MAP
#% description: Name of raster map to display
#%end
#%option
#% key: lines
#% type: integer
#% description: Number of lines to appear in the legend
#% required: no
#%end
#%option G_OPT_R_INPUT
#% key: raster
#% description: Name of input raster map to generate legend from
#% required: no
#%end

import sys
import os
import grass.script as grass

def make_frame(f, b, t, l, r):
    (fl, fr, ft, fb) = f

    t /= 100.0
    b /= 100.0
    l /= 100.0
    r /= 100.0

    rt = fb + t * (ft - fb)
    rb = fb + b * (ft - fb)
    rl = fl + l * (fr - fl)
    rr = fl + r * (fr - fl)
    s = '%f,%f,%f,%f' % (rt, rb, rl, rr)
    print "f = " + str(f)
    print "s = " + str(s)
    os.environ['GRASS_FRAME'] = s

def main():
    map = options['map']
    nlines = options['lines']
    rast = options['raster']
    omit = flags['n']
    flip = flags['f']

    #for -n flag of d.legend
    if not grass.find_file(map)['file']:
        grass.fatal(_("Raster map <%s> not found in mapset search path") % map)

    # for rast=
    if rast and not grass.find_file(rast)['file']:
        grass.fatal(_("Raster map <%s> not found in mapset search path") % rast)

    s = grass.read_command('d.info', flags = 'f')
    f = tuple([float(x) for x in s.split()[1:5]])

    grass.run_command('d.erase')
    os.environ['GRASS_PNG_READ'] = 'TRUE'

    #draw title
    # set vertical divide at 65 instead of 80 if real labels in cats/ file??
    make_frame(f, 90, 100,65, 100)
    grass.write_command('d.text', color = 'black', size = 50, stdin = map)

    #draw legend
    if not nlines:
        nlines = None

    if rast:
        lmap = rast
    else:
        lmap = map

    kv = grass.raster_info(map = lmap)
    if kv['datatype'] is 'CELL':
        leg_at = None
    else:
        leg_at = '10,90,5,15'	

    histfiledir = grass.find_file(lmap, 'cell_misc')['file']
    has_hist = os.path.isfile(os.path.join(histfiledir, 'histogram'))

    lflags = ''
    if flip:
        lflags += 'f'
    if has_hist or omit:
        lflags += 'n'

    make_frame(f, 0, 90, 65, 100)
    grass.run_command('d.legend', flags = lflags, map = lmap, lines = nlines, at = leg_at)

    #draw map
    make_frame(f, 0, 100, 0, 65)
    grass.run_command('d.rast', map = map)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
