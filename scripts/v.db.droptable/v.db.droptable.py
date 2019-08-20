#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.db.droptable
# AUTHOR(S):   	Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to drop an existing table of given vector map
# COPYRIGHT:    (C) 2005, 2008 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################


#%Module
#% description: Removes existing attribute table of a vector map.
#% keyword: vector
#% keyword: attribute table
#% keyword: database
#%End
#%flag
#% key: f
#% description: Force removal (required for actual deletion of table)
#%end
#%option G_OPT_V_MAP
#%end
#%option G_OPT_DB_TABLE
#% description: Table name (default: vector map name)
#%end
#%option G_OPT_V_FIELD
#% required : no
#%end

import sys
import os
import grass.script as gscript
from grass.exceptions import CalledModuleError


def main():
    force = flags['f']
    map = options['map']
    table = options['table']
    layer = options['layer']

    # We check for existence of the map in the current mapset before
    # doing any other operation.
    info = gscript.find_file(map, element='vector', mapset=".")
    if not info['file']:
        mapset = gscript.gisenv()["MAPSET"]
        # Message is formulated in the way that it does not mislead
        # in case where a map of the same name is in another mapset.
        gscript.fatal(_("Vector map <{name}> not found"
                        " in the current mapset ({mapset})").format(
                            name=map, mapset=mapset))

    # do some paranoia tests as well:
    f = gscript.vector_layer_db(map, layer)

    if not table:
        # Removing table name connected to selected layer
        table = f['table']
        if not table:
            gscript.fatal(_("No table assigned to layer <%s>") % layer)
    else:
        # Removing user specified table
        existingtable = f['table']
        if existingtable != table:
            gscript.fatal(_("User selected table <%s> but the table <%s> "
                            "is linked to layer <%s>") %
                          (table, existingtable, layer))

    # we use the DB settings selected layer
    database = f['database']
    driver = f['driver']

    gscript.message(_("Removing table <%s> linked to layer <%s> of vector"
                      " map <%s>") % (table, layer, map))

    if not force:
        gscript.message(_("You must use the -f (force) flag to actually "
                          "remove the table. Exiting."))
        gscript.message(_("Leaving map/table unchanged."))
        sys.exit(0)

    gscript.message(_("Dropping table <%s>...") % table)

    try:
        gscript.write_command('db.execute', stdin="DROP TABLE %s" % table,
                              input='-', database=database, driver=driver)
    except CalledModuleError:
        gscript.fatal(_("An error occurred while running db.execute"))

    gscript.run_command('v.db.connect', flags='d', map=map, layer=layer)

    gscript.message(_("Current attribute table link(s):"))
    # silently test first to avoid confusing error messages
    nuldev = open(os.devnull, 'w')
    try:
        gscript.run_command('v.db.connect', flags='p', map=map, quiet=True,
                            stdout=nuldev, stderr=nuldev)
    except CalledModuleError:
        gscript.message(_("(No database links remaining)"))
    else:
        gscript.run_command('v.db.connect', flags='p', map=map)

    # write cmd history:
    gscript.vector_history(map)

if __name__ == "__main__":
    options, flags = gscript.parser()
    main()
