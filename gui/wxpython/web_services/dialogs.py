"""
@package web_services.dialogs

@brief Dialogs for web services.

List of classes:
 - dialogs::WSDialogBase
 - dialogs::AddWSDialog
 - dialogs::WSPropertiesDialog
 - dialogs::SaveWMSLayerDialog

(C) 2009-2021 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Stepan Turek <stepan.turek seznam.cz>
"""

import wx

import os
import shutil

from copy import deepcopy

import grass.script as gs
from grass.script.task import cmdlist_to_tuple, cmdtuple_to_list

from core import globalvar
from core.debug import Debug
from core.gcmd import GMessage, GWarning, GError
from core.utils import GetSettingsPath
from core.gconsole import CmdThread, GStderr, EVT_CMD_DONE, EVT_CMD_OUTPUT

from gui_core.gselect import Select
from gui_core.wrap import Button, StaticText, StaticBox, TextCtrl, RadioButton

from web_services.widgets import WSPanel, WSManageSettingsWidget


class WSDialogBase(wx.Dialog):
    """Base class for web service dialogs."""

    def __init__(
        self,
        parent,
        id=wx.ID_ANY,
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        wx.Dialog.__init__(self, parent, id, style=style, **kwargs)

        self.parent = parent

        # contains panel for every web service on server
        self.ws_panels = {
            "WMS_1.1.1": {"panel": None, "label": "WMS 1.1.1"},
            "WMS_1.3.0": {"panel": None, "label": "WMS 1.3.0"},
            "WMTS": {"panel": None, "label": "WMTS"},
            "OnEarth": {"panel": None, "label": "OnEarth"},
        }

        # TODO: should be in file
        self.default_servers = {
            "OSM-WMS": [
                "https://ows.terrestris.de/osm/service?",
                "",
                "",
            ],
            "NASA GIBS WMTS": [
                "http://gibs.earthdata.nasa.gov/wmts/epsg4326/best/wmts.cgi",
                "",
                "",
            ],
            "tiles.maps.eox.at (Sentinel-2)": [
                "https://tiles.maps.eox.at/wms",
                "",
                "",
            ],
        }

        # holds reference to web service panel which is showed
        self.active_ws_panel = None

        # buttons which are disabled when the dialog is not connected
        self.run_btns = []

        # stores error messages for GError dialog showed when all web service
        # connections were unsuccessful
        self.error_msgs = ""

        self._createWidgets()
        self._doLayout()

    def _createWidgets(self):
        settingsFile = os.path.join(GetSettingsPath(), "wxWS")

        self.settsManager = WSManageSettingsWidget(
            parent=self, settingsFile=settingsFile, default_servers=self.default_servers
        )

        self.settingsBox = StaticBox(
            parent=self, id=wx.ID_ANY, label=_(" Server settings ")
        )

        self.serverText = StaticText(parent=self, id=wx.ID_ANY, label=_("Server:"))
        self.server = TextCtrl(parent=self, id=wx.ID_ANY)

        self.btn_connect = Button(parent=self, id=wx.ID_ANY, label=_("&Connect"))
        self.btn_connect.SetToolTip(_("Connect to the server"))
        if not self.server.GetValue():
            self.btn_connect.Enable(False)

        self.infoCollapseLabelExp = _("Show advanced connection settings")
        self.infoCollapseLabelCol = _("Hide advanced connection settings")

        self.adv_conn = wx.CollapsiblePane(
            parent=self,
            label=self.infoCollapseLabelExp,
            style=wx.CP_DEFAULT_STYLE | wx.CP_NO_TLW_RESIZE | wx.EXPAND,
        )

        self.MakeAdvConnPane(pane=self.adv_conn.GetPane())
        self.adv_conn.Collapse(True)
        self.Bind(
            wx.EVT_COLLAPSIBLEPANE_CHANGED, self.OnAdvConnPaneChanged, self.adv_conn
        )

        self.reqDataPanel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.layerNameBox = StaticBox(
            parent=self.reqDataPanel, id=wx.ID_ANY, label=_(" Layer Manager Settings ")
        )

        self.layerNameText = StaticText(
            parent=self.reqDataPanel, id=wx.ID_ANY, label=_("Output layer name:")
        )
        self.layerName = TextCtrl(parent=self.reqDataPanel, id=wx.ID_ANY)

        for ws in self.ws_panels.keys():
            # set class WSPanel argument layerNameTxtCtrl
            self.ws_panels[ws]["panel"] = WSPanel(
                parent=self.reqDataPanel, web_service=ws
            )
            self.ws_panels[ws]["panel"].capParsed.connect(self.OnPanelCapParsed)
            self.ws_panels[ws]["panel"].layerSelected.connect(self.OnLayerSelected)

        # buttons
        self.btn_close = Button(parent=self, id=wx.ID_CLOSE)
        self.btn_close.SetToolTip(_("Close dialog"))

        # statusbar
        self.statusbar = wx.StatusBar(parent=self, id=wx.ID_ANY)

        # bindings
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.btn_connect.Bind(wx.EVT_BUTTON, self.OnConnect)

        self.server.Bind(wx.EVT_TEXT, self.OnServer)
        self.layerName.Bind(wx.EVT_TEXT, self.OnOutputLayerName)

        self.settsManager.settingsChanged.connect(self.OnSettingsChanged)
        self.settsManager.settingsSaving.connect(self.OnSettingsSaving)

    def OnLayerSelected(self, title):
        self.layerName.SetValue(title)

    def _doLayout(self):
        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        dialogSizer.Add(
            self.settsManager,
            proportion=0,
            flag=wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT,
            border=5,
        )

        # connection settings
        settingsSizer = wx.StaticBoxSizer(self.settingsBox, wx.VERTICAL)

        serverSizer = wx.FlexGridSizer(cols=3, vgap=5, hgap=5)

        serverSizer.Add(self.serverText, flag=wx.ALIGN_CENTER_VERTICAL)
        serverSizer.AddGrowableCol(1)
        serverSizer.Add(self.server, flag=wx.EXPAND | wx.ALL)

        serverSizer.Add(self.btn_connect)

        settingsSizer.Add(
            serverSizer, proportion=0, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5
        )

        settingsSizer.Add(self.adv_conn, flag=wx.ALL | wx.EXPAND, border=5)

        dialogSizer.Add(
            settingsSizer,
            proportion=0,
            flag=wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT,
            border=5,
        )

        # layer name, parsed capabilities

        reqDataSizer = wx.BoxSizer(wx.VERTICAL)

        layerNameSizer = wx.StaticBoxSizer(self.layerNameBox, wx.HORIZONTAL)

        layerNameSizer.Add(
            self.layerNameText, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5
        )

        layerNameSizer.Add(self.layerName, flag=wx.EXPAND, proportion=1)

        reqDataSizer.Add(
            layerNameSizer, flag=wx.TOP | wx.LEFT | wx.RIGHT | wx.EXPAND, border=5
        )

        self.ch_ws_sizer = wx.BoxSizer(wx.VERTICAL)

        reqDataSizer.Add(
            self.ch_ws_sizer, proportion=0, flag=wx.TOP | wx.EXPAND, border=5
        )

        for ws in self.ws_panels.keys():
            reqDataSizer.Add(
                self.ws_panels[ws]["panel"],
                proportion=1,
                flag=wx.TOP | wx.LEFT | wx.RIGHT | wx.EXPAND,
                border=5,
            )
            self.ws_panels[ws]["panel"].Hide()

        dialogSizer.Add(self.reqDataPanel, proportion=1, flag=wx.EXPAND)

        self.reqDataPanel.SetSizer(reqDataSizer)
        self.reqDataPanel.Hide()

        # buttons
        self.btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)

        self.btnsizer.Add(
            self.btn_close, proportion=0, flag=wx.ALL | wx.ALIGN_CENTER, border=10
        )

        dialogSizer.Add(self.btnsizer, proportion=0, flag=wx.ALIGN_CENTER)

        # expand wxWidget wx.StatusBar
        statusbarSizer = wx.BoxSizer(wx.HORIZONTAL)
        statusbarSizer.Add(self.statusbar, proportion=1, flag=wx.EXPAND)
        dialogSizer.Add(statusbarSizer, proportion=0, flag=wx.EXPAND)

        self.SetSizer(dialogSizer)
        self.Layout()

        self.SetMinSize((550, -1))
        self.SetMaxSize((-1, self.GetBestSize()[1]))
        self.Fit()

    def MakeAdvConnPane(self, pane):
        """Create advanced connection settings pane"""
        self.usernameText = StaticText(parent=pane, id=wx.ID_ANY, label=_("Username:"))
        self.username = TextCtrl(parent=pane, id=wx.ID_ANY)

        self.passwText = StaticText(parent=pane, id=wx.ID_ANY, label=_("Password:"))
        self.password = TextCtrl(parent=pane, id=wx.ID_ANY, style=wx.TE_PASSWORD)

        # pane layout
        adv_conn_sizer = wx.BoxSizer(wx.VERTICAL)

        usernameSizer = wx.BoxSizer(wx.HORIZONTAL)

        usernameSizer.Add(self.usernameText, flag=wx.ALIGN_CENTER_VERTICAL, border=5)

        usernameSizer.Add(self.username, proportion=1, flag=wx.EXPAND, border=5)

        adv_conn_sizer.Add(usernameSizer, flag=wx.ALL | wx.EXPAND, border=5)

        passwSizer = wx.BoxSizer(wx.HORIZONTAL)

        passwSizer.Add(self.passwText, flag=wx.ALIGN_CENTER_VERTICAL, border=5)

        passwSizer.Add(self.password, proportion=1, flag=wx.EXPAND, border=5)

        adv_conn_sizer.Add(passwSizer, flag=wx.ALL | wx.EXPAND, border=5)

        pane.SetSizer(adv_conn_sizer)
        adv_conn_sizer.Fit(pane)

        pane.SetSizer(adv_conn_sizer)
        adv_conn_sizer.Fit(pane)

    def OnSettingsSaving(self, name):
        """Check if required data are filled before setting save is performed."""
        server = self.server.GetValue().strip()
        if not server:
            GMessage(
                parent=self,
                message=_("No data source defined, settings are not saved."),
            )
            return

        self.settsManager.SetDataToSave(
            (server, self.username.GetValue(), self.password.GetValue())
        )
        self.settsManager.SaveSettings(name)

    def OnSettingsChanged(self, data):
        """Update widgets according to chosen settings"""
        # data list: [server, username, password]
        if len(data) < 3:
            return

        self.server.SetValue(data[0])

        self.username.SetValue(data[1])
        self.password.SetValue(data[2])

        if data[1] or data[2]:
            self.adv_conn.Expand()
        else:
            self.adv_conn.Collapse(True)

        # clear content of the wxWidget wx.TextCtrl (Output layer
        # name:), based on changing default server selection in the
        # wxWidget wx.Choice
        if len(self.layerName.GetValue()) > 0:
            self.layerName.Clear()

    def OnClose(self, event):
        """Close the dialog"""
        if not self.IsModal():
            self.Destroy()
        event.Skip()

    def _getCapFiles(self):
        return {
            v["panel"].GetWebService(): v["panel"].GetCapFile()
            for v in self.ws_panels.values()
        }

    def OnServer(self, event):
        """Server settings edited"""
        value = event.GetString()
        if value:
            self.btn_connect.Enable(True)
        else:
            self.btn_connect.Enable(False)

        # clear content of the wxWidget wx.TextCtrl (Output Layer
        # name:), based on changing content of the wxWidget
        # wx.TextCtrl (Server:)
        self.layerName.Clear()

    def OnOutputLayerName(self, event):
        """Update layer name to web service panel"""
        lname = event.GetString()
        for v in self.ws_panels.values():
            v["panel"].SetOutputLayerName(lname.strip())

    def OnConnect(self, event):
        """Connect to the server"""
        server = self.server.GetValue().strip()

        self.ch_ws_sizer.Clear(True)

        if self.active_ws_panel is not None:
            self.reqDataPanel.Hide()
            for btn in self.run_btns:
                btn.Enable(False)
            self.active_ws_panel = None

            self.Layout()
            self.Fit()

        self.statusbar.SetStatusText(
            _("Connecting to <%s>...") % self.server.GetValue().strip()
        )

        # number of panels already connected
        self.finished_panels_num = 0
        for ws in self.ws_panels.keys():
            self.ws_panels[ws]["panel"].ConnectToServer(
                url=server,
                username=self.username.GetValue(),
                password=self.password.GetValue(),
            )
            self.ws_panels[ws]["panel"].Hide()

    def OnPanelCapParsed(self, error_msg):
        """Called when panel has downloaded and parsed capabilities file."""
        # how many web service panels are finished
        self.finished_panels_num += 1

        if error_msg:
            self.error_msgs += "\n" + error_msg

        # if all are finished, show panels, which succeeded in connection
        if self.finished_panels_num == len(self.ws_panels):
            self.UpdateDialogAfterConnection()

            # show error dialog only if connections to all web services were
            # unsuccessful
            if not self._getConnectedWS() and self.error_msgs:
                GError(self.error_msgs, parent=self)
            self.error_msgs = ""

            self.Layout()
            self.Fit()

    def _getConnectedWS(self):
        """
        :return: list of found web services on server (identified as keys in
                 self.ws_panels)
        """
        return [
            ws for ws, data in self.ws_panels.items() if data["panel"].IsConnected()
        ]

    def UpdateDialogAfterConnection(self):
        """Update dialog after all web service panels downloaded and parsed
        capabilities data."""
        avail_ws = {}
        conn_ws = self._getConnectedWS()

        for ws in conn_ws:
            avail_ws[ws] = self.ws_panels[ws]

        self.web_service_sel = []
        self.rb_choices = []

        # at least one web service found on server
        if len(avail_ws) > 0:
            self.reqDataPanel.Show()
            self.rb_order = ["WMS_1.1.1", "WMS_1.3.0", "WMTS", "OnEarth"]

            for ws in self.rb_order:
                if ws in avail_ws:
                    self.web_service_sel.append(ws)
                    self.rb_choices.append(avail_ws[ws]["label"])

            self.choose_ws_rb = wx.RadioBox(
                parent=self.reqDataPanel,
                id=wx.ID_ANY,
                label=_("Available web services"),
                pos=wx.DefaultPosition,
                choices=self.rb_choices,
                majorDimension=1,
                style=wx.RA_SPECIFY_ROWS,
            )

            self.Bind(wx.EVT_RADIOBOX, self.OnChooseWs, self.choose_ws_rb)
            self.ch_ws_sizer.Add(
                self.choose_ws_rb,
                flag=wx.TOP | wx.LEFT | wx.RIGHT | wx.EXPAND,
                border=5,
            )
            self._showWsPanel(self.web_service_sel[self.choose_ws_rb.GetSelection()])
            self.statusbar.SetStatusText(
                _("Connected to <%s>") % self.server.GetValue().strip()
            )
            for btn in self.run_btns:
                btn.Enable(True)
        # no web service found on server
        else:
            self.statusbar.SetStatusText(
                _("Unable to connect to <%s>") % self.server.GetValue().strip()
            )
            for btn in self.run_btns:
                btn.Enable(False)
            self.reqDataPanel.Hide()
            self.active_ws_panel = None

    def OnChooseWs(self, event):
        """Show panel corresponding to selected web service."""
        chosen_r = event.GetInt()
        self._showWsPanel(self.web_service_sel[chosen_r])

    def _showWsPanel(self, ws):
        """Helper function"""
        if self.active_ws_panel is not None:
            self.active_ws_panel.Hide()

        self.active_ws_panel = self.ws_panels[ws]["panel"]
        if not self.active_ws_panel.IsShown():
            self.active_ws_panel.Show()
            self.SetMaxSize((-1, -1))
            self.active_ws_panel.GetContainingSizer().Layout()

    def OnAdvConnPaneChanged(self, event):
        """Collapse search module box"""
        if self.adv_conn.IsExpanded():
            self.adv_conn.SetLabel(self.infoCollapseLabelCol)
        else:
            self.adv_conn.SetLabel(self.infoCollapseLabelExp)

        self.Layout()
        self.SetMaxSize((-1, self.GetBestSize()[1]))
        self.SendSizeEvent()
        self.Fit()


class AddWSDialog(WSDialogBase):
    """Dialog for adding web service layer."""

    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        WSDialogBase.__init__(
            self,
            parent,
            id=wx.ID_ANY,
            style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
            **kwargs,
        )

        self.SetTitle(_("Add web service layer"))

        self.parent = parent
        self.giface = giface
        self.btn_connect.SetDefault()

    def _createWidgets(self):
        WSDialogBase._createWidgets(self)

        self.btn_add = Button(parent=self, id=wx.ID_ANY, label=_("&Add layer"))
        self.btn_add.SetToolTip(
            _("Add selected web service layers as map layer into layer tree")
        )
        self.btn_add.Enable(False)

        self.run_btns.append(self.btn_add)

    def _doLayout(self):
        WSDialogBase._doLayout(self)

        self.btnsizer.Add(
            self.btn_add, proportion=0, flag=wx.ALL | wx.ALIGN_CENTER, border=10
        )

        # bindings
        self.btn_add.Bind(wx.EVT_BUTTON, self.OnAddLayer)

    def UpdateDialogAfterConnection(self):
        """Connect to the server"""
        WSDialogBase.UpdateDialogAfterConnection(self)

        if self._getConnectedWS():
            self.btn_add.SetDefault()
        else:
            self.btn_connect.SetDefault()

    def OnAddLayer(self, event):
        """Add web service layer."""
        # add layer
        if self.active_ws_panel is None:
            return

        lcmd = self.active_ws_panel.CreateCmd()
        if not lcmd:
            return

        # TODO: It is not clear how to do GetOptData in giface
        # knowing what GetOptData is doing might help
        # (maybe Get... is not the right name)
        # please fix giface if you know
        # tree -> giface
        # GetLayerTree -> GetLayerList
        # AddLayer -> AddLayer (but tree ones returns some layer,
        # giface ones nothing)
        # GetLayerInfo -> Layer object can by used instead
        # GetOptData -> unknown
        ltree = self.giface.GetLayerTree()

        active_ws = self.active_ws_panel.GetWebService()
        if "WMS" not in active_ws:
            cap_file = self.active_ws_panel.GetCapFile()
            cmd_cap_file = gs.tempfile()
            shutil.copyfile(cap_file, cmd_cap_file)
            lcmd.append("capfile=" + cmd_cap_file)

        layer = ltree.AddLayer(
            ltype="wms",
            lname=self.active_ws_panel.GetOutputLayerName(),
            lchecked=True,
            lcmd=lcmd,
        )

        ws_cap_files = self._getCapFiles()
        # create properties dialog
        cmd_list = ltree.GetLayerInfo(layer, "cmd")
        cmd = cmdlist_to_tuple(cmd_list)

        prop_win = WSPropertiesDialog(
            parent=self.parent,
            giface=self.giface,
            id=wx.ID_ANY,
            layer=layer,
            ws_cap_files=ws_cap_files,
            cmd=cmd,
        )

        prop_win.Hide()
        ltree.GetOptData(dcmd=None, layer=layer, params=None, propwin=prop_win)


class WSPropertiesDialog(WSDialogBase):
    """Dialog for editing web service properties."""

    def __init__(
        self,
        parent,
        giface,
        layer,
        ws_cap_files,
        cmd,
        id=wx.ID_ANY,
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        """
        :param giface: grass interface
        :param layer: layer tree item
        :param ws_cap_files: dict web service('WMS_1.1.1', 'WMS_1.3.0',
        'WMTS', 'OnEarth') : cap file path cap files, which will be parsed
        :param cmd: cmd to which dialog widgets will be initialized if
        it is possible (cmp parameters exists in parsed web service cap_file)
        """

        WSDialogBase.__init__(
            self,
            parent,
            id=wx.ID_ANY,
            style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
            **kwargs,
        )

        self.SetTitle(_("Web service layer properties"))

        self.layer = layer
        self.giface = giface

        # after web service panels are connected, set dialog widgets
        # according to cmd in this variable (if it is not None)
        self.cmd_to_set = None

        # store data needed for reverting
        self.revert_ws_cap_files = {}
        self.revert_cmd = cmd

        ws_cap = self._getWSfromCmd(cmd)
        for ws in self.ws_panels.keys():
            # cap file used in cmd will be deleted, thanks to the dialogs
            # destructor
            if ws == ws_cap and "capfile" in cmd[1]:
                self.revert_ws_cap_files[ws] = cmd[1]["capfile"]
                del ws_cap_files[ws]
            else:
                self.revert_ws_cap_files[ws] = gs.tempfile()

        self._setRevertCapFiles(ws_cap_files)

        self.LoadCapFiles(ws_cap_files=self.revert_ws_cap_files, cmd=cmd)
        self.btn_ok.SetDefault()

    def __del__(self):
        for f in self.revert_ws_cap_files.values():
            gs.try_remove(f)

    def _setRevertCapFiles(self, ws_cap_files):
        for ws, f in ws_cap_files.items():
            if os.path.isfile(f):
                shutil.copyfile(f, self.revert_ws_cap_files[ws])
            else:
                # delete file content
                f_o = open(f, "w")
                f_o.close()

    def _createWidgets(self):
        WSDialogBase._createWidgets(self)

        self.btn_apply = Button(parent=self, id=wx.ID_ANY, label=_("&Apply"))
        self.btn_apply.SetToolTip(_("Apply changes"))
        self.btn_apply.Enable(False)
        self.run_btns.append(self.btn_apply)

        self.btn_ok = Button(parent=self, id=wx.ID_ANY, label=_("&OK"))
        self.btn_ok.SetToolTip(_("Apply changes and close dialog"))
        self.btn_ok.Enable(False)
        self.run_btns.append(self.btn_ok)

    def _doLayout(self):
        WSDialogBase._doLayout(self)

        self.btnsizer.Add(
            self.btn_apply, proportion=0, flag=wx.ALL | wx.ALIGN_CENTER, border=10
        )

        self.btnsizer.Add(
            self.btn_ok, proportion=0, flag=wx.ALL | wx.ALIGN_CENTER, border=10
        )

        # bindings
        self.btn_apply.Bind(wx.EVT_BUTTON, self.OnApply)
        self.btn_ok.Bind(wx.EVT_BUTTON, self.OnSave)

    def LoadCapFiles(self, ws_cap_files, cmd):
        """Parse cap files and update dialog.

        For parameters description, see the constructor.
        """
        self.ch_ws_sizer.Clear(True)

        self.cmd_to_set = cmd

        self.finished_panels_num = 0

        conn = self._getServerConnFromCmd(cmd)

        self.server.SetValue(conn["url"])
        self.password.SetValue(conn["password"])
        self.username.SetValue(conn["username"])

        self.layerName.SetValue(cmd[1]["map"])

        for ws, data in self.ws_panels.items():
            cap_file = None

            if ws in ws_cap_files:
                cap_file = ws_cap_files[ws]

            data["panel"].ParseCapFile(
                url=conn["url"],
                username=conn["password"],
                password=conn["username"],
                cap_file=cap_file,
            )

    def _getServerConnFromCmd(self, cmd):
        """Get url/server/password from cmd tuple"""
        conn = {"url": "", "username": "", "password": ""}
        conn |= {k: cmd[1][k] for k in conn.keys() if k in cmd[1]}
        return conn

    def _apply(self):
        """Apply chosen values from widgets to web service layer."""
        lcmd = self.active_ws_panel.CreateCmd()
        if not lcmd:
            return

        active_ws = self.active_ws_panel.GetWebService()
        if "WMS" not in active_ws:
            lcmd.append("capfile=" + self.revert_ws_cap_files[active_ws])

        self.giface.GetLayerTree().GetOptData(
            dcmd=lcmd, layer=self.layer, params=True, propwin=self
        )

        # TODO use just list or tuple
        cmd = cmdlist_to_tuple(lcmd)
        self.revert_cmd = cmd
        self._setRevertCapFiles(self._getCapFiles())

        self.giface.updateMap.emit()

    def UpdateDialogAfterConnection(self):
        """Connect to the server"""
        WSDialogBase.UpdateDialogAfterConnection(self)
        if self._getConnectedWS():
            self.btn_ok.SetDefault()
        else:
            self.btn_connect.SetDefault()

    def OnApply(self, event):
        self._apply()

    def OnSave(self, event):
        self._apply()
        self._close()

    def OnClose(self, event):
        """Close dialog"""
        self._close()

    def _close(self):
        """Hide dialog"""
        self.Hide()
        self.LoadCapFiles(cmd=self.revert_cmd, ws_cap_files=self.revert_ws_cap_files)

    def OnPanelCapParsed(self, error_msg):
        """Called when panel has downloaded and parsed capabilities file."""
        WSDialogBase.OnPanelCapParsed(self, error_msg)

        if self.finished_panels_num == len(self.ws_panels):
            if self.cmd_to_set:
                self._updateWsPanelWidgetsByCmd(self.cmd_to_set)
                self.cmd_to_set = None

    def _updateWsPanelWidgetsByCmd(self, cmd):
        """Set values of  widgets according to parameters in cmd."""

        ws = self._getWSfromCmd(cmd)
        if self.ws_panels[ws]["panel"].IsConnected():
            self.choose_ws_rb.SetStringSelection(self.ws_panels[ws]["label"])
            self._showWsPanel(ws)
            self.ws_panels[ws]["panel"].UpdateWidgetsByCmd(cmd)

    def _getWSfromCmd(self, cmd):
        driver = cmd[1]["driver"]
        ws = driver.split("_")[0]

        if ws == "WMS":
            ws += "_" + cmd[1]["wms_version"]
        return ws


class SaveWMSLayerDialog(wx.Dialog):
    """Dialog for saving web service layer into GRASS vector/raster layer.

    .. todo::
        Implement saving data in region of map display.
    """

    def __init__(self, parent, layer, giface):
        wx.Dialog.__init__(
            self,
            parent=parent,
            title=("Save web service layer as raster map"),
            id=wx.ID_ANY,
        )

        self.layer = layer
        self._giface = giface

        self.cmd = self.layer.GetCmd()

        self.thread = CmdThread(self)
        self.cmdStdErr = GStderr(self)

        self._createWidgets()

    def _createWidgets(self):
        self.labels = {}
        self.params = {}

        self.labels["output"] = StaticText(
            parent=self, id=wx.ID_ANY, label=_("Name for output raster map:")
        )

        self.params["output"] = Select(
            parent=self,
            type="raster",
            mapsets=[gs.gisenv()["MAPSET"]],
            size=globalvar.DIALOG_GSELECT_SIZE,
        )

        self.regionStBoxLabel = StaticBox(
            parent=self, id=wx.ID_ANY, label=" %s " % _("Export region")
        )

        self.region_types_order = ["display", "comp", "named"]
        self.region_types = {}
        self.region_types["display"] = RadioButton(
            parent=self, label=_("Map display"), style=wx.RB_GROUP
        )
        self.region_types["comp"] = RadioButton(
            parent=self, label=_("Computational region")
        )
        self.region_types["named"] = RadioButton(parent=self, label=_("Named region"))
        self.region_types["display"].SetToolTip(
            _("Extent and resolution are based on Map Display geometry.")
        )
        self.region_types["comp"].SetToolTip(
            _("Extent and resolution are based on computational region.")
        )
        self.region_types["named"].SetToolTip(
            _("Extent and resolution are based on named region.")
        )
        self.region_types["display"].SetValue(True)  # set default as map display

        self.overwrite = wx.CheckBox(
            parent=self, id=wx.ID_ANY, label=_("Overwrite existing raster map")
        )

        self.named_reg_panel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.labels["region"] = StaticText(
            parent=self.named_reg_panel, id=wx.ID_ANY, label=_("Choose named region:")
        )

        self.params["region"] = Select(
            parent=self.named_reg_panel,
            type="region",
            size=globalvar.DIALOG_GSELECT_SIZE,
        )

        # buttons
        self.btn_close = Button(parent=self, id=wx.ID_CLOSE)
        self.SetEscapeId(self.btn_close.GetId())
        self.btn_close.SetToolTip(_("Close dialog"))

        self.btn_ok = Button(parent=self, label=_("&Save layer"))
        self.btn_ok.SetToolTip(_("Save web service layer as raster map"))

        # statusbar
        self.statusbar = wx.StatusBar(parent=self, id=wx.ID_ANY)

        self._layout()

    def _layout(self):
        self._border = wx.BoxSizer(wx.VERTICAL)
        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        regionSizer = wx.BoxSizer(wx.HORIZONTAL)

        dialogSizer.Add(
            self._addSelectSizer(title=self.labels["output"], sel=self.params["output"])
        )

        regionSizer = wx.StaticBoxSizer(self.regionStBoxLabel, wx.VERTICAL)

        regionTypeSizer = wx.BoxSizer(wx.HORIZONTAL)
        for r_type in self.region_types_order:
            regionTypeSizer.Add(self.region_types[r_type], flag=wx.RIGHT, border=8)

        regionSizer.Add(regionTypeSizer)

        self.named_reg_panel.SetSizer(
            self._addSelectSizer(title=self.labels["region"], sel=self.params["region"])
        )
        regionSizer.Add(self.named_reg_panel)
        self.named_reg_panel.Hide()

        dialogSizer.Add(regionSizer, flag=wx.EXPAND)

        dialogSizer.Add(self.overwrite, flag=wx.TOP, border=10)

        # buttons
        self.btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)

        self.btnsizer.Add(
            self.btn_close, proportion=0, flag=wx.ALL | wx.ALIGN_CENTER, border=10
        )

        self.btnsizer.Add(
            self.btn_ok, proportion=0, flag=wx.ALL | wx.ALIGN_CENTER, border=10
        )

        dialogSizer.Add(self.btnsizer, proportion=0, flag=wx.ALIGN_CENTER)

        self._border.Add(dialogSizer, proportion=0, flag=wx.ALL, border=5)

        self._border.Add(self.statusbar, proportion=0)

        self.SetSizer(self._border)
        self.Layout()
        self.Fit()

        # bindings
        self.btn_ok.Bind(wx.EVT_BUTTON, self.OnSave)

        self.Bind(EVT_CMD_DONE, self.OnCmdDone)
        self.Bind(EVT_CMD_OUTPUT, self.OnCmdOutput)

        for r_type in self.region_types_order:
            self.Bind(wx.EVT_RADIOBUTTON, self.OnRegionType, self.region_types[r_type])

    def _addSelectSizer(self, title, sel):
        """Helper layout function."""
        selSizer = wx.BoxSizer(orient=wx.VERTICAL)

        selTitleSizer = wx.BoxSizer(wx.HORIZONTAL)
        selTitleSizer.Add(
            title, proportion=1, flag=wx.LEFT | wx.TOP | wx.EXPAND, border=5
        )

        selSizer.Add(selTitleSizer, proportion=0, flag=wx.EXPAND)

        selSizer.Add(
            sel,
            proportion=1,
            flag=wx.EXPAND | wx.ALL,
            border=5,
        )

        return selSizer

    def OnRegionType(self, event):
        selected = event.GetEventObject()
        if selected == self.region_types["named"]:
            self.named_reg_panel.Show()
        else:
            self.named_reg_panel.Hide()

        self._border.Layout()
        self.Fit()

    def OnSave(self, event):
        """Import WMS raster data into GRASS as raster layer."""
        self.thread.abort(abortall=True)
        currmapset = gs.gisenv()["MAPSET"]

        self.output = self.params["output"].GetValue().strip()
        l_spl = self.output.strip().split("@")

        # check output layer
        msg = None
        if not self.output:
            msg = _("Missing output raster.")

        elif len(l_spl) > 1 and l_spl[1] != currmapset:
            msg = _("Output map can be added only to current mapset.")

        elif (
            not self.overwrite.IsChecked()
            and gs.find_file(self.output, "cell", ".")["fullname"]
        ):
            msg = _("Output map <%s> already exists") % self.output

        if msg:
            GMessage(parent=self, message=msg)
            return

        self.output = l_spl[0]

        # check region
        region = self.params["region"].GetValue().strip()
        reg_spl = region.strip().split("@")

        reg_mapset = "."
        if len(reg_spl) > 1:
            reg_mapset = reg_spl[1]

        if self.region_types["named"].GetValue():
            if not gs.find_file(reg_spl[0], "windows", reg_mapset)["fullname"]:
                msg = (
                    _("Region <%s> does not exist.") % self.params["region"].GetValue()
                )
                GWarning(parent=self, message=msg)
                return

        # create r.in.wms command
        cmd = ("r.in.wms", deepcopy(self.cmd[1]))

        if "map" in cmd[1]:
            del cmd[1]["map"]

        cmd[1]["output"] = self.output

        if self.overwrite.IsChecked():
            cmd[1]["overwrite"] = True

        env = os.environ.copy()
        if self.region_types["named"].GetValue():
            cmd[1]["region"] = region
        elif self.region_types["display"].GetValue():
            region = self._giface.GetMapWindow().GetMap().SetRegion()
            env["GRASS_REGION"] = region

        cmdList = cmdtuple_to_list(cmd)
        self.currentPid = self.thread.GetId()

        self.thread.RunCmd(cmdList, env=env, stderr=self.cmdStdErr)

        self.statusbar.SetStatusText(_("Downloading data..."))

    def OnCmdDone(self, event):
        """When data are fetched."""
        if event.pid != self.currentPid:
            return

        self._addLayer()
        self.statusbar.SetStatusText("")

    def _addLayer(self):
        """Add layer into layer tree."""
        llist = self._giface.GetLayerList()
        if len(llist.GetLayersByName(self.output)) == 0:
            cmd = ["d.rast", "map=" + self.output]
            llist.AddLayer(ltype="raster", name=self.output, cmd=cmd, checked=True)

    def OnCmdOutput(self, event):
        """Handle cmd output according to debug level."""
        if Debug.GetLevel() == 0:
            if event.type == "error":
                msg = _("Unable to fetch data.\n")
                msg += event.text
                GWarning(parent=self, message=msg)
        else:
            Debug.msg(1, event.text)
