"""!
@package help.py

@brief Help window

@todo Needs improvements...

Classes:
 - HelpWindow

(C) 2008-2009 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import wx

class HelpWindow(wx.Frame):
    """!GRASS Quickstart help window"""
    def __init__(self, parent, id, title, size, file):

        wx.Frame.__init__(self, parent=parent, id=id, title=title, size=size)

        sizer = wx.BoxSizer(wx.VERTICAL)

        # text
        helpFrame = wx.html.HtmlWindow(parent=self, id=wx.ID_ANY)
        helpFrame.SetStandardFonts (size = 10)
        helpFrame.SetBorders(10)
        wx.InitAllImageHandlers()

        helpFrame.LoadFile(file)
        self.Ok = True

        sizer.Add(item=helpFrame, proportion=1, flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        #        sizer.Fit(self)
        #        sizer.SetSizeHints(self)
        self.Layout()
