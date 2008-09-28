#!/usr/bin/env python
#
############################################################################
#
# MODULE:	r.regression line
# AUTHOR(S):	Dr. Agustin Lobo - alobo@ija.csic.es
#               Updated to GRASS 5.7 by Michael Barton
#               Converted to Python by Glynn Clements
# PURPOSE:	Calculates linear regression from two raster maps: y = a + b*x
# COPYRIGHT:	(C) 2004 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################


#%Module
#%  description: Calculates linear regression from two raster maps: y = a + b*x
#%  keywords: raster, statistics
#%End
#%flag
#%  key: g
#%  description: Print in shell script style
#%End
#%flag
#%  key: s
#%  description: Slower but accurate (applies to FP maps only)
#%End
#%option
#% key: map1
#% type: string
#% gisprompt: old,cell,raster
#% description: Map for x coefficient
#% required : yes
#%end
#%option
#% key: map2
#% type: string
#% gisprompt: old,cell,raster
#% description: Map for y coefficient
#% required : yes
#%end
#%option
#% key: output
#% type: string
#% gisprompt: new_file,file,output
#% description: ASCII file for storing regression coefficients (output to screen if file not specified).
#% required : no
#%end

import sys
import os
import math
import grass

def main():
    shell = flags['g']
    slow = flags['s']
    map1 = options['map1']
    map2 = options['map2']
    output = options['output']

    if not grass.find_file(map1)['name']:
	grass.fatal("Raster map <%s> not found" % map1)
    if not grass.find_file(map2)['name']:
	grass.fatal("Raster map <%s> not found" % map2)

    #calculate regression equation
    if slow:
	# slower but accurate
	statsflags = 'n1'
	#				 | sed 's+$+ 1+g' > "$TMP"
    else:
	# count "identical" pixels
	statsflags = 'cnA'

    p = grass.pipe_command('r.stats', flags = statsflags, input = (map1, map2))
    tot = sumX = sumsqX = sumY = sumsqY = sumXY = 0.0
    for line in p.stdout:
	line = line.rstrip('\r\n')
	if slow:
	    line += ' 1'
	f = line.split(' ')
	if len(f) != 3:
	    continue
	x = float(f[0])
	y = float(f[1])
	n = float(f[2])

	tot += n
	sumX += n * x
	sumsqX += n * x * x
	sumY += n * y
	sumsqY += n * y * y
	sumXY += n * x * y

    p.wait()

    B = (sumXY - sumX*sumY/tot)/(sumsqX - sumX*sumX/tot)
    R = (sumXY - sumX*sumY/tot)/math.sqrt((sumsqX - sumX*sumX/tot)*(sumsqY - sumY*sumY/tot))

    mediaX = sumX/tot
    sumsqX = sumsqX/tot
    varX = sumsqX-(mediaX*mediaX)
    sdX = math.sqrt(varX)

    mediaY = sumY/tot
    sumsqY = sumsqY/tot
    varY = sumsqY-(mediaY*mediaY)
    sdY = math.sqrt(varY)

    A = mediaY - B*mediaX
    F =  R*R/(1-R*R/tot-2)

    vars = dict(
	a = A,
	b = B,
	R = R,
	N = tot,
	F = F,
	medX = mediaX,
	sdX = sdX,
	medY = mediaY,
	sdY = sdY)
    keys = ['a', 'b', 'R', 'N', 'F', 'medX', 'sdX', 'medY', 'sdY']

    #send output to screen or text file
    if output:
	fh = file(output, 'w')
    else:
	fh = sys.stdout

    if shell:
	for k, v in vars.iteritems():
	    fh.write('%s=%f\n' % (k, v))
    else:
	lines = [
	    "y = a + b*x",
            "   a: offset",
            "   b: gain",
            "   R: sumXY - sumX*sumY/tot",
            "   N: number of elements",
            "   medX, medY: Means",
            "   sdX, sdY: Standard deviations",
	    '  '.join(keys),
	    ]
	fh.write('\n'.join(lines) + '\n')
	fh.write(' '.join([str(vars[k]) for k in keys]) + '\n')

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
