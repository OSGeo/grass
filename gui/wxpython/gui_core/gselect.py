"""!
@package gui_core.gselect

@brief Custom control that selects elements

Classes:
 - gselect::Select
 - gselect::VectorSelect
 - gselect::TreeCrtlComboPopup
 - gselect::VectorDBInfo
 - gselect::LayerSelect
 - gselect::DriverSelect
 - gselect::DatabaseSelect
 - gselect::TableSelect
 - gselect::ColumnSelect
 - gselect::DbaseSelect
 - gselect::LocationSelect
 - gselect::MapsetSelect
 - gselect::SubGroupSelect
 - gselect::FormatSelect
 - gselect::GdalSelect
 - gselect::ProjSelect
 - gselect::ElementSelect
 - gselect::OgrTypeSelect
 - gselect::CoordinatesSelect
 - gselect::SignatureSelect

(C) 2007-2012 by the GRASS Development Team 

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
@author Stepan Turek <stepan.turek seznam.cz> (CoordinatesSelect)
"""

import os
import sys
import glob
import copy

import wx
import wx.combo
import wx.lib.buttons          as  buttons
import wx.lib.filebrowsebutton as filebrowse

from wx.lib.newevent import NewEvent

from core import globalvar

import grass.script as grass
import grass.temporal as tgis
from   grass.script import task as gtask

from core.gcmd     import RunCommand, GError, GMessage
from core.utils    import GetListOfLocations, GetListOfMapsets, GetFormats
from core.utils    import GetSettingsPath, GetValidLayerName, ListSortLower
from core.settings import UserSettings
from core.debug    import Debug

wxGdalSelect, EVT_GDALSELECT = NewEvent()

class Select(wx.combo.ComboCtrl):
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                 type = None, multiple = False, nmaps = 1,
                 mapsets = None, updateOnPopup = True, onPopup = None,
                 fullyQualified = True):
        """!Custom control to create a ComboBox with a tree control to
        display and select GIS elements within acessible mapsets.
        Elements can be selected with mouse. Can allow multiple
        selections, when argument <em>multiple</em> is True. Multiple
        selections are separated by commas.

        @param type type of GIS elements ('raster, 'vector', ...)
        @param multiple True for multiple input
        @param nmaps number of maps to be entered
        @param mapsets force list of mapsets (otherwise search path)
        @param updateOnPopup True for updating list of elements on popup
        @param onPopup function to be called on Popup
        @param fullyQualified True to provide fully qualified names (map@mapset)
        """
        wx.combo.ComboCtrl.__init__(self, parent=parent, id=id, size=size)
        self.GetChildren()[0].SetName("Select")
        self.GetChildren()[0].type = type
        
        self.tcp = TreeCtrlComboPopup()
        self.SetPopupControl(self.tcp)
        self.SetPopupExtents(0, 100)
        if type:
            self.tcp.SetData(type = type, mapsets = mapsets,
                             multiple = multiple, nmaps = nmaps,
                             updateOnPopup = updateOnPopup, onPopup = onPopup,
                             fullyQualified = fullyQualified)
        self.GetChildren()[0].Bind(wx.EVT_KEY_UP, self.OnKeyUp)
     
    def OnKeyUp(self, event):
        """!Shows popupwindow if down arrow key is released"""
        if event.GetKeyCode() == wx.WXK_DOWN and not self.IsPopupShown():
            self.ShowPopup() 
        else:
            event.Skip()
        
    def SetElementList(self, type, mapsets = None):
        """!Set element list

        @param type GIS element type
        @param mapsets list of acceptable mapsets (None for all in search path)
        """
        self.tcp.SetData(type = type, mapsets = mapsets)
        
    def GetElementList(self):
        """!Load elements"""
        self.tcp.GetElementList()
    
    def SetType(self, etype, multiple = False, nmaps = 1,
                mapsets = None, updateOnPopup = True, onPopup = None):
        """!Param set element type for widget

        @param etype element type, see gselect.ElementSelect
        """
        self.tcp.SetData(type = etype, mapsets = mapsets,
                         multiple = multiple, nmaps = nmaps,
                         updateOnPopup = updateOnPopup, onPopup = onPopup)
        
class VectorSelect(Select):
    def __init__(self, parent, ftype, **kwargs):
        """!Custom to create a ComboBox with a tree control to display and
        select vector maps. Control allows to filter vector maps. If you
        don't need this feature use Select class instead
        
        @param ftype filter vector maps based on feature type
        """
        Select.__init__(self, parent = parent, id = wx.ID_ANY,
                        type = 'vector', **kwargs)
        
        self.ftype = ftype
        
        # remove vector maps which do not contain given feature type
        self.tcp.SetFilter(self._isElement)
        
    def _isElement(self, vectorName):
        """!Check if element should be filtered out"""
        try:
            if int(grass.vector_info_topo(vectorName)[self.ftype]) < 1:
                return False
        except KeyError:
            return False
        
        return True

class TreeCtrlComboPopup(wx.combo.ComboPopup):
    """!Create a tree ComboBox for selecting maps and other GIS elements
    in accessible mapsets within the current location
    """
    # overridden ComboPopup methods
    def Init(self):
        self.value = [] # for multiple is False -> len(self.value) in [0,1]
        self.curitem = None
        self.multiple = False
        self.nmaps = 1
        self.type = None
        self.mapsets = None
        self.updateOnPopup = True
        self.onPopup = None
        self.fullyQualified = True
        
        self.SetFilter(None)
        
    def Create(self, parent):
        self.seltree = wx.TreeCtrl(parent, style=wx.TR_HIDE_ROOT
                                   |wx.TR_HAS_BUTTONS
                                   |wx.TR_SINGLE
                                   |wx.TR_LINES_AT_ROOT
                                   |wx.SIMPLE_BORDER
                                   |wx.TR_FULL_ROW_HIGHLIGHT)
        self.seltree.Bind(wx.EVT_KEY_UP, self.OnKeyUp)
        self.seltree.Bind(wx.EVT_MOTION, self.OnMotion)
        self.seltree.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.seltree.Bind(wx.EVT_TREE_ITEM_EXPANDING, self.mapsetExpanded)
        self.seltree.Bind(wx.EVT_TREE_ITEM_COLLAPSED, self.mapsetCollapsed)
        self.seltree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.mapsetActivated)
        self.seltree.Bind(wx.EVT_TREE_SEL_CHANGED, self.mapsetSelected)
        self.seltree.Bind(wx.EVT_TREE_DELETE_ITEM, lambda x: None)
        
    # the following dummy handler are needed to keep tree events from propagating up to
    # the parent GIS Manager layer tree
    def mapsetExpanded(self, event):
        pass

    def mapsetCollapsed(self, event):
        pass

    def mapsetActivated(self, event):
        pass

    def mapsetSelected(self, event):
        pass
    # end of dummy events

    def GetControl(self):
        return self.seltree

    def GetStringValue(self):
        """!Get value as a string separated by commas"""
        return ','.join(self.value)
    
    def SetFilter(self, filter):
        """!Set filter for GIS elements, see e.g. VectorSelect"""
        self.filterElements = filter
    
    def OnPopup(self, force = False):
        """!Limited only for first selected"""
        if not force and not self.updateOnPopup:
            return
        if self.onPopup:
            selected, exclude = self.onPopup(self.type)
        else:
            selected = None
            exclude  = False
 
        self.GetElementList(selected, exclude)
        
        # selects map starting according to written text
        inputText = self.GetCombo().GetValue().strip()
        if inputText:
            root = self.seltree.GetRootItem()
            match = self.FindItem(root, inputText, startLetters = True)
            self.seltree.EnsureVisible(match)
            self.seltree.SelectItem(match)
            
      
    def GetElementList(self, elements = None, exclude = False):
        """!Get filtered list of GIS elements in accessible mapsets
        and display as tree with all relevant elements displayed
        beneath each mapset branch
        """
        # update list
        self.seltree.DeleteAllItems()
        self._getElementList(self.type, self.mapsets, elements, exclude)
        
        if len(self.value) > 0:
            root = self.seltree.GetRootItem()
            if not root:
                return
            item = self.FindItem(root, self.value[0])
            try:
                self.seltree.EnsureVisible(item)
                self.seltree.SelectItem(item)
            except:
                pass
            
    def SetStringValue(self, value):
        # this assumes that item strings are unique...
        root = self.seltree.GetRootItem()
        if not root:
            return
        winValue = self.GetCombo().GetValue().strip(',')
        self.value = []
        if winValue:
            self.value = winValue.split(',')
        
    def GetAdjustedSize(self, minWidth, prefHeight, maxHeight):
        """!Reads UserSettings to get height (which was 200 in old implementation).
        """
        height = UserSettings.Get(group = 'appearance', key = 'gSelectPopupHeight', subkey = 'value')
        return wx.Size(minWidth, min(height, maxHeight))

    def _getElementList(self, element, mapsets = None, elements = None, exclude = False):
        """!Get list of GIS elements in accessible mapsets and display as tree
        with all relevant elements displayed beneath each mapset branch

        @param element GIS element
        @param mapsets list of acceptable mapsets (None for all mapsets in search path)
        @param elements list of forced GIS elements
        @param exclude True to exclude, False for forcing the list (elements)
        """
        # get current mapset
        curr_mapset = grass.gisenv()['MAPSET']
        
        # map element types to g.mlist types
        elementdict = {'cell':'rast',
                       'raster':'rast',
                       'rast':'rast',
                       'raster files':'rast',
                       'grid3':'rast3d',
                       'rast3d':'rast3d',
                       '3d-raster':'rast3d',
                       'raster3D':'rast3d',
                       'raster3D files':'rast3d',
                       'vector':'vect',
                       'vect':'vect',
                       'binary vector files':'vect',
                       'dig':'oldvect',
                       'oldvect':'oldvect',
                       'old vector':'oldvect',
                       'dig_ascii':'asciivect',
                       'asciivect':'asciivect',
                       'asciivector':'asciivect',
                       'ascii vector files':'asciivect',
                       'icons':'icon',
                       'icon':'icon',
                       'paint icon files':'icon',
                       'paint/labels':'labels',
                       'labels':'labels',
                       'label':'labels',
                       'paint label files':'labels',
                       'site_lists':'sites',
                       'sites':'sites',
                       'site list':'sites',
                       'site list files':'sites',
                       'windows':'region',
                       'region':'region',
                       'region definition':'region',
                       'region definition files':'region',
                       'windows3d':'region3d',
                       'region3d':'region3d',
                       'region3D definition':'region3d',
                       'region3D definition files':'region3d',
                       'group':'group',
                       'imagery group':'group',
                       'imagery group files':'group',
                       '3d.view':'3dview',
                       '3dview':'3dview',
                       '3D viewing parameters':'3dview',
                       '3D view parameters':'3dview',
                       'stds':'stds',
                       'strds':'strds',
                       'str3ds':'str3ds',
                       'stvds':'stvds'}
        
        if element not in elementdict:
            self.AddItem(_('Not selectable element'), node = False)
            return
        
        if element in ('stds', 'strds', 'str3ds', 'stvds'):
            filesdict = tgis.tlist_grouped(elementdict[element], element == 'stds')
        else:
            if globalvar.have_mlist:
                filesdict = grass.mlist_grouped(elementdict[element],
                                                check_search_path = False)
            else:
                filesdict = grass.list_grouped(elementdict[element],
                                           check_search_path = False)
        
        # list of mapsets in current location
        if mapsets is None:
            mapsets = grass.mapsets(search_path = True)
        
        # current mapset first
        if curr_mapset in mapsets and mapsets[0] != curr_mapset:
            mapsets.remove(curr_mapset)
            mapsets.insert(0, curr_mapset)
        
        first_mapset = None
        for mapset in mapsets:
            mapset_node = self.AddItem(_('Mapset') + ': ' + mapset, node = True, mapset = mapset)
            node = mapset_node
            if not first_mapset:
                first_mapset = mapset_node
            
            self.seltree.SetItemTextColour(mapset_node, wx.Colour(50, 50, 200))
            if mapset not in filesdict:
                continue
            try:
                if type(filesdict[mapset]) == dict:
                    for elementType in filesdict[mapset].keys():
                        node = self.AddItem(_('Type: ') + elementType, mapset = mapset,
                                            node = True, parent = mapset_node)
                        self.seltree.SetItemTextColour(node, wx.Colour(50, 50, 200))
                        elem_list = filesdict[mapset][elementType]
                        self._addItems(elist = elem_list, elements = elements,
                                       mapset = mapset, exclude = exclude, node = node)
                else:
                    elem_list = filesdict[mapset]
                    self._addItems(elist = elem_list, elements = elements,
                                   mapset = mapset, exclude = exclude, node = node)
            except StandardError, e:
                sys.stderr.write(_("GSelect: invalid item: %s") % e)
                continue
            
            if self.seltree.ItemHasChildren(mapset_node):
                sel = UserSettings.Get(group='appearance', key='elementListExpand',
                                       subkey='selection')
                collapse = True

                if sel == 0: # collapse all except PERMANENT and current
                    if mapset in ('PERMANENT', curr_mapset):
                        collapse = False
                elif sel == 1: # collapse all except PERMANENT
                    if mapset == 'PERMANENT':
                        collapse = False
                elif sel == 2: # collapse all except current
                    if mapset == curr_mapset:
                        collapse = False
                elif sel == 3: # collapse all
                    pass
                elif sel == 4: # expand all
                    collapse = False
                
                if collapse:
                    self.seltree.CollapseAllChildren(mapset_node)
                else:
                    self.seltree.ExpandAllChildren(mapset_node)
        
        if first_mapset:
            # select first mapset (MSW hack)
            self.seltree.SelectItem(first_mapset)
    
    # helpers
    def _addItems(self, elist, elements, mapset, exclude, node):
        """!Helper function for adding multiple items (maps, stds).

        @param elist list of map/stds names
        @param elements list of forced elements
        @param mapset mapset name
        @param exclude True to exclude, False for forcing the list
        @param node parent node
        """
        elist.sort()
        for elem in elist:
            if elem != '':
                fullqElem = elem + '@' + mapset
                if elements is not None:
                    if (exclude and fullqElem in elements) or \
                            (not exclude and fullqElem not in elements):
                        continue
                
                if self.filterElements:
                    if self.filterElements(fullqElem):
                        self.AddItem(elem, mapset = mapset, node = False, parent = node)
                else:
                    self.AddItem(elem, mapset = mapset, node = False, parent = node)

    def FindItem(self, parentItem, text, startLetters = False):
        """!Finds item with given name or starting with given text"""
        startletters = startLetters
        item, cookie = self.seltree.GetFirstChild(parentItem)
        while wx.TreeItemId.IsOk(item):
            if self.seltree.GetItemText(item) == text:
                return item
            if self.seltree.ItemHasChildren(item):
                item = self.FindItem(item, text, startLetters = startletters)
                if wx.TreeItemId.IsOk(item):
                    return item
            elif startletters and self.seltree.GetItemText(item).startswith(text.split('@', 1)[0]):
                return item
            item, cookie = self.seltree.GetNextChild(parentItem, cookie)
        return wx.TreeItemId()
    
    def AddItem(self, value, mapset = None, node = True, parent = None):
        if not parent:
            root = self.seltree.GetRootItem()
            if not root:
                root = self.seltree.AddRoot("<hidden root>")
            parent = root

        data = {'node': node, 'mapset': mapset}
        item = self.seltree.AppendItem(parent, text = value, data = wx.TreeItemData(data))
        return item
    
    # able to recieve only wx.EVT_KEY_UP
    def OnKeyUp(self, event):
        """!Enables to select items using keyboard"""
        
        item = self.seltree.GetSelection()
        if event.GetKeyCode() == wx.WXK_DOWN:
            self.seltree.SelectItem(self.seltree.GetNextVisible(item))
            
        # problem with GetPrevVisible   
        elif event.GetKeyCode() == wx.WXK_UP: 
            if self.seltree.ItemHasChildren(item) and self.seltree.IsExpanded(self.seltree.GetPrevSibling(item)):
                itemPrev = self.seltree.GetLastChild(self.seltree.GetPrevSibling(item))
            else:
                itemPrev = self.seltree.GetPrevSibling(item)
                if not wx.TreeItemId.IsOk(itemPrev):
                    itemPrev = self.seltree.GetItemParent(item)
                    if item == self.seltree.GetFirstChild(self.seltree.GetRootItem())[0]:
                        itemPrev = item
            self.seltree.SelectItem(itemPrev)
        
        # selects first item starting with the written text in next mapset
        elif event.GetKeyCode() == wx.WXK_TAB:
            selected = self.seltree.GetSelection()
            if self.seltree.ItemHasChildren(selected):
                parent = selected
            else:
                parent = self.seltree.GetItemParent(selected)
            nextSibling = self.seltree.GetNextSibling(parent)
            if wx.TreeItemId.IsOk(nextSibling):
                match = self.FindItem(nextSibling, self.GetCombo().GetValue().strip(), True) 
            else: 
                match = self.FindItem(self.seltree.GetFirstChild(self.seltree.GetItemParent(parent))[0],
                                        self.GetCombo().GetValue().strip(), True) 
            self.seltree.SelectItem(match)
            
        elif event.GetKeyCode() == wx.WXK_RIGHT:
            if self.seltree.ItemHasChildren(item):
                self.seltree.Expand(item)
        
        elif event.GetKeyCode() == wx.WXK_LEFT:
            if self.seltree.ItemHasChildren(item):
                self.seltree.Collapse(item)
                
        elif event.GetKeyCode() == wx.WXK_ESCAPE:
            self.Dismiss()
            
        elif event.GetKeyCode() == wx.WXK_RETURN:
            if self.seltree.GetPyData(item)['node']:
                self.value = []
            else:
                fullName = self.seltree.GetItemText(item)
                if self.fullyQualified:
                    fullName += '@' + self.seltree.GetPyData(item)['mapset']
                
                if self.multiple:
                    self.value.append(fullName)
                else:
                    if self.nmaps > 1: #  see key_desc
                        if len(self.value) >= self.nmaps:
                            self.value = [fullName]
                        else:
                            self.value.append(fullName)
                    else:
                        self.value = [fullName]
            
            self.Dismiss()
        
    def OnMotion(self, evt):
        """!Have the selection follow the mouse, like in a real combobox
        """
        item, flags = self.seltree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.seltree.SelectItem(item)
            self.curitem = item
        evt.Skip()

    def OnLeftDown(self, evt):
        """!Do the combobox selection
        """
        item, flags = self.seltree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.curitem = item
            
            if self.seltree.GetPyData(item)['node']:
                evt.Skip()
                return
            
            fullName = self.seltree.GetItemText(item)
            if self.fullyQualified:
                fullName += '@' + self.seltree.GetPyData(item)['mapset']
            
            if self.multiple:
                self.value.append(fullName)
            else:
                if self.nmaps > 1: #  see key_desc
                    if len(self.value) >= self.nmaps:
                        self.value = [fullName]
                    else:
                        self.value.append(fullName)
                else:
                    self.value = [fullName]
        
            self.Dismiss()
        
        evt.Skip()

    def SetData(self, **kargs):
        """!Set object properties"""
        if 'type' in kargs:
            self.type = kargs['type']
            if self.type in ('stds', 'strds', 'str3ds', 'stvds'):
                tgis.init()
        if 'mapsets' in kargs:
            self.mapsets = kargs['mapsets']
        if 'multiple' in kargs:
            self.multiple = kargs['multiple']
        if 'nmaps' in kargs:
            self.nmaps = kargs['nmaps']
        if 'updateOnPopup' in kargs:
            self.updateOnPopup = kargs['updateOnPopup']
        if 'onPopup' in kargs:
            self.onPopup = kargs['onPopup']
        if 'fullyQualified' in kargs:
            self.fullyQualified = kargs['fullyQualified']
        
    def GetType(self):
        """!Get element type
        """
        return self.type
    
class VectorDBInfo:
    """!Class providing information about attribute tables
    linked to a vector map"""
    def __init__(self, map):
        self.map = map

        # dictionary of layer number and associated (driver, database, table)
        self.layers = {}
         # dictionary of table and associated columns (type, length, values, ids)
        self.tables = {}
        
        if not self._CheckDBConnection(): # -> self.layers
            return

        self._DescribeTables() # -> self.tables

    def _CheckDBConnection(self):
        """!Check DB connection"""
        nuldev = file(os.devnull, 'w+')
        self.layers = grass.vector_db(map = self.map, stderr = nuldev)
        nuldev.close()
        
        return bool(len(self.layers.keys()) > 0)
        
    def _DescribeTables(self):
        """!Describe linked tables"""
        for layer in self.layers.keys():
            # determine column names and types
            table = self.layers[layer]["table"]
            columns = {} # {name: {type, length, [values], [ids]}}
            i = 0
            Debug.msg(1, "gselect.VectorDBInfo._DescribeTables(): table=%s driver=%s database=%s" % \
                          (self.layers[layer]["table"], self.layers[layer]["driver"],
                           self.layers[layer]["database"]))
            for item in grass.db_describe(table = self.layers[layer]["table"],
                                          driver = self.layers[layer]["driver"],
                                          database = self.layers[layer]["database"])['cols']:
                name, type, length = item
                # FIXME: support more datatypes
                if type.lower() == "integer":
                    ctype = int
                elif type.lower() == "double precision":
                    ctype = float
                else:
                    ctype = str

                columns[name.strip()] = { 'index'  : i,
                                          'type'   : type.lower(),
                                          'ctype'  : ctype,
                                          'length' : int(length),
                                          'values' : [],
                                          'ids'    : []}
                i += 1
            
            # check for key column
            # v.db.connect -g/p returns always key column name lowercase
            if self.layers[layer]["key"] not in columns.keys():
                for col in columns.keys():
                    if col.lower() == self.layers[layer]["key"]:
                        self.layers[layer]["key"] = col.upper()
                        break
            
            self.tables[table] = columns
            
        return True
    
    def Reset(self):
        """!Reset"""
        for layer in self.layers:
            table = self.layers[layer]["table"] # get table desc
            for name in self.tables[table].keys():
                self.tables[table][name]['values'] = []
                self.tables[table][name]['ids']    = []
    
    def GetName(self):
        """!Get vector name"""
        return self.map
    
    def GetKeyColumn(self, layer):
        """!Get key column of given layer
        
        @param layer vector layer number
        """
        return str(self.layers[layer]['key'])
    
    def GetTable(self, layer):
        """!Get table name of given layer
        
        @param layer vector layer number
        """
        return self.layers[layer]['table']
    
    def GetDbSettings(self, layer):
        """!Get database settins

        @param layer layer number
        
        @return (driver, database)
        """
        return self.layers[layer]['driver'], self.layers[layer]['database']
    
    def GetTableDesc(self, table):
        """!Get table columns

        @param table table name
        """
        return self.tables[table]

class LayerSelect(wx.ComboBox):
    def __init__(self, parent, id = wx.ID_ANY,
                 size = globalvar.DIALOG_COMBOBOX_SIZE,
                 vector = None, dsn = None, choices = [], all = False, default = None):
        """!Creates combo box for selecting vector map layer names

        @param vector vector map name (native or connected via v.external)
        @param dsn    OGR data source name
        """
        super(LayerSelect, self).__init__(parent, id, size = size, choices = choices)

        self.all = all
        
        self.SetName("LayerSelect")

        # default value
        self.default = default

        self.InsertLayers(vector = vector, dsn = dsn)
        
    def InsertLayers(self, vector = None, dsn = None):
        """!Insert layers for a vector into the layer combobox

        @param vector vector map name (native or connected via v.external)
        @param dsn    OGR data source name
        """
        layers = list()
        if self.all:
            layers.append('-1')

        if vector:
            # TODO: use Vect_get_field2() in C modules where possible
            # currently the following is identical to
            # layers = utils.GetVectorNumberOfLayers(self, vector)

            ret = RunCommand('v.db.connect',
                             read = True,
                             quiet = True,
                             sep = '|',
                             flags = 'g',
                             map = vector)
            if ret:
                for line in ret.splitlines():
                    layerinfo = line.split('|')
                    layername = layerinfo[0].split('/')
                    # use this to get layer names
                    # but only when all modules use Vect_get_field2()
                    # which is not the case right now
                    ### layers.append(layername[len(layername) - 1])
                    layers.append(layername[0])

        elif dsn:
            ret = RunCommand('v.in.ogr',
                             read = True,
                             quiet = True,
                             flags = 'l',
                             dsn = dsn)
            if ret:
                layers = ret.splitlines()
    
        if self.default:
            if len(layers) == 0:
                layers.insert(0, str(self.default))
            elif self.default not in layers:
                layers.append(self.default)

        if len(layers) > 0:
            self.SetItems(layers)
        
class DriverSelect(wx.ComboBox):
    """!Creates combo box for selecting database driver.
    """
    def __init__(self, parent, choices, value,
                 id=wx.ID_ANY, pos=wx.DefaultPosition,
                 size=globalvar.DIALOG_LAYER_SIZE, **kargs):

        super(DriverSelect, self).__init__(parent, id, value, pos, size,
                                           choices, style=wx.CB_READONLY)
        
        self.SetName("DriverSelect")
        
        self.SetStringSelection(value)

class DatabaseSelect(wx.TextCtrl):
    """!Creates combo box for selecting database driver.
    """
    def __init__(self, parent, value = '', id = wx.ID_ANY, 
                 size = globalvar.DIALOG_TEXTCTRL_SIZE, **kargs):
        super(DatabaseSelect, self).__init__(parent, id, value, size = size, **kargs)
        self.SetName("DatabaseSelect")
    
class TableSelect(wx.ComboBox):
    """!Creates combo box for selecting attribute tables from the database
    """
    def __init__(self, parent,
                 id = wx.ID_ANY, value = '', 
                 size = globalvar.DIALOG_COMBOBOX_SIZE, choices = [], **kargs):
        super(TableSelect, self).__init__(parent, id, value, size = size, choices = choices,
                                          style = wx.CB_READONLY, **kargs)
        self.SetName("TableSelect")
        
        if not choices:
            self.InsertTables()
                
    def InsertTables(self, driver = None, database = None):
        """!Insert attribute tables into combobox"""
        items = []

        if not driver or not database:
            connect = grass.db_connection()
            
            driver = connect['driver']
            database = connect['database']
        
        ret = RunCommand('db.tables',
                         flags = 'p',
                         read = True,
                         driver = driver,
                         database = database)
        
        if ret:
            for table in ret.splitlines():
                items.append(table)
        
        self.SetItems(items)
        self.SetValue('')
        
class ColumnSelect(wx.ComboBox):
    """!Creates combo box for selecting columns in the attribute table
    for a vector map.

    @param parent window parent
    @param id window id
    @param value default value
    @param size window size
    @param vector vector map name
    @param layer layer number
    @param param parameters list (see menuform.py)
    @param **kwags wx.ComboBox parameters
    """
    def __init__(self, parent, id = wx.ID_ANY, value = '', 
                 size = globalvar.DIALOG_COMBOBOX_SIZE,
                 vector = None, layer = 1, param = None, **kwargs):
        self.defaultValue = value
        self.param = param
        
        super(ColumnSelect, self).__init__(parent, id, value, size = size, **kwargs)
        self.SetName("ColumnSelect")
        
        if vector:
            self.InsertColumns(vector, layer)
    
    def InsertColumns(self, vector, layer, excludeKey = False, excludeCols = None, type = None, dbInfo = None):
        """!Insert columns for a vector attribute table into the columns combobox

        @param vector vector name
        @param layer vector layer number
        @param excludeKey exclude key column from the list?
        @param excludeCols list of columns to be removed from the list
        @param type only columns of given type (given as list)
        """
        if not dbInfo:
            dbInfo = VectorDBInfo(vector)
        
        try:
            table = dbInfo.GetTable(int(layer))
            columnchoices = dbInfo.GetTableDesc(table)
            keyColumn = dbInfo.GetKeyColumn(int(layer))
            columns = len(columnchoices.keys()) * ['']
            for key, val in columnchoices.iteritems():
                columns[val['index']] = key
            if excludeKey: # exclude key column
                columns.remove(keyColumn)
            if excludeCols: # exclude key column
                for key in columnchoices.iterkeys():
                    if key in excludeCols:
                        columns.remove(key)
            if type: # only selected column types
                for key, value in columnchoices.iteritems():
                    if value['type'] not in type:
                        try:
                            columns.remove(key)
                        except ValueError:
                            pass
        except (KeyError, ValueError):
            columns = list()
            
        self.SetItems(columns)
        self.SetValue(self.defaultValue)
        
        if self.param:
            value = self.param.get('value', '')
            if value != '' and value in columns:
                self.SetValue(value)
        
    def InsertTableColumns(self, table, driver=None, database=None):
        """!Insert table columns

        @param table table name
        @param driver driver name
        @param database database name
        """
        columns = list()
        
        ret = RunCommand('db.columns',
                         read = True,
                         driver = driver,
                         database = database,
                         table = table)
        
        if ret:
            columns = ret.splitlines()
        
        self.SetItems(columns)
        self.SetValue(self.defaultValue)
        
        if self.param:
            value = self.param.get('value', '')
            if value != '' and value in columns:
                self.SetValue(value)

class DbaseSelect(wx.lib.filebrowsebutton.DirBrowseButton):
    """!Widget for selecting GRASS Database"""
    def __init__(self, parent, **kwargs):
        super(DbaseSelect, self).__init__(parent, id = wx.ID_ANY,
                                          size = globalvar.DIALOG_GSELECT_SIZE, labelText = '',
                                          dialogTitle = _('Choose GIS Data Directory'),
                                          buttonText = _('Browse'),
                                          startDirectory = grass.gisenv()['GISDBASE'],
                                          **kwargs)
        
class LocationSelect(wx.ComboBox):
    """!Widget for selecting GRASS location"""
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_COMBOBOX_SIZE, 
                 gisdbase = None, **kwargs):
        super(LocationSelect, self).__init__(parent, id, size = size, 
                                             style = wx.CB_READONLY, **kwargs)
        self.SetName("LocationSelect")
        
        if not gisdbase:
            self.gisdbase = grass.gisenv()['GISDBASE']
        else:
            self.gisdbase = gisdbase
        
        self.SetItems(GetListOfLocations(self.gisdbase))

    def UpdateItems(self, dbase):
        """!Update list of locations

        @param dbase path to GIS database
        """
        self.gisdbase = dbase
        if dbase:
            self.SetItems(GetListOfLocations(self.gisdbase))
        else:
            self.SetItems([])
        
class MapsetSelect(wx.ComboBox):
    """!Widget for selecting GRASS mapset"""
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_COMBOBOX_SIZE, 
                 gisdbase = None, location = None, setItems = True,
                 searchPath = False, new = False, skipCurrent = False, **kwargs):
        style = 0
        if not new:
            style = wx.CB_READONLY
        
        super(MapsetSelect, self).__init__(parent, id, size = size, 
                                           style = style, **kwargs)
        self.searchPath  = searchPath
        self.skipCurrent = skipCurrent
        
        self.SetName("MapsetSelect")
        if not gisdbase:
            self.gisdbase = grass.gisenv()['GISDBASE']
        else:
            self.gisdbase = gisdbase
        
        if not location:
            self.location = grass.gisenv()['LOCATION_NAME']
        else:
            self.location = location
        
        if setItems:
            self.SetItems(self._getMapsets())
        
    def UpdateItems(self, location, dbase = None):
        """!Update list of mapsets for given location

        @param dbase path to GIS database (None to use currently selected)
        @param location name of location
        """
        if dbase:
            self.gisdbase = dbase
        self.location = location
        if location:
            self.SetItems(self._getMapsets())
        else:
            self.SetItems([])
     
    def _getMapsets(self):
        if self.searchPath:
            mlist = RunCommand('g.mapsets',
                               read = True, flags = 'p',
                               sep = 'newline').splitlines()
        else:
            mlist = GetListOfMapsets(self.gisdbase, self.location,
                                     selectable = False)
        
        gisenv = grass.gisenv()
        if self.skipCurrent and \
                gisenv['LOCATION_NAME'] == self.location and \
                gisenv['MAPSET'] in mlist:
            mlist.remove(gisenv['MAPSET'])
        
        return mlist
    
class SubGroupSelect(wx.ComboBox):
    """!Widget for selecting subgroups"""
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE, 
                 **kwargs):
        super(SubGroupSelect, self).__init__(parent, id, size = size, 
                                             **kwargs)
        self.SetName("SubGroupSelect")

    def Insert(self, group):
        """!Insert subgroups for defined group"""
        if not group:
            return
        gisenv = grass.gisenv()
        try:
            name, mapset = group.split('@', 1)
        except ValueError:
            name = group
            mapset = gisenv['MAPSET']
        
        path = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'], mapset,
                            'group', name, 'subgroup')
        try:
            self.SetItems(os.listdir(path))
        except OSError:
            self.SetItems([])
        self.SetValue('')

class FormatSelect(wx.Choice):
    def __init__(self, parent, ogr = False,
                 sourceType = None, id = wx.ID_ANY, size = globalvar.DIALOG_SPIN_SIZE, 
                 **kwargs):
        """!Widget for selecting external (GDAL/OGR) format

        @param parent parent window
        @param sourceType source type ('file', 'directory', 'database', 'protocol') or None
        @param ogr True for OGR otherwise GDAL
        """
        super(FormatSelect, self).__init__(parent, id, size = size, 
                                           **kwargs)
        self.SetName("FormatSelect")
        
        if ogr:
            ftype = 'ogr'
        else:
            ftype = 'gdal'
        
        formats = list()
        for f in GetFormats()[ftype].values():
            formats += f
        self.SetItems(formats)
        
    def GetExtension(self, name):
        """!Get file extension by format name"""
        formatToExt = {
            # raster
            'GeoTIFF' : 'tif',
            'Erdas Imagine Images (.img)' : 'img',
            'Ground-based SAR Applications Testbed File Format (.gff)' : 'gff',
            'Arc/Info Binary Grid' : 'adf',
            'Portable Network Graphics' : 'png',
            'JPEG JFIF' : 'jpg',
            'Japanese DEM (.mem)' : 'mem',
            'Graphics Interchange Format (.gif)' : 'gif',
            'X11 PixMap Format' : 'xpm',
            'MS Windows Device Independent Bitmap' : 'bmp',
            'SPOT DIMAP' : 'dim',
            'RadarSat 2 XML Product' : 'xml',
            'EarthWatch .TIL' : 'til',
            'ERMapper .ers Labelled' : 'ers',
            'ERMapper Compressed Wavelets' : 'ecw',
            'GRIdded Binary (.grb)' : 'grb',
            'EUMETSAT Archive native (.nat)' : 'nat',
            'Idrisi Raster A.1' : 'rst',
            'Golden Software ASCII Grid (.grd)' : 'grd',
            'Golden Software Binary Grid (.grd)' : 'grd',
            'Golden Software 7 Binary Grid (.grd)' : 'grd',
            'R Object Data Store' : 'r',
            'USGS DOQ (Old Style)' : 'doq',
            'USGS DOQ (New Style)' : 'doq',
            'ENVI .hdr Labelled' : 'hdr',
            'ESRI .hdr Labelled' : 'hdr',
            'Generic Binary (.hdr Labelled)' : 'hdr',
            'PCI .aux Labelled' : 'aux',
            'EOSAT FAST Format' : 'fst',
            'VTP .bt (Binary Terrain) 1.3 Format' : 'bt',
            'FARSITE v.4 Landscape File (.lcp)' : 'lcp',
            'Swedish Grid RIK (.rik)' : 'rik',
            'USGS Optional ASCII DEM (and CDED)' : 'dem',
            'Northwood Numeric Grid Format .grd/.tab' : '',
            'Northwood Classified Grid Format .grc/.tab' : '',
            'ARC Digitized Raster Graphics' : 'arc',
            'Magellan topo (.blx)' : 'blx',
            'SAGA GIS Binary Grid (.sdat)' : 'sdat',
            # vector
            'ESRI Shapefile' : 'shp',
            'UK .NTF'        : 'ntf',
            'SDTS'           : 'ddf',
            'DGN'            : 'dgn',
            'VRT'            : 'vrt',
            'REC'            : 'rec',
            'BNA'            : 'bna',
            'CSV'            : 'csv',
            'GML'            : 'gml',
            'GPX'            : 'gpx',
            'KML'            : 'kml',
            'GMT'            : 'gmt',
            'PGeo'           : 'mdb',
            'XPlane'         : 'dat',
            'AVCBin'         : 'adf',
            'AVCE00'         : 'e00',
            'DXF'            : 'dxf',
            'Geoconcept'     : 'gxt',
            'GeoRSS'         : 'xml',
            'GPSTrackMaker'  : 'gtm',
            'VFK'            : 'vfk',
            'SVG'            : 'svg',
            }
        
        return formatToExt.get(name, '')
        
class GdalSelect(wx.Panel):
    def __init__(self, parent, panel, ogr = False, link = False, dest = False, 
                 default = 'file', exclude = [], envHandler = None):
        """!Widget for selecting GDAL/OGR datasource, format
        
        @param parent parent window
        @param ogr    use OGR selector instead of GDAL
        @param dest   True for output (destination)
        @param default deafult type (ignored when dest == True)
        @param exclude list of types to be excluded
        """
        self.parent = parent
        self.ogr    = ogr
        self.dest   = dest 
        wx.Panel.__init__(self, parent = panel, id = wx.ID_ANY)

        self.settingsBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                        label = " %s " % _("Settings"))
        
        self.inputBox = wx.StaticBox(parent = self, id = wx.ID_ANY)
        if dest:
            self.inputBox.SetLabel(" %s " % _("Output settings"))
        else:
            self.inputBox.SetLabel(" %s " % _("Source settings"))
        
        # source type
        sources = list()
        self.sourceMap = { 'file'   : -1,
                           'dir'    : -1,
                           'db'     : -1,
                           'db-pg'  : -1,
                           'pro'    : -1,
                           'native' : -1 }
        idx = 0
        if ogr and (link or dest):
            extraLabel = " (OGR)"
        else:
            extraLabel = ""
        if dest:
            sources.append(_("Native"))
            self.sourceMap['native'] = idx
            idx += 1
        if 'file' not in exclude:
            sources.append(_("File") + extraLabel)
            self.sourceMap['file'] = idx
            idx += 1
        if 'directory' not in exclude:
            sources.append(_("Directory") + extraLabel)
            self.sourceMap['dir'] = idx
            idx += 1
        if 'database' not in exclude:
            sources.append(_("Database") + extraLabel)
            self.sourceMap['db'] = idx
            idx += 1
        if 'protocol' not in exclude:
            sources.append(_("Protocol") + extraLabel)
            self.sourceMap['pro'] = idx
            idx += 1
        if 'database' not in exclude and ogr and (link or dest):
            sources.append(_("PostGIS (PG)"))
            self.sourceMap['db-pg'] = idx
        
        if self.ogr:
            self.settingsFile = os.path.join(GetSettingsPath(), 'wxOGR')
        else:
            self.settingsFile = os.path.join(GetSettingsPath(), 'wxGDAL')

        self.settingsChoice = wx.Choice(parent = self, id = wx.ID_ANY)
        self.settingsChoice.Bind(wx.EVT_CHOICE, self.OnSettingsLoad)
        self._settings = self._loadSettings() # -> self.settingsChoice.SetItems()
        self.btnSettingsSave = wx.Button(parent = self, id = wx.ID_SAVE)
        self.btnSettingsSave.Bind(wx.EVT_BUTTON, self.OnSettingsSave)
        self.btnSettingsSave.SetToolTipString(_("Save current settings"))
        self.btnSettingsDel = wx.Button(parent = self, id = wx.ID_REMOVE)
        self.btnSettingsDel.Bind(wx.EVT_BUTTON, self.OnSettingsDelete)
        self.btnSettingsSave.SetToolTipString(_("Delete currently selected settings"))
        
        self.source = wx.RadioBox(parent = self, id = wx.ID_ANY,
                                  style = wx.RA_SPECIFY_COLS,
                                  choices = sources)
        if dest:
            self.source.SetLabel(" %s " % _('Output type'))
        else:
            self.source.SetLabel(" %s " % _('Source type'))
        
        self.source.SetSelection(0)
        self.source.Bind(wx.EVT_RADIOBOX, self.OnSetType)
        
        # dsn widgets
        if not ogr:
            filemask = 'GeoTIFF (%s)|%s|%s (*.*)|*.*' % \
                (self._getExtPattern('tif'), self._getExtPattern('tif'), _('All files'))
        else:
            filemask = 'ESRI Shapefile (%s)|%s|%s (*.*)|*.*' % \
                (self._getExtPattern('shp'), self._getExtPattern('shp'), _('All files'))
        
        dsnFile = filebrowse.FileBrowseButton(parent=self, id=wx.ID_ANY, 
                                              size=globalvar.DIALOG_GSELECT_SIZE, labelText = '',
                                              dialogTitle=_('Choose file to import'),
                                              buttonText=_('Browse'),
                                              startDirectory=os.getcwd(),
                                              changeCallback=self.OnSetDsn,
                                              fileMask=filemask)
        dsnFile.Hide()
        
        dsnDir = filebrowse.DirBrowseButton(parent=self, id=wx.ID_ANY, 
                                            size=globalvar.DIALOG_GSELECT_SIZE, labelText='',
                                            dialogTitle=_('Choose input directory'),
                                            buttonText=_('Browse'),
                                            startDirectory=os.getcwd(),
                                            changeCallback=self.OnSetDsn)
        dsnDir.SetName('GdalSelect')
        dsnDir.Hide()
        
        dsnDbFile = filebrowse.FileBrowseButton(parent=self, id=wx.ID_ANY, 
                                                size=globalvar.DIALOG_GSELECT_SIZE, labelText='',
                                                dialogTitle=_('Choose file'),
                                                buttonText=_('Browse'),
                                                startDirectory=os.getcwd(),
                                                changeCallback=self.OnSetDsn)
        dsnDbFile.Hide()
        dsnDbFile.SetName('GdalSelect')

        dsnDbText = wx.TextCtrl(parent = self, id = wx.ID_ANY)
        dsnDbText.Hide()
        dsnDbText.Bind(wx.EVT_TEXT, self.OnSetDsn)
        dsnDbText.SetName('GdalSelect')

        dsnDbChoice = wx.Choice(parent = self, id = wx.ID_ANY)
        dsnDbChoice.Hide()
        dsnDbChoice.Bind(wx.EVT_CHOICE, self.OnSetDsn)
        dsnDbChoice.SetName('GdalSelect')

        dsnPro = wx.TextCtrl(parent = self, id = wx.ID_ANY)
        dsnPro.Hide()
        dsnPro.Bind(wx.EVT_TEXT, self.OnSetDsn)
        dsnPro.SetName('GdalSelect')

        # format
        self.format = FormatSelect(parent = self,
                                   ogr = ogr, size = (300, -1))
        self.format.Bind(wx.EVT_CHOICE, self.OnSetFormat)
        self.extension = wx.TextCtrl(parent = self, id = wx.ID_ANY)
        self.extension.Bind(wx.EVT_TEXT, self.OnSetExtension)
        self.extension.Hide()
        
        if ogr:
            fType = 'ogr'
        else:
            fType = 'gdal'
        self.input = { 'file' : [_("File:"),
                                 dsnFile,
                                 GetFormats(writableOnly = dest)[fType]['file']],
                       'dir'  : [_("Name:"),
                                 dsnDir,
                                 GetFormats(writableOnly = dest)[fType]['file']],
                       'db'   : [_("Name:"),
                                 dsnDbText,
                                 GetFormats(writableOnly = dest)[fType]['database']],
                       'pro'  : [_("Protocol:"),
                                 dsnPro,
                                 GetFormats(writableOnly = dest)[fType]['protocol']],
                       'db-win' : { 'file'   : dsnDbFile,
                                    'text'   : dsnDbText,
                                    'choice' : dsnDbChoice },
                       'native' : [_("Name:"), dsnDir, ''],
                       'db-pg' : [_("Name:"),
                                  dsnDbChoice,
                                  ["PostgreSQL"]],
                       }
        
        if self.dest:
            current = RunCommand('v.external.out',
                                 parent = self,
                                 read = True, parse = grass.parse_key_val,
                                 flags = 'g')
            if current['format'] == 'native':
                self.dsnType = 'native'
            elif current['format'] in GetFormats()['ogr']['database']:
                self.dsnType = 'db'
            else:
                self.dsnType = 'dir'
        else:
            self.dsnType = default
        
        self.dsnText = wx.StaticText(parent = self, id = wx.ID_ANY,
                                     label = self.input[self.dsnType][0],
                                     size = (75, -1))
        self.extensionText = wx.StaticText(parent = self, id = wx.ID_ANY,
                                           label = _("Extension:"))
        self.extensionText.Hide()
        
        self.creationOpt = wx.TextCtrl(parent = self, id = wx.ID_ANY)
        if not self.dest:
            self.creationOpt.Hide()
        
        self._layout()
        
        self.OnSetType(event = None, sel = self.sourceMap[self.dsnType])
        if self.dest:
            if current['format'] != 'native':
                self.OnSetFormat(event = None, format = current['format'])
                self.OnSetDsn(event = None, path = current['dsn'])
                self.creationOpt.SetValue(current.get('options', ''))
        else:
            if not ogr:
                self.OnSetFormat(event = None, format = 'GeoTIFF')
            else:
                self.OnSetFormat(event = None, format = 'ESRI Shapefile')
        
    def _layout(self):
        """!Layout"""
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        
        settingsSizer = wx.StaticBoxSizer(self.settingsBox, wx.HORIZONTAL)
        settingsSizer.Add(item = wx.StaticText(parent = self,
                                               id = wx.ID_ANY,
                                               label = _("Load settings:")),
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.RIGHT,
                          border  = 5)
        settingsSizer.Add(item = self.settingsChoice,
                          proportion = 1,
                          flag = wx.EXPAND)
        settingsSizer.Add(item = self.btnSettingsSave,
                          flag = wx.LEFT | wx.RIGHT,
                          border = 5)
        settingsSizer.Add(item = self.btnSettingsDel,
                          flag = wx.RIGHT,
                          border = 5)
        
        inputSizer = wx.StaticBoxSizer(self.inputBox, wx.HORIZONTAL)
        
        self.dsnSizer = wx.GridBagSizer(vgap = 3, hgap = 3)
        self.dsnSizer.AddGrowableCol(3)
        
        row = 0
        self.dsnSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                               label = _("Format:")),
                          flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 0))
        self.dsnSizer.Add(item=self.format,
                          flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 1))
        self.dsnSizer.Add(item = self.extensionText,
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 2))
        self.dsnSizer.Add(item=self.extension,
                          flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 3))
        row += 1
        self.dsnSizer.Add(item = self.dsnText,
                          flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (row, 0))
        self.dsnSizer.Add(item = self.input[self.dsnType][1],
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                          pos = (row, 1), span = (1, 3))
        row += 1
        if self.creationOpt.IsShown():
            self.dsnSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                                   label = _("Creation options:")),
                              flag = wx.ALIGN_CENTER_VERTICAL,
                              pos = (row, 0))
            self.dsnSizer.Add(item = self.creationOpt,
                              flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                              pos = (row, 1), span = (1, 3))
            row += 1
        
        inputSizer.Add(item=self.dsnSizer, proportion = 1,
                       flag=wx.EXPAND | wx.BOTTOM, border = 10)
        
        mainSizer.Add(item=settingsSizer, proportion=0,
                      flag=wx.ALL | wx.EXPAND, border=5)
        mainSizer.Add(item=self.source, proportion=0,
                      flag=wx.LEFT | wx.RIGHT | wx.EXPAND, border=5)
        mainSizer.Add(item=inputSizer, proportion=0,
                      flag=wx.ALL | wx.EXPAND, border=5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def _getExtPatternGlob(self, ext):
        """!Get pattern for case-insensitive globing"""
        pattern = '*.'
        for c in ext:
            pattern += '[%s%s]' % (c.lower(), c.upper())
        return pattern
    
    def _getExtPattern(self, ext):
        """!Get pattern for case-insensitive file mask"""
        return '*.%s;*.%s' % (ext.lower(), ext.upper())

    def OnSettingsLoad(self, event):
        """!Load named settings"""
        name = event.GetString()
        if name not in self._settings:
            GError(parent = self,
                   message = _("Settings <%s> not found") % name)
            return
        data = self._settings[name]
        self.OnSetType(event = None, sel = self.sourceMap[data[0]])
        self.OnSetFormat(event = None, format = data[2])
        self.OnSetDsn(event = None, path = data[1])
        self.creationOpt.SetValue(data[3])
        
    def OnSettingsSave(self, event):
        """!Save settings"""
        dlg = wx.TextEntryDialog(parent = self,
                                 message = _("Name:"),
                                 caption = _("Save settings"))
        if dlg.ShowModal() != wx.ID_OK:
            return
        
        # check required params
        if not dlg.GetValue():
            GMessage(parent = self,
                     message = _("Name not given, settings is not saved."))
            return
        
        if not self.GetDsn():
            GMessage(parent = self,
                     message = _("No data source defined, settings is not saved."))
            return
        
        name = dlg.GetValue()
        
        # check if settings item already exists
        if name in self._settings:
            dlgOwt = wx.MessageDialog(self, message = _("Settings <%s> already exists. "
                                                        "Do you want to overwrite the settings?") % name,
                                      caption = _("Save settings"), style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlgOwt.ShowModal() != wx.ID_YES:
                dlgOwt.Destroy()
                return
        
        self._settings[name] = (self.dsnType, self.GetDsn(),
                                self.format.GetStringSelection(),
                                self.creationOpt.GetValue())
        
        if self._saveSettings() == 0:
            self._settings = self._loadSettings()
            self.settingsChoice.SetStringSelection(name)
        
        dlg.Destroy()
 
    def OnSettingsDelete(self, event):
        """!Save settings"""
        name = self.settingsChoice.GetStringSelection()
        if not name:
            GMessage(parent = self,
                     message = _("No settings is defined. Operation canceled."))
            return
        
        self._settings.pop(name)
        if self._saveSettings() == 0:
            self._settings = self._loadSettings()
        
    def _saveSettings(self):
        """!Save settings into the file

        @return 0 on success
        @return -1 on failure
        """
        try:
            fd = open(self.settingsFile, 'w')
            for key, value in self._settings.iteritems():
                fd.write('%s;%s;%s;%s\n' % (key, value[0], value[1], value[2]))
        except IOError:
            GError(parent = self,
                   message = _("Unable to save settings"))
            return -1
        else:
            fd.close()
        
        return 0

    def _loadSettings(self):
        """!Load settings from the file

        The file is defined by self.SettingsFile.
        
        @return parsed dict
        @return empty dict on error
        """
        data = dict()
        if not os.path.exists(self.settingsFile):
            return data
        
        try:
            fd = open(self.settingsFile, 'r')
            for line in fd.readlines():
                try:
                    lineData = line.rstrip('\n').split(';')
                    if len(lineData) > 4:
                        # type, dsn, format, options
                        data[lineData[0]] = (lineData[1], lineData[2], lineData[3], lineData[4])
                    else:
                        data[lineData[0]] = (lineData[1], lineData[2], lineData[3], '')
                except ValueError:
                    pass
        except IOError:
            return data
        else:
            fd.close()
        
        self.settingsChoice.SetItems(sorted(data.keys()))
        
        return data

    def OnSetType(self, event, sel = None):
        """!Datasource type changed"""
        if event:
            sel = event.GetSelection()
        else:
            self.source.SetSelection(sel)
        win = self.input[self.dsnType][1]
        if win:
            self.dsnSizer.Remove(win)
            win.Hide()
        
        if sel == self.sourceMap['file']:   # file
            self.dsnType = 'file'
            try:
                format = self.input[self.dsnType][2][0]
            except IndexError:
                format = ''
            try:
                ext = self.format.GetExtension(format)
                if not ext:
                    raise KeyError
                format += ' (%s)|%s|%s (*.*)|*.*' % \
                    (self._getExtPattern(ext), self._getExtPattern(ext),
                     _('All files'))
            except KeyError:
                format += '%s (*.*)|*.*' % _('All files')
            
            win = filebrowse.FileBrowseButton(parent=self, id=wx.ID_ANY, 
                                              size=globalvar.DIALOG_GSELECT_SIZE, labelText='',
                                              dialogTitle=_('Choose file to import'),
                                              buttonText=_('Browse'),
                                              startDirectory=os.getcwd(),
                                              changeCallback=self.OnSetDsn,
                                              fileMask = format)
            self.input[self.dsnType][1] = win
        
        elif sel == self.sourceMap['dir']: # directory
            self.dsnType = 'dir'
        elif sel == self.sourceMap['db']:  # database
            self.dsnType = 'db'
        elif sel == self.sourceMap['pro']: # protocol
            self.dsnType = 'pro'
        elif sel == self.sourceMap['native']:
            self.dsnType = 'native'
        elif sel == self.sourceMap['db-pg']: # PostGIS database (PG data provider)
            self.dsnType = 'db-pg'
        
        if self.dsnType == 'db':
            self.input[self.dsnType][1] = self.input['db-win']['text']
        win = self.input[self.dsnType][1]
                
        self.dsnSizer.Add(item = self.input[self.dsnType][1],
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                          pos = (1, 1), span = (1, 3))
        if self.dsnType != 'db-pg':
            win.SetValue('')
        win.Show()
        
        if sel == self.sourceMap['native']: # native
            win.Enable(False)
            self.format.Enable(False)
            self.creationOpt.Enable(False)
            self.parent.btnOk.Enable(True)
        else:
            if not win.IsEnabled():
                win.Enable(True)
            if sel == self.sourceMap['db-pg']:
                if self.format.IsEnabled():
                    self.format.Enable(False)
                self.creationOpt.Enable(True)
            else:
                if not self.format.IsEnabled():
                    self.format.Enable(True)
                    self.creationOpt.Enable(True)
            self.dsnText.SetLabel(self.input[self.dsnType][0])
            items = copy.copy(self.input[self.dsnType][2])
            if sel == self.sourceMap['file']:
                items += ['ZIP', 'GZIP', 'TAR (tar.gz)']
            self.format.SetItems(sorted(items))
            
            if self.parent.GetName() == 'MultiImportDialog':
                self.parent.list.DeleteAllItems()
        
        if sel in (self.sourceMap['file'], self.sourceMap['dir']):
            if not self.ogr:
                self.OnSetFormat(event = None, format = 'GeoTIFF')
            else:
                self.OnSetFormat(event = None, format = 'ESRI Shapefile')
        elif sel == self.sourceMap['db-pg']:
            self.OnSetFormat(event = None, format = 'PostgreSQL')
        
        if sel == self.sourceMap['dir'] and not self.dest:
            if not self.extension.IsShown():
                self.extensionText.Show()
                self.extension.Show()
        else:
            if self.extension.IsShown():
                self.extensionText.Hide()
                self.extension.Hide()
        
        self.dsnSizer.Layout()
        
    def GetDsn(self):
        """!Get datasource name
        """
        if self.format.GetStringSelection() in ('PostgreSQL',
                                                'PostGIS Raster driver'):
            dsn = 'PG:dbname=%s' % self.input[self.dsnType][1].GetStringSelection()
        else:
            dsn = self.input[self.dsnType][1].GetValue()
            # check compressed files
            try:
                ext = os.path.splitext(dsn)[1].lower()
            except KeyError:
                ext = None

            if ext == '.zip':
                dsn = '/vsizip/' + dsn
            elif ext == '.gzip':
                dsn = '/vsigzip/' + dsn
            elif ext in ('.tar', '.tar.gz', '.tgz'):
                dsn = '/vsitar/' + dsn
        
        return dsn
    
    def OnSetDsn(self, event, path = None):
        """!Input DXF file/OGR dsn defined, update list of layer
        widget"""
        if event:
            path = event.GetString()
        else:
            if self.format.GetStringSelection() == 'PostgreSQL':
                for item in path.split(':', 1)[1].split(','):
                    key, value = item.split('=', 1)
                    if key == 'dbname':
                        if not self.input[self.dsnType][1].SetStringSelection(value):
                            GMessage(_("Database <%s> not accessible.") % value,
                                     parent = self)
                        break
            else:
                self.input[self.dsnType][1].SetValue(path)

        if not path:
            if self.dest:
                self.parent.btnOk.Enable(False)
            return
        
        if self.dest:
            self.parent.btnOk.Enable(True)
        else:
            self._reloadLayers()
        
        if event:
            event.Skip()
        
    def _reloadLayers(self):
        """!Reload list of layers"""
        dsn = self.GetDsn()
        if not dsn:
            return
        
        data = list()        
        layerId = 1
        
        if self.ogr:
            ret = RunCommand('v.external',
                             quiet = True,
                             read = True,
                             flags = 't',
                             dsn = dsn)
            if not ret:
                self.parent.list.LoadData()
                if hasattr(self, "btn_run"):
                    self.btn_run.Enable(False)
                return
            
            layerId = 1
            for line in ret.splitlines():
                layerName, featureType = map(lambda x: x.strip(), line.split(' ', 1))
                grassName = GetValidLayerName(layerName)
                data.append((layerId, layerName, featureType, grassName))
                layerId += 1
        else:
            if self.dsnType == 'file':
                baseName = os.path.basename(dsn)
                grassName = GetValidLayerName(baseName.split('.', -1)[0])
                data.append((layerId, baseName, grassName))
            elif self.dsnType == 'dir':
                ext = self.extension.GetValue()
                for filename in glob.glob(os.path.join(dsn, "%s") % self._getExtPatternGlob(ext)):
                    baseName = os.path.basename(filename)
                    grassName = GetValidLayerName(baseName.split('.', -1)[0])
                    data.append((layerId, baseName, grassName))
                    layerId += 1
        if self.ogr:
            dsn += '@OGR'
        
        evt = wxGdalSelect(dsn = dsn)
        evt.SetId(self.input[self.dsnType][1].GetId())
        wx.PostEvent(self.parent, evt)
        
        if self.parent.GetName() == 'MultiImportDialog':
            self.parent.list.LoadData(data)
            if len(data) > 0:
                self.parent.btn_run.Enable(True)
            else:
                self.parent.btn_run.Enable(False)
        
    def OnSetExtension(self, event):
        """!Extension changed"""
        if not self.dest:
            # reload layers
            self._reloadLayers()
        
    def OnSetFormat(self, event, format = None):
        """!Format changed"""
        if self.dsnType not in ['file', 'dir', 'db', 'db-pg']:
            return
        
        win = self.input[self.dsnType][1]
        self.dsnSizer.Remove(win)
        
        if self.dsnType == 'file':
            win.Destroy()
        else: # database
            win.Hide()
        
        if event:
            format = event.GetString()
        else:
            self.format.SetStringSelection(format)
        
        if self.dsnType == 'file':
            try:
                ext = self.format.GetExtension(format)
                if not ext:
                    raise KeyError
                wildcard = format + ' (%s)|%s|%s (*.*)|*.*' % \
                    (self._getExtPattern(ext), self._getExtPattern(ext),
                     _('All files'))
            except KeyError:
                if format == 'ZIP':
                    wildcard = '%s (*.zip;*.ZIP)|*.zip;*.ZIP' % _('ZIP files')
                elif format == 'GZIP':
                    wildcard = '%s (*.gz;*.GZ)|*.gz;*.GZ' % _('GZIP files')
                elif format == 'TAR (tar.gz)':
                    wildcard  = '%s (*.tar;*.TAR)|*.tar;*.TAR|' % _('TAR files')
                    wildcard += '%s (*.tar.gz;*.TAR.GZ;*.tgz;*.TGZ)|*.tar.gz;*.TAR.GZ;*.tgz;*.TGZ' % _('TARGZ files')
                else:
                    wildcard = '%s (*.*)|*.*' % _('All files')
            
            win = filebrowse.FileBrowseButton(parent=self, id=wx.ID_ANY, 
                                              size=globalvar.DIALOG_GSELECT_SIZE, labelText='',
                                              dialogTitle=_('Choose file'),
                                              buttonText=_('Browse'),
                                              startDirectory=os.getcwd(),
                                              changeCallback=self.OnSetDsn,
                                              fileMask = wildcard)
            
        elif self.dsnType == 'dir':
            pass
        
        else: # database
            if format in ('SQLite', 'Rasterlite'):
                win = self.input['db-win']['file']
            elif format in ('PostgreSQL', 'PostGIS WKT Raster driver',
                            'PostGIS Raster driver', 'MSSQLSpatial'):
                win = self.input['db-win']['choice']
                # try to get list of PG databases
                db = RunCommand('db.databases', quiet = True, read = True,
                                driver = 'pg').splitlines()
                if db is not None:
                    win.SetItems(sorted(db))
                elif grass.find_program('psql', ['--help']):
                    if not win.GetItems():
                        p = grass.Popen(['psql', '-ltA'], stdout = grass.PIPE)
                        ret = p.communicate()[0]
                        if ret:
                            db = list()
                            for line in ret.splitlines():
                                sline = line.split('|')
                                if len(sline) < 2:
                                    continue
                                dbname = sline[0]
                                if dbname:
                                    db.append(dbname)
                            win.SetItems(db)
                    if self.dest and win.GetStringSelection():
                        self.parent.btnOk.Enable(True)
                else:
                    win = self.input['db-win']['text']
            else:
                win = self.input['db-win']['text']
        
        self.input[self.dsnType][1] = win
        if not win.IsShown():
            win.Show()
        self.dsnSizer.Add(item = self.input[self.dsnType][1],
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                          pos = (1, 1), span = (1, 3))
        self.dsnSizer.Layout()
        
        # update extension
        self.extension.SetValue(self.GetFormatExt())

        if not self.dest:
            # reload layers
            self._reloadLayers()
        
    def GetType(self):
        """!Get source type"""
        return self.dsnType

    def GetDsnWin(self):
        """!Get list of DSN windows"""
        win = list()
        for stype in ('file', 'dir', 'pro'):
            win.append(self.input[stype][1])
        for stype in ('file', 'text', 'choice'):
            win.append(self.input['db-win'][stype])
        
        return win
    
    def GetFormat(self):
        """!Get format as string"""
        return self.format.GetStringSelection().replace(' ', '_')
    
    def GetFormatExt(self):
        """!Get format extension"""
        return self.format.GetExtension(self.format.GetStringSelection())
    
    def GetOptions(self):
        """!Get creation options"""
        if not self.creationOpt.IsShown():
            return ''
        
        return self.creationOpt.GetValue()
    
class ProjSelect(wx.ComboBox):
    """!Widget for selecting input raster/vector map used by
    r.proj/v.proj modules."""
    def __init__(self, parent, isRaster, id = wx.ID_ANY, size = globalvar.DIALOG_COMBOBOX_SIZE,
                 **kwargs):
        super(ProjSelect, self).__init__(parent, id, size = size, 
                                         style = wx.CB_READONLY, **kwargs)
        self.SetName("ProjSelect")
        self.isRaster = isRaster
        
    def UpdateItems(self, dbase, location, mapset):
        """!Update list of maps
        
        """
        if not dbase:
            dbase = grass.gisenv()['GISDBASE']
        if not mapset:
            mapset = grass.gisenv()['MAPSET']
        if self.isRaster:
            ret = RunCommand('r.proj',
                             quiet = True,
                             read = True,
                             flags = 'l',
                             dbase = dbase,
                             location = location,
                             mapset = mapset)
        else:
            ret = RunCommand('v.proj',
                             quiet = True,
                             read = True,
                             flags = 'l',
                             dbase = dbase,
                             location = location,
                             mapset = mapset)
        listMaps = list()
        if ret:
            for line in ret.splitlines():
                listMaps.append(line.strip())
        ListSortLower(listMaps)
        
        self.SetItems(listMaps)
        self.SetValue('')

class ElementSelect(wx.Choice):
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_COMBOBOX_SIZE, 
                 **kwargs):
        """!Widget for selecting GIS element
        
        @param parent parent window
        """
        super(ElementSelect, self).__init__(parent, id, size = size, 
                                            **kwargs)
        self.SetName("ElementSelect")
        
        task = gtask.parse_interface('g.list')
        p = task.get_param(value = 'type')
        self.values = p.get('values', [])
        self.valuesDesc = p.get('values_desc', [])
        
        self.SetItems(self.valuesDesc)

    def GetValue(self, name):
        """!Translate value

        @param name element name
        """
        idx = self.valuesDesc.index(name)
        if idx > -1:
            return self.values[idx]
        return ''

class OgrTypeSelect(wx.Panel):
    def __init__(self, parent, panel, **kwargs):
        """!Widget to choose OGR feature type

        @param parent parent window
        @param panel wx.Panel instance used as parent window
        """
        wx.Panel.__init__(self, parent = panel, id = wx.ID_ANY)
        
        self.ftype = wx.Choice(parent = self, id = wx.ID_ANY,
                               size = (200, -1),
                               choices = (_("Point"), _("LineString"), _("Polygon")))
        self._layout()

    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.HORIZONTAL)
        sizer.Add(item = wx.StaticText(parent = self,
                                       id = wx.ID_ANY,
                                       label = _("Feature type:")),
                  proportion = 1,
                  flag = wx.ALIGN_CENTER_VERTICAL,
                  border  = 5)
        sizer.Add(item = self.ftype,
                  proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_RIGHT)
        
        self.SetSizer(sizer)
        sizer.Fit(self)

    def GetType(self):
        """!Get selected type as string

        @return feature type as string
        """
        sel = self.ftype.GetSelection()
        if sel == 0:
            return 'point'
        elif sel == 1:
            return 'line'
        elif sel == 2:
            return 'boundary'

class CoordinatesSelect(wx.Panel):
    def __init__(self, parent, giface, multiple = False, **kwargs):
        """!Widget to get coordinates from map window  by mouse click
        
        @param parent parent window
        @param giface GRASS interface
        @param multiple - True if it is possible to insert more coordinates
        """
        self._giface = giface
        self.multiple = multiple
        self.mapWin   = None
        
        super(CoordinatesSelect, self).__init__(parent = parent, id = wx.ID_ANY)
        
        self.coordsField = wx.TextCtrl(parent = self, id = wx.ID_ANY, 
                                       size = globalvar.DIALOG_TEXTCTRL_SIZE)
        
        icon = wx.Bitmap(os.path.join(globalvar.ETCICONDIR, "grass", "pointer.png"))
        self.buttonInsCoords = buttons.ThemedGenBitmapToggleButton(parent = self, id = wx.ID_ANY,
                                                                   bitmap = icon,
                                                                   size = globalvar.DIALOG_COLOR_SIZE)
        self.registered = False
        self.buttonInsCoords.Bind(wx.EVT_BUTTON, self._onClick)
        self._doLayout()
        
    def _doLayout(self):
        self.dialogSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.dialogSizer.Add(item = self.coordsField,  
                             proportion = 1, 
                             flag = wx.EXPAND)
        self.dialogSizer.Add(item = self.buttonInsCoords)
        self.SetSizer(self.dialogSizer)
        
    def _onClick(self, event):
        """!Button for interacitve inserting of coordinates clicked"""
        self.mapWin = self._giface.GetMapWindow()
        if self.buttonInsCoords.GetToggle() and self.mapWin:
            if self.mapWin.RegisterMouseEventHandler(wx.EVT_LEFT_DOWN, 
                                                     self._onMapClickHandler,
                                                     wx.StockCursor(wx.CURSOR_CROSS)) == False:
                self.buttonInsCoords.SetToggle(False)
                return
            
            self.registered = True
            self._giface.GetMapDisplay().Raise()
        else:
            if self.mapWin and \
                    self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN,  
                                                            self._onMapClickHandler):
                self.registered = False
                return
            
            self.buttonInsCoords.SetToggle(False)           
    
    def _onMapClickHandler(self, event):
        """!Gets coordinates from mapwindow"""
        if event == "unregistered":
            if self.buttonInsCoords:
                self.buttonInsCoords.SetToggle(False)
            return
        
        e, n = self.mapWin.GetLastEN()
        prevCoords = ""
        
        if self.multiple:
            prevCoords = self.coordsField.GetValue().strip()
            if prevCoords != "":
                prevCoords += ","
        
        value = prevCoords + str(e) + "," + str(n)
        self.coordsField.SetValue(value)
        
    def __del__(self):
        """!Unregistrates _onMapClickHandler from mapWin"""
        if self.mapWin and self.registered:
            self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN,  
                                                    self._onMapClickHandler)

    def GetTextWin(self):
        """!Get TextCtrl widget"""
        return self.coordsField

class SignatureSelect(wx.ComboBox):
    """!Widget for selecting signatures"""
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE, 
                 **kwargs):
        super(SignatureSelect, self).__init__(parent, id, size = size, 
                                              **kwargs)
        self.SetName("SignatureSelect")

    def Insert(self, group, subgroup = None):
        """!Insert signatures for defined group/subgroup

        @param group group name (can be fully-qualified)
        @param subgroup non fully-qualified name of subgroup
        """
        if not group:
            return
        gisenv = grass.gisenv()
        try:
            name, mapset = group.split('@', 1)
        except ValueError:
            name = group
            mapset = gisenv['MAPSET']
        
        path = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'], mapset,
                            'group', name)
        
        if subgroup:
            path = os.path.join(path, 'subgroup', subgroup)
        try:
            items = list()
            for element in os.listdir(os.path.join(path, 'sig')):
                items.append(element)
            self.SetItems(items)
        except OSError:
            self.SetItems([])
        self.SetValue('')
