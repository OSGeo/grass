#!/usr/bin/env python
#
############################################################################
#
# MODULE:	v.report
# AUTHOR(S):	Markus Neteler, converted to Python by Glynn Clements
# PURPOSE:	Reports geometry statistics for vector maps
# COPYRIGHT:	(C) 2005, 2007-2009 by MN and the GRASS Development Team
#
#		This program is free software under the GNU General Public
#		License (>=v2). Read the file COPYING that comes with GRASS
#		for details.
#
#############################################################################

#%module
#% description: Reports geometry statistics for vector maps.
#% keywords: vector
#% keywords: geometry
#% keywords: statistics
#%end
#%option G_OPT_V_MAP
#%end
#%option G_OPT_V_FIELD
#% guisection: Selection
#%end
#%option
#% key: option
#% type: string
#% description: Value to calculate
#% options: area,length,coor
#% required: yes
#%end
#%option G_OPT_M_UNITS
#% options: miles,feet,meters,kilometers,acres,hectares,percent
#%end
#%option
#% key: sort
#% type: string
#% description: Sort the result
#% options: asc,desc
#% descriptions: asc;Sort in ascending order;desc;Sort in descending order
#%end

import sys
import os
import grass.script as grass

def uniq(l):
    result = []
    last = None
    for i in l:
	if i != last:
	    result.append(i)
	    last = i
    return result

def main():
    mapname = options['map']
    option = options['option']
    layer = options['layer']
    units = options['units']

    nuldev = file(os.devnull, 'w')

    if not grass.find_file(mapname, 'vector')['file']:
	grass.fatal(_("Vector map '%s' not found in mapset search path.") % mapname)

    colnames = grass.vector_columns(mapname, layer, getDict = False, stderr = nuldev)
    
    if not colnames:
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
    if colnames:
	p = grass.pipe_command('v.db.select', quiet = True, flags='c', map = mapname, layer = layer)
	records1 = []
	for line in p.stdout:
	    cols = line.rstrip('\r\n').split('|')
	    if cols[0] == '0':
		continue
	    records1.append([int(cols[0])] + cols[1:])
	p.wait()
        if p.returncode != 0:
            sys.exit(1)
        
	records1.sort()

	if len(records1) == 0:
            try:
                f = grass.vector_db(map = mapname)[int(layer)]
                grass.fatal(_("There is a table connected to input vector map '%s', but"
                              "there are no categories present in the key column '%s'. Consider using"
                              "v.to.db to correct this.") % (mapname, f['key']))
            except KeyError:
                pass

	#fetch the requested attribute sorted by cat:
	p = grass.pipe_command('v.to.db', flags = 'p',
                               quiet = True,
			       map = mapname, option = option, columns = columns,
			       layer = layer, units = unitsp)
	records2 = []
	for line in p.stdout:
	    fields = line.rstrip('\r\n').split('|')
	    if fields[0] in ['cat', '-1', '0']:
		continue
	    records2.append([int(fields[0])] + fields[1:-1] + [float(fields[-1])])
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

    # sort results
    if options['sort']:
        if options['sort'] == 'asc':
            records3.sort(key = lambda r: r[-1])
        else:
            records3.sort(key = lambda r: r[-1], reverse = True)
    
    for r in records3:
	sys.stdout.write('|'.join(map(str,r)) + '\n')

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
