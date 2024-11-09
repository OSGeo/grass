#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.unpack
# AUTHOR(S):    Luca Delucchi
#
# PURPOSE:      Unpack up a vector map packed with v.pack
# COPYRIGHT:    (C) 2010-2017 by the GRASS Development Team
#
#               This program is free software under the GNU General
#               Public License (>=v2). Read the file COPYING that
#               comes with GRASS for details.
#
#############################################################################

# %module
# % description: Imports a GRASS GIS specific vector archive file (packed with v.pack) as a vector map
# % keyword: vector
# % keyword: import
# % keyword: copying
# %end
# %option G_OPT_F_BIN_INPUT
# % description: Name of input pack file
# % key_desc: name.pack
# %end
# %option G_OPT_V_OUTPUT
# % label: Name for output vector map
# % description: Default: taken from input file internals
# % required : no
# % guisection: Output settings
# %end
# %flag
# % key: o
# % label: Override projection check (use current projects's CRS)
# % description: Assume that the dataset has same coordinate reference system as the current project
# % guisection: Output settings
# %end
# %flag
# % key: p
# % label: Print projection information of input pack file and exit
# % guisection: Print
# %end

import os
import sys
import shutil
import tarfile
import atexit

from grass.script.utils import diff_files, try_rmdir
from grass.script import core as grass
from grass.script import db as grassdb
from grass.exceptions import CalledModuleError


def cleanup():
    try_rmdir(tmp_dir)


def main():
    infile = options["input"]

    # create temporary directory
    global tmp_dir
    tmp_dir = grass.tempdir()
    grass.debug("tmp_dir = %s" % tmp_dir)

    # check if the input file exists
    if not os.path.exists(infile):
        grass.fatal(_("File <%s> not found") % infile)

    # copy the files to tmp dir
    input_base = os.path.basename(infile)
    shutil.copyfile(infile, os.path.join(tmp_dir, input_base))
    os.chdir(tmp_dir)
    tar = tarfile.TarFile.open(name=input_base, mode="r")
    try:
        data_name = tar.getnames()[0]
    except IndexError:
        grass.fatal(_("Pack file unreadable"))

    if flags["p"]:
        # print proj info and exit
        try:
            for fname in ["PROJ_INFO", "PROJ_UNITS"]:
                f = tar.extractfile(fname)
                sys.stdout.write(f.read().decode())
        except KeyError:
            grass.fatal(_("Pack file unreadable: file '{}' missing").format(fname))
        tar.close()

        return 0

    # set the output name
    map_name = options["output"] or data_name

    # grass env
    gisenv = grass.gisenv()
    mset_dir = os.path.join(
        gisenv["GISDBASE"], gisenv["LOCATION_NAME"], gisenv["MAPSET"]
    )

    new_dir = os.path.join(mset_dir, "vector", map_name)

    gfile = grass.find_file(name=map_name, element="vector", mapset=".")
    overwrite = os.getenv("GRASS_OVERWRITE")
    if gfile["file"] and overwrite != "1":
        grass.fatal(_("Vector map <%s> already exists") % map_name)
    elif overwrite == "1" and gfile["file"]:
        grass.warning(
            _("Vector map <%s> already exists and will be overwritten") % map_name
        )
        grass.run_command(
            "g.remove", flags="f", quiet=True, type="vector", name=map_name
        )
        shutil.rmtree(new_dir, True)

    # extract data
    # Extraction filters were added in Python 3.12,
    # and backported to 3.8.17, 3.9.17, 3.10.12, and 3.11.4
    # See https://docs.python.org/3.12/library/tarfile.html#tarfile-extraction-filter
    # and https://peps.python.org/pep-0706/
    # In Python 3.12, using `filter=None` triggers a DepreciationWarning,
    # and in Python 3.14, `filter='data'` will be the default
    if hasattr(tarfile, "data_filter"):
        tar.extractall(filter="data")
    else:
        # Remove this when no longer needed
        grass.warning(_("Extracting may be unsafe; consider updating Python"))
        tar.extractall()
    tar.close()
    if os.path.exists(os.path.join(data_name, "coor")):
        pass
    elif os.path.exists(os.path.join(data_name, "cell")):
        grass.fatal(
            _(
                "This GRASS GIS pack file contains raster data. Use "
                "r.unpack to unpack <%s>"
            )
            % map_name
        )
    else:
        grass.fatal(_("Pack file unreadable"))

    # check projection compatibility in a rather crappy way
    loc_proj = os.path.join(mset_dir, "..", "PERMANENT", "PROJ_INFO")
    loc_proj_units = os.path.join(mset_dir, "..", "PERMANENT", "PROJ_UNITS")

    skip_projection_check = False
    if not os.path.exists(os.path.join(tmp_dir, "PROJ_INFO")):
        if os.path.exists(loc_proj):
            grass.fatal(
                _(
                    "PROJ_INFO file is missing, unpack vector map in XY (unprojected) "
                    "project."
                )
            )
        skip_projection_check = True  # XY location

    if not skip_projection_check:
        diff_result_1 = diff_result_2 = None
        if not grass.compare_key_value_text_files(
            filename_a=os.path.join(tmp_dir, "PROJ_INFO"),
            filename_b=loc_proj,
            proj=True,
        ):
            diff_result_1 = diff_files(os.path.join(tmp_dir, "PROJ_INFO"), loc_proj)

        if not grass.compare_key_value_text_files(
            filename_a=os.path.join(tmp_dir, "PROJ_UNITS"),
            filename_b=loc_proj_units,
            units=True,
        ):
            diff_result_2 = diff_files(
                os.path.join(tmp_dir, "PROJ_UNITS"), loc_proj_units
            )

        if diff_result_1 or diff_result_2:
            if flags["o"]:
                grass.warning(_("Projection information does not match. Proceeding..."))
            else:
                if diff_result_1:
                    grass.warning(
                        _(
                            "Difference between PROJ_INFO file of packed map "
                            "and of current project:\n{diff}"
                        ).format(diff="".join(diff_result_1))
                    )
                if diff_result_2:
                    grass.warning(
                        _(
                            "Difference between PROJ_UNITS file of packed map "
                            "and of current project:\n{diff}"
                        ).format(diff="".join(diff_result_2))
                    )
                grass.fatal(
                    _(
                        "Coordinate reference system of dataset does not"
                        " appear to match current project."
                        " In case of no significant differences in the CRS definitions,"
                        " use the -o flag to ignore them and use"
                        " current project definition."
                    )
                )

    # new db
    fromdb = os.path.join(tmp_dir, "db.sqlite")
    # copy file
    shutil.copytree(data_name, new_dir)
    # exist fromdb
    if os.path.exists(fromdb):
        # the db connection in the output mapset
        dbconn = grassdb.db_connection(force=True)
        todb = dbconn["database"]

        # return the list of old connection for extract layer number and key
        dbln = open(os.path.join(new_dir, "dbln"))
        dbnlist = dbln.readlines()
        dbln.close()
        # check if dbf or sqlite directory exists
        if dbconn["driver"] == "dbf" and not os.path.exists(
            os.path.join(mset_dir, "dbf")
        ):
            os.mkdir(os.path.join(mset_dir, "dbf"))
        elif dbconn["driver"] == "sqlite" and not os.path.exists(
            os.path.join(mset_dir, "sqlite")
        ):
            os.mkdir(os.path.join(mset_dir, "sqlite"))
        # for each old connection
        for t in dbnlist:
            # it split the line of each connection, to found layer number and key
            values = t.split("|") if len(t.split("|")) != 1 else t.split(" ")

            from_table = values[1]
            layer = values[0].split("/")[0]
            # we need to take care about the table name in case of several layer
            if options["output"]:
                to_table = "%s_%s" % (map_name, layer) if len(dbnlist) > 1 else map_name
            else:
                to_table = from_table

            grass.verbose(_("Coping table <%s> as table <%s>") % (from_table, to_table))

            # copy the table in the default database
            try:
                grass.run_command(
                    "db.copy",
                    to_driver=dbconn["driver"],
                    to_database=todb,
                    to_table=to_table,
                    from_driver="sqlite",
                    from_database=fromdb,
                    from_table=from_table,
                )
            except CalledModuleError:
                grass.fatal(
                    _("Unable to copy table <%s> as table <%s>")
                    % (from_table, to_table)
                )

            grass.verbose(
                _("Connect table <%s> to vector map <%s> at layer <%s>")
                % (to_table, map_name, layer)
            )

            # and connect the new tables with the right layer
            try:
                grass.run_command(
                    "v.db.connect",
                    overwrite=True,
                    quiet=True,
                    driver=dbconn["driver"],
                    database=todb,
                    map=map_name,
                    key=values[2],
                    layer=layer,
                    table=to_table,
                )
            except CalledModuleError:
                grass.fatal(
                    _("Unable to connect table <%s> to vector map <%s>")
                    % (to_table, map_name)
                )

    grass.message(_("Vector map <%s> successfully unpacked") % map_name)


if __name__ == "__main__":
    options, flags = grass.parser()
    atexit.register(cleanup)
    sys.exit(main())
