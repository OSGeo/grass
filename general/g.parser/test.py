#!/usr/bin/env python3

# g.parser demo script for python programming

# %module
# % description: g.parser test script (python)
# % keyword: keyword1
# % keyword: keyword2
# %end
# %flag
# % key: f
# % description: A flag
# %end
# %option G_OPT_R_MAP
# % key: raster
# % required: yes
# %end
# %option G_OPT_V_MAP
# % key: vector
# %end
# %option
# % key: option1
# % type: string
# % description: An option
# % required: no
# %end

import sys
import atexit

import grass.script as gs


def cleanup():
    # add some cleanup code
    gs.message(_("Inside cleanup function..."))


def main():
    flag_f = flags["f"]
    option1 = options["option1"]
    raster = options["raster"]
    vector = options["vector"]

    # Add your main code here

    exitcode = 0

    if flag_f:
        gs.message(_("Flag -f set"))
    else:
        gs.message(_("Flag -f not set"))

    # test if parameter present:
    if option1:
        gs.message(_("Value of option1 option: '%s'" % option1))

    gs.message(_("Value of raster option: '%s'" % raster))
    gs.message(_("Value of vector option: '%s'" % vector))

    # End of your main code here

    sys.exit(exitcode)


if __name__ == "__main__":
    options, flags = gs.parser()
    atexit.register(cleanup)
    main()
