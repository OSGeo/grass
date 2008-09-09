"""
MODULE: gselect

CLASSES:
 * Select
 * TreeCrtlComboPopup

PURPOSE: Custon control that selects GRASS GIS elements

AUTHORS: The GRASS Development Team. Michael Barton & Martin Landa

COPYRIGHT: (C) 2007 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import os
import sys

import wx
import wx.combo

import grass
import globalvar
import gcmd
import utils
from preferences import globalSettings as UserSettings

class Select(wx.combo.ComboCtrl):
    def __init__(self, parent, id, size,
                 type, multiple=False, mapsets=None, exceptOf=[]):
        """
        Custom control to create a ComboBox with a tree control
        to display and select GIS elements within acessible mapsets.
        Elements can be selected with mouse. Can allow multiple selections, when
        argument multiple=True. Multiple selections are separated by commas.
        """
        wx.combo.ComboCtrl.__init__(self, parent=parent, id=id, size=size)

        self.tcp = TreeCtrlComboPopup()
        self.SetPopupControl(self.tcp)
        self.SetPopupExtents(0,100)
        self.tcp.GetElementList(type, mapsets, exceptOf)
        self.tcp.SetData(type, mapsets, exceptOf, multiple)

    def SetElementList(self, type):
        self.tcp.seltree.DeleteAllItems()
        self.tcp.GetElementList(type)

class TreeCtrlComboPopup(wx.combo.ComboPopup):
    """
    Create a tree ComboBox for selecting maps and other GIS elements
    in accessible mapsets within the current location
    """

    # overridden ComboPopup methods
    def Init(self):
        self.value = [] # for multiple is False -> len(self.value) in [0,1]
        self.curitem = None
        self.multiple = False
        self.type = None
        self.mapsets = []
        self.exceptOf = []

    def Create(self, parent):
        self.seltree = wx.TreeCtrl(parent, style=wx.TR_HIDE_ROOT
                                   |wx.TR_HAS_BUTTONS
                                   |wx.TR_SINGLE
                                   |wx.TR_LINES_AT_ROOT
                                   |wx.SIMPLE_BORDER
                                   |wx.TR_FULL_ROW_HIGHLIGHT)
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

    def OnPopup(self):
        """Limited only for first selected"""
        # update list
        self.seltree.DeleteAllItems()
        self.GetElementList(self.type, self.mapsets, self.exceptOf)

        if len(self.value) > 0:
            root = self.seltree.GetRootItem()
            if not root:
                return
            item = self.FindItem(root, self.value[0])
            self.seltree.EnsureVisible(item)
            self.seltree.SelectItem(item)
            
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

    def GetElementList(self, element, mapsets=None, exceptOf=[]):
        """
        Get list of GIS elements in accessible mapsets and display as tree
        with all relevant elements displayed beneath each mapset branch
        """
        # get current mapset
        curr_mapset = grass.gisenv()['MAPSET']
        
        # list of mapsets in current location
        if mapsets is None:
            mapsets = utils.ListOfMapsets()

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

        filesdict = grass.list_grouped(elementdict[element])
        
        for dir in mapsets:
            dir_node = self.AddItem('Mapset: '+dir)
            self.seltree.SetItemTextColour(dir_node,wx.Colour(50,50,200))
            try:
                elem_list = filesdict[dir]
                elem_list.sort()
                for elem in elem_list:
                    if elem != '':
                        fullqElem = elem + '@' + dir
                        if len(exceptOf) > 0 and fullqElem in exceptOf:
                            continue
                        self.AddItem(fullqElem, parent=dir_node)
            except:
                continue

            if self.seltree.ItemHasChildren(dir_node):
                sel = UserSettings.Get(group='general', key='elementListExpand',
                                       subkey='selection')
                collapse = True

                if sel == 0: # collapse all expect PERMANENT and current
                    if dir in ('PERMANENT', curr_mapset):
                        collapse = False
                elif sel == 1: # collapse all expect PERMANENT
                    if dir == 'PERMANENT':
                        collapse = False
                elif sel == 2: # collapse all
                    pass
                elif sel == 3: # expand all
                    collapse = False
                
                if collapse:
                    self.seltree.Collapse(dir_node)
                else:
                    self.seltree.Expand(dir_node)
                
    # helpers
    def FindItem(self, parentItem, text):
        item, cookie = self.seltree.GetFirstChild(parentItem)
        while item:
            if self.seltree.GetItemText(item) == text:
                return item
            if self.seltree.ItemHasChildren(item):
                item = self.FindItem(item, text)
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

    def OnMotion(self, evt):
        # have the selection follow the mouse, like in a real combobox
        item, flags = self.seltree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.seltree.SelectItem(item)
            self.curitem = item
        evt.Skip()

    def OnLeftDown(self, evt):
        # do the combobox selection
        item, flags = self.seltree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.curitem = item

            if self.seltree.GetRootItem() == self.seltree.GetItemParent(item):
                self.value = [] # cannot select mapset item
            else:
                if self.multiple is True:
                    # text item should be unique
                    self.value.append(self.seltree.GetItemText(item))
                else:
                    self.value = [self.seltree.GetItemText(item), ]

            self.Dismiss()

        evt.Skip()

    def SetData(self, type, mapsets, exceptOf, multiple):
        """Select multiple items?"""
        self.type = type
        self.mapsets = mapsets
        self.exceptOf = exceptOf
        self.multiple = multiple
        
class VectorDBInfo:
    """Class providing information about attribute tables
    linked to a vector map"""
    def __init__(self, map):
        self.map = map

        self.layers = {} # dictionary of layer number and associated table
        self.tables = {} # dictionary of table and associated columns
        
        ## {table : {column name : type, length, values, ids}}
        #self.tables = {}

        if not self.__GetLayers():  # -> self.layers
            return
        self.__GetColumns() # -> self.tables

    def __GetLayers(self):
        """Create layers dictionary"""
        
        layerCommand = gcmd.Command(cmd=["v.db.connect",
                                         "-g", "--q",
                                         "map=%s" % self.map],
                                    rerr=None, stderr=None)
        if layerCommand.returncode != 0:
            return False

        # create dictionary of layers (as strings) and associated table names for vector
        for line in layerCommand.ReadStdOutput():
            lineList = line.split(' ')
            layer = lineList[0]
            if '/' in layer:
                layer = layer.split('/')[0]
            table = lineList[1]
            self.layers[layer] = table
                        
        if (len(self.layers.keys()) == 0):
            return False

        return True

    def __GetColumns(self):
        """Create dictionary of tables and associated columns"""
        for layer in self.layers.keys():
            columns = []
            # determine column names and types
            table = self.layers[layer]
            cmd = ['v.info', '-c', 'map=%s' % table, 'layer=%s' % layer]
            try:
                info = gcmd.Command(cmd).ReadStdOutput()
                for line in info:
                    col = line.split('|')[1]
                    columns.append(col)
            except gcmd.CmdError:
                columns = []
            
            self.tables[table] = columns
        
        return True
    
class LayerSelect(wx.ComboBox):
    """
    Creates combo box for selecting data layers defined for vector.
    The 'layer' terminology is likely to change for GRASS 7
    """
    def __init__(self, parent,
                 id=wx.ID_ANY, value='1', pos=wx.DefaultPosition,
                 size=wx.DefaultSize, choices=['1'], **kargs):

        super(LayerSelect, self).__init__(parent, id, value, pos, size, choices)

        lsvector = kargs['vector'] # vector map to check for attribute tables

        if lsvector == '':
            return
        else:
            self.InsertLayers(lsvector)
        
    def InsertLayers(self, vector):
        """insert layers for a vector into the layer combobox"""
        layerchoices = VectorDBInfo(vector).layers.keys()
        self.SetItems(layerchoices)
        self.SetSelection(0)
        self.SetValue('1') # all vectors have a layer 1 by default

class ColumnSelect(wx.ComboBox):
    """
    Creates combo box for selecting columns in the attribute table for a vector.
    The 'layer' terminology is likely to change for GRASS 7
    """
    def __init__(self, parent,
                 id=wx.ID_ANY, value='', pos=wx.DefaultPosition,
                 size=wx.DefaultSize, choices=[''], **kargs):

        super(ColumnSelect, self).__init__(parent, id, value, pos, size, choices)

        csvector = kargs['vector'] # vector map to check for attribute tables
        cslayer = kargs['layer'] # data layer connected to attribute table

        if csvector == '' or cslayer == '':
            return
        else:
            self.InsertColumns(csvector, cslayer)
                
    def InsertColumns(self, vector, layer):
        """insert columns for a vector attribute table into the columns combobox"""
        if vector == '' or layer == '': return
        table = VectorDBInfo(vector).layers[str(layer)]
        columnchoices = VectorDBInfo(vector).tables[table]
        #columnchoices.sort()
        self.SetItems(columnchoices)

class DbColumnSelect(wx.ComboBox):
    """
    Creates combo box for selecting columns from any table.
    """
    def __init__(self, parent,
                 id=wx.ID_ANY, value='', pos=wx.DefaultPosition,
                 size=wx.DefaultSize, choices=[''], **kargs):

        super(ColumnSelect, self).__init__(parent, id, value, pos, size, choices)

        dbtable = kargs['table'] # table to check for columns
        dbdriver = kargs['driver'] # driver for table
        dbdatabase = kargs['database'] # database for table

        if dbtable == '':
            return
        else:
            self.InsertColumns(dbtable)
                
    def InsertColumns(self, table):
        """insert columns for a table into the columns combobox"""
        if table == '' : return
        
        cmd = ['db.columns','table=%s' % table]
        if dbdriver != '': cmd.append('driver=%s' % dbdriver)
        if dbdatabase != '': cmd.append('database=%s' % dbdatabase)
        
        try:
            columnchoices = gcmd.Command(cmd).ReadStdOutput()
        except gcmd.CmdError:
            columnchoices = []

        #columnchoices.sort()
        self.SetItems(columnchoices)
