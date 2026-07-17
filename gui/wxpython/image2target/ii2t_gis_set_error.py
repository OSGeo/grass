"""
@package gis_set_error

GRASS start-up screen error message.

SPDX-FileCopyrightText: 2010-2011 Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later

@author Martin Landa <landa.martin gmail.com>
"""

import sys

import wx


def main():
    app = wx.App()

    if len(sys.argv) == 1:
        msg = "Unknown reason"
    else:
        msg = ""
        for m in sys.argv[1:]:
            msg += m

    wx.MessageBox(caption="Error", message=msg, style=wx.OK | wx.ICON_ERROR)

    app.MainLoop()


if __name__ == "__main__":
    main()
