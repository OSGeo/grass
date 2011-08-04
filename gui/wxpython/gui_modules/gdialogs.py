"""!
@package gdialogs.py

@brief Various dialogs used in wxGUI.

List of classes:
 - ElementDialog
 - LocationDialog
 - MapsetDialog
 - NewVectorDialog
 - SavedRegion
 - DecorationDialog
 - TextLayerDialog 
 - AddMapLayersDialog
 - ImportDialog
 - GdalImportDialog
 - DxfImportDialog
 - LayersList (used by MultiImport) 
 - SetOpacityDialog
 - StaticWrapText
 - ImageSizeDialog
 - SqlQueryFrame

(C) 2008-2011 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import re

import wx
import wx.lib.filebrowsebutton as filebrowse
import wx.lib.mixins.listctrl as listmix

from grass.script import core as grass
from grass.script import task as gtask

import gcmd
import globalvar
import gselect
import menuform
import utils
from preferences import globalSettings as UserSettings

class ElementDialog(wx.Dialog):
    def __init__(self, parent, title, label, id = wx.ID_ANY,
                 etype = False, style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!General dialog to choose given element (location, mapset, vector map, etc.)
        
        @param parent window
        @param title window title
        @param label element label
        @param etype show also ElementSelect
        """
        wx.Dialog.__init__(self, parent, id, title, style = style, **kwargs)
        
        self.etype = etype
        self.label = label
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btnOK     = wx.Button(parent = self.panel, id = wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)
        
        if self.etype:
            self.typeSelect = gselect.ElementSelect(parent = self.panel,
                                                    size = globalvar.DIALOG_GSELECT_SIZE)
            self.typeSelect.Bind(wx.EVT_CHOICE, self.OnType)
        
        self.element = None # must be defined 
        
        self.__layout()
        
    def PostInit(self):
        self.element.SetFocus()
        self.element.Bind(wx.EVT_TEXT, self.OnElement)
        
    def OnType(self, event):
        """!Select element type"""
        if not self.etype:
            return
        evalue = self.typeSelect.GetValue(event.GetString())
        self.element.SetType(evalue)
        
    def OnElement(self, event):
        """!Name for vector map layer given"""
        if len(event.GetString()) > 0:
            self.btnOK.Enable(True)
        else:
            self.btnOK.Enable(False)
        
    def __layout(self):
        """!Do layout"""
        self.sizer = wx.BoxSizer(wx.VERTICAL)
        
        self.dataSizer = wx.BoxSizer(wx.VERTICAL)
        
        if self.etype:
            self.dataSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                                    label = _("Type of element:")),
                               proportion=0, flag=wx.ALL, border=1)
            self.dataSizer.Add(item = self.typeSelect,
                               proportion=0, flag=wx.ALL, border=1)
        
        self.dataSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                                label = self.label),
                           proportion=0, flag=wx.ALL, border=1)
        
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()
        
        self.sizer.Add(item=self.dataSizer, proportion=1,
                       flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
        
        self.sizer.Add(item=btnSizer, proportion=0,
                       flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
        
    def GetElement(self):
        """!Return (mapName, overwrite)"""
        return self.element.GetValue()
    
    def GetType(self):
        """!Get element type"""
        return self.element.tcp.GetType()
        
class LocationDialog(ElementDialog):
    """!Dialog used to select location"""
    def __init__(self, parent, title = _("Select GRASS location and mapset"), id =  wx.ID_ANY):
        ElementDialog.__init__(self, parent, title, label = _("Name of GRASS location:"))

        self.element = gselect.LocationSelect(parent = self.panel, id = wx.ID_ANY,
                                              size = globalvar.DIALOG_GSELECT_SIZE)

        self.element1 = gselect.MapsetSelect(parent = self.panel, id = wx.ID_ANY,
                                             size = globalvar.DIALOG_GSELECT_SIZE,
                                             setItems = False)
        
        self.PostInit()
        
        self.__Layout()
        self.SetMinSize(self.GetSize())

    def __Layout(self):
        """!Do layout"""
        self.dataSizer.Add(self.element, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=1)
 
        self.dataSizer.Add(wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                         label = _("Name of mapset:")), proportion=0,
                           flag=wx.EXPAND | wx.ALL, border=1)

        self.dataSizer.Add(self.element1, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=1)
       
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def OnElement(self, event):
        """!Select mapset given location name"""
        location = event.GetString()
        
        if location:
            dbase = grass.gisenv()['GISDBASE']
            self.element1.SetItems(utils.GetListOfMapsets(dbase, location, selectable = True))
            self.element1.SetSelection(0)
            mapset = self.element1.GetStringSelection()
        
        if location and mapset:
            self.btnOK.Enable(True)
        else:
            self.btnOK.Enable(False)

    def GetValues(self):
        """!Get location, mapset"""
        return (self.GetElement(), self.element1.GetStringSelection())
    
class MapsetDialog(ElementDialog):
    """!Dialog used to select mapset"""
    def __init__(self, parent, title = _("Select mapset in GRASS location"),
                 location = None, id =  wx.ID_ANY):
        ElementDialog.__init__(self, parent, title, label = _("Name of mapset:"))
        if location:
            self.SetTitle(self.GetTitle() + ' <%s>' % location)
        else:
            self.SetTitle(self.GetTitle() + ' <%s>' % grass.gisenv()['LOCATION_NAME'])
        
        self.element = gselect.MapsetSelect(parent = self.panel, id = wx.ID_ANY,
                                            size = globalvar.DIALOG_GSELECT_SIZE)
        
        self.PostInit()
        
        self.__Layout()
        self.SetMinSize(self.GetSize())

    def __Layout(self):
        """!Do layout"""
        self.dataSizer.Add(self.element, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=1)
        
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetMapset(self):
        return self.GetElement()
    
class NewVectorDialog(ElementDialog):
    def __init__(self, parent, id = wx.ID_ANY, title = _('Create new vector map'),
                 disableAdd = False, disableTable = False,
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, *kwargs):
        """!Dialog for creating new vector map

        @param parent parent window
        @param id window id
        @param title window title
        @param disableAdd disable 'add layer' checkbox
        @param disableTable disable 'create table' checkbox
        @param style window style
        @param kwargs other argumentes for ElementDialog
        
        @return dialog instance       
        """
        ElementDialog.__init__(self, parent, title, label = _("Name for new vector map:"))
        
        self.element = gselect.Select(parent = self.panel, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                                      type = 'vector', mapsets = [grass.gisenv()['MAPSET'],])
        
        self.table = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                 label = _("Create attribute table"))
        self.table.SetValue(True)
        if disableTable:
            self.table.Enable(False)
        
        self.addbox = wx.CheckBox(parent = self.panel,
                                  label = _('Add created map into layer tree'), style = wx.NO_BORDER)
        if disableAdd:
            self.addbox.SetValue(True)
            self.addbox.Enable(False)
        else:
            self.addbox.SetValue(UserSettings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))
        
        self.PostInit()
        
        self._layout()
        self.SetMinSize(self.GetSize())
        
    def OnMapName(self, event):
        """!Name for vector map layer given"""
        self.OnElement(event)
        
    def _layout(self):
        """!Do layout"""
        self.dataSizer.Add(self.element, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 1)
        
        self.dataSizer.Add(self.table, proportion = 0,
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
        name = self.GetElement()
        if full:
            if '@' in name:
                return name
            else:
                return name + '@' + grass.gisenv()['MAPSET']
        
        return name.split('@', 1)[0]

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
    
def CreateNewVector(parent, cmd, title = _('Create new vector map'),
                    exceptMap = None, log = None, disableAdd = False, disableTable = False):
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
    dlg = NewVectorDialog(parent, title = title,
                          disableAdd = disableAdd, disableTable = disableTable)
    
    if dlg.ShowModal() == wx.ID_OK:
        outmap = dlg.GetName()
        if outmap == exceptMap:
            gcmd.GError(parent = parent,
                        message = _("Unable to create vector map <%s>.") % outmap)
            dlg.Destroy()
            return None
        
        if outmap == '': # should not happen
            dlg.Destroy()
            return None
        
        cmd[1][cmd[2]] = outmap
        
        try:
            listOfVectors = grass.list_grouped('vect')[grass.gisenv()['MAPSET']]
        except KeyError:
            listOfVectors = []
        
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
        
        try:
            gcmd.RunCommand(prog = cmd[0],
                            overwrite = overwrite,
                            **cmd[1])
        except gcmd.GException, e:
            gcmd.GError(parent = self,
                        message = e.value)
            dlg.Destroy()
            return None
        
        # create attribute table
        if dlg.table.IsEnabled() and dlg.table.IsChecked():
            key = UserSettings.Get(group = 'atm', key = 'keycolumn', subkey = 'value')
            sql = 'CREATE TABLE %s (%s INTEGER)' % (outmap, key)
            
            gcmd.RunCommand('db.connect',
                            flags = 'c')
            
            gcmd.RunCommand('db.execute',
                            quiet = True,
                            parent = parent,
                            input = '-',
                            stdin = sql)
            
            gcmd.RunCommand('v.db.connect',
                            quiet = True,
                            parent = parent,
                            map = outmap,
                            table = outmap,
                            key = key,
                            layer = '1')
        
        # return fully qualified map name
        if '@' not in outmap:
            outmap += '@' + grass.gisenv()['MAPSET']
        
        if log:
            log.WriteLog(_("New vector map <%s> created") % outmap)

        return dlg
    
    return dlg

class SavedRegion(wx.Dialog):
    def __init__(self, parent, id = wx.ID_ANY, title="", loadsave='load',
                 **kwargs):
        """!Loading and saving of display extents to saved region file

        @param loadsave load or save region?
        """
        wx.Dialog.__init__(self, parent, id, title, **kwargs)

        self.loadsave = loadsave
        self.wind = ''
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(parent=self, id=wx.ID_ANY)
        box.Add(item=label, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
        if loadsave == 'load':
            label.SetLabel(_("Load region:"))
            selection = gselect.Select(parent=self, id=wx.ID_ANY, size=globalvar.DIALOG_GSELECT_SIZE,
                                       type='windows')
        elif loadsave == 'save':
            label.SetLabel(_("Save region:"))
            selection = gselect.Select(parent=self, id=wx.ID_ANY, size=globalvar.DIALOG_GSELECT_SIZE,
                                       type='windows', mapsets = [grass.gisenv()['MAPSET']])
        
        box.Add(item=selection, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
        selection.SetFocus()
        selection.Bind(wx.EVT_TEXT, self.OnRegion)
        
        sizer.Add(item=box, proportion=0, flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL,
                  border=5)
        
        line = wx.StaticLine(parent=self, id=wx.ID_ANY, size=(20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.RIGHT, border=5)
        
        btnsizer = wx.StdDialogButtonSizer()
        
        btn = wx.Button(parent = self, id = wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)
        
        btn = wx.Button(parent = self, id = wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()
        
        sizer.Add(item=btnsizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)
        
        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()
        
    def OnRegion(self, event):
        self.wind = event.GetString()
    
class DecorationDialog(wx.Dialog):
    """
    Controls setting options and displaying/hiding map overlay decorations
    """
    def __init__(self, parent, ovlId, title, cmd, name=None,
                 pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.DEFAULT_DIALOG_STYLE,
                 checktxt='', ctrltxt=''):

        wx.Dialog.__init__(self, parent, wx.ID_ANY, title, pos, size, style)

        self.ovlId   = ovlId   # PseudoDC id
        self.cmd     = cmd
        self.name    = name    # overlay name
        self.parent  = parent  # MapFrame

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.BoxSizer(wx.HORIZONTAL)
        self.chkbox = wx.CheckBox(parent=self, id=wx.ID_ANY, label=checktxt)
        if self.parent.Map.GetOverlay(self.ovlId) is None:
            self.chkbox.SetValue(True)
        else:
            self.chkbox.SetValue(self.parent.MapWindow.overlays[self.ovlId]['layer'].IsActive())
        box.Add(item=self.chkbox, proportion=0,
                flag=wx.ALIGN_CENTRE|wx.ALL, border=5)
        sizer.Add(item=box, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, border=5)

        box = wx.BoxSizer(wx.HORIZONTAL)
        optnbtn = wx.Button(parent=self, id=wx.ID_ANY, label=_("Set options"))
        box.Add(item=optnbtn, proportion=0, flag=wx.ALIGN_CENTRE|wx.ALL, border=5)
        sizer.Add(item=box, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, border=5)
        if self.name == 'legend':
            box = wx.BoxSizer(wx.HORIZONTAL)
            resize = wx.ToggleButton(parent=self, id=wx.ID_ANY, label=_("Set size and position"))
            resize.SetToolTipString(_("Click and drag on the map display to set legend"
                                        " size and position and then press OK"))
            resize.SetName('resize')
            if self.parent.toolbars['nviz']:
                resize.Disable()
            box.Add(item=resize, proportion=0, flag=wx.ALIGN_CENTRE|wx.ALL, border=5)
            sizer.Add(item=box, proportion=0,
                      flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, border=5)

        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label=_("Drag %s with mouse in pointer mode to position.\n"
                                      "Double-click to change options." % ctrltxt))
        if self.name == 'legend':
            label.SetLabel(label.GetLabel() + _('\nDefine raster map name for legend in '
                                                'properties dialog.'))
        box.Add(item=label, proportion=0,
                flag=wx.ALIGN_CENTRE|wx.ALL, border=5)
        sizer.Add(item=box, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, border=5)

        line = wx.StaticLine(parent=self, id=wx.ID_ANY, size=(20,-1), style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, border=5)

        # buttons
        btnsizer = wx.StdDialogButtonSizer()

        self.btnOK = wx.Button(parent=self, id=wx.ID_OK)
        self.btnOK.SetDefault()
        if self.name == 'legend':
            self.btnOK.Enable(False)
        btnsizer.AddButton(self.btnOK)

        btnCancel = wx.Button(parent=self, id=wx.ID_CANCEL)
        btnsizer.AddButton(btnCancel)
        btnsizer.Realize()

        sizer.Add(item=btnsizer, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)

        #
        # bindings
        #
        self.Bind(wx.EVT_BUTTON,   self.OnOptions, optnbtn)
        if self.name == 'legend':
            self.Bind(wx.EVT_TOGGLEBUTTON,   self.OnResize, resize)
        self.Bind(wx.EVT_BUTTON,   self.OnCancel,  btnCancel)
        self.Bind(wx.EVT_BUTTON,   self.OnOK,      self.btnOK)

        self.SetSizer(sizer)
        sizer.Fit(self)

        # create overlay if doesn't exist
        self._CreateOverlay()
        
        if len(self.parent.MapWindow.overlays[self.ovlId]['cmd']) > 1:
            if name == 'legend':
                mapName, found = utils.GetLayerNameFromCmd(self.parent.MapWindow.overlays[self.ovlId]['cmd'])
                if found:
                    # enable 'OK' button
                    self.btnOK.Enable()
                    # set title
                    self.SetTitle(_('Legend of raster map <%s>') % \
                                      mapName)
            
        
    def _CreateOverlay(self):
        if not self.parent.Map.GetOverlay(self.ovlId):
            self.newOverlay = self.parent.Map.AddOverlay(id=self.ovlId, type=self.name,
                                                 command=self.cmd,
                                                 l_active=False, l_render=False, l_hidden=True)
            prop = { 'layer' : self.newOverlay,
                     'params' : None,
                     'propwin' : None,
                     'cmd' : self.cmd,
                     'coords': (0, 0),
                     'pdcType': 'image' }
            self.parent.MapWindow2D.overlays[self.ovlId] = prop
            if self.parent.MapWindow3D:
                self.parent.MapWindow3D.overlays[self.ovlId] = prop
                
        else:
            if self.parent.MapWindow.overlays[self.ovlId]['propwin'] == None:
                return

            self.parent.MapWindow.overlays[self.ovlId]['propwin'].get_dcmd = self.GetOptData


    def OnOptions(self, event):
        """        self.SetSizer(sizer)
        sizer.Fit(self)

        Sets option for decoration map overlays
        """
        if self.parent.MapWindow.overlays[self.ovlId]['propwin'] is None:
            # build properties dialog
            menuform.GUI(parent = self.parent).ParseCommand(cmd=self.cmd,
                                                            completed=(self.GetOptData, self.name, ''))
            
        else:
            if self.parent.MapWindow.overlays[self.ovlId]['propwin'].IsShown():
                self.parent.MapWindow.overlays[self.ovlId]['propwin'].SetFocus()
            else:
                self.parent.MapWindow.overlays[self.ovlId]['propwin'].Show()
    
    def OnResize(self, event):
        if self.FindWindowByName('resize').GetValue():
            self.parent.MapWindow.SetCursor(self.parent.cursors["cross"])
            self.parent.MapWindow.mouse['use'] = 'legend'
            self.parent.MapWindow.mouse['box'] = 'box'
            self.parent.MapWindow.pen = wx.Pen(colour = 'Black', width = 2, style = wx.SHORT_DASH)
        else:
            self.parent.MapWindow.SetCursor(self.parent.cursors["default"])
            self.parent.MapWindow.mouse['use'] = 'pointer'
            
    def OnCancel(self, event):
        """!Cancel dialog"""
        if self.name == 'legend' and self.FindWindowByName('resize').GetValue():
            self.FindWindowByName('resize').SetValue(False)
            self.OnResize(None)
            
        self.parent.dialogs['barscale'] = None
        if event and hasattr(self, 'newOverlay'):
            self.parent.Map.DeleteOverlay(self.newOverlay)
        self.Destroy()

    def OnOK(self, event):
        """!Button 'OK' pressed"""
        # enable or disable overlay
        self.parent.Map.GetOverlay(self.ovlId).SetActive(self.chkbox.IsChecked())

        # update map
        if self.parent.MapWindow.parent.toolbars['nviz']:
            self.parent.MapWindow.UpdateOverlays()
        
        self.parent.MapWindow.UpdateMap()

        # close dialog
        self.OnCancel(None)

    def GetOptData(self, dcmd, layer, params, propwin):
        """!Process decoration layer data"""
        # update layer data
        if params:
            self.parent.MapWindow.overlays[self.ovlId]['params'] = params
        if dcmd:
            self.parent.MapWindow.overlays[self.ovlId]['cmd'] = dcmd
        self.parent.MapWindow.overlays[self.ovlId]['propwin'] = propwin

        # change parameters for item in layers list in render.Map
        # "Use mouse..." (-m) flag causes GUI freeze and is pointless here, trac #119
        
        try:
            self.parent.MapWindow.overlays[self.ovlId]['cmd'].remove('-m')
        except ValueError:
            pass
            
        self.parent.Map.ChangeOverlay(id=self.ovlId, type=self.name,
                                      command=self.parent.MapWindow.overlays[self.ovlId]['cmd'],
                                      l_active=self.parent.MapWindow.overlays[self.ovlId]['layer'].IsActive(),
                                      l_render=False, l_hidden=True)
        if  self.name == 'legend':
            if params and not self.btnOK.IsEnabled():
                self.btnOK.Enable()
            
class TextLayerDialog(wx.Dialog):
    """
    Controls setting options and displaying/hiding map overlay decorations
    """

    def __init__(self, parent, ovlId, title, name='text',
                 pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.DEFAULT_DIALOG_STYLE):

        wx.Dialog.__init__(self, parent, wx.ID_ANY, title, pos, size, style)
        from wx.lib.expando import ExpandoTextCtrl, EVT_ETC_LAYOUT_NEEDED

        self.ovlId = ovlId
        self.parent = parent

        if self.ovlId in self.parent.MapWindow.textdict.keys():
            self.currText = self.parent.MapWindow.textdict[self.ovlId]['text']
            self.currFont = self.parent.MapWindow.textdict[self.ovlId]['font']
            self.currClr  = self.parent.MapWindow.textdict[self.ovlId]['color']
            self.currRot  = self.parent.MapWindow.textdict[self.ovlId]['rotation']
            self.currCoords = self.parent.MapWindow.textdict[self.ovlId]['coords']
            self.currBB = self.parent.MapWindow.textdict[self.ovlId]['bbox']
        else:
            self.currClr = wx.BLACK
            self.currText = ''
            self.currFont = self.GetFont()
            self.currRot = 0.0
            self.currCoords = [10, 10]
            self.currBB = wx.Rect()

        self.sizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.GridBagSizer(vgap=5, hgap=5)

        # show/hide
        self.chkbox = wx.CheckBox(parent=self, id=wx.ID_ANY, \
            label = _('Show text object'))
        if self.parent.Map.GetOverlay(self.ovlId) is None:
            self.chkbox.SetValue(True)
        else:
            self.chkbox.SetValue(self.parent.MapWindow.overlays[self.ovlId]['layer'].IsActive())
        box.Add(item=self.chkbox, span=(1,2),
                flag=wx.ALIGN_LEFT|wx.ALL, border=5,
                pos=(0, 0))

        # text entry
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Enter text:"))
        box.Add(item=label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(1, 0))

        self.textentry = ExpandoTextCtrl(parent=self, id=wx.ID_ANY, value="", size=(300,-1))
        self.textentry.SetFont(self.currFont)
        self.textentry.SetForegroundColour(self.currClr)
        self.textentry.SetValue(self.currText)
        # get rid of unneeded scrollbar when text box first opened
        self.textentry.SetClientSize((300,-1))
        
        box.Add(item=self.textentry,
                pos=(1, 1))

        # rotation
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Rotation:"))
        box.Add(item=label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(2, 0))
        self.rotation = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="", pos=(30, 50),
                                    size=(75,-1), style=wx.SP_ARROW_KEYS)
        self.rotation.SetRange(-360, 360)
        self.rotation.SetValue(int(self.currRot))
        box.Add(item=self.rotation,
                flag=wx.ALIGN_RIGHT,
                pos=(2, 1))

        # font
        fontbtn = wx.Button(parent=self, id=wx.ID_ANY, label=_("Set font"))
        box.Add(item=fontbtn,
                flag=wx.ALIGN_RIGHT,
                pos=(3, 1))

        self.sizer.Add(item=box, proportion=1,
                  flag=wx.ALL, border=10)

        # note
        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label=_("Drag text with mouse in pointer mode "
                                      "to position.\nDouble-click to change options"))
        box.Add(item=label, proportion=0,
                flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
        self.sizer.Add(item=box, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER | wx.ALL, border=5)

        line = wx.StaticLine(parent=self, id=wx.ID_ANY,
                             size=(20,-1), style=wx.LI_HORIZONTAL)
        self.sizer.Add(item=line, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTRE | wx.ALL, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(parent=self, id=wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(parent=self, id=wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        self.sizer.Add(item=btnsizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(self.sizer)
        self.sizer.Fit(self)

        # bindings
        self.Bind(EVT_ETC_LAYOUT_NEEDED, self.OnRefit, self.textentry)
        self.Bind(wx.EVT_BUTTON,     self.OnSelectFont, fontbtn)
        self.Bind(wx.EVT_TEXT,       self.OnText,       self.textentry)
        self.Bind(wx.EVT_SPINCTRL,   self.OnRotation,   self.rotation)

    def OnRefit(self, event):
        """!Resize text entry to match text"""
        self.sizer.Fit(self)

    def OnText(self, event):
        """!Change text string"""
        self.currText = event.GetString()

    def OnRotation(self, event):
        """!Change rotation"""
        self.currRot = event.GetInt()

        event.Skip()

    def OnSelectFont(self, event):
        """!Change font"""
        data = wx.FontData()
        data.EnableEffects(True)
        data.SetColour(self.currClr)         # set colour
        data.SetInitialFont(self.currFont)

        dlg = wx.FontDialog(self, data)

        if dlg.ShowModal() == wx.ID_OK:
            data = dlg.GetFontData()
            self.currFont = data.GetChosenFont()
            self.currClr = data.GetColour()

            self.textentry.SetFont(self.currFont)
            self.textentry.SetForegroundColour(self.currClr)

            self.Layout()

        dlg.Destroy()

    def GetValues(self):
        """!Get text properties"""
        return { 'text' : self.currText,
                 'font' : self.currFont,
                 'color' : self.currClr,
                 'rotation' : self.currRot,
                 'coords' : self.currCoords,
                 'active' : self.chkbox.IsChecked() }

class AddMapLayersDialog(wx.Dialog):
    """!Add selected map layers (raster, vector) into layer tree"""
    def __init__(self, parent, title, style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=title, style=style)

        self.parent = parent # GMFrame
        
        #
        # dialog body
        #
        self.bodySizer = self.__createDialogBody()
        # update list of layer to be loaded
        self.map_layers = [] # list of map layers (full list type/mapset)
        self.LoadMapLayers(self.layerType.GetStringSelection()[:4],
                           self.mapset.GetStringSelection())
        #
        # buttons
        #
        btnCancel = wx.Button(parent = self, id = wx.ID_CANCEL)
        btnOk = wx.Button(parent = self, id = wx.ID_OK, label = _("&Add"))
        btnOk.SetDefault()
        btnOk.SetToolTipString(_("Add selected map layers to current display"))

        #
        # sizers & do layout
        #
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=self.bodySizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(item=btnSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

        # set dialog min size
        self.SetMinSize(self.GetSize())
        
    def __createDialogBody(self):
        bodySizer = wx.GridBagSizer(vgap=3, hgap=3)
        bodySizer.AddGrowableCol(1)
        bodySizer.AddGrowableRow(3)
        
        # layer type
        bodySizer.Add(item=wx.StaticText(parent=self, label=_("Map layer type:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0,0))

        self.layerType = wx.Choice(parent=self, id=wx.ID_ANY,
                                   choices=['raster', 'vector'], size=(100,-1))
        self.layerType.SetSelection(0)
        bodySizer.Add(item=self.layerType,
                      pos=(0,1))

        # select toggle
        self.toggle = wx.CheckBox(parent=self, id=wx.ID_ANY,
                                  label=_("Select toggle"))
        self.toggle.SetValue(True)
        bodySizer.Add(item=self.toggle,
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0,2))
        
        # mapset filter
        bodySizer.Add(item=wx.StaticText(parent=self, label=_("Mapset:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(1,0))

        self.mapset = gselect.MapsetSelect(parent = self)
        self.mapset.SetStringSelection(grass.gisenv()['MAPSET'])
        bodySizer.Add(item=self.mapset,
                      pos=(1,1), span=(1, 2))

        # map name filter
        bodySizer.Add(item=wx.StaticText(parent=self, label=_("Filter:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(2,0))

        self.filter = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                                  value="",
                                  size=(250,-1))
        bodySizer.Add(item=self.filter,
                      flag=wx.EXPAND,
                      pos=(2,1), span=(1, 2))

        # layer list 
        bodySizer.Add(item=wx.StaticText(parent=self, label=_("List of maps:")),
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_TOP,
                      pos=(3,0))
        self.layers = wx.CheckListBox(parent=self, id=wx.ID_ANY,
                                      size=(250, 100),
                                      choices=[])
        bodySizer.Add(item=self.layers,
                      flag=wx.EXPAND,
                      pos=(3,1), span=(1, 2))

        # bindings
        self.layerType.Bind(wx.EVT_CHOICE, self.OnChangeParams)
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
            self.layers.Check(item)

    def OnChangeParams(self, event):
        """!Filter parameters changed by user"""
        # update list of layer to be loaded
        self.LoadMapLayers(self.layerType.GetStringSelection()[:4],
                           self.mapset.GetStringSelection())
    
        event.Skip()

    def OnMenu(self, event):
        """!Table description area, context menu"""
        if not hasattr(self, "popupID1"):
            self.popupDataID1 = wx.NewId()
            self.popupDataID2 = wx.NewId()
            self.popupDataID3 = wx.NewId()

            self.Bind(wx.EVT_MENU, self.OnSelectAll,    id=self.popupDataID1)
            self.Bind(wx.EVT_MENU, self.OnSelectInvert, id=self.popupDataID2)
            self.Bind(wx.EVT_MENU, self.OnDeselectAll,  id=self.popupDataID3)

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
                if re.compile('^' + event.GetString()).search(layer):
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

        # return fully qualified map names
        mapset = self.mapset.GetStringSelection()
        for item in range(self.layers.GetCount()):
            if not self.layers.IsChecked(item):
                continue
            layerNames.append(self.layers.GetString(item) + '@' + mapset)

        return layerNames
    
    def GetLayerType(self):
        """!Get selected layer type"""
        return self.layerType.GetStringSelection()
    
class ImportDialog(wx.Dialog):
    """!Dialog for bulk import of various data (base class)"""
    def __init__(self, parent, itype,
                 id = wx.ID_ANY, title = _("Multiple import"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent    # GMFrame 
        self.importType = itype
        self.options = dict()   # list of options
        
        self.commandId = -1  # id of running command
        
        wx.Dialog.__init__(self, parent, id, title, style=style,
                           name = "MultiImportDialog")
        
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)
        
        self.layerBox = wx.StaticBox(parent=self.panel, id=wx.ID_ANY,
                                     label=_(" List of %s layers ") % self.importType.upper())
        
        #
        # list of layers
        #
        self.list = LayersList(self.panel)
        self.list.LoadData()

        self.optionBox = wx.StaticBox(parent=self.panel, id=wx.ID_ANY,
                                      label="%s" % _("Options"))
        
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
        
        
        self.overwrite = wx.CheckBox(parent=self.panel, id=wx.ID_ANY,
                                     label=_("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))
        
        self.add = wx.CheckBox(parent=self.panel, id=wx.ID_ANY)
        
        #
        # buttons
        #
        # cancel
        self.btn_cancel = wx.Button(parent=self.panel, id=wx.ID_CANCEL)
        self.btn_cancel.SetToolTipString(_("Close dialog"))
        self.btn_cancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        # run
        self.btn_run = wx.Button(parent=self.panel, id=wx.ID_OK, label = _("&Import"))
        self.btn_run.SetToolTipString(_("Import selected layers"))
        self.btn_run.SetDefault()
        self.btn_run.Enable(False)
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnRun)
        # run command dialog
        self.btn_cmd = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                 label = _("Command dialog"))
        self.btn_cmd.Bind(wx.EVT_BUTTON, self.OnCmdDialog)
        
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

        layerSizer.Add(item=self.list, proportion=1,
                      flag=wx.ALL | wx.EXPAND, border=5)
        
        dialogSizer.Add(item=layerSizer, proportion=1,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)

        # options
        optionSizer = wx.StaticBoxSizer(self.optionBox, wx.VERTICAL)
        for key in self.options.keys():
            optionSizer.Add(item=self.options[key], proportion=0)
            
        dialogSizer.Add(item=optionSizer, proportion=0,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)
        
        dialogSizer.Add(item=self.overwrite, proportion=0,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)
        
        dialogSizer.Add(item=self.add, proportion=0,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)
        
        #
        # buttons
        #
        btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)
        
        btnsizer.Add(item=self.btn_cmd, proportion=0,
                     flag=wx.ALL | wx.ALIGN_CENTER,
                     border=10)
        
        btnsizer.Add(item=self.btn_run, proportion=0,
                     flag=wx.ALL | wx.ALIGN_CENTER,
                     border=10)
        
        btnsizer.Add(item=self.btn_cancel, proportion=0,
                     flag=wx.ALL | wx.ALIGN_CENTER,
                     border=10)
        
        dialogSizer.Add(item=btnsizer, proportion=0,
                        flag=wx.ALIGN_CENTER)
        
        # dialogSizer.SetSizeHints(self.panel)
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)
        
        # auto-layout seems not work here - FIXME
        size = wx.Size(globalvar.DIALOG_GSELECT_SIZE[0] + 225, 550)
        self.SetMinSize(size)
        self.SetSize((size.width, size.height + 100))
        width = self.GetSize()[0]
        self.list.SetColumnWidth(col=1, width=width/2 - 50)
        self.Layout()

    def _getCommand(self):
        """!Get command"""
        return ''
    
    def OnCancel(self, event=None):
        """!Close dialog"""
        self.Close()

    def OnRun(self, event):
        """!Import/Link data (each layes as separate vector map)"""
        pass

    def OnCmdDialog(self, event):
        """!Show command dialog"""
        pass
    
    def AddLayers(self, returncode, cmd = None):
        """!Add imported/linked layers into layer tree"""
        self.commandId += 1
        
        if not self.add.IsChecked() or returncode != 0:
            return
        
        maptree = self.parent.curr_page.maptree
        
        layer, output = self.list.GetLayers()[self.commandId]
        
        if '@' not in output:
            name = output + '@' + grass.gisenv()['MAPSET']
        else:
            name = output
        
        # add imported layers into layer tree
        if self.importType == 'gdal':
            cmd = ['d.rast',
                   'map=%s' % name]
            if UserSettings.Get(group='cmd', key='rasterOverlay', subkey='enabled'):
                cmd.append('-o')
                
            item = maptree.AddLayer(ltype = 'raster',
                                    lname = name, lchecked = False,
                                    lcmd = cmd)
        else:
            item = maptree.AddLayer(ltype = 'vector',
                                    lname = name, lchecked = False,
                                    lcmd = ['d.vect',
                                            'map=%s' % name])
        
        maptree.mapdisplay.MapWindow.ZoomToMap()
        
    def OnAbort(self, event):
        """!Abort running import

        @todo not yet implemented
        """
        pass

class GdalImportDialog(ImportDialog):
    """!Dialog for bulk import of various raster/vector data"""
    def __init__(self, parent, ogr = False, link = False):
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
       
        self.dsnInput = gselect.GdalSelect(parent = self, panel = self.panel, ogr = ogr)

        if link:
            self.add.SetLabel(_("Add linked layers into layer tree"))
        else:
            self.add.SetLabel(_("Add imported layers into layer tree"))
        
        self.add.SetValue(UserSettings.Get(group='cmd', key='addNewLayer', subkey='enabled'))

        if link:
            self.btn_run.SetLabel(_("&Link"))
            self.btn_run.SetToolTipString(_("Link selected layers"))
            if ogr:
                self.btn_cmd.SetToolTipString(_('Open %s dialog') % 'v.external')
            else:
                self.btn_cmd.SetToolTipString(_('Open %s dialog') % 'r.external')
        else:
            self.btn_run.SetLabel(_("&Import"))
            self.btn_run.SetToolTipString(_("Import selected layers"))
            if ogr:
                self.btn_cmd.SetToolTipString(_('Open %s dialog') % 'v.in.ogr')
            else:
                self.btn_cmd.SetToolTipString(_('Open %s dialog') % 'r.in.gdal')
        
        self.doLayout()

    def OnRun(self, event):
        """!Import/Link data (each layes as separate vector map)"""
        data = self.list.GetLayers()
        
        # hide dialog
        self.Hide()
        
        dsn = self.dsnInput.GetDsn()
        ext = self.dsnInput.GetFormatExt()
            
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
            
            if UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'):
                cmd.append('--overwrite')
            
            # run in Layer Manager
            self.parent.goutput.RunCmd(cmd, switchPage = True,
                                       onDone = self.AddLayers)
        
        self.OnCancel()

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
    
    def OnCmdDialog(self, event):
        """!Show command dialog"""
        name = self._getCommand()
        menuform.GUI(parent = self, modal = True).ParseCommand(cmd = [name])
                
class DxfImportDialog(ImportDialog):
    """!Dialog for bulk import of DXF layers""" 
    def __init__(self, parent):
        ImportDialog.__init__(self, parent, itype = 'dxf',
                              title = _("Import DXF layers"))
        
        self.dsnInput = filebrowse.FileBrowseButton(parent=self.panel, id=wx.ID_ANY, 
                                                    size=globalvar.DIALOG_GSELECT_SIZE, labelText='',
                                                    dialogTitle=_('Choose DXF file to import'),
                                                    buttonText=_('Browse'),
                                                    startDirectory=os.getcwd(), fileMode=0,
                                                    changeCallback=self.OnSetDsn,
                                                    fileMask="DXF File (*.dxf)|*.dxf")
        
        self.add.SetLabel(_("Add imported layers into layer tree"))
        
        self.add.SetValue(UserSettings.Get(group='cmd', key='addNewLayer', subkey='enabled'))
        
        self.doLayout()

    def _getCommand(self):
        """!Get command"""
        return 'v.in.dxf'
    
    def OnRun(self, event):
        """!Import/Link data (each layes as separate vector map)"""
        data = self.list.GetLayers()
        
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
                    UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'):
                cmd.append('--overwrite')
            
            # run in Layer Manager
            self.parent.goutput.RunCmd(cmd, switchPage=True,
                                       onDone = self.AddLayers)
        
        self.OnCancel()

    def OnSetDsn(self, event):
        """!Input DXF file defined, update list of layer widget"""
        path = event.GetString()
        if not path:
            return 
        
        data = list()        
        ret = gcmd.RunCommand('v.in.dxf',
                              quiet = True,
                              parent = self,
                              read = True,
                              flags = 'l',
                              input = path)
        if not ret:
            self.list.LoadData()
            self.btn_run.Enable(False)
            return
            
        for line in ret.splitlines():
            layerId = line.split(':')[0].split(' ')[1]
            layerName = line.split(':')[1].strip()
            grassName = utils.GetValidLayerName(layerName)
            data.append((layerId, layerName.strip(), grassName.strip()))
        
        self.list.LoadData(data)
        if len(data) > 0:
            self.btn_run.Enable(True)
        else:
            self.btn_run.Enable(False)

    def OnCmdDialog(self, event):
        """!Show command dialog"""
        menuform.GUI(parent = self, modal = True).ParseCommand(cmd = ['v.in.dxf'])
                
class LayersList(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin,
                 listmix.CheckListCtrlMixin, listmix.TextEditMixin):
    """!List of layers to be imported (dxf, shp...)"""
    def __init__(self, parent, pos = wx.DefaultPosition,
                 log = None):
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, wx.ID_ANY,
                             style=wx.LC_REPORT)
        listmix.CheckListCtrlMixin.__init__(self)
        self.log = log

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.TextEditMixin.__init__(self)

        self.InsertColumn(0, _('Layer'))
        self.InsertColumn(1, _('Layer name'))
        self.InsertColumn(2, _('Name for GRASS map'))
        
        self.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnPopupMenu) #wxMSW
        self.Bind(wx.EVT_RIGHT_UP,            self.OnPopupMenu) #wxGTK

    def LoadData(self, data=None):
        """!Load data into list"""
        if data is None:
            return

        self.DeleteAllItems()
        
        for id, name, grassName in data:
            index = self.InsertStringItem(sys.maxint, str(id))
            self.SetStringItem(index, 1, "%s" % str(name))
            self.SetStringItem(index, 2, "%s" % str(grassName))
            # check by default
            self.CheckItem(index, True)
        
        self.SetColumnWidth(col=0, width=wx.LIST_AUTOSIZE_USEHEADER)

    def OnPopupMenu(self, event):
        """!Show popup menu"""
        if self.GetItemCount() < 1:
            return
        
        if not hasattr(self, "popupDataID1"):
            self.popupDataID1 = wx.NewId()
            self.popupDataID2 = wx.NewId()
            
            self.Bind(wx.EVT_MENU, self.OnSelectAll,  id=self.popupDataID1)
            self.Bind(wx.EVT_MENU, self.OnSelectNone, id=self.popupDataID2)
        
        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupDataID1, _("Select all"))
        menu.Append(self.popupDataID2, _("Deselect all"))

        self.PopupMenu(menu)
        menu.Destroy()

    def OnSelectAll(self, event):
        """!Select all items"""
        item = -1
        
        while True:
            item = self.GetNextItem(item)
            if item == -1:
                break
            self.CheckItem(item, True)

        event.Skip()
        
    def OnSelectNone(self, event):
        """!Deselect items"""
        item = -1
        
        while True:
            item = self.GetNextItem(item, wx.LIST_STATE_SELECTED)
            if item == -1:
                break
            self.CheckItem(item, False)

        event.Skip()
        
    def GetLayers(self):
        """!Get list of layers (layer name, output name)"""
        data = []
        item = -1
        while True:
            item = self.GetNextItem(item)
            if item == -1:
                break
            if self.IsChecked(item):
                # layer / output name
                data.append((self.GetItem(item, 1).GetText(),
                             self.GetItem(item, 2).GetText()))

        return data

class SetOpacityDialog(wx.Dialog):
    """!Set opacity of map layers"""
    def __init__(self, parent, id=wx.ID_ANY, title=_("Set Map Layer Opacity"),
                 size=wx.DefaultSize, pos=wx.DefaultPosition,
                 style=wx.DEFAULT_DIALOG_STYLE, opacity=100):

        self.parent = parent    # GMFrame
        self.opacity = opacity  # current opacity

        super(SetOpacityDialog, self).__init__(parent, id=id, pos=pos,
                                               size=size, style=style, title=title)

        panel = wx.Panel(parent=self, id=wx.ID_ANY)
        
        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.GridBagSizer(vgap=5, hgap=5)
        self.value = wx.Slider(panel, id=wx.ID_ANY, value=self.opacity,
                               style=wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | \
                                   wx.SL_TOP | wx.SL_LABELS,
                               minValue=0, maxValue=100,
                               size=(350, -1))

        box.Add(item=self.value,
                flag=wx.ALIGN_CENTRE, pos=(0, 0), span=(1, 2))
        box.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                   label=_("transparent")),
                pos=(1, 0))
        box.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                   label=_("opaque")),
                flag=wx.ALIGN_RIGHT,
                pos=(1, 1))

        sizer.Add(item=box, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)

        line = wx.StaticLine(parent=panel, id=wx.ID_ANY,
                             style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)

        # buttons
        btnsizer = wx.StdDialogButtonSizer()

        btnOK = wx.Button(parent=panel, id=wx.ID_OK)
        btnOK.SetDefault()
        btnsizer.AddButton(btnOK)

        btnCancel = wx.Button(parent=panel, id=wx.ID_CANCEL)
        btnsizer.AddButton(btnCancel)
        btnsizer.Realize()

        sizer.Add(item=btnsizer, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)

        panel.SetSizer(sizer)
        sizer.Fit(panel)

        self.SetSize(self.GetBestSize())

        self.Layout()

    def GetOpacity(self):
        """!Button 'OK' pressed"""
        # return opacity value
        opacity = float(self.value.GetValue()) / 100
        return opacity

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

class StaticWrapText(wx.StaticText):
    """!A Static Text field that wraps its text to fit its width,
    enlarging its height if necessary.
    """
    def __init__(self, parent, id = wx.ID_ANY, label = '', *args, **kwds):
        self.parent        = parent
        self.originalLabel = label
        
        wx.StaticText.__init__(self, parent, id, label = '', *args, **kwds)
        
        self.SetLabel(label)
        self.Bind(wx.EVT_SIZE, self.OnResize)
    
    def SetLabel(self, label):
        self.originalLabel = label
        self.wrappedSize = None
        self.OnResize(None)

    def OnResize(self, event):
        if not getattr(self, "resizing", False):
            self.resizing = True
            newSize = wx.Size(self.parent.GetSize().width - 50,
                              self.GetSize().height)
            if self.wrappedSize != newSize:
                wx.StaticText.SetLabel(self, self.originalLabel)
                self.Wrap(newSize.width)
                self.wrappedSize = newSize
                
                self.SetSize(self.wrappedSize)
            del self.resizing

class ImageSizeDialog(wx.Dialog):
    """!Set size for saved graphic file"""
    def __init__(self, parent, id = wx.ID_ANY, title=_("Set image size"),
                 style = wx.DEFAULT_DIALOG_STYLE, **kwargs):
        self.parent = parent
        
        wx.Dialog.__init__(self, parent, id = id, style=style, title=title, **kwargs)
        
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
