"""
@package lmgr.pyshell

@brief wxGUI Interactive Python Shell for Layer Manager

Classes:
 - pyshell::PyShellWindow

.. todo::
    Run pyshell and evaluate code in a separate instance of python &
    design the widget communicate back and forth with it

(C) 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

from __future__ import print_function

import sys

import wx
from wx.py.shell import Shell as PyShell
from wx.py.version import VERSION

import grass.script as grass

from gui_core.wrap import Button, ClearButton, IsDark
from gui_core.pystc import SetDarkMode
from core.globalvar import CheckWxVersion


class PyShellWindow(wx.Panel):
    """Python Shell Window"""

    def __init__(
        self, parent, giface, id=wx.ID_ANY, simpleEditorHandler=None, **kwargs
    ):
        self.parent = parent
        self.giface = giface

        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)

        self.intro = (
            _("Welcome to wxGUI Interactive Python Shell %s") % VERSION
            + "\n\n"
            + _("Type %s for more GRASS scripting related information.") % '"help(gs)"'
            + "\n"
            + _("Type %s to add raster or vector to the layer tree.") % '"AddLayer()"'
            + "\n\n"
        )

        shellargs = dict(
            parent=self,
            id=wx.ID_ANY,
            introText=self.intro,
            locals={"gs": grass, "AddLayer": self.AddLayer},
        )
        # useStockId (available since wxPython 4.0.2) should be False on macOS
        if sys.platform == "darwin" and CheckWxVersion([4, 0, 2]):
            shellargs["useStockId"] = False
        self.shell = PyShell(**shellargs)
        if IsDark():
            SetDarkMode(self.shell)

        sys.displayhook = self._displayhook

        self.btnClear = ClearButton(self)
        self.btnClear.Bind(wx.EVT_BUTTON, self.OnClear)
        self.btnClear.SetToolTip(_("Delete all text from the shell"))

        self.simpleEditorHandler = simpleEditorHandler
        if simpleEditorHandler:
            self.btnSimpleEditor = Button(self, id=wx.ID_ANY, label=_("Simple &editor"))
            self.btnSimpleEditor.Bind(wx.EVT_BUTTON, simpleEditorHandler)
            self.btnSimpleEditor.SetToolTip(_("Open a simple Python code editor"))

        self._layout()

    def _displayhook(self, value):
        print(value)  # do not modify __builtin__._

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)

        sizer.Add(self.shell, proportion=1, flag=wx.EXPAND)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        if self.simpleEditorHandler:
            btnSizer.Add(
                self.btnSimpleEditor,
                proportion=0,
                flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
                border=5,
            )
        btnSizer.AddStretchSpacer()
        btnSizer.Add(self.btnClear, proportion=0, flag=wx.EXPAND, border=5)
        sizer.Add(btnSizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)

        sizer.Fit(self)
        sizer.SetSizeHints(self)

        self.SetSizer(sizer)

        self.Fit()
        self.SetAutoLayout(True)
        self.Layout()

    def AddLayer(self, name, ltype="auto"):
        """Add selected map to the layer tree

        :param name: name of raster/vector map to be added
        :param type: map type ('raster', 'vector', 'auto' for autodetection)
        """
        fname = None
        if ltype == "raster" or ltype != "vector":
            # check for raster
            fname = grass.find_file(name, element="cell")["fullname"]
            if fname:
                ltype = "raster"
                lcmd = "d.rast"

        if not fname and (ltype == "vector" or ltype != "raster"):
            # if not found check for vector
            fname = grass.find_file(name, element="vector")["fullname"]
            if fname:
                ltype = "vector"
                lcmd = "d.vect"

        if not fname:
            return _("Raster or vector map <%s> not found") % (name)

        self.giface.GetLayerTree().AddLayer(
            ltype=ltype, lname=fname, lchecked=True, lcmd=[lcmd, "map=%s" % fname]
        )
        if ltype == "raster":
            return _("Raster map <%s> added") % fname

        return _("Vector map <%s> added") % fname

    def OnClear(self, event):
        """Delete all text from the shell"""
        self.shell.clear()
        self.shell.showIntro(self.intro)
        self.shell.prompt()
