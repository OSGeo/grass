"""!
@package lmgr.layertree

@brief Utility classes for map layer management.

Classes:
 - layertree::LayerTree

(C) 2007-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.
 
@author Michael Barton (Arizona State University)
@author Jachym Cepicky (Mendel University of Agriculture)
@author Martin Landa <landa.martin gmail.com>
"""

import sys
import wx
try:
    import wx.lib.agw.customtreectrl as CT
except ImportError:
    import wx.lib.customtreectrl as CT
import wx.lib.buttons  as buttons
try:
    import treemixin 
except ImportError:
    from wx.lib.mixins import treemixin

from grass.script import core as grass
from grass.script import vector as gvector

from core                 import globalvar
from gui_core.dialogs     import SqlQueryFrame, SetOpacityDialog
from gui_core.forms       import GUI
from mapdisp.frame        import MapFrame
from core.render          import Map
from wxplot.histogram     import HistogramPlotFrame
from core.utils           import GetLayerNameFromCmd, ltype2command, _
from wxplot.profile       import ProfileFrame
from core.debug           import Debug
from core.settings        import UserSettings, GetDisplayVectSettings
from vdigit.main          import haveVDigit
from core.gcmd            import GWarning, GError
from gui_core.toolbars    import BaseIcons
from icons.icon           import MetaIcon
from web_services.dialogs import SaveWMSLayerDialog
from lmgr.giface import LayerManagerGrassInterfaceForMapDisplay


TREE_ITEM_HEIGHT = 25

LMIcons = {
    'rastImport' : MetaIcon(img = 'layer-import',
                            label = _('Import raster data')),
    'rastLink'   : MetaIcon(img = 'layer-import',
                            label = _('Link external raster data')),
    'rastOut'    : MetaIcon(img = 'layer-export',
                            label = _('Set raster output format')),
    'vectImport' : MetaIcon(img = 'layer-import',
                            label = _('Import vector data')),
    'vectLink'   : MetaIcon(img = 'layer-import',
                                    label = _('Link external vector data')),
    'vectOut'    : MetaIcon(img = 'layer-export',
                            label = _('Set vector output format')),
    'wmsImport'  : MetaIcon(img = 'layer-wms-add',
                            label = _('Import data from WMS server')),
    'addCmd'     : MetaIcon(img = 'layer-command-add',
                            label = _('Add command layer')),
    'quit'       : MetaIcon(img = 'quit',
                            label = _('Quit')),
    'addRgb'     : MetaIcon(img = 'layer-rgb-add',
                            label = _('Add RGB map layer')),
    'addHis'     : MetaIcon(img = 'layer-his-add',
                                    label = _('Add HIS map layer')),
    'addShaded'  : MetaIcon(img = 'layer-shaded-relief-add',
                            label = _('Add shaded relief map layer')),
    'addRArrow'  : MetaIcon(img = 'layer-aspect-arrow-add',
                            label = _('Add raster flow arrows')),
    'addRNum'    : MetaIcon(img = 'layer-cell-cats-add',
                            label = _('Add raster cell numbers')),
    'addThematic': MetaIcon(img = 'layer-vector-thematic-add',
                            label = _('Add thematic area (choropleth) map layer')),
    'addChart'   : MetaIcon(img = 'layer-vector-chart-add',
                            label = _('Add thematic chart layer')),
    'addGrid'    : MetaIcon(img = 'layer-grid-add',
                            label = _('Add grid layer')),
    'addGeodesic': MetaIcon(img = 'shortest-distance',
                            label = _('Add geodesic line layer')),
    'addRhumb'   : MetaIcon(img = 'shortest-distance',
                            label = _('Add rhumbline layer')),
    'addLabels'  : MetaIcon(img = 'layer-label-add',
                            label = _('Add labels')),
    'addRast3d'  : MetaIcon(img = 'layer-raster3d-add',
                            label = _('Add 3D raster map layer'),
                            desc  =  _('Note that 3D raster data are rendered only in 3D view mode')),
    'wsImport'  :  MetaIcon(img = 'layer-wms-add',
                            label = _('Add WMS layer.')),
    'layerOptions'  : MetaIcon(img = 'options',
                               label = _('Set options'))
    }

class LayerTree(treemixin.DragAndDrop, CT.CustomTreeCtrl):
    """!Creates layer tree structure
    """
    def __init__(self, parent, giface,
                 id = wx.ID_ANY, style = wx.SUNKEN_BORDER,
                 ctstyle = CT.TR_HAS_BUTTONS | CT.TR_HAS_VARIABLE_ROW_HEIGHT |
                 CT.TR_HIDE_ROOT | CT.TR_ROW_LINES | CT.TR_FULL_ROW_HIGHLIGHT |
                 CT.TR_MULTIPLE, **kwargs):
        
        if 'style' in kwargs:
            ctstyle |= kwargs['style']
            del kwargs['style']
        self.displayIndex = kwargs['idx']
        del kwargs['idx']
        self.lmgr = kwargs['lmgr']
        del kwargs['lmgr']
        self.notebook = kwargs['notebook']   # GIS Manager notebook for layer tree
        del kwargs['notebook']
        showMapDisplay = kwargs['showMapDisplay']
        del kwargs['showMapDisplay']

        self._giface = giface
        self.treepg = parent                 # notebook page holding layer tree
        self.Map = Map()                     # instance of render.Map to be associated with display
        self.root = None                     # ID of layer tree root node
        self.groupnode = 0                   # index value for layers
        self.optpage = {}                    # dictionary of notebook option pages for each map layer
        self.saveitem = {}                   # dictionary to preserve layer attributes for drag and drop
        self.first = True                    # indicates if a layer is just added or not
        self.flag = ''                       # flag for drag and drop hittest
        # layer change requires a rerendering
        # (used to request rendering only when layer changes are finished)
        self.rerender = False
        self.hitCheckbox = False             # if cursor points at layer checkbox (to cancel selection changes)
        self.forceCheck = False              # force check layer if CheckItem is called
        
        try:
            ctstyle |= CT.TR_ALIGN_WINDOWS
        except AttributeError:
            pass
        
        if globalvar.hasAgw:
            super(LayerTree, self).__init__(parent, id, agwStyle = ctstyle, **kwargs)
        else:
            super(LayerTree, self).__init__(parent, id, style = ctstyle, **kwargs)
        self.SetName("LayerTree")
        
        ### SetAutoLayout() causes that no vertical scrollbar is displayed
        ### when some layers are not visible in layer tree
        # self.SetAutoLayout(True)
        self.SetGradientStyle(1)
        self.EnableSelectionGradient(True)
        self._setGradient()
        
        # init associated map display
        pos = wx.Point((self.displayIndex + 1) * 25, (self.displayIndex + 1) * 25)
        self._gifaceForDisplay = LayerManagerGrassInterfaceForMapDisplay(self._giface,
                                                                         self)
        self.mapdisplay = MapFrame(self, giface=self._gifaceForDisplay,
                                   id = wx.ID_ANY, pos = pos,
                                   size = globalvar.MAP_WINDOW_SIZE,
                                   style = wx.DEFAULT_FRAME_STYLE,
                                   tree=self, notebook=self.notebook,
                                   lmgr = self.lmgr, page = self.treepg,
                                   Map = self.Map)
        
        # title
        self.mapdisplay.SetTitle(_("GRASS GIS Map Display: %(id)d  - Location: %(loc)s") % \
                                     { 'id' : self.displayIndex + 1,
                                       'loc' : grass.gisenv()["LOCATION_NAME"] })
        
        # show new display
        if showMapDisplay is True:
            self.mapdisplay.Show()
            self.mapdisplay.Refresh()
            self.mapdisplay.Update()
        
        self.root = self.AddRoot(_("Map Layers"))
        self.SetPyData(self.root, (None, None))
        
        # create image list to use with layer tree
        il = wx.ImageList(16, 16, mask = False)
        
        trart = wx.ArtProvider.GetBitmap(wx.ART_FILE_OPEN, wx.ART_OTHER, (16, 16))
        self.folder_open = il.Add(trart)
        trart = wx.ArtProvider.GetBitmap(wx.ART_FOLDER, wx.ART_OTHER, (16, 16))
        self.folder = il.Add(trart)
        
        bmpsize = (16, 16)
        trgif = BaseIcons["addRast"].GetBitmap(bmpsize)
        self.rast_icon = il.Add(trgif)
        
        trgif = LMIcons["addRast3d"].GetBitmap(bmpsize)
        self.rast3d_icon = il.Add(trgif)
        
        trgif = LMIcons["addRgb"].GetBitmap(bmpsize)
        self.rgb_icon = il.Add(trgif)
        
        trgif = LMIcons["addHis"].GetBitmap(bmpsize)
        self.his_icon = il.Add(trgif)
        
        trgif = LMIcons["addShaded"].GetBitmap(bmpsize)
        self.shaded_icon = il.Add(trgif)
        
        trgif = LMIcons["addRArrow"].GetBitmap(bmpsize)
        self.rarrow_icon = il.Add(trgif)
        
        trgif = LMIcons["addRNum"].GetBitmap(bmpsize)
        self.rnum_icon = il.Add(trgif)
        
        trgif = BaseIcons["addVect"].GetBitmap(bmpsize)
        self.vect_icon = il.Add(trgif)
        
        trgif = LMIcons["addThematic"].GetBitmap(bmpsize)
        self.theme_icon = il.Add(trgif)
        
        trgif = LMIcons["addChart"].GetBitmap(bmpsize)
        self.chart_icon = il.Add(trgif)
        
        trgif = LMIcons["addGrid"].GetBitmap(bmpsize)
        self.grid_icon = il.Add(trgif)
        
        trgif = LMIcons["addGeodesic"].GetBitmap(bmpsize)
        self.geodesic_icon = il.Add(trgif)
        
        trgif = LMIcons["addRhumb"].GetBitmap(bmpsize)
        self.rhumb_icon = il.Add(trgif)
        
        trgif = LMIcons["addLabels"].GetBitmap(bmpsize)
        self.labels_icon = il.Add(trgif)
        
        trgif = LMIcons["addCmd"].GetBitmap(bmpsize)
        self.cmd_icon = il.Add(trgif)

        trgif = LMIcons["wsImport"].GetBitmap(bmpsize)
        self.ws_icon = il.Add(trgif)
        
        self.AssignImageList(il)

        self.Bind(wx.EVT_TREE_ITEM_ACTIVATED,   self.OnActivateLayer)
        self.Bind(wx.EVT_TREE_SEL_CHANGED,      self.OnChangeSel)
        self.Bind(wx.EVT_TREE_SEL_CHANGING,     self.OnChangingSel)
        self.Bind(CT.EVT_TREE_ITEM_CHECKED,     self.OnLayerChecked)
        self.Bind(CT.EVT_TREE_ITEM_CHECKING,    self.OnLayerChecking)
        self.Bind(wx.EVT_TREE_DELETE_ITEM,      self.OnDeleteLayer)
        self.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, self.OnLayerContextMenu)
        self.Bind(wx.EVT_TREE_END_DRAG,         self.OnEndDrag)
        self.Bind(wx.EVT_TREE_END_LABEL_EDIT,   self.OnRenamed)
        self.Bind(wx.EVT_KEY_UP,                self.OnKeyUp)
        self.Bind(wx.EVT_IDLE,                  self.OnIdle)
        self.Bind(wx.EVT_MOTION,                self.OnMotion)

    def _getSelectedLayer(self):
        """!Get selected layer.

        @return None if no layer selected
        @return first layer (GenericTreeItem instance) of all selected
        """
        return self.GetSelectedLayer(multi = False, checkedOnly = False)

    # for compatibility
    layer_selected = property(fget = _getSelectedLayer)

    def GetSelectedLayers(self, checkedOnly = False):
        """!Get selected layers as a list.

        @todo somewhere we have checkedOnly default True and elsewhere False
        """
        return self.GetSelectedLayer(multi = True, checkedOnly = checkedOnly)

    def GetSelectedLayer(self, multi = False, checkedOnly = False):
        """!Get selected layer from layer tree.
        
        @param multi return multiple selection as a list
        @param checkedOnly return only the checked layers

        @return None or [] for multi == True if no layer selected 
        @return first layer (GenericTreeItem instance) of all selected or a list
        """
        ret = []
        layers = self.GetSelections()

        for layer in layers:
            if not checkedOnly or (checkedOnly and layer.IsChecked()):
                ret.append(layer)
        if multi:
            return ret

        if ret:
            return ret[0]

        return None

    def _setGradient(self, iType = None):
        """!Set gradient for items

        @param iType bgmap, vdigit or None
        """
        if iType == 'bgmap':
            self.SetFirstGradientColour(wx.Colour(0, 100, 0))
            self.SetSecondGradientColour(wx.Colour(0, 150, 0))
        elif iType == 'vdigit':
            self.SetFirstGradientColour(wx.Colour(100, 0, 0))
            self.SetSecondGradientColour(wx.Colour(150, 0, 0))
        else:
            self.SetFirstGradientColour(wx.Colour(100, 100, 100))
            self.SetSecondGradientColour(wx.Colour(150, 150, 150))
        
    def GetSelections(self):
        """Returns a list of selected items.

        This method is copied from customtreecontrol and overriden because
        with some version wx (?) multiple selection doesn't work. 
        Probably it is caused by another GetSelections method in treemixin.DragAndDrop?
        """
        array = []
        idRoot = self.GetRootItem()
        if idRoot:
            array = self.FillArray(idRoot, array)
        
        #else: the tree is empty, so no selections

        return array

    def GetMap(self):
        """!Get map instace"""
        return self.Map
    
    def GetMapDisplay(self):
        """!Get associated MapFrame"""
        return self.mapdisplay

    def GetLayerInfo(self, layer, key = None):
        """!Get layer info.

        @param layer GenericTreeItem instance
        @param key cmd, type, ctrl, label, maplayer, propwin, vdigit, nviz
         (vdigit, nviz for map layers only)
        """
        if not self.GetPyData(layer):
            return None
        if key:
            return self.GetPyData(layer)[0][key]
        return self.GetPyData(layer)[0]

    def SetLayerInfo(self, layer, key, value):
        """!Set layer info.

        @param layer GenericTreeItem instance
        @param key cmd, type, ctrl, label, maplayer, propwin, vdigit, nviz
         (vdigit, nviz for map layers only)
        @param value value
        """
        info = self.GetPyData(layer)[0]
        info[key] = value

    def GetLayerParams(self, layer):
        """!Get layer command params"""
        return self.GetPyData(layer)[1]

    def OnIdle(self, event):
        """!Only re-order and re-render a composite map image from GRASS during
        idle time instead of multiple times during layer changing.
        """
        # no need to check for digitizer since it is handled internaly
        # no need to distinguish 2D and 3D since the interface is the same
        # remove this comment when it is onl enough
        if self.rerender:
            # restart rerender value here before wx.Yield
            # can cause another idle event
            self.rerender = False
            if self.mapdisplay.IsAutoRendered():
                self.mapdisplay.GetMapWindow().UpdateMap(render=True)
        
        event.Skip()
        
    def OnKeyUp(self, event):
        """!Key pressed"""
        key = event.GetKeyCode()
        
        if key == wx.WXK_DELETE and self.lmgr and \
                not self.GetEditControl():
            self.lmgr.OnDeleteLayer(None)
        
        event.Skip()
        
    def OnLayerContextMenu (self, event):
        """!Contextual menu for item/layer"""
        if not self.layer_selected:
            event.Skip()
            return

        ltype = self.GetLayerInfo(self.layer_selected, key = 'type')
        mapLayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        
        Debug.msg (4, "LayerTree.OnContextMenu: layertype=%s" % \
                       ltype)

        if not hasattr (self, "popupID"):
            self.popupID = dict()
            for key in ('remove', 'rename', 'opacity', 'nviz', 'zoom',
                        'region', 'export', 'attr', 'edit0', 'edit1', 'save_ws',
                        'bgmap', 'topo', 'meta', 'null', 'zoom1', 'region1',
                        'color', 'hist', 'univar', 'prof', 'properties', 'sql'):
                self.popupID[key] = wx.NewId()
        
        self.popupMenu = wx.Menu()
        
        numSelected = len(self.GetSelections())
        
        self.popupMenu.Append(self.popupID['remove'], text = _("Remove"))
        self.Bind(wx.EVT_MENU, self.lmgr.OnDeleteLayer, id = self.popupID['remove'])
        
        if ltype != "command" and numSelected == 1:
            self.popupMenu.Append(self.popupID['rename'], text = _("Rename"))
            self.Bind(wx.EVT_MENU, self.OnRenameLayer, id = self.popupID['rename'])

        # when multiple maps are selected of different types
        # we cannot zoom or change region
        # because g.region can handle only the same type
        same = True
        selected = self.GetSelectedLayers()
        for layer in selected:
            if self.GetLayerInfo(layer, key='type') != ltype:
                same = False
                break
        
        # map layer items
        if ltype not in ("group", "command"):
            if numSelected == 1:
                self.popupMenu.AppendSeparator()
                if ltype != '3d-raster':
                    self.popupMenu.Append(self.popupID['opacity'], text=_("Change opacity level"))
                    self.Bind(wx.EVT_MENU, self.OnPopupOpacityLevel, id=self.popupID['opacity'])
                self.popupMenu.Append(self.popupID['properties'], text = _("Properties"))
                self.Bind(wx.EVT_MENU, self.OnPopupProperties, id = self.popupID['properties'])
            
                if ltype in ('raster', 'vector', '3d-raster') and self.lmgr.IsPaneShown('toolbarNviz'):
                    self.popupMenu.Append(self.popupID['nviz'], _("3D view properties"))
                    self.Bind (wx.EVT_MENU, self.OnNvizProperties, id = self.popupID['nviz'])

            if same and ltype in ('raster', 'vector', 'rgb', '3d-raster'):
                self.popupMenu.AppendSeparator()
                self.popupMenu.Append(self.popupID['zoom'], text = _("Zoom to selected map(s)"))
                self.Bind(wx.EVT_MENU, self.mapdisplay.OnZoomToMap, id = self.popupID['zoom'])
                self.popupMenu.Append(self.popupID['region'], text = _("Set computational region from selected map(s)"))
                self.Bind(wx.EVT_MENU, self.OnSetCompRegFromMap, id = self.popupID['region'])
        
        # specific items
        try:
            mltype = self.GetLayerInfo(self.layer_selected, key = 'type')
        except:
            mltype = None
        
        # vector layers (specific items)
        if mltype and mltype == "vector" and numSelected == 1:
            self.popupMenu.AppendSeparator()
            self.popupMenu.Append(self.popupID['export'], text = _("Export"))
            self.Bind(wx.EVT_MENU, lambda x: self.lmgr.OnMenuCmd(cmd = ['v.out.ogr',
                                                                        'input=%s' % mapLayer.GetName()]),
                      id = self.popupID['export'])
            
            self.popupMenu.AppendSeparator()

            self.popupMenu.Append(self.popupID['color'], _("Set color table"))
            self.Bind (wx.EVT_MENU, self.OnVectorColorTable, id = self.popupID['color'])

            self.popupMenu.Append(self.popupID['attr'], text = _("Show attribute data"))
            self.Bind(wx.EVT_MENU, self.lmgr.OnShowAttributeTable, id = self.popupID['attr'])

            self.popupMenu.Append(self.popupID['edit0'], text = _("Start editing"))
            self.popupMenu.Append(self.popupID['edit1'], text = _("Stop editing"))
            self.popupMenu.Enable(self.popupID['edit1'], False)
            self.Bind (wx.EVT_MENU, self.OnStartEditing, id = self.popupID['edit0'])
            self.Bind (wx.EVT_MENU, self.OnStopEditing,  id = self.popupID['edit1'])
            
            layer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
            # enable editing only for vector map layers available in the current mapset
            digitToolbar = self.mapdisplay.GetToolbar('vdigit')
            if digitToolbar:
                # background vector map
                self.popupMenu.Append(self.popupID['bgmap'],
                                      text = _("Use as background vector map for digitizer"),
                                      kind = wx.ITEM_CHECK)
                self.Bind(wx.EVT_MENU, self.OnSetBgMap, id = self.popupID['bgmap'])
                if UserSettings.Get(group = 'vdigit', key = 'bgmap', subkey = 'value',
                                    internal = True) == layer.GetName():
                    self.popupMenu.Check(self.popupID['bgmap'], True)
            
            self.popupMenu.Append(self.popupID['topo'], text = _("Rebuild topology"))
            self.Bind(wx.EVT_MENU, self.OnTopology, id = self.popupID['topo'])

            # determine format
            if layer and layer.GetType() == 'vector':
                if 'info' not in self.GetLayerInfo(self.layer_selected):
                    info = grass.parse_command('v.info',
                                               flags = 'e',
                                               map = layer.GetName())
                    self.SetLayerInfo(self.layer_selected, key = 'info', value = info)
                info = self.GetLayerInfo(self.layer_selected, key = 'info')
                if info and info['format'] != 'native' and \
                        info['format'].split(',')[1] == 'PostgreSQL':
                    self.popupMenu.Append(self.popupID['sql'], text = _("SQL Spatial Query"))
                    self.Bind(wx.EVT_MENU, self.OnSqlQuery, id = self.popupID['sql'])
            
            if layer.GetMapset() != grass.gisenv()['MAPSET']:
                # only vector map in current mapset can be edited
                self.popupMenu.Enable (self.popupID['edit0'], False)
                self.popupMenu.Enable (self.popupID['edit1'], False)
                self.popupMenu.Enable (self.popupID['topo'], False)
            elif digitToolbar and digitToolbar.GetLayer():
                # vector map already edited
                vdigitLayer = digitToolbar.GetLayer()
                if vdigitLayer is layer:
                    self.popupMenu.Enable(self.popupID['edit0'],  False)
                    self.popupMenu.Enable(self.popupID['edit1'],  True)
                    self.popupMenu.Enable(self.popupID['remove'], False)
                    self.popupMenu.Enable(self.popupID['bgmap'],  False)
                    self.popupMenu.Enable(self.popupID['topo'],   False)
                else:
                    self.popupMenu.Enable(self.popupID['edit0'], False)
                    self.popupMenu.Enable(self.popupID['edit1'], False)
                    self.popupMenu.Enable(self.popupID['bgmap'], True)
            
            self.popupMenu.Append(self.popupID['meta'], _("Metadata"))
            self.Bind (wx.EVT_MENU, self.OnMetadata, id = self.popupID['meta'])
            
        # raster layers (specific items)
        elif mltype and mltype == "raster":
            if same:
                self.popupMenu.Append(self.popupID['zoom1'], text=_("Zoom to selected map(s) (ignore NULLs)"))
                self.Bind(wx.EVT_MENU, self.mapdisplay.OnZoomToRaster, id=self.popupID['zoom1'])
                self.popupMenu.Append(self.popupID['region1'], text=_("Set computational region from selected map(s) (ignore NULLs)"))
                self.Bind(wx.EVT_MENU, self.OnSetCompRegFromRaster, id=self.popupID['region1'])
            
            self.popupMenu.AppendSeparator()
            
            if numSelected == 1:
                self.popupMenu.Append(self.popupID['export'], text = _("Export"))
                self.Bind(wx.EVT_MENU, lambda x: self.lmgr.OnMenuCmd(cmd = ['r.out.gdal',
                                                                            'input=%s' % mapLayer.GetName()]),
                          id = self.popupID['export'])
                
                self.popupMenu.AppendSeparator()
                
            self.popupMenu.Append(self.popupID['color'], _("Set color table"))
            self.Bind (wx.EVT_MENU, self.OnRasterColorTable, id = self.popupID['color'])
            self.popupMenu.Append(self.popupID['hist'], _("Histogram"))
            self.Bind (wx.EVT_MENU, self.OnHistogram, id = self.popupID['hist'])
            self.popupMenu.Append(self.popupID['univar'], _("Univariate raster statistics"))
            self.Bind (wx.EVT_MENU, self.OnUnivariateStats, id = self.popupID['univar'])

            if numSelected == 1:
                self.popupMenu.Append(self.popupID['prof'], _("Profile"))
                self.Bind (wx.EVT_MENU, self.OnProfile, id = self.popupID['prof'])
                self.popupMenu.Append(self.popupID['meta'], _("Metadata"))
                self.Bind (wx.EVT_MENU, self.OnMetadata, id = self.popupID['meta'])
            
        elif mltype and mltype == '3d-raster':
            if numSelected == 1:
                self.popupMenu.AppendSeparator()
                self.popupMenu.Append(self.popupID['color'], _("Set color table"))
                self.Bind(wx.EVT_MENU, self.OnRasterColorTable, id=self.popupID['color'])
                self.popupMenu.Append(self.popupID['univar'], _("Univariate raster statistics"))
                self.Bind(wx.EVT_MENU, self.OnUnivariateStats, id=self.popupID['univar'])
                self.popupMenu.Append(self.popupID['meta'], _("Metadata"))
                self.Bind(wx.EVT_MENU, self.OnMetadata, id=self.popupID['meta'])
        
        # web service layers (specific item)
        elif mltype and mltype == "wms":
            self.popupMenu.Append(self.popupID['save_ws'], text = _("Save web service layer"))
            self.Bind(wx.EVT_MENU, self.OnSaveWs, id = self.popupID['save_ws'])

        self.PopupMenu(self.popupMenu)
        self.popupMenu.Destroy()

    def OnSaveWs(self, event):
        """!Show dialog for saving web service layer into GRASS vector/raster layer"""
        mapLayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        dlg = SaveWMSLayerDialog(parent=self, layer=mapLayer,
                                 giface=self._gifaceForDisplay)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnTopology(self, event):
        """!Rebuild topology of selected vector map"""
        mapLayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        cmd = ['v.build',
               'map=%s' % mapLayer.GetName()]
        self._giface.RunCmd(cmd, switchPage = True)

    def OnSqlQuery(self, event):
        """!Show SQL query window for PostGIS layers
        """
        dlg = SqlQueryFrame(parent = self)
        dlg.CentreOnScreen()
        dlg.Show()
        
    def OnMetadata(self, event):
        """!Print metadata of raster/vector map layer
        TODO: Dialog to modify metadata
        """
        mapLayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        mltype = self.GetLayerInfo(self.layer_selected,key = 'type')

        if mltype == 'raster':
            cmd = ['r.info']
        elif mltype == 'vector':
            cmd = ['v.info']
        elif mltype == '3d-raster':
            cmd = ['r3.info']
        cmd.append('map=%s' % mapLayer.GetName())

        # print output to command log area
        self._giface.RunCmd(cmd, switchPage = True)

    def OnSetCompRegFromRaster(self, event):
        """!Set computational region from selected raster map (ignore NULLs)"""
        mapLayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        
        cmd = ['g.region',
               '-p',
               'zoom=%s' % mapLayer.GetName()]
        
        # print output to command log area
        self._giface.RunCmd(cmd)
        
        # re-render map display
        self._giface.GetMapWindow().UpdateMap(render=True)

    def OnSetCompRegFromMap(self, event):
        """!Set computational region from selected raster/vector map
        """
        rast = []
        vect = []
        rast3d = []
        for layer in self.GetSelections():
            mapLayer = self.GetLayerInfo(layer, key = 'maplayer')
            mltype = self.GetLayerInfo(layer, key = 'type')
                
            if mltype == 'raster':
                rast.append(mapLayer.GetName())
            elif mltype == 'vector':
                vect.append(mapLayer.GetName())
            elif mltype == '3d-raster':
                rast3d.append(mapLayer.GetName())
            elif mltype == 'rgb':
                for rname in mapLayer.GetName().splitlines():
                    rast.append(rname)
        
        cmd = ['g.region']
        if rast:
            cmd.append('rast=%s' % ','.join(rast))
        if vect:
            cmd.append('vect=%s' % ','.join(vect))
        if rast3d:
            cmd.append('rast3d=%s' % ','.join(rast3d))
        
        # print output to command log area
        if len(cmd) > 1:
            cmd.append('-p')
            if mltype == '3d-raster':
                cmd.append('-3')
            self._giface.RunCmd(cmd, compReg = False)
        
        # re-render map display
        self._giface.GetMapWindow().UpdateMap(render=True)
            
    def OnProfile(self, event):
        """!Plot profile of given raster map layer"""
        mapLayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        if not mapLayer.GetName():
            wx.MessageBox(parent = self,
                          message = _("Unable to create profile of "
                                    "raster map."),
                          caption = _("Error"), style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return
        self.mapdisplay.Profile(rasters=[mapLayer.GetName()])

    def OnRasterColorTable(self, event):
        """!Set color table for 2D/3D raster map"""
        raster2d = []
        raster3d = []
        for layer in self.GetSelectedLayers():
            if self.GetLayerInfo(layer, key='type') == '3d-raster':
                raster3d.append(self.GetLayerInfo(layer, key = 'maplayer').GetName())
            else:
                raster2d.append(self.GetLayerInfo(layer, key = 'maplayer').GetName())
        
        if raster2d:
            GUI(parent = self, giface = self._giface).ParseCommand(['r.colors',
                                                                    'map=%s' % ','.join(raster2d)])
        if raster3d:
            GUI(parent = self, giface = self._giface).ParseCommand(['r3.colors',
                                                                    'map=%s' % ','.join(raster3d)])
            
    def OnVectorColorTable(self, event):
        """!Set color table for vector map"""
        name = self.GetLayerInfo(self.layer_selected, key = 'maplayer').GetName()
        GUI(parent = self, centreOnParent = False).ParseCommand(['v.colors',
                                                                 'map=%s' % name])
        
    def OnHistogram(self, event):
        """!Plot histogram for given raster map layer
        """
        rasterList = []
        for layer in self.GetSelectedLayers():
            rasterList.append(self.GetLayerInfo(layer, key = 'maplayer').GetName())

        if not rasterList:
            GError(parent = self,
                   message = _("Unable to display histogram of "
                               "raster map. No map name defined."))
            return
        
        win = HistogramPlotFrame(parent = self, rasterList = rasterList)
        win.CentreOnScreen()
        win.Show()
                
    def OnUnivariateStats(self, event):
        """!Univariate 2D/3D raster statistics"""
        raster2d = []
        raster3d = []
        for layer in self.GetSelectedLayers():
            if self.GetLayerInfo(layer, key='type') == '3d-raster':
                raster3d.append(self.GetLayerInfo(layer, key = 'maplayer').GetName())
            else:
                raster2d.append(self.GetLayerInfo(layer, key = 'maplayer').GetName())
        
        if raster2d:
            self._giface.RunCmd(['r.univar', 'map=%s' % ','.join(raster2d)], switchPage=True)
        
        if raster3d:
            self._giface.RunCmd(['r3.univar', 'map=%s' % ','.join(raster3d)], switchPage=True)

    def OnStartEditing(self, event):
        """!Start editing vector map layer requested by the user
        """
        try:
            maplayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        except:
            event.Skip()
            return
        
        if not haveVDigit:
            from vdigit import errorMsg
            
            self.mapdisplay.toolbars['map'].combo.SetValue (_("2D view"))
            
            GError(_("Unable to start wxGUI vector digitizer.\n"
                     "Details: %s") % errorMsg, parent = self)
            return
        
        if not self.mapdisplay.GetToolbar('vdigit'): # enable tool
            self.mapdisplay.AddToolbar('vdigit')
        else: # tool already enabled
            pass
        
        # mark layer as 'edited'
        self.mapdisplay.toolbars['vdigit'].StartEditing(maplayer)
        
        self._setGradient('vdigit')
        self.RefreshLine(self.layer_selected)
        
    def OnStopEditing(self, event):
        """!Stop editing the current vector map layer
        """
        maplayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        
        self.mapdisplay.toolbars['vdigit'].OnExit()
        # here was dead code to enable vdigit button in toolbar

        self._setGradient()
        self.RefreshLine(self.layer_selected)
        
    def OnSetBgMap(self, event):
        """!Set background vector map for editing sesstion"""
        digit = self.mapdisplay.GetWindow().digit
        if event.IsChecked():
            mapName = self.GetLayerInfo(self.layer_selected, key = 'maplayer').GetName()
            UserSettings.Set(group = 'vdigit', key = 'bgmap', subkey = 'value',
                             value = str(mapName), internal = True)
            digit.OpenBackgroundMap(mapName)
            self._setGradient('bgmap')
        else:
            UserSettings.Set(group = 'vdigit', key = 'bgmap', subkey = 'value',
                             value = '', internal = True)
            digit.CloseBackgroundMap()
            self._setGradient()
        
        self.RefreshLine(self.layer_selected)

    def OnPopupProperties (self, event):
        """!Popup properties dialog"""
        self.PropertiesDialog(self.layer_selected)

    def OnPopupOpacityLevel(self, event):
        """!Popup opacity level indicator"""
        if not self.GetLayerInfo(self.layer_selected, key = 'ctrl'):
            return
        
        maplayer = self.GetLayerInfo(self.layer_selected, key = 'maplayer')
        current_opacity = maplayer.GetOpacity()
        
        dlg = SetOpacityDialog(self, opacity = current_opacity,
                               title = _("Set opacity of <%s>") % maplayer.GetName())
        dlg.applyOpacity.connect(lambda value:
                                 self.ChangeLayerOpacity(layer=self.layer_selected, value=value))
        dlg.CentreOnParent()

        if dlg.ShowModal() == wx.ID_OK:
            self.ChangeLayerOpacity(layer = self.layer_selected, value = dlg.GetOpacity())
        dlg.Destroy()

    def ChangeLayerOpacity(self, layer, value):
        """!Change opacity value of layer
        @param layer layer for which to change (item in layertree)
        @param value opacity value (float between 0 and 1)
        """
        maplayer = self.GetLayerInfo(layer, key = 'maplayer')
        self.Map.ChangeOpacity(maplayer, value)
        maplayer.SetOpacity(value)
        self.SetItemText(layer,
                         self._getLayerName(layer))
        
        # vector layer currently edited
        if self.GetMapDisplay().GetToolbar('vdigit') and \
                self.GetMapDisplay().GetToolbar('vdigit').GetLayer() == maplayer:
            alpha = int(value * 255)
            self.GetMapDisplay().GetWindow().digit.GetDisplay().UpdateSettings(alpha = alpha)
            
        # redraw map if auto-rendering is enabled
        renderVector = False
        if self.GetMapDisplay().GetToolbar('vdigit'):
            renderVector = True
        self.GetMapDisplay().GetWindow().UpdateMap(render = False, renderVector = renderVector)

    def OnNvizProperties(self, event):
        """!Nviz-related properties (raster/vector/volume)

        @todo vector/volume
        """
        self.lmgr.notebook.SetSelectionByName('nviz')
        ltype = self.GetLayerInfo(self.layer_selected, key = 'type')
        if ltype == 'raster':
            self.lmgr.nviz.SetPage('surface')
        elif ltype == 'vector':
            self.lmgr.nviz.SetPage('vector')
        elif ltype == '3d-raster':
            self.lmgr.nviz.SetPage('volume')
        
    def OnRenameLayer (self, event):
        """!Rename layer"""
        self.EditLabel(self.layer_selected)
        self.GetEditControl().SetSelection(-1, -1)
        
    def OnRenamed(self, event):
        """!Layer renamed"""
        if not event.GetLabel():
            event.Skip()
            return
        
        item = self.layer_selected
        self.SetLayerInfo(item, key = 'label', value = event.GetLabel())
        self.SetItemText(item, self._getLayerName(item))
        
        event.Skip()

    def AddLayer(self, ltype, lname = None, lchecked = None,
                 lopacity = 1.0, lcmd = None, lgroup = None, lvdigit = None, lnviz = None, multiple = True):
        """!Add new item to the layer tree, create corresponding MapLayer instance.
        Launch property dialog if needed (raster, vector, etc.)

        @param ltype layer type (raster, vector, 3d-raster, ...)
        @param lname layer name
        @param lchecked if True layer is checked
        @param lopacity layer opacity level
        @param lcmd command (given as a list)
        @param lgroup index of group item (-1 for root) or None
        @param lvdigit vector digitizer settings (eg. geometry attributes)
        @param lnviz layer Nviz properties
        @param multiple True to allow multiple map layers in layer tree
        """
        if lname and not multiple:
            # check for duplicates
            item = self.GetFirstVisibleItem()
            while item and item.IsOk():
                if self.GetLayerInfo(item, key = 'type') == 'vector':
                    name = self.GetLayerInfo(item, key = 'maplayer').GetName()
                    if name == lname:
                        return
                item = self.GetNextVisible(item)
        
        self.first = True
        
        selectedLayer = self.GetSelectedLayer()
        # deselect active item
        if selectedLayer:
            self.SelectItem(selectedLayer, select=False)
        
        Debug.msg (3, "LayerTree().AddLayer(): ltype=%s" % (ltype))
        
        if ltype == 'command':
            # generic command item
            ctrl = self._createCommandCtrl()
            ctrl.Bind(wx.EVT_TEXT_ENTER, self.OnCmdChanged)

        elif ltype == 'group':
            # group item
            ctrl = None
            grouptext = _('Layer group:') + str(self.groupnode)
            self.groupnode += 1
        else:
            btnbmp = LMIcons["layerOptions"].GetBitmap((16,16))
            ctrl = buttons.GenBitmapButton(self, id = wx.ID_ANY, bitmap = btnbmp, size = (24,24))
            ctrl.SetToolTipString(_("Click to edit layer settings"))
            self.Bind(wx.EVT_BUTTON, self.OnLayerContextMenu, ctrl)
        # add layer to the layer tree
        if selectedLayer and selectedLayer != self.GetRootItem():
            if self.GetLayerInfo(selectedLayer, key = 'type') == 'group' \
                and self.IsExpanded(selectedLayer):
                # add to group (first child of self.layer_selected) if group expanded
                layer = self.PrependItem(parent = selectedLayer,
                                         text = '', ct_type = 1, wnd = ctrl)
            else:
                # prepend to individual layer or non-expanded group
                if lgroup == -1:
                    # -> last child of root (loading from workspace)
                    layer = self.AppendItem(parentId = self.root,
                                            text = '', ct_type = 1, wnd = ctrl)
                elif lgroup > -1:
                    # -> last child of group (loading from workspace)
                    parent = self.FindItemByIndex(index = lgroup)
                    if not parent:
                        parent = self.root
                    layer = self.AppendItem(parentId = parent,
                                            text = '', ct_type = 1, wnd = ctrl)
                elif lgroup is None:
                    # -> previous sibling of selected layer
                    parent = self.GetItemParent(selectedLayer)
                    layer = self.InsertItem(parentId = parent,
                                            input = self.GetPrevSibling(selectedLayer),
                                            text = '', ct_type = 1, wnd = ctrl)
        else: # add first layer to the layer tree (first child of root)
            layer = self.PrependItem(parent = self.root, text = '', ct_type = 1, wnd = ctrl)

        # layer is initially unchecked as inactive (beside 'command')
        # use predefined value if given
        if lchecked is not None:
            checked = lchecked
        else:
            checked = True
        
        self.forceCheck = True
        self.CheckItem(layer, checked = checked)
        
        # add text and icons for each layer ltype
        label =  _('(double click to set properties)') + ' ' * 15
        if ltype == 'raster':
            self.SetItemImage(layer, self.rast_icon)
            self.SetItemText(layer, '%s %s' % (_('raster'), label))
        elif ltype == '3d-raster':
            self.SetItemImage(layer, self.rast3d_icon)
            self.SetItemText(layer, '%s %s' % (_('3D raster'), label))
        elif ltype == 'rgb':
            self.SetItemImage(layer, self.rgb_icon)
            self.SetItemText(layer, '%s %s' % (_('RGB'), label))
        elif ltype == 'his':
            self.SetItemImage(layer, self.his_icon)
            self.SetItemText(layer, '%s %s' % (_('HIS'), label))
        elif ltype == 'shaded':
            self.SetItemImage(layer, self.shaded_icon)
            self.SetItemText(layer, '%s %s' % (_('shaded relief'), label))
        elif ltype == 'rastnum':
            self.SetItemImage(layer, self.rnum_icon)
            self.SetItemText(layer, '%s %s' % (_('raster cell numbers'), label))
        elif ltype == 'rastarrow':
            self.SetItemImage(layer, self.rarrow_icon)
            self.SetItemText(layer, '%s %s' % (_('raster flow arrows'), label))
        elif ltype == 'vector':
            self.SetItemImage(layer, self.vect_icon)
            self.SetItemText(layer, '%s %s' % (_('vector'), label))
        elif ltype == 'thememap':
            self.SetItemImage(layer, self.theme_icon)
            self.SetItemText(layer, '%s %s' % (_('thematic area (choropleth) map'), label))
        elif ltype == 'themechart':
            self.SetItemImage(layer, self.chart_icon)
            self.SetItemText(layer, '%s %s' % (_('thematic charts'), label))
        elif ltype == 'grid':
            self.SetItemImage(layer, self.grid_icon)
            self.SetItemText(layer, '%s %s' % (_('grid'), label))
        elif ltype == 'geodesic':
            self.SetItemImage(layer, self.geodesic_icon)
            self.SetItemText(layer, '%s %s' % (_('geodesic line'), label))
        elif ltype == 'rhumb':
            self.SetItemImage(layer, self.rhumb_icon)
            self.SetItemText(layer, '%s %s' % (_('rhumbline'), label))
        elif ltype == 'labels':
            self.SetItemImage(layer, self.labels_icon)
            self.SetItemText(layer, '%s %s' % (_('vector labels'), label))
        elif ltype == 'command':
            self.SetItemImage(layer, self.cmd_icon)
        elif ltype == 'group':
            self.SetItemImage(layer, self.folder, CT.TreeItemIcon_Normal)
            self.SetItemImage(layer, self.folder_open, CT.TreeItemIcon_Expanded)
            self.SetItemText(layer, grouptext)
        elif ltype == 'wms':            
            self.SetItemImage(layer, self.ws_icon)
            self.SetItemText(layer, '%s %s' % (_('wms'), label))
        else:
            self.SetItemImage(layer, self.cmd_icon)
            self.SetItemText(layer, '%s %s' % (_('unknown'), label))
        
        self.first = False
        
        if ltype != 'group':
            if lcmd and len(lcmd) > 1:
                cmd = lcmd
                render = False
                name, found = GetLayerNameFromCmd(lcmd)
            else:
                cmd = []
                if ltype == 'command' and lname:
                    for c in lname.split(';'):
                        cmd.append(c.split(' '))
                
                render = False
                name = None
            
            if ctrl:
                ctrlId = ctrl.GetId()
            else:
                ctrlId = None
            
            # add a data object to hold the layer's command (does not apply to generic command layers)
            self.SetPyData(layer, ({'cmd'      : cmd,
                                    'type'     : ltype,
                                    'ctrl'     : ctrlId,
                                    'label'    : None,
                                    'maplayer' : None,
                                    'vdigit'   : lvdigit,
                                    'nviz'     : lnviz,
                                    'propwin'  : None}, 
                                   None))
            
            # find previous map layer instance 
            prevItem = self.GetFirstChild(self.root)[0]
            prevMapLayer = None 
            pos = -1
            while prevItem and prevItem.IsOk() and prevItem != layer: 
                if self.GetLayerInfo(prevItem, key = 'maplayer'): 
                    prevMapLayer = self.GetLayerInfo(prevItem, key = 'maplayer')
                
                prevItem = self.GetNextSibling(prevItem) 
                
                if prevMapLayer: 
                    pos = self.Map.GetLayerIndex(prevMapLayer)
                else: 
                    pos = -1
            
            maplayer = self.Map.AddLayer(pos = pos,
                                         ltype = ltype, command = self.GetLayerInfo(prevItem, key = 'cmd'), name = name,
                                         active = checked, hidden = False,
                                         opacity = lopacity, render = render)
            self.SetLayerInfo(layer, key = 'maplayer', value = maplayer)
            
            # run properties dialog if no properties given
            if len(cmd) == 0:
                self.PropertiesDialog(layer, show = True)
        
        else: # group
            self.SetPyData(layer, ({'cmd'      : None,
                                    'type'     : ltype,
                                    'ctrl'     : None,
                                    'label'    : None,
                                    'maplayer' : None,
                                    'propwin'  : None}, 
                                   None))
        
        # select new item
        self.SelectItem(layer, select = True)

        # use predefined layer name if given
        if lname:
            if ltype == 'group':
                self.SetItemText(layer, lname)
            elif ltype == 'command':
                ctrl.SetValue(lname)
            else:
                self.SetItemText(layer, self._getLayerName(layer, lname))
        else:
            if ltype == 'group':
                self.OnRenameLayer(None)
                
        return layer

    def PropertiesDialog(self, layer, show = True):
        """!Launch the properties dialog"""
        ltype  = self.GetLayerInfo(layer, key = 'type')
        if 'propwin' in self.GetLayerInfo(layer) and \
                self.GetLayerInfo(layer, key = 'propwin') is not None:
            # recycle GUI dialogs
            win = self.GetLayerInfo(layer, key = 'propwin')
            if win.IsShown():
                win.SetFocus()
            else:
                win.Show()
            
            return
        
        params = self.GetLayerParams(layer)
                
        Debug.msg (3, "LayerTree.PropertiesDialog(): ltype=%s" % \
                   ltype)

        cmd = None
        if self.GetLayerInfo(layer, key = 'cmd'):

            module = GUI(parent = self, show = show, centreOnParent = False)
            module.ParseCommand(self.GetLayerInfo(layer, key = 'cmd'),
                                completed = (self.GetOptData,layer,params))
            
            self.SetLayerInfo(layer, key = 'cmd', value = module.GetCmd())
        elif self.GetLayerInfo(layer, key = 'type') != 'command':
            cmd = [ltype2command[ltype]]
            if ltype == 'raster':
                if UserSettings.Get(group = 'rasterLayer', key = 'opaque', subkey = 'enabled'):
                    cmd.append('-n')
            elif ltype == 'rgb':
                if UserSettings.Get(group = 'rasterLayer', key = 'opaque', subkey = 'enabled'):
                    cmd.append('-n')
            elif ltype == 'vector':
                cmd += GetDisplayVectSettings()
            
            # ltype == 'thememap':
            # -s flag requested, otherwise only first thematic category is displayed
            # should be fixed by C-based d.thematic.* modules
        
        if cmd:
            GUI(parent = self, centreOnParent = False).ParseCommand(cmd,
                                                                    completed = (self.GetOptData,layer,params))
        
    def OnActivateLayer(self, event):
        """!Double click on the layer item.
        Launch property dialog, or expand/collapse group of items, etc.
        """
        self.lmgr.WorkspaceChanged()
        layer = event.GetItem()
        
        self.PropertiesDialog(layer)
        
        if self.GetLayerInfo(layer, key = 'type') == 'group':
            if self.IsExpanded(layer):
                self.Collapse(layer)
            else:
                self.Expand(layer)
        
    def OnDeleteLayer(self, event):
        """!Remove selected layer item from the layer tree"""
        self.lmgr.WorkspaceChanged()
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

        try:
            if self.GetLayerInfo(item, key = 'type') != 'group':
                self.Map.DeleteLayer(self.GetLayerInfo(item, key = 'maplayer'))
        except:
            pass

        # redraw map if auto-rendering is enabled
        self.rerender = True
        self.Map.SetLayers(self.GetVisibleLayers())
        
        if self.mapdisplay.GetToolbar('vdigit'):
            self.mapdisplay.toolbars['vdigit'].UpdateListOfLayers (updateTool = True)

        # here was some dead code related to layer and nviz
        # however, in condition was rerender = False
        # but rerender is alway True
        # (here no change and also in UpdateListOfLayers and GetListOfLayers)
        # You can safely remove this comment after some testing.

        event.Skip()

    def OnLayerChecking(self, event):
        """!Layer checkbox is being checked.

        Continue only if mouse is above checkbox or layer was checked programatically.
        """
        if self.hitCheckbox or self.forceCheck:
            self.forceCheck = False
            event.Skip()
        else:
            event.Veto()

    def OnLayerChecked(self, event):
        """!Enable/disable data layer"""
        self.lmgr.WorkspaceChanged()
        
        item    = event.GetItem()
        checked = item.IsChecked()
        
        digitToolbar = self.mapdisplay.GetToolbar('vdigit')
        if not self.first:
            # change active parameter for item in layers list in render.Map
            if self.GetLayerInfo(item, key = 'type') == 'group':
                child, cookie = self.GetFirstChild(item)
                while child:
                    self.forceCheck = True
                    self.CheckItem(child, checked)
                    mapLayer = self.GetLayerInfo(child, key = 'maplayer')
                    if not digitToolbar or \
                           (digitToolbar and digitToolbar.GetLayer() != mapLayer):
                        # ignore when map layer is edited
                        self.Map.ChangeLayerActive(mapLayer, checked)
                    child = self.GetNextSibling(child)
            else:
                mapLayer = self.GetLayerInfo(item, key = 'maplayer')
                if not digitToolbar or \
                       (digitToolbar and digitToolbar.GetLayer() != mapLayer):
                    # ignore when map layer is edited
                    self.Map.ChangeLayerActive(mapLayer, checked)
        
        # nviz
        if self.lmgr.IsPaneShown('toolbarNviz') and \
                self.GetPyData(item) is not None:
            # nviz - load/unload data layer
            mapLayer = self.GetLayerInfo(item, key = 'maplayer')

            self.mapdisplay.SetStatusText(_("Please wait, updating data..."), 0)

            if checked: # enable
                if mapLayer.type == 'raster':
                    self.mapdisplay.MapWindow.LoadRaster(item)
                elif mapLayer.type == '3d-raster':
                    self.mapdisplay.MapWindow.LoadRaster3d(item)
                elif mapLayer.type == 'vector':
                    vInfo = gvector.vector_info_topo(mapLayer.GetName())
                    if (vInfo['points'] + vInfo['centroids']) > 0:
                        self.mapdisplay.MapWindow.LoadVector(item, points = True)
                    if (vInfo['lines'] + vInfo['boundaries']) > 0:
                        self.mapdisplay.MapWindow.LoadVector(item, points = False)

            else: # disable
                if mapLayer.type == 'raster':
                    self.mapdisplay.MapWindow.UnloadRaster(item)
                elif mapLayer.type == '3d-raster':
                    self.mapdisplay.MapWindow.UnloadRaster3d(item)
                elif mapLayer.type == 'vector':
                    self.mapdisplay.MapWindow.UnloadVector(item)
            
            self.mapdisplay.SetStatusText("", 0)
        
        # redraw map if auto-rendering is enabled
        self.rerender = True
        self.Map.SetLayers(self.GetVisibleLayers())
        
    def OnCmdChanged(self, event):
        """!Change command string"""
        ctrl = event.GetEventObject().GetId()
        
        # find layer tree item by ctrl
        layer = self.GetFirstVisibleItem()
        while layer and layer.IsOk():
            if self.GetLayerInfo(layer, key = 'ctrl') == ctrl:
                break
            layer = self.GetNextVisible(layer)
        
        # change parameters for item in layers list in render.Map
        self.ChangeLayer(layer)
        
        event.Skip()

    def OnMotion(self, event):
        """!Mouse is moving.

        Detects if mouse points at checkbox.
        """
        thisItem, flags = self.HitTest(event.GetPosition())
        # workaround: in order not to check checkox when clicking outside
        # we need flag TREE_HITTEST_ONITEMCHECKICON but not TREE_HITTEST_ONITEMLABEL
        # this applies only for TR_FULL_ROW_HIGHLIGHT style
        if (flags & CT.TREE_HITTEST_ONITEMCHECKICON) and not (flags & CT.TREE_HITTEST_ONITEMLABEL):
            self.hitCheckbox = True
        else:
            self.hitCheckbox = False
        event.Skip()
        
    def OnChangingSel(self, event):
        """!Selection is changing.

        If the user is clicking on checkbox, selection change is vetoed.
        """
        if self.hitCheckbox:
            event.Veto()

    def OnChangeSel(self, event):
        """!Selection changed

        Preconditions:
            event.GetItem() is a valid layer;
            self.layer_selected is a valid layer
        """
        layer = event.GetItem()
        digitToolbar = self.mapdisplay.GetToolbar('vdigit')
        if digitToolbar:
            mapLayer = self.GetLayerInfo(layer, key = 'maplayer')
            bgmap = UserSettings.Get(group = 'vdigit', key = 'bgmap', subkey = 'value',
                                     internal = True)
            
            if digitToolbar.GetLayer() == mapLayer:
                self._setGradient('vdigit')
            elif bgmap == mapLayer.GetName():
                self._setGradient('bgmap')
            else:
                self._setGradient()
        else:
            self._setGradient()

        self.RefreshLine(layer)
        
        # update statusbar -> show command string
        if self.GetLayerInfo(layer, key = 'maplayer'):
            cmd = self.GetLayerInfo(layer, key = 'maplayer').GetCmd(string = True)
            if len(cmd) > 0:
                self.lmgr.SetStatusText(cmd)
        
        # set region if auto-zooming is enabled
        if self.GetLayerInfo(layer, key = 'cmd') and \
               UserSettings.Get(group = 'display', key = 'autoZooming', subkey = 'enabled'):
            mapLayer = self.GetLayerInfo(layer, key = 'maplayer')
            if mapLayer.GetType() in ('raster', 'vector'):
                render = self.mapdisplay.IsAutoRendered()
                self.mapdisplay.MapWindow.ZoomToMap(layers = [mapLayer,],
                                                    render = render)
        
        # update nviz tools
        if self.lmgr.IsPaneShown('toolbarNviz'):
            if self.layer_selected.IsChecked():
                # update Nviz tool window
                type = self.GetLayerInfo(self.layer_selected, key = 'maplayer').type
                
                if type == 'raster':
                    self.lmgr.nviz.UpdatePage('surface')
                    self.lmgr.nviz.SetPage('surface')
                elif type == 'vector':
                    self.lmgr.nviz.UpdatePage('vector')
                    self.lmgr.nviz.SetPage('vector')
                elif type == '3d-raster':
                    self.lmgr.nviz.UpdatePage('volume')
                    self.lmgr.nviz.SetPage('volume')

    def OnEndDrag(self, event):
        self.StopDragging()
        dropTarget = event.GetItem()
        self.flag = self.HitTest(event.GetPoint())[1]
        if self.lmgr.IsPaneShown('toolbarNviz'):
            self.mapdisplay.MapWindow.UnloadDataLayers(True)
        if self.IsValidDropTarget(dropTarget):
            self.UnselectAll()
            if dropTarget != None:
                self.SelectItem(dropTarget)
            self.OnDrop(dropTarget, self._dragItem)
        elif dropTarget == None:
            self.OnDrop(dropTarget, self._dragItem)
            
    def OnDrop(self, dropTarget, dragItem):
        # save everthing associated with item to drag
        try:
            old = dragItem  # make sure this member exists
        except:
            return

        Debug.msg (4, "LayerTree.OnDrop(): layer=%s" % \
                   (self.GetItemText(dragItem)))
        
        # recreate data layer, insert copy of layer in new position, and delete original at old position
        newItem  = self.RecreateItem (dragItem, dropTarget)

        # if recreated layer is a group, also recreate its children
        if  self.GetLayerInfo(newItem, key = 'type') == 'group':
            (child, cookie) = self.GetFirstChild(dragItem)
            if child:
                while child:
                    self.RecreateItem(child, dropTarget, parent = newItem)
                    self.Delete(child)
                    child = self.GetNextChild(old, cookie)[0]
        
        # delete layer at original position
        try:
            self.Delete(old) # entry in render.Map layers list automatically deleted by OnDeleteLayer handler
        except AttributeError:
            pass

        # redraw map if auto-rendering is enabled
        self.rerender = True
        self.Map.SetLayers(self.GetVisibleLayers())
        
        # select new item
        self.SelectItem(newItem)
        
    def RecreateItem (self, dragItem, dropTarget, parent = None):
        """!Recreate item (needed for OnEndDrag())
        """
        Debug.msg (4, "LayerTree.RecreateItem(): layer=%s" % \
                   self.GetItemText(dragItem))

        # fetch data (dragItem)
        checked = self.IsItemChecked(dragItem)
        image   = self.GetItemImage(dragItem, 0)
        text    = self.GetItemText(dragItem)
        if self.GetLayerInfo(dragItem, key = 'type') == 'command':
            # recreate command layer
            newctrl = self._createCommandCtrl()
            try:
                newctrl.SetValue(self.GetLayerInfo(dragItem, key = 'maplayer').GetCmd(string = True))
            except:
                pass
            newctrl.Bind(wx.EVT_TEXT_ENTER, self.OnCmdChanged)
            data = self.GetPyData(dragItem)

        elif self.GetLayerInfo(dragItem, key = 'ctrl'):
            # recreate data layer
            btnbmp = LMIcons["layerOptions"].GetBitmap((16,16))
            newctrl = buttons.GenBitmapButton(self, id = wx.ID_ANY, bitmap = btnbmp, size = (24, 24))
            newctrl.SetToolTipString(_("Click to edit layer settings"))
            self.Bind(wx.EVT_BUTTON, self.OnLayerContextMenu, newctrl)
            data = self.GetPyData(dragItem)
        

        elif self.GetLayerInfo(dragItem, key = 'type') == 'group':
            # recreate group
            newctrl = None
            data    = None
            
        # decide where to put recreated item
        if dropTarget != None and dropTarget != self.GetRootItem():
            if parent:
                # new item is a group
                afteritem = parent
            else:
                # new item is a single layer
                afteritem = dropTarget

            # dragItem dropped on group
            if  self.GetLayerInfo(afteritem, key = 'type') == 'group':
                newItem = self.PrependItem(afteritem, text = text, \
                                      ct_type = 1, wnd = newctrl, image = image, \
                                      data = data)
                self.Expand(afteritem)
            else:
                #dragItem dropped on single layer
                newparent = self.GetItemParent(afteritem)
                newItem = self.InsertItem(newparent, self.GetPrevSibling(afteritem), \
                                       text = text, ct_type = 1, wnd = newctrl, \
                                       image = image, data = data)
        else:
            # if dragItem not dropped on a layer or group, append or prepend it to the layer tree
            if self.flag & wx.TREE_HITTEST_ABOVE:
                newItem = self.PrependItem(self.root, text = text, \
                                      ct_type = 1, wnd = newctrl, image = image, \
                                      data = data)
            elif (self.flag &  wx.TREE_HITTEST_BELOW) or (self.flag & wx.TREE_HITTEST_NOWHERE) \
                     or (self.flag & wx.TREE_HITTEST_TOLEFT) or (self.flag & wx.TREE_HITTEST_TORIGHT):
                newItem = self.AppendItem(self.root, text = text, \
                                      ct_type = 1, wnd = newctrl, image = image, \
                                      data = data)

        #update new layer 
        self.SetPyData(newItem, self.GetPyData(dragItem))
        if newctrl:
            self.SetLayerInfo(newItem, key = 'ctrl', value = newctrl.GetId())
        else:
            self.SetLayerInfo(newItem, key = 'ctrl', value = None)
            
        self.forceCheck = True
        self.CheckItem(newItem, checked = checked) # causes a new render
        
        return newItem

    def _getLayerName(self, item, lname = ''):
        """!Get layer name string

        @param lname optional layer name
        """
        mapLayer = self.GetLayerInfo(item, key = 'maplayer')
        if not mapLayer:
            return lname
        
        if not lname:
            lname  = self.GetLayerInfo(item, key = 'label')
        opacity  = int(mapLayer.GetOpacity(float = True) * 100)
        if not lname:
            dcmd    = self.GetLayerInfo(item, key = 'cmd')
            lname, found = GetLayerNameFromCmd(dcmd, layerType = mapLayer.GetType(),
                                               fullyQualified = True)
            if not found:
                return None
        
        if opacity < 100:
            return lname + ' (%s %d' % (_('opacity:'), opacity) + '%)'
        
        return lname
                
    def GetOptData(self, dcmd, layer, params, propwin):
        """!Process layer data (when changes in properties dialog are applied)
        """
        # set layer text to map name
        if dcmd:
            self.SetLayerInfo(layer, key = 'cmd', value = dcmd)
            mapText  = self._getLayerName(layer)
            mapName, found = GetLayerNameFromCmd(dcmd)
            mapLayer = self.GetLayerInfo(layer, key = 'maplayer')
            self.SetItemText(layer, mapName)
            
            if not mapText or not found:
                propwin.Hide()
                GWarning(parent = self,
                         message = _("Map <%s> not found.") % mapName)
                return
        
        # update layer data
        if params:
            self.SetPyData(layer, (self.GetLayerInfo(layer), params))
        self.SetLayerInfo(layer, key = 'propwin', value = propwin)
        
        # change parameters for item in layers list in render.Map
        self.ChangeLayer(layer)
        
        # set region if auto-zooming is enabled or layer tree contains
        # only one map layer
        if dcmd and (len(self.GetVisibleLayers()) < 2 or \
                         UserSettings.Get(group = 'display', key = 'autoZooming', subkey = 'enabled')):
            mapLayer = self.GetLayerInfo(layer, key = 'maplayer')
            if mapLayer.GetType() in ('raster', 'vector'):
                render = UserSettings.Get(group = 'display', key = 'autoRendering', subkey = 'enabled')
                self.mapdisplay.MapWindow.ZoomToMap(layers = [mapLayer,],
                                                    render = render)
        
        # update nviz session        
        if self.lmgr.IsPaneShown('toolbarNviz') and dcmd:
            mapLayer = self.GetLayerInfo(layer, key = 'maplayer')
            mapWin = self.mapdisplay.MapWindow
            if len(mapLayer.GetCmd()) > 0:
                if mapLayer.type == 'raster':
                    if mapWin.IsLoaded(layer):
                        mapWin.UnloadRaster(layer)
                    
                    mapWin.LoadRaster(layer)
                    
                elif mapLayer.type == '3d-raster':
                    if mapWin.IsLoaded(layer):
                        mapWin.UnloadRaster3d(layer)
                    
                    mapWin.LoadRaster3d(layer)
                    
                elif mapLayer.type == 'vector':
                    if mapWin.IsLoaded(layer):
                        mapWin.UnloadVector(layer)
                    
                    mapWin.LoadVector(layer)

                # reset view when first layer loaded
                nlayers = len(mapWin.Map.GetListOfLayers(ltype = ('raster', '3d-raster', 'vector'),
                                                         active = True))
                if nlayers < 2:
                    mapWin.ResetView()

    def GetVisibleLayers(self):
        # make a list of visible layers
        layers = []

        vislayer = self.GetFirstVisibleItem()

        if not vislayer or self.GetPyData(vislayer) is None:
            return layers

        itemList = ""
        for item in range(self.GetCount()):
            itemList += self.GetItemText(vislayer) + ','
            lType = self.GetLayerInfo(vislayer, key='type')
            if lType and lType != 'group':
                layers.append(self.GetLayerInfo(vislayer, key='maplayer'))

            if not self.GetNextVisible(vislayer):
                break
            else:
                vislayer = self.GetNextVisible(vislayer)

        Debug.msg(5, "LayerTree.GetVisibleLayers(): items=%s" %
                  (reversed(itemList)))

        layers.reverse()
        return layers

    def ChangeLayer(self, item):
        """!Change layer"""
        type = self.GetLayerInfo(item, key = 'type')
        layerName = None
        
        if type == 'command':
            win = self.FindWindowById(self.GetLayerInfo(item, key = 'ctrl'))
            if win.GetValue() != None:
                cmd = win.GetValue().split(';')
                cmdlist = []
                for c in cmd:
                    cmdlist.append(c.split(' '))
                opac = 1.0
                chk = self.IsItemChecked(item)
                hidden = not self.IsVisible(item)
        elif type != 'group':
            if self.GetPyData(item) is not None:
                cmdlist = self.GetLayerInfo(item, key = 'cmd')
                opac = self.GetLayerInfo(item, key = 'maplayer').GetOpacity(float = True)
                chk = self.IsItemChecked(item)
                hidden = not self.IsVisible(item)
                # determine layer name
                layerName, found = GetLayerNameFromCmd(cmdlist, fullyQualified = True)
                if not found:
                    layerName = self.GetItemText(item)
        
        maplayer = self.Map.ChangeLayer(layer = self.GetLayerInfo(item, key = 'maplayer'), type = type,
                                        command = cmdlist, name = layerName,
                                        active = chk, hidden = hidden, opacity = opac, render = False)
        
        self.SetLayerInfo(item, key = 'maplayer', value = maplayer)
        
        # if digitization tool enabled -> update list of available vector map layers
        if self.mapdisplay.GetToolbar('vdigit'):
            self.mapdisplay.GetToolbar('vdigit').UpdateListOfLayers(updateTool = True)

        self.Map.SetLayers(self.GetVisibleLayers())
        # redraw map if auto-rendering is enabled
        self.rerender = True

    def OnCloseWindow(self, event):
        pass
        # self.Map.Clean()

    def FindItemByData(self, key, value):
        """!Find item based on key and value (see PyData[0]).
        
        If key is 'name', finds item(s) of given maplayer name.
        
        @return item instance
        @return None not found
        """
        item = self.GetFirstChild(self.root)[0]
        if key == 'name':
            return self.__FindSubItemByName(item, value)
        else:
            return self.__FindSubItemByData(item, key, value)

    def FindItemByIndex(self, index):
        """!Find item by index (starting at 0)

        @return item instance
        @return None not found
        """
        item = self.GetFirstChild(self.root)[0]
        i = 0
        while item and item.IsOk():
            if i == index:
                return item
            
            item = self.GetNextVisible(item)
            i += 1
        
        return None
    
    def EnableItemType(self, type, enable = True):
        """!Enable/disable items in layer tree"""
        item = self.GetFirstChild(self.root)[0]
        while item and item.IsOk():
            mapLayer = self.GetLayerInfo(item, key = 'maplayer')
            if mapLayer and type == mapLayer.type:
                self.EnableItem(item, enable)
            
            item = self.GetNextSibling(item)
        
    def __FindSubItemByData(self, item, key, value):
        """!Support method for FindItemByData"""
        while item and item.IsOk():
            itemValue = self.GetLayerInfo(item, key = key)
            
            if value == itemValue:
                return item
            if self.GetLayerInfo(item, key = 'type') == 'group':
                subItem = self.GetFirstChild(item)[0]
                found = self.__FindSubItemByData(subItem, key, value)
                if found:
                    return found
            item = self.GetNextSibling(item)

        return None

    def __FindSubItemByName(self, item, value):
        """!Support method for FindItemByData for searching by name"""
        items = []
        while item and item.IsOk():
            try:
                itemLayer = self.GetLayerInfo(item, key = 'maplayer')
            except KeyError:
                return None
            
            if value == itemLayer.GetName():
                items.append(item)
            if self.GetLayerInfo(item, key = 'type') == 'group':
                subItem = self.GetFirstChild(item)[0]
                found = self.__FindSubItemByName(subItem, value)
                if found:
                    items.extend(found)
            item = self.GetNextSibling(item)
        
        if items:
            return items
        return None

    def _createCommandCtrl(self):
        """!Creates text control for command layer"""
        height = 25
        if sys.platform in ('win32', 'darwin'):
            height = 40
        ctrl = wx.TextCtrl(self, id = wx.ID_ANY, value = '',
                               pos = wx.DefaultPosition, size = (self.GetSize()[0]-100, height),
                               style = wx.TE_PROCESS_ENTER | wx.TE_DONTWRAP)
        return ctrl
