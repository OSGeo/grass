#!/usr/bin/env python3

############################################################################
#
# MODULE:       v.db.droprow
# AUTHOR(S):    Markus Neteler
#               Pythonized by Martin Landa
# PURPOSE:      Interface to v.extract -r to drop ...
# SPDX-FileCopyrightText: 2009 by the GRASS Development Team
#
# SPDX-License-Identifier: GPL-2.0-or-later.
#
#############################################################################


# %module
# % description: Removes a vector feature from a vector map through attribute selection.
# % keyword: vector
# % keyword: attribute table
# % keyword: database
# %end

# %option G_OPT_V_INPUT
# %end

# %option G_OPT_V_FIELD
# %end

# %option G_OPT_DB_WHERE
# % required :yes
# %end

# %option G_OPT_V_OUTPUT
# %end

import sys
import grass.script as gs
from grass.exceptions import CalledModuleError


def main():
    # delete vectors via reverse selection
    try:
        gs.run_command(
            "v.extract",
            flags="r",
            input=options["input"],
            layer=options["layer"],
            output=options["output"],
            where=options["where"],
        )
    except CalledModuleError:
        return 1

    # write cmd history:
    gs.vector_history(map=options["output"])

    return 0


if __name__ == "__main__":
    options, flags = gs.parser()
    sys.exit(main())
