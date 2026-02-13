"""
@package jupyter_notebook.toolbars

@brief wxGUI Jupyter toolbars classes

Classes:
 - toolbars::JupyterToolbar

(C) 2026 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Karlovska <linda.karlovska seznam.cz>
"""

import sys

import wx

from core.globalvar import CheckWxVersion
from gui_core.toolbars import BaseToolbar, BaseIcons

from icons.icon import MetaIcon


class JupyterToolbar(BaseToolbar):
    """Jupyter toolbar"""

    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)

        # workaround for http://trac.wxwidgets.org/ticket/13888
        if sys.platform == "darwin" and not CheckWxVersion([4, 2, 1]):
            parent.SetToolBar(self)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        icons = {
            "create": MetaIcon(
                img="create",
                label=_("Create new notebook"),
            ),
            "open": MetaIcon(
                img="open",
                label=_("Import notebook"),
            ),
            "save": MetaIcon(
                img="save",
                label=_("Export notebook"),
            ),
            "docking": BaseIcons["docking"],
            "quit": BaseIcons["quit"],
        }
        data = (
            (
                ("create", icons["create"].label.rsplit(" ", 1)[0]),
                icons["create"],
                self.parent.OnCreate,
            ),
            (
                ("open", icons["open"].label.rsplit(" ", 1)[0]),
                icons["open"],
                self.parent.OnImport,
            ),
            (
                ("save", icons["save"].label.rsplit(" ", 1)[0]),
                icons["save"],
                self.parent.OnExport,
            ),
            (None,),
        )
        if self.parent.IsDockable():
            data += (
                (
                    ("docking", icons["docking"].label),
                    icons["docking"],
                    self.parent.OnDockUndock,
                    wx.ITEM_CHECK,
                ),
            )
        data += (
            (
                ("quit", icons["quit"].label),
                icons["quit"],
                self.parent.OnCloseWindow,
            ),
        )

        return self._getToolbarData(data)
