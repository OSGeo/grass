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
from core.treemodel import TreeModel, DictFilterNode
from gui_core.treeview import CTreeView
from gui_core.wrap import Menu

from icons.icon import MetaIcon

from grass.pydispatch.signal import Signal

from grass.grassdb import history
from grass.grassdb.history import Status


# global variables for node types
TIME_PERIOD = "time_period"
COMMAND = "command"

# global variable for purposes of sorting "No time info" node
OLD_DATE = datetime.datetime(1950, 1, 1).date()


class HistoryBrowserNode(DictFilterNode):
    """Node representing item in history browser."""

    def __init__(self, data=None):
        super().__init__(data=data)

    @property
    def label(self):
        if "day" in self.data.keys():
            return self.dayToLabel(self.data["day"])
        else:
            return self.data["name"]

    def dayToLabel(self, day):
        """
        Convert day (midnight timestamp) to a day node label.
        :param day datetime.date object: midnight of a day.
        :return str: Corresponding day label.
        """
        current_date = datetime.date.today()

        if day == OLD_DATE:
            return _("No time info")

        month_name = day.strftime("%B")
        base_date = _("{month_name} {day_number}").format(
            month_name=month_name, day_number=day.day
        )

        if day == current_date:
            return _("{base_date} (today)").format(base_date=base_date)
        elif day == current_date - datetime.timedelta(days=1):
            return _("{base_date} (yesterday)").format(base_date=base_date)
        elif day >= (current_date - datetime.timedelta(days=current_date.weekday())):
            return _("{base_date} (this week)").format(base_date=base_date)
        elif day.year == current_date.year:
            return _("{base_date}").format(base_date=base_date)
        else:
            return _("{base_date}, {year}").format(base_date=base_date, year=day.year)


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

        self._iconTypes = [
            TIME_PERIOD,
            Status.ABORTED.value,
            Status.FAILED.value,
            Status.RUNNING.value,
            Status.SUCCESS.value,
            Status.UNKNOWN.value,
        ]

        self._initImages()

        self._initHistoryModel()

        self.showNotification = Signal("HistoryBrowserTree.showNotification")
        self.runIgnoredCmdPattern = Signal("HistoryBrowserTree.runIgnoredCmdPattern")

        self._giface.currentMapsetChanged.connect(self.UpdateHistoryModelFromScratch)
        self._giface.entryToHistoryAdded.connect(self.InsertCommand)
        self._giface.entryInHistoryUpdated.connect(self.UpdateCommand)

        self.SetToolTip(_("Double-click to open the tool"))
        self.selectionChanged.connect(self.OnItemSelected)
        self.itemActivated.connect(self.OnDoubleClick)
        self.contextMenu.connect(self.OnRightClick)

    def _sortDays(self):
        """Sort day nodes from earliest to oldest."""
        if self._model.root.children:
            self._model.root.children.sort(
                key=lambda node: node.data["day"], reverse=True
            )

    def _refreshTree(self):
        """Refresh tree models"""
        self._sortDays()
        self.SetModel(copy.deepcopy(self._model))
        self._orig_model = self._model

    def _resetSelectVariables(self):
        """Reset variables related to item selection."""
        self.selected_day = []
        self.selected_command = []

    def _initImages(self):
        bmpsize = (16, 16)
        icons = {
            TIME_PERIOD: MetaIcon(img="time-period").GetBitmap(bmpsize),
            Status.ABORTED.value: MetaIcon(img="exclamation-mark").GetBitmap(bmpsize),
            Status.FAILED.value: MetaIcon(img="cross").GetBitmap(bmpsize),
            Status.RUNNING.value: MetaIcon(img="circle").GetBitmap(bmpsize),
            Status.SUCCESS.value: MetaIcon(img="success").GetBitmap(bmpsize),
            Status.UNKNOWN.value: MetaIcon(img="question-mark").GetBitmap(bmpsize),
        }
        il = wx.ImageList(bmpsize[0], bmpsize[1], mask=False)
        for each in self._iconTypes:
            il.Add(icons[each])
        self.AssignImageList(il)

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

    def _timestampToDay(self, timestamp=None):
        """
        Convert timestamp to datetime.date object with time set to midnight.
        :param str timestamp: Timestamp as a string in ISO format.
        :return datetime.date day_midnight: midnight of a day.
        """
        if not timestamp:
            return OLD_DATE

        timestamp_datetime = datetime.datetime.fromisoformat(timestamp)
        return datetime.datetime(
            timestamp_datetime.year, timestamp_datetime.month, timestamp_datetime.day
        ).date()

    def _initHistoryModel(self):
        """Fill tree history model based on the current history log."""
        content_list = self.ReadFromHistory()

        for entry in content_list:
            timestamp = None
            if entry["command_info"]:
                # Find day node for entries with command info
                timestamp = entry["command_info"].get("timestamp")
                if timestamp:
                    day = self._model.SearchNodes(
                        parent=self._model.root,
                        day=self._timestampToDay(timestamp),
                        type=TIME_PERIOD,
                    )
            else:
                # Find day node prepared for entries without any command info
                day = self._model.SearchNodes(
                    parent=self._model.root,
                    day=self._timestampToDay(),
                    type=TIME_PERIOD,
                )

            if day:
                day = day[0]
            # Create time period node if not found
            elif not entry["command_info"]:
                # Prepare it for entries without command info
                day = self._model.AppendNode(
                    parent=self._model.root,
                    data={"type": TIME_PERIOD, "day": self._timestampToDay()},
                )
            else:
                day = self._model.AppendNode(
                    parent=self._model.root,
                    data={
                        "type": TIME_PERIOD,
                        "day": self._timestampToDay(entry["command_info"]["timestamp"]),
                    },
                )

            # Determine status and create command node
            status = (
                entry["command_info"].get("status")
                if entry.get("command_info")
                and entry["command_info"].get("status") is not None
                else Status.UNKNOWN.value
            )

            # Add command to time period node
            self._model.AppendNode(
                parent=day,
                data={
                    "type": COMMAND,
                    "name": entry["command"].strip(),
                    "timestamp": timestamp or None,
                    "status": status,
                },
            )

        # Refresh the tree view
        self._refreshTree()

    def _getIndexFromFile(self, command_node):
        """Get index of command node in the corresponding history log file."""
        if not command_node.data["timestamp"]:
            return self._model.GetIndexOfNode(command_node)[1]
        else:
            return history.filter(
                json_data=self.ReadFromHistory(),
                command=command_node.data["name"],
                timestamp=command_node.data["timestamp"],
            )

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
            if type == COMMAND:
                self.selected_command.append(item)
                self.selected_day.append(item.parent)
            elif type == TIME_PERIOD:
                self.selected_command.append(None)
                self.selected_day.append(item)

    def Filter(self, text):
        """Filter history
        :param str text: text string
        """
        if text:
            try:
                compiled = re.compile(text)
            except re.error:
                return
            self._model = self._orig_model.Filtered(
                method="filtering", name=compiled, type=COMMAND
            )
        else:
            self._model = self._orig_model
        self.RefreshItems()
        self.ExpandAll()

    def UpdateHistoryModelFromScratch(self):
        """Reload tree history model based on the current history log from scratch."""
        self._model.RemoveNode(self._model.root)
        self._initHistoryModel()
        self.infoPanel.hideCommandInfo()

    def InsertCommand(self, entry):
        """Insert command node to the model and refresh the tree.

        :param entry dict: entry with 'command' and 'command_info' keys
        """
        # Check if today time period node exists or create it
        today = self._timestampToDay(entry["command_info"]["timestamp"])
        today_nodes = self._model.SearchNodes(
            parent=self._model.root, day=today, type=TIME_PERIOD
        )
        if not today_nodes:
            today_node = self._model.AppendNode(
                parent=self._model.root,
                data={
                    "type": TIME_PERIOD,
                    "day": today,
                },
            )
        else:
            today_node = today_nodes[0]

        # Create the command node under today time period node
        command_node = self._model.AppendNode(
            parent=today_node,
            data={
                "type": COMMAND,
                "name": entry["command"].strip(),
                "timestamp": entry["command_info"]["timestamp"],
                "status": entry["command_info"].get("status", Status.UNKNOWN.value),
            },
        )

        # Refresh the tree
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
        today = self._timestampToDay(entry["command_info"]["timestamp"])
        today_node = self._model.SearchNodes(
            parent=self._model.root, day=today, type=TIME_PERIOD
        )[0]
        command_nodes = self._model.SearchNodes(parent=today_node, type=COMMAND)
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

    def OnGetItemImage(self, index, which=wx.TreeItemIcon_Normal, column=0):
        """Overridden method to return image for each item."""
        node = self._model.GetNodeByIndex(index)
        try:
            if node.data["type"] == TIME_PERIOD:
                return self._iconTypes.index(node.data["type"])
            elif node.data["type"] == COMMAND:
                return self._iconTypes.index(node.data["status"])
        except ValueError:
            return 0

    def OnRemoveCmd(self, event):
        """Remove cmd from the history file"""
        self.DefineItems(self.GetSelected())
        if not self.selected_command:
            return

        selected_command = self.selected_command[0]
        selected_day = self.selected_day[0]
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
        self.infoPanel.hideCommandInfo()
        self._giface.entryFromHistoryRemoved.emit(index=history_index)
        self._model.RemoveNode(selected_command)

        # Check if the time period node should also be removed
        selected_day = selected_command.parent
        if selected_day and len(selected_day.children) == 0:
            self._model.RemoveNode(selected_day)

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
            return

        if self.IsNodeExpanded(node):
            self.CollapseNode(node, recursive=False)
        else:
            self.ExpandNode(node, recursive=False)
