#!/usr/bin/env python3

############################################################################
#
# MODULE:	d.redraw
# AUTHOR(S):	Martin Landa <landa.martin gmail.com>
# PURPOSE:	Redraws the content of currently selected monitor
# COPYRIGHT:	(C) 2011-2015 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Redraws the content of currently selected monitor.
#% keyword: display
#% keyword: graphics
#% keyword: monitors
#%end

import os
import sys

from grass.script import core as grass
from grass.script.utils import split


def main():
    mon = grass.gisenv().get('MONITOR', None)
    if not mon:
        grass.fatal(_("No graphics device selected. Use d.mon to select graphics device."))

    monCmd = grass.parse_command('d.mon', flags='g').get('cmd', None)
    if not monCmd or not os.path.isfile(monCmd):
        grass.fatal(_("Unable to open file '%s'") % monCmd)

    try:
        fd = open(monCmd, 'r')
        cmdList = fd.readlines()

        grass.run_command('d.erase')

        for cmd in cmdList:
            if cmd.startswith('#'):
                continue
            grass.call(split(cmd))
    except IOError as e:
        grass.fatal(_("Unable to open file '%s' for reading. Details: %s") %
                    (monCmd, e))

    fd.close()

    # restore cmd file
    try:
        fd = open(monCmd, "w")
        fd.writelines(cmdList)
    except IOError as e:
        grass.fatal(_("Unable to open file '%s' for writing. Details: %s") %
                    (monCmd, e))

    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
