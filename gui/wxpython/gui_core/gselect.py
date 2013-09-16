"""!
@package gui_core.gselect

@brief Custom control that selects elements

Classes:
 - gselect::Select
 - gselect::VectorSelect
 - gselect::ListCtrlComboPopup
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

(C) 2007-2013 by the GRASS Development Team 

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
@author Stepan Turek <stepan.turek seznam.cz> (CoordinatesSelect, ListCtrlComboPopup)
"""

import os
import sys
import glob
import copy

import wx
import wx.combo
import wx.lib.buttons          as  buttons
import wx.lib.filebrowsebutton as filebrowse

from core import globalvar

import grass.script as grass
from   grass.script import task as gtask

from gui_core.widgets  import ManageSettingsWidget

from core.gcmd     import RunCommand, GError, GMessage
from core.utils    import GetListOfLocations, GetListOfMapsets, \
                          GetFormats, rasterFormatExtension, vectorFormatExtension
from core.utils    import GetSettingsPath, GetValidLayerName, ListSortLower
from core.utils    import GetVectorNumberOfLayers, _
from core.settings import UserSettings
from core.debug    import Debug

from grass.pydispatch.signal import Signal

class Select(wx.combo.ComboCtrl):
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                 type = None, multiple = False, nmaps = 1,
                 mapsets = None, updateOnPopup = True, onPopup = None,
                 fullyQualified = True, extraItems = {},
                 validator = wx.DefaultValidator):
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
        @param extraItems extra items to add (given as dictionary) - see gmodeler for usage
        @param validator validator for TextCtrl
        """
        wx.combo.ComboCtrl.__init__(self, parent=parent, id=id, size=size, validator=validator)
        self.GetChildren()[0].SetName("Select")
        self.GetChildren()[0].type = type
        
        self.tcp = TreeCtrlComboPopup()
        self.SetPopupControl(self.tcp)
        self.SetPopupExtents(0, 100)
        if type:
            self.tcp.SetData(type = type, mapsets = mapsets,
                             multiple = multiple, nmaps = nmaps,
                             updateOnPopup = updateOnPopup, onPopup = onPopup,
                             fullyQualified = fullyQualified, extraItems = extraItems)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)

    def OnKeyDown(self, event):
        """!Open popup and send key events to the tree."""
        if self.IsPopupShown():
            self.tcp.OnKeyDown(event)
        else:
            if event.GetKeyCode() == wx.WXK_DOWN:
                self.ShowPopup()
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
        select vector maps. You can filter the vector maps. If you
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

class ListCtrlComboPopup(wx.combo.ComboPopup):
    """!Create a list ComboBox using TreeCtrl with hidden root.
    """    
    # overridden ComboPopup methods
    def Init(self):
        self.value = [] # for multiple is False -> len(self.value) in [0,1]
        self.curitem = None
        self.multiple = False
        self.updateOnPopup = True

    def Create(self, parent):
        self.seltree = wx.TreeCtrl(parent, style=wx.TR_HIDE_ROOT
                                   |wx.TR_HAS_BUTTONS
                                   |wx.TR_SINGLE
                                   |wx.TR_LINES_AT_ROOT
                                   |wx.SIMPLE_BORDER
                                   |wx.TR_FULL_ROW_HIGHLIGHT)
        self.seltree.Bind(wx.EVT_MOTION, self.OnMotion)
        self.seltree.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        # the following dummy handler are needed to keep tree events
        # from propagating up to the parent GIS Manager layer tree
        self.seltree.Bind(wx.EVT_TREE_ITEM_EXPANDING, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_ITEM_COLLAPSED, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_SEL_CHANGED, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_DELETE_ITEM, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_BEGIN_DRAG, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, lambda x: None)
        

    def GetControl(self):
        return self.seltree

    def GetStringValue(self):
        """!Get value as a string separated by commas
        """
        return ','.join(self.value)

    def SetStringValue(self, value):
        # this assumes that item strings are unique...
        root = self.seltree.GetRootItem()
        if not root:
            return
        winValue = self.GetCombo().GetValue().strip(',')
        self.value = []
        if winValue:
            self.value = winValue.split(',')

    def OnPopup(self, force = False):
        """!Limited only for first selected
        """
        if not force and not self.updateOnPopup:
            return

        # selects map starting according to written text
        inputText = self.GetCombo().GetValue().strip()
        if inputText:
            root = self.seltree.GetRootItem()
            match = self.FindItem(root, inputText, startLetters = True)
            self.seltree.EnsureVisible(match)
            self.seltree.SelectItem(match)

    def GetAdjustedSize(self, minWidth, prefHeight, maxHeight):
        """!Reads UserSettings to get height (which was 200 in old implementation).
        """
        height = UserSettings.Get(group = 'appearance', key = 'gSelectPopupHeight', subkey = 'value')
        return wx.Size(minWidth, min(height, maxHeight))

    def FindItem(self, parentItem, text, startLetters = False):
        """!Finds item with given name or starting with given text
        """
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

    def AddItem(self, value):
        root = self.seltree.GetRootItem()
        if not root:
            root = self.seltree.AddRoot("<hidden root>")
        self.seltree.AppendItem(root, text = value)

    def OnKeyUp(self, event):
        """!Enable to select items using keyboard
        """
        item = self.seltree.GetSelection()
        if event.GetKeyCode() == wx.WXK_DOWN:
            self.seltree.SelectItem(self.seltree.GetNextVisible(item))
            
        elif event.GetKeyCode() == wx.WXK_UP: 
            itemPrev = self.seltree.GetPrevSibling(item)
            self.seltree.SelectItem(itemPrev)

        elif event.GetKeyCode() == wx.WXK_ESCAPE:
            self.Dismiss()

        elif event.GetKeyCode() == wx.WXK_RETURN:
            self.seltree.SelectItem(item)
            self.curitem = item
            item_str = self.seltree.GetItemText(self.curitem)
            if self.multiple:
                if item_str not in self.value: 
                    self.value.append(item_str)
            else:   
                self.value = [item_str]
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
        if self.curitem is None:
            return

        item_str = self.seltree.GetItemText(self.curitem)
        if self.multiple:
            if item_str not in self.value: 
                self.value.append(item_str)
        else:
            self.value = [item_str]
        self.Dismiss()
        
        evt.Skip()

    def SetData(self, **kargs):
        """!Set object properties"""
        if 'multiple' in kargs:
            self.multiple = kargs['multiple']
        if 'onPopup' in kargs:
            self.onPopup = kargs['onPopup']

    def DeleteAllItems(self):
        """!Delete all items in popup"""
        self.seltree.DeleteAllItems()

class TreeCtrlComboPopup(ListCtrlComboPopup):
    """!Create a tree ComboBox for selecting maps and other GIS elements
    in accessible mapsets within the current location
    """
    # overridden ComboPopup methods
    def Init(self):

        ListCtrlComboPopup.Init(self)
        self.nmaps = 1
        self.type = None
        self.mapsets = None
        self.onPopup = None
        self.fullyQualified = True
        self.extraItems = dict()
        
        self.SetFilter(None)
    
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
        
        ListCtrlComboPopup.OnPopup(self, force)
      
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
            import grass.temporal as tgis
            filesdict = tgis.tlist_grouped(elementdict[element], element == 'stds')
        else:
            if globalvar.have_mlist:
                filesdict = grass.mlist_grouped(elementdict[element],
                                                check_search_path = False)
            else:
                filesdict = grass.list_grouped(elementdict[element],
                                               check_search_path = False)
        
        # add extra items first
        if self.extraItems:
            for group, items in self.extraItems.iteritems():
                node = self.AddItem(group, node = True)
                self.seltree.SetItemTextColour(node, wx.Colour(50, 50, 200))
                for item in items:
                    self.AddItem(item, node = False, parent = node)
                self.seltree.ExpandAllChildren(node)
        
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
    
    def AddItem(self, value, mapset = None, node = True, parent = None):
        if not parent:
            root = self.seltree.GetRootItem()
            if not root:
                root = self.seltree.AddRoot("<hidden root>")
            parent = root

        data = {'node': node, 'mapset': mapset}
        item = self.seltree.AppendItem(parent, text = value, data = wx.TreeItemData(data))
        return item

    def OnKeyDown(self, event):
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
                if self.fullyQualified and self.seltree.GetPyData(item)['mapset']:
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
            if self.fullyQualified and self.seltree.GetPyData(item)['mapset']:
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
        ListCtrlComboPopup.SetData(self, **kargs)
        if 'type' in kargs:
            self.type = kargs['type']
            if self.type in ('stds', 'strds', 'str3ds', 'stvds'):
                import grass.temporal as tgis
                tgis.init()
        if 'mapsets' in kargs:
            self.mapsets = kargs['mapsets']
        if 'nmaps' in kargs:
            self.nmaps = kargs['nmaps']
        if 'updateOnPopup' in kargs:
            self.updateOnPopup = kargs['updateOnPopup']
        if 'fullyQualified' in kargs:
            self.fullyQualified = kargs['fullyQualified']
        if 'extraItems' in kargs:
            self.extraItems = kargs['extraItems']
        
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

        if vector:
            layers = GetVectorNumberOfLayers(vector)

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

        if self.all:
            layers.insert(0, '-1')

        if len(layers) > 0:
            self.SetItems(layers)
        else:
            self.Clear()

        self.SetValue("")

        if self.default and self.default in layers:
            self.SetValue(self.default)

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

class ColumnSelect(wx.combo.ComboCtrl):
    """!Creates combo box for selecting columns in the attribute table
    for a vector map.

    @param parent window parent
    @param id window id
    @param value default value
    @param size window size
    @param vector vector map name
    @param layer layer number
    @param multiple - True if it is possible to add multiple columns
    @param param parameters list (see menuform.py)
    @param **kwags wx.ComboBox parameters
    """
    def __init__(self, parent, id = wx.ID_ANY, value = '', 
                 size = globalvar.DIALOG_COMBOBOX_SIZE,
                 vector = None, layer = 1, multiple = False, 
                 param = None, **kwargs):
        self.defaultValue = value
        self.param = param
        
        wx.combo.ComboCtrl.__init__(self, parent, id, size = size, **kwargs)
        self.GetChildren()[0].SetName("ColumnSelect")
        self.GetChildren()[0].type = type

        self.tcp = ListCtrlComboPopup()
        self.SetPopupControl(self.tcp)
        self.tcp.SetData(multiple = multiple)

        if vector:
            self.InsertColumns(vector, layer)
        self.GetChildren()[0].Bind(wx.EVT_KEY_UP, self.OnKeyUp)
     
    def OnKeyUp(self, event):
        """!Shows popupwindow if down arrow key is released"""
        if event.GetKeyCode() == wx.WXK_DOWN and not self.IsPopupShown():
            self.ShowPopup() 
        else:
            event.Skip()

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
            try:
                layer = int(layer)
            except TypeError:
                # assuming layer 1
                layer = 1
            table = dbInfo.GetTable(layer)
            columnchoices = dbInfo.GetTableDesc(table)
            keyColumn = dbInfo.GetKeyColumn(layer)
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

        # update list
        self.tcp.DeleteAllItems()
        for col in columns:
            self.tcp.AddItem(col)

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
        
        # update list
        self.tcp.DeleteAllItems()
        self.SetValue(self.defaultValue)
        
        for col in columns:
            self.tcp.AddItem(col)
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
        
        mlist = RunCommand('i.group', group=group,
                           read=True, flags='sg').splitlines()
        try:
            self.SetItems(mlist)
        except OSError:
            self.SetItems([])
        self.SetValue('')

class FormatSelect(wx.Choice):
    def __init__(self, parent, srcType, ogr=False,
                 size=globalvar.DIALOG_SPIN_SIZE, 
                 **kwargs):
        """!Widget for selecting external (GDAL/OGR) format

        @param parent parent window
        @param srcType source type ('file', 'database', 'protocol')
        @param ogr True for OGR otherwise GDAL
        """
        super(FormatSelect, self).__init__(parent, id=wx.ID_ANY, size=size, 
                                           **kwargs)
        self.SetName("FormatSelect")
        
        if ogr:
            ftype = 'ogr'
        else:
            ftype = 'gdal'

        formats = list()
        for f in GetFormats()[ftype][srcType].values():
            formats += f
        self.SetItems(formats)
        
    def GetExtension(self, name):
        """!Get file extension by format name"""
        formatToExt = dict()
        formatToExt.update(rasterFormatExtension)
        formatToExt.update(vectorFormatExtension)
        
        return formatToExt.get(name, '')

# unused code since r47938
# wxGdalSelect, EVT_GDALSELECT = NewEvent()

class GdalSelect(wx.Panel):
    def __init__(self, parent, panel, ogr=False, link=False, dest=False, 
                 exclude=None):
        """!Widget for selecting GDAL/OGR datasource, format
        
        @param parent parent window
        @param ogr    use OGR selector instead of GDAL
        @param dest   True for output (destination)
        @param default deafult type (ignored when dest == True)
        @param exclude list of types to be excluded
        """
        self.parent = parent
        self.ogr = ogr
        self.dest = dest 
        self._sourceType = None

        wx.Panel.__init__(self, parent=panel)

        self.reloadDataRequired = Signal('GdalSelect.reloadDataRequired')

        if self.ogr:
            settingsFile = os.path.join(GetSettingsPath(), 'wxOGR')
        else:
            settingsFile = os.path.join(GetSettingsPath(), 'wxGDAL')

        self.settsManager = ManageSettingsWidget(parent=self, 
                                                 settingsFile=settingsFile)
        self.settsManager.settingsChanged.connect(self.OnSettingsChanged)
        self.settsManager.settingsSaving.connect(self.OnSettingsSaving)

        self.inputBox = wx.StaticBox(parent=self)
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
        if exclude is None:
            exclude = []
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
        self.sourceMapByIdx = {}
        for name, idx in self.sourceMap.items():
            self.sourceMapByIdx[ idx ] = name

        self.source = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                  style=wx.RA_SPECIFY_COLS,
                                  choices=sources)
        if dest:
            self.source.SetLabel(" %s " % _('Output type'))
        else:
            self.source.SetLabel(" %s " % _('Source type'))
        
        self.source.SetSelection(0)
        self.source.Bind(wx.EVT_RADIOBOX,
                         lambda evt: self.SetSourceType(self.sourceMapByIdx[evt.GetInt()]))
        
        
        self.nativeWidgets = {}
        self.fileWidgets = {}
        self.dirWidgets = {}
        self.dbWidgets = {}
        self.protocolWidgets = {}
        self.pgWidgets = {}

        if ogr:
            fType = 'ogr'
        else:
            fType = 'gdal'

        # file
        fileMask = '%(all)s (*.*)|*.*|' % {'all': _('All files')}
        for name, ext in sorted(rasterFormatExtension.items()):
            fileMask += '%(name)s (*.%(low)s;*.%(up)s)|*.%(low)s;*.%(up)s|' % {'name': name,
                                                                               'low': ext.lower(),
                                                                               'up': ext.upper()}
        fileMask += '%s (*.zip;*.ZIP)|*.zip;*.ZIP|' % _('ZIP files')
        fileMask += '%s (*.gz;*.GZ)|*.gz;*.GZ|' % _('GZIP files')
        fileMask += '%s (*.tar;*.TAR)|*.tar;*.TAR|' % _('TAR files')
        fileMask += '%s (*.tar.gz;*.TAR.GZ;*.tgz;*.TGZ)|*.tar.gz;*.TAR.GZ;*.tgz;*.TGZ' % _('TARGZ files')
        # only contains formats with extensions hardcoded    

        self.filePanel = wx.Panel(parent=self)
        browse = filebrowse.FileBrowseButton(parent=self.filePanel, id=wx.ID_ANY, 
                                             size=globalvar.DIALOG_GSELECT_SIZE,
                                             labelText = _('File:'),
                                             dialogTitle=_('Choose file to import'),
                                             buttonText=_('Browse'),
                                             startDirectory=os.getcwd(),
                                             changeCallback=self.OnUpdate,
                                             fileMask=fileMask)
        self.fileWidgets['browse'] = browse
        self.fileWidgets['options'] = wx.TextCtrl(parent=self.filePanel)

        # directory
        self.dirPanel = wx.Panel(parent=self)
        browse = filebrowse.DirBrowseButton(parent=self.dirPanel, id=wx.ID_ANY, 
                                            size=globalvar.DIALOG_GSELECT_SIZE,
                                            labelText=_('Directory:'),
                                            dialogTitle=_('Choose input directory'),
                                            buttonText=_('Browse'),
                                            startDirectory=os.getcwd(),
                                            changeCallback=self.OnUpdate)

        self.dirWidgets['browse'] = browse
        formatSelect = wx.Choice(parent=self.dirPanel, size=(300, -1))
        self.dirWidgets['format'] = formatSelect
        fileFormats = GetFormats(writableOnly=dest)[fType]['file']
        formatSelect.SetItems(sorted(list(fileFormats)))
        formatSelect.Bind(wx.EVT_CHOICE, lambda evt: self.SetExtension(self.dirWidgets['format'].GetStringSelection()))
        formatSelect.Bind(wx.EVT_CHOICE, self.OnUpdate)

        self.dirWidgets['extensionLabel'] = wx.StaticText(parent=self.dirPanel,
                                                          label = _("Extension:"))
        self.dirWidgets['extension'] = wx.TextCtrl(parent=self.dirPanel)
        self.dirWidgets['extension'].Bind(wx.EVT_TEXT, self.ExtensionChanged)
        self.dirWidgets['options'] = wx.TextCtrl(parent=self.dirPanel)
        if self.ogr:
            shapefile = 'ESRI Shapefile'
            if shapefile in fileFormats:
                formatSelect.SetStringSelection(shapefile)
                self.SetExtension(shapefile)
        else:
            tiff = 'GeoTIFF'
            if tiff in fileFormats:
                formatSelect.SetStringSelection(tiff)
                self.SetExtension(tiff)

        # database
        self.dbPanel = wx.Panel(parent=self)
        self.dbFormats = GetFormats(writableOnly=dest)[fType]['database']
        dbChoice = wx.Choice(parent=self.dbPanel, choices=self.dbFormats)
        dbChoice.Bind(wx.EVT_CHOICE, lambda evt: self.SetDatabase(db=dbChoice.GetStringSelection()))
        self.dbWidgets['format'] = dbChoice

        browse = filebrowse.FileBrowseButton(parent=self.dbPanel, id=wx.ID_ANY, 
                                             size=globalvar.DIALOG_GSELECT_SIZE,
                                             labelText=_("Name:"),
                                             dialogTitle=_('Choose file'),
                                             buttonText=_('Browse'),
                                             startDirectory=os.getcwd(),
                                             changeCallback=self.OnUpdate)
        self.dbWidgets['browse'] = browse
        self.dbWidgets['choice'] = wx.Choice(parent=self.dbPanel)
        self.dbWidgets['choice'].Bind(wx.EVT_CHOICE, self.OnUpdate)
        self.dbWidgets['text'] = wx.TextCtrl(parent=self.dbPanel)
        self.dbWidgets['text'].Bind(wx.EVT_TEXT, self.OnUpdate)
        self.dbWidgets['textLabel1'] = wx.StaticText(parent=self.dbPanel, label=_("Name:"))
        self.dbWidgets['textLabel2'] = wx.StaticText(parent=self.dbPanel, label=_("Name:"))
        browse = filebrowse.DirBrowseButton(parent=self.dbPanel, id=wx.ID_ANY, 
                                            size=globalvar.DIALOG_GSELECT_SIZE,
                                            labelText=_('Directory:'),
                                            dialogTitle=_('Choose input directory'),
                                            buttonText=_('Browse'),
                                            startDirectory=os.getcwd(),
                                            changeCallback=self.OnUpdate)
        self.dbWidgets['dirbrowse'] = browse
        self.dbWidgets['options'] = wx.TextCtrl(parent=self.dbPanel)

        # protocol
        self.protocolPanel = wx.Panel(parent=self)
        protocolFormats = GetFormats(writableOnly=self.dest)[fType]['protocol']
        protocolChoice = wx.Choice(parent=self.protocolPanel, choices=protocolFormats)
        self.protocolWidgets['format'] = protocolChoice   

        self.protocolWidgets['text'] = wx.TextCtrl(parent=self.protocolPanel)  
        self.protocolWidgets['text'].Bind(wx.EVT_TEXT, self.OnUpdate)                   
        self.protocolWidgets['options'] = wx.TextCtrl(parent=self.protocolPanel)

        # native
        self.nativePanel = wx.Panel(parent=self)

        self._layout()
        sourceType = 'file'
        self.SetSourceType(sourceType) # needed always to fit dialog size
        if self.dest:
            current = RunCommand('v.external.out',
                                 parent=self,
                                 read=True, parse=grass.parse_key_val,
                                 flags='g')
            if current['format'] == 'native':
                sourceType = 'native'
            elif current['format'] in GetFormats()['ogr']['database']:
                sourceType = 'db'
            else:
                sourceType = 'dir'

        if self.dest:
            wx.CallAfter(self._postInit, sourceType,
                         current['format'], current.get('dsn', ''), current.get('options', ''))


    def _postInit(self, sourceType, format, dsn, options):
        """!Fill in default values."""
        self.SetSourceType(sourceType)
        self.source.SetSelection(self.sourceMap[sourceType])

        dsn = os.path.expandvars(dsn) # v.external.out uses $HOME
        # fill in default values
        if sourceType == 'dir':
            self.dirWidgets['format'].SetStringSelection(format)
            self.dirWidgets['browse'].SetValue(dsn)
            self.dirWidgets['options'].SetValue(options)
        elif sourceType == 'db':
            self.dbWidgets['format'].SetStringSelection(format)
            self.dbWidgets['options'].SetValue(options)
            name = self._getCurrentDbWidgetName()
            if name == 'choice':
                if dsn in self.dbWidgets[name].GetItems():
                    self.dbWidgets[name].SetStringSelection(dsn)
            else:
                self.dbWidgets[name].SetValue(dsn)

    def _layout(self):
        """!Layout"""
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        self.changingSizer = wx.StaticBoxSizer(self.inputBox, wx.VERTICAL)

        # file
        paddingSizer = wx.BoxSizer(wx.VERTICAL)
        sizer = wx.GridBagSizer(vgap=5, hgap=10)
        paddingSizer.Add(item=self.fileWidgets['browse'],
                         flag=wx.BOTTOM | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                         border=30)
        sizer.Add(item=paddingSizer, flag=wx.EXPAND, pos=(0, 0), span=(1, 2))
        sizer.AddGrowableCol(0)
        if self.dest:
            sizer.Add(item=wx.StaticText(parent=self.filePanel,
                                         label = _("Creation options:")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (1, 0))
            sizer.Add(item=self.fileWidgets['options'],
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                      pos=(1, 1))
        else:
            self.fileWidgets['options'].Hide()
        self.filePanel.SetSizer(sizer)

        # directory
        sizer = wx.GridBagSizer(vgap=3, hgap=10)
        sizer.Add(item=wx.StaticText(parent=self.dirPanel,
                                       label = _("Format:")),
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(0, 0))
        sizer.Add(item=self.dirWidgets['format'],
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                  pos=(0, 1))
        sizer.Add(item=self.dirWidgets['extensionLabel'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 2))
        sizer.Add(item=self.dirWidgets['extension'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 3))
        sizer.Add(item=self.dirWidgets['browse'],
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                  pos=(1, 0), span=(1, 4))       
        if self.dest:
            sizer.Add(item=wx.StaticText(parent=self.dirPanel,
                                         label = _("Creation options:")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (2, 0))
            sizer.Add(item=self.dirWidgets['options'],
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                      pos=(2, 1), span=(1, 3))
            self.dirWidgets['extensionLabel'].Hide()
            self.dirWidgets['extension'].Hide()
        else:
            self.dirWidgets['options'].Hide()
        sizer.AddGrowableCol(1)
        self.dirPanel.SetSizer(sizer)

        # database
        sizer = wx.GridBagSizer(vgap=1, hgap=10)
        sizer.Add(item=wx.StaticText(parent=self.dbPanel,
                                       label = _("Format:")),
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(0, 0))
        sizer.Add(item=self.dbWidgets['format'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 1))
        sizer.Add(item=self.dbWidgets['textLabel1'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(1, 0))
        sizer.Add(item=self.dbWidgets['text'],
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                  pos=(1, 1))
        sizer.Add(item=self.dbWidgets['browse'],
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                  pos=(2, 0), span=(1, 2))
        sizer.Add(item=self.dbWidgets['dirbrowse'],
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                  pos=(3, 0), span=(1, 2))
        sizer.Add(item=self.dbWidgets['textLabel2'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(4, 0))
        sizer.Add(item=self.dbWidgets['choice'],
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                  pos=(4, 1))
        if self.dest:
            sizer.Add(item=wx.StaticText(parent=self.dbPanel,
                                         label = _("Creation options:")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (5, 0))
            sizer.Add(item=self.dbWidgets['options'],
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                      pos=(5, 1))
        
        else:
            self.dbWidgets['options'].Hide()

        self.dbPanel.SetSizer(sizer)
        sizer.SetEmptyCellSize((0, 0))
        sizer.AddGrowableCol(1)

        # protocol
        sizer = wx.GridBagSizer(vgap=3, hgap=3)
        sizer.Add(item=wx.StaticText(parent=self.protocolPanel,
                                       label = _("Format:")),
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(0, 0))
        sizer.Add(item=self.protocolWidgets['format'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 1))
        sizer.Add(item=wx.StaticText(parent=self.protocolPanel,
                                     label = _("Protocol:")),
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(1, 0))
        sizer.Add(item=self.protocolWidgets['text'],
                  flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                  pos=(1, 1))
        if self.dest:
            sizer.Add(item=wx.StaticText(parent=self.protocolPanel,
                                         label = _("Creation options:")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (2, 0))
            sizer.Add(item=self.protocolWidgets['options'],
                      flag=wx.ALIGN_CENTER_VERTICAL|wx.EXPAND,
                      pos=(2, 1))

        else:
            self.protocolWidgets['options'].Hide()
        sizer.AddGrowableCol(1)
        self.protocolPanel.SetSizer(sizer)

        # native
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(item=wx.StaticText(parent=self.nativePanel,
                                     label = _("No settings available")),
                          flag=wx.ALIGN_CENTER_VERTICAL|wx.ALL|wx.EXPAND, border=5)
        self.nativePanel.SetSizer(sizer)

        for panel in (self.nativePanel, self.filePanel,
                      self.dirPanel, self.dbPanel,
                      self.protocolPanel):
            
            self.changingSizer.Add(item=panel, proportion=1,
                                   flag=wx.EXPAND)
        
        mainSizer.Add(item=self.settsManager, proportion=0,
                      flag=wx.ALL | wx.EXPAND, border=5)
        mainSizer.Add(item=self.source, proportion=0,
                      flag=wx.LEFT | wx.RIGHT | wx.EXPAND, border=5)
        mainSizer.Add(item=self.changingSizer, proportion=1,
                      flag=wx.ALL | wx.EXPAND, border=5)
        self.mainSizer = mainSizer
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def _getExtension(self, name):
        """!Get file extension by format name"""
        formatToExt = dict()
        formatToExt.update(rasterFormatExtension)
        formatToExt.update(vectorFormatExtension)

        return formatToExt.get(name, '')        

    def SetSourceType(self, sourceType):
        """!Set source type (db, file, dir, ...).
        Does not switch radioboxes."""
        self._sourceType = sourceType
        self.changingSizer.Show(item=self.filePanel, show=(sourceType == 'file'))
        self.changingSizer.Show(item=self.nativePanel, show=(sourceType == 'native'))
        self.changingSizer.Show(item=self.dirPanel, show=(sourceType == 'dir'))
        self.changingSizer.Show(item=self.protocolPanel, show=(sourceType == 'pro'))
        self.changingSizer.Show(item=self.dbPanel, show=(sourceType in ('db', 'db-pg')))

        self.changingSizer.Layout()

        if sourceType == 'db':
            self.dbWidgets['format'].SetItems(self.dbFormats)
            if self.dbFormats:
                if 'PostgreSQL' in self.dbFormats:                
                    self.dbWidgets['format'].SetStringSelection('PostgreSQL')
                else:
                    self.dbWidgets['format'].SetSelection(0)
            self.dbWidgets['format'].Enable()

        if sourceType == 'db-pg':
            self.dbWidgets['format'].SetItems(['PostgreSQL'])
            self.dbWidgets['format'].SetSelection(0)
            self.dbWidgets['format'].Disable()

        if sourceType in ('db','db-pg'):
            db = self.dbWidgets['format'].GetStringSelection()
            self.SetDatabase(db)

        self.reloadDataRequired.emit(data=None)
        self._reloadLayers()

    def OnSettingsChanged(self, data):
        """!User changed setting"""
        # data list: [type, dsn, format, options]
        if len(data) == 3:
            data.append('')
        elif len < 3:
            return     

        self.source.SetSelection(self.sourceMap[data[0]]) 
        self.SetSourceType(data[0])
        if data[0] == 'file':
            self.fileWidgets['browse'].SetValue(data[1])
            self.fileWidgets['options'].SetValue(data[3])
        elif data[0] == 'dir':
            self.dirWidgets['browse'].SetValue(data[1])
            self.dirWidgets['format'].SetStringSelection(data[2])
            self.dirWidgets['options'].SetValue(data[3])
            self.SetExtension(data[2])
        elif data[0] == 'pro':
            self.protocolWidgets['text'].SetValue(data[2])
            self.protocolWidgets['options'].SetValue(data[3])
        elif data[0] in ('db', 'db-pg'):
            name = self._getCurrentDbWidgetName()
            if name =='choice':
                if len(data[1].split(':', 1)) > 1:
                    for item in data[1].split(':', 1)[1].split(','):
                        key, value = item.split('=', 1)
                        if key == 'dbname':
                            self.dbWidgets[name].SetStringSelection(value)
                            break
                else:
                    self.dbWidgets[name].SetStringSelection(data[1])
            else:
                self.dbWidgets[name].SetValue(data[1])
            self.dbWidgets['options'].SetValue(data[3])
        
        self.reloadDataRequired.emit(data=None)
        self._reloadLayers()

    def OnSettingsSaving(self, name):
        """!Saving data"""
        if not self.GetDsn():
            GMessage(parent = self,
                     message = _("No data source defined, settings are not saved."))
            return

        self.settsManager.SetDataToSave((self._sourceType, self.GetDsn(),
                                         self.GetFormat(), self.GetOptions()))
        self.settsManager.SaveSettings(name)

    def _getExtPatternGlob(self, ext):
        """!Get pattern for case-insensitive globing"""
        pattern = '*.'
        for c in ext:
            pattern += '[%s%s]' % (c.lower(), c.upper())
        return pattern

    def _getCurrentDbWidgetName(self):
        """!Returns active dns database widget name."""
        for widget in ('browse', 'dirbrowse', 'text', 'choice'):
            if self.dbWidgets[widget].IsShown():
                return widget

    def GetDsn(self):
        """!Get datasource name
        """
        if self._sourceType in ('db', 'db-pg'):
            if self.dbWidgets['format'].GetStringSelection() in ('PostgreSQL',
                                                                 'PostGIS Raster driver'):

                dsn = 'PG:dbname=%s' % self.dbWidgets['choice'].GetStringSelection()
            else:
                name = self._getCurrentDbWidgetName()
                if name == 'choice':
                    dsn = self.dbWidgets[name].GetStringSelection()
                else:
                    dsn = self.dbWidgets[name].GetValue()

        else:
            if self._sourceType == 'file':
                dsn = self.fileWidgets['browse'].GetValue()
            elif self._sourceType == 'dir':
                dsn = self.dirWidgets['browse'].GetValue()
            elif self._sourceType == 'pro':
                dsn = self.protocolWidgets['text'].GetValue()
            else:
                dsn = ''
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

    def SetDatabase(self, db):
        """!Update database panel."""
        sizer = self.dbPanel.GetSizer()
        showBrowse = db in ('SQLite', 'Rasterlite')
        showDirbrowse = db in ('FileGDB')
        showChoice = db in ('PostgreSQL','PostGIS WKT Raster driver',
                            'PostGIS Raster driver')

        showText = not(showBrowse or showChoice or showDirbrowse)
        sizer.Show(self.dbWidgets['browse'], show=showBrowse)
        sizer.Show(self.dbWidgets['dirbrowse'], show=showDirbrowse)
        sizer.Show(self.dbWidgets['choice'], show=showChoice)
        sizer.Show(self.dbWidgets['textLabel2'], show=showChoice)
        sizer.Show(self.dbWidgets['text'], show=showText)
        sizer.Show(self.dbWidgets['textLabel1'], show=showText)
        if showChoice:
            # try to get list of PG databases
            dbNames = RunCommand('db.databases', quiet=True, read=True,
                            driver='pg').splitlines()
            if dbNames is not None:
                self.dbWidgets['choice'].SetItems(sorted(dbNames))
                self.dbWidgets['choice'].SetSelection(0)
            elif grass.find_program('psql', '--help'):
                if not self.dbWidgets['choice'].GetItems():
                    p = grass.Popen(['psql', '-ltA'], stdout = grass.PIPE)
                    ret = p.communicate()[0]
                    if ret:
                        dbNames = list()
                        for line in ret.splitlines():
                            sline = line.split('|')
                            if len(sline) < 2:
                                continue
                            dbname = sline[0]
                            if dbname:
                                dbNames.append(dbname)
                        self.dbWidgets['choice'].SetItems(db)
                        self.dbWidgets['choice'].SetSelection(0)
            else:
                sizer.Show(self.dbWidgets['text'])
                sizer.Show(self.dbWidgets['choice'], False)

        sizer.Layout()

    def OnUpdate(self, event):
        """!Update required - load layers."""
        self._reloadLayers()

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
                self.reloadDataRequired.emit(data=None)
                return

            layerId = 1
            for line in ret.splitlines():
                layerName, featureType, projection = map(lambda x: x.strip(), line.split(','))
                if projection == '0':
                    projectionMatch = _("No")
                else:
                    projectionMatch = _("Yes")
                grassName = GetValidLayerName(layerName)
                data.append((layerId, layerName, featureType, projectionMatch, grassName))
                layerId += 1
        else:
            if self._sourceType == 'file':
                baseName = os.path.basename(dsn)
                grassName = GetValidLayerName(baseName.split('.', -1)[0])
                data.append((layerId, baseName, grassName))
            elif self._sourceType == 'dir':
                ext = self.dirWidgets['extension'].GetValue()
                for filename in glob.glob(os.path.join(dsn, "%s") % self._getExtPatternGlob(ext)):
                    baseName = os.path.basename(filename)
                    grassName = GetValidLayerName(baseName.split('.', -1)[0])
                    data.append((layerId, baseName, grassName))
                    layerId += 1
# unused code since r47938
#        if self.ogr:
#            dsn += '@OGR'
#        
#        evt = wxGdalSelect(dsn = dsn)
#        evt.SetId(self.input[self.dsnType][1].GetId())
#        wx.PostEvent(self.parent, evt)

        if self.parent.GetName() == 'MultiImportDialog':
            self.reloadDataRequired.emit(data=data)

    def ExtensionChanged(self, event):
        if not self.dest:
            # reload layers
            self._reloadLayers()

    def SetExtension(self, name):
        """!Extension changed"""
        ext = self._getExtension(name)
        self.dirWidgets['extension'].SetValue(ext)

    def GetType(self):
        """!Get source type"""
        return self._sourceType

    def GetFormat(self):
        """!Get format as string"""
        if self._sourceType == 'dir':
            format = self.dirWidgets['format'].GetStringSelection()
        elif self._sourceType == 'pro':
            format = self.protocolWidgets['format'].GetStringSelection()
        elif self._sourceType in ('db', 'db-pg'):
            format = self.dbWidgets['format'].GetStringSelection()
        else:
            format = ''

        return format.replace(' ', '_')

    def GetFormatExt(self):
        """!Get format extension"""
        return self._getExtension(self.GetFormat())

    def GetOptions(self):
        """!Get creation options"""
        if self._sourceType == 'file':
            options = self.fileWidgets['options'].GetValue()
        elif self._sourceType == 'dir':
            options = self.dirWidgets['options'].GetValue()
        elif self._sourceType == 'pro':
            options = self.protocolWidgets['options'].GetValue()
        elif self._sourceType in ('db', 'db-pg'):
            options = self.dbWidgets['options'].GetValue()

        return options


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
        switcher = self._giface.GetMapDisplay().GetToolSwitcher()
        switcher.AddCustomToolToGroup(group='mouseUse',
                                      btnId=self.buttonInsCoords.GetId(), 
                                      toggleHandler=self.buttonInsCoords.SetValue)
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
        switcher = self._giface.GetMapDisplay().GetToolSwitcher()
        switcher.ToolChanged(self.buttonInsCoords.GetId())
        self.mapWin = self._giface.GetMapWindow()
        if self.buttonInsCoords.GetToggle() and self.mapWin:
            if self.mapWin.RegisterMouseEventHandler(wx.EVT_LEFT_DOWN, 
                                                     self._onMapClickHandler,
                                                     'cross') == False:
                return
            
            self.registered = True
            self._giface.GetMapDisplay().Raise()
        else:
            if self.mapWin and \
                    self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN,  
                                                            self._onMapClickHandler):
                self.registered = False
                return

    def _onMapClickHandler(self, event):
        """!Gets coordinates from mapwindow"""
        if event == "unregistered":
            return
        
        e, n = self.mapWin.GetLastEN()
        prevCoords = ""
        
        if self.multiple:
            prevCoords = self.coordsField.GetValue().strip()
            if prevCoords != "":
                prevCoords += ","
        
        value = prevCoords + str(e) + "," + str(n)
        self.coordsField.SetValue(value)

    def OnClose(self):
        """!Unregistrates _onMapClickHandler from mapWin"""
        switcher = self._giface.GetMapDisplay().GetToolSwitcher()
        switcher.RemoveCustomToolFromGroup(self.buttonInsCoords.GetId())
        if self.mapWin and self.registered:
            self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN,  
                                                    self._onMapClickHandler)

    def GetTextWin(self):
        """!Get TextCtrl widget"""
        return self.coordsField

class SignatureSelect(wx.ComboBox):
    """!Widget for selecting signatures"""
    def __init__(self, parent, element, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE, 
                 **kwargs):
        super(SignatureSelect, self).__init__(parent, id, size = size, 
                                              **kwargs)
        self.element = element
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
            for element in os.listdir(os.path.join(path, self.element)):
                items.append(element)
            self.SetItems(items)
        except OSError:
            self.SetItems([])
        self.SetValue('')
