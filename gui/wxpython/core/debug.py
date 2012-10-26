"""!
@package core.debug

@brief wxGUI debugging

Classes:
 - debug::DebugMsg

@code
from core.debug import Debug
Debug.msg (3, 'debug message')
@endcode
         
(C) 2007-2009, 2011-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys

class DebugMsg:
    """!wxGUI debugging
    
    @code
    g.gisenv set=WX_DEBUG=[0-5]
    @endcode
    """
    def __init__(self):
        self.SetLevel() # -> default debug level

    def SetLevel(self):
        """!Initialize gui debug level
        """
        try:
            import grass.script as grass
        except ImportError:
            self.debuglevel = 0
            return
        
        self.debuglevel = int(grass.gisenv().get('WX_DEBUG', 0))
        
    def msg(self, level, message, *args):
        """!Print debug message

        @param level debug level (0-5)
        @param message message to be printed
        @param *args formatting params
        """
        # self.SetLevel()
        if self.debuglevel > 0 and level > 0 and level <= self.debuglevel:
            if args:
                sys.stderr.write("GUI D%d/%d: " % (level, self.debuglevel) + \
                    message % args + os.linesep)
            else:
                sys.stderr.write("GUI D%d/%d: " % (level, self.debuglevel) + \
                                     message + os.linesep)
            sys.stderr.flush() # force flush (required for MS Windows)
        
    def GetLevel(self):
        """!Return current GUI debug level"""
        return self.debuglevel

# Debug instance
Debug = DebugMsg()
