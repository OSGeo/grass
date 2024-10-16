#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.db.renamecolumn
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      interface to db.execute to drop a column from the
#               attribute table connected to a given vector map
#               - Based on v.db.dropcolumn
#               - with special trick for SQLite and DBF (here the new col is
#                 added/values copied/old col deleted)
# COPYRIGHT:    (C) 2007 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
# TODO: MySQL untested
#############################################################################


# %module
# % description: Renames a column in the attribute table connected to a given vector map.
# % keyword: vector
# % keyword: attribute table
# % keyword: database
# % keyword: rename
# %end
# %option G_OPT_V_MAP
# %end
# %option G_OPT_V_FIELD
# %end
# %option
# % key: column
# % type: string
# % description: Old and new name of the column (old,new)
# % required: yes
# % multiple: no
# % key_desc: oldcol,newcol
# %end

import grass.script as gs


def main():
    map = options["map"]
    layer = options["layer"]
    column = options["column"]

    mapset = gs.gisenv()["MAPSET"]

    if not gs.find_file(map, element="vector", mapset=mapset):
        gs.fatal(_("Vector map <%s> not found in current mapset") % map)

    f = gs.vector_layer_db(map, layer)

    table = f["table"]
    keycol = f["key"]
    database = f["database"]
    driver = f["driver"]

    if not table:
        gs.fatal(
            _(
                "There is no table connected to the input vector map. Cannot rename "
                "any column"
            )
        )

    cols = column.split(",")
    oldcol = cols[0]
    newcol = cols[1]

    if driver == "dbf":
        if len(newcol) > 10:
            gs.fatal(
                _(
                    "Column name <%s> too long. The DBF driver supports column names "
                    "not longer than 10 characters"
                )
                % newcol
            )

    if oldcol == keycol:
        gs.fatal(
            _(
                "Cannot rename column <%s> as it is needed to keep table <%s> "
                "connected to the input vector map"
            )
            % (oldcol, table)
        )

    # describe old col
    oldcoltype = None
    for f in gs.db_describe(table, database=database, driver=driver)["cols"]:
        if f[0] != oldcol:
            continue
        oldcoltype = f[1]
        oldcollength = f[2]

    # old col there?
    if not oldcoltype:
        gs.fatal(_("Column <%s> not found in table <%s>") % (oldcol, table))

    # some tricks
    if driver in {"sqlite", "dbf"}:
        if oldcoltype.upper() == "CHARACTER":
            colspec = f"{newcol} varchar({oldcollength})"
        else:
            colspec = f"{newcol} {oldcoltype}"

        gs.run_command("v.db.addcolumn", map=map, layer=layer, column=colspec)
        sql = f'UPDATE {table} SET "{newcol}"="{oldcol}"'
        gs.write_command(
            "db.execute", input="-", database=database, driver=driver, stdin=sql
        )
        gs.run_command("v.db.dropcolumn", map=map, layer=layer, column=oldcol)
    elif driver == "mysql":
        if oldcoltype.upper() == "CHARACTER":
            newcoltype = "varchar(%s)" % (oldcollength)
        else:
            newcoltype = oldcoltype

        sql = f'ALTER TABLE {table} CHANGE "{oldcol}" "{newcol}" {newcoltype}'
        gs.write_command(
            "db.execute", input="-", database=database, driver=driver, stdin=sql
        )
    else:
        sql = f'ALTER TABLE {table} RENAME "{oldcol}" TO "{newcol}"'
        gs.write_command(
            "db.execute", input="-", database=database, driver=driver, stdin=sql
        )

    # write cmd history:
    gs.vector_history(map)


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
