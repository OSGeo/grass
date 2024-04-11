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
import datetime

import wx

from core import globalvar

from core.gcmd import GError, GException
from core.utils import (
    parse_mapcalc_cmd,
    replace_module_cmd_special_flags,
    split,
)
from gui_core.forms import GUI
from core.treemodel import TreeModel, DictNode
from gui_core.treeview import CTreeView
from gui_core.wrap import Menu

from grass.pydispatch.signal import Signal

from grass.grassdb import history


class HistoryBrowserNode(DictNode):
    """Node representing item in history browser."""

    def __init__(self, data=None):
        super().__init__(data=data)

    @property
    def label(self):
        if "time_period" in self.data:
            return self.data["time_period"]
        else:
            return self.data["name"]

    def match(self, method="exact", **kwargs):
        """Method used for searching according to given parameters.

        :param method: 'exact' for exact match or 'filtering' for filtering by type/name
        :param kwargs key-value to be matched, filtering method uses 'type' and 'name'
        """
        if not kwargs:
            return False

        if method == "exact":
            for key, value in kwargs.items():
                if not (key in self.data and self.data[key] == value):
                    return False
            return True
        # for filtering
        if (
            "type" in kwargs
            and "type" in self.data
            and kwargs["type"] != self.data["type"]
        ):
            return False
        if (
            "name" in kwargs
            and "name" in self.data
            and not kwargs["name"].search(self.data["name"])
        ):
            return False
        return True


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
        self._model = TreeModel(HistoryBrowserNode)
        self._orig_model = self._model
        super().__init__(parent=parent, model=self._model, id=wx.ID_ANY, style=style)

        self._giface = giface
        self.parent = parent
        self.infoPanel = infoPanel

        self._resetSelectVariables()

        self._initHistoryModel()

        self.showNotification = Signal("HistoryBrowserTree.showNotification")
        self.runIgnoredCmdPattern = Signal("HistoryBrowserTree.runIgnoredCmdPattern")

        self._giface.currentMapsetChanged.connect(self.UpdateHistoryModelFromScratch)
        self._giface.entryToHistoryAdded.connect(
            lambda entry: self.InsertCommand(entry)
        )
        self._giface.entryInHistoryUpdated.connect(
            lambda entry: self.UpdateCommand(entry)
        )

        self.SetToolTip(_("Double-click to open the tool"))
        self.selectionChanged.connect(self.OnItemSelected)
        self.itemActivated.connect(self.OnDoubleClick)
        self.contextMenu.connect(self.OnRightClick)

    def _sortTimePeriods(self):
        """Sort periods from newest to oldest based on the underlying timestamps."""
        if self._model.root.children:
            self._model.root.children.sort(
                key=lambda node: node.data["timestamp"], reverse=True
            )

    def _refreshTree(self):
        """Refresh tree models"""
        self._sortTimePeriods()
        self.SetModel(copy.deepcopy(self._model))
        self._orig_model = self._model

    def _resetSelectVariables(self):
        """Reset variables related to item selection."""
        self.selected_time_period = []
        self.selected_command = []

    def _getIndexFromFile(self, command_node):
        """Get index of command node in the corresponding history log file."""
        if command_node.data["missing_info"]:
            return self._model.GetIndexOfNode(command_node)[1]
        else:
            return history.filter(
                json_data=self.ReadFromHistory(),
                command=command_node.data["name"],
                timestamp=command_node.data["timestamp"],
            )

    def _timestampToTimePeriod(self, timestamp):
        """
        Convert timestamp to a time period label.

        :param timestamp: Timestamp string in ISO format.
        :return: Corresponding time period label.
        """
        current_date = datetime.date.today()
        timestamp_date = datetime.datetime.fromisoformat(timestamp).date()

        if timestamp_date == current_date:
            return _("Today")
        elif timestamp_date == current_date - datetime.timedelta(days=1):
            return _("Yesterday")
        else:
            return timestamp_date.strftime("%B %Y")

    def _confirmDialog(self, question, title):
        """Confirm dialog"""
        dlg = wx.MessageDialog(self, question, title, wx.YES_NO)
        res = dlg.ShowModal()
        dlg.Destroy()
        return res

    def _popupMenuCommand(self):
        """Create popup menu for commands"""
        menu = Menu()

        item = wx.MenuItem(menu, wx.ID_ANY, _("&Remove"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRemoveCmd, item)

        self.PopupMenu(menu)
        menu.Destroy()

    def _popupMenuEmpty(self):
        """Create empty popup when multiple different types of items are selected"""
        menu = Menu()
        item = wx.MenuItem(menu, wx.ID_ANY, _("No available options"))
        menu.AppendItem(item)
        item.Enable(False)
        self.PopupMenu(menu)
        menu.Destroy()

    def _initHistoryModel(self):
        """Fill tree history model based on the current history log."""
        content_list = self.ReadFromHistory()

        for entry in content_list:
            timestamp = None
            if entry["command_info"]:
                # Find time period node for entries with command info
                timestamp = entry["command_info"].get("timestamp")
                if timestamp:
                    time_period = self._model.SearchNodes(
                        parent=self._model.root,
                        time_period=self._timestampToTimePeriod(timestamp),
                        type="time_period",
                    )
            else:
                # Find time period node prepared for entries without any command info
                time_period = self._model.SearchNodes(
                    parent=self._model.root,
                    time_period=_("Missing info"),
                    type="time_period",
                )

            if time_period:
                time_period = time_period[0]
            else:
                # Create time period node if not found
                if not entry["command_info"]:
                    # Prepare it for entries without command info
                    random_history_date = datetime.datetime(1960, 1, 1).isoformat()
                    time_period = self._model.AppendNode(
                        parent=self._model.root,
                        data=dict(
                            type="time_period",
                            timestamp=random_history_date,
                            time_period=_("Missing info"),
                        ),
                    )
                else:
                    time_period = self._model.AppendNode(
                        parent=self._model.root,
                        data=dict(
                            type="time_period",
                            timestamp=entry["command_info"]["timestamp"],
                            time_period=self._timestampToTimePeriod(
                                entry["command_info"]["timestamp"]
                            ),
                        ),
                    )

            # Determine status and create command node
            if entry["command_info"]:
                status = entry["command_info"].get("status", _("Unknown"))
            else:
                status = _("Unknown")

            self._model.AppendNode(
                parent=time_period,
                data=dict(
                    type="command",
                    name=entry["command"].strip(),
                    timestamp=timestamp if timestamp else _("Unknown"),
                    status=status,
                    missing_info=not bool(entry["command_info"]),
                ),
            )

        # Sort time periods and refresh the tree view
        self._sortTimePeriods()
        self._refreshTree()

    def ReadFromHistory(self):
        """Read content of command history log.
        It is a wrapper which considers GError.
        """
        try:
            history_path = history.get_current_mapset_gui_history_path()
            content_list = history.read(history_path)
        except (OSError, ValueError) as e:
            GError(str(e))
        return content_list

    def RemoveEntryFromHistory(self, index):
        """Remove entry from command history log.
        It is a wrapper which considers GError.

        :param int index: index of the entry which should be removed
        """
        try:
            history_path = history.get_current_mapset_gui_history_path()
            history.remove_entry(history_path, index)
        except (OSError, ValueError) as e:
            GError(str(e))

    def GetCommandInfo(self, index):
        """Get command info for the given command index.
        It is a wrapper which considers GError.

        :param int index: index of the command
        """
        command_info = {}
        try:
            history_path = history.get_current_mapset_gui_history_path()
            command_info = history.read(history_path)[index]["command_info"]
        except (OSError, ValueError) as e:
            GError(str(e))
        return command_info

    def DefineItems(self, selected):
        """Set selected items."""
        self._resetSelectVariables()
        for item in selected:
            type = item.data["type"]
            if type == "command":
                self.selected_command.append(item)
                self.selected_time_period.append(item.parent)
            elif type == "time_period":
                self.selected_command.append(None)
                self.selected_time_period.append(item)

    def Filter(self, text):
        """Filter history
        :param str text: text string
        """
        if text:
            try:
                compiled = re.compile(text)
            except re.error:
                return
            self._model = self._orig_model.Filtered(method="filtering", name=compiled)
        else:
            self._model = self._orig_model
        self.RefreshItems()
        self.ExpandAll()

    def UpdateHistoryModelFromScratch(self):
        """Reload tree history model based on the current history log from scratch."""
        self._model.RemoveNode(self._model.root)
        self._initHistoryModel()
        self.infoPanel.clearCommandInfo()

    def InsertCommand(self, entry):
        """Insert command node to the model and refresh the tree.

        :param entry dict: entry with 'command' and 'command_info' keys
        """
        # Check if today time period node exists or create it
        today_nodes = self._model.SearchNodes(
            parent=self._model.root, time_period=_("Today"), type="time_period"
        )
        if not today_nodes:
            today_node = self._model.AppendNode(
                parent=self._model.root,
                data=dict(
                    type="time_period",
                    timestamp=entry["command_info"]["timestamp"],
                    time_period=_("Today"),
                ),
            )
        else:
            today_node = today_nodes[0]

        # Create the command node under today time period node
        command_node = self._model.AppendNode(
            parent=today_node,
            data=dict(
                type="command",
                name=entry["command"].strip(),
                timestamp=entry["command_info"]["timestamp"],
                status=entry["command_info"].get("status", "In process"),
                missing_info=False,
            ),
        )

        today_node = self._model.SearchNodes(
            parent=self._model.root, time_period=_("Today"), type="time_period"
        )

        # Sort time periods and refresh the tree
        self._sortTimePeriods()
        self._refreshTree()

        # Select and expand the newly added command node
        self.Select(command_node)
        self.ExpandNode(command_node)

        # Show command info in info panel
        self.infoPanel.showCommandInfo(entry["command_info"])

    def UpdateCommand(self, entry):
        """Update last node in the model and refresh the tree.

        :param entry dict: entry with 'command' and 'command_info' keys
        """
        # Get node of last command
        today_node = self._model.SearchNodes(
            parent=self._model.root, time_period=_("Today"), type="time_period"
        )[0]
        command_nodes = self._model.SearchNodes(parent=today_node, type="command")
        last_node = command_nodes[-1]

        # Remove last node
        self._model.RemoveNode(last_node)

        # Add new command node to the model
        self.InsertCommand(entry)

    def Run(self, node=None):
        """Parse selected history command into list and launch module dialog."""
        if not node:
            node = self.GetSelected()
            self.DefineItems(node)

        if not self.selected_command:
            return

        selected_command = self.selected_command[0]
        command = selected_command.data["name"]

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

    def OnRemoveCmd(self, event):
        """Remove cmd from the history file"""
        self.DefineItems(self.GetSelected())
        if not self.selected_command:
            return

        selected_command = self.selected_command[0]
        selected_time_period = self.selected_time_period[0]
        command = selected_command.data["name"]

        # Confirm deletion with user
        question = _("Do you really want to remove <{}> command?").format(command)
        if self._confirmDialog(question, title=_("Remove command")) != wx.ID_YES:
            return

        self.showNotification.emit(message=_("Removing <{}>").format(command))

        # Find the index of the selected command in history file
        history_index = self._getIndexFromFile(selected_command)

        # Remove the entry from history
        self.RemoveEntryFromHistory(history_index)
        self.infoPanel.clearCommandInfo()
        self._giface.entryFromHistoryRemoved.emit(index=history_index)
        self._model.RemoveNode(selected_command)

        # Check if the time period node should also be removed
        selected_time_period = selected_command.parent
        if selected_time_period and len(selected_time_period.children) == 0:
            self._model.RemoveNode(selected_time_period)

        self._refreshTree()
        self.showNotification.emit(message=_("<{}> removed").format(command))

    def OnItemSelected(self, node):
        """Handle item selection in the tree view."""
        self.DefineItems([node])
        if not self.selected_command[0]:
            return

        selected_command = self.selected_command[0]
        self.showNotification.emit(message=selected_command.data["name"])

        # Find the index of the selected command in history file
        history_index = self._getIndexFromFile(selected_command)

        # Show command info in info panel
        command_info = self.GetCommandInfo(history_index)
        self.infoPanel.showCommandInfo(command_info)

    def OnRightClick(self, node):
        """Display popup menu."""
        self.DefineItems([node])
        if self.selected_command[0]:
            self._popupMenuCommand()
        else:
            self._popupMenuEmpty()

    def OnDoubleClick(self, node):
        """Double click on item/node.

        Launch module dialog if node is a command otherwise
        expand/collapse node.
        """
        self.DefineItems([node])
        if self.selected_command[0]:
            self.Run(node)
        else:
            if self.IsNodeExpanded(node):
                self.CollapseNode(node, recursive=False)
            else:
                self.ExpandNode(node, recursive=False)
