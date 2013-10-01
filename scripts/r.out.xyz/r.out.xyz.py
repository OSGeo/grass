#!/usr/bin/env python
############################################################################
#
# MODULE:       r.out.xyz
# AUTHOR:       M. Hamish Bowman, Dept. Marine Science, Otago Univeristy,
#                 New Zealand
#		Converted to Python by Glynn Clements
# PURPOSE:      Export a raster map as x,y,z values based on cell centers
#               This is a simple wrapper script for "r.stats -1ng"
#
# COPYRIGHT:    (c) 2006 Hamish Bowman, and the GRASS Development Team
#		(c) 2008 Glynn Clements, and the GRASS Development Team
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

#%module
#% description: Exports a raster map to a text file as x,y,z values based on cell centers.
#% keywords: raster
#% keywords: export
#% keywords: ASCII
#% keywords: conversion
#%end
#%option G_OPT_R_INPUT
#%end
#%option G_OPT_F_OUTPUT
#% description: Name for output file (if omitted or "-" output to stdout)
#% required: no
#%end
#%option G_OPT_F_SEP
#%end

import sys
from grass.script import core as grass


def main():
    # if no output filename, output to stdout
    output = options['output']
    sep = options['separator']

    parameters = dict(flags="1gn",
                      input=options['input'])
    if output:
        parameters.update(output=output)
    # windows don't like pipe so we don't include it in the command explicitly
    if sep != '|':
        parameters.update(separator=sep)

    ret = grass.run_command("r.stats", **parameters)
    sys.exit(ret)

if __name__ == "__main__":
    options, flags = grass.parser()
    main()
