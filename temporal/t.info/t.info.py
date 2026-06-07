#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.info
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Print information about a space-time dataset
# COPYRIGHT:    (C) 2011-2026 by the GRASS Development Team
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

# %option G_OPT_F_FORMAT
# % options: plain,json,shell
# % descriptions: plain;Human readable text output;shell;Shell script style text output;json;JSON (JavaScript Object Notation)
# % guisection: Print
# %end

# %flag
# % key: g
# % description: This flag is deprecated and will be removed in a future release. Use format=shell instead.
# % label: Print in shell script style (deprecated)
# %end

# %flag
# % key: h
# % description: Print history information in human readable or shell style for space time datasets
# %end

# %flag
# % key: d
# % description: Print information about the temporal DBMI interface and exit
# % suppress_required: yes
# %end

import json

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis
    from grass.temporal.utils import TemporalJSONEncoder

    name = options["input"]
    type_ = options["type"]
    format_ = options["format"]
    shellstyle = flags["g"] or format_ == "shell"
    system = flags["d"]
    history = flags["h"]

    if shellstyle:
        gs.warning(
            _(
                "Flag 'g' is deprecated and will be removed in a future "
                "release. Please use format=shell instead.",
            ),
        )

    # Make sure the temporal database exists
    tgis.init()

    dbif, connection_state_changed = tgis.init_dbif(None)

    rows = tgis.get_tgis_metadata(dbif)

    if system and not history and format_ != "json":
        if shellstyle:
            print(
                f"dbmi_python_interface='{dbif.get_dbmi().__name__}'\n"
                f"dbmi_string='{tgis.get_tgis_database_string()}'\n"
                f"sql_template_path='{tgis.get_sql_template_path()}'",
            )
            if rows:
                for row in rows:
                    print(f"{row[0]}={row[1]}")
            return

        print(
            " +------------------- Temporal DBMI backend information"
            " ----------------------+\n"
            f" | DBMI Python interface:...... {dbif.get_dbmi().__name__}\n"
            f" | Temporal database string:... {tgis.get_tgis_database_string()}\n"
            f" | SQL template path:.......... {tgis.get_sql_template_path()}\n",
        )
        if rows:
            for row in rows:
                print(f" | {row[0]} .......... {row[1]}")
        print(
            " +-----------------------------------------------------"
            "-----------------------+",
        )
        return

    if not system and not name:
        gs.fatal(_("Please specify %s=") % ("name"))

    id_ = name if "@" in name else f"{name}@{gs.gisenv()['MAPSET']}"
    dataset = tgis.dataset_factory(type_, id_)

    if not dataset.is_in_db(dbif):
        gs.fatal(
            _("Dataset <{n}> of type <{t}> not found in temporal database").format(
                n=id_,
                t=type_,
            ),
        )

    dataset.select(dbif)

    if history and type_ in {"strds", "stvds", "str3ds"} and format_ != "json":
        dataset.print_history()
        return

    if format_ == "json":
        metadata = dataset.get_metadata_dict()
        metadata.update(
            {
                "dbmi_python_interface": dbif.get_dbmi().__name__,
                "dbmi_string": tgis.get_tgis_database_string(),
                "sql_template_path": tgis.get_sql_template_path(),
            }
        )
        if rows:
            metadata["tgis_db"] = {
                row[0]: int(row[1]) if row[1].isdigit() else row[1] for row in rows
            }
        print(json.dumps(metadata, cls=TemporalJSONEncoder, indent=4))

    elif format_ == "shell" or shellstyle:
        dataset.print_shell_info()

    else:
        dataset.print_info()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
