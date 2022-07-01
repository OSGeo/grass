#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.db.join
# AUTHOR(S):    Markus Neteler
#               Converted to Python by Glynn Clements
# PURPOSE:      Join a table to a map table
# COPYRIGHT:    (C) 2007-2021 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# %module
# % description: Joins a database table to a vector map table.
# % keyword: vector
# % keyword: attribute table
# % keyword: database
# %end

# %option G_OPT_V_MAP
# % description: Vector map to which to join other table
# % guidependency: layer,column
# %end

# %option G_OPT_V_FIELD
# % description: Layer where to join
# % guidependency: column
# %end

# %option G_OPT_DB_COLUMN
# % description: Identifier column (e.g.: cat) in the vector table to be used for join
# % required : yes
# %end

# %option G_OPT_DB_TABLE
# % key: other_table
# % description: Other table name
# % required: yes
# % guidependency: ocolumn,scolumns
# %end

# %option G_OPT_DB_COLUMN
# % key: other_column
# % description: Identifier column (e.g.: id) in the other table used for join
# % required: yes
# %end

# %option G_OPT_DB_COLUMN
# % key: subset_columns
# % multiple: yes
# % required: no
# % description: Subset of columns from the other table
# %end

import sys
import string
import grass.script as grass
from grass.exceptions import CalledModuleError


def main():
    map = options["map"]
    layer = options["layer"]
    column = options["column"]
    otable = options["other_table"]
    ocolumn = options["other_column"]
    if options["subset_columns"]:
        scolumns = options["subset_columns"].split(",")
    else:
        scolumns = None

    try:
        f = grass.vector_layer_db(map, layer)
    except CalledModuleError:
        sys.exit(1)

    # Include mapset into the name, so we avoid multiple messages about
    # found in more mapsets. The following generates an error message, while the code
    # above does not. However, the above checks that the map exists, so we don't
    # check it here.
    map = grass.find_file(map, element="vector")["fullname"]

    maptable = f["table"]
    database = f["database"]
    driver = f["driver"]

    if driver == "dbf":
        grass.fatal(_("JOIN is not supported for tables stored in DBF format"))

    if not maptable:
        grass.fatal(
            _("There is no table connected to this map. Unable to join any column.")
        )

    # check if column is in map table
    if column not in grass.vector_columns(map, layer):
        grass.fatal(_("Column <%s> not found in table <%s>") % (column, maptable))

    # describe other table
    all_cols_ot = grass.db_describe(otable, driver=driver, database=database)["cols"]

    # check if ocolumn is on other table
    if ocolumn not in [ocol[0] for ocol in all_cols_ot]:
        grass.fatal(_("Column <%s> not found in table <%s>") % (ocolumn, otable))

    # determine columns subset from other table
    if not scolumns:
        # select all columns from other table
        cols_to_add = all_cols_ot
    else:
        cols_to_add = []
        # check if scolumns exists in the other table
        for scol in scolumns:
            found = False
            for col_ot in all_cols_ot:
                if scol == col_ot[0]:
                    found = True
                    cols_to_add.append(col_ot)
                    break
            if not found:
                grass.warning(_("Column <%s> not found in table <%s>") % (scol, otable))

    all_cols_tt = grass.vector_columns(map, int(layer)).keys()
    # This is used for testing presence (and potential name conflict) with
    # the newly added columns, but the test needs to case-insensitive since it
    # is SQL, so we lowercase the names here and in the test.
    all_cols_tt = [name.lower() for name in all_cols_tt]

    select = "SELECT $colname FROM $otable WHERE $otable.$ocolumn=$table.$column"
    template = string.Template("UPDATE $table SET $colname=(%s);" % select)

    for col in cols_to_add:
        # skip the vector column which is used for join
        colname = col[0]
        if colname == column:
            continue

        use_len = False
        if len(col) > 2:
            use_len = True
            # Sqlite 3 does not support the precision number any more
            if driver == "sqlite":
                use_len = False
            # MySQL - expect format DOUBLE PRECISION(M,D), see #2792
            elif driver == "mysql" and col[1] == "DOUBLE PRECISION":
                use_len = False

        if use_len:
            coltype = "%s(%s)" % (col[1], col[2])
        else:
            coltype = "%s" % col[1]

        colspec = "%s %s" % (colname, coltype)

        # add only the new column to the table
        if colname.lower() not in all_cols_tt:
            try:
                grass.run_command(
                    "v.db.addcolumn", map=map, columns=colspec, layer=layer
                )
            except CalledModuleError:
                grass.fatal(_("Error creating column <%s>") % colname)

        stmt = template.substitute(
            table=maptable,
            column=column,
            otable=otable,
            ocolumn=ocolumn,
            colname=colname,
        )
        grass.debug(stmt, 1)
        grass.verbose(_("Updating column <%s> of vector map <%s>...") % (colname, map))
        try:
            grass.write_command(
                "db.execute", stdin=stmt, input="-", database=database, driver=driver
            )
        except CalledModuleError:
            grass.fatal(_("Error filling column <%s>") % colname)

    # write cmd history
    grass.vector_history(map)

    return 0


if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
