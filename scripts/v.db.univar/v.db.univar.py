#!/usr/bin/env python

############################################################################
#
# MODULE:	v.db.univar (formerly called v.univar.sh)
# AUTHOR(S):	Michael Barton, Arizona State University
#               Converted to Python by Glynn Clements
# PURPOSE:	Calculates univariate statistics from a GRASS vector map attribute column.
#               Based on r.univar.sh by Markus Neteler
# COPYRIGHT:	(C) 2005, 2007, 2008 by the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#% description: Calculates univariate statistics on selected table column for a GRASS vector map.
#% keywords: vector, statistics
#%End
#%flag
#%  key: e
#%  description: Extended statistics (quartiles and 90th percentile)
#%END
#%option
#% key: table
#% type: string
#% gisprompt: old,vector,vector
#% description: Name of data table
#% required : yes
#%End
#%option
#% key: column
#% type: string
#% description: Column on which to calculate statistics (must be numeric)
#% required : yes
#%end
#%option
#% key: database
#% type: string
#% description: Database/directory for table
#% required : no
#%end
#%option
#% key: driver
#% type: string
#% description: Database driver
#% required : no
#%end
#%option
#% key: where
#% type: string
#% description: WHERE conditions of SQL statement without 'where' keyword
#% required : no
#%end

import sys
import os
import atexit
import math
import grass

def cleanup():
    for ext in ['', '.sort']:
	grass.try_remove(tmp + ext)

def sortfile(infile, outfile):
    inf = file(infile, 'r')
    outf = file(outfile, 'w')

    if grass.find_program('sort', ['-n']):
	grass.run_command('sort', flags = 'n', stdin = inf, stdout = outf)
    else:
	# FIXME: we need a large-file sorting function
	grass.warning("'sort' not found: sorting in memory")
	lines = inf.readlines()
	for i in range(len(lines)):
	    lines[i] = float(lines[i].rstrip('\r\n'))
	lines.sort()
	for line in lines:
	    outf.write(str(line) + '\n')

    inf.close()
    outf.close()

def main():
    global tmp
    tmp = grass.tempfile()

    extend = flags['e']
    table = options['table']
    column = options['column']
    database = options['database']
    driver = options['driver']
    where = options['where']

    grass.message("Calculation for column <%s> of table <%s>..." % (column, table))
    grass.message("Reading column values...")

    sql = "SELECT %s FROM %s" % (column, table)
    if where:
	sql += " WHERE " + where

    if not database:
	database = None

    if not driver:
	driver = None

    tmpf = file(tmp, 'w')
    grass.run_command('db.select', flags = 'c', table = table,
		      database = database, driver = driver, sql = sql,
		      stdout = tmpf)
    tmpf.close()

    # check if result is empty
    tmpf = file(tmp)
    if tmpf.read(1) == '':
	grass.fatal("Table <%s> contains no data.", table)
    tmpf.close()

    # calculate statistics
    grass.message("Calculating statistics...")

    N = 0
    sum = 0.0
    sum2 = 0.0
    sum3 = 0.0
    minv = 1e300
    maxv = -1e300

    tmpf = file(tmp)
    for line in tmpf:
	x = float(line.rstrip('\r\n'))
	N += 1
	sum += x
	sum2 += x * x
	sum3 += abs(x)
	maxv = max(maxv, x)
	minv = min(minv, x)
    tmpf.close()

    if N <= 0:
	grass.fatal("No non-null values found")

    print ""
    print "Number of values:", N
    print "Minimum:", minv
    print "Maximum:", maxv
    print "Range:", maxv - minv
    print "-----"
    print "Mean:", sum/N
    print "Arithmetic mean of absolute values:", sum3/N
    print "Variance:", (sum2 - sum*sum/N)/N
    print "Standard deviation:", math.sqrt((sum2 - sum*sum/N)/N)
    print "Coefficient of variation:", (math.sqrt((sum2 - sum*sum/N)/N))/(math.sqrt(sum*sum)/N)
    print "-----"

    if not extend:
	return

    #preparations:
    sortfile(tmp, tmp + ".sort")

    number = N
    odd = N % 2
    eostr = ['even','odd'][odd]

    q25pos = round(N * 0.25)
    q50apos = round(N * 0.50)
    q50bpos = q50apos + (1 - odd)
    q75pos = round(N * 0.75)
    q90pos = round(N * 0.90)

    inf = file(tmp + ".sort")
    l = 1
    for line in inf:
	if l == q25pos:
	    q25 = float(line.rstrip('\r\n'))
	if l == q50apos:
	    q50a = float(line.rstrip('\r\n'))
	if l == q50bpos:
	    q50b = float(line.rstrip('\r\n'))
	if l == q75pos:
	    q75 = float(line.rstrip('\r\n'))
	if l == q90pos:
	    q90 = float(line.rstrip('\r\n'))
	l += 1

    q50 = (q50a + q50b) / 2

    print "1st Quartile: %f" % q25
    print "Median (%s N): %f" % (eostr, q50)
    print "3rd Quartile: %f" % q75
    print "90th Percentile: %f" % q90

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
