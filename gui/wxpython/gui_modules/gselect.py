"""!
@package gselect

@brief Custom control that selects elements

Classes:
 - Select
 - VectorSelect
 - TreeCrtlComboPopup
 - VectorDBInfo
 - LayerSelect
 - LayerNameSelect
 - DriverSelect
 - DatabaseSelect
 - ColumnSelect
 - DbaseSelect
 - LocationSelect
 - MapsetSelect
 - SubGroupSelect
 - FormatSelect
 - GdalSelect
 - ProjSelect
 
(C) 2007-2011 by the GRASS Development Team This program is free
software under the GNU General Public License (>=v2). Read the file
COPYING that comes with GRASS for details.

@author Michael Barton
@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import glob

import wx
import wx.combo
import wx.lib.filebrowsebutton as filebrowse
from wx.lib.newevent import NewEvent

import globalvar

sys.path.append(os.path.join(globalvar.ETCDIR, "python"))
import grass.script as grass

import gcmd
import utils
from preferences import globalSettings as UserSettings
from debug       import Debug

wxGdalSelect, EVT_GDALSELECT = NewEvent()

class Select(wx.combo.ComboCtrl):
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                 type = None, multiple = False, mapsets = None,
                 updateOnPopup = True, onPopup = None):
        """!Custom control to create a ComboBox with a tree control to
        display and select GIS elements within acessible mapsets.
        Elements can be selected with mouse. Can allow multiple
        selections, when argument multiple=True. Multiple selections
        are separated by commas.

        @param type type of GIS elements ('raster, 'vector', ...)
        @param multiple multiple input allowed?
        @param mapsets force list of mapsets (otherwise search path)
        @param updateOnPopup True for updating list of elements on popup
        @param onPopup function to be called on Popup
        """
        wx.combo.ComboCtrl.__init__(self, parent=parent, id=id, size=size)
        self.GetChildren()[0].SetName("Select")
        self.GetChildren()[0].type = type
        
        self.tcp = TreeCtrlComboPopup()
        self.SetPopupControl(self.tcp)
        self.SetPopupExtents(0, 100)
        if type:
            self.tcp.SetData(type = type, mapsets = mapsets,
                             multiple = multiple,
                             updateOnPopup = updateOnPopup, onPopup = onPopup)
        self.GetChildren()[0].Bind(wx.EVT_KEY_UP, self.OnKeyUp)
     
    def OnKeyUp(self, event):
        """!Shows popupwindow if down arrow key is released"""
        if event.GetKeyCode() == wx.WXK_DOWN:
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
    
class VectorSelect(Select):
    def __init__(self, parent, ftype, **kwargs):
        """!Custom to create a ComboBox with a tree control to display and
        select vector maps. Control allows to filter vector maps. If you
        don't need this feature use Select class instead
        
        @ftype filter vector maps based on feature type
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
        self.type = None
        self.mapsets = None
        self.updateOnPopup = True
        self.onPopup = None
        
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
        str = ""
        for value in self.value:
            str += value + ","
        str = str.rstrip(',')
        
        return str

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
        found = self.FindItem(root, value)
        if found:
            self.value.append(found)
            self.seltree.SelectItem(found)

    def GetAdjustedSize(self, minWidth, prefHeight, maxHeight):
        return wx.Size(minWidth, min(200, maxHeight))

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
        
        # list of mapsets in current location
        if mapsets is None:
            mapsets = utils.ListOfMapsets(get = 'accessible')
        
        # map element types to g.mlist types
        elementdict = {'cell':'rast',
                       'raster':'rast',
                       'rast':'rast',
                       'raster files':'rast',
                       'grid3':'rast3d',
                       'rast3d':'rast3d',
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
                       '3D view parameters':'3dview'}
        
        if element not in elementdict:
            self.AddItem(_('Not selectable element'))
            return
        
        # get directory tree nodes
        # reorder mapsets based on search path (TODO)
        for i in range(len(mapsets)):
            if i > 0 and mapsets[i] == curr_mapset:
                mapsets[i] = mapsets[0]
                mapsets[0] = curr_mapset
        
        if globalvar.have_mlist:
            filesdict = grass.mlist_grouped(elementdict[element])
        else:
            filesdict = grass.list_grouped(elementdict[element])
        
        first_mapset = None
        for mapset in mapsets:
            mapset_node = self.AddItem('Mapset: ' + mapset)
            if not first_mapset:
                first_mapset = mapset_node
            
            self.seltree.SetItemTextColour(mapset_node, wx.Colour(50, 50, 200))
            if mapset not in filesdict:
                continue
            try:
                elem_list = filesdict[mapset]
                elem_list.sort(key = unicode.lower)
                for elem in elem_list:
                    if elem != '':
                        fullqElem = elem + '@' + mapset
                        if elements:
                            if (exclude and fullqElem in elements) or \
                                    (not exclude and fullqElem not in elements):
                                continue
                        
                        if self.filterElements:
                            if self.filterElements(fullqElem):
                                self.AddItem(elem, parent=mapset_node)
                        else:
                            self.AddItem(elem, parent=mapset_node)
            except StandardError, e:
                sys.stderr.write(_("GSelect: invalid item: %s") % e)
                continue
            
            if self.seltree.ItemHasChildren(mapset_node):
                sel = UserSettings.Get(group='general', key='elementListExpand',
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
                    self.seltree.Collapse(mapset_node)
                else:
                    self.seltree.Expand(mapset_node)
        
        if first_mapset:
            # select first mapset (MSW hack)
            self.seltree.SelectItem(first_mapset)
    
    # helpers
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
            elif startletters and self.seltree.GetItemText(item).startswith(text.split('@')[0]):
                return item
            item, cookie = self.seltree.GetNextChild(parentItem, cookie)
        return wx.TreeItemId()
    
    def AddItem(self, value, parent=None):
        if not parent:
            root = self.seltree.GetRootItem()
            if not root:
                root = self.seltree.AddRoot("<hidden root>")
            parent = root

        item = self.seltree.AppendItem(parent, text=value)
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
            if self.seltree.GetRootItem() == self.seltree.GetItemParent(item):
                self.value = [] 
            else:
                mapsetItem = self.seltree.GetItemParent(item)
                fullName = self.seltree.GetItemText(item) + '@' \
                            + self.seltree.GetItemText(mapsetItem).split(' ', 1)[1]
                if self.multiple is True:
                    # text item should be unique
                    self.value.append(fullName)
                else:
                    self.value = [fullName, ]
            
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
            
            if self.seltree.GetRootItem() == self.seltree.GetItemParent(item):
                self.value = [] # cannot select mapset item
            else:
                mapsetItem = self.seltree.GetItemParent(item)
                fullName = self.seltree.GetItemText(item) + '@' + \
                    self.seltree.GetItemText(mapsetItem).split(' ', 1)[1]
                if self.multiple is True:
                    # text item should be unique
                    self.value.append(fullName)
                else:
                    self.value = [fullName, ]
            
            self.Dismiss()
        
        evt.Skip()

    def SetData(self, **kargs):
        """!Set object properties"""
        if 'type' in kargs:
            self.type = kargs['type']
        if 'mapsets' in kargs:
            self.mapsets = kargs['mapsets']
        if 'multiple' in kargs:
            self.multiple = kargs['multiple']
        if 'updateOnPopup' in kargs:
            self.updateOnPopup = kargs['updateOnPopup']
        if 'onPopup' in kargs:
            self.onPopup = kargs['onPopup']
        
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
        self.layers = grass.vector_db(map=self.map, stderr=nuldev)
        nuldev.close()
        
        if (len(self.layers.keys()) == 0):
            return False

        return True

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
            columns = self.tables[table]
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
        return self.layers[layer]['key']
    
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

class LayerSelect(wx.Choice):
    def __init__(self, parent, id = wx.ID_ANY, 
                 size=globalvar.DIALOG_LAYER_SIZE,
                 vector = None, choices = [], all = False, default = None):
        """!Creates widget for selecting vector map layer numbers

        @param vector  vector map name or None
        @param choices list of predefined choices
        @param all     adds layer '-1' (e.g., for d.vect)
        @param default default layer number
        """

        super(LayerSelect, self).__init__(parent, id, size = size,
                                          choices = choices)

        self.all = all
        
        self.SetName("LayerSelect")

        # default value
        self.default = default
            
        if len(choices) > 1:
            return

        if vector:
            self.InsertLayers(vector)
        else:
            if all:
                self.SetItems(['-1', '1'])
            else:
                self.SetItems(['1'])
            self.SetStringSelection('1')
        
    def InsertLayers(self, vector):
        """!Insert layers for a vector into the layer combobox"""
        layerchoices = utils.GetVectorNumberOfLayers(vector)
        
        if self.all:
            layerchoices.insert(0, '-1')
        if len(layerchoices) > 1:
            self.SetItems(layerchoices)
            self.SetStringSelection('1')
        else:
            self.SetItems(['1'])
            self.SetStringSelection('1')
        
        if self.default:
            self.SetStringSelection(str(self.default))

class LayerNameSelect(wx.ComboBox):
    def __init__(self, parent, id = wx.ID_ANY,
                 size = globalvar.DIALOG_COMBOBOX_SIZE,
                 vector = None, dsn = None):
        """!Creates combo box for selecting vector map layer names

        @param vector vector map name (native or connected via v.external)
        @param dsn    OGR data source name
        """
        super(LayerNameSelect, self).__init__(parent, id, size = size)
        self.SetName("LayerNameSelect")

        if vector:
            # -> native
            self.InsertLayers(vector = vector)
        elif dsn:
            self.InsertLayers(dsn = dsn)
        
    def InsertLayers(self, vector = None, dsn = None):
        """!Insert layers for a vector into the layer combobox

        @todo Implement native format
        
        @param vector vector map name (native or connected via v.external)
        @param dsn    OGR data source name
        """
        layers = list()
        if vector:
            # TODO
            pass
        elif dsn:
            ret = gcmd.RunCommand('v.in.ogr',
                                  read = True,
                                  quiet = True,
                                  flags = 'l',
                                  dsn = dsn)
            if ret:
                layers = ret.splitlines()
        
        self.SetItems(layers)
        self.SetSelection(0)
        
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
    def __init__(self, parent, value='',
                 id=wx.ID_ANY, pos=wx.DefaultPosition,
                 size=globalvar.DIALOG_TEXTCTRL_SIZE, **kargs):
        
        super(DatabaseSelect, self).__init__(parent, id, value, pos, size)
                               
        self.SetName("DatabaseSelect")

class TableSelect(wx.ComboBox):
    """!Creates combo box for selecting attribute tables from the database
    """
    def __init__(self, parent,
                 id=wx.ID_ANY, value='', pos=wx.DefaultPosition,
                 size=globalvar.DIALOG_COMBOBOX_SIZE,
                 choices=[]):

        super(TableSelect, self).__init__(parent, id, value, pos, size, choices,
                                          style=wx.CB_READONLY)

        self.SetName("TableSelect")

        if not choices:
            self.InsertTables()
                
    def InsertTables(self, driver=None, database=None):
        """!Insert attribute tables into combobox"""
        items = []

        if not driver or not database:
            connect = grass.db_connection()
            
            driver = connect['driver']
            database = connect['database']
        
        ret = gcmd.RunCommand('db.tables',
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
                 size=globalvar.DIALOG_COMBOBOX_SIZE,
                 vector = None, layer = 1, param = None, **kwargs):
        self.defaultValue = value
        self.param = param
        
        super(ColumnSelect, self).__init__(parent, id, value, size = size, **kwargs)
        self.SetName("ColumnSelect")
        
        if vector:
            self.InsertColumns(vector, layer)
                
    def InsertColumns(self, vector, layer, excludeKey = False, type = None, dbInfo = None):
        """!Insert columns for a vector attribute table into the columns combobox

        @param vector vector name
        @param layer vector layer number
        @param excludeKey exclude key column from the list?
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
            if type: # only selected column types
                for key, value in columnchoices.iteritems():
                    if value['type'] not in type:
                        columns.remove(key)
        except (KeyError, ValueError):
            columns = list()
        
        self.SetItems(columns)
        self.SetValue(self.defaultValue)
        
        if self.param:
            self.param['value'] = ''
        
    def InsertTableColumns(self, table, driver=None, database=None):
        """!Insert table columns

        @param table table name
        @param driver driver name
        @param database database name
        """
        columns = list()
        
        ret = gcmd.RunCommand('db.columns',
                              read = True,
                              driver = driver,
                              database = database,
                              table = table)
        
        if ret:
            columns = ret.splitlines()
        
        self.SetItems(columns)
        self.SetValue(self.defaultValue)
        
        if self.param:
            self.param['value'] = ''

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
        
        self.SetItems(utils.GetListOfLocations(self.gisdbase))

    def UpdateItems(self, dbase):
        """!Update list of locations

        @param dbase path to GIS database
        """
        self.gisdbase = dbase
        if dbase:
            self.SetItems(utils.GetListOfLocations(self.gisdbase))
        else:
            self.SetItems([])
        
class MapsetSelect(wx.ComboBox):
    """!Widget for selecting GRASS mapset"""
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_COMBOBOX_SIZE, 
                 gisdbase = None, location = None, setItems = True, **kwargs):
        super(MapsetSelect, self).__init__(parent, id, size = size, 
                                           style = wx.CB_READONLY, **kwargs)
        
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
            self.SetItems(utils.GetListOfMapsets(self.gisdbase, self.location, selectable = False)) # selectable

    def UpdateItems(self, location, dbase = None):
        """!Update list of mapsets for given location

        @param dbase path to GIS database (None to use currently selected)
        @param location name of location
        """
        if dbase:
            self.gisdbase = dbase
        self.location = location
        if location:
            self.SetItems(utils.GetListOfMapsets(self.gisdbase, self.location, selectable = False))
        else:
            self.SetItems([])
        
class SubGroupSelect(wx.ComboBox):
    """!Widget for selecting subgroups"""
    def __init__(self, parent, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE, 
                 **kwargs):
        super(SubGroupSelect, self).__init__(parent, id, size = size, 
                                             style = wx.CB_READONLY, **kwargs)
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
                 sourceType = None, id = wx.ID_ANY, size = globalvar.DIALOG_COMBOBOX_SIZE, 
                 **kwargs):
        """!Widget for selecting external (GDAL/OGR) format

        @param parent parent window
        @param sourceType source type ('file', 'directory', 'database', 'protocol') or None
        @param ogr True for OGR otherwise GDAL
        """
        super(FormatSelect, self).__init__(parent, id, size = size, 
                                           style = wx.CB_READONLY, **kwargs)
        self.SetName("FormatSelect")
        
        if ogr:
            ftype = 'ogr'
        else:
            ftype = 'gdal'
        
        formats = list()
        for f in utils.GetFormats()[ftype].values():
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
            'VFK'            : 'vfk'
            }
        
        try:
            return formatToExt[name]
        except KeyError:
            return ''
        
class GdalSelect(wx.Panel):
    def __init__(self, parent, panel, ogr = False,
                 default = 'file',
                 exclude = [],
                 envHandler = None):
        """!Widget for selecting GDAL/OGR datasource, format
        
        @param parent parent window
        @param ogr    use OGR selector instead of GDAL
        """
        self.parent = parent
        self.ogr    = ogr
        wx.Panel.__init__(self, parent = panel, id = wx.ID_ANY)

        self.settingsBox = wx.StaticBox(parent = self, id=wx.ID_ANY,
                                        label=" %s " % _("Settings"))
        
        self.inputBox = wx.StaticBox(parent = self, id=wx.ID_ANY,
                                     label=" %s " % _("Source name"))
        
        # source type
        sources = list()
        self.sourceMap = { 'file' : -1,
                           'dir'  : -1,
                           'db'   : -1,
                           'pro'  : -1 }
        idx = 0
        if 'file' not in exclude:
            sources.append(_("File"))
            self.sourceMap['file'] = idx
            idx += 1
        if 'directory' not in exclude:
            sources.append(_("Directory"))
            self.sourceMap['dir'] = idx
            idx += 1
        if 'database' not in exclude:
            sources.append(_("Database"))
            self.sourceMap['db'] = idx
            idx += 1
        if 'protocol' not in exclude:
            sources.append(_("Protocol"))
            self.sourceMap['pro'] = idx
        
        if self.ogr:
            self.settingsFile = os.path.join(os.path.expanduser("~"), '.grass7', 'wxOGR')
        else:
            self.settingsFile = os.path.join(os.path.expanduser("~"), '.grass7', 'wxGDAL')
        
        self._settings = self._loadSettings()
        self.settingsChoice = wx.Choice(parent = self, id = wx.ID_ANY)
        self.settingsChoice.Bind(wx.EVT_CHOICE, self.OnSettingsLoad)
        self.settingsChoice.SetItems(self._settings.keys())
        self.btnSettings = wx.Button(parent = self, id = wx.ID_SAVE)
        self.btnSettings.Bind(wx.EVT_BUTTON, self.OnSettingsSave)
        
        self.source = wx.RadioBox(parent = self, id = wx.ID_ANY,
                                  label = _('Source type'),
                                  style = wx.RA_SPECIFY_COLS,
                                  choices = sources)
        self.source.SetSelection(0)
        self.source.Bind(wx.EVT_RADIOBOX, self.OnSetType)
        
        # dsn widgets
        if not ogr:
            filemask = 'GeoTIFF (%s)|%s' % (self._getExtPattern('tif'), self._getExtPattern('tif'))
        else:
            filemask = 'ESRI Shapefile (%s)|%s' % (self._getExtPattern('shp'), self._getExtPattern('shp'))
        
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
                                   ogr = ogr)
        self.format.Bind(wx.EVT_CHOICE, self.OnSetFormat)
        
        if ogr:
            fType = 'ogr'
        else:
            fType = 'gdal'
        self.input = { 'file' : [_("File:"),
                                 dsnFile,
                                 utils.GetFormats()[fType]['file']],
                       'dir'  : [_("Directory:"),
                                 dsnDir,
                                 utils.GetFormats()[fType]['file']],
                       'db'   : [_("Database:"),
                                 dsnDbFile,
                                 utils.GetFormats()[fType]['database']],
                       'pro'  : [_("Protocol:"),
                                 dsnPro,
                                 utils.GetFormats()[fType]['protocol']],
                       'db-win' : { 'file'   : dsnDbFile,
                                    'text'   : dsnDbText,
                                    'choice' : dsnDbChoice },
                       }
        
        self.dsnType = default
        self.input[self.dsnType][1].Show()
        self.format.SetItems(self.input[self.dsnType][2])
        
        if not ogr:
            self.format.SetStringSelection('GeoTIFF')
        else:
            self.format.SetStringSelection('ESRI Shapefile')
        
        self.dsnText = wx.StaticText(parent = self, id = wx.ID_ANY,
                                     label = self.input[self.dsnType][0],
                                     size = (75, -1))
        self.formatText = wx.StaticText(parent = self, id = wx.ID_ANY,
                                        label = _("Format:"))
        self._layout()
        
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
        settingsSizer.Add(item = self.btnSettings,
                          flag = wx.LEFT,
                          border = 5)
        
        inputSizer = wx.StaticBoxSizer(self.inputBox, wx.HORIZONTAL)
        
        self.dsnSizer = wx.GridBagSizer(vgap=3, hgap=3)
        self.dsnSizer.AddGrowableRow(1)
        self.dsnSizer.AddGrowableCol(1)
        
        self.dsnSizer.Add(item=self.dsnText,
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos = (0, 0))
        self.dsnSizer.Add(item=self.input[self.dsnType][1],
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                          pos = (0, 1))
        
        self.dsnSizer.Add(item=self.formatText,
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos = (1, 0))
        self.dsnSizer.Add(item=self.format,
                          flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (1, 1))
        
        inputSizer.Add(item=self.dsnSizer, proportion=1,
                       flag=wx.EXPAND | wx.ALL)

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
            gcmd.GError(parent = self,
                        message = _("Settings named '%s' not found") % name)
            return
        data = self._settings[name]
        self.OnSetType(event = None, sel = self.sourceMap[data[0]])
        self.OnSetFormat(event = None, format = data[2])
        self.OnSetDsn(event = None, path = data[1])
        
    def OnSettingsSave(self, event):
        """!Save settings"""
        dlg = wx.TextEntryDialog(parent = self,
                                 message = _("Name:"),
                                 caption = _("Save settings"))
        if dlg.ShowModal() != wx.ID_OK:
            return
        
        if not dlg.GetValue():
            gcmd.GMessage(parent = self,
                          message = _("Name not given, settings is not saved."))
            return

        data = self._loadSettings()
        try:
            fd = open(self.settingsFile, 'w')
            fd.write(dlg.GetValue() + ';' + self.dsnType + ';' +
                     self._getDsn() + ';' +
                     self.format.GetStringSelection())
            fd.write('\n')
        except IOError:
            gcmd.GError(parent = self,
                        message = _("Unable to save settings."))
            return
        fd.close()
        dlg.Destroy()
                
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
                    name, ftype, dsn, format = line.rstrip('\n').split(';')
                    data[name] = (ftype, dsn, format)
                except ValueError:
                    pass
        except IOError:
            return data
        
        fd.close()
        
        return data

    def OnSetType(self, event, sel = None):
        """!Datasource type changed"""
        if event:
            sel = event.GetSelection()
        else:
            self.source.SetSelection(sel)
        
        win = self.input[self.dsnType][1]
        self.dsnSizer.Remove(win)
        win.Hide()
        if sel == self.sourceMap['file']:   # file
            self.dsnType = 'file'
            format = self.input[self.dsnType][2][0]
            try:
                ext = self.format.GetExtension(format)
                if not ext:
                    raise KeyError
                format += ' (%s)|%s' % (self._getExtPattern(ext), self._getExtPattern(ext))
            except KeyError:
                format += ' (*.*)|*.*'
            
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
        elif sel == self.sourceMap['db']: # database
            self.dsnType = 'db'
        elif sel == self.sourceMap['pro']: # protocol
            self.dsnType = 'pro'
        
        self.dsnText.SetLabel(self.input[self.dsnType][0])
        if self.parent.GetName() == 'MultiImportDialog':
            self.parent.list.DeleteAllItems()
        self.format.SetItems(self.input[self.dsnType][2])        
        
        if sel in (self.sourceMap['file'],
                   self.sourceMap['dir']):
            win = self.input[self.dsnType][1]
            self.dsnSizer.Add(item=self.input[self.dsnType][1],
                              flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                              pos = (0, 1))
            win.SetValue('')
            win.Show()
            
            if not self.ogr:
                self.format.SetStringSelection('GeoTIFF')
            else:
                self.format.SetStringSelection('ESRI Shapefile')
        elif sel == self.sourceMap['pro']:
            win = self.input[self.dsnType][1]
            self.dsnSizer.Add(item=self.input[self.dsnType][1],
                              flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                              pos = (0, 1))
            win.SetValue('')
            win.Show()
        
        self.dsnSizer.Layout()
        
    def _getDsn(self):
        """!Get datasource name"""
        if self.format.GetStringSelection() == 'PostgreSQL':
            return 'PG:dbname=%s' % self.input[self.dsnType][1].GetStringSelection()
        
        return self.input[self.dsnType][1].GetValue()
    
    def OnSetDsn(self, event, path = None):
        """!Input DXF file/OGR dsn defined, update list of layer widget"""
        if event:
            path = event.GetString()
        else:
            if self.format.GetStringSelection() == 'PostgreSQL':
                for item in path.split(':', 1)[1].split(','):
                    key, value = item.split('=', 1)
                    if key == 'dbname':
                        self.input[self.dsnType][1].SetStringSelection(value)
                        break
            else:
                self.input[self.dsnType][1].SetValue(path)
        
        if not path:
            return 
        
        data = list()        
        
        layerId = 1
        dsn = self._getDsn()
        
        if self.dsnType == 'file':
            baseName = os.path.basename(dsn)
            grassName = utils.GetValidLayerName(baseName.split('.', -1)[0])
            data.append((layerId, baseName, grassName))
        elif self.dsnType == 'dir':
            try:
                ext = self.format.GetExtension(self.format.GetStringSelection())
            except KeyError:
                ext = ''
            for file in glob.glob(os.path.join(dsn, "%s") % self._getExtPatternGlob(ext)):
                baseName = os.path.basename(file)
                grassName = utils.GetValidLayerName(baseName.split('.', -1)[0])
                data.append((layerId, baseName, grassName))
                layerId += 1
        elif self.dsnType == 'db':
            ret = gcmd.RunCommand('v.in.ogr',
                                  quiet = True,
                                  read = True,
                                  flags = 'l',
                                  dsn = dsn)
            if not ret:
                self.parent.list.LoadData()
                if hasattr(self, "btn_run"):
                    self.btn_run.Enable(False)
                return
            layerId = 1
            for line in ret.splitlines():
                layerName = line.strip()
                grassName = utils.GetValidLayerName(layerName)
                data.append((layerId, layerName.strip(), grassName.strip()))
                layerId += 1
        
        evt = wxGdalSelect(dsn = dsn + '@OGR')
        evt.SetId(self.input[self.dsnType][1].GetId())
        wx.PostEvent(self.parent, evt)
        
        if self.parent.GetName() == 'MultiImportDialog':
            self.parent.list.LoadData(data)
            if len(data) > 0:
                self.parent.btn_run.Enable(True)
            else:
                self.parent.btn_run.Enable(False)
        
        if event:
            event.Skip()
        
    def OnSetFormat(self, event, format = None):
        """!Format changed"""
        if self.dsnType not in ['file', 'db']:
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
                format += ' (%s)|%s' % (self._getExtPattern(ext), self._getExtPattern(ext))
            except KeyError:
                format += ' (*.*)|*.*'
            
            win = filebrowse.FileBrowseButton(parent=self, id=wx.ID_ANY, 
                                              size=globalvar.DIALOG_GSELECT_SIZE, labelText='',
                                              dialogTitle=_('Choose file'),
                                              buttonText=_('Browse'),
                                              startDirectory=os.getcwd(),
                                              changeCallback=self.OnSetDsn,
                                              fileMask = format)
        else: # database
            if format == 'SQLite' or format == 'Rasterlite':
                win = self.input['db-win']['file']
            elif format == 'PostgreSQL' or format == 'PostGIS WKT Raster driver':
                if grass.find_program('psql'):
                    win = self.input['db-win']['choice']
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
                else:
                    win = self.input['db-win']['text']
            else:
                win = self.input['db-win']['text']
        
        self.input[self.dsnType][1] = win
        if not win.IsShown():
            win.Show()
        self.dsnSizer.Add(item=self.input[self.dsnType][1],
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                          pos = (0, 1))
        self.dsnSizer.Layout()

    def GetType(self):
        """!Get source type"""
        return self.dsnType

    def GetDsn(self):
        """!Get DSN"""
        if self.format.GetStringSelection() == 'PostgreSQL':
            return 'PG:dbname=%s' % self.input[self.dsnType][1].GetStringSelection()
        
        return self.input[self.dsnType][1].GetValue()

    def GetDsnWin(self):
        """!Get list of DSN windows"""
        win = list()
        for stype in ('file', 'dir', 'pro'):
            win.append(self.input[stype][1])
        for stype in ('file', 'text', 'choice'):
            win.append(self.input['db-win'][stype])
        
        return win
    
    def GetFormatExt(self):
        """!Get format extension"""
        return self.format.GetExtension(self.format.GetStringSelection())
    
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
            ret = gcmd.RunCommand('r.proj',
                                  quiet = True,
                                  read = True,
                                  flags = 'l',
                                  dbase = dbase,
                                  location = location,
                                  mapset = mapset)
        else:
            ret = gcmd.RunCommand('v.proj',
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
        utils.ListSortLower(listMaps)
        
        self.SetItems(listMaps)
        self.SetValue('')
