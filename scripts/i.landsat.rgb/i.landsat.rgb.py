#!/usr/bin/env python

############################################################################
#
# MODULE:	i.landsat.rgb
#
# AUTHOR(S):	Markus Neteler. <neteler itc it>
#		Hamish Bowman, scripting enhancements
#               Converted to Python by Glynn Clements
#
# PURPOSE:      create pretty LANDSAT RGBs: the trick is to remove outliers 
#               using percentiles (area under the histogram curve)
#
# COPYRIGHT:	(C) 2006, 2008, 2012 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
# TODO: implement better brightness control
#############################################################################

#%module
#% description: Performs auto-balancing of colors for LANDSAT images.
#% keywords: imagery
#% keywords: landsat
#% keywords: colors
#%end
#%option G_OPT_R_INPUT
#% key: red
#% description: Name of LANDSAT red channel
#%end
#%option G_OPT_R_INPUT
#% key: green
#% description: Name of LANDSAT green channel
#%end
#%option G_OPT_R_INPUT
#% key: blue
#% description: Name of LANDSAT blue channel
#%end
#%option
#% key: strength
#% type: double
#% description: Cropping intensity (upper brightness level)
#% options: 0-100
#% answer : 98
#% required: no
#%end
#%flag
#% key: f
#% description: Extend colors to full range of data on each channel
#% guisection: Colors
#%end
#%flag
#% key: p
#% description: Preserve relative colors, adjust brightness only
#% guisection: Colors
#%end
#%flag
#% key: r
#% description: Reset to standard color range
#% guisection: Colors
#%end
#%flag
#% key: s
#% description: Process bands serially (default: run in parallel)
#%end

import sys
import os
import string
import grass.script as grass
try:
    # new for python 2.6, in 2.5 it may be easy_install'd.
    import multiprocessing as mp
    do_mp = True
except:
    do_mp = False


def get_percentile(map, percentiles):
    # todo: generalize for any list length
    val1 = percentiles[0]
    val2 = percentiles[1]
    values = '%s,%s' % (val1, val2)
    s = grass.read_command('r.univar', flags = 'ge',
			   map = map, percentile = values)
    kv = grass.parse_key_val(s)
    # cleanse to match what the key name will become
    val_str1 = ('percentile_%.15g' % float(val1)).replace('.','_')
    val_str2 = ('percentile_%.15g' % float(val2)).replace('.','_')
    return (float(kv[val_str1]), float(kv[val_str2]))

# wrapper to handle multiprocesses communications back to the parent
def get_percentile_mp(map, percentiles, conn):
    # Process() doesn't like storing connection parts in
    #  separate dictionaries, only wants to pass through tuples,
    #  so instead of just sending the sending the pipe we have to
    #  send both parts then keep the one we want.  ??
    output_pipe, input_pipe = conn
    input_pipe.close()
    result = get_percentile(map, percentiles)
    grass.debug('child (%s) (%.1f, %.1f)' % (map, result[0], result[1]))
    output_pipe.send(result)
    output_pipe.close()

def set_colors(map, v0, v1):
    rules = [
	"0% black\n",
	"%f black\n" % v0,
	"%f white\n" % v1,
	"100% white\n"
	]
    rules = ''.join(rules)
    grass.write_command('r.colors', map = map, rules = '-', stdin = rules, quiet = True)


def main():
    red = options['red']
    green = options['green']
    blue = options['blue']
    brightness = options['strength']
    full = flags['f']
    preserve = flags['p']
    reset = flags['r']
    
    global do_mp

    if flags['s']:
        do_mp = False

    # 90 or 98? MAX value controls brightness
    # think of percent (0-100), must be positive or 0
    # must be more than "2" ?

    if full:
	for i in [red, green, blue]:
	    grass.run_command('r.colors', map = i, color = 'grey', quiet = True)
	sys.exit(0)

    if reset:
	for i in [red, green, blue]:
	    grass.run_command('r.colors', map = i, color = 'grey255', quiet = True)
	sys.exit(0)


    if not preserve:
        if do_mp:
            grass.message(_("Processing..."))
	    # set up jobs and launch them
	    proc = {}
	    conn = {}
	    for i in [red, green, blue]:
	        conn[i] = mp.Pipe()
	        proc[i] = mp.Process(target = get_percentile_mp,
				     args = (i, ['2', brightness],
				     conn[i],))
		proc[i].start()
            grass.percent(1, 2, 1)
            
	    # collect results and wait for jobs to finish
	    for i in [red, green, blue]:
		output_pipe, input_pipe = conn[i]
		(v0, v1) = input_pipe.recv()
		grass.debug('parent (%s) (%.1f, %.1f)' % (i, v0, v1))
		input_pipe.close()
		proc[i].join()
		set_colors(i, v0, v1)
            grass.percent(1, 1, 1)
	else:
	    for i in [red, green, blue]:
	        grass.message(_("Processing..."))
	        (v0, v1) = get_percentile(i, ['2', brightness])
	        grass.debug("<%s>:  min=%f   max=%f" % (i, v0, v1))
	        set_colors(i, v0, v1)

    else:
	all_max = 0
	all_min = 999999

	if do_mp:
	    grass.message(_("Processing..."))
	    # set up jobs and launch jobs
	    proc = {}
	    conn = {}
	    for i in [red, green, blue]:
		conn[i] = mp.Pipe()
		proc[i] = mp.Process(target = get_percentile_mp,
				     args = (i, ['2', brightness],
				     conn[i],))
		proc[i].start()
            grass.percent(1, 2, 1)
            
	    # collect results and wait for jobs to finish
	    for i in [red, green, blue]:
		output_pipe, input_pipe = conn[i]
		(v0, v1) = input_pipe.recv()
		grass.debug('parent (%s) (%.1f, %.1f)' % (i, v0, v1))
		input_pipe.close()
		proc[i].join()
		all_min = min(all_min, v0)
		all_max = max(all_max, v1)
            grass.percent(1, 1, 1)
	else:
	    for i in [red, green, blue]:
		grass.message(_("Processing..."))
		(v0, v1) = get_percentile(i, ['2', brightness])
		grass.debug("<%s>:  min=%f   max=%f" % (i, v0, v1))
		all_min = min(all_min, v0)
		all_max = max(all_max, v1)

	grass.debug("all_min=%f   all_max=%f" % (all_min, all_max))
	for i in [red, green, blue]:
	    set_colors(i, all_min, all_max)


    # write cmd history:
    mapset = grass.gisenv()['MAPSET']
    for i in [red, green, blue]:
        if grass.find_file(i)['mapset'] == mapset:
            grass.raster_history(i)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
