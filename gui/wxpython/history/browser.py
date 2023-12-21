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
from gui_core.wrap import Menu
from history.tree import HistoryBrowserTree

from grass.grassdb.history import get_current_mapset_gui_history_path
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
        self._selected_cmd_node = None

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
        self._tree.contextMenu.connect(self.OnRightClick)

    def _confirmDialog(self, question, title):
        """Confirm dialog"""
        dlg = wx.MessageDialog(self, question, title, wx.YES_NO)
        res = dlg.ShowModal()
        dlg.Destroy()
        return res

    def _getTreeInstance(self):
        return CTreeView(model=self._model.GetModel(), parent=self)

    def _getSelectedNode(self):
        selection = self._tree.GetSelected()
        if not selection:
            return None
        return selection[0]

    def _popupMenuLayer(self):
        """Create popup menu for commands"""
        menu = Menu()

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Delete"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnDeleteCmd, item)

        self.PopupMenu(menu)
        menu.Destroy()

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

    def OnDeleteCmd(self, event):
        """Delete cmd from the history file"""
        cmd = self._selected_cmd_node.data["command"]
        question = _("Do you really want to delete <{}> command?").format(cmd)
        if self._confirmDialog(question, title=_("Delete command")) == wx.ID_YES:
            history_path = get_current_mapset_gui_history_path()
            if history_path:
                del_line_number = self._model.model.GetIndexOfNode(
                    self._selected_cmd_node,
                )[0]
                self.showNotification.emit(message=_("Deleting <{}>").format(cmd))
                try:
                    with open(history_path, "r+") as f:
                        lines = f.readlines()
                        f.seek(0)
                        f.truncate()
                        for number, line in enumerate(lines):
                            if number not in [del_line_number]:
                                f.write(line)
                except OSError as e:
                    GError(
                        parent=self,
                        message=str(e),
                        caption=_("Cannot update history file"),
                        showTraceback=False,
                    )
                    return
            self._model.CreateModel()
            self._refreshTree()
            self.showNotification.emit(message=_("<{}> deleted").format(cmd))

    def OnItemSelected(self, node):
        """Item selected"""
        command = node.data["command"]
        self.showNotification.emit(message=command)

    def OnRightClick(self, node):
        """Display popup menu"""
        self._selected_cmd_node = node
        self._popupMenuLayer()

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
