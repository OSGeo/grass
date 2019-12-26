#!/usr/bin/env python3
############################################################################
#
# MODULE:       r.out.xyz
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago University,
#                 New Zealand
#               Converted to Python by Glynn Clements
# PURPOSE:      Export a raster map as x,y,z values based on cell centers
#               This is a simple wrapper script for "r.stats -1ng"
#
# COPYRIGHT:    (c) 2006 Hamish Bowman, and the GRASS Development Team
#               (c) 2008 Glynn Clements, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Exports a raster map to a text file as x,y,z values based on cell centers.
#% keyword: raster
#% keyword: export
#% keyword: output
#% keyword: ASCII
#% keyword: conversion
#%end
#%option G_OPT_R_INPUTS
#% multiple: yes
#%end
#%option G_OPT_F_OUTPUT
#% description: Name for output file (if omitted or "-" output to stdout)
#% required: no
#%end
#%option G_OPT_F_SEP
#%end
#%flag
#% key: i
#% description: Include no data values
#%end

import sys
from grass.script import core as grass
from grass.exceptions import CalledModuleError


def main():
    # if no output filename, output to stdout
    output = options['output']
    donodata = flags['i']

    if donodata:
        statsflags="1g"
    else:
        statsflags="1gn"
    parameters = dict(flags=statsflags,
                      input=options['input'],
                      separator=options['separator'])
    if output:
        parameters.update(output=output)

    ret = 0
    try:
        grass.run_command("r.stats", **parameters)
    except CalledModuleError:
        ret = 1
    sys.exit(ret)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
