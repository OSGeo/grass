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
@author Anna Petrasova (kratochanna gmail com)
@author Tomas Zigo
"""

import wx

from gui_core.wrap import SearchCtrl, Button
from history.tree import HistoryBrowserTree

from grass.pydispatch.signal import Signal
from grass.grassdb.history import (
    copy_history,
    get_current_mapset_gui_history_path,
)

from core.gcmd import GError


class HistoryBrowser(wx.Panel):
    """History browser panel for executing the commands from history log.

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

        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)
        self.SetName("HistoryBrowser")

        self.showNotification = Signal("HistoryBrowser.showNotification")
        self.runIgnoredCmdPattern = Signal("HistoryBrowser.runIgnoredCmdPattern")

        # search box
        self.search = SearchCtrl(self)
        self.search.SetDescriptiveText(_("Search"))
        self.search.ShowCancelButton(True)
        self.search.Bind(wx.EVT_TEXT, lambda evt: self.tree.Filter(evt.GetString()))
        self.search.Bind(wx.EVT_SEARCHCTRL_CANCEL_BTN, lambda evt: self.tree.Filter(""))

        # buttons
        self.btnCmdExportHistory = Button(parent=self, id=wx.ID_ANY)
        self.btnCmdExportHistory.SetLabel(_("&Export history"))
        self.btnCmdExportHistory.SetToolTip(
            _("Export history of executed commands to a file")
        )
        self.btnCmdExportHistory.Bind(wx.EVT_BUTTON, self.OnCmdExportHistory)

        # tree with layers
        self.tree = HistoryBrowserTree(self, giface=giface)
        self.tree.SetToolTip(_("Double-click to run selected tool"))
        self.tree.showNotification.connect(self.showNotification)
        self.tree.runIgnoredCmdPattern.connect(self.runIgnoredCmdPattern)

        self._layout()

    def _layout(self):
        """Dialog layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(
            self.search,
            proportion=0,
            flag=wx.ALL | wx.EXPAND,
            border=5,
        )
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.AddStretchSpacer()
        btnSizer.Add(
            self.btnCmdExportHistory,
            proportion=0,
            flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
            border=5,
        )
        sizer.Add(
            self.tree, proportion=1, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5
        )
        sizer.Add(btnSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        self.SetSizerAndFit(sizer)
        self.SetAutoLayout(True)
        self.Layout()

    def OnCmdExportHistory(self, event):
        """Export the history of executed commands stored
        in a .wxgui_history file to a selected file."""
        dlg = wx.FileDialog(
            self,
            message=_("Save file as..."),
            defaultFile="grass_cmd_log.txt",
            wildcard=_("{txt} (*.txt)|*.txt|{files} (*)|*").format(
                txt=_("Text files"), files=_("Files")
            ),
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT,
        )

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            history_path = get_current_mapset_gui_history_path()
            try:
                copy_history(path, history_path)
                self.showNotification.emit(
                    message=_("Command history saved to '{}'".format(path))
                )
            except OSError as e:
                GError(str(e))

        dlg.Destroy()
        event.Skip()
