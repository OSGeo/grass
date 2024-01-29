"""
@package history.tree

@brief History browser tree classes

Classes:
 - history::CommandInfoMapper
 - history::HistoryInfoDialog
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
from datetime import datetime

import wx
import wx.lib.scrolledpanel as SP

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
from gui_core.wrap import Menu, Button, StaticText

from grass.pydispatch.signal import Signal

from grass.grassdb.history import create_history_manager


class CommandInfoMapper:
    """Class for mapping command info values to the structure used in GUI."""

    def __init__(self, command_info):
        self.command_info = command_info

    def get_translated_value(self, key):
        if key == "timestamp":
            exec_datetime = datetime.fromisoformat(self.command_info[key])
            return exec_datetime.strftime("%Y-%m-%d %H:%M:%S")
        elif key == "runtime":
            return _("{} sec".format(self.command_info[key]))
        elif key == "status":
            return _(self.command_info[key].capitalize())
        elif key == "mask2d" or key == "mask3d":
            return _(str(self.command_info[key]))

    def make_label(self, key):
        if key == "timestamp":
            return _("Timestamp: ")
        elif key == "runtime":
            return _("Runtime duration: ")
        elif key == "status":
            return _("Status: ")
        elif key == "mask2d":
            return _("Mask 2D: ")
        elif key == "mask3d":
            return _("Mask 3D: ")
        elif key == "n":
            return _("North: ")
        elif key == "s":
            return _("South: ")
        elif key == "w":
            return _("West: ")
        elif key == "e":
            return _("East: ")
        elif key == "nsres":
            return _("North-south resolution: ")
        elif key == "ewres":
            return _("East-west resolution: ")
        elif key == "rows":
            return _("Number of rows: ")
        elif key == "cols":
            return _("Number of columns: ")
        elif key == "cells":
            return _("Number of cells: ")


class HistoryInfoDialog(wx.Dialog):
    def __init__(
        self,
        parent,
        command_info,
        title=("Command Info"),
        size=(-1, 400),
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
    ):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title, style=style)

        self.parent = parent
        self.title = title
        self.size = size
        self.command_info = command_info
        self.mapper = CommandInfoMapper(command_info)

        # notebook
        self.notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)
        # create notebook pages
        self._createGeneralInfoPage(parent=self.notebook)
        self._createRegionSettingsPage(parent=self.notebook)

        self.btnClose = Button(self, wx.ID_CLOSE)
        self.SetEscapeId(wx.ID_CLOSE)

        self._layout()

    def _layout(self):
        """Layout window"""
        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(self.btnClose)
        btnStdSizer.Realize()

        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(self.notebook, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(btnStdSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        self.SetSizer(mainSizer)
        self.SetMinSize(self.GetBestSize())
        self.SetSize(self.size)

    def _createGeneralInfoPage(self, parent):
        """Create notebook page for general info about the command"""

        panel = SP.ScrolledPanel(parent=parent, id=wx.ID_ANY)
        panel.SetupScrolling(scroll_x=False, scroll_y=True)
        parent.AddPage(page=panel, text=_("General info"))

        # General settings
        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        idx = 1
        for key, value in self.command_info.items():
            if (
                key == "timestamp"
                or key == "runtime"
                or key == "status"
                or ((key == "mask2d" or key == "mask3d") and value is True)
            ):
                self.sizer.Add(
                    StaticText(
                        parent=panel,
                        id=wx.ID_ANY,
                        label=self.mapper.make_label(key),
                        style=wx.ALIGN_LEFT,
                    ),
                    flag=wx.ALIGN_LEFT | wx.ALL,
                    border=5,
                    pos=(idx, 0),
                )
                self.sizer.Add(
                    StaticText(
                        parent=panel,
                        id=wx.ID_ANY,
                        label=self.mapper.get_translated_value(key),
                        style=wx.ALIGN_LEFT,
                    ),
                    flag=wx.ALIGN_LEFT | wx.ALL,
                    border=5,
                    pos=(idx, 1),
                )
                idx += 1

        self.sizer.AddGrowableCol(1)
        panel.SetSizer(self.sizer)

    def _createRegionSettingsPage(self, parent):
        """Create notebook page for displaying region settings of the command"""

        region_settings = self.command_info["region"]

        panel = SP.ScrolledPanel(parent=parent, id=wx.ID_ANY)
        panel.SetupScrolling(scroll_x=False, scroll_y=True)
        parent.AddPage(page=panel, text=_("Region settings"))

        # General settings
        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        idx = 1
        for key, value in region_settings.items():
            if (key != "projection") and (key != "zone"):
                self.sizer.Add(
                    StaticText(
                        parent=panel,
                        id=wx.ID_ANY,
                        label=self.mapper.make_label(key),
                        style=wx.ALIGN_LEFT,
                    ),
                    flag=wx.ALIGN_LEFT | wx.ALL,
                    border=5,
                    pos=(idx, 0),
                )
                self.sizer.Add(
                    StaticText(
                        parent=panel,
                        id=wx.ID_ANY,
                        label=str(value),
                        style=wx.ALIGN_LEFT,
                    ),
                    flag=wx.ALIGN_LEFT | wx.ALL,
                    border=5,
                    pos=(idx, 1),
                )
                idx += 1

        self.sizer.AddGrowableCol(1)
        panel.SetSizer(self.sizer)


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
        self.history_manager = create_history_manager()
        try:
            content_list = self.history_manager.get_content()
        except (OSError, ValueError) as e:
            GError(str(e))

        for data in content_list:
            data["command"] = " ".join(data["command"])
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

        if self.history_manager.filetype == "json":
            item = wx.MenuItem(menu, wx.ID_ANY, _("&Show info"))
            menu.AppendItem(item)
            self.Bind(wx.EVT_MENU, self.OnShowInfo, item)

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

    def AppendNodeToHistoryModel(self, entry):
        """Append node to the model and refresh the tree.

        :param entry dict: entry with 'command' and 'command_info' keys
        """
        entry["command"] = " ".join(entry["command"])

        self._model.AppendNode(
            parent=self._model.root,
            label=entry["command"].strip(),
            data=entry,
        )
        self._refreshTree()

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
            self.history_manager.remove_entry_from_history(index)
        except (OSError, ValueError) as e:
            GError(str(e))

    def GetCommandInfo(self, index):
        """Get command info for the given command index.

        :param int index: index of the command
        """
        command_info = {}
        try:
            command_info = self.history_manager.get_content()[index]["command_info"]
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
            self._giface.entryFromHistoryRemoved.emit(index=tree_index)
            self._model.RemoveNode(tree_node)
            self._refreshTree()
            self.showNotification.emit(message=_("<{}> removed").format(cmd))

    def OnShowInfo(self, event):
        """Show info about command in the small dialog"""
        tree_node = self._getSelectedNode()
        tree_index = self._model.GetIndexOfNode(tree_node)[0]
        command_info = self.GetCommandInfo(tree_index)
        dialog = HistoryInfoDialog(self, command_info)
        dialog.ShowModal()

    def OnItemSelected(self, node):
        """Item selected"""
        command = node.data["command"]
        self.showNotification.emit(message=command)

    def OnRightClick(self, node):
        """Display popup menu"""
        self._popupMenuLayer()
