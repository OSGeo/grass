#!/usr/bin/env python
############################################################################
#
# MODULE:	d.redraw
# AUTHOR(S):	Martin Landa <landa.martin gmail.com>
# PURPOSE:	Redraws the content of currently selected monitor
# COPYRIGHT:	(C) 2011 by the GRASS Development Team
#
#		This program is free software under the GNU General
#		Public License (>=v2). Read the file COPYING that
#		comes with GRASS for details.
#
#############################################################################

#%module
#% description: Redraws the content of currently selected monitor.
#% keywords: display
#% keywords: graphics
#% keywords: monitors
#%end

import sys
import shlex

from grass.script import core as grass

def split(s):
    """!Platform spefic shlex.split"""
    if sys.version_info >= (2, 6):
        return shlex.split(s, posix = (sys.platform != "win32"))
    elif sys.platform == "win32":
        return shlex.split(s.replace('\\', r'\\'))
    else:
        return shlex.split(s)

def main():
    env = grass.gisenv()
    mon = env.get('MONITOR', None)
    if not mon:
        grass.fatal(_("No monitor selected. Run `d.mon` to select monitor."))
    
    monCmd = env.get('MONITOR_%s_CMDFILE' % mon)
    if not monCmd:
        grass.fatal(_("No cmd file found for monitor <%s>") % mon)

    try:
        fd = open(monCmd, 'r')
        cmdList = fd.readlines()
        
        grass.run_command('d.erase')
        
        for cmd in cmdList:
            grass.call(split(cmd))
    except IOError, e:
        grass.fatal(_("Unable to open file '%s' for reading. Details: %s") % \
                        (monCmd, e))
    
    fd.close()
    
    # restore cmd file
    try:
        fd = open(monCmd, "w")
        fd.writelines(cmdList)
    except IOError, e:
        grass.fatal(_("Unable to open file '%s' for writing. Details: %s") % \
                        (monCmd, e))
    
    return 0

if __name__ == "__main__":
    options, flags = grass.parser()
    sys.exit(main())
