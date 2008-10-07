#!/usr/bin/env python

############################################################################
#
# MODULE:       i.spectral
# AUTHOR(S):    Markus Neteler, 18. August 1998
#               Converted to Python by Glynn Clements
# PURPOSE:      displays spectral response at user specified locations in group or images
# COPYRIGHT:    (C) 1999 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# written by Markus Neteler 18. August 1998
#            neteler geog.uni-hannover.de
# 
# bugfix: 25. Nov.98/20. Jan. 1999
# 3 March 2006: Added multiple images and group support by Francesco Pirotti - CIRGEO
# this script needs gnuplot

#%Module
#%  description: displays spectral response at user specified locations in group or images
#%  keywords: imagery, raster, multispectral
#%End
#%option
#% key: group
#% type: string
#% gisprompt: old,group
#% description: group input
#% required : no
#%end
#%option
#% key: raster
#% type: string
#% gisprompt: old,cell,raster
#% description: raster input maps
#% multiple : yes
#% required : no
#%end
#%option
#% key: output
#% type: string
#% gisprompt: new_file,file,PNG
#% description: write output to PNG image
#% multiple : no
#% required : no
#%end
#%option
#% key: coords
#% type: double
#% key_desc: east,north
#% description: coordinates
#% multiple : yes
#% required : yes
#%end
#%flag 
#%key: c
#%description: label with coordinates instead of numbering
#%end

import sys
import os
import subprocess
import atexit
import glob
import grass

def cleanup():
    grass.try_remove('spectrum.gnuplot')
    for name in glob.glob('data_[0-9]*'):
	if name[5:].isdigit():
	   grass.try_remove(name)

def main():
    group = options['group']
    raster = options['raster']
    output = options['output']
    coords = options['coords']
    label = flags['c']

    if not group and not raster:
	grass.fatal("Either group= or raster= is required")

    if group and raster:
	grass.fatal("group= and raster= are mutually exclusive")

    #check if present
    if not grass.find_program('gnuplot', ['-V']):
	grass.fatal("gnuplot required, please install first")

    tmp1 = grass.tempfile()
    tmp2 = grass.tempfile()

    # get y-data for gnuplot-data file
    # get data from group files and set the x-axis labels

    if group:
	# ## PARSES THE GROUP FILES - gets rid of ugly header info from group list output
	maps = []
	s = grass.read_command('i.group', flags='l', group = group, quiet = True)
	active = False
	for l in s.splitlines():
	    if l == '-------------':
		active = not active
		continue
	    if not active:
		continue
	    f = l.replace(' in ','@').split()
	    maps += f
	rastermaps = maps
    else:
	# ## get data from list of files and set the x-axis labels
	rastermaps = raster.split(',')

    xlabels = ["'%s' %d" % (n, i) for i, n in enumerate(rastermaps)]
    xlabels = ','.join(xlabels)
    numbands = len(rastermaps)

    what = []
    s = grass.read_command('r.what', input = rastermaps, east_north = coords, quiet = True)
    for l in s.splitlines():
	f = l.split('|')
	for i, v in enumerate(f):
	    if v in ['', '*']:
		f[i] = 0
	    else:
		f[i] = float(v)
	what.append(f)

    numclicks = len(what)
    start = 0

    num = numbands

    # build data files
    for i, row in enumerate(what):
	outfile = 'data_%d' % i
	outf = file(outfile, 'w')
	for j, val in enumerate(row[3:]):
	    outf.write("%s %s\n" % (j, val))
	outf.close()

    xrange = numbands + 1

    # build gnuplot script
    lines = []
    if output:
	lines += [
	    "set term png large",
	    "set output '%s'" % output
	    ]
    lines += [
	"set xtics (%s)" % xlabels,
	"set grid",
	"set title 'Spectral signatures'",
	"set xrange [0:%s]" % xrange,
	"set noclabel",
	"set xlabel 'Bands'",
	"set ylabel 'DN Value'",
	"set data style lines"
	]


    cmd = []
    for i, row in enumerate(what):
	if not label:
	    title = str(i)
	else:
	    title = str(tuple(row[0:2]))
	cmd.append("'data_%d' title '%s'" % (i, title))
    cmd = ','.join(cmd)
    cmd = ' '.join(['plot', cmd, "with linespoints pt 779"])
    lines.append(cmd)

    plotfile = 'spectrum.gnuplot'
    plotf = file(plotfile, 'w')
    for line in lines:
	plotf.write(line + '\n')
    plotf.close()

    if output:
	subprocess.call(['gnuplot', plotfile])
    else:
	subprocess.call(['gnuplot', '-persist', plotfile])

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
