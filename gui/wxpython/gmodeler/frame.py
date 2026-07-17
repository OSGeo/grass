"""
@package gmodeler.frame

@brief wxGUI Graphical Modeler for creating, editing, and managing models

Classes:
 - frame::ModelerFrame

SPDX-FileCopyrightText: 2010-2023 Other GRASS authors
SPDX-License-Identifier: GPL-2.0-or-later

@author Martin Landa <landa.martin gmail.com>
@author Python exports Ondrej Pesek <pesej.ondrek gmail.com>
"""

import os

import wx

from core import globalvar
from gui_core.menu import Menu as Menubar

from gmodeler.menudata import ModelerMenuData
from gmodeler.panels import ModelerPanel


class ModelerFrame(wx.Frame):
    def __init__(
        self, parent, giface, id=wx.ID_ANY, title=_("Graphical Modeler"), **kwargs
    ):
        """Graphical modeler main window
        :param parent: parent window
        :param giface: GRASS interface
        :param id: window id
        :param title: window title

        :param kwargs: wx.Frames' arguments
        """
        wx.Frame.__init__(self, parent=parent, id=id, title=title, **kwargs)

        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )

        self.statusbar = self.CreateStatusBar(number=1)
        self.panel = ModelerPanel(parent=self, giface=giface, statusbar=self.statusbar)

        self.menubar = Menubar(
            parent=self,
            model=ModelerMenuData().GetModel(separators=True),
            class_handler=self.panel,
        )
        self.SetMenuBar(self.menubar)

        self.SetName("ModelerFrame")
        self.SetMinSize((640, 300))
        self.SetSize((800, 600))
