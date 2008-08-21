"""
@package gdialogs.py

@brief Common dialog used in wxGUI.

List of classes:
 - NewVectorDialog
 - SavedRegion
 - DecorationDialog
 - TextLayerDialog 
 - LoadMapLayersDialog
 - MultiImportDialog
 - LayerList (used by MultiImport) 
 - SetOpacityDialog

(C) 2008 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import re
import glob

import wx
import wx.lib.filebrowsebutton as filebrowse
import wx.lib.mixins.listctrl as listmix

import gcmd
import grassenv
import globalvar
import gselect
import menuform
import utils
import grass
from preferences import globalSettings as UserSettings

class NewVectorDialog(wx.Dialog):
    """Create new vector map layer"""
    def __init__(self, parent, id, title, 
                style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):

        wx.Dialog.__init__(self, parent, id, title, style=style)

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.btnCancel = wx.Button(self.panel, wx.ID_CANCEL)
        self.btnOK = wx.Button(self.panel, wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)

        self.label = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                   label=_("Name for new vector map:"))
        self.mapName = gselect.Select(parent=self.panel, id=wx.ID_ANY, size=globalvar.DIALOG_GSELECT_SIZE,
                                      type='vector', mapsets=[grassenv.GetGRASSVariable('MAPSET'),])

        self.mapName.Bind(wx.EVT_TEXT, self.OnMapName)
        
        self.__Layout()

        self.SetMinSize(self.GetSize())

    def OnMapName(self, event):
        """Name for vector map layer given"""
        if len(event.GetString()) > 0:
            self.btnOK.Enable(True)
        else:
            self.btnOK.Enable(False)

    def __Layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        dataSizer = wx.BoxSizer(wx.VERTICAL)
        dataSizer.Add(self.label, proportion=0,
                      flag=wx.ALL, border=1)
        dataSizer.Add(self.mapName, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=1)
        
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()

        sizer.Add(item=dataSizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
       
        self.panel.SetSizer(sizer)
        sizer.Fit(self)

    def GetName(self):
        """Return (mapName, overwrite)"""
        mapName = self.mapName.GetValue().split('@', 1)[0]

        return mapName
    
def CreateNewVector(parent, cmdDef, title=_('Create new vector map'),
                    exceptMap=None, log=None):
    """Create new vector map layer

    @cmdList tuple/list (cmd list, output paramater)
    
    @return name of create vector map
    @return None of failure
    """
    cmd = cmdDef[0]
    dlg = NewVectorDialog(parent=parent, id=wx.ID_ANY, title=title)
    if dlg.ShowModal() == wx.ID_OK:
        outmap = dlg.GetName()
        if outmap == exceptMap:
            wx.MessageBox(parent=parent,
                          message=_("Unable to create vector map <%s>.") % outmap,
                          caption=_("Error"),
                          style=wx.ID_OK | wx.ICON_ERROR | wx.CENTRE)
            return False
        
        if outmap == '': # should not happen
            return False
        
        cmd.append("%s=%s" % (cmdDef[1], outmap))
        
        try:
            listOfVectors = grass.list_grouped('vect')[grass.gisenv()['MAPSET']]
        except KeyError:
            listOfVectors = []
        
        if not UserSettings.Get(group='cmd', key='overwrite', subkey='enabled') and \
                outmap in listOfVectors:
            dlg = wx.MessageDialog(parent, message=_("Vector map <%s> already exists "
                                                     "in the current mapset. "
                                                     "Do you want to overwrite it?") % outmap,
                                   caption=_("Overwrite?"),
                                   style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_YES:
                cmd.append('--overwrite')
            else:
                dlg.Destroy()
                return False

        if UserSettings.Get(group='cmd', key='overwrite', subkey='enabled') is True:
            cmd.append('--overwrite')
        
        try:
            gcmd.Command(cmd)
        except gcmd.CmdError, e:
            print >> sys.stderr, e
            return None

        # return fully qualified map name
        if '@' not in outmap:
            outmap += '@' + grassenv.GetGRASSVariable('MAPSET')

        if log:
            log.WriteLog(_("New vector map <%s> created") % outmap)

        return outmap

    return None

class SavedRegion(wx.Dialog):
    def __init__(self, parent, id, title="", pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE,
                 loadsave='load'):
        """
        Loading and saving of display extents to saved region file
        """
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)

        self.loadsave = loadsave
        self.wind = ''

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.BoxSizer(wx.HORIZONTAL)
        if loadsave == 'load':
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Load region:"))
            box.Add(item=label, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
            self.selection = gselect.Select(parent=self, id=wx.ID_ANY, size=globalvar.DIALOG_GSELECT_SIZE,
                                            type='windows')
            box.Add(item=self.selection, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
            self.selection.Bind(wx.EVT_TEXT, self.OnSelection)

        elif loadsave == 'save':
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Save region:"))
            box.Add(item=label, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
            self.textentry = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="",
                                         size=globalvar.DIALOG_TEXTCTRL_SIZE)
            box.Add(item=self.textentry, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
            self.textentry.Bind(wx.EVT_TEXT, self.OnText)

        sizer.Add(item=box, proportion=0, flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL,
                  border=5)

        line = wx.StaticLine(parent=self, id=wx.ID_ANY, size=(20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.RIGHT, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(self, wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(self, wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(item=btnsizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnSelection(self, event):
        self.wind = event.GetString()

    def OnText(self, event):
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
        self.Bind(wx.EVT_BUTTON,   self.OnCancel,  btnCancel)
        self.Bind(wx.EVT_BUTTON,   self.OnOK,      self.btnOK)

        self.SetSizer(sizer)
        sizer.Fit(self)

        # create overlay if doesn't exist
        self._CreateOverlay()

        if len(self.parent.MapWindow.overlays[self.ovlId]['cmd']) > 1:
            mapName = utils.GetLayerNameFromCmd(self.parent.MapWindow.overlays[self.ovlId]['cmd'])
            if self.parent.MapWindow.overlays[self.ovlId]['propwin'] is None and mapName:
                # build properties dialog
                menuform.GUI().ParseCommand(cmd=self.cmd,
                                            completed=(self.GetOptData, self.name, ''),
                                            parentframe=self.parent, show=False)
            if mapName:
                # enable 'OK' button
                self.btnOK.Enable()
                # set title
                self.SetTitle(_('Legend of raster map <%s>') % \
                              mapName)
        
    def _CreateOverlay(self):
        if not self.parent.Map.GetOverlay(self.ovlId):
            overlay = self.parent.Map.AddOverlay(id=self.ovlId, type=self.name,
                                                 command=self.cmd,
                                                 l_active=False, l_render=False, l_hidden=True)

            self.parent.MapWindow.overlays[self.ovlId] = {}
            self.parent.MapWindow.overlays[self.ovlId] = { 'layer' : overlay,
                                                           'params' : None,
                                                           'propwin' : None,
                                                           'cmd' : self.cmd,
                                                           'coords': (10, 10),
                                                           'pdcType': 'image' }
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
            menuform.GUI().ParseCommand(cmd=self.cmd,
                                        completed=(self.GetOptData, self.name, ''),
                                        parentframe=self.parent)
        else:
            if self.parent.MapWindow.overlays[self.ovlId]['propwin'].IsShown():
                self.parent.MapWindow.overlays[self.ovlId]['propwin'].SetFocus()
            else:
                self.parent.MapWindow.overlays[self.ovlId]['propwin'].Show()

    def OnCancel(self, event):
        """Cancel dialog"""
        self.parent.dialogs['barscale'] = None

        self.Destroy()

    def OnOK(self, event):
        """Button 'OK' pressed"""
        # enable or disable overlay
        self.parent.Map.GetOverlay(self.ovlId).SetActive(self.chkbox.IsChecked())

        # update map
        self.parent.MapWindow.UpdateMap()

        # close dialog
        self.OnCancel(None)

    def GetOptData(self, dcmd, layer, params, propwin):
        """Process decoration layer data"""
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
        
            #self.SetTitle(_('Legend of raster map <%s>') % \
            #              utils.GetLayerNameFromCmd(self.parent.MapWindow.overlays[self.ovlId]['cmd']))
            
class TextLayerDialog(wx.Dialog):
    """
    Controls setting options and displaying/hiding map overlay decorations
    """

    def __init__(self, parent, ovlId, title, name='text',
                 pos=wx.DefaultPosition, size=wx.DefaultSize, style=wx.DEFAULT_DIALOG_STYLE):

        wx.Dialog.__init__(self, parent, wx.ID_ANY, title, pos, size, style)

        self.ovlId = ovlId
        self.parent = parent

        if self.ovlId in self.parent.MapWindow.textdict:
            self.currText, self.currFont, self.currClr, self.currRot = self.parent.MapWindow.textdict[drawid]
        else:
            self.currClr = wx.BLACK
            self.currText = ''
            self.currFont = self.GetFont()
            self.currRot = 0.0

        sizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.GridBagSizer(vgap=5, hgap=5)

        # text entry
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Enter text:"))
        box.Add(item=label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(0, 0))

        self.textentry = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(300,-1))
        self.textentry.SetFont(self.currFont)
        self.textentry.SetForegroundColour(self.currClr)
        self.textentry.SetValue(self.currText)
        box.Add(item=self.textentry,
                pos=(0, 1))

        # rotation
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Rotation:"))
        box.Add(item=label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(1, 0))
        self.rotation = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="", pos=(30, 50),
                                    size=(75,-1), style=wx.SP_ARROW_KEYS)
        self.rotation.SetRange(-360, 360)
        self.rotation.SetValue(int(self.currRot))
        box.Add(item=self.rotation,
                flag=wx.ALIGN_RIGHT,
                pos=(1, 1))

        # font
        fontbtn = wx.Button(parent=self, id=wx.ID_ANY, label=_("Set font"))
        box.Add(item=fontbtn,
                flag=wx.ALIGN_RIGHT,
                pos=(2, 1))

        sizer.Add(item=box, proportion=1,
                  flag=wx.ALL, border=10)

        # note
        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label=_("Drag text with mouse in pointer mode "
                                      "to position.\nDouble-click to change options"))
        box.Add(item=label, proportion=0,
                flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
        sizer.Add(item=box, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER | wx.ALL, border=5)

        line = wx.StaticLine(parent=self, id=wx.ID_ANY,
                             size=(20,-1), style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_CENTRE | wx.ALL, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(parent=self, id=wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(parent=self, id=wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(item=btnsizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)

        # bindings
        self.Bind(wx.EVT_BUTTON,     self.OnSelectFont, fontbtn)
        self.Bind(wx.EVT_TEXT,       self.OnText,       self.textentry)
        self.Bind(wx.EVT_SPINCTRL,   self.OnRotation,   self.rotation)

    def OnText(self, event):
        """Change text string"""
        self.currText = event.GetString()

    def OnRotation(self, event):
        """Change rotation"""
        self.currRot = event.GetInt()

        event.Skip()

    def OnSelectFont(self, event):
        """Change font"""
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
        """Get text properties"""
        return (self.currText, self.currFont,
                self.currClr, self.currRot)

class LoadMapLayersDialog(wx.Dialog):
    """Load selected map layers (raster, vector) into layer tree"""
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
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOk = wx.Button(self, wx.ID_OK, _("Load") )
        btnOk.SetDefault()

        #
        # bindigs
        #
        #btnOk.Bind(wx.EVT_BUTTON, self.OnOK)
        #btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)

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
        
        # mapset filter
        bodySizer.Add(item=wx.StaticText(parent=self, label=_("Mapset:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(1,0))

        self.mapset = wx.ComboBox(parent=self, id=wx.ID_ANY,
                                  style=wx.CB_SIMPLE | wx.CB_READONLY,
                                  choices=utils.ListOfMapsets(),
                                  size=(250,-1))
        self.mapset.SetStringSelection(grassenv.GetGRASSVariable("MAPSET"))
        bodySizer.Add(item=self.mapset,
                      pos=(1,1))

        # map name filter
        bodySizer.Add(item=wx.StaticText(parent=self, label=_("Filter:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(2,0))

        self.filter = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                                  value="",
                                  size=(250,-1))
        bodySizer.Add(item=self.filter,
                      flag=wx.EXPAND,
                      pos=(2,1))

        # layer list 
        bodySizer.Add(item=wx.StaticText(parent=self, label=_("List of maps:")),
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_TOP,
                      pos=(3,0))
        self.layers = wx.CheckListBox(parent=self, id=wx.ID_ANY,
                                      size=(250, 100),
                                      choices=[])
        bodySizer.Add(item=self.layers,
                      flag=wx.EXPAND,
                      pos=(3,1))

        # bindings
        self.layerType.Bind(wx.EVT_CHOICE, self.OnChangeParams)
        self.mapset.Bind(wx.EVT_COMBOBOX, self.OnChangeParams)
        self.layers.Bind(wx.EVT_RIGHT_DOWN, self.OnMenu)
        self.filter.Bind(wx.EVT_TEXT, self.OnFilter)
        return bodySizer

    def LoadMapLayers(self, type, mapset):
        """Load list of map layers

        @param type layer type ('raster' or 'vector')
        @param mapset mapset name
        """
        list = gcmd.Command(['g.mlist',
                             'type=%s' % type,
                             'mapset=%s' % mapset])

        self.map_layers = []
        for map in list.ReadStdOutput():
            self.map_layers.append(map)
            
        self.layers.Set(self.map_layers)
        
        # check all items by default
        for item in range(self.layers.GetCount()):
            self.layers.Check(item)

    def OnChangeParams(self, event):
        """Filter parameters changed by user"""
        # update list of layer to be loaded
        self.LoadMapLayers(self.layerType.GetStringSelection()[:4],
                           self.mapset.GetStringSelection())
    
        event.Skip()

    def OnMenu(self, event):
        """Table description area, context menu"""
        if not hasattr(self, "popupID1"):
            self.popupDataID1 = wx.NewId()
            self.popupDataID2 = wx.NewId()

            self.Bind(wx.EVT_MENU, self.OnSelectAll,   id=self.popupDataID1)
            self.Bind(wx.EVT_MENU, self.OnDeselectAll, id=self.popupDataID2)

        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupDataID1, _("Select all"))
        menu.Append(self.popupDataID2, _("Deselect all"))

        self.PopupMenu(menu)
        menu.Destroy()

    def OnSelectAll(self, event):
        """Select all map layer from list"""
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, True)

    def OnDeselectAll(self, event):
        """Select all map layer from list"""
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, False)

    def OnFilter(self, event):
        """Apply filter for map names"""
        if len(event.GetString()) == 0:
           self.layers.Set(self.map_layers) 
           return 

        list = []
        for layer in self.map_layers:
            if re.compile('^' + event.GetString()).search(layer):
                list.append(layer)

        self.layers.Set(list)

        event.Skip()

    def GetMapLayers(self):
        """Return list of checked map layers"""
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
        """Get selected layer type"""
        return self.layerType.GetStringSelection()
    
class MultiImportDialog(wx.Dialog):
    """Import dxf layers"""
    def __init__(self, parent, type,
                 id=wx.ID_ANY, title=_("Multiple import"), 
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):

        self.parent = parent # GMFrame 
        self.inputType = type
        
        wx.Dialog.__init__(self, parent, id, title, style=style)

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        #
        # input
        #
        if self.inputType == 'dxf':
            self.inputTitle = _("Input DXF file")
            self.inputText = wx.StaticText(self.panel, id=wx.ID_ANY, label=_("Choose DXF file:"))
            self.input = filebrowse.FileBrowseButton(parent=self.panel, id=wx.ID_ANY, 
                                                     size=globalvar.DIALOG_GSELECT_SIZE, labelText='',
                                                     dialogTitle=_('Choose DXF file to import'),
                                                     buttonText=_('Browse'),
                                                     startDirectory=os.getcwd(), fileMode=0,
                                                     changeCallback=self.OnSetInput,
                                                     fileMask="*.dxf")
        else:
            self.inputTitle = _("Input directory")
            self.inputText = wx.StaticText(self.panel, id=wx.ID_ANY, label=_("Choose directory:"))
            self.input = filebrowse.DirBrowseButton(parent=self.panel, id=wx.ID_ANY, 
                                                     size=globalvar.DIALOG_GSELECT_SIZE, labelText='',
                                                     dialogTitle=_('Choose input directory'),
                                                     buttonText=_('Browse'),
                                                     startDirectory=os.getcwd(),
                                                     changeCallback=self.OnSetInput)
            self.formatText = wx.StaticText(self.panel, id=wx.ID_ANY, label=_("Select file extension:"))
            self.format = wx.TextCtrl(parent=self.panel, id=wx.ID_ANY, size=(100, -1),
                                      value="")
            if self.inputType == 'gdal':
                self.format.SetValue('tif')
            else: # ogr
                self.format.SetValue('shp')

            self.format.Bind(wx.EVT_TEXT, self.OnSetInput)
        
        #
        # list of layers
        #
        self.list = LayersList(self.panel)
        self.list.LoadData()

        self.add = wx.CheckBox(parent=self.panel, id=wx.ID_ANY,
                               label=_("Add imported layers into layer tree"))
        self.add.SetValue(UserSettings.Get(group='cmd', key='addNewLayer', subkey='enabled'))

        #
        # buttons
        #
        # cancel
        self.btn_cancel = wx.Button(parent=self.panel, id=wx.ID_CANCEL)
        self.btn_cancel.SetToolTipString(_("Close dialog"))
        self.btn_cancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        # run
        self.btn_run = wx.Button(parent=self.panel, id=wx.ID_OK, label= _("&Import"))
        self.btn_run.SetToolTipString(_("Import selected layers"))
        self.btn_run.SetDefault()
        self.btn_run.Enable(False)
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnRun)
        
        self.__doLayout()
        self.Layout()

    def __doLayout(self):
        dialogSizer = wx.BoxSizer(wx.VERTICAL)
        #
        # input
        #
        inputBox = wx.StaticBox(parent=self.panel, id=wx.ID_ANY,
                                label=" %s " % self.inputTitle)
        inputSizer = wx.StaticBoxSizer(inputBox, wx.HORIZONTAL)
        
        gridSizer = wx.FlexGridSizer(cols=2, vgap=5, hgap=5)
       
        gridSizer.Add(item=self.inputText,
                      flag=wx.ALIGN_CENTER_VERTICAL)
        gridSizer.AddGrowableCol(1)
        gridSizer.Add(item=self.input,
                      flag=wx.EXPAND | wx.ALL)
        
        if self.inputType != 'dxf':
            gridSizer.Add(item=self.formatText,
                          flag=wx.ALIGN_CENTER_VERTICAL)
            gridSizer.Add(item=self.format)
        
        inputSizer.Add(item=gridSizer, proportion=1,
                       flag=wx.EXPAND | wx.ALL)
        
        dialogSizer.Add(item=inputSizer, proportion=0,
                        flag=wx.ALL | wx.EXPAND, border=5)

        #
        # list of DXF layers
        #
        layerBox = wx.StaticBox(parent=self.panel, id=wx.ID_ANY,
                                label=_(" List of %s layers ") % self.inputType.upper())
        layerSizer = wx.StaticBoxSizer(layerBox, wx.HORIZONTAL)

        layerSizer.Add(item=self.list, proportion=1,
                      flag=wx.ALL | wx.EXPAND, border=5)
        
        dialogSizer.Add(item=layerSizer, proportion=1,
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
        
        btnsizer.Add(item=self.btn_run, proportion=0,
                     flag=wx.ALL | wx.ALIGN_CENTER,
                     border=10)
        
        dialogSizer.Add(item=btnsizer, proportion=0,
                        flag=wx.ALIGN_CENTER)
        
        # dialogSizer.SetSizeHints(self.panel)
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)
        
        self.Layout()
        # auto-layout seems not work here - FIXME
        self.SetMinSize((globalvar.DIALOG_GSELECT_SIZE[0] + 175, 300))
        width = self.GetSize()[0]
        self.list.SetColumnWidth(col=1, width=width/2 - 50)

    def OnCancel(self, event=None):
        """Close dialog"""
        self.Close()

    def OnRun(self, event):
        """Import data (each layes as separate vector map)"""
        data = self.list.GetLayers()
        
        # hide dialog
        self.Hide()
        
        for layer, output in data:
            if self.inputType == 'dxf':
                cmd = ['v.in.dxf',
                       'input=%s' % self.input.GetValue(),
                       'layers=%s' % layer,
                       'output=%s' % output]
            elif self.inputType == 'ogr':
                cmd = ['v.in.ogr',
                       'dsn=%s' % (os.path.join(self.input.GetValue(), layer)),
                       'output=%s' % output]
            else:
                cmd = ['r.in.gdal', '-o', # override projection by default
                       'input=%s' % (os.path.join(self.input.GetValue(), layer)),
                       'output=%s' % output]
            
            if UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'):
                cmd.append('--overwrite')
            
            # run in Layer Manager
            self.parent.goutput.RunCmd(cmd)

        if self.add.IsChecked():
            maptree = self.parent.curr_page.maptree
            for layer, output in data:
                if '@' not in output:
                    name = output + '@' + grass.gisenv()['MAPSET']
                else:
                    name = output
                # add imported layers into layer tree
                if self.inputType == 'gdal':
                    cmd = ['d.rast',
                           'map=%s' % name]
                    if UserSettings.Get(group='cmd', key='rasterOverlay', subkey='enabled'):
                        cmd.append('-o')

                    maptree.AddLayer(ltype='raster',
                                     lname=name,
                                     lcmd=cmd)
                else:
                    maptree.AddLayer(ltype='vector',
                                     lname=name,
                                     lcmd=['d.vect',
                                           'map=%s' % name])
        
        wx.CallAfter(self.parent.notebook.SetSelection, 0)
        
        self.OnCancel()
        
    def OnAbort(self, event):
        """Abort running import

        @todo not yet implemented
        """
        pass
        
    def OnSetInput(self, event):
        """Input DXF file/OGR dsn defined, update list of layer widget"""
        path = event.GetString()

        if self.inputType == 'dxf':
            try:
                cmd = gcmd.Command(['v.in.dxf',
                                    'input=%s' % path,
                                    '-l', '--q'], stderr=None)
            except gcmd.CmdError, e:
                wx.MessageBox(parent=self, message=_("File <%(file)s>: Unable to get list of DXF layers.\n\n%(details)s") % \
                              { 'file' : path, 'details' : e.message },
                              caption=_("Error"), style=wx.ID_OK | wx.ICON_ERROR | wx.CENTRE)
                self.list.LoadData()
                self.btn_run.Enable(False)
                return

        data = []
        if self.inputType == 'dxf':
            for line in cmd.ReadStdOutput():
                layerId = line.split(':')[0].split(' ')[1]
                layerName = line.split(':')[1].strip()
                grassName = utils.GetValidLayerName(layerName)
                data.append((layerId, layerName.strip(), grassName.strip()))
                
        else: # gdal/ogr (for ogr maybe to use v.in.ogr -l)
            layerId = 1
            for file in glob.glob(os.path.join(self.input.GetValue(), "*.%s") % self.format.GetValue()):
                baseName = os.path.basename(file)
                grassName = utils.GetValidLayerName(baseName.split('.', -1)[0])
                data.append((layerId, baseName, grassName))
                layerId += 1
            
        self.list.LoadData(data)
        if len(data) > 0:
            self.btn_run.Enable(True)
        else:
            self.btn_run.Enable(False)
        
class LayersList(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin,
                 listmix.CheckListCtrlMixin):
#                 listmix.CheckListCtrlMixin, listmix.TextEditMixin):
    """List of layers to be imported (dxf, shp...)"""
    def __init__(self, parent, pos=wx.DefaultPosition,
                 log=None):
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, wx.ID_ANY,
                             style=wx.LC_REPORT)
        listmix.CheckListCtrlMixin.__init__(self)
        self.log = log

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        # listmix.TextEditMixin.__init__(self)
        
        self.InsertColumn(0, _('Layer'))
        self.InsertColumn(1, _('Layer name'))
        self.InsertColumn(2, _('Output vector map name'))
        
        self.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnPopupMenu) #wxMSW
        self.Bind(wx.EVT_RIGHT_UP,            self.OnPopupMenu) #wxGTK

    def LoadData(self, data=None):
        """Load data into list"""
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
        """Show popup menu"""
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
        """Select all items"""
        item = -1
        
        while True:
            item = self.GetNextItem(item)
            if item == -1:
                break
            self.CheckItem(item, True)

        event.Skip()
        
    def OnSelectNone(self, event):
        """Deselect items"""
        item = -1
        
        while True:
            item = self.GetNextItem(item, wx.LIST_STATE_SELECTED)
            if item == -1:
                break
            self.CheckItem(item, False)

        event.Skip()
        
    def GetLayers(self):
        """Get list of layers (layer name, output name)"""
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
    """Set opacity of map layers"""
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
        """Button 'OK' pressed"""
        # return opacity value
        opacity = float(self.value.GetValue()) / 100
        return opacity


