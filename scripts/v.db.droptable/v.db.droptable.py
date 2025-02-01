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


# %Module
# % description: Removes existing attribute table of a vector map.
# % keyword: vector
# % keyword: attribute table
# % keyword: database
# %End
# %flag
# % key: f
# % description: Force removal (required for actual deletion of table)
# %end
# %option G_OPT_V_MAP
# %end
# %option G_OPT_DB_TABLE
# % description: Table name (default: vector map name)
# %end
# %option G_OPT_V_FIELD
# % required : no
# %end

import sys
import os
import grass.script as gs
from grass.exceptions import CalledModuleError


def main():
    force = flags["f"]
    map = options["map"]
    table = options["table"]
    layer = options["layer"]

    # We check for existence of the map in the current mapset before
    # doing any other operation.
    info = gs.find_file(map, element="vector", mapset=".")
    if not info["file"]:
        mapset = gs.gisenv()["MAPSET"]
        # Message is formulated in the way that it does not mislead
        # in case where a map of the same name is in another mapset.
        gs.fatal(
            _("Vector map <{name}> not found in the current mapset ({mapset})").format(
                name=map, mapset=mapset
            )
        )

    # do some paranoia tests as well:
    f = gs.vector_layer_db(map, layer)

    if not table:
        # Removing table name connected to selected layer
        table = f["table"]
        if not table:
            gs.fatal(_("No table assigned to layer <%s>") % layer)
    else:
        # Removing user specified table
        existingtable = f["table"]
        if existingtable != table:
            gs.fatal(
                _("User selected table <%s> but the table <%s> is linked to layer <%s>")
                % (table, existingtable, layer)
            )

    # we use the DB settings selected layer
    database = f["database"]
    driver = f["driver"]

    gs.message(
        _("Removing table <%s> linked to layer <%s> of vector map <%s>")
        % (table, layer, map)
    )

    if not force:
        gs.message(
            _("You must use the -f (force) flag to actually remove the table. Exiting.")
        )
        gs.message(_("Leaving map/table unchanged."))
        sys.exit(0)

    gs.message(_("Dropping table <%s>...") % table)

    try:
        gs.write_command(
            "db.execute",
            stdin="DROP TABLE %s" % table,
            input="-",
            database=database,
            driver=driver,
        )
    except CalledModuleError:
        gs.fatal(_("An error occurred while running db.execute"))

    gs.run_command("v.db.connect", flags="d", map=map, layer=layer)

    gs.message(_("Current attribute table link(s):"))
    # silently test first to avoid confusing error messages
    nuldev = open(os.devnull, "w")
    try:
        gs.run_command(
            "v.db.connect", flags="p", map=map, quiet=True, stdout=nuldev, stderr=nuldev
        )
    except CalledModuleError:
        gs.message(_("(No database links remaining)"))
    else:
        gs.run_command("v.db.connect", flags="p", map=map)

    # write cmd history:
    gs.vector_history(map)


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
