#!/usr/bin/env python3
#
############################################################################
#
# MODULE:       v.db.addtable
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
#               Key column added by Martin Landa <landa.martin gmail.com>
#               Table index added by Markus Metz
# PURPOSE:      interface to db.execute to creates and add a new table to given vector map
# COPYRIGHT:    (C) 2005, 2007, 2008, 2011  by Markus Neteler & the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# %module
# % description: Creates and connects a new attribute table to a given layer of an existing vector map.
# % keyword: vector
# % keyword: attribute table
# % keyword: database
# %end
# %option G_OPT_V_MAP
# %end
# %option
# % key: table
# % type: string
# % description: Name of new attribute table (default: vector map name)
# % required: no
# % guisection: Definition
# %end
# %option
# % key: layer
# % type: integer
# % description: Layer number where to add new attribute table
# % answer: 1
# % required: no
# % guisection: Definition
# %end
# %option G_OPT_DB_KEYCOLUMN
# % guisection: Definition
# %end
# %option
# % key: columns
# % type: string
# % label: Name and type of the new column(s) ('name type [,name type, ...]')
# % description: Types depend on database backend, but all support VARCHAR(), INT, DOUBLE PRECISION and DATE. Example: 'label varchar(250), value integer'
# % required: no
# % multiple: yes
# % key_desc: name type
# % guisection: Definition
# %end

import sys
import os
import grass.script as gs
from grass.script.utils import decode
from grass.exceptions import CalledModuleError


def main():
    vector = options["map"]
    table = options["table"]
    layer = options["layer"]
    columns = options["columns"]
    key = options["key"]

    # does map exist in CURRENT mapset?
    mapset = gs.gisenv()["MAPSET"]
    if not gs.find_file(vector, element="vector", mapset=mapset)["file"]:
        gs.fatal(_("Vector map <%s> not found in current mapset") % vector)

    map_name = vector.split("@")[0]

    if not table:
        if layer == "1":
            gs.verbose(_("Using vector map name as table name: <%s>") % map_name)
            table = map_name
        else:
            # to avoid tables with identical names on higher layers
            table = "%s_%s" % (map_name, layer)
            gs.verbose(
                _("Using vector map name extended by layer number as table name: <%s>")
                % table
            )
    else:
        gs.verbose(_("Using user specified table name: %s") % table)

    # check if DB parameters are set, and if not set them.
    gs.run_command("db.connect", flags="c", quiet=True)
    gs.verbose(_("Creating new DB connection based on default mapset settings..."))
    kv = gs.db_connection()
    database = kv["database"]
    driver = kv["driver"]
    schema = kv["schema"]

    database2 = database.replace("$MAP/", map_name + "/")

    # maybe there is already a table linked to the selected layer?
    nuldev = open(os.devnull, "w")
    try:
        gs.vector_db(map_name, stderr=nuldev)[int(layer)]
        gs.fatal(_("There is already a table linked to layer <%s>") % layer)
    except KeyError:
        pass

    # maybe there is already a table with that name?
    tables = gs.read_command(
        "db.tables", flags="p", database=database2, driver=driver, stderr=nuldev
    )
    tables = decode(tables)

    if table not in tables.splitlines():
        colnames = []
        column_def = []
        if columns:
            column_def = []
            for x in " ".join(columns.split()).split(","):
                colname = x.lower().split()[0]
                if colname in colnames:
                    gs.fatal(_("Duplicate column name '%s' not allowed") % colname)
                colnames.append(colname)
                column_def.append(x)

        # if not existing, create it:
        if key not in colnames:
            column_def.insert(0, "%s integer" % key)
        column_def = ",".join(column_def)

        gs.verbose(_("Creating table with columns (%s)...") % column_def)

        sql = "CREATE TABLE %s (%s)" % (table, column_def)
        try:
            gs.run_command("db.execute", database=database2, driver=driver, sql=sql)
        except CalledModuleError:
            gs.fatal(_("Unable to create table <%s>") % table)

    # connect the map to the DB:
    if schema:
        table = "{schema}.{table}".format(schema=schema, table=table)
    gs.verbose(_("Connecting new table to vector map <%s>...") % map_name)
    gs.run_command(
        "v.db.connect",
        quiet=True,
        map=map_name,
        database=database,
        driver=driver,
        layer=layer,
        table=table,
        key=key,
    )

    # finally we have to add cats into the attribute DB to make
    # modules such as v.what.rast happy: (creates new row for each
    # vector line):
    try:
        gs.run_command(
            "v.to.db",
            overwrite=True,
            map=map_name,
            layer=layer,
            option="cat",
            column=key,
            qlayer=layer,
        )
    except CalledModuleError:
        # remove link
        gs.run_command("v.db.connect", quiet=True, flags="d", map=map_name, layer=layer)
        return 1

    gs.verbose(_("Current attribute table links:"))
    if gs.verbosity() > 2:
        gs.run_command("v.db.connect", flags="p", map=map_name)

    # write cmd history:
    gs.vector_history(map_name)

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())
