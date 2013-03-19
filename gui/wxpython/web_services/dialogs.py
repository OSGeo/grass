"""!
@package web_services.dialogs

@brief Dialogs for web services.

List of classes:
 - dialogs::WSDialogBase
 - dialogs::AddWSDialog
 - dialogs::WSPropertiesDialog
 - dialogs::SaveWMSLayerDialog

(C) 2009-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Stepan Turek <stepan.turek seznam.cz>
"""

import wx

import os
import sys
import shutil

from copy      import deepcopy

import grass.script as grass

from core             import globalvar
from core.debug       import Debug
from core.ws          import RenderWMSMgr
from core.events      import gUpdateMap
from core.gcmd        import GMessage, GWarning, GError, RunCommand
from core.utils       import GetSettingsPath, CmdToTuple, CmdTupleToList
from core.gconsole    import CmdThread, GStderr, EVT_CMD_DONE, EVT_CMD_OUTPUT

from gui_core.gselect import Select
from gui_core.widgets import ManageSettingsWidget

from web_services.widgets import WSPanel

class WSDialogBase(wx.Dialog):
    """!Base class for web service dialogs. 
    """
    def __init__(self, parent, id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):

        wx.Dialog.__init__(self, parent, id, style = style, **kwargs)

        self.parent = parent 

        # contains panel for every web service on server
        self.ws_panels =  {'WMS_1.1.1'  : {'panel' : None,
                                           'label' : 'WMS 1.1.1'},
                           'WMS_1.3.0' : {'panel' : None,
                                          'label' : 'WMS 1.3.0'},
                           'WMTS' : {'panel' : None,
                                     'label' : 'WMTS'},
                           'OnEarth' : {'panel' : None,
                                        'label' : 'OnEarth'},
                          }

        #TODO: should be in file
        self.default_servers = { 'OSM-WMS-EUROPE' : ['http://129.206.228.72/cached/osm', '', ''],
                                 'irs.gis-lab.info (OSM)' : ['http://irs.gis-lab.info', '', ''],
                                 'NASA OnEarth' : ['http://onearth.jpl.nasa.gov/wms.cgi', '', '']
                               }

        # holds reference to web service panel which is showed
        self.active_ws_panel = None

        # buttons which are disabled when the dialog is not connected
        self.run_btns = []

        # stores error messages for GError dialog showed when all web service connections were unsuccessful
        self.error_msgs = ''

        self._createWidgets()
        self._doLayout()

    def _createWidgets(self):

        settingsFile = os.path.join(GetSettingsPath(), 'wxWS')

        self.settsManager = ManageSettingsWidget(parent = self, 
                                                 id = wx.ID_ANY,
                                                 settingsFile = settingsFile)

        self.settingsBox = wx.StaticBox(parent = self, 
                                        id = wx.ID_ANY,
                                        label = _(" Server settings "))
        
        self.serverText = wx.StaticText(parent = self, 
                                        id = wx.ID_ANY, label = _("Server:"))
        self.server  = wx.TextCtrl(parent = self, id = wx.ID_ANY)

        self.btn_connect = wx.Button(parent = self, 
                                     id = wx.ID_ANY, label = _("&Connect"))
        self.btn_connect.SetToolTipString(_("Connect to the server"))
        if not self.server.GetValue():
            self.btn_connect.Enable(False)

        self.infoCollapseLabelExp = _('Show advanced connection settings')
        self.infoCollapseLabelCol = _('Hide advanced connection settings')

        self.adv_conn = wx.CollapsiblePane(parent = self,
                                           label = self.infoCollapseLabelExp,
                                           style = wx.CP_DEFAULT_STYLE |
                                                   wx.CP_NO_TLW_RESIZE | wx.EXPAND)

        self.MakeAdvConnPane(pane = self.adv_conn.GetPane())
        self.adv_conn.Collapse(True)
        self.Bind(wx.EVT_COLLAPSIBLEPANE_CHANGED, self.OnAdvConnPaneChanged, self.adv_conn) 

        self.reqDataPanel = wx.Panel(parent = self, id = wx.ID_ANY)

        self.layerNameBox = wx.StaticBox(parent = self.reqDataPanel, 
                                         id = wx.ID_ANY,
                                         label = _(" Layer Manager Settings "))

        self.layerNameText = wx.StaticText(parent = self.reqDataPanel, id = wx.ID_ANY, 
                                           label = _("Output layer name:"))
        self.layerName = wx.TextCtrl(parent = self.reqDataPanel, id = wx.ID_ANY)

        for ws in self.ws_panels.iterkeys():
            self.ws_panels[ws]['panel'] =  WSPanel(parent = self.reqDataPanel,
                                                   web_service = ws)
            self.ws_panels[ws]['panel'].capParsed.connect(self.OnPanelCapParsed)

        # buttons
        self.btn_close = wx.Button(parent = self, id = wx.ID_CLOSE)
        self.btn_close.SetToolTipString(_("Close dialog"))
        
        # statusbar
        self.statusbar = wx.StatusBar(parent = self, id = wx.ID_ANY)

        # bindings
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        self.btn_connect.Bind(wx.EVT_BUTTON, self.OnConnect)

        self.server.Bind(wx.EVT_TEXT, self.OnServer)
        self.layerName.Bind(wx.EVT_TEXT, self.OnOutputLayerName)

        self.settsManager.settingsChanged.connect(self.OnSettingsChanged)
        self.settsManager.settingsLoaded.connect(self.OnSettingsLoaded)
        self.settsManager.settingsSaving.connect(self.OnSettingsSaving)

    def _doLayout(self):

        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        dialogSizer.Add(item = self.settsManager, proportion = 0,
                        flag = wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border = 5)
        
        # connectin settings
        settingsSizer = wx.StaticBoxSizer(self.settingsBox, wx.VERTICAL)
        
        serverSizer = wx.FlexGridSizer(cols = 3, vgap = 5, hgap = 5)

        serverSizer.Add(item = self.serverText,
                      flag = wx.ALIGN_CENTER_VERTICAL)
        serverSizer.AddGrowableCol(1)
        serverSizer.Add(item = self.server,
                      flag = wx.EXPAND | wx.ALL)

        serverSizer.Add(item = self.btn_connect)

        settingsSizer.Add(item = serverSizer, proportion = 0,
                          flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 5)
        
        settingsSizer.Add(item = self.adv_conn,
                          flag = wx.ALL | wx.EXPAND, border = 5)

        dialogSizer.Add(item = settingsSizer, proportion = 0,
                        flag = wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border = 5)

        # layer name, parsed capabilites

        reqDataSizer = wx.BoxSizer(wx.VERTICAL)

        layerNameSizer = wx.StaticBoxSizer(self.layerNameBox, wx.HORIZONTAL)

        layerNameSizer.Add(item = self.layerNameText,
                           flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL, border = 5)

        layerNameSizer.Add(item = self.layerName, 
                           flag = wx.EXPAND, proportion = 1)
 
        reqDataSizer.Add(item = layerNameSizer,
                         flag = wx.TOP | wx.LEFT | wx.RIGHT | wx.EXPAND, border = 5)

        self.ch_ws_sizer = wx.BoxSizer(wx.VERTICAL)

        reqDataSizer.Add(item = self.ch_ws_sizer, proportion = 0,
                         flag = wx.TOP | wx.EXPAND, border = 5)

        for ws in self.ws_panels.iterkeys():
            reqDataSizer.Add(item = self.ws_panels[ws]['panel'], proportion = 1,
                             flag = wx.TOP | wx.LEFT | wx.RIGHT | wx.EXPAND, border = 5)
            self.ws_panels[ws]['panel'].Hide()

        dialogSizer.Add(item = self.reqDataPanel, proportion = 1,
                        flag = wx.EXPAND)

        self.reqDataPanel.SetSizer(reqDataSizer)
        self.reqDataPanel.Hide()

        # buttons
        self.btnsizer = wx.BoxSizer(orient = wx.HORIZONTAL)

        self.btnsizer.Add(item = self.btn_close, proportion = 0,
                          flag = wx.ALL | wx.ALIGN_CENTER,
                          border = 10)
        
        dialogSizer.Add(item = self.btnsizer, proportion = 0,
                        flag = wx.ALIGN_CENTER)

        dialogSizer.Add(item = self.statusbar, proportion = 0)

        self.SetSizer(dialogSizer)
        self.Layout()

        self.SetMinSize((550, -1))
        self.SetMaxSize((-1, self.GetBestSize()[1]))
        self.Fit()

    def MakeAdvConnPane(self, pane):
        """!Create advanced connection settings pane
        """
        self.usernameText = wx.StaticText(parent = pane,
                                          id = wx.ID_ANY, label = _("Username:"))
        self.username  = wx.TextCtrl(parent = pane, id = wx.ID_ANY)

        self.passwText = wx.StaticText(parent = pane, 
                                        id = wx.ID_ANY, label = _("Password:"))
        self.password  = wx.TextCtrl(parent = pane, id = wx.ID_ANY,
                                     style = wx.TE_PASSWORD)

        # pane layout
        adv_conn_sizer = wx.BoxSizer(wx.VERTICAL)

        usernameSizer = wx.BoxSizer(wx.HORIZONTAL)

        usernameSizer.Add(item = self.usernameText,
                          flag = wx.ALIGN_CENTER_VERTICAL, border = 5)

        usernameSizer.Add(item = self.username, proportion = 1, 
                          flag = wx.EXPAND, border = 5)

        adv_conn_sizer.Add(item = usernameSizer,
                           flag = wx.ALL | wx.EXPAND, border = 5)

        passwSizer = wx.BoxSizer(wx.HORIZONTAL)

        passwSizer.Add(item = self.passwText,
                       flag = wx.ALIGN_CENTER_VERTICAL, border = 5)

        passwSizer.Add(item = self.password, proportion = 1, 
                       flag = wx.EXPAND, border = 5)

        adv_conn_sizer.Add(item = passwSizer,
                           flag = wx.ALL | wx.EXPAND, border = 5)
        
        pane.SetSizer(adv_conn_sizer)
        adv_conn_sizer.Fit(pane)

        pane.SetSizer(adv_conn_sizer)
        adv_conn_sizer.Fit(pane)

    def OnSettingsSaving(self, name):
        """!Check if required data are filled before setting save is performed.
        """
        server = self.server.GetValue().strip()
        if not server:
            GMessage(parent = self,
                     message = _("No data source defined, settings are not saved."))
            return

        self.settsManager.SetDataToSave((server,
                                         self.username.GetValue(),
                                         self.password.GetValue()))
        self.settsManager.SaveSettings(name)

    def OnSettingsChanged(self, data):
        """!Update widgets according to chosen settings"""
        # data list: [server, username, password]
        if len < 3:
            return

        self.server.SetValue(data[0])

        self.username.SetValue(data[1])
        self.password.SetValue(data[2])

        if data[1] or data[2]:
            self.adv_conn.Expand()
        else:
            self.adv_conn.Collapse(True)

    def OnSettingsLoaded(self, settings):
        """!If settings are empty set default servers
        """
        if not settings:
            self.settsManager.SetSettings(self.default_servers)

    def OnClose(self, event):
        """!Close the dialog
        """
        """!Close dialog"""
        if not self.IsModal():
            self.Destroy()
        event.Skip()

    def _getCapFiles(self):
        ws_cap_files = {}
        for v in self.ws_panels.itervalues():
            ws_cap_files[v['panel'].GetWebService()] = v['panel'].GetCapFile()

        return ws_cap_files

    def OnServer(self, event):
        """!Server settings edited
        """
        value = event.GetString()
        if value:
            self.btn_connect.Enable(True)
        else:
            self.btn_connect.Enable(False)
        
    def OnOutputLayerName(self, event):
        """!Update layer name to web service panel
        """
        lname = event.GetString()

        for v in self.ws_panels.itervalues():
            v['panel'].SetOutputLayerName(lname.strip())

    def OnConnect(self, event):
        """!Connect to the server
        """
        server = self.server.GetValue().strip()

        self.ch_ws_sizer.Clear(deleteWindows = True)

        if self.active_ws_panel is not None:
            self.reqDataPanel.Hide()
            for btn in self.run_btns:
                btn.Enable(False)
            self.active_ws_panel = None

            self.Layout()
            self.Fit()

        self.statusbar.SetStatusText(_("Connectig to <%s>..." % self.server.GetValue().strip()))

        # number of panels already connected
        self.finished_panels_num = 0
        for ws in self.ws_panels.iterkeys():
            self.ws_panels[ws]['panel'].ConnectToServer(url = server,
                                                        username = self.username.GetValue(),
                                                        password = self.password.GetValue())
            self.ws_panels[ws]['panel'].Hide()
        
    def OnPanelCapParsed(self, error_msg):
        """!Called when panel has downloaded and parsed capabilities file.
        """
        # how many web service panels are finished
        self.finished_panels_num +=  1

        if error_msg:
            self.error_msgs += '\n' + error_msg

        # if all are finished, show panels, which succeeded in connection
        if self.finished_panels_num == len(self.ws_panels):
            self.UpdateDialogAfterConnection()

            # show error dialog only if connections to all web services were unsuccessful
            if not self._getConnectedWS() and self.error_msgs:
                GError(self.error_msgs, parent = self)
            self.error_msgs = ''

            self.Layout()
            self.Fit()

    def _getConnectedWS(self):
        """
        @return list of found web services on server (identified as keys in self.ws_panels) 
        """
        conn_ws = []
        for ws, data in self.ws_panels.iteritems():
            if data['panel'].IsConnected():
                conn_ws.append(ws)

        return conn_ws

    def UpdateDialogAfterConnection(self):
        """!Update dialog after all web service panels downloaded and parsed capabilities data.
        """
        avail_ws = {}        
        conn_ws = self._getConnectedWS()

        for ws in conn_ws:
            avail_ws[ws] = self.ws_panels[ws]

        self.web_service_sel = []
        self.rb_choices = []

        # at least one web service found on server
        if len(avail_ws) > 0:
            self.reqDataPanel.Show()
            self.rb_order = ['WMS_1.1.1', 'WMS_1.3.0', 'WMTS', 'OnEarth']

            for ws in self.rb_order:

                if ws in avail_ws:
                    self.web_service_sel.append(ws)
                    self.rb_choices.append(avail_ws[ws]['label'])

            self.choose_ws_rb = wx.RadioBox(parent = self.reqDataPanel, id = wx.ID_ANY, 
                                            label = _("Available web services"), 
                                            pos = wx.DefaultPosition, choices = self.rb_choices, 
                                            majorDimension = 1, style = wx.RA_SPECIFY_ROWS)
        
            self.Bind(wx.EVT_RADIOBOX, self.OnChooseWs, self.choose_ws_rb)
            self.ch_ws_sizer.Add(item = self.choose_ws_rb,
                                 flag = wx.TOP | wx.LEFT | wx.RIGHT | wx.EXPAND, border = 5)
            self._showWsPanel(self.web_service_sel[self.choose_ws_rb.GetSelection()])
            self.statusbar.SetStatusText(_("Connected to <%s>" % self.server.GetValue().strip()))
            for btn in self.run_btns:
                btn.Enable(True)
        # no web service found on server
        else:
            self.statusbar.SetStatusText(_("Unable to connect to <%s>" % self.server.GetValue().strip()))
            for btn in self.run_btns:
                btn.Enable(False)
            self.reqDataPanel.Hide()
            self.active_ws_panel = None

    def OnChooseWs(self, event):
        """!Show panel corresponding to selected web service.
        """
        choosen_r = event.GetInt() 
        self._showWsPanel(self.web_service_sel[choosen_r])

    def _showWsPanel(self, ws):
        """!Helper function
        """
        if self.active_ws_panel is not None:
            self.active_ws_panel.Hide()

        self.active_ws_panel = self.ws_panels[ws]['panel']
        self.active_ws_panel.Show()
        self.SetMaxSize((-1, -1))
        self.Layout()
        self.Fit()

    def OnAdvConnPaneChanged(self, event):
        """!Collapse search module box
        """
        if self.adv_conn.IsExpanded():
            self.adv_conn.SetLabel(self.infoCollapseLabelCol)
        else:
            self.adv_conn.SetLabel(self.infoCollapseLabelExp)

        self.Layout()
        self.SetMaxSize((-1, self.GetBestSize()[1]))
        self.SendSizeEvent()
        self.Fit()

class AddWSDialog(WSDialogBase):
    """!Dialog for adding web service layer."""
    def __init__(self, parent, gmframe, id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):

        WSDialogBase.__init__(self, parent, id = wx.ID_ANY,
                              style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs)

        self.SetTitle(_("Add web service layer"))

        self.gmframe = gmframe
        self.btn_connect.SetDefault()

    def _createWidgets(self):

        WSDialogBase._createWidgets(self)

        self.btn_add = wx.Button(parent = self, id = wx.ID_ANY, label = _("&Add layer"))
        self.btn_add.SetToolTipString(_("Add selected web service layers as map layer into layer tree"))        
        self.btn_add.Enable(False)
        
        self.run_btns.append(self.btn_add)

    def _doLayout(self):

        WSDialogBase._doLayout(self)

        self.btnsizer.Add(item = self.btn_add, proportion = 0,
                          flag = wx.ALL | wx.ALIGN_CENTER,
                          border = 10)

        # bindings
        self.btn_add.Bind(wx.EVT_BUTTON, self.OnAddLayer)

    def UpdateDialogAfterConnection(self):
        """!Connect to the server
        """
        WSDialogBase.UpdateDialogAfterConnection(self)

        if self._getConnectedWS():
            self.btn_add.SetDefault()
        else:
            self.btn_connect.SetDefault()

    def OnAddLayer(self, event):
        """!Add web service layer.
        """
        # add layer
        if self.active_ws_panel is None:
            return 

        lcmd = self.active_ws_panel.CreateCmd()
        if not lcmd:
            return None

        ltree = self.gmframe.GetLayerTree()

        active_ws = self.active_ws_panel.GetWebService()
        if 'WMS' not in active_ws:
            cap_file =  self.active_ws_panel.GetCapFile()
            cmd_cap_file = grass.tempfile()
            shutil.copyfile(cap_file, cmd_cap_file)
            lcmd.append('capfile=' + cmd_cap_file)

        layer = ltree.AddLayer(ltype = 'wms', lname = self.layerName.GetValue(), 
                               lchecked = True, lcmd = lcmd)


        ws_cap_files = self._getCapFiles()
        # create properties dialog
        cmd_list = ltree.GetLayerInfo(layer,'cmd')
        cmd = CmdToTuple(cmd_list)

        prop_win = WSPropertiesDialog(parent = self.gmframe,
                                      id = wx.ID_ANY,
                                      layer = layer,
                                      ltree = ltree,
                                      ws_cap_files = ws_cap_files,
                                      cmd = cmd)

        prop_win.Hide()
        ltree.GetOptData(dcmd = None, layer = layer, 
                         params = None, propwin = prop_win)

class WSPropertiesDialog(WSDialogBase):
    """!Dialog for editing web service properties."""
    def __init__(self, parent, layer, ltree, ws_cap_files, cmd, id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        """
        @param layer - layer tree item
        @param ltree - layer tree reference
        @param ws_cap_files - dict web service('WMS_1.1.1', 'WMS_1.3.0', 'WMTS', 'OnEarth') : cap file path
                            - cap files, which will be parsed
        @param cmd - cmd to which dialog widgets will be initialized if it is possible 
                    (cmp parameters exists in parsed web service cap_file)
        """

        WSDialogBase.__init__(self, parent, id = wx.ID_ANY,
                               style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs)

        self.SetTitle(_("Web service layer properties"))

        self.ltree = ltree
        self.layer = layer

        # after web service panels are connected, set dialog widgets
        # according to cmd in this variable (if it is not None) 
        self.cmd_to_set = None

        # store data needed for reverting
        self.revert_ws_cap_files = {}
        self.revert_cmd = cmd

        ws_cap = self._getWSfromCmd(cmd)
        for ws in self.ws_panels.iterkeys():
            # cap file used in cmd will be deleted, thnaks to the dialogs destructor
            if ws == ws_cap and cmd[1].has_key('capfile'):
                self.revert_ws_cap_files[ws] = cmd[1]['capfile']
                del ws_cap_files[ws]
            else:
                self.revert_ws_cap_files[ws] = grass.tempfile()

        self._setRevertCapFiles(ws_cap_files)

        self.LoadCapFiles(ws_cap_files = self.revert_ws_cap_files, cmd = cmd)
        self.btn_ok.SetDefault()

    def __del__(self):
        for f in self.revert_ws_cap_files.itervalues():
            grass.try_remove(f)

    def _setRevertCapFiles(self, ws_cap_files):

        for ws, f in ws_cap_files.iteritems():
            if os.path.isfile(ws_cap_files[ws]):
                shutil.copyfile(f, self.revert_ws_cap_files[ws])
            else:
                # delete file content
                f_o = open(f, 'w')
                f_o.close()

    def _createWidgets(self):

        WSDialogBase._createWidgets(self)

        self.btn_apply = wx.Button(parent = self, id = wx.ID_ANY, label = _("&Apply"))
        self.btn_apply.SetToolTipString(_("Apply changes"))        
        self.btn_apply.Enable(False)
        self.run_btns.append(self.btn_apply)

        self.btn_ok = wx.Button(parent = self, id = wx.ID_ANY, label = _("&OK"))
        self.btn_ok.SetToolTipString(_("Apply changes and close dialog"))
        self.btn_ok.Enable(False)
        self.run_btns.append(self.btn_ok)

    def _doLayout(self):

        WSDialogBase._doLayout(self)

        self.btnsizer.Add(item = self.btn_apply, proportion = 0,
                          flag = wx.ALL | wx.ALIGN_CENTER,
                          border = 10)

        self.btnsizer.Add(item = self.btn_ok, proportion = 0,
                          flag = wx.ALL | wx.ALIGN_CENTER,
                          border = 10)

        # bindings
        self.btn_apply.Bind(wx.EVT_BUTTON, self.OnApply)
        self.btn_ok.Bind(wx.EVT_BUTTON, self.OnSave)

    def LoadCapFiles(self, ws_cap_files, cmd):
        """!Parse cap files and update dialog.

        For parameters description, see the constructor.
        """
        self.ch_ws_sizer.Clear(deleteWindows = True)

        self.cmd_to_set = cmd

        self.finished_panels_num = 0

        conn = self._getServerConnFromCmd(cmd)

        self.server.SetValue(conn['url'])
        self.password.SetValue(conn['password'])
        self.username.SetValue(conn['username'])

        self.layerName.SetValue(cmd[1]['map'])

        for ws, data in self.ws_panels.iteritems():
            cap_file = None

            if ws_cap_files.has_key(ws):
                cap_file = ws_cap_files[ws]

            data['panel'].ParseCapFile(url = conn['url'], 
                                       username = conn['password'], 
                                       password = conn['username'], 
                                       cap_file = cap_file)

    def _getServerConnFromCmd(self, cmd):
        """!Get url/server/passwod from cmd tuple 
        """
        conn = { 'url' : '', 'username' : '', 'password' : ''}
        
        for k in conn.iterkeys():
            if cmd[1].has_key(k):
                conn[k] = cmd[1][k]
        return conn

    def _apply(self):
        """!Apply chosen values from widgets to web service layer."""
        lcmd = self.active_ws_panel.CreateCmd()
        if not lcmd:
            return

        active_ws = self.active_ws_panel.GetWebService()
        if 'WMS' not in active_ws:
            lcmd.append('capfile=' + self.revert_ws_cap_files[active_ws])

        self.ltree.GetOptData(dcmd = lcmd, 
                              layer = self.layer, 
                              params = None,
                              propwin = self)

        #TODO use just list or tuple
        cmd = CmdToTuple(lcmd)
        self.revert_cmd = cmd
        self._setRevertCapFiles(self._getCapFiles())

        display = self.ltree.GetMapDisplay().GetMapWindow()
        event = gUpdateMap()
        wx.PostEvent(display, event)

    def UpdateDialogAfterConnection(self):
        """!Connect to the server
        """
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
        """!Close dialog"""
        self._close()

    def _close(self):
        """!Hide dialog"""
        self.Hide()
        self.LoadCapFiles(cmd = self.revert_cmd,
                          ws_cap_files = self.revert_ws_cap_files)

    def OnPanelCapParsed(self, error_msg):
        """!Called when panel has downloaded and parsed capabilities file.
        """
        WSDialogBase.OnPanelCapParsed(self, error_msg)

        if self.finished_panels_num == len(self.ws_panels):
            if self.cmd_to_set:
                self._updateWsPanelWidgetsByCmd(self.cmd_to_set)
                self.cmd_to_set = None

    def _updateWsPanelWidgetsByCmd(self, cmd):
        """!Set values of  widgets according to parameters in cmd.
        """

        ws = self._getWSfromCmd(cmd)
        if self.ws_panels[ws]['panel'].IsConnected():
            self.ws_panels[ws]['panel'].UpdateWidgetsByCmd(cmd)
            self.choose_ws_rb.SetStringSelection(self.ws_panels[ws]['label'])
            self._showWsPanel(ws)

    def _getWSfromCmd(self, cmd):
        driver = cmd[1]['driver']
        ws = driver.split('_')[0]
        
        if ws == 'WMS':
            ws += '_' + cmd[1]['wms_version']
        return ws

class SaveWMSLayerDialog(wx.Dialog):
    """!Dialog for saving web service layer into GRASS vector/raster layer.

    @todo Implement saving data in region of map display.
    """
    def __init__(self, parent, layer, ltree):
        
        wx.Dialog.__init__(self, parent = parent, title = ("Save web service layer"), id = wx.ID_ANY)

        self.layer = layer
        self.ltree = ltree

        self.cmd = self.layer.GetCmd()

        self.thread = CmdThread(self)
        self.cmdStdErr = GStderr(self)

        self._createWidgets()

    def _createWidgets(self):

        self.labels = {}
        self.params = {}

        self.labels['output'] = wx.StaticText(parent = self, id = wx.ID_ANY, label = _("Name for output raster layer:"))

        self.params['output'] = Select(parent = self, type = 'rast', mapsets = [grass.gisenv()['MAPSET']],
                                       size = globalvar.DIALOG_GSELECT_SIZE)

        self.regionStBoxLabel = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                             label = _("Region"))

        self.region_types_order = ['comp', 'named']
        self.region_types =  {}
        #self.region_types['map_display'] = wx.RadioButton(parent = self, id = wx.ID_ANY, label = 'Map display', style = wx.RB_GROUP )
        self.region_types['comp'] = wx.RadioButton(parent = self, id = wx.ID_ANY, label = 'Computational region')
        self.region_types['named'] = wx.RadioButton(parent = self, id = wx.ID_ANY, label = 'Named region')

        self.overwrite  = wx.CheckBox(parent = self, id = wx.ID_ANY,
                                      label = _("Overwrite existing layer"))

        self.named_reg_panel = wx.Panel(parent = self, id = wx.ID_ANY)
        self.labels['region'] = wx.StaticText(parent = self.named_reg_panel, id = wx.ID_ANY, 
                                             label = _("Choose named region:"))

        self.params['region'] = Select(parent = self.named_reg_panel, type = 'region',
                                       size = globalvar.DIALOG_GSELECT_SIZE)

        # buttons
        self.btn_close = wx.Button(parent = self, id = wx.ID_CLOSE)
        self.btn_close.SetToolTipString(_("Close dialog"))
        
        self.btn_ok = wx.Button(parent = self, id = wx.ID_ANY, label = _("&Save layer"))
        self.btn_ok.SetToolTipString(_("Add web service layer"))     

        # statusbar
        self.statusbar = wx.StatusBar(parent = self, id = wx.ID_ANY)

        self._layout()

    def _layout(self):

        border = wx.BoxSizer(wx.VERTICAL) 
        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        regionSizer = wx.BoxSizer(wx.HORIZONTAL)

        dialogSizer.Add(item = self._addSelectSizer(title = self.labels['output'], 
                                                    sel = self.params['output']))

        dialogSizer.Add(item = self.overwrite)

        regionSizer = wx.StaticBoxSizer(self.regionStBoxLabel, wx.VERTICAL)

        regionTypeSizer = wx.BoxSizer(wx.HORIZONTAL)
        for r_type in self.region_types_order:
            regionTypeSizer.Add(item = self.region_types[r_type])

        regionSizer.Add(item = regionTypeSizer)

        self.named_reg_panel.SetSizer(self._addSelectSizer(title = self.labels['region'],
                                                            sel = self.params['region']))
        regionSizer.Add(item = self.named_reg_panel)
        self.named_reg_panel.Hide()

        dialogSizer.Add(item = regionSizer, flag = wx.EXPAND)

        # buttons
        self.btnsizer = wx.BoxSizer(orient = wx.HORIZONTAL)

        self.btnsizer.Add(item = self.btn_close, proportion = 0,
                          flag = wx.ALL | wx.ALIGN_CENTER,
                          border = 10)
        
        self.btnsizer.Add(item = self.btn_ok, proportion = 0,
                          flag = wx.ALL | wx.ALIGN_CENTER,
                          border = 10)

        dialogSizer.Add(item = self.btnsizer, proportion = 0,
                        flag = wx.ALIGN_CENTER)

        border.Add(item = dialogSizer, proportion = 0,
                   flag = wx.ALL, border = 5)

        border.Add(item = self.statusbar, proportion = 0)

        self.SetSizer(border)
        self.Layout()
        self.Fit()

        # bindings
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_ok.Bind(wx.EVT_BUTTON, self.OnSave)

        self.Bind(EVT_CMD_DONE,   self.OnCmdDone)
        self.Bind(EVT_CMD_OUTPUT, self.OnCmdOutput)

        for r_type in self.region_types_order:
            self.Bind(wx.EVT_RADIOBUTTON, self.OnRegionType, self.region_types[r_type])

    def _addSelectSizer(self, title, sel): 
        """!Helper layout function.
        """
        selSizer = wx.BoxSizer(orient = wx.VERTICAL)

        selTitleSizer = wx.BoxSizer(wx.HORIZONTAL)
        selTitleSizer.Add(item = title, proportion = 1,
                          flag = wx.LEFT | wx.TOP | wx.EXPAND, border = 5)

        selSizer.Add(item = selTitleSizer, proportion = 0,
                     flag = wx.EXPAND)

        selSizer.Add(item = sel, proportion = 1,
                     flag = wx.EXPAND | wx.ALL| wx.ALIGN_CENTER_VERTICAL,
                     border = 5)

        return selSizer

    def OnClose(self, event):
        """!Close dialog
        """
        if not self.IsModal():
            self.Destroy()
        event.Skip()

    def OnRegionType(self, event):
        
        selected = event.GetEventObject()
        if selected == self.region_types['named']:
            self.named_reg_panel.Show()
        else:
            self.named_reg_panel.Hide()

        self.Layout()
        self.Fit()

    def OnSave(self, event):
        """!Import WMS raster data into GRASS as raster layer.
        """
        self.thread.abort(abortall = True)
        currmapset = grass.gisenv()['MAPSET']
        
        self.output = self.params['output'].GetValue().strip()
        l_spl = self.output.strip().split("@")

        # check output layer
        msg = None
        if not self.output:
            msg = _('Missing output raster.')

        elif len(l_spl) > 1 and \
             l_spl[1] != currmapset:
                msg = _('Output map can be added only to current mapset.')

        elif not self.overwrite.IsChecked() and\
            grass.find_file(self.output, 'cell', '.')['fullname']:
            msg = _('Output map <%s> already exists' % self.output)

        if msg:
            GMessage(parent = self,
                     message = msg)
            return

        self.output = l_spl[0]


        # check region
        region = self.params['region'].GetValue().strip()
        reg_spl = region.strip().split("@")

        reg_mapset = '.'
        if len(reg_spl) > 1:
            reg_mapset = reg_spl[1]

        if self.region_types['comp'].GetValue() == 1: 
            pass
        elif grass.find_file(reg_spl[0], 'region', reg_mapset)['fullname']:
            msg = _('Region <%s> does not exists.' % self.params['region'].GetValue())
            GWarning(parent = self,
                     message = msg)
            return

        # create r.in.wms command
        cmd = ('r.in.wms', deepcopy(self.cmd[1]))

        if cmd[1].has_key('map'):
            del cmd[1]['map']

        cmd[1]['output'] = self.output

        if self.overwrite.IsChecked():
            cmd[1]['overwrite'] = True

        if self.region_types['named'].GetValue() == 1:
            cmd[1]['region'] = region

        cmdList = CmdTupleToList(cmd)
        self.currentPid = self.thread.GetId()

        self.thread.RunCmd(cmdList, stderr = self.cmdStdErr)

        self.statusbar.SetStatusText(_("Downloading data..."))

    def OnCmdDone(self, event):
        """!When data are fetched.
        """
        if event.pid != self.currentPid:
            return

        self._addLayer()
        self.statusbar.SetStatusText("")

    def _addLayer(self):
        """!Add layer into layer tree.
        """
        if self.ltree.FindItemByData(key = 'name', value = self.output) is None: 
            cmd = ['d.rast', 'map=' + self.output]
            self.ltree.AddLayer(ltype = 'raster',
                                lname = self.output,
                                lcmd = cmd,
                                lchecked = True)

    def OnCmdOutput(self, event):
        """!Handle cmd output according to debug level.
        """
        if Debug.GetLevel() == 0:
            if event.type == 'error':
                msg = _('Unable to fetch data.\n')
                msg += event.text
                GWarning(parent = self,
                         message = msg)
        else:
            Debug.msg(1, event.text)
