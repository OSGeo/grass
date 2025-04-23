#!/usr/bin/env python3

############################################################################
#
# MODULE:	db.univar (formerly called v.univar.sh)
# AUTHOR(S):	Michael Barton, Arizona State University
#               Converted to Python by Glynn Clements
#               Sync'ed to r.univar by Markus Metz
# PURPOSE:	Calculates univariate statistics from a GRASS vector map attribute column.
#               Based on r.univar.sh by Markus Neteler
# COPYRIGHT:	(C) 2005, 2007, 2008 by the GRASS Development Team
#
# 		This program is free software under the GNU General Public
# 		License (>=v2). Read the file COPYING that comes with GRASS
# 		for details.
#
#############################################################################

# %module
# % description: Calculates univariate statistics on selected table column.
# % keyword: database
# % keyword: statistics
# % keyword: attribute table
# %end
# %option G_OPT_DB_TABLE
# % key: table
# % required: yes
# %end
# %option G_OPT_DB_COLUMN
# % description: Name of attribute column on which to calculate statistics (must be numeric)
# % required: yes
# %end
# %option G_OPT_DB_DATABASE
# %end
# %option G_OPT_DB_DRIVER
# % options: dbf,odbc,ogr,sqlite,pg
# %end
# %option G_OPT_DB_WHERE
# %end
# %option
# % key: percentile
# % type: double
# % description: Percentile to calculate (requires extended statistics flag)
# % required : no
# % answer: 90
# % options: 0-100
# % multiple: yes
# %end
# %option
# % key: format
# % type: string
# % multiple: no
# % options: plain,json,shell
# % label: Output format
# % descriptions: plain;Plain text output;json;JSON (JavaScript Object Notation);shell;Shell script style for Bash eval
# %end
# %flag
# % key: e
# % description: Extended statistics (quartiles and 90th percentile)
# %end
# %flag
# % key: g
# % description: Print stats in shell script style
# %end

import sys
import atexit
import json
import math

import grass.script as gs


def cleanup():
    for ext in ["", ".sort"]:
        gs.try_remove(tmp + ext)


def sortfile(infile, outfile):
    with open(infile) as inf, open(outfile, "w") as outf:
        if (not gs.setup.WINDOWS) and gs.find_program("sort", "--help"):
            gs.run_command("sort", flags="n", stdin=inf, stdout=outf)
        else:
            # FIXME: we need a large-file sorting function
            gs.warning(_("'sort' not found: sorting in memory"))
            lines = inf.readlines()
            for i in range(len(lines)):
                lines[i] = float(lines[i].rstrip("\r\n"))
            lines.sort()
            outf.writelines(str(line) + "\n" for line in lines)


def main():
    # A more substantial rewrite of the code is needed, possibly to C or
    # using Python packages such as statistics or NumPy,
    # so ignoring the duplication of final computation of some statistics
    # as well as pushing the limit of how long the function can be.
    # pylint: disable=too-many-branches
    global tmp
    tmp = gs.tempfile()

    extend = flags["e"]
    shellstyle = flags["g"]
    table = options["table"]
    column = options["column"]
    database = options["database"]
    driver = options["driver"]
    where = options["where"]
    perc = options["percentile"]
    output_format = options["format"]

    perc = [float(p) for p in perc.split(",")]

    if not output_format:
        output_format = "shell" if shellstyle else "plain"
    elif shellstyle:
        # This can be a message or warning in future versions.
        # In version 9, -g may be removed.
        gs.verbose(_("The format option is used and -g flag ignored"))

    desc_table = gs.db_describe(table, database=database, driver=driver)
    if not desc_table:
        gs.fatal(_("Unable to describe table <%s>") % table)
    found = False
    for cname, ctype, cwidth in desc_table["cols"]:
        if cname == column:
            found = True
            if ctype not in {"INTEGER", "DOUBLE PRECISION"}:
                gs.fatal(_("Column <%s> is not numeric") % cname)
    if not found:
        gs.fatal(_("Column <%s> not found in table <%s>") % (column, table))

    if output_format == "plain":
        gs.verbose(_("Calculation for column <%s> of table <%s>...") % (column, table))
        gs.message(_("Reading column values..."))

    sql = "SELECT %s FROM %s" % (column, table)
    if where:
        sql += " WHERE " + where

    if not database:
        database = None

    if not driver:
        driver = None

    with open(tmp, "w") as tmpf:
        gs.run_command(
            "db.select",
            flags="c",
            table=table,
            database=database,
            driver=driver,
            sql=sql,
            stdout=tmpf,
        )

    # check if result is empty
    with open(tmp) as tmpf:
        if tmpf.read(1) == "":
            if output_format in {"plain", "shell"}:
                gs.fatal(_("Table <%s> contains no data.") % table)

    # calculate statistics
    if output_format == "plain":
        gs.verbose(_("Calculating statistics..."))

    N = 0
    sum = 0.0
    sum2 = 0.0
    sum3 = 0.0
    minv = 1e300
    maxv = -1e300

    with open(tmp) as tmpf:
        for line in tmpf:
            line = line.rstrip("\r\n")
            if len(line) == 0:
                continue
            x = float(line)
            N += 1
            sum += x
            sum2 += x * x
            sum3 += abs(x)
            maxv = max(maxv, x)
            minv = min(minv, x)

    if N <= 0:
        if output_format in {"plain", "shell"}:
            gs.fatal(_("No non-null values found"))
        else:
            # We produce valid JSON with a value for n even when the query returned
            # no rows or when all values are nulls.
            result = {}
            result["n"] = N
            nan_value = None
            result["min"] = nan_value
            result["max"] = nan_value
            result["range"] = nan_value
            result["mean"] = nan_value
            result["mean_abs"] = nan_value
            result["variance"] = nan_value
            result["stddev"] = nan_value
            result["coeff_var"] = nan_value
            result["sum"] = nan_value
            json.dump({"statistics": result}, sys.stdout)
            return

    if output_format == "plain":
        sys.stdout.write("Number of values: %d\n" % N)
        sys.stdout.write("Minimum: %.15g\n" % minv)
        sys.stdout.write("Maximum: %.15g\n" % maxv)
        sys.stdout.write("Range: %.15g\n" % (maxv - minv))
        sys.stdout.write("Mean: %.15g\n" % (sum / N))
        sys.stdout.write("Arithmetic mean of absolute values: %.15g\n" % (sum3 / N))
        if not ((sum2 - sum * sum / N) / N) < 0:
            sys.stdout.write("Variance: %.15g\n" % ((sum2 - sum * sum / N) / N))
            sys.stdout.write(
                "Standard deviation: %.15g\n" % (math.sqrt((sum2 - sum * sum / N) / N))
            )
            sys.stdout.write(
                "Coefficient of variation: %.15g\n"
                % ((math.sqrt((sum2 - sum * sum / N) / N)) / (math.sqrt(sum * sum) / N))
            )
        else:
            sys.stdout.write("Variance: 0\n")
            sys.stdout.write("Standard deviation: 0\n")
            sys.stdout.write("Coefficient of variation: 0\n")
        sys.stdout.write("Sum: %.15g\n" % sum)
    elif output_format == "json":
        result = {}
        result["n"] = N
        result["min"] = minv
        result["max"] = maxv
        result["range"] = maxv - minv
        result["mean"] = sum / N
        result["mean_abs"] = sum3 / N
        if not ((sum2 - sum * sum / N) / N) < 0:
            result["variance"] = (sum2 - sum * sum / N) / N
            result["stddev"] = math.sqrt((sum2 - sum * sum / N) / N)
            result["coeff_var"] = (math.sqrt((sum2 - sum * sum / N) / N)) / (
                math.sqrt(sum * sum) / N
            )
        else:
            result["variance"] = 0
            result["stddev"] = 0
            result["coeff_var"] = 0
        result["sum"] = sum
        if not extend:
            json.dump({"statistics": result}, sys.stdout)
    elif output_format == "shell":
        sys.stdout.write("n=%d\n" % N)
        sys.stdout.write("min=%.15g\n" % minv)
        sys.stdout.write("max=%.15g\n" % maxv)
        sys.stdout.write("range=%.15g\n" % (maxv - minv))
        sys.stdout.write("mean=%.15g\n" % (sum / N))
        sys.stdout.write("mean_abs=%.15g\n" % (sum3 / N))
        if not ((sum2 - sum * sum / N) / N) < 0:
            sys.stdout.write("variance=%.15g\n" % ((sum2 - sum * sum / N) / N))
            sys.stdout.write("stddev=%.15g\n" % (math.sqrt((sum2 - sum * sum / N) / N)))
            sys.stdout.write(
                "coeff_var=%.15g\n"
                % ((math.sqrt((sum2 - sum * sum / N) / N)) / (math.sqrt(sum * sum) / N))
            )
        else:
            sys.stdout.write("variance=0\n")
            sys.stdout.write("stddev=0\n")
            sys.stdout.write("coeff_var=0\n")
        sys.stdout.write("sum=%.15g\n" % sum)
    else:
        msg = f"Unknown output format {output_format}"
        raise ValueError(msg)

    if not extend:
        return

    # preparations:
    sortfile(tmp, tmp + ".sort")

    odd = N % 2
    eostr = ["even", "odd"][odd]

    q25pos = round(N * 0.25)
    if q25pos == 0:
        q25pos = 1
    q50apos = round(N * 0.50)
    if q50apos == 0:
        q50apos = 1
    q50bpos = q50apos + (1 - odd)
    q75pos = round(N * 0.75)
    if q75pos == 0:
        q75pos = 1

    ppos = {}
    pval = {}
    for i in range(len(perc)):
        ppos[i] = round(N * perc[i] / 100)
        if ppos[i] == 0:
            ppos[i] = 1
        pval[i] = 0

    with open(tmp + ".sort") as inf:
        line_number = 1
        for line in inf:
            line = line.rstrip("\r\n")
            if len(line) == 0:
                continue
            if line_number == q25pos:
                q25 = float(line)
            if line_number == q50apos:
                q50a = float(line)
            if line_number == q50bpos:
                q50b = float(line)
            if line_number == q75pos:
                q75 = float(line)
            for i in range(len(ppos)):
                if line_number == ppos[i]:
                    pval[i] = float(line)
            line_number += 1

    q50 = (q50a + q50b) / 2

    if output_format == "plain":
        sys.stdout.write("1st Quartile: %.15g\n" % q25)
        sys.stdout.write("Median (%s N): %.15g\n" % (eostr, q50))
        sys.stdout.write("3rd Quartile: %.15g\n" % q75)
        for i in range(len(perc)):
            if perc[i] == int(perc[i]):  # integer
                if int(perc[i]) % 10 == 1 and int(perc[i]) != 11:
                    sys.stdout.write(
                        "%dst Percentile: %.15g\n" % (int(perc[i]), pval[i])
                    )
                elif int(perc[i]) % 10 == 2 and int(perc[i]) != 12:
                    sys.stdout.write(
                        "%dnd Percentile: %.15g\n" % (int(perc[i]), pval[i])
                    )
                elif int(perc[i]) % 10 == 3 and int(perc[i]) != 13:
                    sys.stdout.write(
                        "%drd Percentile: %.15g\n" % (int(perc[i]), pval[i])
                    )
                else:
                    sys.stdout.write(
                        "%dth Percentile: %.15g\n" % (int(perc[i]), pval[i])
                    )
            else:
                sys.stdout.write("%.15g Percentile: %.15g\n" % (perc[i], pval[i]))
    elif output_format == "json":
        result["first_quartile"] = q25
        result["median"] = q50
        result["third_quartile"] = q75
        if options["percentile"]:
            percentile_values = []
            for i in range(len(perc)):
                percentile_values.append(pval[i])
        result["percentiles"] = perc
        result["percentile_values"] = percentile_values
        json.dump({"statistics": result}, sys.stdout)
    else:
        sys.stdout.write("first_quartile=%.15g\n" % q25)
        sys.stdout.write("median=%.15g\n" % q50)
        sys.stdout.write("third_quartile=%.15g\n" % q75)
        for i in range(len(perc)):
            percstr = "%.15g" % perc[i]
            percstr = percstr.replace(".", "_")
            sys.stdout.write("percentile_%s=%.15g\n" % (percstr, pval[i]))


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    main()
