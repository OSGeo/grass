#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.rast.stats
# AUTHOR(S):    Markus Neteler
#               converted to Python by Glynn Clements
#               speed up by Markus Metz
#               add column choose by Luca Delucchi
# PURPOSE:      Calculates univariate statistics from a GRASS raster map
#               only for areas covered by vector objects on a per-category base
# COPYRIGHT:    (C) 2005-2016 by the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

# %module
# % description: Calculates univariate statistics from a raster map based on a vector map and uploads statistics to new attribute columns.
# % keyword: vector
# % keyword: statistics
# % keyword: raster
# % keyword: univariate statistics
# % keyword: zonal statistics
# % keyword: sampling
# % keyword: querying
# %end
# %flag
# % key: c
# % description: Continue if upload column(s) already exist
# %end
# %flag
# % key: d
# % label: Create densified lines (default: thin lines)
# % description: All cells touched by the line will be set, not only those on the render path
# %end
# %option G_OPT_V_MAP
# %end
# %option G_OPT_V_FIELD
# %end
# %option G_OPT_V_TYPE
# %end
# %option G_OPT_DB_WHERE
# %end
# %option G_OPT_R_INPUTS
# % key: raster
# % description: Name of input raster map to calculate statistics from
# %end
# %option
# % key: column_prefix
# % type: string
# % description: Column prefix for new attribute columns
# % required : yes
# % multiple: yes
# %end
# %option
# % key: method
# % type: string
# % description: The methods to use
# % required: no
# % multiple: yes
# % options: number,null_cells,minimum,maximum,range,average,stddev,variance,coeff_var,sum,first_quartile,median,third_quartile,percentile
# % answer: number,null_cells,minimum,maximum,range,average,stddev,variance,coeff_var,sum,first_quartile,median,third_quartile,percentile
# %end
# %option
# % key: percentile
# % type: integer
# % description: Percentile to calculate
# % options: 0-100
# % answer: 90
# % required : no
# %end

import sys
import os
import atexit
import grass.script as gs
from grass.script.utils import decode
from grass.exceptions import CalledModuleError


def cleanup():
    if rastertmp:
        gs.run_command("g.remove", flags="f", type="raster", name=rastertmp, quiet=True)


#    for f in [tmp, tmpname, sqltmp]:
#        grass.try_remove(f)


def main():
    global tmp, sqltmp, tmpname, nuldev, vector, rastertmp
    rastertmp = False
    # setup temporary files
    tmp = gs.tempfile()
    sqltmp = tmp + ".sql"
    # we need a random name
    tmpname = gs.basename(tmp)

    nuldev = open(os.devnull, "w")

    rasters = options["raster"].split(",")
    colprefixes = options["column_prefix"].split(",")
    vector = options["map"]
    layer = options["layer"]
    vtypes = options["type"]
    where = options["where"]
    percentile = options["percentile"]
    basecols = options["method"].split(",")

    # Get current mapset
    env = gs.gisenv()
    mapset = env["MAPSET"]

    # Get mapset of the vector
    vs = vector.split("@")
    vect_mapset = vs[1] if len(vs) > 1 else mapset

    # does map exist in CURRENT mapset?
    if vect_mapset != mapset or not gs.find_file(vector, "vector", mapset)["file"]:
        gs.fatal(_("Vector map <%s> not found in current mapset") % vector)

    # check if DBF driver used, in this case cut to 10 chars col names:
    try:
        fi = gs.vector_db(map=vector)[int(layer)]
    except KeyError:
        gs.fatal(
            _(
                "There is no table connected to this map. Run v.db.connect or "
                "v.db.addtable first."
            )
        )
    # we need this for non-DBF driver:
    dbfdriver = fi["driver"] == "dbf"

    # colprefix for every raster map?
    if len(colprefixes) != len(rasters):
        gs.fatal(
            _(
                "Number of raster maps ({0}) different from \
                      number of column prefixes ({1})"
            ).format(len(rasters), len(colprefixes))
        )

    vector = vs[0]

    rastertmp = "%s_%s" % (vector, tmpname)

    for raster in rasters:
        # check the input raster map
        if not gs.find_file(raster, "cell")["file"]:
            gs.fatal(_("Raster map <%s> not found") % raster)

    # save current settings:
    gs.use_temp_region()

    # Align region cells with the first input raster,
    # keeping the (approximate) extent settings.
    gs.run_command("g.region", align=rasters[0])

    # check if DBF driver used, in this case cut to 10 chars col names:
    try:
        fi = gs.vector_db(map=vector)[int(layer)]
    except KeyError:
        gs.fatal(
            _(
                "There is no table connected to this map. "
                "Run v.db.connect or v.db.addtable first."
            )
        )
    # we need this for non-DBF driver:
    dbfdriver = fi["driver"] == "dbf"

    # Find out which table is linked to the vector map on the given layer
    if not fi["table"]:
        gs.fatal(
            _(
                "There is no table connected to this map. "
                "Run v.db.connect or v.db.addtable first."
            )
        )

    # prepare base raster for zonal statistics
    prepare_base_raster(vector, layer, rastertmp, vtypes, where)

    # get number of raster categories to be processed
    number = get_nr_of_categories(
        vector,
        layer,
        rasters,
        rastertmp,
        percentile,
        colprefixes,
        basecols,
        dbfdriver,
        flags["c"],
    )

    # calculate statistics:
    gs.message(_("Processing input data (%d categories)...") % number)

    for i in range(len(rasters)):
        raster = rasters[i]

        colprefix, variables_dbf, variables, colnames, extstat = set_up_columns(
            vector, layer, percentile, colprefixes[i], basecols, dbfdriver, flags["c"]
        )

        # get rid of any earlier attempts
        gs.try_remove(sqltmp)

        # do the stats
        perform_stats(
            raster,
            percentile,
            fi,
            dbfdriver,
            colprefix,
            variables_dbf,
            variables,
            colnames,
            extstat,
        )

        gs.message(_("Updating the database ..."))
        exitcode = 0
        try:
            gs.run_command(
                "db.execute", input=sqltmp, database=fi["database"], driver=fi["driver"]
            )
            gs.verbose(
                _(
                    "Statistics calculated from raster map <{raster}>"
                    " and uploaded to attribute table"
                    " of vector map <{vector}>."
                ).format(raster=raster, vector=vector)
            )
        except CalledModuleError:
            gs.warning(
                _("Failed to upload statistics to attribute table of vector map <%s>.")
                % vector
            )
            exitcode = 1

            sys.exit(exitcode)


def prepare_base_raster(vector, layer, rastertmp, vtypes, where):
    """Prepare base raster for zonal statistics.

    :param vector: name of vector map or data source for direct OGR access
    :param layer: layer number or name
    :param where: WHERE conditions of SQL statement without 'where' keyword
    :param rastertmp: name of temporary raster map
    :param vtypes: input feature type
    """
    try:
        nlines = gs.vector_info_topo(vector)["lines"]
        kwargs = {}
        if where:
            kwargs["where"] = where
        # Create densified lines rather than thin lines
        if flags["d"] and nlines > 0:
            kwargs["flags"] = "d"

        gs.run_command(
            "v.to.rast",
            input=vector,
            layer=layer,
            output=rastertmp,
            use="cat",
            type=vtypes,
            quiet=True,
            **kwargs,
        )
    except CalledModuleError:
        gs.fatal(_("An error occurred while converting vector to raster"))


def get_nr_of_categories(
    vector, layer, rasters, rastertmp, percentile, colprefixes, basecols, dbfdriver, c
):
    """Get number of raster categories to be processed.

    Perform also checks of raster and vector categories. In the case of no
    raster categories, create the desired columns and exit.

    :param vector: name of vector map or data source for direct OGR access
    :param layer: layer number or name
    :param rastertmp: name of temporary raster map
    :return: number of raster categories or exit (if no categories found)
    """
    # dump cats to file to avoid "too many argument" problem:
    p = gs.pipe_command("r.category", map=rastertmp, sep=";", quiet=True)
    cats = []

    for line in p.stdout:
        line = decode(line)
        cats.append(line.rstrip("\r\n").split(";")[0])
    p.wait()

    number = len(cats)
    if number < 1:
        # create columns and exit
        gs.warning(_("No categories found in raster map"))
        for i in range(len(rasters)):
            set_up_columns(
                vector,
                layer,
                percentile,
                colprefixes[i],
                basecols,
                dbfdriver,
                flags["c"],
            )
            sys.exit(0)

    # Check if all categories got converted
    # Report categories from vector map
    vect_cats = (
        gs.read_command("v.category", input=vector, option="report", flags="g")
        .rstrip("\n")
        .split("\n")
    )

    # get number of all categories in selected layer
    vect_cats_n = 0  # to be modified below
    for vcl in vect_cats:
        if vcl.split(" ")[0] == layer and vcl.split(" ")[1] == "all":
            vect_cats_n = int(vcl.split(" ")[2])

    if vect_cats_n != number:
        gs.warning(
            _(
                "Not all vector categories converted to raster. \
                        Converted {0} of {1}."
            ).format(number, vect_cats_n)
        )

    return number


def set_up_columns(vector, layer, percentile, colprefix, basecols, dbfdriver, c):
    """Get columns-depending variables and create columns, if needed.

    :param vector: name of vector map or data source for direct OGR access
    :param layer: layer number or name
    :param percentile: percentile to calculate
    :param colprefix: column prefix for new attribute columns
    :param basecols: the methods to use
    :param dbfdriver: boolean saying if the driver is dbf
    :param c: boolean saying if it should continue if upload column(s) already
        exist
    :return: colprefix, variables_dbf, variables, colnames, extstat
    """
    # we need at least three chars to distinguish [mea]n from [med]ian
    # so colprefix can't be longer than 6 chars with DBF driver
    variables_dbf = {}

    if dbfdriver:
        colprefix = colprefix[:6]

    # by default perccol variable is used only for "variables" variable
    perccol = "percentile"
    perc = None
    for b in basecols:
        if b.startswith("p"):
            perc = b
    if perc:
        # namespace is limited in DBF but the % value is important
        perccol = "per" + percentile if dbfdriver else "percentile_" + percentile
        percindex = basecols.index(perc)
        basecols[percindex] = perccol

    # dictionary with name of methods and position in "r.univar -gt"  output
    variables = {
        "number": 2,
        "null_cells": 3,
        "minimum": 4,
        "maximum": 5,
        "range": 6,
        "average": 7,
        "stddev": 9,
        "variance": 10,
        "coeff_var": 11,
        "sum": 12,
        "first_quartile": 14,
        "median": 15,
        "third_quartile": 16,
        perccol: 17,
    }
    # this list is used to set the 'e' flag for r.univar
    extracols = ["first_quartile", "median", "third_quartile", perccol]
    addcols = []
    colnames = []
    extstat = ""
    for i in basecols:
        # this check the complete name of out input that should be truncated
        for k in variables.keys():
            if i in k:
                i = k
                break
        if i in extracols:
            extstat = "e"
        # check if column already present
        currcolumn = "%s_%s" % (colprefix, i)
        if dbfdriver:
            currcolumn = currcolumn[:10]
            variables_dbf[currcolumn.replace("%s_" % colprefix, "")] = i

        colnames.append(currcolumn)
        if currcolumn in gs.vector_columns(vector, layer).keys():
            if not c:
                gs.fatal(
                    (_("Cannot create column <%s> (already present). ") % currcolumn)
                    + _("Use -c flag to update values in this column.")
                )
        else:
            coltype = "INTEGER" if i == "n" else "DOUBLE PRECISION"
            addcols.append(currcolumn + " " + coltype)

    if addcols:
        gs.verbose(_("Adding columns '%s'") % addcols)
        try:
            gs.run_command("v.db.addcolumn", map=vector, columns=addcols, layer=layer)
        except CalledModuleError:
            gs.fatal(_("Adding columns failed. Exiting."))

    return colprefix, variables_dbf, variables, colnames, extstat


def perform_stats(
    raster,
    percentile,
    fi,
    dbfdriver,
    colprefix,
    variables_dbf,
    variables,
    colnames,
    extstat,
):
    with open(sqltmp, "w") as f:
        # do the stats
        p = gs.pipe_command(
            "r.univar",
            flags="t" + extstat,
            map=raster,
            zones=rastertmp,
            percentile=percentile,
            sep=";",
        )

        first_line = 1

        f.write("{0}\n".format(gs.db_begin_transaction(fi["driver"])))
        for line in p.stdout:
            if first_line:
                first_line = 0
                continue

            vars = decode(line).rstrip("\r\n").split(";")

            f.write("UPDATE %s SET" % fi["table"])
            first_var = 1
            for colname in colnames:
                variable = colname.replace("%s_" % colprefix, "", 1)
                if dbfdriver:
                    variable = variables_dbf[variable]
                i = variables[variable]
                value = vars[i]
                # convert nan, +nan, -nan, inf, +inf, -inf, Infinity, +Infinity,
                # -Infinity to NULL
                if value.lower().endswith("nan") or "inf" in value.lower():
                    value = "NULL"
                if not first_var:
                    f.write(" , ")
                else:
                    first_var = 0
                f.write(" %s=%s" % (colname, value))

            f.write(" WHERE %s=%s;\n" % (fi["key"], vars[0]))
        f.write("{0}\n".format(gs.db_commit_transaction(fi["driver"])))
        p.wait()


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    main()
