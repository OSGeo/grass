"""
MODULE: debug

CLASSES:
 * DebugMsg

PURPOSE: GRASS debugging

         from debug import Debug as Debug
         Debug.msg (3, 'debug message')
         
AUTHORS: The GRASS Development Team
         Martin Landa <landa.martin gmail.com>

COPYRIGHT: (C) 2007-2008 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import sys

import globalvar

class DebugMsg:
    """
    GRASS Debugging

    export GRASS_WX_DEBUG=[0-5]
    """
    def __init__(self):
        # default level
        self.debuglevel = 0
        # update level
        self._update_level()

    def _update_level(self):
        debug = os.getenv("GRASS_WX_DEBUG")
        if debug is not None:
            try:
                # only GUI debug messages [GUI:level]
                level = int (debug[-1])
            except:
                level = self.debuglevel
                
            if self.debuglevel != level:
                self.debuglevel = level

    def msg (self, level, message):
        self._update_level()
        if self.debuglevel > 0 and level > 0 and level <= self.debuglevel:
            print >> sys.stderr, "GUI D%d/%d: %s" % (level, self.debuglevel, message)

    def get_level(self):
        """Return current GUI debug level"""
        return self.debuglevel

# Debug instance
Debug = DebugMsg()

# testing
if __name__ == "__main__":
    import gcmd
    gcmd.Command (cmd=["g.gisenv", "set=DEBUG=3"])
                
    for level in range (4):
        Debug.msg (level, "message level=%d" % level)
