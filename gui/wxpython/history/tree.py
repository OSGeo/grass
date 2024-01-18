"""
@package history.tree

@brief History browser tree classes

Classes:
 - history::HistoryBrowserTree

(C) 2023 by Linda Karlovska, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
@author Anna Petrasova (kratochanna gmail com)
@author Tomas Zigo
"""

import wx
import re
import copy

from core import globalvar

from core.gcmd import GError, GException
from core.utils import (
    parse_mapcalc_cmd,
    replace_module_cmd_special_flags,
    split,
)
from gui_core.forms import GUI
from core.treemodel import TreeModel, ModuleNode
from gui_core.treeview import CTreeView
from gui_core.wrap import Menu

from grass.pydispatch.signal import Signal

from grass.grassdb.history import (
    get_current_mapset_gui_history_path,
    read_history,
    remove_entry_from_history,
)


class HistoryBrowserTree(CTreeView):
    """Tree structure visualizing and managing history of executed commands.
    Uses virtual tree and model defined in core/treemodel.py.
    """

    def __init__(
        self,
        parent,
        model=None,
        giface=None,
        style=wx.TR_HIDE_ROOT
        | wx.TR_LINES_AT_ROOT
        | wx.TR_HAS_BUTTONS
        | wx.TR_FULL_ROW_HIGHLIGHT,
    ):
        """History Browser Tree constructor."""
        self._model = TreeModel(ModuleNode)
        self._orig_model = self._model
        super().__init__(parent=parent, model=self._model, id=wx.ID_ANY, style=style)

        self._giface = giface
        self.parent = parent

        self._initHistoryModel()

        self.showNotification = Signal("HistoryBrowserTree.showNotification")
        self.runIgnoredCmdPattern = Signal("HistoryBrowserTree.runIgnoredCmdPattern")

        self._giface.currentMapsetChanged.connect(self.UpdateHistoryModelFromScratch)
        self._giface.entryToHistoryAdded.connect(
            lambda cmd: self.UpdateHistoryModelByCommand(cmd)
        )

        self.SetToolTip(_("Double-click to open the tool"))
        self.selectionChanged.connect(self.OnItemSelected)
        self.itemActivated.connect(lambda node: self.Run(node))
        self.contextMenu.connect(self.OnRightClick)

    def _initHistoryModel(self):
        """Fill tree history model based on the current history log."""
        self._history_path = get_current_mapset_gui_history_path()
        if self._history_path:
            cmd_list = read_history(self._history_path)
            for label in cmd_list:
                data = {"command": label.strip()}
                self._model.AppendNode(
                    parent=self._model.root,
                    label=data["command"],
                    data=data,
                )
            self._refreshTree()

    def _refreshTree(self):
        """Refresh tree models"""
        self.SetModel(copy.deepcopy(self._model))
        self._orig_model = self._model

    def _getSelectedNode(self):
        selection = self.GetSelected()
        if not selection:
            return None
        return selection[0]

    def _confirmDialog(self, question, title):
        """Confirm dialog"""
        dlg = wx.MessageDialog(self, question, title, wx.YES_NO)
        res = dlg.ShowModal()
        dlg.Destroy()
        return res

    def _popupMenuLayer(self):
        """Create popup menu for commands"""
        menu = Menu()

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Remove"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRemoveCmd, item)

        self.PopupMenu(menu)
        menu.Destroy()

    def Filter(self, text):
        """Filter history
        :param str text: text string
        """
        if text:
            self._model = self._orig_model.Filtered(key=["command"], value=text)
        else:
            self._model = self._orig_model
        self.RefreshItems()

    def UpdateHistoryModelFromScratch(self):
        """Reload tree history model based on the current history log from scratch."""
        self._model.RemoveNode(self._model.root)
        self._initHistoryModel()

    def UpdateHistoryModelByCommand(self, label):
        """Update the model by the command and refresh the tree.

        :param label: model node label"""
        data = {"command": label}
        self._model.AppendNode(
            parent=self._model.root,
            label=data["command"],
            data=data,
        )
        self._refreshTree()

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
                self.runIgnoredCmdPattern.emit(cmd=split(command))
                return
            if re.compile(r"^r[3]?\.mapcalc").search(command):
                command = parse_mapcalc_cmd(command)
            command = replace_module_cmd_special_flags(command)
            lst = split(command)
            try:
                GUI(parent=self, giface=self._giface).ParseCommand(lst)
            except GException as e:
                GError(
                    parent=self,
                    message=str(e),
                    caption=_("Cannot be parsed into command"),
                    showTraceback=False,
                )

    def RemoveEntryFromHistory(self, del_line_number):
        """Remove entry from command history log"""
        history_path = get_current_mapset_gui_history_path()
        try:
            remove_entry_from_history(del_line_number, history_path)
        except OSError as e:
            GError(str(e))

    def OnRemoveCmd(self, event):
        """Remove cmd from the history file"""
        tree_node = self._getSelectedNode()
        cmd = tree_node.data["command"]
        question = _("Do you really want to remove <{}> command?").format(cmd)
        if self._confirmDialog(question, title=_("Remove command")) == wx.ID_YES:
            self.showNotification.emit(message=_("Removing <{}>").format(cmd))
            tree_index = self._model.GetIndexOfNode(tree_node)[0]
            self.RemoveEntryFromHistory(tree_index)
            self._giface.entryFromHistoryRemoved.emit(index=tree_index)
            self._model.RemoveNode(tree_node)
            self._refreshTree()
            self.showNotification.emit(message=_("<{}> removed").format(cmd))

    def OnItemSelected(self, node):
        """Item selected"""
        command = node.data["command"]
        self.showNotification.emit(message=command)

    def OnRightClick(self, node):
        """Display popup menu"""
        self._popupMenuLayer()
