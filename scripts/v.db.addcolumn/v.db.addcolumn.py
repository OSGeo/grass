#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       v.db.addcolumnumn
# AUTHOR(S):    Moritz Lennert
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to add a column to the attribute table
#               connected to a given vector map
# COPYRIGHT:    (C) 2005 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


#%module
#% description: Adds one or more columns to the attribute table connected to a given vector map.
#% keyword: vector
#% keyword: attribute table
#% keyword: database
#%end

#%option G_OPT_V_MAP
#%end

#%option G_OPT_V_FIELD
#% label: Layer number where to add column(s)
#%end

#%option
#% key: columns
#% type: string
#% label: Name and type of the new column(s) ('name type [,name type, ...]')
#% description: Types depend on database backend, but all support VARCHAR(), INT, DOUBLE PRECISION and DATE. Example: 'label varchar(250), value integer'
#% required: yes
#% multiple: yes
#% key_desc: name type
#%end

import sys
import os
import grass.script as grass
from grass.script.utils import encode


def main():
    map = options['map']
    layer = options['layer']
    columns = options['columns']
    columns = [col.strip() for col in columns.split(',')]

    # does map exist in CURRENT mapset?
    mapset = grass.gisenv()['MAPSET']
    exists = bool(grass.find_file(map, element='vector', mapset=mapset)['file'])

    if not exists:
        grass.fatal(_("Vector map <%s> not found in current mapset") % map)

    try:
        f = grass.vector_db(map)[int(layer)]
    except KeyError:
        grass.fatal(
            _("There is no table connected to this map. Run v.db.connect or v.db.addtable first."))

    table = f['table']
    database = f['database']
    driver = f['driver']
    column_existing = grass.vector_columns(map, int(layer)).keys()

    for col in columns:
        if not col:
            grass.fatal(_("There is an empty column. Did you leave a trailing comma?"))
        col_name = col.split(' ')[0].strip()
        if col_name in column_existing:
            grass.error(_("Column <%s> is already in the table. Skipping.") % col_name)
            continue
        grass.verbose(_("Adding column <%s> to the table") % col_name)
        p = grass.feed_command('db.execute', input='-',
                               database=database, driver=driver)
        res = "ALTER TABLE {} ADD COLUMN {}".format(table, col)
        p.stdin.write(encode(res))
        grass.debug(res)
        p.stdin.close()
        if p.wait() != 0:
            grass.fatal(_("Unable to add column <%s>.") % col)

    # write cmd history:
    grass.vector_history(map)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
