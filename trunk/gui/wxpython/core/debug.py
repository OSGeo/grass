"""
@package core.debug

@brief wxGUI debugging

Classes:
 - debug::DebugMsg

@code
from core.debug import Debug
Debug.msg (3, 'debug message')
@endcode

(C) 2007-2009, 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys

import grass.script as grass


class DebugMsg:
    """wxGUI debugging

        g.gisenv set=WX_DEBUG=[0-5]

    """

    def __init__(self):
        # default level
        self.debuglevel = 0

        self.SetLevel()

    def SetLevel(self):
        """Initialize gui debug level
        """
        try:
            self.debuglevel = int(grass.gisenv().get('WX_DEBUG', 0))
            if self.debuglevel < 0 or self.debuglevel > 5:
                raise ValueError(
                    _("Wx debug level {0}.").format(
                        self.debuglevel))
        except ValueError as e:
            self.debuglevel = 0
            sys.stderr.write(
                _("WARNING: Ignoring unsupported wx debug level (must be >=0 and <=5). {0}\n").format(e))

    def msg(self, level, message, *args):
        """Print debug message

        :param level: debug level (0-5)
        :param message: message to be printed
        :param args: formatting params
        """
        # self.SetLevel()
        if self.debuglevel > 0 and level > 0 and level <= self.debuglevel:
            if args:
                sys.stderr.write("GUI D%d/%d: " % (level, self.debuglevel) +
                                 message % args + os.linesep)
            else:
                sys.stderr.write("GUI D%d/%d: " % (level, self.debuglevel) +
                                 message + os.linesep)
            sys.stderr.flush()  # force flush (required for MS Windows)

    def GetLevel(self):
        """Return current GUI debug level"""
        return self.debuglevel

# Debug instance
Debug = DebugMsg()
