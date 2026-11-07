#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.info
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Print information about a space-time dataset
# SPDX-FileCopyrightText: 2011-2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later
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

# %rules
# % required: -d, input
# %end

import json

import grass.script as gs


def main():
    # lazy imports
    import grass.temporal as tgis
    from grass.temporal.utils import TemporalJSONEncoder

    name = options["input"]
    stds_type = options["type"]
    format_ = options["format"]
    shellstyle = flags["g"] or format_ == "shell"
    system = flags["d"]
    history = flags["h"]
    qualified = "@" in name
    mapset = name.split("@", 1)[1] if qualified else None

    if flags["g"]:
        gs.warning(
            _(
                "Flag 'g' is deprecated and will be removed in a future "
                "release. Please use format=shell instead.",
            ),
        )

    if format_ == "json" and (history or system):
        gs.warning(
            _(
                "Output from the 'd' and 'h' flag are always included "
                "with format=json.",
            ),
        )

    # Make sure the temporal database exists
    tgis.init(skip_db_init=True)

    dbif = tgis.SQLDatabaseInterfaceConnection(mapsets=mapset)

    if not dbif.tgis_mapsets:
        msg = _("No temporal database found in available mapsets.")
        if mapset:
            msg = _("No temporal database found in requested mapset <%s>.") % mapset
        gs.fatal(msg)

    # Connect once here, otherwise helpers close the connection they open
    dbif.connect()

    rows = tgis.get_tgis_metadata(dbif)

    if system and not history and format_ != "json":
        if shellstyle:
            print(
                f"dbmi_python_interface='{dbif.get_dbmi().__name__}'\n"
                f"dbmi_string='{next(iter(dbif.tgis_mapsets.values()))[1]}'\n"
                f"sql_template_path='{tgis.get_sql_template_path()}'",
            )
            if rows:
                for row in rows:
                    print(f"{row[0]}='{row[1]}'")
            return

        print(
            " +------------------- Temporal DBMI backend information"
            " ----------------------+\n"
            f" | DBMI Python interface:...... {dbif.get_dbmi().__name__}\n"
            f" | Temporal database string:... {next(iter(dbif.tgis_mapsets.values()))[1]}\n"
            f" | SQL template path:.......... {tgis.get_sql_template_path()}",
        )
        if rows:
            for row in rows:
                print(f" | {row[0]} .......... {row[1]}")
        print(
            " +-----------------------------------------------------"
            "-----------------------+",
        )
        return

    if qualified:
        dataset = tgis.dataset_factory(stds_type, name)
        if not dataset.is_in_db(dbif):
            gs.fatal(
                _("Dataset <{n}> of type <{t}> not found in temporal database").format(
                    n=name,
                    t=stds_type,
                ),
            )
    else:
        datasets = {}
        for tgis_mapset in dbif.tgis_mapsets:
            tgis_dataset = tgis.dataset_factory(stds_type, f"{name}@{tgis_mapset}")
            if tgis_dataset.is_in_db(dbif):
                datasets[tgis_mapset] = tgis_dataset
        if not datasets:
            gs.fatal(
                _(
                    "Dataset <{n}> of type <{t}> not found in any temporal database on the current search path."
                ).format(
                    n=name,
                    t=stds_type,
                ),
            )
        # The current mapset comes first on the search path
        used_mapset, dataset = next(iter(datasets.items()))
        if len(datasets) > 1:
            gs.warning(
                _(
                    "Dataset <{n}> of type <{t}> found in multiple temporal databases on the current search path. Using the one from mapset <{m}>."
                ).format(
                    n=name,
                    t=stds_type,
                    m=used_mapset,
                )
            )
    dataset.select(dbif)

    if history and stds_type in {"strds", "stvds", "str3ds"} and format_ != "json":
        dataset.print_history()
        return

    if format_ == "json":
        metadata = dataset.get_metadata_dict()
        metadata["tgis_db"] = {}
        metadata["tgis_db"].update(
            {
                "dbmi_python_interface": dbif.get_dbmi().__name__,
                "dbmi_string": dbif.tgis_mapsets[dataset.get_mapset()][1],
                "sql_template_path": tgis.get_sql_template_path(),
            }
        )
        if rows:
            metadata["tgis_db"].update(
                {row[0]: int(row[1]) if row[1].isdigit() else row[1] for row in rows}
            )
        print(json.dumps(metadata, cls=TemporalJSONEncoder, indent=4))

    elif format_ == "shell" or shellstyle:
        dataset.print_shell_info()

    else:
        dataset.print_info()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()
