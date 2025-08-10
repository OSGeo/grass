"""
@package gmodeler.toolbars

@brief wxGUI Graphical Modeler toolbars classes

Classes:
 - toolbars::ModelerToolbar

(C) 2010-2023 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import sys

import wx

from core.globalvar import CheckWxVersion
from gui_core.toolbars import BaseToolbar, BaseIcons

from icons.icon import MetaIcon


class ModelerToolbar(BaseToolbar):
    """Graphical modeler toolbar (see gmodeler.py)"""

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
            "new": MetaIcon(
                img="create",
                label=_("Create new model") + " (Ctrl+Alt+N)",
            ),
            "open": MetaIcon(
                img="open",
                label=_("Load model from file") + " (Ctrl+Alt+O)",
            ),
            "save": MetaIcon(
                img="save",
                label=_("Save current model to file") + " (Ctrl+Alt+S)",
            ),
            "toImage": MetaIcon(img="image-export", label=_("Export model to image")),
            "toPython": MetaIcon(
                img="python-export",
                label=_("Export model to Python script") + " (Ctrl+Alt+P)",
            ),
            "actionAdd": MetaIcon(
                img="module-add",
                label=_("Add GRASS tool (module) to model") + " (Ctrl+Alt+A)",
            ),
            "dataAdd": MetaIcon(
                img="data-add", label=_("Add data to model") + " (Ctrl+Alt+D)"
            ),
            "relation": MetaIcon(
                img="relation-create",
                label=_("Manually define relation between data and commands"),
            ),
            "loop": MetaIcon(
                img="loop-add", label=_("Add loop/series to model") + " (Ctrl+Alt+L)"
            ),
            "comment": MetaIcon(
                img="label-add", label=_("Add comment to model") + " (Ctrl+Alt+#)"
            ),
            "run": MetaIcon(img="execute", label=_("Run model") + " (Ctrl+Alt+R)"),
            "validate": MetaIcon(img="check", label=_("Validate model")),
            "settings": MetaIcon(img="modeler-settings", label=_("Modeler settings")),
            "properties": MetaIcon(img="options", label=_("Set model properties")),
            "variables": MetaIcon(
                img="modeler-variables", label=_("Manage model variables")
            ),
            "redraw": MetaIcon(img="redraw", label=_("Redraw model canvas")),
            "help": BaseIcons["help"],
            "quit": BaseIcons["quit"],
        }

        data = (
            (
                ("new", icons["new"].label.rsplit(" ", 1)[0]),
                icons["new"],
                self.parent.OnModelNew,
            ),
            (
                ("open", icons["open"].label.rsplit(" ", 1)[0]),
                icons["open"],
                self.parent.OnModelOpen,
            ),
            (
                ("save", icons["save"].label.rsplit(" ", 1)[0]),
                icons["save"],
                self.parent.OnModelSave,
            ),
            (
                ("image", icons["toImage"].label.rsplit(" ", 1)[0]),
                icons["toImage"],
                self.parent.OnExportImage,
            ),
            (
                ("python", icons["toPython"].label),
                icons["toPython"],
                self.parent.OnExportPython,
            ),
            (None,),
            (
                ("action", icons["actionAdd"].label),
                icons["actionAdd"],
                self.parent.OnAddAction,
            ),
            (
                ("data", icons["dataAdd"].label),
                icons["dataAdd"],
                self.parent.OnAddData,
            ),
            (
                ("relation", icons["relation"].label),
                icons["relation"],
                self.parent.OnDefineRelation,
            ),
            (
                ("loop", icons["loop"].label),
                icons["loop"],
                self.parent.OnDefineLoop,
            ),
            (
                ("comment", icons["comment"].label),
                icons["comment"],
                self.parent.OnAddComment,
            ),
            (None,),
            (
                ("redraw", icons["redraw"].label),
                icons["redraw"],
                self.parent.OnCanvasRefresh,
            ),
            (
                ("validate", icons["validate"].label),
                icons["validate"],
                self.parent.OnValidateModel,
            ),
            (
                ("run", icons["run"].label),
                icons["run"],
                self.parent.OnRunModel,
            ),
            (None,),
            (
                ("variables", icons["variables"].label),
                icons["variables"],
                self.parent.OnVariables,
            ),
            (
                ("properties", icons["properties"].label),
                icons["properties"],
                self.parent.OnModelProperties,
            ),
            (None,),
            (
                ("settings", icons["settings"].label),
                icons["settings"],
                self.parent.OnPreferences,
            ),
            (
                ("help", icons["help"].label),
                icons["help"],
                self.parent.OnHelp,
            ),
            (None,),
        )
        if self.parent.IsDockable():
            data += (
                (
                    ("docking", BaseIcons["docking"].label),
                    BaseIcons["docking"],
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
