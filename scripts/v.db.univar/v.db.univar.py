#!/usr/bin/env python

############################################################################
#
# MODULE:	v.db.univar (formerly called v.univar.sh)
# AUTHOR(S):	Michael Barton, Arizona State University
#               Converted to Python by Glynn Clements
#               Sync'ed to r.univar by Markus Metz
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
#% keywords: vector
#% keywords: statistics
#%End
#%flag
#%  key: e
#%  description: Extended statistics (quartiles and 90th percentile)
#%END
#%flag
#%  key: g
#%  description: Print stats in shell script style
#%END
#%option
#% key: table
#% type: string
#% gisprompt: old_dbtable,dbtable,dbtable
#% description: Name of data table
#% required : yes
#%End
#%option
#% key: column
#% type: string
#% gisprompt: old_dbcolumn,dbcolumn,dbcolumn
#% description: Column on which to calculate statistics (must be numeric)
#% required : yes
#%end
#%option
#% key: database
#% type: string
#% gisprompt: old_dbname,dbname,dbname
#% answer: DEFAULT_DBNAME
#% description: Database/directory for table
#% required : no
#%end
#%option
#% key: driver
#% type: string
#% gisprompt: old_dbdriver,dbdriver,dbdriver
#% options: dbf,odbc,ogr,sqlite
#% description: Database driver
#% required : no
#%end
#%option
#% key: where
#% type: string
#% description: WHERE conditions of SQL statement without 'where' keyword
#% required : no
#%end
#%option
#% key: percentile
#% type: double
#% description: Percentile to calculate (requires extended statistics flag)
#% required : no
#% answer: 90
#% options: 0-100
#% multiple: yes
#%end

import sys
import os
import atexit
import math
from grass.script import core as grass

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
        grass.warning(_("'sort' not found: sorting in memory"))
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
    shellstyle = flags['g']
    table = options['table']
    column = options['column']
    database = options['database']
    driver = options['driver']
    where = options['where']
    perc = options['percentile']
    
    perc = [float(p) for p in perc.split(',')]

    if not shellstyle:
        grass.message(_("Calculation for column <%s> of table <%s>...") % (column, table))
        grass.message(_("Reading column values..."))

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
        grass.fatal(_("Table <%s> contains no data.") % table)
        tmpf.close()

    # calculate statistics
    if not shellstyle:
        grass.message(_("Calculating statistics..."))

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
        grass.fatal(_("No non-null values found"))

    if not shellstyle:
        print ""
        print "Number of values: %d" % N
        print "Minimum: %g" % minv
        print "Maximum: %g" % maxv
        print "Range: %g" % (maxv - minv)
        print "-----"
        print "Mean: %g" % (sum/N)
        print "Arithmetic mean of absolute values: %g" % (sum3/N)
        print "Variance: %g" % ((sum2 - sum*sum/N)/N)
        print "Standard deviation: %g" % (math.sqrt((sum2 - sum*sum/N)/N))
        print "Coefficient of variation: %g" % ((math.sqrt((sum2 - sum*sum/N)/N))/(math.sqrt(sum*sum)/N))
        print "Sum: %g" % sum
        print "-----"
    else:
        print "n=%d" % N
        print "min=%.15g" % minv
        print "max=%.15g" % maxv
        print "range=%.15g" % (maxv - minv)
        print "mean=%.15g" % (sum/N)
        print "mean_abs=%.15g" % (sum3/N)
        print "variance=%.15g" % ((sum2 - sum*sum/N)/N)
        print "stddev=%.15g" % (math.sqrt((sum2 - sum*sum/N)/N))
        print "coeff_var=%.15g" % ((math.sqrt((sum2 - sum*sum/N)/N))/(math.sqrt(sum*sum)/N))
        print "sum=%.15g" % sum

    if not extend:
        return

    # preparations:
    sortfile(tmp, tmp + ".sort")

    number = N
    odd = N % 2
    eostr = ['even','odd'][odd]

    q25pos = round(N * 0.25)
    q50apos = round(N * 0.50)
    q50bpos = q50apos + (1 - odd)
    q75pos = round(N * 0.75)

    ppos = {}
    pval = {}
    for i in range(len(perc)):
        ppos[i] = round(N * perc[i] / 100)
        pval[i] = 0

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
        for i in range(len(ppos)):
            if l == ppos[i]:
                pval[i] = float(line.rstrip('\r\n'))
        l += 1

    q50 = (q50a + q50b) / 2

    if not shellstyle:
        print "1st Quartile: %g" % q25
        print "Median (%s N): %g" % (eostr, q50)
        print "3rd Quartile: %g" % q75
        for i in range(len(perc)):
            if perc[i] == int(perc[i]): # integer
                if int(perc[i]) % 10 == 1 and int(perc[i]) != 11:
                    print "%dst Percentile: %g" % (int(perc[i]), pval[i])
                elif int(perc[i]) % 10 == 2 and int(perc[i]) != 12:
                    print "%dnd Percentile: %g" % (int(perc[i]), pval[i])
                elif int(perc[i]) % 10 == 3 and int(perc[i]) != 13:
                    print "%drd Percentile: %g" % (int(perc[i]), pval[i])
                else:
                    print "%dth Percentile: %g" % (int(perc[i]), pval[i])
            else:
                print "%.15g Percentile: %g" % (perc[i], pval[i])
    else:
        print "first_quartile=%g" % q25
        print "median=%g" % q50
        print "third_quartile=%g" % q75
        for i in range(len(perc)):
            percstr = "%.15g" % perc[i]
            percstr = percstr.replace('.','_')
            print "percentile_%s=%g" % (percstr, pval[i])

if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    main()
