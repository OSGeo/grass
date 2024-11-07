#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.info
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Print information about a space-time dataset
# COPYRIGHT:    (C) 2011-2017 by the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#############################################################################

# %module
# % description: Lists information about space time datasets and maps.
# % keyword: temporal
# % keyword: metadata
# % keyword: extent
# % keyword: time
# %end

# %option G_OPT_STDS_INPUT
# % description: Name of an existing space time dataset or map
# %end

# %option G_OPT_STDS_TYPE
# % guidependency: input
# % guisection: Required
# % options: strds, str3ds, stvds, raster, raster_3d, vector
# %end

# %flag
# % key: g
# % description: Print in shell script style
# %end

# %flag
# % key: h
# % description: Print history information in human readable shell style for space time datasets
# %end

# %flag
# % key: d
# % description: Print information about the temporal DBMI interface and exit
# % suppress_required: yes
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    name = options["input"]
    type_ = options["type"]
    shellstyle = flags["g"]
    system = flags["d"]
    history = flags["h"]

    # Make sure the temporal database exists
    tgis.init()

    dbif, connection_state_changed = tgis.init_dbif(None)

    rows = tgis.get_tgis_metadata(dbif)

    if system and not shellstyle and not history:
        #      0123456789012345678901234567890
        print(
            " +------------------- Temporal DBMI backend information ----------------------+"  # noqa: E501
        )
        print(" | DBMI Python interface:...... " + str(dbif.get_dbmi().__name__))
        print(" | Temporal database string:... " + str(tgis.get_tgis_database_string()))
        print(" | SQL template path:.......... " + str(tgis.get_sql_template_path()))
        if rows:
            for row in rows:
                print(" | %s .......... %s" % (row[0], row[1]))
        print(
            " +----------------------------------------------------------------------------+"  # noqa: E501
        )
        return
    if system and not history:
        print("dbmi_python_interface='" + str(dbif.get_dbmi().__name__) + "'")
        print("dbmi_string='" + str(tgis.get_tgis_database_string()) + "'")
        print("sql_template_path='" + str(tgis.get_sql_template_path()) + "'")
        if rows:
            for row in rows:
                print("%s='%s'" % (row[0], row[1]))
        return

    if not system and not name:
        gs.fatal(_("Please specify %s=") % ("name"))

    id_ = name if name.find("@") >= 0 else name + "@" + gs.gisenv()["MAPSET"]
    dataset = tgis.dataset_factory(type_, id_)

    if not dataset.is_in_db(dbif):
        gs.fatal(
            _("Dataset <{n}> of type <{t}> not found in temporal database").format(
                n=id_, t=type_
            )
        )

    dataset.select(dbif)

    if history and type_ in {"strds", "stvds", "str3ds"}:
        dataset.print_history()
        return

    if shellstyle:
        dataset.print_shell_info()
    else:
        dataset.print_info()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
