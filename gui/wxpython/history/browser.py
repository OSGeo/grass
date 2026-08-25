"""
@package history.browser

@brief History browser

Classes:
 - browser::HistoryInfoPanel
 - browser::HistoryBrowser

(C) 2023-2024 by Linda Karlovska, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
@author Anna Petrasova (kratochanna gmail com)
@author Tomas Zigo
"""

from datetime import datetime

import wx
import wx.lib.scrolledpanel as SP

from gui_core.wrap import SearchCtrl, StaticText, StaticBox, Button
from history.tree import HistoryBrowserTree
from icons.icon import MetaIcon

import grass.script as gs

from grass.grassdb import history

from grass.pydispatch.signal import Signal

from core.gcmd import GError


TRANSLATION_KEYS = {
    "timestamp": _("Timestamp:"),
    "runtime": _("Runtime duration:"),
    "status": _("Status:"),
    "mask2d": _("Mask 2D:"),
    "mask3d": _("Mask 3D:"),
    "n": _("North:"),
    "s": _("South:"),
    "w": _("West:"),
    "e": _("East:"),
    "nsres": _("North-south resolution:"),
    "ewres": _("East-west resolution:"),
    "rows": _("Number of rows:"),
    "cols": _("Number of columns:"),
    "cells": _("Number of cells:"),
}


def get_translated_value(key, value):
    """Function for mapping command info values to the structure used in GUI."""
    if key == "timestamp":
        exec_datetime = datetime.fromisoformat(value)
        return exec_datetime.strftime("%Y-%m-%d %H:%M:%S")
    if key == "runtime":
        return _("{} sec").format(value)
    if key == "status":
        return _(value.capitalize())
    if key in {"mask2d", "mask3d"}:
        return _(str(value))


def make_label(key):
    return TRANSLATION_KEYS.get(key, "")


class HistoryInfoPanel(SP.ScrolledPanel):
    def __init__(self, parent, giface, title=("Command Info"), style=wx.TAB_TRAVERSAL):
        super().__init__(parent=parent, id=wx.ID_ANY, style=style)

        self.parent = parent
        self.giface = giface
        self.title = title

        self.region_settings = None

        self._initImages()

        self._createGeneralInfoBox()
        self._createRegionSettingsBox()

        self._layout()

    def _layout(self):
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(
            self.general_info_box_sizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5
        )
        mainSizer.Add(
            self.sizer_region_settings,
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=5,
        )
        self.SetSizer(mainSizer)
        self.SetMinSize(self.GetBestSize())

        self.Layout()

    def _initImages(self):
        bmpsize = (16, 16)
        self.icons = {
            "check": MetaIcon(img="success").GetBitmap(bmpsize),
            "cross": MetaIcon(img="cross").GetBitmap(bmpsize),
        }

    def _createGeneralInfoBox(self):
        """Create static box for general info about the command"""
        self.general_info_box = StaticBox(
            parent=self, id=wx.ID_ANY, label=_("General info")
        )
        self.general_info_box_sizer = wx.StaticBoxSizer(
            self.general_info_box, wx.VERTICAL
        )

        self.sizer_general_info = wx.GridBagSizer(hgap=0, vgap=0)
        self.sizer_general_info.SetCols(2)
        self.sizer_general_info.SetRows(5)

        self.general_info_box_sizer.Add(
            self.sizer_general_info, proportion=1, flag=wx.ALL | wx.EXPAND, border=5
        )
        self.sizer_general_info.AddGrowableCol(1)
        self.general_info_box.Hide()

    def _createRegionSettingsBox(self):
        """Create a static box for displaying region settings of the command"""
        self.region_settings_box = StaticBox(
            parent=self,
            id=wx.ID_ANY,
            label=_("Computational region during command execution"),
        )
        self.sizer_region_settings = wx.StaticBoxSizer(
            self.region_settings_box, wx.VERTICAL
        )

        self.sizer_region_settings_match = wx.BoxSizer(wx.HORIZONTAL)
        self.sizer_region_settings.Add(
            self.sizer_region_settings_match,
            proportion=0,
            flag=wx.ALL | wx.EXPAND,
            border=5,
        )

        self.sizer_region_settings_grid = wx.GridBagSizer(hgap=0, vgap=0)
        self.sizer_region_settings_grid.SetCols(2)
        self.sizer_region_settings_grid.SetRows(9)

        self.sizer_region_settings.Add(
            self.sizer_region_settings_grid,
            proportion=1,
            flag=wx.ALL | wx.EXPAND,
            border=5,
        )

        self.sizer_region_settings_grid.AddGrowableCol(1)
        self.region_settings_box.Hide()

    def _general_info_filter(self, key, value):
        filter_keys = ["timestamp", "runtime", "status"]
        return key in filter_keys or ((key in {"mask2d", "mask3d"}) and value is True)

    def _region_settings_filter(self, key):
        return key not in {"projection", "zone", "cells"}

    def _updateGeneralInfoBox(self, command_info):
        """Update a static box for displaying general info about the command.

        :param dict command_info: command info entry for update
        """
        self.sizer_general_info.Clear(True)

        idx = 0
        for key, value in command_info.items():
            if self._general_info_filter(key, value):
                self.sizer_general_info.Add(
                    StaticText(
                        parent=self.general_info_box,
                        id=wx.ID_ANY,
                        label=make_label(key),
                        style=wx.ALIGN_LEFT,
                    ),
                    flag=wx.ALIGN_LEFT | wx.ALL,
                    border=5,
                    pos=(idx, 0),
                )
                self.sizer_general_info.Add(
                    StaticText(
                        parent=self.general_info_box,
                        id=wx.ID_ANY,
                        label=get_translated_value(key, value),
                        style=wx.ALIGN_LEFT,
                    ),
                    flag=wx.ALIGN_LEFT | wx.ALL,
                    border=5,
                    pos=(idx, 1),
                )
                idx += 1

        self.general_info_box.Layout()
        self.general_info_box.Show()

    def _updateRegionSettingsGrid(self, command_info):
        """Update a grid that displays numerical values
        for the regional settings of the executed command.

        :param dict command_info: command info entry for update
        """
        self.sizer_region_settings_grid.Clear(True)

        self.region_settings = command_info["region"]
        idx = 0
        for key, value in self.region_settings.items():
            if self._region_settings_filter(key):
                self.sizer_region_settings_grid.Add(
                    StaticText(
                        parent=self.region_settings_box,
                        id=wx.ID_ANY,
                        label=make_label(key),
                        style=wx.ALIGN_LEFT,
                    ),
                    flag=wx.ALIGN_LEFT | wx.ALL,
                    border=5,
                    pos=(idx, 0),
                )
                self.sizer_region_settings_grid.Add(
                    StaticText(
                        parent=self.region_settings_box,
                        id=wx.ID_ANY,
                        label=str(value),
                        style=wx.ALIGN_LEFT,
                    ),
                    flag=wx.ALIGN_LEFT | wx.ALL,
                    border=5,
                    pos=(idx, 1),
                )
                idx += 1

        self.region_settings_box.Show()

    def _updateRegionSettingsMatch(self):
        """Update text, icon and button dealing with region update"""
        self.sizer_region_settings_match.Clear(True)

        # Region condition
        history_region = self.region_settings
        current_region = self._get_current_region()
        region_matches = history_region == current_region

        # Icon and button according to the condition
        if region_matches:
            icon = self.icons["check"]
            button_label = None
        else:
            icon = self.icons["cross"]
            button_label = _("Update current region")

        # Static text
        textRegionMatch = StaticText(
            parent=self.region_settings_box,
            id=wx.ID_ANY,
            label=_("Region match"),
        )
        self.sizer_region_settings_match.Add(
            textRegionMatch,
            proportion=0,
            flag=wx.ALIGN_CENTER_VERTICAL | wx.RIGHT,
            border=10,
        )

        # Static bitmap for icon
        iconRegionMatch = wx.StaticBitmap(self.region_settings_box, bitmap=icon)
        self.sizer_region_settings_match.Add(
            iconRegionMatch,
            proportion=0,
            flag=wx.ALIGN_CENTER_VERTICAL | wx.RIGHT,
            border=10,
        )

        if button_label:
            # Button for region update
            buttonUpdateRegion = Button(self.region_settings_box, id=wx.ID_ANY)
            buttonUpdateRegion.SetLabel(_("Update current region"))
            buttonUpdateRegion.SetToolTip(
                _("Set current computational region to the region of executed command")
            )
            buttonUpdateRegion.Bind(wx.EVT_BUTTON, self.OnUpdateRegion)
            self.sizer_region_settings_match.Add(
                buttonUpdateRegion,
                proportion=1,
                flag=wx.ALIGN_CENTER_VERTICAL,
                border=10,
            )

        self.region_settings_box.Show()

    def showCommandInfo(self, command_info):
        """Show command info input.

        :param dict command_info: command info entry for update
        """
        if command_info:
            self._updateGeneralInfoBox(command_info)
            self._updateRegionSettingsGrid(command_info)
            self._updateRegionSettingsMatch()
        else:
            self.hideCommandInfo()
        self.SetupScrolling(scroll_x=False, scroll_y=True)
        self.Layout()

    def hideCommandInfo(self):
        """Hide command info input."""
        self.general_info_box.Hide()
        self.region_settings_box.Hide()

    def _get_current_region(self):
        """Get current computational region settings."""
        return gs.region()

    def _get_history_region(self):
        """Get computational region settings of executed command."""
        return {
            key: value
            for key, value in self.region_settings.items()
            if self._region_settings_filter(key)
        }

    def OnUpdateRegion(self, event):
        """Set current region to the region of executed command."""
        history_region = self._get_history_region()
        gs.run_command("g.region", **history_region)
        self.giface.updateMap.emit(render=False, renderVector=False)
        self._updateRegionSettingsMatch()
        self.Layout()


class HistoryBrowser(wx.SplitterWindow):
    """History browser window for executing the commands from history log
    and showing history info.

    Signal:
        showNotification - attribute 'message'
    """

    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        title=_("History browser"),
        style=wx.TAB_TRAVERSAL | wx.FULL_REPAINT_ON_RESIZE,
        name="history",
        **kwargs,
    ):
        super().__init__(parent=parent, id=wx.ID_ANY, style=style, **kwargs)
        self.SetName("HistoryBrowser")

        self.parent = parent
        self._giface = giface

        self.showNotification = Signal("HistoryBrowser.showNotification")
        self.runIgnoredCmdPattern = Signal("HistoryBrowser.runIgnoredCmdPattern")

        self.panelBrowser = wx.Panel(parent=self)
        self.panelInfo = wx.Panel(parent=self)

        # info panel
        self.infoPanel = HistoryInfoPanel(parent=self.panelInfo, giface=giface)

        # tree with layers
        self.tree = HistoryBrowserTree(
            parent=self.panelBrowser, giface=giface, infoPanel=self.infoPanel
        )
        self.tree.SetToolTip(_("Double-click to run selected tool"))
        self.tree.showNotification.connect(self.showNotification)
        self.tree.runIgnoredCmdPattern.connect(self.runIgnoredCmdPattern)

        # search box
        self.search = SearchCtrl(parent=self.panelBrowser)
        self.search.SetDescriptiveText(_("Search"))
        self.search.ShowCancelButton(True)
        self.search.Bind(wx.EVT_TEXT, lambda evt: self.tree.Filter(evt.GetString()))
        self.search.Bind(wx.EVT_SEARCHCTRL_CANCEL_BTN, lambda evt: self.tree.Filter(""))

        # buttons
        self.btnCmdExportHistory = Button(parent=self.panelInfo, id=wx.ID_ANY)
        self.btnCmdExportHistory.SetLabel(_("&Export history"))
        self.btnCmdExportHistory.SetToolTip(
            _("Export history of executed commands to a file")
        )
        self.btnCmdExportHistory.Bind(wx.EVT_BUTTON, self.OnCmdExportHistory)

        self._layout()

    def _layout(self):
        """Dialog layout"""
        self.browserSizer = wx.BoxSizer(wx.VERTICAL)
        self.infoSizer = wx.BoxSizer(wx.VERTICAL)

        self.browserSizer.Add(
            self.search,
            proportion=0,
            flag=wx.ALL | wx.EXPAND,
            border=5,
        )
        self.browserSizer.Add(
            self.tree, proportion=1, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5
        )

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.AddStretchSpacer()
        btnSizer.Add(
            self.btnCmdExportHistory,
            proportion=0,
            flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
            border=5,
        )
        self.infoSizer.Add(
            self.infoPanel,
            proportion=1,
            flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP,
            border=5,
        )
        self.infoSizer.Add(
            btnSizer,
            proportion=0,
            flag=wx.ALL | wx.EXPAND,
            border=5,
        )

        self.browserSizer.Fit(self)
        self.browserSizer.SetSizeHints(self)
        self.panelBrowser.SetSizer(self.browserSizer)
        self.browserSizer.FitInside(self.panelBrowser)

        self.infoSizer.Fit(self)
        self.infoSizer.SetSizeHints(self)
        self.panelInfo.SetSizer(self.infoSizer)
        self.infoSizer.FitInside(self.panelInfo)

        # split window
        self.SplitHorizontally(self.panelBrowser, self.panelInfo, 0)
        self.SetMinimumPaneSize(self.btnCmdExportHistory.GetSize()[1] + 100)
        self.SetSashPosition(self.GetSize().y - 500)
        self.SetSashGravity(1.0)

        # layout
        self.SetAutoLayout(True)
        self.Layout()

    def OnCmdExportHistory(self, event):
        """Export the history of executed commands to a selected file."""
        history_path = history.get_current_mapset_gui_history_path()
        if history.get_history_file_extension(history_path) == ".json":
            defaultFile = "grass_cmd_log.json"
            wildcard = _("{json} (*.json)|*.txt|{files} (*)|*").format(
                json=_("JSON files"), files=_("Files")
            )
        else:
            defaultFile = "grass_cmd_log.txt"
            wildcard = _("{txt} (*.txt)|*.txt|{files} (*)|*").format(
                txt=_("Text files"), files=_("Files")
            )
        dlg = wx.FileDialog(
            self,
            message=_("Save file as..."),
            defaultFile=defaultFile,
            wildcard=wildcard,
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT,
        )
        if dlg.ShowModal() == wx.ID_OK:
            target_path = dlg.GetPath()
            try:
                history.copy(history_path, target_path)
                self.showNotification.emit(
                    message=_("Command history saved to '{}'").format(target_path)
                )
            except OSError as e:
                GError(str(e))

        dlg.Destroy()
        event.Skip()
