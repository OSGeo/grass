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

import re
import copy

import wx

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

from grass.grassdb import history


class HistoryBrowserTree(CTreeView):
    """Tree structure visualizing and managing history of executed commands.
    Uses virtual tree and model defined in core/treemodel.py.
    """

    def __init__(
        self,
        parent,
        infoPanel,
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
        self.infoPanel = infoPanel

        self._initHistoryModel()

        self.showNotification = Signal("HistoryBrowserTree.showNotification")
        self.runIgnoredCmdPattern = Signal("HistoryBrowserTree.runIgnoredCmdPattern")

        self._giface.currentMapsetChanged.connect(self.UpdateHistoryModelFromScratch)
        self._giface.entryToHistoryAdded.connect(
            lambda entry: self.AppendNodeToHistoryModel(entry)
        )
        self._giface.entryInHistoryUpdated.connect(
            lambda entry: self.UpdateNodeInHistoryModel(entry)
        )

        self.SetToolTip(_("Double-click to open the tool"))
        self.selectionChanged.connect(self.OnItemSelected)
        self.itemActivated.connect(lambda node: self.Run(node))
        self.contextMenu.connect(self.OnRightClick)

    def _initHistoryModel(self):
        """Fill tree history model based on the current history log."""
        try:
            history_path = history.get_current_mapset_gui_history_path()
            content_list = history.read(history_path)
        except (OSError, ValueError) as e:
            GError(str(e))

        for data in content_list:
            self._model.AppendNode(
                parent=self._model.root,
                label=data["command"].strip(),
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
        self.infoPanel.clearCommandInfo()

    def AppendNodeToHistoryModel(self, entry):
        """Append node to the model and refresh the tree.

        :param entry dict: entry with 'command' and 'command_info' keys
        """
        new_node = self._model.AppendNode(
            parent=self._model.root,
            label=entry["command"].strip(),
            data=entry,
        )
        self._refreshTree()
        self.Select(new_node)
        self.ExpandNode(new_node)
        self.infoPanel.showCommandInfo(entry["command_info"])

    def UpdateNodeInHistoryModel(self, entry):
        """Update last node in the model and refresh the tree.

        :param entry dict: entry with 'command' and 'command_info' keys
        """
        # Remove last node
        index = [self._model.GetLeafCount(self._model.root) - 1]
        tree_node = self._model.GetNodeByIndex(index)
        self._model.RemoveNode(tree_node)

        # Add new node to the model
        self.AppendNodeToHistoryModel(entry)

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

    def RemoveEntryFromHistory(self, index):
        """Remove entry from command history log.

        :param int index: index of the entry which should be removed
        """
        try:
            history_path = history.get_current_mapset_gui_history_path()
            history.remove_entry(history_path, index)
        except (OSError, ValueError) as e:
            GError(str(e))

    def GetCommandInfo(self, index):
        """Get command info for the given command index.

        :param int index: index of the command
        """
        command_info = {}
        try:
            history_path = history.get_current_mapset_gui_history_path()
            command_info = history.read(history_path)[index]["command_info"]
        except (OSError, ValueError) as e:
            GError(str(e))
        return command_info

    def OnRemoveCmd(self, event):
        """Remove cmd from the history file"""
        tree_node = self._getSelectedNode()
        cmd = tree_node.data["command"]
        question = _("Do you really want to remove <{}> command?").format(cmd)
        if self._confirmDialog(question, title=_("Remove command")) == wx.ID_YES:
            self.showNotification.emit(message=_("Removing <{}>").format(cmd))
            tree_index = self._model.GetIndexOfNode(tree_node)[0]
            self.RemoveEntryFromHistory(tree_index)
            self.infoPanel.clearCommandInfo()
            self._giface.entryFromHistoryRemoved.emit(index=tree_index)
            self._model.RemoveNode(tree_node)
            self._refreshTree()
            self.showNotification.emit(message=_("<{}> removed").format(cmd))

    def OnItemSelected(self, node):
        """Item selected"""
        command = node.data["command"]
        self.showNotification.emit(message=command)
        tree_index = self._model.GetIndexOfNode(node)[0]
        command_info = self.GetCommandInfo(tree_index)
        self.infoPanel.showCommandInfo(command_info)

    def OnRightClick(self, node):
        """Display popup menu"""
        self._popupMenuLayer()
