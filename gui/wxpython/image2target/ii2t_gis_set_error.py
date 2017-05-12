"""
@package gis_set_error

GRASS start-up screen error message.

(C) 2010-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys

from core import globalvar
import wx


def main():
    app = wx.App()

    if len(sys.argv) == 1:
        msg = "Unknown reason"
    else:
        msg = ''
        for m in sys.argv[1:]:
            msg += m

    wx.MessageBox(caption="Error",
                  message=msg,
                  style=wx.OK | wx.ICON_ERROR)

    app.MainLoop()

if __name__ == "__main__":
    main()
