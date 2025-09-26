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
        return self.data["name"]

    @property
    def time_sort(self):
        if "day" in self.data.keys():
            return self.data["day"]
        if "timestamp" in self.data.keys():
            return self.data["timestamp"]
        return None

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
        if day == current_date - datetime.timedelta(days=1):
            return _("{base_date} (yesterday)").format(base_date=base_date)
        if day >= (current_date - datetime.timedelta(days=current_date.weekday())):
            return _("{base_date} (this week)").format(base_date=base_date)
        if day.year == current_date.year:
            return _("{base_date}").format(base_date=base_date)
        return _("{base_date}, {year}").format(base_date=base_date, year=day.year)


class HistoryTreeModel(TreeModel):
    """Customized tree model defined in core/treemodel.py."""

    def __init__(self, nodeClass):
        """Constructor creates root node.

        :param nodeClass: class which is used for creating nodes
        """
        super().__init__(nodeClass=nodeClass)

    def SortChildren(self, node):
        """
        Sort children chronologically based on time_sort property.
        Leave out day nodes that include commands with missing info.

        :param node: node whose children are sorted
        """
        if node.children and node.time_sort != OLD_DATE:
            node.children.sort(key=lambda node: node.time_sort, reverse=True)

    def UpdateNode(self, node, **kwargs):
        """Update node attributes.

        :param node: node to be updated
        :param kwargs: key-value pairs of attributes to update
        """
        for key, value in kwargs.items():
            node.data[key] = value


class HistoryBrowserTree(CTreeView):
    """Tree structure visualizing and managing history of executed commands.
    Uses customized virtual tree and model.
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
        self._model = HistoryTreeModel(HistoryBrowserNode)
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

        copyItem = wx.MenuItem(menu, wx.ID_ANY, _("&Copy"))
        menu.AppendItem(copyItem)
        self.Bind(wx.EVT_MENU, self.OnCopyCmd, copyItem)

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

    def _timestampToDay(self, datetime_timestamp=None):
        """
        Convert a timestamp to datetime.date object with time set to midnight.

        :param datetime.datetime datetime_timestamp: Command timestamp
        :return datetime.date day_midnight: midnight of a day.
        """
        if not datetime_timestamp:
            return OLD_DATE

        return datetime.datetime(
            datetime_timestamp.year, datetime_timestamp.month, datetime_timestamp.day
        ).date()

    def _timestampToISO(self, datetime_timestamp=None):
        """
        Convert a datetime timestamp to ISO format.

        :param str datetime_timestamp: datetime.datetime object
        :return: Command timestamp in ISO format or None if datetime_timestamp is None.
        """
        return (
            datetime.datetime.isoformat(datetime_timestamp)
            if datetime_timestamp
            else None
        )

    def _timestampToDatetime(self, iso_timestamp=None):
        """
        Convert an ISO format timestamp string to a datetime object.

        :param str iso_timestamp: Command timestamp in ISO format.
        :return: datetime.datetime object or None if iso_timestamp is None.
        """
        return datetime.datetime.fromisoformat(iso_timestamp) if iso_timestamp else None

    def _reloadNode(self, node):
        """Reload the model of a specific node."""
        self._model.SortChildren(node)
        self._orig_model = copy.deepcopy(self._model)
        if node == self._model.root:
            self.RefreshItems()
        else:
            self.RefreshNode(node, recursive=True)

    def _populateDayItem(self, day_node, entry):
        """
        Populate a day item with a command info node.

        :param entry dict: entry with 'command' and 'command_info' keys
        :return: The newly created command node.
        """
        # Determine command timestamp
        command_info = entry.get("command_info", {})
        timestamp = command_info.get("timestamp") if command_info else None

        # Determine command status
        status = (
            command_info.get("status")
            if command_info and command_info.get("status") is not None
            else Status.UNKNOWN.value
        )

        # Add command node to day node
        return self._model.AppendNode(
            parent=day_node,
            data={
                "type": COMMAND,
                "name": entry["command"].strip(),
                "timestamp": (
                    self._timestampToDatetime(timestamp) if timestamp else None
                ),
                "status": status,
            },
        )

    def _initHistoryModel(self):
        """
        Populate the tree history model based on the current history log.
        """
        for entry in self.ReadFromHistory():
            # Get history day node
            day_node = self.GetHistoryNode(entry)

            if not day_node:
                # Create the day node if it doesn't exist
                day_node = self.InsertDay(entry)

            # Populate the day node with the command entry
            self._populateDayItem(day_node, entry)

            # Sort command nodes inside the day node from newest to oldest
            self._model.SortChildren(day_node)

        # Refresh the tree view
        self._reloadNode(self._model.root)

    def _getIndexFromFile(self, command_node):
        """
        Get index of command node in the corresponding history log file.
        """
        if not command_node.data["timestamp"]:
            return self._model.GetIndexOfNode(command_node)[1]
        return history.filter(
            json_data=self.ReadFromHistory(),
            command=command_node.data["name"],
            timestamp=self._timestampToISO(command_node.data["timestamp"]),
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

    def GetHistoryNode(self, entry, command_index=None):
        """
        Get node representing time/command or None if not found.

        :param entry dict: entry with 'command' and 'command_info' keys
        :param int command_index: index of the command from the particular day
        :return: Node representing the time/command or None if not found.
        """
        if entry["command_info"]:
            # Find the day node for entries with command info
            timestamp = entry["command_info"].get("timestamp")
            day = self._timestampToDay(self._timestampToDatetime(timestamp))
        else:
            # Find the day node for entries without command info
            day = self._timestampToDay()

        # Search for day nodes matching the given day and type
        day_nodes = self._model.SearchNodes(
            parent=self._model.root,
            day=day,
            type=TIME_PERIOD,
        )

        if day_nodes:
            if command_index is None:
                # If no command index is specified, return the first found day node
                return day_nodes[0]
            # Search for command nodes under the first day node
            command_nodes = self._model.SearchNodes(parent=day_nodes[0], type=COMMAND)
            if 0 <= command_index < len(command_nodes):
                return command_nodes[command_index]

        return None

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

    def InsertDay(self, entry):
        """Insert a node representing the time period into the model.

        :param entry dict: entry with 'command' and 'command_info' keys
        :return: Node representing the day.
        """
        if entry.get("command_info"):
            # Create time period node
            day = self._model.AppendNode(
                parent=self._model.root,
                data={
                    "type": TIME_PERIOD,
                    "day": self._timestampToDay(
                        self._timestampToDatetime(
                            entry["command_info"].get("timestamp")
                        )
                    ),
                },
            )
        else:
            # Create time period node for entries with missing timestamp info
            day = self._model.AppendNode(
                parent=self._model.root,
                data={
                    "type": TIME_PERIOD,
                    "day": self._timestampToDay(),
                },
            )

        return day

    def InsertCommand(self, entry):
        """Insert command node to the model and reload it.

        :param entry dict: Dictionary with 'command' and 'command_info' keys
        """
        # Check if time period node exists or create it
        today_node = self.GetHistoryNode(entry=entry)
        command_info = entry["command_info"]

        if not today_node:
            today_node = self._model.AppendNode(
                parent=self._model.root,
                data={
                    "type": TIME_PERIOD,
                    "day": self._timestampToDay(
                        self._timestampToDatetime(command_info["timestamp"])
                    ),
                },
            )
            self._model.SortChildren(self._model.root)
            self.RefreshItems()

        # Populate today's node by executed command
        command_node = self._populateDayItem(today_node, entry)
        self._reloadNode(today_node)

        # Select and expand the newly added command node
        self.Select(command_node)
        self.ExpandNode(command_node)

        # Show command info in info panel
        self.infoPanel.showCommandInfo(command_info)

    def UpdateCommand(self, entry):
        """Update first command node in the model and refresh it.

        :param entry dict: entry with 'command' and 'command_info' keys
        """
        # Get node of first command
        first_node = self.GetHistoryNode(entry=entry, command_index=0)

        # Extract command info
        command_info = entry["command_info"]
        status = command_info["status"]
        timestamp = command_info["timestamp"]

        # Convert timestamp to datetime object
        datetime_timestamp = self._timestampToDatetime(timestamp)

        # Update command node
        self._model.UpdateNode(
            first_node,
            status=status,
            timestamp=datetime_timestamp,
        )
        self._reloadNode(first_node.parent)

        # Show command info in info panel
        self.infoPanel.showCommandInfo(command_info)

    def Run(self, node=None):
        """Parse selected history command into list and launch module dialog."""
        if not node:
            node = self.GetSelected()
            self.DefineItems(node)

        if not self.selected_command:
            return

        selected_command = self.selected_command[0]
        command = selected_command.data["name"]

        if (
            globalvar.ignoredCmdPattern
            and re.compile(globalvar.ignoredCmdPattern).search(command)
            and "--help" not in command
            and "--ui" not in command
        ):
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
            if node.data["type"] == COMMAND:
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
            self._reloadNode(self._model.root)
        else:
            self._reloadNode(selected_day)
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

    def OnCopyCmd(self, event):
        """Copy selected cmd to clipboard"""
        self.DefineItems(self.GetSelected())
        if not self.selected_command:
            return

        selected_command = self.selected_command[0]
        command = selected_command.data["name"]

        # Copy selected command to clipboard
        try:
            if wx.TheClipboard.Open():
                try:
                    wx.TheClipboard.SetData(wx.TextDataObject(command))
                    self.showNotification.emit(
                        message=_("Command <{}> copied to clipboard").format(command)
                    )
                finally:
                    wx.TheClipboard.Close()
        except wx.PyWidgetError:
            self.showNotification.emit(message=_("Failed to copy command to clipboard"))
