#!/usr/bin/env python3

############################################################################
#
# MODULE:	i.colors.enhance (former i.landsat.rgb)
#
# AUTHOR(S):	Markus Neteler, original author
#		Hamish Bowman, scripting enhancements
#               Converted to Python by Glynn Clements
#
# PURPOSE:      create pretty RGBs: the trick is to remove outliers
#               using percentiles (area under the histogram curve)
#
# COPYRIGHT:	(C) 2006, 2008, 2012-2014 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
# TODO: implement better brightness control
#############################################################################

#%module
#% description: Performs auto-balancing of colors for RGB images.
#% keyword: imagery
#% keyword: RGB
#% keyword: satellite
#% keyword: colors
#%end
#%option G_OPT_R_INPUT
#% key: red
#% description: Name of red channel
#%end
#%option G_OPT_R_INPUT
#% key: green
#% description: Name of green channel
#%end
#%option G_OPT_R_INPUT
#% key: blue
#% description: Name of blue channel
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

import grass.script as gscript

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

    s = gscript.read_command('r.quantile', input=map,
                             percentiles=values, quiet=True)

    val_str1 = s.splitlines()[0].split(':')[2]
    val_str2 = s.splitlines()[1].split(':')[2]
    return (float(val_str1), float(val_str2))

# wrapper to handle multiprocesses communications back to the parent


def get_percentile_mp(map, percentiles, conn):
    # Process() doesn't like storing connection parts in
    #  separate dictionaries, only wants to pass through tuples,
    #  so instead of just sending the sending the pipe we have to
    #  send both parts then keep the one we want.  ??
    output_pipe, input_pipe = conn
    input_pipe.close()
    result = get_percentile(map, percentiles)
    gscript.debug('child (%s) (%.1f, %.1f)' % (map, result[0], result[1]))
    output_pipe.send(result)
    output_pipe.close()


def set_colors(map, v0, v1):
    rules = ''.join(["0% black\n", "%f black\n" % v0,
                     "%f white\n" % v1, "100% white\n"])
    gscript.write_command('r.colors', map=map, rules='-', stdin=rules,
                          quiet=True)


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

    check = True
    for m in [red, green, blue]:
        ex = gscript.find_file(m)
        if ex['name'] == '':
            check = False
            gscript.warning("Raster map <{}> not found ".format(m))
    if not check:
        gscript.fatal("At least one of the input raster map was not found")
    # 90 or 98? MAX value controls brightness
    # think of percent (0-100), must be positive or 0
    # must be more than "2" ?

    if full:
        for i in [red, green, blue]:
            gscript.run_command('r.colors', map=i, color='grey', quiet=True)
        sys.exit(0)

    if reset:
        for i in [red, green, blue]:
            gscript.run_command('r.colors', map=i, color='grey255', quiet=True)
        sys.exit(0)

    if not preserve:
        if do_mp:
            gscript.message(_("Processing..."))
            # set up jobs and launch them
            proc = {}
            conn = {}
            for i in [red, green, blue]:
                conn[i] = mp.Pipe()
                proc[i] = mp.Process(target=get_percentile_mp,
                                     args=(i, ['2', brightness],
                                           conn[i],))
                proc[i].start()
            gscript.percent(1, 2, 1)

            # collect results and wait for jobs to finish
            for i in [red, green, blue]:
                output_pipe, input_pipe = conn[i]
                (v0, v1) = input_pipe.recv()
                gscript.debug('parent (%s) (%.1f, %.1f)' % (i, v0, v1))
                input_pipe.close()
                proc[i].join()
                set_colors(i, v0, v1)
            gscript.percent(1, 1, 1)
        else:
            for i in [red, green, blue]:
                gscript.message(_("Processing..."))
                (v0, v1) = get_percentile(i, ['2', brightness])
                gscript.debug("<%s>:  min=%f   max=%f" % (i, v0, v1))
                set_colors(i, v0, v1)

    else:
        all_max = 0
        all_min = 999999

        if do_mp:
            gscript.message(_("Processing..."))
            # set up jobs and launch jobs
            proc = {}
            conn = {}
            for i in [red, green, blue]:
                conn[i] = mp.Pipe()
                proc[i] = mp.Process(target=get_percentile_mp,
                                     args=(i, ['2', brightness],
                                           conn[i],))
                proc[i].start()
            gscript.percent(1, 2, 1)

            # collect results and wait for jobs to finish
            for i in [red, green, blue]:
                output_pipe, input_pipe = conn[i]
                (v0, v1) = input_pipe.recv()
                gscript.debug('parent (%s) (%.1f, %.1f)' % (i, v0, v1))
                input_pipe.close()
                proc[i].join()
                all_min = min(all_min, v0)
                all_max = max(all_max, v1)
            gscript.percent(1, 1, 1)
        else:
            for i in [red, green, blue]:
                gscript.message(_("Processing..."))
                (v0, v1) = get_percentile(i, ['2', brightness])
                gscript.debug("<%s>:  min=%f   max=%f" % (i, v0, v1))
                all_min = min(all_min, v0)
                all_max = max(all_max, v1)

        gscript.debug("all_min=%f   all_max=%f" % (all_min, all_max))
        for i in [red, green, blue]:
            set_colors(i, all_min, all_max)

    # write cmd history:
    mapset = gscript.gisenv()['MAPSET']
    for i in [red, green, blue]:
        if gscript.find_file(i)['mapset'] == mapset:
            gscript.raster_history(i)

if __name__ == "__main__":
    options, flags = gscript.parser()
    main()
