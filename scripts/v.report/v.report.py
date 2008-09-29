#!/usr/bin/env python
#
############################################################################
#
# MODULE:	v.report
# AUTHOR(S):	Markus Neteler, converted to Python by Glynn Clements
# PURPOSE:	Reports geometry statistics for vector maps
# COPYRIGHT:	(C) 2005, 2007 by MN and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%Module
#%  description: Reports geometry statistics for vectors.
#%  keywords: vector, report, statistics
#%End
#%Flag
#% key: r
#% description: Reverse sort the result
#%End
#%Flag
#% key: s
#% description: Sort the result
#%End
#%option
#% key: map
#% type: string
#% gisprompt: old,vector,vector
#% description: Name of input vector map
#% required: yes
#%end
#%option
#% key: layer
#% type: integer
#% answer: 1
#% description: Layer number
#% required: no
#%end
#%option
#% key: option
#% type: string
#% description: Value to calculate
#% options: area,length,coor
#% required: yes
#%end
#%option
#% key: units
#% type: string
#% description: mi(les),f(eet),me(ters),k(ilometers),a(cres),h(ectares),p(ercent)
#% options: mi,miles,f,feet,me,meters,k,kilometers,a,acres,h,hectares,p,percent
#% required: no
#%end

import sys
import os
import grass

def uniq(l):
    result = []
    last = None
    for i in l:
	if i != last:
	    result.append(i)
	    last = i
    return result

def main():
    if flags['r'] and flags['s']:
	grass.fatal("Either -r or -s flag")

    mapname = options['map']
    option = options['option']
    layer = options['layer']
    units = options['units']

    nuldev = file(os.devnull, 'w')

    if not grass.find_file(mapname, 'vector')['file']:
	grass.fatal("Vector map '%s' not found in mapset search path." % mapname)

    table_exists = grass.vector_columns(mapname, layer, stderr = nuldev)

    if table_exists:
	colnames = [f[1] for f in grass.vector_columns(mapname, layer, stderr = nuldev)]
    else:
	colnames = ['cat']

    if option == 'coor':
	columns = ['dummy1','dummy2','dummy3']
	extracolnames = ['x','y','z']
    else:
	columns = ['dummy1']
	extracolnames = [option]

    if units in ['p','percent']:
	unitsp = 'meters'
    elif units:
	unitsp = units
    else:
	unitsp = None

    # NOTE: we suppress -1 cat and 0 cat

    if table_exists:
	p = grass.pipe_command('v.db.select', flags='c', map = mapname, layer = layer)
	records1 = []
	for line in p.stdout:
	    cols = line.rstrip('\r\n').split('|')
	    if cols[0] == '0':
		continue
	    records1.append([int(cols[0])] + cols[1:])
	p.wait()
	records1.sort()

	if len(records1) == 0:
	    f = grass.vector_db(mapname, layer)
	    key = f[2]
	    grass.fatal("There is a table connected to input vector map '%s', but" +
			"there are no categories present in the key column '%s'. Consider using" +
			"v.to.db to correct this." % (mapname, key))

	#fetch the requested attribute sorted by cat:
	p = grass.pipe_command('v.to.db', flags = 'p',
			       map = mapname, option = option, columns = columns,
			       layer = layer, units = unitsp)
	records2 = []
	for line in p.stdout:
	    fields = line.rstrip('\r\n').split('|')
	    if fields[0] in ['cat', '-1', '0']:
		continue
	    records2.append([int(fields[0])] + fields[1:])
	p.wait()
	records2.sort()

	#make pre-table
	records3 = [r1 + r2[1:] for r1, r2 in zip(records1, records2)]
    else:
	records1 = []
        p = grass.pipe_command('v.category', inp = mapname, layer = layer, option = 'print')
	for line in p.stdout:
	    field = int(line.rstrip())
	    if field > 0:
		records1.append(field)
	p.wait()
	records1.sort()
	records1 = uniq(records1)

        #make pre-table
	p = grass.pipe_command('v.to.db', flags = 'p',
			       map = mapname, option = option, columns = columns,
			       layer = layer, units = unitsp)
	records3 = []
	for line in p.stdout:
	    fields = line.split('|')
	    if fields[0] in ['cat', '-1', '0']:
		continue
	    records3.append([int(fields[0])] + fields[1:])
	p.wait()
	records3.sort()

    # print table header
    sys.stdout.write('|'.join(colnames + extracolnames) + '\n')

    #make and print the table:
    numcols = len(colnames) + len(extracolnames)

    # calculate percents if requested
    if units != '' and units in ['p','percent']:
	# calculate total area value
	areatot = 0
	for r in records3:
	    areatot += float(r[-1])

	# calculate area percentages
	records4 = [float(r[-1]) * 100 / areatot for r in records3]
	records3 = [r1 + [r4] for r1, r4 in zip(records1, records4)]

    if flags['s']:
	# sort
	records3.sort(key = lambda r: (r[0], r[-1]))
    elif flags['r']:
	# reverse sort
	records3.sort(key = lambda r: (r[0], r[-1]), reverse = True)

    for r in records3:
	sys.stdout.write('|'.join(map(str,r)) + '\n')

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
