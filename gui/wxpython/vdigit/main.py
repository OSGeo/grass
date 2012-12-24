"""!
@package vdigit.main

@brief wxGUI vector digitizer

Classes:
 - main::VDigit

(C) 2007-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

try:
    from vdigit.wxdigit import IVDigit, GV_LINES, CFUNCTYPE
    haveVDigit = True
    errorMsg   = ''
except (ImportError, NameError), err:
    haveVDigit = False
    errorMsg   = err
    GV_LINES   = -1
    class IVDigit:
        def __init__(self):
            pass

class VDigit(IVDigit):
    def __init__(self, mapwindow):
        """!Base class of vector digitizer
        
        @param mapwindow reference to mapwindow (mapdisp_window.BufferedWindow) instance
        """
        IVDigit.__init__(self, mapwindow)
