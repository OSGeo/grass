"""
@package lmgr.layertree

@brief Utility classes for map layer management.

Classes:
 - layertree::LayerTree

(C) 2007-2015 by the GRASS Development Team

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

try:
    import treemixin
except ImportError:
    from wx.lib.mixins import treemixin

from grass.script import core as grass
from grass.script import vector as gvector
from grass.script import utils as gutils

from core import globalvar
from gui_core.dialogs import SqlQueryFrame, SetOpacityDialog, TextEntryDialog
from gui_core.forms import GUI
from mapdisp.frame import MapFrame
from core.render import Map
from wxplot.histogram import HistogramPlotFrame
from core.utils import GetLayerNameFromCmd, ltype2command
from core.debug import Debug
from core.settings import UserSettings, GetDisplayVectSettings
from vdigit.main import haveVDigit
from core.gcmd import GWarning, GError, RunCommand
from icons.icon import MetaIcon
from web_services.dialogs import SaveWMSLayerDialog
from gui_core.widgets import MapValidator
from gui_core.wrap import Menu, GenBitmapButton, TextCtrl, NewId
from lmgr.giface import LayerManagerGrassInterfaceForMapDisplay
from core.giface import Notification

TREE_ITEM_HEIGHT = 25

LMIcons = {
    'rastImport': MetaIcon(img='layer-import',
                           label=_('Import raster data')),
    'rastLink': MetaIcon(img='layer-import',
                         label=_('Link external raster data')),
    'rastUnpack': MetaIcon(img='layer-import',
                           label=_('Unpack GRASS raster map')),
    'rastOut': MetaIcon(img='layer-export',
                        label=_('Set raster output format')),
    'vectImport': MetaIcon(img='layer-import',
                           label=_('Import vector data')),
    'vectLink': MetaIcon(img='layer-import',
                         label=_('Link external vector data')),
    'vectUnpack': MetaIcon(img='layer-import',
                           label=_('Unpack GRASS vector map')),
    'vectOut': MetaIcon(img='layer-export',
                        label=_('Set vector output format')),
    'wmsImport': MetaIcon(img='layer-wms-add',
                          label=_('Import data from WMS server')),
    'layerCmd': MetaIcon(img='layer-command-add',
                         label=_('Add command layer')),
    'quit': MetaIcon(img='quit',
                     label=_('Quit')),
    'layerRaster': MetaIcon(img='raster',
                            label=_('Add raster map layer')),
    'layerRgb': MetaIcon(img='rgb',
                         label=_('Add RGB map layer')),
    'layerHis': MetaIcon(img='his',
                         label=_('Add HIS map layer')),
    'layerShaded': MetaIcon(img='shaded-relief',
                            label=_('Add shaded relief map layer')),
    'layerRastarrow': MetaIcon(img='aspect-arrow',
                               label=_('Add raster flow arrows')),
    'layerRastnum': MetaIcon(img='cell-cats',
                             label=_('Add raster cell numbers')),
    'layerVector': MetaIcon(img='vector',
                            label=_('Add vector map layer')),
    'layerThememap': MetaIcon(img='vector-thematic',
                              label=_('Add thematic (choropleth) vector map layer')),
    'layerThemechart': MetaIcon(img='vector-chart',
                                label=_('Add thematic chart layer')),
    'layerGrid': MetaIcon(img='layer-grid-add',
                          label=_('Add grid layer')),
    'layerGeodesic': MetaIcon(img='shortest-distance',
                              label=_('Add geodesic line layer')),
    'layerRhumb': MetaIcon(img='shortest-distance',
                           label=_('Add rhumbline layer')),
    'layerLabels': MetaIcon(img='label',
                            label=_('Add labels')),
    'layerRaster_3d': MetaIcon(img='raster3d',
                               label=_('Add 3D raster map layer'),
                               desc=_('Note that 3D raster data are rendered only in 3D view mode')),
    'layerWms': MetaIcon(img='wms',
                         label=_('Add WMS layer.')),
    'layerOptions': MetaIcon(img='options',
                             label=_('Set options')),
    'layerEdited': MetaIcon(img='layer-edit',
                            label=_("Editing mode")),
    'layerBgmap': MetaIcon(img='layer-bottom',
                           label=_("Background vector map"))
}


class LayerTree(treemixin.DragAndDrop, CT.CustomTreeCtrl):
    """Creates layer tree structure
    """

    def __init__(self, parent, giface,
                 id=wx.ID_ANY, style=wx.SUNKEN_BORDER,
                 ctstyle=CT.TR_HAS_BUTTONS | CT.TR_HAS_VARIABLE_ROW_HEIGHT |
                 CT.TR_HIDE_ROOT | CT.TR_ROW_LINES | CT.TR_FULL_ROW_HIGHLIGHT |
                 CT.TR_MULTIPLE, **kwargs):

        if 'style' in kwargs:
            ctstyle |= kwargs['style']
            del kwargs['style']
        self.displayIndex = kwargs['idx']
        del kwargs['idx']
        self.lmgr = kwargs['lmgr']
        del kwargs['lmgr']
        # GIS Manager notebook for layer tree
        self.notebook = kwargs['notebook']
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
        # if cursor points at layer checkbox (to cancel selection changes)
        self.hitCheckbox = False
        self.forceCheck = False              # force check layer if CheckItem is called
        # forms default to centering on screen, this will put on lmgr
        self.centreFromsOnParent = True

        try:
            ctstyle |= CT.TR_ALIGN_WINDOWS
        except AttributeError:
            pass

        if globalvar.hasAgw:
            super(
                LayerTree,
                self).__init__(
                parent,
                id,
                agwStyle=ctstyle,
                **kwargs)
        else:
            super(
                LayerTree,
                self).__init__(
                parent,
                id,
                style=ctstyle,
                **kwargs)
        self.SetName("LayerTree")
        self.SetBackgroundColour("white")

        # SetAutoLayout() causes that no vertical scrollbar is displayed
        # when some layers are not visible in layer tree
        # self.SetAutoLayout(True)
        self.SetGradientStyle(1)
        self.EnableSelectionGradient(True)
        self._setGradient()

        # init associated map display
        pos = wx.Point(
            (self.displayIndex + 1) * 25,
            (self.displayIndex + 1) * 25)
        self._gifaceForDisplay = LayerManagerGrassInterfaceForMapDisplay(
            self._giface, self)
        self.mapdisplay = MapFrame(self, giface=self._gifaceForDisplay,
                                   id=wx.ID_ANY, pos=pos,
                                   size=globalvar.MAP_WINDOW_SIZE,
                                   style=wx.DEFAULT_FRAME_STYLE,
                                   tree=self, notebook=self.notebook,
                                   lmgr=self.lmgr, page=self.treepg,
                                   Map=self.Map)

        # here (with initial auto-generated names) we use just the
        # number, not the whole name for simplicity
        self.mapdisplay.SetTitleWithName(str(self.displayIndex + 1))

        # show new display
        if showMapDisplay is True:
            self.mapdisplay.Show()
            self.mapdisplay.Refresh()
            self.mapdisplay.Update()

        self.root = self.AddRoot(_("Map Layers"))
        self.SetPyData(self.root, (None, None))

        # create image list to use with layer tree
        self.bmpsize = (16, 16)
        il = wx.ImageList(16, 16, mask=False)
        trart = wx.ArtProvider.GetBitmap(
            wx.ART_FILE_OPEN, wx.ART_OTHER, (16, 16))
        self.folder_open = il.Add(trart)
        trart = wx.ArtProvider.GetBitmap(wx.ART_FOLDER, wx.ART_OTHER, (16, 16))
        self.folder = il.Add(trart)
        self._setIcons(il)
        self.AssignImageList(il)

        self.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.OnActivateLayer)
        self.Bind(wx.EVT_TREE_SEL_CHANGED, self.OnChangeSel)
        self.Bind(wx.EVT_TREE_SEL_CHANGING, self.OnChangingSel)
        self.Bind(CT.EVT_TREE_ITEM_CHECKED, self.OnLayerChecked)
        self.Bind(CT.EVT_TREE_ITEM_CHECKING, self.OnLayerChecking)
        self.Bind(wx.EVT_TREE_DELETE_ITEM, self.OnDeleteLayer)
        self.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, self.OnLayerContextMenu)
        self.Bind(wx.EVT_TREE_END_DRAG, self.OnEndDrag)
        self.Bind(wx.EVT_TREE_END_LABEL_EDIT, self.OnRenamed)
        self.Bind(wx.EVT_KEY_UP, self.OnKeyUp)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        self.Bind(wx.EVT_IDLE, self.OnIdle)
        self.Bind(wx.EVT_MOTION, self.OnMotion)

    def _setIcons(self, il):
        self._icon = {}
        for iconName in ("layerRaster", "layerRaster_3d", "layerRgb",
                         "layerHis", "layerShaded", "layerRastarrow",
                         "layerRastnum", "layerVector", "layerThememap",
                         "layerThemechart", "layerGrid", "layerGeodesic",
                         "layerRhumb", "layerLabels", "layerCmd",
                         "layerWms", "layerEdited", "layerBgmap"):
            iconKey = iconName[len("layer"):].lower()
            icon = LMIcons[iconName].GetBitmap(self.bmpsize)
            self._icon[iconKey] = il.Add(icon)

    def _getSelectedLayer(self):
        """Get selected layer.

        :return: None if no layer selected
        :return: first layer (GenericTreeItem instance) of all selected
        """
        return self.GetSelectedLayer(multi=False, checkedOnly=False)

    # for compatibility
    layer_selected = property(fget=_getSelectedLayer)

    def GetSelectedLayers(self, checkedOnly=False):
        """Get selected layers as a list.

        .. todo::
            somewhere we have checkedOnly default True and elsewhere False
        """
        return self.GetSelectedLayer(multi=True, checkedOnly=checkedOnly)

    def GetSelectedLayer(self, multi=False, checkedOnly=False):
        """Get selected layer from layer tree.

        :param bool multi: return multiple selection as a list
        :param bool checkedOnly: return only the checked layers

        :return: None or [] for multi == True if no layer selected
        :return: first layer (GenericTreeItem instance) of all selected or a list
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

    def GetNextItem(self, item):
        """!Returns next item from tree (flattened expanded tree)"""
        # this is a not empty group
        if self.GetChildrenCount(item):
            return self.GetFirstChild(item)[0]
        # this is a layer outside group
        if self.GetItemParent(item) == self.root:
            return self.GetNextSibling(item)

        # this is a layer inside group
        sibling = self.GetNextSibling(item)
        if sibling:
            # this is a layer inside group
            return sibling
        # skip one up the hierarchy
        return self.GetNextSibling(self.GetItemParent(item))

    def SetItemIcon(self, item, iconName=None):
        if not iconName:
            iconName = self.GetLayerInfo(item, key='maplayer').GetType()
        self.SetItemImage(item, self._icon[iconName])

    def _setGradient(self, iType=None):
        """Set gradient for items

        :param iType: bgmap, vdigit or None
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

        # else: the tree is empty, so no selections

        return array

    def GetMap(self):
        """Get map instace"""
        return self.Map

    def GetMapDisplay(self):
        """Get associated MapFrame"""
        return self.mapdisplay

    def GetLayerInfo(self, layer, key=None):
        """Get layer info.

        :param layer: GenericTreeItem instance
        :param key: cmd, type, ctrl, label, maplayer, propwin, vdigit, nviz
         (vdigit, nviz for map layers only)
        """
        if not self.GetPyData(layer):
            return None
        if key:
            return self.GetPyData(layer)[0][key]
        return self.GetPyData(layer)[0]

    def SetLayerInfo(self, layer, key, value):
        """Set layer info.

        :param layer: GenericTreeItem instance
        :param key: cmd, type, ctrl, label, maplayer, propwin, vdigit, nviz
         (vdigit, nviz for map layers only)
        :param value: value
        """
        info = self.GetPyData(layer)[0]
        info[key] = value

    def GetLayerParams(self, layer):
        """Get layer command params"""
        return self.GetPyData(layer)[1]

    def OnIdle(self, event):
        """Only re-order and re-render a composite map image from GRASS during
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
                self.mapdisplay.GetMapWindow().UpdateMap(render=False)

        event.Skip()

    def OnKeyUp(self, event):
        """Key pressed"""
        key = event.GetKeyCode()

        if key == wx.WXK_DELETE and self.lmgr and \
                not self.GetEditControl():
            self.lmgr.OnDeleteLayer(None)

        event.Skip()

    def OnKeyDown(self, event):
        """Skip event, otherwise causing error when layertree is empty"""
        event.Skip()

    def OnLayerContextMenuButton(self, event):
        """Contextual menu for item/layer when button pressed"""
        # determine which tree item has the button
        button = event.GetEventObject()
        layer = self.FindItemByWindow(button)
        if layer:
            # select the layer in the same way as right click
            if not self.IsSelected(layer):
                self.DoSelectItem(layer, True, False)
            # CallAfter to allow context button events to finish
            # before destroying it when layer is deleted (mac specific)
            wx.CallAfter(self.OnLayerContextMenu, event)

    def OnLayerContextMenu(self, event):
        """Contextual menu for item/layer"""
        if not self.layer_selected:
            event.Skip()
            return

        ltype = self.GetLayerInfo(self.layer_selected, key='type')
        mapLayer = self.GetLayerInfo(self.layer_selected, key='maplayer')

        Debug.msg(4, "LayerTree.OnContextMenu: layertype=%s" %
                  ltype)

        if not hasattr(self, "popupID"):
            self.popupID = dict()
            for key in (
                    'remove', 'rename', 'opacity', 'nviz', 'zoom', 'region', 'align',
                    'export', 'attr', 'edit', 'save_ws', 'bgmap', 'topo', 'meta',
                    'null', 'zoom1', 'color', 'colori', 'hist', 'univar', 'prof',
                    'properties', 'sql', 'copy', 'report', 'export-pg',
                    'export-attr', 'pack'):
                self.popupID[key] = NewId()

        # get current mapset
        currentMapset = grass.gisenv()['MAPSET']

        self.popupMenu = Menu()

        numSelected = len(self.GetSelections())

        item = wx.MenuItem(
            self.popupMenu,
            id=self.popupID['remove'],
            text=_("Remove"))
        item.SetBitmap(MetaIcon(img='layer-remove').GetBitmap(self.bmpsize))
        self.popupMenu.AppendItem(item)
        self.Bind(
            wx.EVT_MENU,
            self.lmgr.OnDeleteLayer,
            id=self.popupID['remove'])

        if ltype != "command" and numSelected == 1:
            self.popupMenu.Append(self.popupID['rename'], _("Rename"))
            self.Bind(
                wx.EVT_MENU,
                self.OnRenameLayer,
                id=self.popupID['rename'])

        # when multiple maps are selected of different types
        # we cannot zoom or change region
        # because g.region can handle only the same type
        same = True
        selected = self.GetSelectedLayers()
        for layer in selected:
            if self.GetLayerInfo(layer, key='type') != ltype:
                same = False
                break

        if ltype not in ("group", "command"):
            if numSelected == 1:
                self.popupMenu.AppendSeparator()
                if ltype != 'raster_3d':
                    item = wx.MenuItem(
                        self.popupMenu,
                        id=self.popupID['opacity'],
                        text=_("Change opacity level"))
                    item.SetBitmap(
                        MetaIcon(img='layer-opacity').GetBitmap(self.bmpsize))
                    self.popupMenu.AppendItem(item)
                    self.Bind(
                        wx.EVT_MENU,
                        self.OnPopupOpacityLevel,
                        id=self.popupID['opacity'])
                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['properties'],
                    text=_("Properties"))
                item.SetBitmap(MetaIcon(img='options').GetBitmap(self.bmpsize))
                self.popupMenu.AppendItem(item)
                self.Bind(
                    wx.EVT_MENU,
                    self.OnPopupProperties,
                    id=self.popupID['properties'])

                if ltype in ('raster', 'vector',
                             'raster_3d') and self.mapdisplay.IsPaneShown('3d'):
                    self.popupMenu.Append(
                        self.popupID['nviz'],
                        _("3D view properties"))
                    self.Bind(
                        wx.EVT_MENU,
                        self.OnNvizProperties,
                        id=self.popupID['nviz'])

            if same and ltype in ('raster', 'vector', 'rgb', 'raster_3d'):
                self.popupMenu.AppendSeparator()
                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['zoom'],
                    text=_("Zoom to selected map(s)"))
                item.SetBitmap(
                    MetaIcon(img='zoom-layer').GetBitmap(self.bmpsize))
                self.popupMenu.AppendItem(item)
                self.Bind(
                    wx.EVT_MENU,
                    self.mapdisplay.OnZoomToMap,
                    id=self.popupID['zoom'])

                # raster-specific zoom
                if ltype and ltype == "raster" and same:
                    self.popupMenu.Append(
                        self.popupID['zoom1'], _("Zoom to selected map(s) (ignore NULLs)"))
                    self.Bind(
                        wx.EVT_MENU,
                        self.mapdisplay.OnZoomToRaster,
                        id=self.popupID['zoom1'])

                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['region'],
                    text=_("Set computational region from selected map(s)"))
                item.SetBitmap(MetaIcon(img='region').GetBitmap(self.bmpsize))
                self.popupMenu.AppendItem(item)
                self.Bind(
                    wx.EVT_MENU,
                    self.OnSetCompRegFromMap,
                    id=self.popupID['region'])

                # raster align
                if ltype and ltype == "raster" and len(selected) == 1:
                    item = wx.MenuItem(
                        self.popupMenu,
                        id=self.popupID['align'],
                        text=_("Align computational region to selected map"))
                    item.SetBitmap(MetaIcon(img='region').GetBitmap(self.bmpsize))
                    self.popupMenu.AppendItem(item)
                    self.Bind(
                        wx.EVT_MENU,
                        self.OnAlignCompRegToRaster,
                        id=self.popupID['align'])

        # vector layers (specific items)
        if ltype and ltype == "vector" and numSelected == 1:
            self.popupMenu.AppendSeparator()
            item = wx.MenuItem(
                self.popupMenu,
                id=self.popupID['export'],
                text=_("Export common formats"))
            item.SetBitmap(
                MetaIcon(img='layer-export').GetBitmap(self.bmpsize))
            self.popupMenu.AppendItem(item)
            self.Bind(
                wx.EVT_MENU,
                lambda x: self.lmgr.OnMenuCmd(
                    cmd=[
                        'v.out.ogr',
                        'input=%s' %
                        mapLayer.GetName()]),
                id=self.popupID['export'])
            if 'v.out.ogr' not in globalvar.grassCmd:
                self.popupMenu.Enable(self.popupID['export'], False)

            self.popupMenu.Append(
                self.popupID['export-pg'], _("Export PostGIS"))
            self.Bind(
                wx.EVT_MENU,
                lambda x: self.lmgr.OnMenuCmd(
                    cmd=[
                        'v.out.postgis',
                        'input=%s' %
                        mapLayer.GetName()]),
                id=self.popupID['export-pg'])
            if 'v.out.postgis' not in globalvar.grassCmd:
                self.popupMenu.Enable(self.popupID['export-pg'], False)

            self.popupMenu.Append(
                self.popupID['export-attr'], _("Export attribute table"))
            self.Bind(
                wx.EVT_MENU,
                lambda x: self.lmgr.OnMenuCmd(
                    cmd=[
                        'v.db.select',
                        'map=%s' %
                        mapLayer.GetName()]),
                id=self.popupID['export-attr'])
            if 'v.db.select' not in globalvar.grassCmd:
                self.popupMenu.Enable(self.popupID['export-attr'], False)

            item = wx.MenuItem(
                self.popupMenu,
                id=self.popupID['pack'],
                text=_("Create pack"))
            self.popupMenu.AppendItem(item)
            self.Bind(
                wx.EVT_MENU,
                lambda x: self.lmgr.OnMenuCmd(
                    cmd=[
                        'v.pack',
                        'input=%s' %
                        mapLayer.GetName()]),
                id=self.popupID['pack'])

            lmapset = self.GetLayerInfo(
                self.layer_selected, key='maplayer').GetMapset()
            if lmapset != currentMapset:
                self.popupMenu.Append(
                    self.popupID['copy'], _("Make a copy in the current mapset"))
                self.Bind(wx.EVT_MENU, self.OnCopyMap, id=self.popupID['copy'])

            self.popupMenu.AppendSeparator()

            self.popupMenu.Append(self.popupID['color'], _("Set color table"))
            self.Bind(
                wx.EVT_MENU,
                self.OnVectorColorTable,
                id=self.popupID['color'])

            self.popupMenu.Append(
                self.popupID['colori'],
                _("Set color table interactively"))
            self.Bind(
                wx.EVT_MENU,
                self.lmgr.OnVectorRules,
                id=self.popupID['colori'])

            item = wx.MenuItem(
                self.popupMenu,
                id=self.popupID['attr'],
                text=_("Show attribute data"))
            item.SetBitmap(MetaIcon(img='table').GetBitmap(self.bmpsize))
            self.popupMenu.AppendItem(item)
            self.Bind(
                wx.EVT_MENU,
                self.lmgr.OnShowAttributeTable,
                id=self.popupID['attr'])

            digitToolbar = self.mapdisplay.GetToolbar('vdigit')
            if digitToolbar:
                vdigitLayer = digitToolbar.GetLayer()
            else:
                vdigitLayer = None
            layer = self.GetLayerInfo(self.layer_selected, key='maplayer')
            if vdigitLayer is not layer:
                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['edit'],
                    text=_("Start editing"))
                self.Bind(
                    wx.EVT_MENU,
                    self.OnStartEditing,
                    id=self.popupID['edit'])
            else:
                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['edit'],
                    text=_("Stop editing"))
                self.Bind(
                    wx.EVT_MENU,
                    self.OnStopEditing,
                    id=self.popupID['edit'])
            item.SetBitmap(MetaIcon(img='edit').GetBitmap(self.bmpsize))
            self.popupMenu.AppendItem(item)

            # removed from layer tree
            #  if digitToolbar:
            # background vector map
            # self.popupMenu.Append(self.popupID['bgmap'],
            #                       text = _("Use as background vector map for digitizer"),
            #                       kind = wx.ITEM_CHECK)
            # self.Bind(wx.EVT_MENU, self.OnSetBgMap, id = self.popupID['bgmap'])
            # if UserSettings.Get(group = 'vdigit', key = 'bgmap', subkey = 'value',
            #                     internal = True) == layer.GetName():
            #     self.popupMenu.Check(self.popupID['bgmap'], True)

            self.popupMenu.Append(
                self.popupID['topo'], _("Rebuild topology"))
            self.Bind(wx.EVT_MENU, self.OnTopology, id=self.popupID['topo'])

            # determine format
            # if layer and layer.GetType() == 'vector':
            #     if 'info' not in self.GetLayerInfo(self.layer_selected):
            #         info = grass.parse_command('v.info',
            #                                    flags = 'e',
            #                                    map = layer.GetName())
            #         self.SetLayerInfo(self.layer_selected, key = 'info', value = info)
            #     info = self.GetLayerInfo(self.layer_selected, key = 'info')
            #     if info and info['format'] != 'native' and \
            #             info['format'].split(',')[1] == 'PostgreSQL':
            #         self.popupMenu.Append(self.popupID['sql'], text = _("SQL Spatial Query"))
            #         self.Bind(wx.EVT_MENU, self.OnSqlQuery, id = self.popupID['sql'])

            if layer.GetMapset() != currentMapset:
                # only vector map in current mapset can be edited
                self.popupMenu.Enable(self.popupID['edit'], False)
                self.popupMenu.Enable(self.popupID['topo'], False)
            elif digitToolbar and digitToolbar.GetLayer():
                # vector map already edited
                vdigitLayer = digitToolbar.GetLayer()
                if vdigitLayer is layer:
                    self.popupMenu.Enable(self.popupID['remove'], False)
                    # self.popupMenu.Enable(self.popupID['bgmap'],  False)
                    self.popupMenu.Enable(self.popupID['topo'], False)
                # else:
                ###    self.popupMenu.Enable(self.popupID['bgmap'], True)

            item = wx.MenuItem(
                self.popupMenu,
                id=self.popupID['meta'],
                text=_("Metadata"))
            item.SetBitmap(MetaIcon(img='layer-info').GetBitmap(self.bmpsize))
            self.popupMenu.AppendItem(item)
            self.Bind(wx.EVT_MENU, self.OnMetadata, id=self.popupID['meta'])

        # raster layers (specific items)
        elif same and ltype and ltype == "raster":
            self.popupMenu.AppendSeparator()

            if numSelected == 1:
                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['export'],
                    text=_("Export"))
                item.SetBitmap(
                    MetaIcon(img='layer-export').GetBitmap(self.bmpsize))
                self.popupMenu.AppendItem(item)
                self.Bind(
                    wx.EVT_MENU,
                    lambda x: self.lmgr.OnMenuCmd(
                        cmd=[
                            'r.out.gdal',
                            'input=%s' %
                            mapLayer.GetName()]),
                    id=self.popupID['export'])

                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['pack'],
                    text=_("Create pack"))
                self.popupMenu.AppendItem(item)
                self.Bind(
                    wx.EVT_MENU,
                    lambda x: self.lmgr.OnMenuCmd(
                        cmd=[
                            'r.pack',
                            'input=%s' %
                            mapLayer.GetName()]),
                    id=self.popupID['pack'])

                lmapset = self.GetLayerInfo(
                    self.layer_selected, key='maplayer').GetMapset()
                if lmapset != currentMapset:
                    self.popupMenu.Append(
                        self.popupID['copy'], _("Make a copy in the current mapset"))
                    self.Bind(
                        wx.EVT_MENU,
                        self.OnCopyMap,
                        id=self.popupID['copy'])

                self.popupMenu.AppendSeparator()

            self.popupMenu.Append(self.popupID['color'], _("Set color table"))
            self.Bind(
                wx.EVT_MENU,
                self.OnRasterColorTable,
                id=self.popupID['color'])
            if len(selected) < 2:
                self.popupMenu.Append(
                    self.popupID['colori'],
                    _("Set color table interactively"))
                self.Bind(
                    wx.EVT_MENU,
                    self.lmgr.OnRasterRules,
                    id=self.popupID['colori'])

            item = wx.MenuItem(
                self.popupMenu,
                id=self.popupID['hist'],
                text=_("Histogram"))
            item.SetBitmap(
                MetaIcon(img='layer-raster-histogram').GetBitmap(self.bmpsize))
            self.popupMenu.AppendItem(item)
            self.Bind(wx.EVT_MENU, self.OnHistogram, id=self.popupID['hist'])

            item = wx.MenuItem(
                self.popupMenu,
                id=self.popupID['univar'],
                text=_("Univariate raster statistics"))
            item.SetBitmap(
                MetaIcon(img='raster-stats').GetBitmap(self.bmpsize))
            self.popupMenu.AppendItem(item)
            self.Bind(
                wx.EVT_MENU,
                self.OnUnivariateStats,
                id=self.popupID['univar'])

            item = wx.MenuItem(
                self.popupMenu,
                id=self.popupID['report'],
                text=_("Report raster statistics"))
            item.SetBitmap(MetaIcon(img='stats').GetBitmap(self.bmpsize))
            self.popupMenu.AppendItem(item)
            self.Bind(
                wx.EVT_MENU,
                self.OnReportStats,
                id=self.popupID['report'])

            if numSelected == 1:
                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['prof'],
                    text=_("Profile"))
                item.SetBitmap(
                    MetaIcon(img='layer-raster-profile').GetBitmap(self.bmpsize))
                self.popupMenu.AppendItem(item)
                self.Bind(wx.EVT_MENU, self.OnProfile, id=self.popupID['prof'])

                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['meta'],
                    text=_("Metadata"))
                item.SetBitmap(
                    MetaIcon(img='layer-info').GetBitmap(self.bmpsize))
                self.popupMenu.AppendItem(item)
                self.Bind(
                    wx.EVT_MENU,
                    self.OnMetadata,
                    id=self.popupID['meta'])

        elif ltype and ltype == 'raster_3d':
            if numSelected == 1:
                self.popupMenu.AppendSeparator()
                self.popupMenu.Append(
                    self.popupID['color'],
                    _("Set color table"))
                self.Bind(
                    wx.EVT_MENU,
                    self.OnRasterColorTable,
                    id=self.popupID['color'])

                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['univar'],
                    text=_("Univariate raster statistics"))
                item.SetBitmap(MetaIcon(img='stats').GetBitmap(self.bmpsize))
                self.popupMenu.AppendItem(item)
                self.Bind(
                    wx.EVT_MENU,
                    self.OnUnivariateStats,
                    id=self.popupID['univar'])

                item = wx.MenuItem(
                    self.popupMenu,
                    id=self.popupID['meta'],
                    text=_("Metadata"))
                item.SetBitmap(
                    MetaIcon(img='layer-info').GetBitmap(self.bmpsize))
                self.popupMenu.AppendItem(item)
                self.Bind(
                    wx.EVT_MENU,
                    self.OnMetadata,
                    id=self.popupID['meta'])

        # web service layers (specific item)
        elif ltype and ltype == "wms":
            self.popupMenu.Append(
                self.popupID['save_ws'], _("Save web service layer"))
            self.Bind(wx.EVT_MENU, self.OnSaveWs, id=self.popupID['save_ws'])

        self.PopupMenu(self.popupMenu)
        self.popupMenu.Destroy()

    def OnSaveWs(self, event):
        """Show dialog for saving web service layer into GRASS vector/raster layer"""
        mapLayer = self.GetLayerInfo(self.layer_selected, key='maplayer')
        dlg = SaveWMSLayerDialog(parent=self, layer=mapLayer,
                                 giface=self._gifaceForDisplay)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnTopology(self, event):
        """Rebuild topology of selected vector map"""
        mapLayer = self.GetLayerInfo(self.layer_selected, key='maplayer')
        cmd = ['v.build',
               'map=%s' % mapLayer.GetName()]
        self._giface.RunCmd(cmd)

    def OnSqlQuery(self, event):
        """Show SQL query window for PostGIS layers
        """
        dlg = SqlQueryFrame(parent=self)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnMetadata(self, event):
        """Print metadata of raster/vector map layer

        .. todo::
            Dialog to modify metadata
        """
        mapLayer = self.GetLayerInfo(self.layer_selected, key='maplayer')
        mltype = self.GetLayerInfo(self.layer_selected, key='type')

        if mltype == 'raster':
            cmd = ['r.info']
        elif mltype == 'vector':
            cmd = ['v.info']
        elif mltype == 'raster_3d':
            cmd = ['r3.info']
        cmd.append('map=%s' % mapLayer.GetName())

        # print output to command log area
        self._giface.RunCmd(cmd)

    def OnSetCompRegFromMap(self, event):
        """Set computational region from selected raster/vector map(s)
        """
        rast = []
        vect = []
        rast3d = []
        for layer in self.GetSelections():
            mapLayer = self.GetLayerInfo(layer, key='maplayer')
            mltype = self.GetLayerInfo(layer, key='type')

            if mltype == 'raster':
                rast.append(mapLayer.GetName())
            elif mltype == 'vector':
                vect.append(mapLayer.GetName())
            elif mltype == 'raster_3d':
                rast3d.append(mapLayer.GetName())
            elif mltype == 'rgb':
                for rname in mapLayer.GetName().splitlines():
                    rast.append(rname)

        kwargs = {}
        if rast:
            kwargs['raster'] = ','.join(rast)
        if vect:
            kwargs['vector'] = ','.join(vect)
        if rast3d:
            kwargs['raster_3d'] = ','.join(rast3d)

        if kwargs:
            if UserSettings.Get(group='general',
                                key='region', subkey=['resAlign', 'enabled']):
                kwargs['flags'] = 'a'
            # command must run in main thread otherwise it can be
            # launched after rendering is done (region extent will
            # remain untouched)
            RunCommand('g.region', **kwargs)

        # re-render map display
        self._giface.GetMapWindow().UpdateMap(render=False)

    def OnAlignCompRegToRaster(self, event):
        """Align computational region to selected raster map
        """
        selected = self.GetSelections()
        if len(selected) != 1 or \
           self.GetLayerInfo(selected[0], key='type') != 'raster':
            return

        kwargs = {'align': self.GetLayerInfo(selected[0],
                                             key='maplayer').GetName()
        }

        if UserSettings.Get(group='general',
                            key='region', subkey=['resAlign', 'enabled']):
            kwargs['flags'] = 'a'
        # command must run in main thread otherwise it can be
        # launched after rendering is done (region extent will
        # remain untouched)
        RunCommand('g.region', **kwargs)

        # re-render map display
        self._giface.GetMapWindow().UpdateMap(render=False)

    def OnProfile(self, event):
        """Plot profile of given raster map layer"""
        mapLayer = self.GetLayerInfo(self.layer_selected, key='maplayer')
        if not mapLayer.GetName():
            wx.MessageBox(
                parent=self,
                message=_(
                    "Unable to create profile of "
                    "raster map."),
                caption=_("Error"),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return
        self.mapdisplay.Profile(rasters=[mapLayer.GetName()])

    def OnRasterColorTable(self, event):
        """Set color table for 2D/3D raster map"""
        raster2d = []
        raster3d = []
        for layer in self.GetSelectedLayers():
            if self.GetLayerInfo(layer, key='type') == 'raster_3d':
                raster3d.append(
                    self.GetLayerInfo(
                        layer, key='maplayer').GetName())
            else:
                raster2d.append(
                    self.GetLayerInfo(
                        layer, key='maplayer').GetName())

        if raster2d:
            GUI(parent=self, giface=self._giface).ParseCommand(
                ['r.colors', 'map=%s' % ','.join(raster2d)])
        if raster3d:
            GUI(parent=self, giface=self._giface).ParseCommand(
                ['r3.colors', 'map=%s' % ','.join(raster3d)])

    def OnVectorColorTable(self, event):
        """Set color table for vector map"""
        name = self.GetLayerInfo(self.layer_selected, key='maplayer').GetName()
        GUI(parent=self, centreOnParent=self.centreFromsOnParent).ParseCommand(
            ['v.colors', 'map=%s' % name])

    def OnCopyMap(self, event):
        """Copy selected map into current mapset"""
        layer = self.GetSelectedLayer()
        ltype = self.GetLayerInfo(layer, key='type')
        lnameSrc = self.GetLayerInfo(layer, key='maplayer').GetName()

        if ltype == 'raster':
            key = 'raster'
            module = 'rast'
            label = _('Raster map')
        elif ltype == 'vector':
            key = 'vector'
            module = 'vect'
            label = _('Vector map')
        elif ltype == 'raster_3d':
            key = 'raster_3d'
            module = 'rast3d'
            label = _('3D raster map')
        else:
            GError(_("Unsupported map type <%s>") % ltype, parent=self)
            return

        # TODO: replace by New[Raster|Vector]Dialog
        dlg = TextEntryDialog(
            parent=self,
            message=_('Enter name for the new %s in the current mapset:') %
            label.lower(),
            caption=_('Make a copy of %s <%s>') % (label.lower(),
                                                   lnameSrc),
            defaultValue=lnameSrc.split('@')[0],
            validator=MapValidator(),
            size=(700, -1))
        if dlg.ShowModal() == wx.ID_OK:
            lnameDst = dlg.GetValue()
            dlg.Destroy()
        else:
            dlg.Destroy()
            return

        currentMapset = grass.gisenv()['MAPSET']
        # check if map already exists
        if lnameDst in grass.list_grouped(key)[currentMapset]:
            dlgOw = wx.MessageDialog(
                parent=self,
                message=_(
                    "%s <%s> already exists "
                    "in the current mapset. "
                    "Do you want to overwrite it?") %
                (label,
                 lnameDst),
                caption=_("Overwrite?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlgOw.ShowModal() != wx.ID_YES:
                return

        kwargs = {key: '%s,%s' % (lnameSrc, lnameDst)}
        if 0 != RunCommand('g.copy', overwrite=True, **kwargs):
            GError(_("Unable to make copy of <%s>") % lnameSrc,
                   parent=self)
            return

        if '@' in lnameDst:
            mapsetDst = lnameDst.split('@')[1]
            if mapsetDst != currentMapset:
                GError(
                    _("Unable to make copy of <%s>. Mapset <%s> is not current mapset.") %
                    (lnameSrc, mapsetDst))
                return

        lnameDst += '@' + currentMapset
        # add copied map to the layer tree
        self.AddLayer(
            ltype,
            lname=lnameDst,
            lcmd=[
                'd.%s' %
                module,
                'map=%s' %
                lnameDst])

    def OnHistogram(self, event):
        """Plot histogram for given raster map layer
        """
        rasterList = []
        for layer in self.GetSelectedLayers():
            rasterList.append(
                self.GetLayerInfo(
                    layer, key='maplayer').GetName())

        if not rasterList:
            GError(parent=self,
                   message=_("Unable to display histogram of "
                             "raster map. No map name defined."))
            return

        win = HistogramPlotFrame(parent=self, giface=self._giface, rasterList=rasterList)
        win.CentreOnScreen()
        win.Show()

    def OnUnivariateStats(self, event):
        """Univariate 2D/3D raster statistics"""
        raster2d = []
        raster3d = []
        for layer in self.GetSelectedLayers():
            if self.GetLayerInfo(layer, key='type') == 'raster_3d':
                raster3d.append(
                    self.GetLayerInfo(
                        layer, key='maplayer').GetName())
            else:
                raster2d.append(
                    self.GetLayerInfo(
                        layer, key='maplayer').GetName())

        if raster2d:
            self._giface.RunCmd(['r.univar', 'map=%s' % ','.join(raster2d)])

        if raster3d:
            self._giface.RunCmd(['r3.univar', 'map=%s' % ','.join(raster3d)])

    def OnReportStats(self, event):
        """Print 2D statistics"""
        rasters = []
        # TODO: Implement self.GetSelectedLayers(ltype='raster')
        for layer in self.GetSelectedLayers():
            if self.GetLayerInfo(layer, key='type') == 'raster':
                rasters.append(
                    self.GetLayerInfo(
                        layer, key='maplayer').GetName())

        if rasters:
            self._giface.RunCmd(['r.report', 'map=%s' %
                                 ','.join(rasters), 'units=h,c,p'])

    def OnStartEditing(self, event):
        """Start editing vector map layer requested by the user
        """
        mapLayer = self.GetLayerInfo(self.layer_selected, key='maplayer')
        if not haveVDigit:
            from vdigit import errorMsg

            self.mapdisplay.toolbars['map'].combo.SetValue(_("2D view"))

            GError(_("Unable to start wxGUI vector digitizer.\n"
                     "Details: %s") % errorMsg, parent=self)
            return

        if not self.mapdisplay.GetToolbar('vdigit'):  # enable tool
            self.mapdisplay.AddToolbar('vdigit')

        else:  # tool already enabled
            pass

        # mark layer as 'edited'
        self.mapdisplay.toolbars['vdigit'].StartEditing(mapLayer)

    def StartEditing(self, layerItem):
        self._setGradient('vdigit')
        if layerItem:
            self.SetItemIcon(layerItem, 'edited')
            self.RefreshLine(layerItem)

    def OnStopEditing(self, event):
        """Stop editing the current vector map layer
        """
        self.mapdisplay.toolbars['vdigit'].OnExit()

    def StopEditing(self, layerItem):
        self._setGradient()
        self.SetItemIcon(layerItem)
        self.RefreshLine(layerItem)

    def SetBgMapForEditing(self, mapName, unset=False):
        try:
            layerItem = self.FindItemByData('name', mapName)[0]
        except IndexError:
            return

        if not unset:
            self._setGradient('bgmap')
            self.SetItemIcon(layerItem, 'bgmap')
        else:
            self._setGradient()
            self.SetItemIcon(layerItem)

        self.RefreshLine(layerItem)

    def OnPopupProperties(self, event):
        """Popup properties dialog"""
        self.PropertiesDialog(self.layer_selected)

    def OnPopupOpacityLevel(self, event):
        """Popup opacity level indicator"""
        if not self.GetLayerInfo(self.layer_selected, key='ctrl'):
            return

        maplayer = self.GetLayerInfo(self.layer_selected, key='maplayer')
        current_opacity = maplayer.GetOpacity()

        dlg = SetOpacityDialog(
            self,
            opacity=current_opacity,
            title=_("Set opacity of <%s>") %
            maplayer.GetName())
        dlg.applyOpacity.connect(
            lambda value: self.ChangeLayerOpacity(
                layer=self.layer_selected, value=value))
        dlg.CentreOnParent()

        if dlg.ShowModal() == wx.ID_OK:
            self.ChangeLayerOpacity(
                layer=self.layer_selected,
                value=dlg.GetOpacity())
        dlg.Destroy()

    def ChangeLayerOpacity(self, layer, value):
        """Change opacity value of layer

        :param layer: layer for which to change (item in layertree)
        :param value: opacity value (float between 0 and 1)
        """
        maplayer = self.GetLayerInfo(layer, key='maplayer')
        self.Map.ChangeOpacity(maplayer, value)
        maplayer.SetOpacity(value)
        self.SetItemText(layer,
                         self._getLayerName(layer))

        # vector layer currently edited
        if self.GetMapDisplay().GetToolbar('vdigit') and self.GetMapDisplay(
        ).GetToolbar('vdigit').GetLayer() == maplayer:
            alpha = int(value * 255)
            self.GetMapDisplay().GetWindow().digit.GetDisplay().UpdateSettings(alpha=alpha)

        # redraw map if auto-rendering is enabled
        renderVector = False
        if self.GetMapDisplay().GetToolbar('vdigit'):
            renderVector = True
        self.GetMapDisplay().GetWindow().UpdateMap(
            render=False, renderVector=renderVector)

    def OnNvizProperties(self, event):
        """Nviz-related properties (raster/vector/volume)

        .. todo::
            vector/volume
        """
        self.lmgr.notebook.SetSelectionByName('nviz')
        ltype = self.GetLayerInfo(self.layer_selected, key='type')
        if ltype == 'raster':
            self.lmgr.nviz.SetPage('surface')
        elif ltype == 'vector':
            self.lmgr.nviz.SetPage('vector')
        elif ltype == 'raster_3d':
            self.lmgr.nviz.SetPage('volume')

    def OnRenameLayer(self, event):
        """Rename layer"""
        self.EditLabel(self.layer_selected)
        self.GetEditControl().SetSelection(-1, -1)

    def OnRenamed(self, event):
        """Layer renamed"""
        if not event.GetLabel():
            event.Skip()
            return

        item = self.layer_selected
        self.SetLayerInfo(item, key='label', value=event.GetLabel())
        self.SetItemText(item, self._getLayerName(item))

        event.Skip()

    def AddLayer(self, ltype, lname=None, lchecked=None, lopacity=1.0,
                 lcmd=None, lgroup=None, lvdigit=None, lnviz=None,
                 multiple=True, loadWorkspace=False):
        """Add new item to the layer tree, create corresponding MapLayer instance.
        Launch property dialog if needed (raster, vector, etc.)

        :param ltype: layer type (raster, vector, raster_3d, ...)
        :param lname: layer name
        :param lchecked: if True layer is checked
        :param lopacity: layer opacity level
        :param lcmd: command (given as a list)
        :param lgroup: index of group item (-1 for root) or None
        :param lvdigit: vector digitizer settings (eg. geometry attributes)
        :param lnviz: layer Nviz properties
        :param bool multiple: True to allow multiple map layers in layer tree
        :param bool loadWorkspace: True if called when loading workspace
        """
        if lname and not multiple:
            # check for duplicates
            item = self.GetFirstChild(self.root)[0]
            while item and item.IsOk():
                if self.GetLayerInfo(item, key='type') == 'vector':
                    name = self.GetLayerInfo(item, key='maplayer').GetName()
                    if name == lname:
                        return
                item = self.GetNextItem(item)

        selectedLayer = self.GetSelectedLayer()
        # deselect active item
        if lchecked != False and selectedLayer:
            self.SelectItem(selectedLayer, select=False)

        Debug.msg(3, "LayerTree().AddLayer(): ltype=%s" % (ltype))

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
            btnbmp = LMIcons["layerOptions"].GetBitmap((16, 16))
            ctrl = GenBitmapButton(
                self, id=wx.ID_ANY, bitmap=btnbmp, size=(24, 24))
            ctrl.SetToolTip(_("Click to edit layer settings"))
            self.Bind(wx.EVT_BUTTON, self.OnLayerContextMenuButton, ctrl)
        # add layer to the layer tree
        if loadWorkspace:
            # when loading workspace, we always append
            if lgroup == -1:
                # -> last child of root (loading from workspace)
                layer = self.AppendItem(parentId=self.root,
                                        text='', ct_type=1, wnd=ctrl)
            elif lgroup > -1:
                # -> last child of group (loading from workspace)
                parent = self.FindItemByIndex(index=lgroup)
                if not parent:
                    parent = self.root
                layer = self.AppendItem(parentId=parent,
                                        text='', ct_type=1, wnd=ctrl)
        else:
            if selectedLayer and selectedLayer != self.GetRootItem():
                if selectedLayer and self.GetLayerInfo(selectedLayer, key='type') == 'group':
                    # add to group (first child of self.layer_selected)
                    layer = self.PrependItem(parent=selectedLayer,
                                             text='', ct_type=1, wnd=ctrl)
                else:
                    # -> previous sibling of selected layer
                    parent = self.GetItemParent(selectedLayer)
                    layer = self.InsertItem(
                                parentId=parent, input=self.GetPrevSibling(selectedLayer),
                                text='', ct_type=1, wnd=ctrl)
            else:  # add first layer to the layer tree (first child of root)
                layer = self.PrependItem(
                    parent=self.root, text='', ct_type=1, wnd=ctrl)

        # layer is initially unchecked as inactive (beside 'command')
        # use predefined value if given
        if lchecked is not None:
            checked = lchecked
            render = True if checked else False
        else:
            checked = False
            render = False

        self.forceCheck = True

        # add text and icons for each layer ltype
        if ltype == 'command':
            self.SetItemImage(layer, self._icon['cmd'])
        elif ltype == 'group':
            self.SetItemImage(layer, self.folder, CT.TreeItemIcon_Normal)
            self.SetItemImage(
                layer,
                self.folder_open,
                CT.TreeItemIcon_Expanded)
            self.SetItemText(layer, grouptext)
        else:
            if ltype in self._icon:
                self.SetItemImage(layer, self._icon[ltype])
                # do not use title() - will not work with ltype == 'raster_3d'
                self.SetItemText(
                    layer,
                    '%s %s' %
                    (LMIcons[
                        "layer" +
                        ltype[0].upper() +
                        ltype[
                            1:]].GetLabel(),
                        _('(double click to set properties)') +
                        ' ' *
                        15))
            else:
                self.SetItemImage(layer, self._icon['cmd'])
                self.SetItemText(layer, ltype)

        if ltype != 'group':
            if lcmd and len(lcmd) > 1:
                cmd = lcmd
                name, found = GetLayerNameFromCmd(lcmd)
            else:
                cmd = []
                if ltype == 'command' and lname:
                    for c in lname.split(';'):
                        cmd.append(gutils.split(c))

                name = None

            if ctrl:
                ctrlId = ctrl.GetId()
            else:
                ctrlId = None

            # add a data object to hold the layer's command (does not
            # apply to generic command layers)
            self.SetPyData(layer, ({'cmd': cmd,
                                    'type': ltype,
                                    'ctrl': ctrlId,
                                    'label': None,
                                    'maplayer': None,
                                    'vdigit': lvdigit,
                                    'nviz': lnviz,
                                    'propwin': None},
                                   None))

            # must be after SetPyData because it calls OnLayerChecked
            # which calls GetVisibleLayers which requires already set PyData
            self.CheckItem(layer, checked=checked)

            # find previous map layer instance
            prevItem = self.GetFirstChild(self.root)[0]
            prevMapLayer = None
            pos = -1
            while prevItem and prevItem.IsOk() and prevItem != layer:
                if self.GetLayerInfo(prevItem, key='maplayer'):
                    prevMapLayer = self.GetLayerInfo(prevItem, key='maplayer')

                prevItem = self.GetNextItem(prevItem)

                if prevMapLayer:
                    pos = self.Map.GetLayerIndex(prevMapLayer)
                else:
                    pos = -1

            maplayer = self.Map.AddLayer(
                pos=pos,
                ltype=ltype,
                command=self.GetLayerInfo(
                    prevItem,
                    key='cmd'),
                name=name,
                active=checked,
                hidden=False,
                opacity=lopacity,
                render=render)
            self.SetLayerInfo(layer, key='maplayer', value=maplayer)

            # run properties dialog if no properties given
            if len(cmd) == 0:
                self.PropertiesDialog(layer, show=True)
            else:
                self.first = False
        else:  # group
            self.SetPyData(layer, ({'cmd': None,
                                    'type': ltype,
                                    'ctrl': None,
                                    'label': None,
                                    'maplayer': None,
                                    'propwin': None},
                                   None))

        # select new item
        if lchecked != False:
            self.SelectItem(layer, select=True)

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

    def DeleteAllLayers(self):
        """Delete all items in the tree"""
        self.DeleteAllItems()
        # add new root element
        self.root = self.AddRoot(_("Map Layers"))
        self.SetPyData(self.root, (None, None))

    def PropertiesDialog(self, layer, show=True):
        """Launch the properties dialog"""
        ltype = self.GetLayerInfo(layer, key='type')
        if 'propwin' in self.GetLayerInfo(layer) and \
                self.GetLayerInfo(layer, key='propwin') is not None:
            # recycle GUI dialogs
            win = self.GetLayerInfo(layer, key='propwin')
            if win.IsShown():
                win.SetFocus()
            else:
                win.Show()
            return

        params = self.GetLayerParams(layer)

        Debug.msg(3, "LayerTree.PropertiesDialog(): ltype=%s" %
                  ltype)

        cmd = None
        if self.GetLayerInfo(layer, key='cmd'):
            module = GUI(parent=self, show=show,
                         centreOnParent=self.centreFromsOnParent)
            module.ParseCommand(self.GetLayerInfo(layer, key='cmd'),
                                completed=(self.GetOptData, layer, params))
            self.SetLayerInfo(layer, key='cmd', value=module.GetCmd())
        elif self.GetLayerInfo(layer, key='type') != 'command':
            cmd = [ltype2command[ltype]]
            if ltype in ('raster', 'rgb'):
                if UserSettings.Get(group='rasterLayer',
                                    key='opaque', subkey='enabled'):
                    cmd.append('-n')
            elif ltype == 'vector':
                cmd += GetDisplayVectSettings()

        if cmd:
            module = GUI(parent=self, centreOnParent=self.centreFromsOnParent)
            module.ParseCommand(cmd,
                                completed=(self.GetOptData, layer, params))

    def OnActivateLayer(self, event):
        """Double click on the layer item.
        Launch property dialog, or expand/collapse group of items, etc.
        """
        self.lmgr.WorkspaceChanged()
        layer = event.GetItem()

        if self.GetLayerInfo(layer, key='type') == 'group':
            if self.IsExpanded(layer):
                self.Collapse(layer)
            else:
                self.Expand(layer)
            return

        self.PropertiesDialog(layer)

    def OnDeleteLayer(self, event):
        """Remove selected layer item from the layer tree"""
        self.lmgr.WorkspaceChanged()
        item = event.GetItem()

        try:
            item.properties.Close(True)
        except:
            pass

        if item != self.root:
            Debug.msg(3, "LayerTree.OnDeleteLayer(): name=%s" %
                      (self.GetItemText(item)))
        else:
            self.root = None

        # unselect item
        self.Unselect()

        try:
            if self.GetLayerInfo(item, key='type') != 'group':
                self.Map.DeleteLayer(self.GetLayerInfo(item, key='maplayer'))
        except:
            pass

        # redraw map if auto-rendering is enabled
        self.rerender = True
        nlayers = self.GetVisibleLayers()
        if not nlayers:
            self.first = True  # layer tree is empty
        self.Map.SetLayers(nlayers)

        if self.mapdisplay.GetToolbar('vdigit'):
            self.mapdisplay.toolbars[
                'vdigit'].UpdateListOfLayers(updateTool=True)

        # here was some dead code related to layer and nviz
        # however, in condition was rerender = False
        # but rerender is alway True
        # (here no change and also in UpdateListOfLayers and GetListOfLayers)
        # You can safely remove this comment after some testing.

        event.Skip()

    def OnLayerChecking(self, event):
        """Layer checkbox is being checked.

        Continue only if mouse is above checkbox or layer was checked programatically.
        """
        if self.hitCheckbox or self.forceCheck:
            self.forceCheck = False
            event.Skip()
        else:
            event.Veto()

    def OnLayerChecked(self, event):
        """Enable/disable data layer"""

        item = event.GetItem()
        checked = item.IsChecked()

        digitToolbar = self.mapdisplay.GetToolbar('vdigit')
        if not self.first:
            # change active parameter for item in layers list in render.Map
            if self.GetLayerInfo(item, key='type') == 'group':
                child, cookie = self.GetFirstChild(item)
                while child:
                    self.forceCheck = True
                    self.CheckItem(child, checked)
                    mapLayer = self.GetLayerInfo(child, key='maplayer')
                    if not digitToolbar or(
                            digitToolbar
                            and digitToolbar.GetLayer() !=
                            mapLayer):
                        # layer is maplayer type
                        if mapLayer:
                            # ignore when map layer is edited
                            self.Map.ChangeLayerActive(mapLayer, checked)
                        self.lmgr.WorkspaceChanged()
                    child = self.GetNextSibling(child)
            else:
                mapLayer = self.GetLayerInfo(item, key='maplayer')
                if mapLayer and (not digitToolbar or (
                        digitToolbar and digitToolbar.GetLayer() != mapLayer)):
                    # ignore when map layer is edited
                    self.Map.ChangeLayerActive(mapLayer, checked)
                    self.lmgr.WorkspaceChanged()

        # nviz
        if self.mapdisplay.IsPaneShown('3d') and \
                self.GetPyData(item) is not None:
            # nviz - load/unload data layer
            mapLayer = self.GetLayerInfo(item, key='maplayer')
            if mapLayer is None:
                return

            self.mapdisplay.SetStatusText(
                _("Please wait, updating data..."), 0)
            self.lmgr.WorkspaceChanged()

            if checked:  # enable
                if mapLayer.type == 'raster':
                    self.mapdisplay.MapWindow.LoadRaster(item)
                elif mapLayer.type == 'raster_3d':
                    self.mapdisplay.MapWindow.LoadRaster3d(item)
                elif mapLayer.type == 'vector':
                    vInfo = gvector.vector_info_topo(mapLayer.GetName())
                    if (vInfo['points'] + vInfo['centroids']) > 0:
                        self.mapdisplay.MapWindow.LoadVector(item, points=True)
                    if (vInfo['lines'] + vInfo['boundaries']) > 0:
                        self.mapdisplay.MapWindow.LoadVector(
                            item, points=False)

            else:  # disable
                if mapLayer.type == 'raster':
                    self.mapdisplay.MapWindow.UnloadRaster(item)
                elif mapLayer.type == 'raster_3d':
                    self.mapdisplay.MapWindow.UnloadRaster3d(item)
                elif mapLayer.type == 'vector':
                    self.mapdisplay.MapWindow.UnloadVector(item)

            self.mapdisplay.SetStatusText("", 0)

        # redraw map if auto-rendering is enabled
        self.rerender = True
        self.Map.SetLayers(self.GetVisibleLayers())

        # if interactive vector feature selection is open -> reset
        vselect = self._giface.GetMapDisplay().GetDialog('vselect')
        if vselect:
            vselect.Reset()

    def OnCmdChanged(self, event):
        """Change command string"""
        ctrl = event.GetEventObject().GetId()

        # find layer tree item by ctrl
        layer = self.GetFirstChild(self.root)[0]
        while layer and layer.IsOk():
            if self.GetLayerInfo(layer, key='ctrl') == ctrl:
                break
            layer = self.GetNextItem(layer)

        # change parameters for item in layers list in render.Map
        self.ChangeLayer(layer)

        event.Skip()

    def OnMotion(self, event):
        """Mouse is moving.

        Detects if mouse points at checkbox.
        """
        thisItem, flags = self.HitTest(event.GetPosition())
        # workaround: in order not to check checkox when clicking outside
        # we need flag TREE_HITTEST_ONITEMCHECKICON but not TREE_HITTEST_ONITEMLABEL
        # this applies only for TR_FULL_ROW_HIGHLIGHT style
        if (flags & CT.TREE_HITTEST_ONITEMCHECKICON) and not (
                flags & CT.TREE_HITTEST_ONITEMLABEL):
            self.hitCheckbox = True
        else:
            self.hitCheckbox = False
        event.Skip()

    def OnChangingSel(self, event):
        """Selection is changing.

        If the user is clicking on checkbox, selection change is vetoed.
        """
        if self.hitCheckbox:
            event.Veto()

    def OnChangeSel(self, event):
        """Selection changed

        Preconditions:
            event.GetItem() is a valid layer;
            self.layer_selected is a valid layer
        """
        # when no layer selected, nothing to do here
        if self.layer_selected is None:
            event.Skip()
            return

        layer = event.GetItem()
        digitToolbar = self.mapdisplay.GetToolbar('vdigit')
        if digitToolbar:
            mapLayer = self.GetLayerInfo(layer, key='maplayer')
            bgmap = UserSettings.Get(
                group='vdigit',
                key='bgmap',
                subkey='value',
                settings_type='internal')

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
        if self.GetLayerInfo(layer, key='maplayer'):
            cmd = self.GetLayerInfo(layer, key='maplayer').GetCmd(string=True)
            if len(cmd) > 0:
                self.lmgr.SetStatusText(cmd)

        # set region if auto-zooming is enabled
        if self.GetLayerInfo(layer, key='cmd') and UserSettings.Get(
                group='display', key='autoZooming', subkey='enabled'):
            mapLayer = self.GetLayerInfo(layer, key='maplayer')
            if mapLayer.GetType() in ('raster', 'vector'):
                render = self.mapdisplay.IsAutoRendered()
                self.mapdisplay.MapWindow.ZoomToMap(layers=[mapLayer, ],
                                                    render=render)

        # update nviz tools
        if self.mapdisplay.IsPaneShown('3d'):
            if self.layer_selected.IsChecked():
                # update Nviz tool window
                type = self.GetLayerInfo(
                    self.layer_selected, key='maplayer').type

                if type == 'raster':
                    self.lmgr.nviz.UpdatePage('surface')
                    self.lmgr.nviz.SetPage('surface')
                elif type == 'vector':
                    self.lmgr.nviz.UpdatePage('vector')
                    self.lmgr.nviz.SetPage('vector')
                elif type == 'raster_3d':
                    self.lmgr.nviz.UpdatePage('volume')
                    self.lmgr.nviz.SetPage('volume')

        # if interactive vector feature selection is open -> reset
        vselect = self._giface.GetMapDisplay().GetDialog('vselect')
        if vselect:
            vselect.Reset()

    def OnEndDrag(self, event):
        self.StopDragging()
        dropTarget = event.GetItem()
        self.flag = self.HitTest(event.GetPoint())[1]
        if self.mapdisplay.IsPaneShown('3d'):
            self.mapdisplay.MapWindow.UnloadDataLayers(True)
        if self.IsValidDropTarget(dropTarget):
            self.UnselectAll()
            if dropTarget is not None:
                self.SelectItem(dropTarget)
            self.OnDrop(dropTarget, self._dragItem)
        elif dropTarget is None:
            self.OnDrop(dropTarget, self._dragItem)

    def OnDrop(self, dropTarget, dragItem):
        # save everthing associated with item to drag
        try:
            old = dragItem  # make sure this member exists
        except:
            return

        Debug.msg(4, "LayerTree.OnDrop(): layer=%s" %
                  (self.GetItemText(dragItem)))

        # recreate data layer, insert copy of layer in new position, and delete
        # original at old position
        newItem = self.RecreateItem(dragItem, dropTarget)

        # if recreated layer is a group, also recreate its children
        if self.GetLayerInfo(newItem, key='type') == 'group':
            (child, cookie) = self.GetFirstChild(dragItem)
            if child:
                while child:
                    self.RecreateItem(child, dropTarget, parent=newItem)
                    child, cookie = self.GetNextChild(old, cookie)

        # delete layer at original position
        try:
            # entry in render.Map layers list automatically deleted by
            # OnDeleteLayer handler
            self.Delete(old)
        except AttributeError:
            pass

        # redraw map if auto-rendering is enabled
        self.rerender = True
        self.Map.SetLayers(self.GetVisibleLayers())

        # select new item
        self.SelectItem(newItem)

    def RecreateItem(self, dragItem, dropTarget, parent=None):
        """Recreate item (needed for OnEndDrag())
        """
        Debug.msg(4, "LayerTree.RecreateItem(): layer=%s" %
                  self.GetItemText(dragItem))

        # fetch data (dragItem)
        checked = self.IsItemChecked(dragItem)
        image = self.GetItemImage(dragItem, 0)
        text = self.GetItemText(dragItem)
        if self.GetLayerInfo(dragItem, key='type') == 'command':
            # recreate command layer
            newctrl = self._createCommandCtrl()
            try:
                newctrl.SetValue(
                    self.GetLayerInfo(
                        dragItem,
                        key='maplayer').GetCmd(
                        string=True))
            except:
                pass
            newctrl.Bind(wx.EVT_TEXT_ENTER, self.OnCmdChanged)
            data = self.GetPyData(dragItem)

        elif self.GetLayerInfo(dragItem, key='ctrl'):
            # recreate data layer
            btnbmp = LMIcons["layerOptions"].GetBitmap((16, 16))
            newctrl = GenBitmapButton(
                self, id=wx.ID_ANY, bitmap=btnbmp, size=(24, 24))
            newctrl.SetToolTip(_("Click to edit layer settings"))
            self.Bind(wx.EVT_BUTTON, self.OnLayerContextMenuButton, newctrl)
            data = self.GetPyData(dragItem)

        elif self.GetLayerInfo(dragItem, key='type') == 'group':
            # recreate group
            newctrl = None
            data = None

        # decide where to put recreated item
        if dropTarget is not None and dropTarget != self.GetRootItem():
            if parent:
                # new item is a group
                afteritem = parent
            else:
                # new item is a single layer
                afteritem = dropTarget

            # dragItem dropped on group
            if self.GetLayerInfo(afteritem, key='type') == 'group':
                newItem = self.PrependItem(afteritem, text=text,
                                           ct_type=1, wnd=newctrl, image=image,
                                           data=data)
                self.Expand(afteritem)
            else:
                # dragItem dropped on single layer
                newparent = self.GetItemParent(afteritem)
                newItem = self.InsertItem(
                    newparent,
                    self.GetPrevSibling(afteritem),
                    text=text,
                    ct_type=1,
                    wnd=newctrl,
                    image=image,
                    data=data)
        else:
            # if dragItem not dropped on a layer or group, append or prepend it
            # to the layer tree
            if self.flag & wx.TREE_HITTEST_ABOVE:
                newItem = self.PrependItem(self.root, text=text,
                                           ct_type=1, wnd=newctrl, image=image,
                                           data=data)
            elif (self.flag &  wx.TREE_HITTEST_BELOW) or (self.flag & wx.TREE_HITTEST_NOWHERE) \
                    or (self.flag & wx.TREE_HITTEST_TOLEFT) or (self.flag & wx.TREE_HITTEST_TORIGHT):
                newItem = self.AppendItem(self.root, text=text,
                                          ct_type=1, wnd=newctrl, image=image,
                                          data=data)

        # update new layer
        self.SetPyData(newItem, self.GetPyData(dragItem))
        if newctrl:
            self.SetLayerInfo(newItem, key='ctrl', value=newctrl.GetId())
        else:
            self.SetLayerInfo(newItem, key='ctrl', value=None)

        self.forceCheck = True
        self.CheckItem(newItem, checked=checked)  # causes a new render

        return newItem

    def _getLayerName(self, item, lname=''):
        """Get layer name string

        :param lname: optional layer name
        """
        mapLayer = self.GetLayerInfo(item, key='maplayer')
        if not mapLayer:
            return lname

        if not lname:
            lname = self.GetLayerInfo(item, key='label')
        opacity = int(mapLayer.GetOpacity() * 100)
        if not lname:
            dcmd = self.GetLayerInfo(item, key='cmd')
            lname, found = GetLayerNameFromCmd(
                dcmd, layerType=mapLayer.GetType(), fullyQualified=True)
            if not found:
                return None

        if opacity < 100:
            return lname + ' (%s %d' % (_('opacity:'), opacity) + '%)'

        return lname

    def GetOptData(self, dcmd, layer, params, propwin):
        """Process layer data (when changes in properties dialog are applied)
        """
        # set layer text to map name
        if dcmd:
            self.SetLayerInfo(layer, key='cmd', value=dcmd)
            mapText = self._getLayerName(layer)
            mapName, found = GetLayerNameFromCmd(dcmd)
            mapLayer = self.GetLayerInfo(layer, key='maplayer')
            self.SetItemText(layer, mapName)

            if not mapText or not found:
                propwin.Hide()
                GWarning(parent=self,
                         message=_("Map <%s> not found.") % mapName)
                return

        # update layer data
        if params:
            self.SetPyData(layer, (self.GetLayerInfo(layer), params))
        self.SetLayerInfo(layer, key='propwin', value=propwin)

        # change parameters for item in layers list in render.Map
        if params:
            self.ChangeLayer(layer)

        # set region if auto-zooming is enabled or layer tree contains
        # only one map layer
        if dcmd:
            if not self.mapdisplay.IsPaneShown('3d') and(
                self.first or UserSettings.Get(
                    group='display', key='autoZooming', subkey='enabled')):
                mapLayer = self.GetLayerInfo(layer, key='maplayer')
                if mapLayer.GetType() in ('raster', 'vector'):
                    self.mapdisplay.MapWindow.ZoomToMap(layers=[mapLayer, ],
                                                        render=False)

            self.first = False  # first layer has been already added to
            # the layer tree

        if dcmd:
            if not mapLayer.IsActive():
                self.forceCheck = True
                self.CheckItem(layer, True)
                mapLayer.SetActive(True)
                # no need to update nviz here, already done by OnLayerChecked

            # update nviz session
            elif self.mapdisplay.IsPaneShown('3d'):
                mapLayer = self.GetLayerInfo(layer, key='maplayer')
                mapWin = self.mapdisplay.MapWindow
                if len(mapLayer.GetCmd()) > 0:
                    if mapLayer.type == 'raster':
                        if mapWin.IsLoaded(layer):
                            mapWin.UnloadRaster(layer)

                        mapWin.LoadRaster(layer)

                    elif mapLayer.type == 'raster_3d':
                        if mapWin.IsLoaded(layer):
                            mapWin.UnloadRaster3d(layer)

                        mapWin.LoadRaster3d(layer)

                    elif mapLayer.type == 'vector':
                        if mapWin.IsLoaded(layer):
                            mapWin.UnloadVector(layer)
                        vInfo = gvector.vector_info_topo(mapLayer.GetName())
                        if (vInfo['points'] + vInfo['centroids']) > 0:
                            mapWin.LoadVector(layer, points=True)
                        if (vInfo['lines'] + vInfo['boundaries']) > 0:
                            mapWin.LoadVector(layer, points=False)

                    # reset view when first layer loaded
                    nlayers = len(
                        mapWin.Map.GetListOfLayers(
                            ltype=(
                                'raster',
                                'raster_3d',
                                'vector'),
                            active=True))
                    if nlayers < 2:
                        mapWin.ResetView()

    def GetVisibleLayers(self, skipDigitized=False):
        # make a list of visible layers
        layers = []
        if self.root is None:
            return layers

        vislayer = self.GetFirstChild(self.root)[0]

        if not vislayer or self.GetPyData(vislayer) is None:
            return layers

        vdigitLayer = None
        if skipDigitized:
            digitToolbar = self.mapdisplay.GetToolbar('vdigit')
            if digitToolbar:
                vdigitLayer = digitToolbar.GetLayer()

        itemList = ""
        while vislayer:
            itemList += self.GetItemText(vislayer) + ','
            lType = self.GetLayerInfo(vislayer, key='type')
            mapLayer = self.GetLayerInfo(vislayer, key='maplayer')
            if lType and lType != 'group' and mapLayer is not vdigitLayer:
                layers.append(mapLayer)

            vislayer = self.GetNextItem(vislayer)

        Debug.msg(5, "LayerTree.GetVisibleLayers(): items=%s" %
                  (reversed(itemList)))

        layers.reverse()
        return layers

    def ChangeLayer(self, item):
        """Change layer"""
        type = self.GetLayerInfo(item, key='type')
        layerName = None

        if type == 'command':
            win = self.FindWindowById(self.GetLayerInfo(item, key='ctrl'))
            if win.GetValue() is not None:
                cmd = win.GetValue().split(';')
                cmdlist = []
                for c in cmd:
                    cmdlist.append(c.split(' '))
                opac = 1.0
                chk = self.IsItemChecked(item)
                hidden = not self.IsVisible(item)
        elif type != 'group':
            if self.GetPyData(item) is not None:
                cmdlist = self.GetLayerInfo(item, key='cmd')
                opac = self.GetLayerInfo(item, key='maplayer').GetOpacity()
                chk = self.IsItemChecked(item)
                hidden = not self.IsVisible(item)
                # determine layer name
                layerName, found = GetLayerNameFromCmd(
                    cmdlist, fullyQualified=True)
                if not found:
                    layerName = self.GetItemText(item)

        maplayer = self.Map.ChangeLayer(
            layer=self.GetLayerInfo(
                item,
                key='maplayer'),
            ltype=type,
            command=cmdlist,
            name=layerName,
            active=chk,
            hidden=hidden,
            opacity=opac)

        self.SetLayerInfo(item, key='maplayer', value=maplayer)

        # if digitization tool enabled -> update list of available vector map
        # layers
        if self.mapdisplay.GetToolbar('vdigit'):
            self.mapdisplay.GetToolbar(
                'vdigit').UpdateListOfLayers(updateTool=True)

        self.Map.SetLayers(self.GetVisibleLayers())

        # redraw map if auto-rendering is enabled
        self.rerender = True

    def OnCloseWindow(self, event):
        pass
        # self.Map.Clean()

    def FindItemByData(self, key, value):
        """Find item based on key and value (see PyData[0]).

        If key is 'name', finds item(s) of given maplayer name.

        :return: item instance
        :return: None not found
        """
        item = self.GetFirstChild(self.root)[0]
        if key == 'name':
            return self.__FindSubItemByName(item, value)
        else:
            return self.__FindSubItemByData(item, key, value)

    def FindItemByIndex(self, index):
        """Find item by index (starting at 0)

        :return: item instance
        :return: None not found
        """
        item = self.GetFirstChild(self.root)[0]
        i = 0
        while item and item.IsOk():
            if i == index:
                return item

            item = self.GetNextItem(item)
            i += 1

        return None

    def FindItemByWindow(self, window):
        """Find item by window (button for context menu)

        :return: window instance
        :return: None not found
        """
        item = self.GetFirstChild(self.root)[0]
        while item and item.IsOk():
            if self.GetItemWindow(item) == window:
                return item

            item = self.GetNextItem(item)

        return None

    def EnableItemType(self, type, enable=True):
        """Enable/disable items in layer tree"""
        item = self.GetFirstChild(self.root)[0]
        while item and item.IsOk():
            mapLayer = self.GetLayerInfo(item, key='maplayer')
            if mapLayer and type == mapLayer.type:
                self.EnableItem(item, enable)

            item = self.GetNextSibling(item)

    def __FindSubItemByData(self, item, key, value):
        """Support method for FindItemByData"""
        while item and item.IsOk():
            itemValue = self.GetLayerInfo(item, key=key)

            if value == itemValue:
                return item
            if self.GetLayerInfo(item, key='type') == 'group':
                subItem = self.GetFirstChild(item)[0]
                found = self.__FindSubItemByData(subItem, key, value)
                if found:
                    return found
            item = self.GetNextSibling(item)

        return None

    def __FindSubItemByName(self, item, value):
        """Support method for FindItemByData for searching by name"""
        items = []
        while item and item.IsOk():
            try:
                itemLayer = self.GetLayerInfo(item, key='maplayer')
            except KeyError:
                return None

            if itemLayer and value == itemLayer.GetName():
                items.append(item)
            if self.GetLayerInfo(item, key='type') == 'group':
                subItem = self.GetFirstChild(item)[0]
                found = self.__FindSubItemByName(subItem, value)
                if found:
                    items.extend(found)
            item = self.GetNextSibling(item)

        if items:
            return items
        return None

    def _createCommandCtrl(self):
        """Creates text control for command layer"""
        height = 25
        if sys.platform in ('win32', 'darwin'):
            height = 40
        ctrl = TextCtrl(self, id=wx.ID_ANY, value='', pos=wx.DefaultPosition, size=(
            self.GetSize()[0] - 100, height), style=wx.TE_PROCESS_ENTER | wx.TE_DONTWRAP)
        return ctrl
