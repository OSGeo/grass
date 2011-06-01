"""!
@package gpyshell.py

@brief wxGUI Interactive Python Shell

Classes:
 - PyShellWindow

@todo run pyshell and evaluate code in a separate instance of python
& design the widget communicate back and forth with it

(C) 2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys

import wx
from wx.py.shell import Shell as PyShell
from wx.py.version import VERSION

import grass.script as grass

class PyShellWindow(wx.Panel):
    """!Python Shell Window"""
    def __init__(self, parent, id = wx.ID_ANY, **kwargs):
        self.parent = parent
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self.intro = _("Welcome to wxGUI Interactive Python Shell %s") % VERSION + "\n\n" + \
            _("Type %s for more GRASS scripting related information.") % "\"help(grass)\"" + "\n\n"
        self.shell = PyShell(parent = self, id = wx.ID_ANY,
                             introText = self.intro, locals = {'grass' : grass})
        
        sys.displayhook = self._displayhook
        
        self.btnClear = wx.Button(self, wx.ID_CLEAR)
        self.btnClear.Bind(wx.EVT_BUTTON, self.OnClear)
        self.btnClear.SetToolTipString(_("Delete all text from the shell"))
                
        self._layout()
        
    def _displayhook(self, value):
        print value # do not modify __builtin__._
        
    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        sizer.Add(item = self.shell, proportion = 1,
                  flag = wx.EXPAND)
        
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = self.btnClear, proportion = 0,
                     flag = wx.EXPAND | wx.RIGHT, border = 5)
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        sizer.Fit(self)
        sizer.SetSizeHints(self)
        
        self.SetSizer(sizer)
        
        self.Fit()
        self.SetAutoLayout(True)        
        self.Layout()

    def OnClear(self, event):
        """!Delete all text from the shell
        """
        self.shell.clear()
        self.shell.showIntro(self.intro)
        self.shell.prompt()
