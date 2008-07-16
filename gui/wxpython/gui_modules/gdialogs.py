"""
@package gdialogs.py

@brief Common dialog used in wxGUI.

List of classes:
 - NewVectorDialog
 - SavedRegion
 - DecorationDialog
 - TextLayerDialog 
 - LoadMapLayersDialog

(C) 2008 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import sys
import re

import wx

import gcmd
import grassenv
import globalvar
import gselect
import menuform
import utils
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

        # TODO remove (see Preferences dialog)
        self.overwrite = wx.CheckBox(parent=self.panel, id=wx.ID_ANY,
                                     label=_("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))

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
        dataSizer.Add(self.overwrite, proportion=0,
                      flag=wx.ALL, border=1)

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

        return (mapName,
                self.overwrite.IsChecked())

def CreateNewVector(parent, title=_('Create new vector map'),
                    exceptMap=None):
    """Create new vector map layer

    @return name of create vector map
    @return None of failure
    """
    dlg = NewVectorDialog(parent=parent, id=wx.ID_ANY, title=title)
    if dlg.ShowModal() == wx.ID_OK:
        outmap, overwrite = dlg.GetName()
        if outmap == exceptMap:
            wx.MessageBox(parent=parent,
                          message=_("Unable to create vector map <%s>.") % outmap,
                          caption=_("Error"),
                          style=wx.ID_OK | wx.ICON_ERROR | wx.CENTRE)
            return False

        if outmap == '': # should not happen
            return False
        
        cmd = ["v.edit",
               "map=%s" % outmap,
               "tool=create"]
                
        if overwrite is True:
            cmd.append('--overwrite')
            
        try:
            p = gcmd.Command(cmd, stderr=None)
        except gcmd.CmdError, e:
            print >> sys.stderr, e
            return None

        if p.returncode == 0:
            # return fully qualified map name
            return outmap + '@' + grassenv.GetGRASSVariable('MAPSET')

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
        # "Use mouse..." (-m) flag causes GUI freeze, trac #119
        try:
            self.parent.MapWindow.overlays[self.ovlId]['cmd'].remove('-m')
        except ValueError:
            pass
        
        self.parent.Map.ChangeOverlay(id=self.ovlId, type=self.name,
                                      command=self.parent.MapWindow.overlays[self.ovlId]['cmd'],
                                      l_active=self.parent.MapWindow.overlays[self.ovlId]['layer'].IsActive(),
                                      l_render=False, l_hidden=True)

        if self.name == 'legend' and \
                params and \
                not self.btnOK.IsEnabled():
            self.btnOK.Enable()
        
        self.SetTitle(_('Legend of raster map <%s>') % \
                      utils.GetLayerNameFromCmd(self.parent.MapWindow.overlays[self.ovlId]['cmd']))
            
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
    
