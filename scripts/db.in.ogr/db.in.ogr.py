#!/usr/bin/env python3

############################################################################
#
# MODULE:       db.in.ogr
# AUTHOR(S):   	Markus Neteler
# PURPOSE:      Imports attribute tables in various formats
#               Converted to Python by Glynn Clements
# COPYRIGHT:    (C) 2007-2021 by Markus Neteler and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# %Module
# % description: Imports attribute tables in various formats.
# % keyword: database
# % keyword: import
# % keyword: attribute table
# % overwrite: yes
# %End

# %option G_OPT_F_BIN_INPUT
# % description: Table file to be imported or DB connection string
# %end

# %option
# % key: gdal_config
# % type: string
# % label: GDAL configuration options
# % description: Comma-separated list of key=value pairs
# %end

# %option
# % key: gdal_doo
# % type: string
# % label: GDAL dataset open options
# % description: Comma-separated list of key=value pairs
# %end

# %option
# % key: db_table
# % type: string
# % key_desc : name
# % description: Name of table from given DB to be imported
# % required : no
# %end

# %option G_OPT_DB_TABLE
# % key: output
# % description: Name for output table
# % required : no
# % guisection: Output
# % gisprompt: new,dbtable,dbtable
# %end

# %option
# % key: key
# % type: string
# % description: Name for auto-generated unique key column
# % required : no
# % guisection: Output
# %end

# %option
# % key: encoding
# % type: string
# % label: Encoding value for attribute data
# % descriptions: Overrides encoding interpretation, useful when importing DBF tables
# % guisection: Output
# %end

import os
import grass.script as gs
from grass.script.utils import decode
from grass.exceptions import CalledModuleError


def main():
    input = options["input"]
    gdal_config = options["gdal_config"]
    gdal_doo = options["gdal_doo"]
    db_table = options["db_table"]
    output = options["output"]
    key = options["key"]

    mapset = gs.gisenv()["MAPSET"]

    if db_table:
        input = db_table

    if not output:
        tmpname = input.replace(".", "_")
        output = gs.basename(tmpname)

    # check if table exists
    try:
        with open(os.devnull, "w+") as nuldev:
            s = gs.read_command("db.tables", flags="p", quiet=True, stderr=nuldev)
    except CalledModuleError:
        # check connection parameters, set if uninitialized
        gs.read_command("db.connect", flags="c")
        s = gs.read_command("db.tables", flags="p", quiet=True)

    for line in decode(s).splitlines():
        if line == output:
            if gs.overwrite():
                gs.warning(
                    _("Table <%s> already exists and will be overwritten") % output
                )
                gs.write_command(
                    "db.execute", input="-", stdin="DROP TABLE %s" % output
                )
                break
            gs.fatal(_("Table <%s> already exists") % output)

    # treat DB as real vector map...
    layer = db_table or None

    vopts = {}
    if options["encoding"]:
        vopts["encoding"] = options["encoding"]

    try:
        gs.run_command(
            "v.in.ogr",
            flags="o",
            input=input,
            gdal_config=gdal_config,
            gdal_doo=gdal_doo,
            output=output,
            layer=layer,
            quiet=True,
            **vopts,
        )
    except CalledModuleError:
        if db_table:
            gs.fatal(_("Input table <%s> not found or not readable") % input)
        else:
            gs.fatal(_("Input DSN <%s> not found or not readable") % input)

    # save db connection settings of the output
    f = gs.vector_layer_db(output, "1")

    table = f["table"]
    database = f["database"]
    driver = f["driver"]

    # rename ID col if requested from cat to new name
    if key:
        gs.write_command(
            "db.execute",
            quiet=True,
            input="-",
            stdin="ALTER TABLE %s ADD COLUMN %s integer" % (output, key),
        )
        gs.write_command(
            "db.execute",
            quiet=True,
            input="-",
            stdin="UPDATE %s SET %s=cat" % (output, key),
        )

    # ... and immediately drop the empty geometry
    vectfile = gs.find_file(output, element="vector", mapset=mapset)["file"]
    if not vectfile:
        gs.fatal(_("Something went wrong. Should not happen"))
    else:
        # remove the vector part
        gs.run_command("v.db.connect", quiet=True, map=output, layer="1", flags="d")
        gs.run_command("g.remove", flags="f", quiet=True, type="vector", name=output)

    # get rid of superfluous auto-added cat column (and cat_ if present)
    with open(os.devnull, "w+") as nuldev:
        gs.run_command(
            "db.dropcolumn",
            quiet=True,
            flags="f",
            table=table,
            database=database,
            driver=driver,
            column="cat",
            stdout=nuldev,
            stderr=nuldev,
        )

    records = gs.db_describe(table, database=database, driver=driver)["nrows"]
    gs.message(_("Imported table <%s> with %d rows") % (output, records))


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
