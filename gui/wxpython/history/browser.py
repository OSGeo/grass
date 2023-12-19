"""
@package history.browser

@brief History browser

Classes:
 - browser::HistoryBrowser

(C) 2023 by Linda Karlovska, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
"""

import wx
import re

from core import globalvar
from core.gcmd import GError, GException
from gui_core.forms import GUI
from gui_core.treeview import CTreeView
from history.tree import HistoryBrowserTree

from grass.pydispatch.signal import Signal


class HistoryBrowser(wx.Panel):
    """History browser for executing the commands from history log.

    Signal:
        showNotification - attribute 'message'
    """

    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        title=_("History browser"),
        name="history",
        **kwargs,
    ):
        self.parent = parent
        self._giface = giface

        self.showNotification = Signal("HistoryBrowser.showNotification")
        self.runIgnoredCmdPattern = Signal("HistoryBrowser.runIgnoredCmdPattern")
        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)

        self._createTree()

        self._giface.currentMapsetChanged.connect(self.UpdateHistoryModelFromScratch)
        self._giface.updateHistory.connect(
            lambda cmd: self.UpdateHistoryModelByCommand(cmd)
        )

        self._layout()

    def _layout(self):
        """Dialog layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(
            self._tree, proportion=1, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5
        )

        self.SetSizerAndFit(sizer)
        self.SetAutoLayout(True)
        self.Layout()

    def _createTree(self):
        """Create tree based on the model"""
        self._model = HistoryBrowserTree()
        self._tree = self._getTreeInstance()
        self._tree.SetToolTip(_("Double-click to open the tool"))
        self._tree.selectionChanged.connect(self.OnItemSelected)
        self._tree.itemActivated.connect(lambda node: self.Run(node))

    def _getTreeInstance(self):
        return CTreeView(model=self._model.GetModel(), parent=self)

    def _getSelectedNode(self):
        selection = self._tree.GetSelected()
        if not selection:
            return None
        return selection[0]

    def _refreshTree(self):
        self._tree.SetModel(self._model.GetModel())

    def UpdateHistoryModelFromScratch(self):
        """Update the model from scratch and refresh the tree"""
        self._model.CreateModel()
        self._refreshTree()

    def UpdateHistoryModelByCommand(self, cmd):
        """Update the model by the command and refresh the tree"""
        self._model.UpdateModel(cmd)
        self._refreshTree()

    def OnItemSelected(self, node):
        """Item selected"""
        command = node.data["command"]
        self.showNotification.emit(message=command)

    def Run(self, node=None):
        """Parse selected history command into list and launch module dialog."""
        node = node or self._getSelectedNode()
        if node:
            command = node.data["command"]
            lst = re.split(r"\s+", command)
            if (
                globalvar.ignoredCmdPattern
                and re.compile(globalvar.ignoredCmdPattern).search(command)
                and "--help" not in command
                and "--ui" not in command
            ):
                self.runIgnoredCmdPattern.emit(cmd=lst)
                return
            try:
                GUI(parent=self, giface=self._giface).ParseCommand(lst)
            except GException as e:
                GError(
                    parent=self,
                    message=str(e),
                    caption=_("Cannot be parsed into command"),
                    showTraceback=False,
                )
