"""!
@package ogc_services.py

@brief Dialogs for OGC services

Currently only implemeted WMS.

List of classes:
 - WMSDialog
 - LayersList

(C) 2009 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import wx
from wx.gizmos import TreeListCtrl
import wx.lib.mixins.listctrl as listmix

import gcmd

from preferences import globalSettings as UserSettings

class WMSDialog(wx.Dialog):
    def __init__(self, parent, service = 'wms',
                 id=wx.ID_ANY,
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        """!Dialog to import data from WMS server"""
        self.parent  = parent  # GMFrame 
        self.service = service # currently only WMS is implemented
        
        wx.Dialog.__init__(self, parent, id, style=style)
        if self.service == 'wms':
            self.SetTitle(_("Import data from WMS server"))
            
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.__createWidgets()
        
        self.__doLayout()

        self.SetMinSize((550, 400))
        
    def __createWidgets(self):
        """!Create dialog widgets"""
        #
        # settings
        #
        self.settingsBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                        label = _(" Server settings "))

        self.serverText = wx.StaticText(parent = self.panel, id = wx.ID_ANY, label = _("Server:"))
        self.server  = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY)
        
        #
        # list of layers
        #
        self.layersBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                label=_(" List of layers "))

        self.list = LayersList(self.panel)
        self.list.LoadData()

        self.add = wx.CheckBox(parent=self.panel, id=wx.ID_ANY,
                               label=_("Add imported layers into layer tree"))
        self.add.SetValue(UserSettings.Get(group='cmd', key='addNewLayer', subkey='enabled'))
                
        #
        # buttons
        #
        # cancel
        self.btn_cancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btn_cancel.SetToolTipString(_("Close dialog"))
        # connect
        self.btn_connect = wx.Button(parent = self.panel, id = wx.ID_ANY, label = _("&Connect"))
        self.btn_connect.SetToolTipString(_("Connect to the server"))
        self.btn_connect.SetDefault()
        if not self.server.GetValue():
            self.btn_connect.Enable(False)
        # import
        self.btn_import = wx.Button(parent = self.panel, id = wx.ID_OK, label = _("&Import"))
        self.btn_import.SetToolTipString(_("Import selected layers"))
        self.btn_import.Enable(False)
        
        #
        # bindings
        #
        self.btn_cancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        self.btn_connect.Bind(wx.EVT_BUTTON, self.OnConnect)
        self.server.Bind(wx.EVT_TEXT, self.OnServer)
        
    def __doLayout(self):
        """!Do dialog layout"""
        dialogSizer = wx.BoxSizer(wx.VERTICAL)
        
        #
        # settings
        #
        settingsSizer = wx.StaticBoxSizer(self.settingsBox, wx.HORIZONTAL)
        
        gridSizer = wx.FlexGridSizer(cols=2, vgap=5, hgap=5)

        gridSizer.Add(item=self.serverText,
                      flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.AddGrowableCol(1)
        gridSizer.Add(item=self.server,
                      flag=wx.EXPAND | wx.ALL)
        
        settingsSizer.Add(item=gridSizer, proportion=1,
                       flag=wx.EXPAND | wx.ALL)
        
        dialogSizer.Add(item=settingsSizer, proportion=0,
                        flag=wx.ALL | wx.EXPAND, border=5)
        
        #
        # list of layers
        #
        layersSizer = wx.StaticBoxSizer(self.layersBox, wx.HORIZONTAL)

        layersSizer.Add(item=self.list, proportion=1,
                        flag=wx.ALL | wx.EXPAND, border=5)
        
        dialogSizer.Add(item=layersSizer, proportion=1,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)

        dialogSizer.Add(item=self.add, proportion=0,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)
        
        #
        # buttons
        #
        btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)

        btnsizer.Add(item=self.btn_cancel, proportion=0,
                     flag=wx.ALL | wx.ALIGN_CENTER,
                     border=10)
        
        btnsizer.Add(item=self.btn_connect, proportion=0,
                     flag=wx.ALL | wx.ALIGN_CENTER,
                     border=10)
        
        btnsizer.Add(item=self.btn_import, proportion=0,
                     flag=wx.ALL | wx.ALIGN_CENTER,
                     border=10)
        
        dialogSizer.Add(item=btnsizer, proportion=0,
                        flag=wx.ALIGN_CENTER)
        
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)
        self.Layout()

    def OnCancel(self, event):
        """!Button 'Cancel' pressed -> close the dialog"""
        self.Close()

    def OnConnect(self, event):
        """!Button 'Connect' pressed"""
        server = self.server.GetValue()
        if not server:
            self.btn_import.Enable(False)
            return # not reachable

        layers = {}
        ret = gcmd.RunCommand('r.in.wms',
                              quiet = True,
                              parent = self,
                              read = True,
                              flags = 'l',
                              mapserver = server)
        
        if not ret:
            self.list.LoadData()
            self.btn_import.Enable(False)
            return # no layers found
        
        lastLayer = lastStyle = ''
        for line in ret.splitlines():
            try:
                key, value = line.split(':', 1)
            except ValueError:
                continue
            key = key.strip().lower()
            value = value.strip()
            
            if key == 'layer':
                layers[value] = {}
                lastLayer = value
            elif key == 'title':
                layers[lastLayer][key] = value
            elif key == 'style':
                if not layers[lastLayer].has_key('style'):
                    layers[lastLayer]['style'] = {}
                layers[lastLayer]['style'][value] = ''
                lastStyle = value
            elif key == 'style title':
                layers[lastLayer]['style'][lastStyle] = value
        
        # update list of layers
        self.list.LoadData(layers)
        
        if len(layers.keys()) > 0:
            self.btn_import.Enable(True)
        else:
            self.btn_import.Enable(False)
        
    def OnServer(self, event):
        """!Server settings changed"""
        value = event.GetString()
        if value:
            self.btn_connect.Enable(True)
        else:
            self.btn_connect.Enable(False)
        
    def GetLayers(self):
        """!Get list of selected layers/styles to be imported"""
        return self.list.GetSelectedLayers()

    def GetSettings(self):
        """!Get connection settings"""
        return { 'server' : self.server.GetValue() }
    
class LayersList(TreeListCtrl, listmix.ListCtrlAutoWidthMixin):
    def __init__(self, parent, pos=wx.DefaultPosition):
        """!List of layers to be imported (dxf, shp...)"""
        self.parent = parent
        
        TreeListCtrl.__init__(self, parent, wx.ID_ANY,
                              style = wx.TR_DEFAULT_STYLE | wx.TR_HIDE_ROOT |
                              wx.TR_FULL_ROW_HIGHLIGHT | wx.TR_MULTIPLE)
        
        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
        self.AddColumn(_('Layer / Style'))
        self.AddColumn(_('Title'))
        self.SetMainColumn(0) # column with the tree
        self.SetColumnWidth(0, 175)
        
        self.root = None
        
    def LoadData(self, data = {}):
        """!Load data into list"""
        # detete first all items
        self.DeleteAllItems()
        self.root = self.AddRoot(_("Layers"))

        layers = data.keys()
        if not layers:
            return
        
        layers.sort()

        for layer in layers:
            title = data[layer]['title']
            lchild = self.AppendItem(self.root, layer)
            self.SetItemText(lchild, title, 1)
            if data[layer].has_key('style'):
                styles = data[layer]['style'].keys()
                if not styles:
                    continue
                styles.sort()
                for style in styles:
                    title = data[layer]['style'][style]
                    schild = self.AppendItem(lchild, style)
                    self.SetItemText(schild, title, 1)
        
        self.Expand(self.root)
        
    def GetItemCount(self):
        """!Required for listmix.ListCtrlAutoWidthMixin"""
        return 0

    def GetCountPerPage(self):
        """!Required for listmix.ListCtrlAutoWidthMixin"""
        return 0

    def GetSelectedLayers(self):
        """!Get selected layers/styles"""
        layers = dict()

        for item in self.GetSelections():
            parent = self.GetItemParent(item)
            if parent == self.root: # -> layer
                layer = self.GetItemText(item, 0)
                layers[layer] = list()
                sitem, cookie = self.GetFirstChild(item)
                while sitem:
                    layers[layer].append(self.GetItemText(sitem, 0))
                    sitem, cookie = self.GetNextChild(item, cookie)
            else: # -> style
                layer = self.GetItemText(parent, 0)
                layers[layer] = list()
                layers[layer].append(self.GetItemText(item, 0))
        
        return layers
