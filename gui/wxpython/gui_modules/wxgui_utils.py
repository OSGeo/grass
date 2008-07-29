"""
MODULE:     wxgui_utils.py

CLASSES:
    * AbstractLayer
    * Layer
    * LayerTree

PURPOSE:    Utility classes for GRASS wxPython GUI. Main functions include tree control
            for GIS map layer management, command console, and command parsing.

AUTHORS:    The GRASS Development Team
            Michael Barton (Arizona State University)
            Jachym Cepicky (Mendel University of Agriculture)
            Martin Landa <landa.martin gmail.com>

COPYRIGHT:  (C) 2007-2008 by the GRASS Development Team
            This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.
"""

import os
import sys
import string

import wx
import wx.lib.customtreectrl as CT
import wx.combo
import wx.lib.newevent

import globalvar
import menuform
import mapdisp
import render
import gcmd
import grassenv
import histogram
import utils
import profile
from debug import Debug as Debug
from icon import Icons as Icons
from preferences import globalSettings as UserSettings
try:
    import subprocess
except:
    from compat import subprocess

TREE_ITEM_HEIGHT = 25

class LayerTree(CT.CustomTreeCtrl):
    """
    Creates layer tree structure
    """
    #	def __init__(self, parent, id, pos, size, style):
    def __init__(self, parent,
                 id=wx.ID_ANY, pos=wx.DefaultPosition,
                 size=wx.DefaultSize, style=wx.SUNKEN_BORDER,
                 ctstyle=CT.TR_HAS_BUTTONS | CT.TR_HAS_VARIABLE_ROW_HEIGHT |
                 CT.TR_HIDE_ROOT | CT.TR_ROW_LINES | CT.TR_FULL_ROW_HIGHLIGHT |
                 CT.TR_EDIT_LABELS | CT.TR_MULTIPLE,
                 **kargs):
        CT.CustomTreeCtrl.__init__(self, parent, id, pos, size, style, ctstyle)

        ### SetAutoLayout() causes that no vertical scrollbar is displayed
        ### when some layers are not visible in layer tree
        # self.SetAutoLayout(True)
        self.SetGradientStyle(1)
        self.EnableSelectionGradient(True)
        self.SetFirstGradientColour(wx.Colour(100, 100, 100))
        self.SetSecondGradientColour(wx.Colour(150, 150, 150))

        self.Map = render.Map()    # instance of render.Map to be associated with display
        self.root = None           # ID of layer tree root node
        self.groupnode = 0         # index value for layers
        self.optpage = {}          # dictionary of notebook option pages for each map layer
        self.layer_selected = None # ID of currently selected layer
        self.saveitem = {}         # dictionary to preserve layer attributes for drag and drop
        self.first = True          # indicates if a layer is just added or not
        self.drag = False          # flag to indicate a drag event is in process
        self.disp_idx = kargs['idx']
        self.gismgr = kargs['gismgr']
        self.notebook = kargs['notebook'] # Layer Manager notebook for layer tree
        self.treepg = parent              # notebook page holding layer tree
        self.auimgr = kargs['auimgr']     # aui manager

        # init associated map display
        self.mapdisplay = mapdisp.MapFrame(self,
                                           id=wx.ID_ANY, pos=wx.DefaultPosition,
                                           size=globalvar.MAP_WINDOW_SIZE,
                                           style=wx.DEFAULT_FRAME_STYLE,
                                           tree=self, notebook=self.notebook,
                                           gismgr=self.gismgr, page=self.treepg,
                                           Map=self.Map, auimgr=self.auimgr)

        # title
        self.mapdisplay.SetTitle(_("GRASS GIS Map Display: " +
                                   str(self.disp_idx + 1) + 
                                   " - Location: " + grassenv.GetGRASSVariable("LOCATION_NAME")))

        # show new display
        if kargs['showMapDisplay'] is True:
            self.mapdisplay.Show()
            self.mapdisplay.Refresh()
            self.mapdisplay.Update()

        self.root = self.AddRoot(_("Map Layers"))
        self.SetPyData(self.root, (None,None))

        #create image list to use with layer tree
        il = wx.ImageList(16, 16, mask=False)

        trart = wx.ArtProvider.GetBitmap(wx.ART_FOLDER_OPEN, wx.ART_OTHER, (16, 16))
        self.folder_open = il.Add(trart)
        trart = wx.ArtProvider.GetBitmap(wx.ART_FOLDER, wx.ART_OTHER, (16, 16))
        self.folder = il.Add(trart)

        bmpsize = (16, 16)
        trgif = Icons["addrast"].GetBitmap(bmpsize)
        self.rast_icon = il.Add(trgif)

        trgif = Icons["addrgb"].GetBitmap(bmpsize)
        self.rgb_icon = il.Add(trgif)

        trgif = Icons["addhis"].GetBitmap(bmpsize)
        self.his_icon = il.Add(trgif)

        trgif = Icons["addshaded"].GetBitmap(bmpsize)
        self.shaded_icon = il.Add(trgif)

        trgif = Icons["addrarrow"].GetBitmap(bmpsize)
        self.rarrow_icon = il.Add(trgif)

        trgif = Icons["addrnum"].GetBitmap(bmpsize)
        self.rnum_icon = il.Add(trgif)

        trgif = Icons["addvect"].GetBitmap(bmpsize)
        self.vect_icon = il.Add(trgif)

        trgif = Icons["addthematic"].GetBitmap(bmpsize)
        self.theme_icon = il.Add(trgif)

        trgif = Icons["addchart"].GetBitmap(bmpsize)
        self.chart_icon = il.Add(trgif)

        trgif = Icons["addgrid"].GetBitmap(bmpsize)
        self.grid_icon = il.Add(trgif)

        trgif = Icons["addgeodesic"].GetBitmap(bmpsize)
        self.geodesic_icon = il.Add(trgif)

        trgif = Icons["addrhumb"].GetBitmap(bmpsize)
        self.rhumb_icon = il.Add(trgif)

        trgif = Icons["addlabels"].GetBitmap(bmpsize)
        self.labels_icon = il.Add(trgif)

        trgif = Icons["addcmd"].GetBitmap(bmpsize)
        self.cmd_icon = il.Add(trgif)

        self.AssignImageList(il)

        # use when groups implemented
        ## self.tree.SetItemImage(self.root, fldridx, wx.TreeItemIcon_Normal)
        ## self.tree.SetItemImage(self.root, fldropenidx, wx.TreeItemIcon_Expanded)

        self.Bind(wx.EVT_TREE_ITEM_EXPANDING,   self.OnExpandNode)
        self.Bind(wx.EVT_TREE_ITEM_COLLAPSED,   self.OnCollapseNode)
        self.Bind(wx.EVT_TREE_ITEM_ACTIVATED,   self.OnActivateLayer)
        self.Bind(wx.EVT_TREE_SEL_CHANGED,      self.OnChangeSel)
        self.Bind(CT.EVT_TREE_ITEM_CHECKED,     self.OnLayerChecked)
        self.Bind(wx.EVT_TREE_DELETE_ITEM,      self.OnDeleteLayer)
        self.Bind(wx.EVT_TREE_BEGIN_DRAG,       self.OnBeginDrag)
        self.Bind(wx.EVT_TREE_END_DRAG,         self.OnEndDrag)
        self.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, self.OnLayerContextMenu)
        self.Bind(wx.EVT_TREE_END_LABEL_EDIT,   self.OnChangeLayerName)
        self.Bind(wx.EVT_KEY_UP,                self.OnKeyUp)
        # self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

    def OnKeyUp(self, event):
        """Key pressed"""
        key = event.GetKeyCode()
        if key == wx.WXK_DELETE and self.gismgr:
            self.gismgr.OnDeleteLayer(None)

        event.Skip()

    def OnChangeLayerName (self, event):
        """Change layer name"""
        Debug.msg (3, "LayerTree.OnChangeLayerName: name=%s" % event.GetLabel())

    def OnLayerContextMenu (self, event):
        """Contextual menu for item/layer"""
        if not self.layer_selected:
            event.Skip()
            return

        ltype = self.GetPyData(self.layer_selected)[0]['type']

        Debug.msg (4, "LayerTree.OnContextMenu: layertype=%s" % \
                       ltype)

        ## pos = event.GetPosition()
        ## pos = self.ScreenToClient(pos)

        if not hasattr (self, "popupID1"):
            self.popupID1 = wx.NewId()
            self.popupID2 = wx.NewId()
            self.popupID3 = wx.NewId()
            self.popupID4 = wx.NewId()
            self.popupID5 = wx.NewId()
            self.popupID6 = wx.NewId()
            self.popupID7 = wx.NewId()
            self.popupID8 = wx.NewId()
            self.popupID9 = wx.NewId()
            self.popupID10 = wx.NewId()
            self.popupID11 = wx.NewId() # nviz
            self.popupID12 = wx.NewId()
            self.popupID13 = wx.NewId()
            
        self.popupMenu = wx.Menu()
        # general item
        self.popupMenu.Append(self.popupID1, text=_("Remove"))
        self.Bind(wx.EVT_MENU, self.gismgr.OnDeleteLayer, id=self.popupID1)

        if ltype != "command": # rename
            self.popupMenu.Append(self.popupID2, text=_("Rename"))
            self.Bind(wx.EVT_MENU, self.RenameLayer, id=self.popupID2)

        # map layer items
        if ltype != "group" and \
                ltype != "command": # properties
            self.popupMenu.AppendSeparator()
            self.popupMenu.Append(self.popupID8, text=_("Change opacity level"), kind=wx.ITEM_CHECK)
            if self.FindWindowById(self.GetPyData(self.layer_selected)[0]['ctrl']).GetName() == 'spinCtrl':
                checked = True
            else:
                checked = False
            self.popupMenu.Check(self.popupID8, checked)
            self.Bind(wx.EVT_MENU, self.OnPopupOpacityLevel, id=self.popupID8)
            self.popupMenu.Append(self.popupID3, text=_("Properties"))
            self.Bind(wx.EVT_MENU, self.OnPopupProperties, id=self.popupID3)
            self.popupMenu.Append(self.popupID9, text=_("Zoom to selected map"))
            self.Bind(wx.EVT_MENU, self.mapdisplay.MapWindow.OnZoomToMap, id=self.popupID9)
            self.popupMenu.Append(self.popupID10, text=_("Set computational region from selected map"))
            self.Bind(wx.EVT_MENU, self.OnSetCompRegFromMap, id=self.popupID10)

        # specific items
        try:
            mltype = self.GetPyData(self.layer_selected)[0]['type']
        except:
            mltype = None
        #
        # vector layers (specific items)
        #
        if mltype and mltype == "vector":
            self.popupMenu.AppendSeparator()
            self.popupMenu.Append(self.popupID4, text=_("Show attribute data"))
            self.Bind (wx.EVT_MENU, self.gismgr.OnShowAttributeTable, id=self.popupID4)

            self.popupMenu.Append(self.popupID5, text=_("Start editing"))
            self.popupMenu.Append(self.popupID6, text=_("Stop editing"))
            self.popupMenu.Enable(self.popupID6, False)
            self.Bind (wx.EVT_MENU, self.OnStartEditing, id=self.popupID5)
            self.Bind (wx.EVT_MENU, self.OnStopEditing,  id=self.popupID6)

            layer = self.GetPyData(self.layer_selected)[0]['maplayer']
            # enable editing only for vector map layers available in the current mapset
            digit = self.mapdisplay.toolbars['vdigit']
            if layer.GetMapset() != grassenv.GetGRASSVariable("MAPSET"):
                # only vector map in current mapset can be edited
                self.popupMenu.Enable (self.popupID5, False)
                self.popupMenu.Enable (self.popupID6, False)
            elif digit and digit.layerSelectedID != None:
                # vector map already edited
                if digit.layers[digit.layerSelectedID] == layer:
                    self.popupMenu.Enable (self.popupID5, False)
                    self.popupMenu.Enable(self.popupID6, True)
                    self.popupMenu.Enable(self.popupID1, False)
                else:
                    self.popupMenu.Enable(self.popupID5, False)
                    self.popupMenu.Enable(self.popupID6, False)
            self.popupMenu.Append(self.popupID7, _("Metadata"))
            self.Bind (wx.EVT_MENU, self.OnMetadata, id=self.popupID7)

        #
        # raster layers (specific items)
        #
        elif mltype and mltype == "raster":
            self.popupMenu.Append(self.popupID12, text=_("Zoom to selected map (ignore NULLs)"))
            self.Bind(wx.EVT_MENU, self.mapdisplay.MapWindow.OnZoomToRaster, id=self.popupID12)
            self.popupMenu.Append(self.popupID13, text=_("Set computational region from selected map (ignore NULLs)"))
            self.Bind(wx.EVT_MENU, self.OnSetCompRegFromRaster, id=self.popupID13)
            self.popupMenu.AppendSeparator()
            self.popupMenu.Append(self.popupID4, _("Histogram"))
            self.Bind (wx.EVT_MENU, self.OnHistogram, id=self.popupID4)
            self.popupMenu.Append(self.popupID5, _("Profile"))
            self.Bind (wx.EVT_MENU, self.OnProfile, id=self.popupID5)
            self.popupMenu.Append(self.popupID6, _("Metadata"))
            self.Bind (wx.EVT_MENU, self.OnMetadata, id=self.popupID6)
            if self.mapdisplay.toolbars['nviz']:
                self.popupMenu.Append(self.popupID11, _("Nviz properties"))
                self.Bind (wx.EVT_MENU, self.OnNvizProperties, id=self.popupID11)

        ## self.PopupMenu(self.popupMenu, pos)
        self.PopupMenu(self.popupMenu)
        self.popupMenu.Destroy()

    def OnMetadata(self, event):
        """Print metadata of raster/vector map layer
        TODO: Dialog to modify metadata
        """
        mapLayer = self.GetPyData(self.layer_selected)[0]['maplayer']
        mltype = self.GetPyData(self.layer_selected)[0]['type']

        if mltype == 'raster':
            cmd = ['r.info']
        elif mltype == 'vector':
            cmd = ['v.info']
        cmd.append('map=%s' % mapLayer.name)

        # print output to command log area
        self.gismgr.goutput.RunCmd(cmd)

    def OnSetCompRegFromRaster(self, event):
        """Set computational region from selected raster map (ignore NULLs)"""
        mapLayer = self.GetPyData(self.layer_selected)[0]['maplayer']

        cmd = ['g.region',
               '-p',
               'zoom=%s' % mapLayer.name]
        
        # print output to command log area
        self.gismgr.goutput.RunCmd(cmd)
         
    def OnSetCompRegFromMap(self, event):
        """Set computational region from selected raster/vector map"""
        mapLayer = self.GetPyData(self.layer_selected)[0]['maplayer']
        mltype = self.GetPyData(self.layer_selected)[0]['type']

        cmd = ['g.region',
               '-p'] # print by default

        # TODO: other elements
        if mltype == 'raster':
            cmd.append('rast=%s' % mapLayer.name)
        elif mltype == 'vector':
            cmd.append('vect=%s' % mapLayer.name)

        # print output to command log area
        self.gismgr.goutput.RunCmd(cmd)
        
    def OnProfile(self, event):
        """Plot profile of given raster map layer"""
        mapLayer = self.GetPyData(self.layer_selected)[0]['maplayer']
        if not mapLayer.name:
            wx.MessageBox(parent=self,
                          message=_("Unable to create profile of "
                                    "raster map."),
                          caption=_("Error"), style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return False

        if not hasattr (self, "profileFrame"):
            self.profileFrame = None

        if hasattr (self.mapdisplay, "profile") and self.mapdisplay.profile:
            self.profileFrame = self.mapdisplay.profile

        if not self.profileFrame:
            self.profileFrame = profile.ProfileFrame(self.mapdisplay,
                                                     id=wx.ID_ANY, pos=wx.DefaultPosition, size=(700,300),
                                                     style=wx.DEFAULT_FRAME_STYLE, rasterList=[mapLayer.name])
            # show new display
            self.profileFrame.Show()
        
    def OnHistogram(self, event):
        """
        Plot histogram for given raster map layer
        """
        mapLayer = self.GetPyData(self.layer_selected)[0]['maplayer']
        if not mapLayer.name:
            wx.MessageBox(parent=self,
                          message=_("Unable to display histogram of "
                                    "raster map."),
                          caption=_("Error"), style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return False

        if not hasattr (self, "histogramFrame"):
            self.histogramFrame = None

        if hasattr (self.mapdisplay, "histogram") and self.mapdisplay.histogram:
            self.histogramFrame = self.mapdisplay.histogram

        if not self.histogramFrame:
            self.histogramFrame = histogram.HistFrame(self,
                                                      id=wx.ID_ANY,
                                                      pos=wx.DefaultPosition, size=globalvar.HIST_WINDOW_SIZE,
                                                      style=wx.DEFAULT_FRAME_STYLE)
            # show new display
            self.histogramFrame.Show()

        self.histogramFrame.SetHistLayer(['d.histogram', 'map=%s' % mapLayer.name])
        self.histogramFrame.HistWindow.UpdateHist()
        self.histogramFrame.Refresh()
        self.histogramFrame.Update()

        return True

    def OnStartEditing (self, event):
        """
        Start editing vector map layer requested by the user
        """
        try:
            maplayer = self.GetPyData(self.layer_selected)[0]['maplayer']
        except:
            event.Skip()
            return

        if not self.mapdisplay.toolbars['vdigit']: # enable tool
            self.mapdisplay.AddToolbar('vdigit')
        else: # tool already enabled
            pass

        # mark layer as 'edited'
        self.mapdisplay.toolbars['vdigit'].StartEditing (maplayer)

    def OnStopEditing (self, event):
        """
        Stop editing the current vector map layer
        """
        try:
            maplayer = self.GetPyData(self.layer_selected)[0]['maplayer']
        except:
            event.Skip()
            return

        self.mapdisplay.toolbars['vdigit'].OnExit()
        self.mapdisplay.imgVectorMap = None

    def OnPopupProperties (self, event):
        """Popup properties dialog"""
        self.PropertiesDialog(self.layer_selected)

    def OnPopupOpacityLevel(self, event):
        """Popup opacity level indicator"""
        if not self.GetPyData(self.layer_selected)[0]['ctrl']:
            return

        win = self.FindWindowById(self.GetPyData(self.layer_selected)[0]['ctrl'])
        type = win.GetName()

        self.layer_selected.DeleteWindow()

        opacity = self.GetPyData(self.layer_selected)[0]['maplayer'].GetOpacity()
        if type == 'staticText':
            ctrl = wx.SpinCtrl(self, id=wx.ID_ANY, value="",
                               style=wx.SP_ARROW_KEYS, initial=100, min=0, max=100,
                               name='spinCtrl')
            ctrl.SetValue(opacity)
            self.Bind(wx.EVT_SPINCTRL, self.OnOpacity, ctrl)
        else:
            ctrl = wx.StaticText(self, id=wx.ID_ANY,
                                 name='staticText')
            if opacity < 100:
                ctrl.SetLabel('   (' + str(opacity) + '%)')
                
        self.GetPyData(self.layer_selected)[0]['ctrl'] = ctrl.GetId()
        self.layer_selected.SetWindow(ctrl)

        self.RefreshSelected()
        self.Refresh()

    def OnNvizProperties(self, event):
        """Nviz-related properties (raster/vector/volume)

        @todo vector/volume
        """
        if not self.mapdisplay.nvizToolWin.IsShown():
            self.mapdisplay.nvizToolWin.Show()

        self.mapdisplay.nvizToolWin.SetPage('surface')

    def RenameLayer (self, event):
        """Rename layer"""
        self.EditLabel(self.layer_selected)

    def AddLayer(self, ltype, lname=None, lchecked=None,
                 lopacity=None, lcmd=None, lgroup=None, lnviz=None):
        """Add new item to the layer tree, create corresponding MapLayer instance.
        Launch property dialog if needed (raster, vector, etc.)

        @param ltyle layer type (raster, vector, ...)
        @param lname layer name
        @param lchecked if True layer is checked
        @param lopacity layer opacity level
        @param lcmd command (given as a list)
        @param lgroup group name or None
        @param lnviz layer Nviz properties
        """

        self.first = True
        params = {} # no initial options parameters

        # deselect active item
        if self.layer_selected:
            self.SelectItem(self.layer_selected, select=False)

        Debug.msg (3, "LayerTree().AddLayer(): ltype=%s" % (ltype))
        
        if ltype == 'command':
            # generic command item
            ctrl = wx.TextCtrl(self, id=wx.ID_ANY, value='',
                               pos=wx.DefaultPosition, size=(250,25),
                               style=wx.TE_MULTILINE|wx.TE_WORDWRAP)
            ctrl.Bind(wx.EVT_TEXT_ENTER, self.OnCmdChanged)
            ctrl.Bind(wx.EVT_TEXT,       self.OnCmdChanged)
        elif ltype == 'group':
            # group item
            ctrl = None
            grouptext = _('Layer group:') + str(self.groupnode)
            self.groupnode += 1
        else:
            # all other items (raster, vector, ...)
            if UserSettings.Get(group='manager', key='changeOpacityLevel', subkey='enabled'):
                ctrl = wx.SpinCtrl(self, id=wx.ID_ANY, value="",
                                   style=wx.SP_ARROW_KEYS, initial=100, min=0, max=100,
                                   name='spinCtrl')
                
                self.Bind(wx.EVT_SPINCTRL, self.OnOpacity, ctrl)
            else:
                ctrl = wx.StaticText(self, id=wx.ID_ANY,
                                     name='staticText')
        # add layer to the layer tree
        if self.layer_selected and self.layer_selected != self.GetRootItem():
            if self.GetPyData(self.layer_selected)[0]['type'] != 'group':
                if lgroup is False:
                    # last child of root
                    layer = self.AppendItem(parentId=self.root,
                                            text='', ct_type=1, wnd=ctrl)
                elif lgroup is None or lgroup is True:
                    # insert item as last child
                    parent = self.GetItemParent(self.layer_selected)
                    # layer = self.InsertItem(parentId=parent, input=self.GetPrevSibling(self.layer_selected),
                    #                        text='', ct_type=1, wnd=ctrl)
                    layer = self.AppendItem(parentId=parent,
                                            text='', ct_type=1, wnd=ctrl)

            else: # group (first child of self.layer_selected)
                layer = self.PrependItem(parent=self.layer_selected,
                                         text='', ct_type=1, wnd=ctrl)
                self.Expand(self.layer_selected)
        else: # add first layer to the layer tree (first child of root)
            layer = self.PrependItem(parent=self.root, text='', ct_type=1, wnd=ctrl)

        # layer is initially unchecked as inactive (beside 'command')
        # use predefined value if given
        if lchecked is not None:
            checked = lchecked
        else:
            checked = True

        self.CheckItem(layer, checked=checked)

        # select new item
        self.SelectItem(layer, select=True)
        self.layer_selected = layer

        # add text and icons for each layer ltype
        if ltype == 'raster':
            self.SetItemImage(layer, self.rast_icon)
            self.SetItemText(layer, '%s %s' % (_('raster'), _('(double click to set properties)')))
        elif ltype == 'rgb':
            self.SetItemImage(layer, self.rgb_icon)
            self.SetItemText(layer, '%s %s' % (_('RGB'), _('(double click to set properties)')))
        elif ltype == 'his':
            self.SetItemImage(layer, self.his_icon)
            self.SetItemText(layer, '%s %s' % (_('HIS'), _('(double click to set properties)')))
        elif ltype == 'shaded':
            self.SetItemImage(layer, self.shaded_icon)
            self.SetItemText(layer, '%s %s' % (_('Shaded relief'), _('(double click to set properties)')))
        elif ltype == 'rastnum':
            self.SetItemImage(layer, self.rnum_icon)
            self.SetItemText(layer, '%s %s' % (_('raster cell numbers'), _('(double click to set properties)')))
        elif ltype == 'rastarrow':
            self.SetItemImage(layer, self.rarrow_icon)
            self.SetItemText(layer, '%s %s' % (_('raster flow arrows'), _('(double click to set properties)')))
        elif ltype == 'vector':
            self.SetItemImage(layer, self.vect_icon)
            self.SetItemText(layer, '%s %s' % (_('vector'), _('(double click to set properties)')))
        elif ltype == 'thememap':
            self.SetItemImage(layer, self.theme_icon)
            self.SetItemText(layer, '%s %s' % (_('thematic map'), _('(double click to set properties)')))
        elif ltype == 'themechart':
            self.SetItemImage(layer, self.chart_icon)
            self.SetItemText(layer, '%s %s' % (_('thematic charts'), _('(double click to set properties)')))
        elif ltype == 'grid':
            self.SetItemImage(layer, self.grid_icon)
            self.SetItemText(layer, '%s %s' % (_('grid'), _('(double click to set properties)')))
        elif ltype == 'geodesic':
            self.SetItemImage(layer, self.geodesic_icon)
            self.SetItemText(layer, '%s %s' % (_('geodesic line'), _('(double click to set properties)')))
        elif ltype == 'rhumb':
            self.SetItemImage(layer, self.rhumb_icon)
            self.SetItemText(layer, '%s %s' % (_('rhumbline'), _('(double click to set properties)')))
        elif ltype == 'labels':
            self.SetItemImage(layer, self.labels_icon)
            self.SetItemText(layer, '%s %s' % (_('vector labels'), _('(double click to set properties)')))
        elif ltype == 'command':
            self.SetItemImage(layer, self.cmd_icon)
        elif ltype == 'group':
            self.SetItemImage(layer, self.folder)
            self.SetItemText(layer, grouptext)

        self.first = False

        if ltype != 'group':
            if lopacity:
                opacity = lopacity
                if UserSettings.Get(group='manager', key='changeOpacityLevel', subkey='enabled'):
                    ctrl.SetValue(int(lopacity * 100))
                else:
                    if opacity < 1.0:
                        ctrl.SetLabel('   (' + str(int(opacity * 100)) + '%)')
            else:
                opacity = 1.0
            if lcmd and len(lcmd) > 1:
                cmd = lcmd
                render = False
                name = utils.GetLayerNameFromCmd(lcmd)
            else:
                cmd = []
                render = False
                name = None

            if ctrl:
                ctrlId = ctrl.GetId()
            else:
                ctrlId = None
                
            # add a data object to hold the layer's command (does not apply to generic command layers)
            self.SetPyData(layer, ({'cmd': cmd,
                                    'type' : ltype,
                                    'ctrl' : ctrlId,
                                    'maplayer' : None,
                                    'nviz' : lnviz,
                                    'prowin' : None}, 
                                   None))

            maplayer = self.Map.AddLayer(type=ltype, command=self.GetPyData(layer)[0]['cmd'], name=name,
                                         l_active=checked, l_hidden=False,
                                         l_opacity=opacity, l_render=render)
            self.GetPyData(layer)[0]['maplayer'] = maplayer

            # run properties dialog if no properties given
            if len(cmd) == 0:
                self.PropertiesDialog(layer, show=True)

        else: # group
            self.SetPyData(layer, ({'cmd': None,
                                    'type' : ltype,
                                    'ctrl' : None,
                                    'maplayer' : None,
                                    'prowin' : None}, 
                                   None))

        # use predefined layer name if given
        if lname:
            if ltype != 'command':
                self.SetItemText(layer, lname)
            else:
                ctrl.SetValue(lname)

        # updated progress bar range (mapwindow statusbar)
        if checked is True:
            self.mapdisplay.onRenderGauge.SetRange(len(self.Map.GetListOfLayers(l_active=True)))

        # layer.SetHeight(TREE_ITEM_HEIGHT)

        return layer

    def PropertiesDialog (self, layer, show=True):
        """Launch the properties dialog"""
        if self.GetPyData(layer)[0].has_key('propwin') and \
                self.GetPyData(layer)[0]['propwin'] is not None:
            # recycle GUI dialogs
            if self.GetPyData(layer)[0]['propwin'].IsShown():
                self.GetPyData(layer)[0]['propwin'].SetFocus()
            else:
                self.GetPyData(layer)[0]['propwin'].Show()
            return
        
        completed = ''
        params = self.GetPyData(layer)[1]
        ltype  = self.GetPyData(layer)[0]['type']
                
        Debug.msg (3, "LayerTree.PropertiesDialog(): ltype=%s" % \
                   ltype)

        if self.GetPyData(layer)[0]['cmd']:
            cmdValidated = menuform.GUI().ParseCommand(self.GetPyData(layer)[0]['cmd'],
                                                       completed=(self.GetOptData,layer,params),
                                                       parentframe=self, show=show)
            self.GetPyData(layer)[0]['cmd'] = cmdValidated
        elif ltype == 'raster':
            cmd = ['d.rast']
            if UserSettings.Get(group='cmd', key='rasterOverlay', subkey='enabled'):
                cmd.append('-o')
            menuform.GUI().ParseCommand(cmd, completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'rgb':
            menuform.GUI().ParseCommand(['d.rgb'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'his':
            menuform.GUI().ParseCommand(['d.his'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'shaded':
            menuform.GUI().ParseCommand(['d.shadedmap'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'rastarrow':
            menuform.GUI().ParseCommand(['d.rast.arrow'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'rastnum':
            menuform.GUI().ParseCommand(['d.rast.num'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'vector':
            menuform.GUI().ParseCommand(['d.vect'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'thememap':
            # -s flag requested, otherwise only first thematic category is displayed
            # should be fixed by C-based d.thematic.* modules
            menuform.GUI().ParseCommand(['d.vect.thematic', '-s'], 
                                        completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'themechart':
            menuform.GUI().ParseCommand(['d.vect.chart'],
                                        completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'grid':
            menuform.GUI().ParseCommand(['d.grid'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'geodesic':
            menuform.GUI().ParseCommand(['d.geodesic'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'rhumb':
            menuform.GUI().ParseCommand(['d.rhumbline'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'labels':
            menuform.GUI().ParseCommand(['d.labels'], completed=(self.GetOptData,layer,params),
                                        parentframe=self)
        elif ltype == 'cmdlayer':
            pass
        elif ltype == 'group':
            pass

    def OnActivateLayer(self, event):
        """Click on the layer item.
        Launch property dialog, or expand/collapse group of items, etc."""
        
        layer = event.GetItem()
        self.layer_selected = layer

        self.PropertiesDialog (layer)

        if self.GetPyData(layer)[0]['type'] == 'group':
            if self.IsExpanded(layer):
                self.Collapse(layer)
            else:
                self.Expand(layer)

    def OnDeleteLayer(self, event):
        """Remove selected layer item from the layer tree"""

        item = event.GetItem()

        try:
            item.properties.Close(True)
        except:
            pass

        if item != self.root:
            Debug.msg (3, "LayerTree.OnDeleteLayer(): name=%s" % \
                           (self.GetItemText(item)))
        else:
            self.root = None

        # unselect item
        self.Unselect()
        self.layer_selected = None

        try:
            if self.GetPyData(item)[0]['type'] != 'group':
                self.Map.DeleteLayer( self.GetPyData(item)[0]['maplayer'])
        except:
            pass

        # redraw map if auto-rendering is enabled
        if self.mapdisplay.autoRender.GetValue(): 
            self.mapdisplay.OnRender(None)

        if self.mapdisplay.toolbars['vdigit']:
            self.mapdisplay.toolbars['vdigit'].UpdateListOfLayers (updateTool=True)

        # update progress bar range (mapwindow statusbar)
        self.mapdisplay.onRenderGauge.SetRange(len(self.Map.GetListOfLayers(l_active=True)))

        event.Skip()

    def OnLayerChecked(self, event):
        """Enable/disable data layer"""
        item    = event.GetItem()
        checked = item.IsChecked()
        
        if self.drag == False and self.first == False:
            # change active parameter for item in layers list in render.Map
            if self.GetPyData(item)[0]['type'] == 'group':
                child, cookie = self.GetFirstChild(item)
                while child:
                    self.CheckItem(child, checked)
                    self.Map.ChangeLayerActive(self.GetPyData(child)[0]['maplayer'], checked)
                    child = self.GetNextSibling(child)
            else:
                self.Map.ChangeLayerActive(self.GetPyData(item)[0]['maplayer'], checked)

        # update progress bar range (mapwindow statusbar)
        self.mapdisplay.onRenderGauge.SetRange(len(self.Map.GetListOfLayers(l_active=True)))

        if self.mapdisplay.toolbars['nviz'] and \
                self.GetPyData(item) is not None:
            # nviz - load/unload data layer
            mapLayer = self.GetPyData(item)[0]['maplayer']

            self.mapdisplay.SetStatusText(_("Please wait, updating data..."), 0)

            if checked: # enable
                if mapLayer.type == 'raster':
                    self.mapdisplay.MapWindow.LoadRaster(item)
                elif mapLayer.type == 'vector':
                    self.mapdisplay.MapWindow.LoadVector(item)

            else: # disable
                data = self.GetPyData(item)[0]['nviz']

                if mapLayer.type == 'raster':
                    self.mapdisplay.MapWindow.UnloadRaster(item)
                elif mapLayer.type == 'vector':
                    self.mapdisplay.MapWindow.UnloadVector(item)
                    
                    if hasattr(self.mapdisplay, "nvizToolWin"):
                        toolWin = self.mapdisplay.nvizToolWin
                        # remove vector page
                        if toolWin.notebook.GetSelection() == toolWin.page['vector']['id']:
                            toolWin.notebook.RemovePage(toolWin.page['vector']['id'])
                            toolWin.page['vector']['id'] = -1
                            toolWin.page['settings']['id'] = 1

            self.mapdisplay.SetStatusText("", 0)

        # redraw map if auto-rendering is enabled
        if self.mapdisplay.autoRender.GetValue(): 
            self.mapdisplay.OnRender(None)

    def OnCmdChanged(self, event):
        """Change command string"""
        ctrl = event.GetEventObject().GetId()
        cmd = event.GetString()
        layer = None

        layer = self.GetFirstVisibleItem()

        while layer and layer.IsOk():
            if self.GetPyData(layer)[0]['ctrl'] == ctrl:
                break
            
            layer = self.GetNextVisible(layer)

        # change parameters for item in layers list in render.Map
        if layer and self.drag == False:
            self.ChangeLayer(layer)
            self.GetPyData(layer)[0]['cmd'] = cmd.split(' ')
            maplayer = self.GetPyData(layer)[0]['maplayer']
            for option in maplayer.GetCmd():
                if 'map=' in option:
                    mapname = option.split('=')[1]
                    self.Map.ChangeLayerName(maplayer, mapname)

        event.Skip()

    def OnOpacity(self, event):
        """
        Set opacity level for map layer
        """
        Debug.msg (3, "LayerTree.OnOpacity(): %s" % event.GetInt())

        ctrl = event.GetEventObject().GetId()
        maplayer = None

        vislayer = self.GetFirstVisibleItem()

        layer = None
        for item in range(0, self.GetCount()):
            if self.GetPyData(vislayer)[0]['ctrl'] == ctrl:
                layer = vislayer

            if not self.GetNextVisible(vislayer):
                break
            else:
                vislayer = self.GetNextVisible(vislayer)

        if layer:
            maplayer = self.GetPyData(layer)[0]['maplayer']

        opacity = event.GetInt() / 100.
        # change opacity parameter for item in layers list in render.Map
        if maplayer and self.drag == False:
            self.Map.ChangeOpacity(maplayer, opacity)

        # redraw map if auto-rendering is enabled
        if self.mapdisplay.autoRender.GetValue(): 
            self.mapdisplay.OnRender(None)

    def OnChangeSel(self, event):
        """Selection changed"""
        oldlayer = event.GetOldItem()
        layer = event.GetItem()
        self.layer_selected = layer
        
        try:
            self.RefreshLine(oldlayer)
            self.RefreshLine(layer)
        except:
            pass

        # update statusbar -> show command string
        if self.GetPyData(layer) and self.GetPyData(layer)[0]['maplayer']:
            cmd = self.GetPyData(layer)[0]['maplayer'].GetCmd(string=True)
            if len(cmd) > 0:
                self.gismgr.SetStatusText(cmd)
        
        # update nviz tools
        if self.mapdisplay.toolbars['nviz'] and \
                self.GetPyData(self.layer_selected) is not None:

            if self.layer_selected.IsChecked():
                # update Nviz tool window
                type = self.GetPyData(self.layer_selected)[0]['maplayer'].type

                if type == 'raster':
                    self.mapdisplay.nvizToolWin.UpdatePage('surface')
                    self.mapdisplay.nvizToolWin.SetPage('surface')
                elif type == 'vector':
                    self.mapdisplay.nvizToolWin.UpdatePage('vector')
                    self.mapdisplay.nvizToolWin.SetPage('vector')
            else:
                for page in ('surface', 'vector'):
                    pageId = self.mapdisplay.nvizToolWin.page[page]['id']
                    if pageId > -1:
                        self.mapdisplay.nvizToolWin.notebook.RemovePage(pageId)
                        self.mapdisplay.nvizToolWin.page[page]['id'] = -1
                        self.mapdisplay.nvizToolWin.page['settings']['id'] = 1 

    def OnCollapseNode(self, event):
        """
        Collapse node
        """
        if self.GetPyData(self.layer_selected)[0]['type'] == 'group':
            self.SetItemImage(self.layer_selected, self.folder)

    def OnExpandNode(self, event):
        """
        Expand node
        """
        self.layer_selected = event.GetItem()
        if self.GetPyData(self.layer_selected)[0]['type'] == 'group':
            self.SetItemImage(self.layer_selected, self.folder_open)

    def OnBeginDrag(self, event):
        """
        Drag and drop of tree nodes
        """

        item  = event.GetItem()
        Debug.msg (3, "LayerTree.OnBeginDrag(): layer=%s" % \
                   (self.GetItemText(item)))

        event.Allow()
        self.drag = True
        self.DoSelectItem(item, unselect_others=True)

        # save everthing associated with item to drag
        self.dragItem = item

    def RecreateItem (self, event, oldItem, parent=None):
        """
        Recreate item (needed for OnEndDrag())
        """
        Debug.msg (4, "LayerTree.RecreateItem(): layer=%s" % \
                   self.GetItemText(oldItem))

        # fetch data (olditem)
        text    = self.GetItemText(oldItem)
        image   = self.GetItemImage(oldItem, 0)
        if self.GetPyData(oldItem)[0]['ctrl']:
            oldctrl = self.FindWindowById(self.GetPyData(oldItem)[0]['ctrl'])
        else:
            oldctrl = None
        checked = self.IsItemChecked(oldItem)
        
        # recreate spin/text control for layer
        if self.GetPyData(oldItem)[0]['type'] == 'command':
            newctrl = wx.TextCtrl(self, id=wx.ID_ANY, value='',
                                  pos=wx.DefaultPosition, size=(250,25),
                                  style=wx.TE_MULTILINE|wx.TE_WORDWRAP)
            try:
                newctrl.SetValue(self.GetPyData(oldItem)[0]['maplayer'].GetCmd(string=True))
            except:
                pass
            newctrl.Bind(wx.EVT_TEXT_ENTER, self.OnCmdChanged)
            newctrl.Bind(wx.EVT_TEXT,       self.OnCmdChanged)
        elif self.GetPyData(oldItem)[0]['type'] == 'group' or oldctrl is None:
            newctrl = None
        else:
            opacity = self.GetPyData(oldItem)[0]['maplayer'].GetOpacity()
            if oldctrl.GetName() == 'staticText':
                newctrl = wx.StaticText(self, id=wx.ID_ANY,
                                        name='staticText')
                if opacity < 100:
                    newctrl.SetLabel('   (' + str(opacity) + '%)')
            else:
                newctrl = wx.SpinCtrl(self, id=wx.ID_ANY, value="",
                                      style=wx.SP_ARROW_KEYS, min=0, max=100,
                                      name='spinCtrl')
                newctrl.SetValue(opacity)
                self.Bind(wx.EVT_SPINCTRL, self.OnOpacity, newctrl)

        # decide where to put new layer and put it there
        if not parent:
            flag = self.HitTest(event.GetPoint())[1]
        else:
            flag = 0

        if self.GetPyData(oldItem)[0]['type'] == 'group':
            windval = None
            data    = None
        else:
            windval = self.GetPyData(self.layer_selected)[0]['maplayer'].GetOpacity()
            data    = self.GetPyData(oldItem)

        # create GenericTreeItem instance
        if flag & wx.TREE_HITTEST_ABOVE:
            newItem = self.PrependItem(self.root, text=text, \
                                   ct_type=1, wnd=newctrl, image=image, \
                                   data=data)
        elif (flag &  wx.TREE_HITTEST_BELOW) or (flag & wx.TREE_HITTEST_NOWHERE) \
                 or (flag & wx.TREE_HITTEST_TOLEFT) or (flag & wx.TREE_HITTEST_TORIGHT):
            newItem = self.AppendItem(self.root, text=text, \
                                  ct_type=1, wnd=newctrl, image=image, \
                                  data=data)
        else:
            if parent:
                afteritem = parent
            else:
                afteritem = event.GetItem()

            if  self.GetPyData(afteritem)[0]['type'] == 'group':
                parent = afteritem
                newItem = self.AppendItem(parent, text=text, \
                                      ct_type=1, wnd=newctrl, image=image, \
                                      data=data)
                self.Expand(afteritem)
            else:
                parent = self.GetItemParent(afteritem)
                newItem = self.InsertItem(parent, afteritem, text=text, \
                                      ct_type=1, wnd=newctrl, image=image, \
                                      data=data)

        # add layer at new position
        self.SetPyData(newItem, self.GetPyData(oldItem))
        if newctrl:
            self.GetPyData(newItem)[0]['ctrl'] = newctrl.GetId()
        else:
            self.GetPyData(newItem)[0]['ctrl'] = None
            
        self.CheckItem(newItem, checked=checked)

        # newItem.SetHeight(TREE_ITEM_HEIGHT)

        return newItem

    def OnEndDrag(self, event):
        """
        Insert copy of layer in new
        position and delete original at old position
        """

        self.drag = True
        try:
            old = self.dragItem  # make sure this member exists
        except:
            return

        Debug.msg (4, "LayerTree.OnEndDrag(): layer=%s" % \
                   (self.GetItemText(self.dragItem)))

        newItem  = self.RecreateItem (event, self.dragItem)

        if  self.GetPyData(newItem)[0]['type'] == 'group':
            (child, cookie) = self.GetFirstChild(self.dragItem)
            if child:
                while child:
                    self.RecreateItem(event, child, parent=newItem)
                    self.Delete(child)
                    child = self.GetNextChild(old, cookie)[0]

            self.Expand(newItem)

        # delete layer at original position
        try:
            self.Delete(old) # entry in render.Map layers list automatically deleted by OnDeleteLayer handler
        except AttributeError:
            # FIXME being ugly (item.SetWindow(None))
            pass

        # reorder layers in render.Map to match new order after drag and drop
        self.ReorderLayers()

        # select new item
        self.SelectItem(newItem)

        # completed drag and drop
        self.drag = False

    def GetOptData(self, dcmd, layer, params, propwin):
        """Process layer data"""

        # set layer text to map name
        if dcmd:
            mapname = utils.GetLayerNameFromCmd(dcmd, fullyQualified=True)
            self.SetItemText(layer, mapname)

        # update layer data
        if params:
            self.SetPyData(layer, (self.GetPyData(layer)[0], params))
        if dcmd:
            self.GetPyData(layer)[0]['cmd'] = dcmd
        self.GetPyData(layer)[0]['propwin'] = propwin
        
        # check layer as active
        # self.CheckItem(layer, checked=True)

        # change parameters for item in layers list in render.Map
        self.ChangeLayer(layer)
        
        if self.mapdisplay.toolbars['nviz']:
            # update nviz session
            mapLayer = self.GetPyData(layer)[0]['maplayer']
            mapWin = self.mapdisplay.MapWindow
            if len(mapLayer.GetCmd()) > 0:
                id = -1
                if mapLayer.type == 'raster':
                    if not mapWin.IsLoaded(layer):
                        mapWin.LoadRaster(layer)

                elif mapLayer.type == 'vector':
                    if not mapWin.IsLoaded(layer):
                        id = mapWin.LoadVector(mapLayer)
                        if id > 0:
                            self.mapdisplay.MapWindow.SetLayerData(layer, id)
                            self.mapdisplay.MapWindow.UpdateLayerProperties(layer)
                            
                        self.mapdisplay.nvizToolWin.UpdatePage('vector')
                        self.mapdisplay.nvizToolWin.SetPage('vector')

                # reset view when first layer loaded
                nlayers = len(mapWin.Map.GetListOfLayers(l_type=('raster', 'vector'),
                                                         l_active=True))
                if nlayers < 2:
                    mapWin.ResetView()
        
    def ReorderLayers(self):
        """Add commands from data associated with
        any valid layers (checked or not) to layer list in order to
        match layers in layer tree."""

        # make a list of visible layers
        treelayers = []

        vislayer = self.GetFirstVisibleItem()

        if not vislayer:
            return

        itemList = ""

        for item in range(0, self.GetCount()):
            itemList += self.GetItemText(vislayer) + ','
            if self.GetPyData(vislayer)[0]['type'] != 'group':
                treelayers.append(self.GetPyData(vislayer)[0]['maplayer'])

            if not self.GetNextVisible(vislayer):
                break
            else:
                vislayer = self.GetNextVisible(vislayer)

        Debug.msg (4, "LayerTree.ReoderLayers(): items=%s" % \
                   (itemList))

        # reorder map layers
        treelayers.reverse()
        self.Map.ReorderLayers(treelayers)

    def ChangeLayer(self, item):
        """Change layer"""

        type = self.GetPyData(item)[0]['type']

        if type == 'command':
            win = self.FindWindowById(self.GetPyData(item)[0]['ctrl'])
            if win.GetValue() != None:
                cmdlist = win.GetValue().split(' ')
                opac = 1.0
                chk = self.IsItemChecked(item)
                hidden = not self.IsVisible(item)
        elif type != 'group':
            if self.GetPyData(item)[0] is not None:
                cmdlist = self.GetPyData(item)[0]['cmd']
                opac = self.GetPyData(item)[0]['maplayer'].GetOpacity(float=True)
                chk = self.IsItemChecked(item)
                hidden = not self.IsVisible(item)
        maplayer = self.Map.ChangeLayer(layer=self.GetPyData(item)[0]['maplayer'], type=type,
                                        command=cmdlist, name=self.GetItemText(item),
                                        l_active=chk, l_hidden=hidden, l_opacity=opac, l_render=False)

        self.GetPyData(item)[0]['maplayer'] = maplayer

        # if digitization tool enabled -> update list of available vector map layers
        if self.mapdisplay.toolbars['vdigit']:
            self.mapdisplay.toolbars['vdigit'].UpdateListOfLayers(updateTool=True)

        # redraw map if auto-rendering is enabled
        if self.mapdisplay.autoRender.GetValue(): 
            self.mapdisplay.OnRender(None)

        # item.SetHeight(TREE_ITEM_HEIGHT)

    def OnCloseWindow(self, event):
        pass
        # self.Map.Clean()

    def FindItemByData(self, key, value):
        """Find item based on key and value (see PyData[0])"""
        item = self.GetFirstChild(self.root)[0]
        return self.__FindSubItemByData(item, key, value)

    def __FindSubItemByData(self, item, key, value):
        """Support method for FindItemByValue"""
        while item and item.IsOk():
            itemValue = self.GetPyData(item)[0][key]
            if value == itemValue:
                return item
            if self.GetPyData(item)[0]['type'] == 'group':
                subItem = self.GetFirstChild(item)[0]
                found = self.__FindSubItemByData(subItem, key, value)
                if found:
                    return found
            item = self.GetNextSibling(item)

        return None
