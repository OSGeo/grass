"""!
@package gui_core.dialogs

@brief Various dialogs used in wxGUI.

List of classes:
 - dialogs::SimpleDialog
 - dialogs::LocationDialog
 - dialogs::MapsetDialog
 - dialogs::NewVectorDialog
 - dialogs::SavedRegion
 - dialogs::GroupDialog
 - dialogs::MapLayersDialog
 - dialogs::ImportDialog
 - dialogs::GdalImportDialog
 - dialogs::GdalOutputDialog
 - dialogs::DxfImportDialog
 - dialogs::LayersList (used by MultiImport) 
 - dialogs::SetOpacityDialog
 - dialogs::ImageSizeDialog
 - dialogs::SqlQueryFrame
 - dialogs::SymbolDialog

(C) 2008-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com> (GroupDialog, SymbolDialog)
"""

import os
import sys
import re
from bisect import bisect

import wx
import wx.lib.filebrowsebutton as filebrowse
import wx.lib.mixins.listctrl as listmix

from grass.script import core as grass
from grass.script import task as gtask

from grass.pydispatch.signal import Signal

from core             import globalvar
from core.gcmd        import GError, RunCommand, GMessage
from gui_core.gselect import LocationSelect, MapsetSelect, Select, OgrTypeSelect, GdalSelect, MapsetSelect
from gui_core.widgets import SingleSymbolPanel, GListCtrl, SimpleValidator
from core.utils       import GetValidLayerName, _
from core.settings    import UserSettings, GetDisplayVectSettings
from core.debug       import Debug

class SimpleDialog(wx.Dialog):
    def __init__(self, parent, title, id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!General dialog to choose given element (location, mapset, vector map, etc.)
        
        @param parent window
        @param title window title
        """
        wx.Dialog.__init__(self, parent, id, title, style = style, **kwargs)
        self.SetExtraStyle(wx.WS_EX_VALIDATE_RECURSIVELY)
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btnOK     = wx.Button(parent = self.panel, id = wx.ID_OK)
        self.btnOK.SetDefault()
        self.__layout()
        self.warning = _("Required item is not set.")

    def __layout(self):
        """!Do layout"""
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        
        self.dataSizer = wx.BoxSizer(wx.VERTICAL)
        
        # self.informLabel = wx.StaticText(self.panel, id = wx.ID_ANY)
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()
        
        self.sizer.Add(item = self.dataSizer, proportion = 1,
                       flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        # self.sizer.Add(item = self.informLabel, proportion = 0, flag = wx.ALL, border = 5)
        self.sizer.Add(item = btnSizer, proportion = 0,
                       flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)

    def ValidatorCallback(self, win):
        GMessage(parent = self, message = self.warning)
        # self.informLabel.SetForegroundColour(wx.Colour(255, 0, 0))
        # self.informLabel.SetLabel(self.warning)


class LocationDialog(SimpleDialog):
    """!Dialog used to select location"""
    def __init__(self, parent, title = _("Select GRASS location and mapset")):
        SimpleDialog.__init__(self, parent, title)

        self.element1 = LocationSelect(parent = self.panel, id = wx.ID_ANY,
                                      size = globalvar.DIALOG_GSELECT_SIZE,
                                      validator = SimpleValidator(callback = self.ValidatorCallback))
        self.element1.Bind(wx.EVT_TEXT, self.OnLocation)
        self.element2 = MapsetSelect(parent = self.panel, id = wx.ID_ANY,
                                     size = globalvar.DIALOG_GSELECT_SIZE,
                                     setItems = False, skipCurrent = True,
                                     validator = SimpleValidator(callback = self.ValidatorCallback))
        self.element1.SetFocus()
        self.warning = _("Location or mapset is missing.")
        self._layout()
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """!Do layout"""
        self.dataSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                                label = _("Name of GRASS location:")),
                           proportion = 0, flag = wx.ALL, border = 1)
        self.dataSizer.Add(self.element1, proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)
 
        self.dataSizer.Add(wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                         label = _("Name of mapset:")), proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)

        self.dataSizer.Add(self.element2, proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)
       
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def OnLocation(self, event):
        """!Select mapset given location name"""
        location = event.GetString()
        
        if location:
            dbase = grass.gisenv()['GISDBASE']
            self.element2.UpdateItems(dbase = dbase, location = location)
            self.element2.SetSelection(0)
            mapset = self.element2.GetStringSelection()

    def GetValues(self):
        """!Get location, mapset"""
        return (self.element1.GetValue(), self.element2.GetValue())
    
class MapsetDialog(SimpleDialog):
    """!Dialog used to select mapset"""
    def __init__(self, parent, title = _("Select mapset in GRASS location"),
                 location = None):
        SimpleDialog.__init__(self, parent, title)

        if location:
            self.SetTitle(self.GetTitle() + ' <%s>' % location)
        else:
            self.SetTitle(self.GetTitle() + ' <%s>' % grass.gisenv()['LOCATION_NAME'])
        
        self.element = MapsetSelect(parent = self.panel, id = wx.ID_ANY, skipCurrent = True,
                                    size = globalvar.DIALOG_GSELECT_SIZE,
                                    validator = SimpleValidator(callback = self.ValidatorCallback))
        
        self.element.SetFocus()
        self.warning = _("Name of mapset is missing.")
        
        self._layout()
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """!Do layout"""
        self.dataSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                                label = _("Name of mapset:")),
                           proportion = 0, flag = wx.ALL, border = 1)
        self.dataSizer.Add(self.element, proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetMapset(self):
        return self.element.GetValue()
    
class NewVectorDialog(SimpleDialog):
    def __init__(self, parent, title = _("Create new vector map"),
                 disableAdd = False, disableTable = False, showType = False):
        """!Dialog for creating new vector map

        @param parent parent window
        @param title window title
        @param disableAdd disable 'add layer' checkbox
        @param disableTable disable 'create table' checkbox
        @param showType True to show feature type selector (used for creating new empty OGR layers)
        
        @return dialog instance
        """
        SimpleDialog.__init__(self, parent, title)
        
        self.element = Select(parent = self.panel, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                              type = 'vector', mapsets = [grass.gisenv()['MAPSET'],],
                              validator = SimpleValidator(callback = self.ValidatorCallback))
        self.element.SetFocus()
        # determine output format
        if showType:
            self.ftype = OgrTypeSelect(parent = self, panel = self.panel)
        else:
            self.ftype = None
        
        self.table = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                 label = _("Create attribute table"))
        self.table.SetValue(True)
        if disableTable:
            self.table.Enable(False)
        
        if showType:
            self.keycol = None
        else:
            self.keycol = wx.TextCtrl(parent = self.panel, id =  wx.ID_ANY,
                                      size = globalvar.DIALOG_SPIN_SIZE)
            self.keycol.SetValue(UserSettings.Get(group = 'atm', key = 'keycolumn', subkey = 'value'))
            if disableTable:
                self.keycol.Enable(False)
        
        self.addbox = wx.CheckBox(parent = self.panel,
                                  label = _('Add created map into layer tree'), style = wx.NO_BORDER)
        if disableAdd:
            self.addbox.SetValue(True)
            self.addbox.Enable(False)
        else:
            self.addbox.SetValue(UserSettings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))

        self.table.Bind(wx.EVT_CHECKBOX, self.OnTable)
        
        self.warning = _("Name of new vector map is missing.")
        self._layout()
        self.SetMinSize(self.GetSize())
        
    def OnTable(self, event):
        if self.keycol:
            self.keycol.Enable(event.IsChecked())
        
    def _layout(self):
        """!Do layout"""
        self.dataSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                                label = _("Name for new vector map:")),
                           proportion = 0, flag = wx.ALL, border = 1)
        self.dataSizer.Add(item = self.element, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 1)
        if self.ftype:
            self.dataSizer.AddSpacer(1)
            self.dataSizer.Add(item = self.ftype, proportion = 0,
                               flag = wx.EXPAND | wx.ALL, border = 1)
        
        self.dataSizer.Add(item = self.table, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 1)
        
        if self.keycol:
            keySizer = wx.BoxSizer(wx.HORIZONTAL)
            keySizer.Add(item = wx.StaticText(parent = self.panel, label = _("Key column:")),
                         proportion = 0,
                         flag = wx.ALIGN_CENTER_VERTICAL)
            keySizer.AddSpacer(10)
            keySizer.Add(item = self.keycol, proportion = 0,
                         flag = wx.ALIGN_RIGHT)
            self.dataSizer.Add(item = keySizer, proportion = 1,
                               flag = wx.EXPAND | wx.ALL, border = 1)
            
        self.dataSizer.AddSpacer(5)
        
        self.dataSizer.Add(item = self.addbox, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 1)
        
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetName(self, full = False):
        """!Get name of vector map to be created

        @param full True to get fully qualified name
        """
        name = self.element.GetValue()
        if full:
            if '@' in name:
                return name
            else:
                return name + '@' + grass.gisenv()['MAPSET']
        
        return name.split('@', 1)[0]

    def GetKey(self):
        """!Get key column name"""
        if self.keycol:
            return self.keycol.GetValue()
        return UserSettings.Get(group = 'atm', key = 'keycolumn', subkey = 'value')
    
    def IsChecked(self, key):
        """!Get dialog properties

        @param key window key ('add', 'table')

        @return True/False
        @return None on error
        """
        if key == 'add':
            return self.addbox.IsChecked()
        elif key == 'table':
            return self.table.IsChecked()
        
        return None
    
    def GetFeatureType(self):
        """!Get feature type for OGR

        @return feature type as string
        @return None for native format
        """
        if self.ftype:
            return self.ftype.GetType()
        
        return None

def CreateNewVector(parent, cmd, title = _('Create new vector map'),
                    exceptMap=None, giface=None,
                    disableAdd = False, disableTable = False):
    """!Create new vector map layer
    
    @param cmd (prog, **kwargs)
    @param title window title
    @param exceptMap list of maps to be excepted
    @param log
    @param disableAdd disable 'add layer' checkbox
    @param disableTable disable 'create table' checkbox
    
    @return dialog instance
    @return None on error
    """
    vExternalOut = grass.parse_command('v.external.out', flags = 'g')
    isNative = vExternalOut['format'] == 'native'
    if cmd[0] == 'v.edit' and not isNative:
        showType = True
    else:
        showType = False
    dlg = NewVectorDialog(parent, title = title,
                          disableAdd = disableAdd, disableTable = disableTable,
                          showType = showType)
    
    if dlg.ShowModal() != wx.ID_OK:
        dlg.Destroy()
        return None

    outmap = dlg.GetName()
    key    = dlg.GetKey()
    if outmap == exceptMap:
        GError(parent = parent,
               message = _("Unable to create vector map <%s>.") % outmap)
        dlg.Destroy()
        return None
    if dlg.table.IsEnabled() and not key:
        GError(parent = parent,
               message = _("Invalid or empty key column.\n"
                           "Unable to create vector map <%s>.") % outmap)
        dlg.Destroy()
        return
        
    if outmap == '': # should not happen
        dlg.Destroy()
        return None

    # update cmd -> output name defined
    cmd[1][cmd[2]] = outmap
    if showType:
        cmd[1]['type'] = dlg.GetFeatureType()
        
    if isNative:
        listOfVectors = grass.list_grouped('vect')[grass.gisenv()['MAPSET']]
    else:
        listOfVectors = RunCommand('v.external',
                                   quiet = True,
                                   parent = parent,
                                   read = True,
                                   flags = 'l',
                                   dsn = vExternalOut['dsn']).splitlines()
    
    overwrite = False
    if not UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled') and \
            outmap in listOfVectors:
        dlgOw = wx.MessageDialog(parent, message = _("Vector map <%s> already exists "
                                                     "in the current mapset. "
                                                     "Do you want to overwrite it?") % outmap,
                                 caption = _("Overwrite?"),
                                 style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
        if dlgOw.ShowModal() == wx.ID_YES:
            overwrite = True
        else:
            dlgOw.Destroy()
            dlg.Destroy()
            return None
        
    if UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'):
        overwrite = True
        
    ret = RunCommand(prog = cmd[0],
                     parent = parent,
                     overwrite = overwrite,
                     **cmd[1])
    if ret != 0:
        dlg.Destroy()
        return None
    
    if not isNative:
        # create link for OGR layers
        RunCommand('v.external',
                   overwrite = overwrite,
                   parent = parent,
                   dsn = vExternalOut['dsn'],
                   layer = outmap)
        
    # create attribute table
    if dlg.table.IsEnabled() and dlg.table.IsChecked():
        if isNative:
            sql = 'CREATE TABLE %s (%s INTEGER)' % (outmap, key)
            
            RunCommand('db.connect',
                       flags = 'c')
            
            Debug.msg(1, "SQL: %s" % sql)
            RunCommand('db.execute',
                       quiet = True,
                       parent = parent,
                       input = '-',
                       stdin = sql)
            
            RunCommand('v.db.connect',
                       quiet = True,
                       parent = parent,
                       map = outmap,
                       table = outmap,
                       key = key,
                       layer = '1')
        # TODO: how to deal with attribute tables for OGR layers?
            
    # return fully qualified map name
    if '@' not in outmap:
        outmap += '@' + grass.gisenv()['MAPSET']
        
    if giface:
        giface.WriteLog(_("New vector map <%s> created") % outmap)
        
    return dlg

class SavedRegion(wx.Dialog):
    def __init__(self, parent, title, id = wx.ID_ANY, loadsave = 'load',
                 **kwargs):
        """!Loading or saving of display extents to saved region file

        @param loadsave load or save region?
        """
        wx.Dialog.__init__(self, parent, id, title, **kwargs)
        
        self.loadsave = loadsave
        self.wind = ''
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(parent = self, id = wx.ID_ANY)
        box.Add(item = label, proportion = 0, flag = wx.ALIGN_CENTRE | wx.ALL, border = 5)
        if loadsave == 'load':
            label.SetLabel(_("Load region:"))
            selection = Select(parent = self, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                               type = 'windows')
        elif loadsave == 'save':
            label.SetLabel(_("Save region:"))
            selection = Select(parent = self, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                               type = 'windows', mapsets = [grass.gisenv()['MAPSET']], fullyQualified = False)
        
        box.Add(item = selection, proportion = 0, flag = wx.ALIGN_CENTRE | wx.ALL, border = 5)
        selection.SetFocus()
        selection.Bind(wx.EVT_TEXT, self.OnRegion)
        
        sizer.Add(item = box, proportion = 0, flag = wx.GROW | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                  border = 5)
        
        line = wx.StaticLine(parent = self, id = wx.ID_ANY, size = (20, -1), style = wx.LI_HORIZONTAL)
        sizer.Add(item = line, proportion = 0,
                  flag = wx.GROW | wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT, border = 5)
        
        btnsizer = wx.StdDialogButtonSizer()
        
        btn = wx.Button(parent = self, id = wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)
        
        btn = wx.Button(parent = self, id = wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()
        
        sizer.Add(item = btnsizer, proportion = 0, flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()
        
    def OnRegion(self, event):
        self.wind = event.GetString()

    def GetName(self):
        """!Return region name"""
        return self.wind


class GroupDialog(wx.Dialog):
    """!Dialog for creating/editing groups"""
    def __init__(self, parent = None, defaultGroup = None, 
                 title = _("Create or edit imagery groups"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
                     
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title,
                            style = style, **kwargs)
                            
        self.parent = parent
        self.defaultGroup = defaultGroup
        self.currentGroup = self.defaultGroup
        self.groupChanged = False
        
        self.bodySizer = self._createDialogBody()
        
        # buttons
        btnOk = wx.Button(parent = self, id = wx.ID_OK)
        btnApply = wx.Button(parent = self, id = wx.ID_APPLY)
        btnClose = wx.Button(parent = self, id = wx.ID_CANCEL)
        
        btnOk.SetToolTipString(_("Apply changes to selected group and close dialog"))
        btnApply.SetToolTipString(_("Apply changes to selected group"))
        btnClose.SetToolTipString(_("Close dialog, changes are not applied"))

        btnOk.SetDefault()
        
        # sizers & do layout
        # btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        # btnSizer.Add(item = btnClose, proportion = 0,
        #              flag = wx.RIGHT | wx.ALIGN_RIGHT | wx.EXPAND, border = 5)
        # btnSizer.Add(item = btnApply, proportion = 0,
        #              flag = wx.LEFT, border = 5)
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnOk)
        btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnClose)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = self.bodySizer, proportion = 1,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 10)
        mainSizer.Add(item = wx.StaticLine(parent = self, id = wx.ID_ANY,
                      style = wx.LI_HORIZONTAL), proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 10) 
        
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.ALL | wx.ALIGN_RIGHT, border = 10)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        
        btnOk.Bind(wx.EVT_BUTTON, self.OnOk)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnClose.Bind(wx.EVT_BUTTON, self.OnClose)

        # set dialog min size
        self.SetMinSize(self.GetSize())
        
    def _createDialogBody(self):
        bodySizer = wx.BoxSizer(wx.VERTICAL)
    
        # group selection
        bodySizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                           label = _("Select the group you want to edit or "
                                                     "enter name of new group:")),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.TOP, border = 10)
        self.groupSelect = Select(parent = self, type = 'group',
                                  mapsets = [grass.gisenv()['MAPSET']],
                                  size = globalvar.DIALOG_GSELECT_SIZE,
                                  fullyQualified = False) # searchpath?
            
        bodySizer.Add(item = self.groupSelect, flag = wx.TOP | wx.EXPAND, border = 5)
        
        bodySizer.AddSpacer(10)
        # layers in group
        bodySizer.Add(item = wx.StaticText(parent = self, label = _("Raster maps in selected group:")),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.BOTTOM, border = 5)
        
        buttonSizer = wx.BoxSizer(wx.VERTICAL)
        listSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        self.layerBox = wx.ListBox(parent = self,  id = wx.ID_ANY, size = (-1, 150),
                                   style = wx.LB_MULTIPLE | wx.LB_NEEDED_SB)
        listSizer.Add(item = self.layerBox, proportion = 1, flag = wx.EXPAND)
        
        self.addLayer = wx.Button(self, id = wx.ID_ADD)
        self.addLayer.SetToolTipString(_("Select map layers and add them to the list."))
        buttonSizer.Add(item = self.addLayer, flag = wx.BOTTOM, border = 10)
        
        self.removeLayer = wx.Button(self, id = wx.ID_REMOVE)
        self.removeLayer.SetToolTipString(_("Remove selected layer(s) from list."))
        buttonSizer.Add(item = self.removeLayer)
        listSizer.Add(item = buttonSizer, flag = wx.LEFT, border = 5)
        
        bodySizer.Add(item = listSizer, proportion = 1, flag = wx.EXPAND)
        
        self.infoLabel = wx.StaticText(parent = self, id = wx.ID_ANY)
        bodySizer.Add(item = self.infoLabel, 
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.TOP | wx.BOTTOM, border = 5)
        
        # TODO: subgroup can have a different name than the group
        # self.subGroup must be a text field, not a checkbox
        #self.subGroup = wx.CheckBox(parent = self, id = wx.ID_ANY,
        #                            label = _("Define also sub-group (same name as group)"))
        #self.subGroup.SetValue(True) # most of imagery modules requires also subgroup
        #bodySizer.Add(item = self.subGroup, flag = wx.BOTTOM | wx.EXPAND, border = 5)

        # bindings
        self.groupSelect.GetTextCtrl().Bind(wx.EVT_TEXT, self.OnGroupSelected)
        self.addLayer.Bind(wx.EVT_BUTTON, self.OnAddLayer)
        self.removeLayer.Bind(wx.EVT_BUTTON, self.OnRemoveLayer)
        
        if self.defaultGroup:
            self.groupSelect.SetValue(self.defaultGroup)
        
        return bodySizer
        
    def OnAddLayer(self, event):
        """!Add new layer to listbox"""
        dlg = MapLayersDialogForGroups(parent = self, title = _("Add selected map layers into group"))
        
        if dlg.ShowModal() != wx.ID_OK:
            dlg.Destroy()
            return
        
        layers = dlg.GetMapLayers()
        for layer in layers:
            if layer not in self.GetLayers():
                self.layerBox.Append(layer)
                self.groupChanged = True
            
    
    def OnRemoveLayer(self, event):
        """!Remove layer from listbox"""
        while self.layerBox.GetSelections():
            sel = self.layerBox.GetSelections()[0]
            self.layerBox.Delete(sel)
            self.groupChanged = True
                
    def GetLayers(self):
        """!Get layers"""
        return self.layerBox.GetItems()
        
    def OnGroupSelected(self, event):
        """!Text changed in group selector"""
        # callAfter must be called to close popup before other actions
        wx.CallAfter(self.GroupSelected)
        
    def GroupSelected(self):
        """!Group was selected, check if changes were apllied"""
        group = self.GetSelectedGroup()
        if  self.currentGroup and self.groupChanged:
            dlg = wx.MessageDialog(self, message = _("Group <%s> was changed, "
                                                     "do you want to apply changes?") % self.currentGroup,
                                   caption = _("Unapplied changes"),
                                   style = wx.YES_NO | wx.ICON_QUESTION | wx.YES_DEFAULT)
            if dlg.ShowModal() == wx.ID_YES:
                self.ApplyChanges()
                
            dlg.Destroy()
        
        maps = list()
        groups = self.GetExistGroups()
        if group in groups:
            maps = self.GetGroupLayers(group)
        
        self.ShowGroupLayers(maps)
        self.currentGroup = group
        self.groupChanged = False
        
        self.ClearNotification()
        
    def ShowGroupLayers(self, mapList):
        """!Show map layers in currently selected group"""
        self.layerBox.Set(mapList)
                
    def EditGroup(self, group):
        """!Edit selected group"""
        layersNew = self.GetLayers()
        layersOld = self.GetGroupLayers(group)
        
        add = []
        remove = []
        for layerNew in layersNew:
            if layerNew not in layersOld:
                add.append(layerNew)
                
        for layerOld in layersOld:
            if layerOld not in layersNew:
                remove.append(layerOld)
                
        kwargs = {}
        # TODO: self.subGroup must be a text field
        #if self.subGroup.IsChecked():
        #    kwargs['subgroup'] = group
        
        ret = None
        if remove:
            ret = RunCommand('i.group',
                             parent = self,
                             group = group,
                             flags = 'r',
                             input = ','.join(remove),
                             **kwargs)
                        
        if add:
            ret = RunCommand('i.group',
                             parent = self,
                             group = group,
                             input = ','.join(add),
                             **kwargs)
                            
        return ret
        
    def CreateNewGroup(self, group):
        """!Create new group"""
        layers = self.GetLayers()
        if not layers:
            GMessage(parent = self,
                     message = _("No raster maps selected."))
            return 1
        
        kwargs = {}
        # TODO: self.subGroup must be a text field
        #if self.subGroup.IsChecked():
        #    kwargs['subgroup'] = group
        
        return RunCommand('i.group',
                          parent = self,
                          group = group,
                          input = layers,
                          **kwargs)
    
    def GetExistGroups(self):
        """!Returns existing groups in current mapset"""
        return grass.list_grouped('group')[grass.gisenv()['MAPSET']]
        
    def ShowResult(self, group, returnCode, create):
        """!Show if operation was successfull."""
        group += '@' + grass.gisenv()['MAPSET']
        if returnCode is None:
            label = _("No changes to apply in group <%s>.") % group
        elif returnCode == 0:
            if create:
                label = _("Group <%s> was successfully created.") % group
            else:
                label = _("Group <%s> was successfully changed.") % group
        else:
            if create:
                label = _("Creating of new group <%s> failed.") % group
            else:
                label = _("Changing of group <%s> failed.") % group
            
        self.infoLabel.SetLabel(label)
        wx.FutureCall(4000, self.ClearNotification)
        
    def GetSelectedGroup(self):
        """!Return currently selected group (without mapset)"""
        return self.groupSelect.GetValue().split('@')[0]
        
    def GetGroupLayers(self, group):
        """!Get layers in group"""
        kwargs = dict()
        kwargs['group'] = group
        # TODO: self.subGroup must be a text field
        #if self.subGroup.IsChecked():
        #    kwargs['subgroup'] = group
        
        res = RunCommand('i.group',
                         parent = self,
                         flags = 'g',
                         read = True, **kwargs)
        if not res:
            return []
        return res.splitlines()
    
    def ClearNotification(self):
        """!Clear notification string"""
        self.infoLabel.SetLabel("")
       
    def ApplyChanges(self):
        """!Create or edit group"""
        group = self.currentGroup
        if not group:
            GMessage(parent = self,
                     message = _("No group selected."))
            return False
        
        groups = self.GetExistGroups()
        if group in groups:
            ret = self.EditGroup(group)
            self.ShowResult(group = group, returnCode = ret, create = False)
            
        else:
            ret = self.CreateNewGroup(group)
            self.ShowResult(group = group, returnCode = ret, create = True)
            
        self.groupChanged = False
        
        return True
        
    def OnApply(self, event):
        """!Apply changes"""
        self.ApplyChanges()
        
    def OnOk(self, event):
        """!Apply changes and close dialog"""
        if self.ApplyChanges():
            self.OnClose(event)
        
    def OnClose(self, event):
        """!Close dialog"""
        if not self.IsModal():
            self.Destroy()
        event.Skip()
        
class MapLayersDialogBase(wx.Dialog):
    """!Base dialog for selecting map layers (raster, vector).

    There are 3 subclasses: MapLayersDialogForGroups, MapLayersDialogForModeler,
    MapLayersDialog. Base class contains core functionality.
    """
    def __init__(self, parent, title, 
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, title = title,
                           style = style, **kwargs)
        
        self.parent = parent # GMFrame or ?
        
        self.applyAddingMapLayers = Signal('MapLayersDialogBase.applyAddingMapLayers')
        
        self.mainSizer = wx.BoxSizer(wx.VERTICAL)
        
        # dialog body
        self.bodySizer = self._createDialogBody()
        self.mainSizer.Add(item = self.bodySizer, proportion = 1,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        
        # update list of layer to be loaded
        self.map_layers = [] # list of map layers (full list type/mapset)
        self.LoadMapLayers(self.GetLayerType(cmd = True),
                           self.mapset.GetStringSelection())

        self._fullyQualifiedNames()
        self._modelerDSeries()

        # buttons
        btnCancel = wx.Button(parent = self, id = wx.ID_CANCEL)
        btnOk = wx.Button(parent = self, id = wx.ID_OK)
        btnOk.SetDefault()
        
        # sizers & do layout
        self.btnSizer = wx.StdDialogButtonSizer()
        self.btnSizer.AddButton(btnCancel)
        self.btnSizer.AddButton(btnOk)
        self._addApplyButton()
        self.btnSizer.Realize()
        
        self.mainSizer.Add(item = self.btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)

        self.SetSizer(self.mainSizer)
        self.mainSizer.Fit(self)

        # set dialog min size
        self.SetMinSize(self.GetSize())

    def _modelerDSeries(self):
        """!Method used only by MapLayersDialogForModeler,
        for other subclasses does nothing.
        """
        pass

    def _addApplyButton(self):
        """!Method used only by MapLayersDialog,
        for other subclasses does nothing.
        """
        pass

    def _fullyQualifiedNames(self):
        """!Adds CheckBox which determines is fully qualified names are retuned.
        """
        self.fullyQualified = wx.CheckBox(parent = self, id = wx.ID_ANY,
                                           label = _("Use fully-qualified map names"))
        self.fullyQualified.SetValue(True)
        self.mainSizer.Add(item = self.fullyQualified, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 5)

    def _useFullyQualifiedNames(self):
        return self.fullyQualified.IsChecked()

    def _layerTypes(self):
        """!Determines which layer types can be chosen.

         Valid values:
         - raster
         - raster3d
         - vector
         """
        return [_('raster'), _('3D raster'), _('vector')]

    def _selectAll(self):
        """!Check all layers by default"""
        return True

    def _createDialogBody(self):
        bodySizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        
        # layer type
        bodySizer.Add(item = wx.StaticText(parent = self, label = _("Map type:")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0,0))
        
        self.layerType = wx.Choice(parent = self, id = wx.ID_ANY,
                                   choices = self._layerTypes(), size = (100,-1))

        self.layerType.SetSelection(0)
            
        bodySizer.Add(item = self.layerType,
                           pos = (0,1))
        self.layerType.Bind(wx.EVT_CHOICE, self.OnChangeParams)

        # select toggle
        self.toggle = wx.CheckBox(parent = self, id = wx.ID_ANY,
                                  label = _("Select toggle"))
        self.toggle.SetValue(self._selectAll())
        bodySizer.Add(item = self.toggle,
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0,2))
        
        # mapset filter
        bodySizer.Add(item = wx.StaticText(parent = self, label = _("Mapset:")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (1,0))
        
        self.mapset = MapsetSelect(parent = self, searchPath = True)
        self.mapset.SetStringSelection(grass.gisenv()['MAPSET'])
        bodySizer.Add(item = self.mapset,
                      pos = (1,1), span = (1, 2))
        
        # map name filter
        bodySizer.Add(item = wx.StaticText(parent = self, label = _("Pattern:")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (2,0))
        
        self.filter = wx.TextCtrl(parent = self, id = wx.ID_ANY,
                                  value = "",
                                  size = (250,-1))
        bodySizer.Add(item = self.filter,
                      flag = wx.EXPAND,
                      pos = (2,1), span = (1, 2))

        self.filter.SetFocus()
        self.filter.SetToolTipString(_("Put here a regular expression."
                                       " Characters '.*' stand for anything,"
                                       " character '^' stands for the beginning"
                                       " and '$' for the end."))

        # layer list 
        bodySizer.Add(item = wx.StaticText(parent = self, label = _("List of maps:")),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_TOP,
                      pos = (3,0))
        self.layers = wx.CheckListBox(parent = self, id = wx.ID_ANY,
                                      size = (250, 100),
                                      choices = [])
        bodySizer.Add(item = self.layers,
                      flag = wx.EXPAND,
                      pos = (3,1), span = (1, 2))
        
        bodySizer.AddGrowableCol(1)
        bodySizer.AddGrowableRow(3)
        # bindings
        self.mapset.Bind(wx.EVT_COMBOBOX, self.OnChangeParams)
        self.layers.Bind(wx.EVT_RIGHT_DOWN, self.OnMenu)
        self.filter.Bind(wx.EVT_TEXT, self.OnFilter)
        self.toggle.Bind(wx.EVT_CHECKBOX, self.OnToggle)
        
        return bodySizer

    def LoadMapLayers(self, type, mapset):
        """!Load list of map layers

        @param type layer type ('raster' or 'vector')
        @param mapset mapset name
        """
        self.map_layers = grass.mlist_grouped(type = type)[mapset]
        self.layers.Set(self.map_layers)
        
        # check all items by default
        for item in range(self.layers.GetCount()):
            
            self.layers.Check(item, check = self._selectAll())
        
    def OnChangeParams(self, event):
        """!Filter parameters changed by user"""
        # update list of layer to be loaded
        self.LoadMapLayers(self.GetLayerType(cmd = True),
                           self.mapset.GetStringSelection())
        
        event.Skip()
        
    def OnMenu(self, event):
        """!Table description area, context menu"""
        if not hasattr(self, "popupID1"):
            self.popupDataID1 = wx.NewId()
            self.popupDataID2 = wx.NewId()
            self.popupDataID3 = wx.NewId()

            self.Bind(wx.EVT_MENU, self.OnSelectAll,    id = self.popupDataID1)
            self.Bind(wx.EVT_MENU, self.OnSelectInvert, id = self.popupDataID2)
            self.Bind(wx.EVT_MENU, self.OnDeselectAll,  id = self.popupDataID3)
        
        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupDataID1, _("Select all"))
        menu.Append(self.popupDataID2, _("Invert selection"))
        menu.Append(self.popupDataID3, _("Deselect all"))
        
        self.PopupMenu(menu)
        menu.Destroy()

    def OnSelectAll(self, event):
        """!Select all map layer from list"""
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, True)
        
    def OnSelectInvert(self, event):
        """!Invert current selection"""
        for item in range(self.layers.GetCount()):
            if self.layers.IsChecked(item):
                self.layers.Check(item, False)
            else:
                self.layers.Check(item, True)
        
    def OnDeselectAll(self, event):
        """!Select all map layer from list"""
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, False)
        
    def OnFilter(self, event):
        """!Apply filter for map names"""
        if len(event.GetString()) == 0:
            self.layers.Set(self.map_layers) 
            return 
        
        list = []
        for layer in self.map_layers:
            try:
                if re.compile(event.GetString()).search(layer):
                    list.append(layer)
            except:
                pass
        
        self.layers.Set(list)
        self.OnSelectAll(None)
        
        event.Skip()
        
    def OnToggle(self, event):
        """!Select toggle (check or uncheck all layers)"""
        check = event.Checked()
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, check)
        
        event.Skip()
        
    def GetMapLayers(self):
        """!Return list of checked map layers"""
        layerNames = []
        for indx in self.layers.GetSelections():
            # layers.append(self.layers.GetStringSelec(indx))
            pass

        mapset = self.mapset.GetStringSelection()
        for item in range(self.layers.GetCount()):
            if not self.layers.IsChecked(item):
                continue
            if self._useFullyQualifiedNames():
                layerNames.append(self.layers.GetString(item) + '@' + mapset)
            else:
                layerNames.append(self.layers.GetString(item))
        
        return layerNames
    
    def GetLayerType(self, cmd = False):
        """!Get selected layer type

        @param cmd True for g.mlist
        """
        if not cmd:
            return self.layerType.GetStringSelection()
        
        sel = self.layerType.GetSelection()
        if sel == 0:
            ltype = 'rast'
        elif sel == 1:
            ltype = 'rast3d'
        else:
            ltype = 'vect'
        
        return ltype

class MapLayersDialog(MapLayersDialogBase):
    """!Subclass of MapLayersDialogBase used in Layer Manager. 

    Contains apply button, which sends wxApplyMapLayers event.
    """
    def __init__(self, parent, title, **kwargs):
        MapLayersDialogBase.__init__(self, parent = parent, title = title, **kwargs)

    def _addApplyButton(self):
        btnApply = wx.Button(parent = self, id = wx.ID_APPLY)
        self.btnSizer.AddButton(btnApply)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)

    def OnApply(self, event):
        self.applyAddingMapLayers.emit(mapLayers = self.GetMapLayers(),
                                       ltype = self.GetLayerType(cmd = True))

class MapLayersDialogForGroups(MapLayersDialogBase):
    """!Subclass of MapLayersDialogBase used for specyfying maps in an imagery group. 

    Shows only raster maps.
    """
    def __init__(self, parent, title, **kwargs):
        MapLayersDialogBase.__init__(self, parent = parent, title = title, **kwargs)

    def _layerTypes(self):
        return [_('raster'),]

    def _selectAll(self):
        """!Could be overriden"""
        return False

    def _fullyQualifiedNames(self):
        pass

    def _useFullyQualifiedNames(self):
        return True


class MapLayersDialogForModeler(MapLayersDialogBase):
    """!Subclass of MapLayersDialogBase used in Modeler. 
    """
    def __init__(self, parent, title, **kwargs):
        MapLayersDialogBase.__init__(self, parent = parent, title = title, **kwargs)

    def _modelerDSeries(self):
        self.dseries = wx.CheckBox(parent = self, id = wx.ID_ANY,
                                   label = _("Dynamic series (%s)") % 'g.mlist')
        self.dseries.SetValue(False)
        self.mainSizer.Add(item = self.dseries, proportion = 0,
                           flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 5)

    def GetDSeries(self):
        """!Used by modeler only

        @return g.mlist command
        """
        if not self.dseries or not self.dseries.IsChecked():
            return ''
        
        cond = 'map in `g.mlist type=%s ' % self.GetLayerType(cmd = True)
        patt = self.filter.GetValue()
        if patt:
            cond += 'pattern=%s ' % patt
        cond += 'mapset=%s`' % self.mapset.GetStringSelection()
        
        return cond

    
class ImportDialog(wx.Dialog):
    """!Dialog for bulk import of various data (base class)"""
    def __init__(self, parent, itype,
                 id = wx.ID_ANY, title = _("Multiple import"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent    # GMFrame 
        self.importType = itype
        self.options = dict()   # list of options
        
        self.commandId = -1  # id of running command
        
        wx.Dialog.__init__(self, parent, id, title, style = style,
                           name = "MultiImportDialog")
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.layerBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY)
        if self.importType == 'gdal':
            label = _("List of raster layers")
        elif self.importType == 'ogr':
            label = _("List of vector layers")
        else:
            label = _("List of %s layers") % self.importType.upper()
        self.layerBox.SetLabel(" %s - %s " % (label, _("right click to (un)select all")))
        
        # list of layers
        columns = [_('Layer id'),
                   _('Layer name'),
                   _('Name for output GRASS map (editable)')]
        if itype == 'ogr':
            columns.insert(2, _('Feature type'))
            columns.insert(3, _('Projection match'))

        self.list = LayersList(parent = self.panel, columns = columns)
        self.list.LoadData()

        self.optionBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                      label = "%s" % _("Options"))
        
        cmd = self._getCommand()
        task = gtask.parse_interface(cmd)
        for f in task.get_options()['flags']:
            name = f.get('name', '')
            desc = f.get('label', '')
            if not desc:
                desc = f.get('description', '')
            if not name and not desc:
                continue
            if cmd == 'r.in.gdal' and name not in ('o', 'e', 'l', 'k'):
                continue
            elif cmd == 'r.external' and name not in ('o', 'e', 'r', 'h', 'v'):
                continue
            elif cmd == 'v.in.ogr' and name not in ('c', 'z', 't', 'o', 'r', 'e', 'w'):
                continue
            elif cmd == 'v.external' and name not in ('b'):
                continue
            elif cmd == 'v.in.dxf' and name not in ('e', 't', 'b', 'f', 'i'):
                continue
            self.options[name] = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                             label = desc)
        
        
        self.overwrite = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                     label = _("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'))
        
        self.add = wx.CheckBox(parent = self.panel, id = wx.ID_ANY)
        self.closeOnFinish = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                     label = _("Close dialog on finish"))
        self.closeOnFinish.SetValue(UserSettings.Get(group = 'cmd', key = 'closeDlg', subkey = 'enabled'))
        
        #
        # buttons
        #
        # cancel
        self.btn_close = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btn_close.SetToolTipString(_("Close dialog"))
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        # run
        self.btn_run = wx.Button(parent = self.panel, id = wx.ID_OK, label = _("&Import"))
        self.btn_run.SetToolTipString(_("Import selected layers"))
        self.btn_run.SetDefault()
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnRun)
        
    def doLayout(self):
        """!Do layout"""
        dialogSizer = wx.BoxSizer(wx.VERTICAL)
        
        # dsn input
        dialogSizer.Add(item = self.dsnInput, proportion = 0,
                        flag = wx.EXPAND)
        
        #
        # list of DXF layers
        #
        layerSizer = wx.StaticBoxSizer(self.layerBox, wx.HORIZONTAL)

        layerSizer.Add(item = self.list, proportion = 1,
                      flag = wx.ALL | wx.EXPAND, border = 5)
        
        dialogSizer.Add(item = layerSizer, proportion = 1,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)

        # options
        optionSizer = wx.StaticBoxSizer(self.optionBox, wx.VERTICAL)
        for key in self.options.keys():
            optionSizer.Add(item = self.options[key], proportion = 0)
            
        dialogSizer.Add(item = optionSizer, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        
        dialogSizer.Add(item = self.overwrite, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        dialogSizer.Add(item = self.add, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        dialogSizer.Add(item = self.closeOnFinish, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        #
        # buttons
        #
        btnsizer = wx.BoxSizer(orient = wx.HORIZONTAL)
        
        btnsizer.Add(item = self.btn_close, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT | wx.ALIGN_CENTER,
                     border = 10)
        
        btnsizer.Add(item = self.btn_run, proportion = 0,
                     flag = wx.RIGHT | wx.ALIGN_CENTER,
                     border = 10)
        
        dialogSizer.Add(item = btnsizer, proportion = 0,
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.BOTTOM | wx.ALIGN_RIGHT,
                        border = 10)
        
        # dialogSizer.SetSizeHints(self.panel)
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)
        
        # auto-layout seems not work here - FIXME
        size = wx.Size(globalvar.DIALOG_GSELECT_SIZE[0] + 225, 550)
        self.SetMinSize(size)
        self.SetSize((size.width, size.height + 100))
        # width = self.GetSize()[0]
        # self.list.SetColumnWidth(col = 1, width = width / 2 - 50)
        self.Layout()

    def _getCommand(self):
        """!Get command"""
        return ''
    
    def OnClose(self, event = None):
        """!Close dialog"""
        self.Close()

    def OnRun(self, event):
        """!Import/Link data (each layes as separate vector map)"""
        pass

    def AddLayers(self, returncode, cmd = None):
        """!Add imported/linked layers into layer tree"""
        if not self.add.IsChecked() or returncode != 0:
            return
        
        self.commandId += 1
        maptree = self.parent.GetLayerTree()
        
        layer, output = self.list.GetLayers()[self.commandId]
        
        if '@' not in output:
            name = output + '@' + grass.gisenv()['MAPSET']
        else:
            name = output
        
        # add imported layers into layer tree
        if self.importType == 'gdal':
            cmd = ['d.rast',
                   'map=%s' % name]
            if UserSettings.Get(group = 'rasterLayer', key = 'opaque', subkey = 'enabled'):
                cmd.append('-n')
            
            item = maptree.AddLayer(ltype = 'raster',
                                    lname = name, lchecked = False,
                                    lcmd = cmd, multiple = False)
        else:
            item = maptree.AddLayer(ltype = 'vector',
                                    lname = name, lchecked = False,
                                    lcmd = ['d.vect',
                                            'map=%s' % name] + GetDisplayVectSettings(),
                                    multiple = False)
        
        maptree.mapdisplay.MapWindow.ZoomToMap()
        
    def OnAbort(self, event):
        """!Abort running import

        @todo not yet implemented
        """
        pass

class GdalImportDialog(ImportDialog):
    def __init__(self, parent, giface, ogr = False, link = False):
        """!Dialog for bulk import of various raster/vector data

        @todo Split into GdalImportDialog and OgrImportDialog

        @param parent parent window
        @param ogr True for OGR (vector) otherwise GDAL (raster)
        @param link True for linking data otherwise importing data
        """
        self._giface = giface
        self.link = link
        self.ogr  = ogr
        
        if ogr:
            ImportDialog.__init__(self, parent, itype = 'ogr')
            if link:
                self.SetTitle(_("Link external vector data"))
            else:
                self.SetTitle(_("Import vector data"))
        else:
            ImportDialog.__init__(self, parent, itype = 'gdal') 
            if link:
                self.SetTitle(_("Link external raster data"))
            else:
                self.SetTitle(_("Import raster data"))
        
        self.dsnInput = GdalSelect(parent = self, panel = self.panel,
                                   ogr = ogr, link = link)
        self.dsnInput.reloadDataRequired.connect(lambda data: self.list.LoadData(data))
        
        if link:
            self.add.SetLabel(_("Add linked layers into layer tree"))
        else:
            self.add.SetLabel(_("Add imported layers into layer tree"))
        
        self.add.SetValue(UserSettings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))

        if link:
            self.btn_run.SetLabel(_("&Link"))
            self.btn_run.SetToolTipString(_("Link selected layers"))
        else:
            self.btn_run.SetLabel(_("&Import"))
            self.btn_run.SetToolTipString(_("Import selected layers"))
        
        self.doLayout()

    def OnRun(self, event):
        """!Import/Link data (each layes as separate vector map)"""
        self.commandId = -1
        data = self.list.GetLayers()
        if not data:
            GMessage(_("No layers selected. Operation canceled."),
                     parent = self)
            return
        
        dsn  = self.dsnInput.GetDsn()
        ext  = self.dsnInput.GetFormatExt()
        
        # determine data driver for PostGIS links
        popOGR = False
        if self.importType == 'ogr' and \
                self.dsnInput.GetType() == 'db' and \
                self.dsnInput.GetFormat() == 'PostgreSQL' and \
                'GRASS_VECTOR_OGR' not in os.environ:
            popOGR = True
            os.environ['GRASS_VECTOR_OGR'] = '1'
        
        for layer, output in data:
            if self.importType == 'ogr':
                if ext and layer.rfind(ext) > -1:
                    layer = layer.replace('.' + ext, '')
                if self.link:
                    cmd = ['v.external',
                           'dsn=%s' % dsn,
                           'output=%s' % output,
                           'layer=%s' % layer]
                else:
                    cmd = ['v.in.ogr',
                           'dsn=%s' % dsn,
                           'layer=%s' % layer,
                           'output=%s' % output]
            else: # gdal
                if self.dsnInput.GetType() == 'dir':
                    idsn = os.path.join(dsn, layer)
                else:
                    idsn = dsn
                
                if self.link:
                    cmd = ['r.external',
                           'input=%s' % idsn,
                           'output=%s' % output]
                else:
                    cmd = ['r.in.gdal',
                           'input=%s' % idsn,
                           'output=%s' % output]
            
            if self.overwrite.IsChecked():
                cmd.append('--overwrite')
            
            for key in self.options.keys():
                if self.options[key].IsChecked():
                    cmd.append('-%s' % key)
            
            if UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled') and \
                    '--overwrite' not in cmd:
                cmd.append('--overwrite')
            
            # run in Layer Manager
            self._giface.RunCmd(cmd, switchPage = True, onDone = self.AddLayers)
        
        if popOGR:
            os.environ.pop('GRASS_VECTOR_OGR')

        if self.closeOnFinish.IsChecked():
            self.Close()
        
    def _getCommand(self):
        """!Get command"""
        if self.link:
            if self.ogr:
                return 'v.external'
            else:
                return 'r.external'
        else:
            if self.ogr:
                return 'v.in.ogr'
            else:
                return 'r.in.gdal'
        
        return ''

class GdalOutputDialog(wx.Dialog):
    def __init__(self, parent, id = wx.ID_ANY, ogr = False,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, *kwargs):
        """!Dialog for setting output format for rasters/vectors

        @todo Split into GdalOutputDialog and OgrOutputDialog

        @param parent parent window
        @param id window id
        @param ogr True for OGR (vector) otherwise GDAL (raster)
        @param style window style
        @param *kwargs other wx.Dialog's arguments
        """
        self.parent = parent # GMFrame 
        self.ogr = ogr
        wx.Dialog.__init__(self, parent, id = id, style = style, *kwargs)
        if self.ogr:
            self.SetTitle(_("Define output format for vector data"))
        else:
            self.SetTitle(_("Define output format for raster data"))
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)

        # buttons
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btnCancel.SetToolTipString(_("Close dialog"))
        self.btnOk = wx.Button(parent = self.panel, id = wx.ID_OK)
        self.btnOk.SetToolTipString(_("Set external format and close dialog"))
        self.btnOk.SetDefault()
        
        self.dsnInput = GdalSelect(parent = self, panel = self.panel,
                                   ogr = ogr,
                                   exclude = ['file', 'protocol'], dest = True)
        
        self.Bind(wx.EVT_BUTTON, self.OnCancel, self.btnCancel)
        self.Bind(wx.EVT_BUTTON, self.OnOK, self.btnOk)
        
        self._layout()

    def _layout(self):
        dialogSizer = wx.BoxSizer(wx.VERTICAL)
        
        dialogSizer.Add(item = self.dsnInput, proportion = 0,
                        flag = wx.EXPAND)

        btnSizer = wx.BoxSizer(orient = wx.HORIZONTAL)
        btnSizer.Add(item = self.btnCancel, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT | wx.ALIGN_CENTER,
                     border = 10)
        btnSizer.Add(item = self.btnOk, proportion = 0,
                     flag = wx.RIGHT | wx.ALIGN_CENTER,
                     border = 10)
        
        dialogSizer.Add(item = btnSizer, proportion = 0,
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.BOTTOM | wx.TOP | wx.ALIGN_RIGHT,
                        border = 10)
        
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)

        size = wx.Size(globalvar.DIALOG_GSELECT_SIZE[0] + 225, self.GetBestSize()[1])
        self.SetMinSize(size)
        self.SetSize((size.width, size.height))
        self.Layout()
        
    def OnCancel(self, event):
        self.Destroy()
        
    def OnOK(self, event):
        if self.dsnInput.GetType() == 'native':
            RunCommand('v.external.out',
                       parent = self,
                       flags = 'r')
        else:
            dsn = self.dsnInput.GetDsn()
            frmt = self.dsnInput.GetFormat()
            options = self.dsnInput.GetOptions()
            if not dsn:
                GMessage(_("No data source selected."), parent=self)
                return
            
            RunCommand('v.external.out',
                       parent = self,
                       dsn = dsn, format = frmt,
                       options = options)
        self.Close()
        
class DxfImportDialog(ImportDialog):
    """!Dialog for bulk import of DXF layers""" 
    def __init__(self, parent, giface):
        ImportDialog.__init__(self, parent, itype = 'dxf',
                              title = _("Import DXF layers"))
        self._giface = giface
        self.dsnInput = filebrowse.FileBrowseButton(parent = self.panel, id = wx.ID_ANY, 
                                                    size = globalvar.DIALOG_GSELECT_SIZE, labelText = '',
                                                    dialogTitle = _('Choose DXF file to import'),
                                                    buttonText = _('Browse'),
                                                    startDirectory = os.getcwd(), fileMode = 0,
                                                    changeCallback = self.OnSetDsn,
                                                    fileMask = "DXF File (*.dxf)|*.dxf")
        
        self.add.SetLabel(_("Add imported layers into layer tree"))
        
        self.add.SetValue(UserSettings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))
        
        self.doLayout()

    def _getCommand(self):
        """!Get command"""
        return 'v.in.dxf'
    
    def OnRun(self, event):
        """!Import/Link data (each layes as separate vector map)"""
        data = self.list.GetLayers()
        if not data:
            GMessage(_("No layers selected."), parent=self)
            return
        
        # hide dialog
        self.Hide()
        
        inputDxf = self.dsnInput.GetValue()
        
        for layer, output in data:
            cmd = ['v.in.dxf',
                   'input=%s' % inputDxf,
                   'layers=%s' % layer,
                   'output=%s' % output]

            for key in self.options.keys():
                if self.options[key].IsChecked():
                    cmd.append('-%s' % key)
            
            if self.overwrite.IsChecked() or \
                    UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'):
                cmd.append('--overwrite')
            
            # run in Layer Manager
            self._giface.RunCmd(cmd, switchPage = True, onDone = self.AddLayers)
        
        self.Close()

    def OnSetDsn(self, event):
        """!Input DXF file defined, update list of layer widget"""
        path = event.GetString()
        if not path:
            return 
        
        data = list()        
        ret = RunCommand('v.in.dxf',
                         quiet = True,
                         parent = self,
                         read = True,
                         flags = 'l',
                         input = path)
        if not ret:
            self.list.LoadData()
            return
            
        for line in ret.splitlines():
            layerId = line.split(':')[0].split(' ')[1]
            layerName = line.split(':')[1].strip()
            grassName = GetValidLayerName(layerName)
            data.append((layerId, layerName.strip(), grassName.strip()))
        
        self.list.LoadData(data)


class LayersList(GListCtrl, listmix.TextEditMixin):
    """!List of layers to be imported (dxf, shp...)"""
    def __init__(self, parent, columns, log = None):
        GListCtrl.__init__(self, parent)
        
        self.log = log
        
        # setup mixins
        listmix.TextEditMixin.__init__(self)
        
        for i in range(len(columns)):
            self.InsertColumn(i, columns[i])
        
        if len(columns) == 3:
            width = (65, 200)
        else:
            width = (65, 180, 90, 70)
        
        for i in range(len(width)):
            self.SetColumnWidth(col = i, width = width[i])
        
    def LoadData(self, data = None):
        """!Load data into list"""
        self.DeleteAllItems()
        if data is None:
            return
        
        for item in data:
            index = self.InsertStringItem(sys.maxint, str(item[0]))
            for i in range(1, len(item)):
                self.SetStringItem(index, i, "%s" % str(item[i]))
        
        # check by default only on one item
        if len(data) == 1:
            self.CheckItem(index, True)
        
    def OnLeftDown(self, event):
        """!Allow editing only output name
        
        Code taken from TextEditMixin class.
        """
        x, y = event.GetPosition()
        
        colLocs = [0]
        loc = 0
        for n in range(self.GetColumnCount()):
            loc = loc + self.GetColumnWidth(n)
            colLocs.append(loc)
        
        col = bisect(colLocs, x + self.GetScrollPos(wx.HORIZONTAL)) - 1
        
        if col == self.GetColumnCount() - 1:
            listmix.TextEditMixin.OnLeftDown(self, event)
        else:
            event.Skip()
        
    def GetLayers(self):
        """!Get list of layers (layer name, output name)"""
        data = []
        item = -1
        while True:
            item = self.GetNextItem(item)
            if item == -1:
                break
            if not self.IsChecked(item):
                continue
            # layer / output name
            data.append((self.GetItem(item, 1).GetText(),
                         self.GetItem(item, self.GetColumnCount() - 1).GetText()))
        
        return data

class SetOpacityDialog(wx.Dialog):
    """!Set opacity of map layers"""
    def __init__(self, parent, id = wx.ID_ANY, title = _("Set Map Layer Opacity"),
                 size = wx.DefaultSize, pos = wx.DefaultPosition,
                 style = wx.DEFAULT_DIALOG_STYLE, opacity = 100):

        self.parent = parent    # GMFrame
        self.opacity = opacity  # current opacity

        super(SetOpacityDialog, self).__init__(parent, id = id, pos = pos,
                                               size = size, style = style, title = title)

        self.applyOpacity = Signal('SetOpacityDialog.applyOpacity')
        panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.GridBagSizer(vgap = 5, hgap = 5)
        self.value = wx.Slider(panel, id = wx.ID_ANY, value = self.opacity,
                               style = wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | \
                                   wx.SL_TOP | wx.SL_LABELS,
                               minValue = 0, maxValue = 100,
                               size = (350, -1))

        box.Add(item = self.value,
                flag = wx.ALIGN_CENTRE, pos = (0, 0), span = (1, 2))
        box.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                   label = _("transparent")),
                pos = (1, 0))
        box.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                   label = _("opaque")),
                flag = wx.ALIGN_RIGHT,
                pos = (1, 1))

        sizer.Add(item = box, proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border = 5)

        line = wx.StaticLine(parent = panel, id = wx.ID_ANY,
                             style = wx.LI_HORIZONTAL)
        sizer.Add(item = line, proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border = 5)

        # buttons
        btnsizer = wx.StdDialogButtonSizer()

        btnOK = wx.Button(parent = panel, id = wx.ID_OK)
        btnOK.SetDefault()
        btnsizer.AddButton(btnOK)

        btnCancel = wx.Button(parent = panel, id = wx.ID_CANCEL)
        btnsizer.AddButton(btnCancel)

        btnApply = wx.Button(parent = panel, id = wx.ID_APPLY)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnsizer.AddButton(btnApply)
        btnsizer.Realize()

        sizer.Add(item = btnsizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border = 5)

        panel.SetSizer(sizer)
        sizer.Fit(panel)

        self.SetSize(self.GetBestSize())

        self.Layout()

    def GetOpacity(self):
        """!Button 'OK' pressed"""
        # return opacity value
        opacity = float(self.value.GetValue()) / 100
        return opacity

    def OnApply(self, event):
        self.applyOpacity.emit(value = self.GetOpacity())


def GetImageHandlers(image):
    """!Get list of supported image handlers"""
    lext = list()
    ltype = list()
    for h in image.GetHandlers():
        lext.append(h.GetExtension())
        
    filetype = ''
    if 'png' in lext:
        filetype += "PNG file (*.png)|*.png|"
        ltype.append({ 'type' : wx.BITMAP_TYPE_PNG,
                       'ext'  : 'png' })
    filetype +=  "BMP file (*.bmp)|*.bmp|"
    ltype.append({ 'type' : wx.BITMAP_TYPE_BMP,
                   'ext'  : 'bmp' })
    if 'gif' in lext:
        filetype += "GIF file (*.gif)|*.gif|"
        ltype.append({ 'type' : wx.BITMAP_TYPE_GIF,
                       'ext'  : 'gif' })
        
    if 'jpg' in lext:
        filetype += "JPG file (*.jpg)|*.jpg|"
        ltype.append({ 'type' : wx.BITMAP_TYPE_JPEG,
                       'ext'  : 'jpg' })

    if 'pcx' in lext:
        filetype += "PCX file (*.pcx)|*.pcx|"
        ltype.append({ 'type' : wx.BITMAP_TYPE_PCX,
                       'ext'  : 'pcx' })
        
    if 'pnm' in lext:
        filetype += "PNM file (*.pnm)|*.pnm|"
        ltype.append({ 'type' : wx.BITMAP_TYPE_PNM,
                       'ext'  : 'pnm' })

    if 'tif' in lext:
        filetype += "TIF file (*.tif)|*.tif|"
        ltype.append({ 'type' : wx.BITMAP_TYPE_TIF,
                       'ext'  : 'tif' })

    if 'xpm' in lext:
        filetype += "XPM file (*.xpm)|*.xpm"
        ltype.append({ 'type' : wx.BITMAP_TYPE_XPM,
                       'ext'  : 'xpm' })
    
    return filetype, ltype

class ImageSizeDialog(wx.Dialog):
    """!Set size for saved graphic file"""
    def __init__(self, parent, id = wx.ID_ANY, title = _("Set image size"),
                 style = wx.DEFAULT_DIALOG_STYLE, **kwargs):
        self.parent = parent
        
        wx.Dialog.__init__(self, parent, id = id, style = style, title = title, **kwargs)
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.box = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                label = ' % s' % _("Image size"))
        
        size = self.parent.GetWindow().GetClientSize()
        self.width = wx.SpinCtrl(parent = self.panel, id = wx.ID_ANY,
                                 style = wx.SP_ARROW_KEYS)
        self.width.SetRange(20, 1e6)
        self.width.SetValue(size.width)
        wx.CallAfter(self.width.SetFocus)
        self.height = wx.SpinCtrl(parent = self.panel, id = wx.ID_ANY,
                                  style = wx.SP_ARROW_KEYS)
        self.height.SetRange(20, 1e6)
        self.height.SetValue(size.height)
        self.template = wx.Choice(parent = self.panel, id = wx.ID_ANY,
                                  size = (125, -1),
                                  choices = [ "",
                                              "640x480",
                                              "800x600",
                                              "1024x768",
                                              "1280x960",
                                              "1600x1200",
                                              "1920x1440" ])
        
        self.btnOK = wx.Button(parent = self.panel, id = wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        
        self.template.Bind(wx.EVT_CHOICE, self.OnTemplate)
        
        self._layout()
        self.SetSize(self.GetBestSize())
        
    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        # body
        box = wx.StaticBoxSizer(self.box, wx.HORIZONTAL)
        fbox = wx.FlexGridSizer(cols = 2, vgap = 5, hgap = 5)
        fbox.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                      label = _("Width:")),
                 flag = wx.ALIGN_CENTER_VERTICAL)
        fbox.Add(item = self.width)
        fbox.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                      label = _("Height:")),
                 flag = wx.ALIGN_CENTER_VERTICAL)
        fbox.Add(item = self.height)
        fbox.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                      label = _("Template:")),
                 flag = wx.ALIGN_CENTER_VERTICAL)
        fbox.Add(item = self.template)
        
        box.Add(item = fbox, proportion = 1,
                flag = wx.EXPAND | wx.ALL, border = 5)
        sizer.Add(item = box, proportion = 1,
                  flag=wx.EXPAND | wx.ALL, border = 3)
        
        # buttons
        btnsizer = wx.StdDialogButtonSizer()
        btnsizer.AddButton(self.btnOK)
        btnsizer.AddButton(self.btnCancel)
        btnsizer.Realize()

        sizer.Add(item = btnsizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_RIGHT | wx.ALL, border=5)
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        self.Layout()
    
    def GetValues(self):
        """!Get width/height values"""
        return self.width.GetValue(), self.height.GetValue()
    
    def OnTemplate(self, event):
        """!Template selected"""
        sel = event.GetString()
        if not sel:
            width, height = self.parent.GetWindow().GetClientSize()
        else:
            width, height = map(int, sel.split('x'))
        self.width.SetValue(width)
        self.height.SetValue(height)
        
class SqlQueryFrame(wx.Frame):
    def __init__(self, parent, id = wx.ID_ANY,
                 title = _("GRASS GIS SQL Query Utility"),
                 *kwargs):
        """!SQL Query Utility window
        """
        self.parent = parent

        wx.Frame.__init__(self, parent = parent, id = id, title = title, *kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_sql.ico'), wx.BITMAP_TYPE_ICO))
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.sqlBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                   label = _(" SQL statement "))
        self.sql = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY,
                               style = wx.TE_MULTILINE)
        
        self.btnApply = wx.Button(parent = self.panel, id = wx.ID_APPLY)
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.Bind(wx.EVT_BUTTON, self.OnCloseWindow, self.btnCancel)
        
        self._layout()

        self.SetMinSize(wx.Size(300, 150))
        self.SetSize(wx.Size(500, 200))
        
    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        sqlSizer = wx.StaticBoxSizer(self.sqlBox, wx.HORIZONTAL)
        sqlSizer.Add(item = self.sql, proportion = 1,
                     flag = wx.EXPAND)

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnApply)
        btnSizer.AddButton(self.btnCancel)
        btnSizer.Realize()
        
        sizer.Add(item = sqlSizer, proportion = 1,
                  flag = wx.EXPAND | wx.ALL, border = 5) 
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
       
        self.panel.SetSizer(sizer)
        
        self.Layout()

    def OnCloseWindow(self, event):
        """!Close window
        """
        self.Close()

class SymbolDialog(wx.Dialog):
    """!Dialog for GRASS symbols selection.
    
    Dialog is called in gui_core::forms module.
    """
    def __init__(self, parent, symbolPath, currentSymbol = None, title = _("Symbols")):
        """!Dialog constructor.
        
        It is assumed that symbolPath contains folders with symbols.
        
        @param parent dialog parent
        @param symbolPath absolute path to symbols
        @param currentSymbol currently selected symbol (e.g. 'basic/x')
        @param title dialog title
        """
        wx.Dialog.__init__(self, parent = parent, title = title, id = wx.ID_ANY)
        
        self.symbolPath = symbolPath
        self.currentSymbol = currentSymbol # default basic/x
        self.selected = None
        self.selectedDir = None
        
        self._layout()
        
    def _layout(self):
        mainPanel = wx.Panel(self, id = wx.ID_ANY)
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        vSizer = wx.BoxSizer( wx.VERTICAL)
        fgSizer = wx.FlexGridSizer(rows = 2, vgap = 5, hgap = 5)
        self.folderChoice = wx.Choice(mainPanel, id = wx.ID_ANY, choices = os.listdir(self.symbolPath))
        self.folderChoice.Bind(wx.EVT_CHOICE, self.OnFolderSelect)
        
        fgSizer.Add(item = wx.StaticText(mainPanel, id = wx.ID_ANY, label = _("Symbol directory:")),
                   proportion = 0,
                   flag = wx.ALIGN_CENTER_VERTICAL)
                   
        fgSizer.Add(item = self.folderChoice, proportion = 0,
                   flag = wx.ALIGN_CENTER, border = 0)
                   
        self.infoLabel = wx.StaticText(mainPanel, id = wx.ID_ANY)
        fgSizer.Add(wx.StaticText(mainPanel, id = wx.ID_ANY, label = _("Symbol name:")), 
                    flag = wx.ALIGN_CENTRE_VERTICAL)
        fgSizer.Add(self.infoLabel, proportion = 0, 
                    flag = wx.ALIGN_CENTRE_VERTICAL)
        vSizer.Add(fgSizer, proportion = 0, flag = wx.ALL, border = 5)
        
        self.panels = self._createSymbolPanels(mainPanel)
        for panel in self.panels:
            vSizer.Add(panel, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        mainSizer.Add(vSizer, proportion = 1, flag = wx.ALL| wx.EXPAND, border = 5)
        self.btnCancel = wx.Button(parent = mainPanel, id = wx.ID_CANCEL)
        self.btnOK     = wx.Button(parent = mainPanel, id = wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)
        
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
                      
        # show panel with the largest number of images and fit size
        count = []
        for folder in os.listdir(self.symbolPath):
            count.append(len(os.listdir(os.path.join(self.symbolPath, folder))))
            
        index = count.index(max(count))
        self.folderChoice.SetSelection(index)
        self.OnFolderSelect(None)
        self.infoLabel.Show()
        
        mainPanel.SetSizerAndFit(mainSizer)
        self.SetSize(self.GetBestSize())
        
        # show currently selected symbol
        if self.currentSymbol:
            # set directory
            self.selectedDir, self.selected = os.path.split(self.currentSymbol)
            self.folderChoice.SetStringSelection(self.selectedDir)
            # select symbol
            panelIdx = self.folderChoice.GetSelection()
            for panel in self.symbolPanels[panelIdx]:
                if panel.GetName() == self.selected:
                    panel.Select()
        else:
            self.folderChoice.SetSelection(0)
            
        self.OnFolderSelect(None)
        
    def _createSymbolPanels(self, parent):
        """!Creates multiple panels with symbols.
        
        Panels are shown/hidden according to selected folder."""
        folders = os.listdir(self.symbolPath)
        
        panels = []
        self.symbolPanels = []
        
        for folder in folders:
            panel = wx.Panel(parent, style = wx.BORDER_RAISED)
            sizer = wx.GridSizer(cols = 6, vgap = 3, hgap = 3)
            images = self._getSymbols(path = os.path.join(self.symbolPath, folder))
        
            symbolPanels = []
            for img in images:
                iP = SingleSymbolPanel(parent = panel, symbolPath = img)
                iP.symbolSelectionChanged.connect(self.SelectionChanged)
                sizer.Add(item = iP, proportion = 0, flag = wx.ALIGN_CENTER)
                symbolPanels.append(iP)
            
            panel.SetSizerAndFit(sizer)
            panel.Hide()
            panels.append(panel)
            self.symbolPanels.append(symbolPanels)
            
        return panels
        
    def _getSymbols(self, path):
        # we assume that images are in subfolders (1 level only)
        imageList = []
        for image in os.listdir(path):
            imageList.append(os.path.join(path, image))
                
        return sorted(imageList)
            
    def OnFolderSelect(self, event):
        """!Selected folder with symbols changed."""
        idx = self.folderChoice.GetSelection()
        for i in range(len(self.panels)):
            sizer = self.panels[i].GetContainingSizer()
            sizer.Show(self.panels[i], i == idx, recursive = True)
            sizer.Layout()
        
        if self.selectedDir == self.folderChoice.GetStringSelection():
            self.btnOK.Enable()
            self.infoLabel.SetLabel(self.selected)
        else:
            self.btnOK.Disable()
            self.infoLabel.SetLabel('')
        
    def SelectionChanged(self, name, doubleClick):
        """!Selected symbol changed."""
        if doubleClick:
            self.EndModal(wx.ID_OK)
        # deselect all
        for i in range(len(self.panels)):
            for panel in self.symbolPanels[i]:
                if panel.GetName() != name:
                    panel.Deselect()
                
        self.btnOK.Enable()
        
        self.selected = name
        self.selectedDir = self.folderChoice.GetStringSelection()
        
        self.infoLabel.SetLabel(name)
        
    def GetSelectedSymbolName(self):
        """!Returns currently selected symbol name (e.g. 'basic/x').
        """ 
        # separator must be '/' and not dependent on OS
        return self.selectedDir + '/' + self.selected

    def GetSelectedSymbolPath(self):
        """!Returns currently selected symbol full path.
        """
        return os.path.join(self.symbolPath, self.selectedDir, self.selected)


class TextEntryDialog(wx.Dialog):
    """!Simple dialog with text field. 

    It differs from wx.TextEntryDialog because it allows adding validator.
    """
    def __init__(self, parent, message, caption='',
                 defaultValue='', pos=wx.DefaultPosition, validator=wx.DefaultValidator,
                 style=wx.OK | wx.CANCEL):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=caption, pos=pos)

        vbox = wx.BoxSizer(wx.VERTICAL)

        stline = wx.StaticText(self, id=wx.ID_ANY, label=message)
        vbox.Add(item=stline, proportion=0, flag=wx.EXPAND | wx.ALL, border=10)

        self._textCtrl = wx.TextCtrl(self, id=wx.ID_ANY, size = (300, -1),
                                     value=defaultValue, validator=validator)
        vbox.Add(item=self._textCtrl, proportion=0, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=10)
        self._textCtrl.SetFocus()

        sizer = self.CreateSeparatedButtonSizer(style)
        vbox.Add(item=sizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=10)

        self.SetSizerAndFit(vbox)
        
    def GetValue(self):
        return self._textCtrl.GetValue()

    def SetValue(self, value):
        self._textCtrl.SetValue(value)


class HyperlinkDialog(wx.Dialog):
    """!Dialog for displaying message with hyperlink."""
    def __init__(self, parent, title, message, hyperlink,
                hyperlinkLabel=None, style=wx.OK):
        """!Constructor

        @param parent gui parent         
        @param title dialog title
        @param message message
        @param hyperlink url
        @param hyperlinkLabel label shown instead of url
        @param style button style
        """
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title,
                           style=wx.DEFAULT_DIALOG_STYLE)

        sizer = wx.BoxSizer(wx.VERTICAL)

        label = wx.StaticText(self, label=message)
        sizer.Add(item=label, proportion=0, flag=wx.ALIGN_CENTRE|wx.ALL, border=10)
        hyperlinkLabel = hyperlinkLabel if hyperlinkLabel else hyperlink
        hyperlinkCtrl = wx.HyperlinkCtrl(self, id=wx.ID_ANY,
                                         label=hyperlinkLabel, url=hyperlink,
                                         style=wx.HL_ALIGN_LEFT|wx.HL_CONTEXTMENU)
        sizer.Add(item=hyperlinkCtrl, proportion=0, flag=wx.EXPAND|wx.ALL, border=10)        

        btnsizer = self.CreateSeparatedButtonSizer(style)
        sizer.Add(item=btnsizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=10)
    
        self.SetSizer(sizer)
        sizer.Fit(self)
