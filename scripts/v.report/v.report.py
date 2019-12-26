#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       v.report
# AUTHOR(S):    Markus Neteler, converted to Python by Glynn Clements
#               Bug fixes, sort for coor by Huidae Cho <grass4u gmail.com>
# PURPOSE:      Reports geometry statistics for vector maps
# COPYRIGHT:    (C) 2005, 2007-2017, 2019 by MN and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Reports geometry statistics for vector maps.
#% keyword: vector
#% keyword: geometry
#% keyword: statistics
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
#%flag
#% key: c
#% description: Do not include column names in output
#%end
#%flag
#% key: d
#% description: Report for geometries with no database records
#%end

import sys
import os
import grass.script as grass
from grass.script.utils import decode


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

    nuldev = open(os.devnull, 'w')

    if not grass.find_file(mapname, 'vector')['file']:
        grass.fatal(_("Vector map <%s> not found") % mapname)

    if int(layer) in grass.vector_db(mapname):
        colnames = grass.vector_columns(mapname, layer, getDict=False, stderr=nuldev)
        isConnection = True
    else:
        isConnection = False
        colnames = ['cat']

    if option == 'coor':
        extracolnames = ['x', 'y', 'z']
    else:
        extracolnames = [option]

    if units == 'percent':
        unitsp = 'meters'
    elif units:
        unitsp = units
    else:
        unitsp = None

    # NOTE: we suppress -1 cat and 0 cat
    if isConnection:
        f = grass.vector_db(map=mapname)[int(layer)]
        p = grass.pipe_command('v.db.select', quiet=True, map=mapname, layer=layer)
        records1 = []
        catcol = -1
        ncols = 0
        for line in p.stdout:
            cols = decode(line).rstrip('\r\n').split('|')
            if catcol == -1:
                ncols = len(cols)
                for i in range(0, ncols):
                    if cols[i] == f['key']:
                        catcol = i
                        break
                if catcol == -1:
                    grass.fatal(_("There is a table connected to input vector map '%s', but "
                                  "there is no key column '%s'.") % (mapname, f['key']))
                continue
            if cols[catcol] == '-1' or cols[catcol] == '0':
                continue
            records1.append(cols[:catcol] + [int(cols[catcol])] + cols[(catcol+1):])
        p.wait()
        if p.returncode != 0:
            sys.exit(1)

        records1.sort(key=lambda r: r[catcol])

        if len(records1) == 0:
            try:
                grass.fatal(_("There is a table connected to input vector map '%s', but "
                              "there are no categories present in the key column '%s'. Consider using "
                              "v.to.db to correct this.") % (mapname, f['key']))
            except KeyError:
                pass

        # fetch the requested attribute sorted by cat:
        p = grass.pipe_command('v.to.db', flags='p', quiet=True,
                               map=mapname, option=option,
                               layer=layer, units=unitsp)
        records2 = []
        for line in p.stdout:
            fields = decode(line).rstrip('\r\n').split('|')
            if fields[0] in ['cat', '-1', '0']:
                continue
            records2.append([int(fields[0])] + fields[1:])
        p.wait()
        records2.sort()

        # make pre-table
        # len(records1) may not be the same as len(records2) because
        # v.db.select can return attributes that are not linked to features.
        records3 = []
        for r2 in records2:
            rec = list(filter(lambda r1: r1[catcol] == r2[0], records1))
            if len(rec) > 0:
                res = rec[0] + r2[1:]
            elif flags['d']:
                res = [r2[0]] + [''] * (ncols - 1) + r2[1:]
            else:
                continue
            records3.append(res)
    else:
        catcol = 0
        records1 = []
        p = grass.pipe_command('v.category', inp=mapname, layer=layer, option='print')
        for line in p.stdout:
            field = int(decode(line).rstrip())
            if field > 0:
                records1.append(field)
        p.wait()
        records1.sort()
        records1 = uniq(records1)

        # make pre-table
        p = grass.pipe_command('v.to.db', flags='p', quiet=True,
                               map=mapname, option=option,
                               layer=layer, units=unitsp)
        records3 = []
        for line in p.stdout:
            fields = decode(line).rstrip('\r\n').split('|')
            if fields[0] in ['cat', '-1', '0']:
                continue
            records3.append([int(fields[0])] + fields[1:])
        p.wait()
        records3.sort()

    # print table header
    if not flags['c']:
        sys.stdout.write('|'.join(colnames + extracolnames) + '\n')

    # make and print the table:
    numcols = len(colnames) + len(extracolnames)

    # calculate percents if requested
    if units == 'percent' and option != 'coor':
        # calculate total value
        total = 0
        for r in records3:
            total += float(r[-1])

        # calculate percentages
        records4 = [float(r[-1]) * 100 / total for r in records3]
        if type(records1[0]) == int:
            records3 = [[r1] + [r4] for r1, r4 in zip(records1, records4)]
        else:
            records3 = [r1 + [r4] for r1, r4 in zip(records1, records4)]

    # sort results
    if options['sort']:
        if options['sort'] == 'asc':
            if options['option'] == 'coor':
                records3.sort(key=lambda r: (float(r[-3]), float(r[-2]), float(r[-1])))
            else:
                records3.sort(key=lambda r: float(r[-1]))
        else:
            if options['option'] == 'coor':
                records3.sort(key=lambda r: (float(r[-3]), float(r[-2]), float(r[-1])), reverse=True)
            else:
                records3.sort(key=lambda r: float(r[-1]), reverse=True)

    for r in records3:
        sys.stdout.write('|'.join(map(str, r)) + '\n')

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
