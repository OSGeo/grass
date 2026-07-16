#!/usr/bin/env python3

############################################################################
#
# MODULE:       t.shift
# AUTHOR(S):    Soeren Gebbert
#
# PURPOSE:      Temporally shift the maps of a space time dataset
# SPDX-FileCopyrightText: 2013-2017 Other GRASS authors
# SPDX-License-Identifier: GPL-2.0-or-later
#############################################################################

# %module
# % description: Shifts temporally the maps of a space time dataset.
# % keyword: temporal
# % keyword: time management
# % keyword: shift
# % keyword: time
# %end

# %option G_OPT_STDS_INPUT
# % description: Name of an existing space time dataset
# %end

# %option G_OPT_STDS_TYPE
# % guidependency: input
# % guisection: Required
# %end

# %option
# % key: granularity
# % type: string
# % label: Shift granularity
# % description: Format absolute time: "x years, x months, x weeks, x days, x hours, x minutes, x seconds", relative time is of type integer
# % required: yes
# % multiple: no
# %end

import grass.script as gs

############################################################################


def main():
    # lazy imports
    import grass.temporal as tgis

    name = options["input"]
    type = options["type"]
    gran = options["granularity"]

    # Make sure the temporal database exists
    tgis.init()

    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()

    stds = tgis.open_old_stds(name, type, dbif)
    check = stds.shift(gran=gran, dbif=dbif)

    if check is False:
        dbif.close()
        gs.fatal(
            _("Unable to temporally shift the space time %s dataset <%s>")
            % (stds.get_new_map_instance(None).get_type(), id)
        )

    stds.update_command_string(dbif=dbif)
    dbif.close()


if __name__ == "__main__":
    options, flags = gs.parser()
    main()

