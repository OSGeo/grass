"""
@package gui_core.gselect

@brief Custom control that selects elements

Classes:
 - :class:`Select`
 - :class:`VectorSelect`
 - :class:`ListCtrlComboPopup`
 - :class:`TreeCrtlComboPopup`
 - :class:`VectorDBInfo`
 - :class:`LayerSelect`
 - :class:`DriverSelect`
 - :class:`DatabaseSelect`
 - :class:`TableSelect`
 - :class:`ColumnSelect`
 - :class:`DbaseSelect`
 - :class:`LocationSelect`
 - :class:`MapsetSelect`
 - :class:`SubGroupSelect`
 - :class:`FormatSelect`
 - :class:`GdalSelect`
 - :class:`ProjSelect`
 - :class:`ElementSelect`
 - :class:`OgrTypeSelect`
 - :class:`CoordinatesSelect`
 - :class:`VectorCategorySelect`
 - :class:`SignatureSelect`
 - :class:`SeparatorSelect`
 - :class:`SqlWhereSelect`

(C) 2007-2018 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
@author Stepan Turek <stepan.turek seznam.cz> (CoordinatesSelect, ListCtrlComboPopup)
@author Matej Krejci <matejkrejci gmail.com> (VectorCategorySelect)
"""

from __future__ import print_function

import os
import sys
import glob
import six

import wx

from core import globalvar
import wx.lib.buttons as buttons
import wx.lib.filebrowsebutton as filebrowse



import grass.script as grass
from grass.script import task as gtask
from grass.exceptions import CalledModuleError

from gui_core.widgets import ManageSettingsWidget, CoordinatesValidator

from core.gcmd import RunCommand, GMessage, GWarning, GException
from core.utils    import GetListOfLocations, GetListOfMapsets, \
    GetFormats, rasterFormatExtension, vectorFormatExtension
from core.utils import GetSettingsPath, GetValidLayerName, ListSortLower
from core.utils import GetVectorNumberOfLayers
from core.settings import UserSettings
from core.debug import Debug
from gui_core.vselect import VectorSelectBase
from gui_core.wrap import TreeCtrl, Button, StaticText, StaticBox, \
    TextCtrl, Panel, ComboPopup, ComboCtrl

from grass.pydispatch.signal import Signal


class Select(ComboCtrl):

    def __init__(
            self, parent, id=wx.ID_ANY, size=globalvar.DIALOG_GSELECT_SIZE,
            type=None, multiple=False, nmaps=1, mapsets=None,
            updateOnPopup=True, onPopup=None, fullyQualified=True,
            extraItems={},
            layerTree=None, validator=wx.DefaultValidator):
        """Custom control to create a ComboBox with a tree control to
        display and select GIS elements within acessible mapsets.
        Elements can be selected with mouse. Can allow multiple
        selections, when argument <em>multiple</em> is True. Multiple
        selections are separated by commas.

        :param type: type of GIS elements ('raster, 'vector', ...)
        :param multiple: True for multiple input
        :param nmaps: number of maps to be entered
        :param mapsets: force list of mapsets (otherwise search path)
        :param updateOnPopup: True for updating list of elements on popup
        :param onPopup: function to be called on Popup
        :param fullyQualified: True to provide fully qualified names (map@mapset)
        :param extraItems: extra items to add (given as dictionary) - see gmodeler for usage
        :param layerTree: show only elements from given layer tree if not None
        :param validator: validator for TextCtrl
        """
        ComboCtrl.__init__(
            self,
            parent=parent,
            id=id,
            size=size,
            validator=validator)
        if globalvar.CheckWxVersion([3]):
            self.SetName("Select")
        else:
            self.GetChildren()[0].SetName("Select")

        self.GetChildren()[0].type = type

        self.tcp = TreeCtrlComboPopup()
        self.SetPopupControl(self.tcp)
        self.SetPopupExtents(0, 100)
        if type:
            self.tcp.SetData(
                type=type,
                mapsets=mapsets,
                multiple=multiple,
                nmaps=nmaps,
                updateOnPopup=updateOnPopup,
                onPopup=onPopup,
                fullyQualified=fullyQualified,
                extraItems=extraItems,
                layerTree=layerTree)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)

    def OnKeyDown(self, event):
        """Open popup and send key events to the tree."""
        if self.IsPopupShown():
            # on some configurations we get key down, with some only key up
            # so we are trying to catch either key up or down
            # this mess shouldn't be necessary with wxPython 3
            self.tcp.OnKeyUp(event)
        else:
            if event.GetKeyCode() == wx.WXK_DOWN:
                self.ShowPopup()
            event.Skip()

    def SetElementList(self, type, mapsets=None):
        """Set element list

        :param type: GIS element type
        :param mapsets: list of acceptable mapsets (None for all in search path)
        """
        self.tcp.SetData(type=type, mapsets=mapsets)

    def GetElementList(self):
        """Load elements"""
        self.tcp.GetElementList()

    def SetType(self, etype, multiple=False, nmaps=1,
                mapsets=None, updateOnPopup=True, onPopup=None):
        """Param set element type for widget

        :param etype: element type, see gselect.ElementSelect
        """
        self.tcp.SetData(type=etype, mapsets=mapsets,
                         multiple=multiple, nmaps=nmaps,
                         updateOnPopup=updateOnPopup, onPopup=onPopup)


class VectorSelect(Select):

    def __init__(self, parent, ftype, **kwargs):
        """Custom to create a ComboBox with a tree control to display and
        select vector maps. You can filter the vector maps. If you
        don't need this feature use Select class instead

        :param ftype: filter vector maps based on feature type
        """
        Select.__init__(self, parent=parent, id=wx.ID_ANY,
                        type='vector', **kwargs)

        self.ftype = ftype

        # remove vector maps which do not contain given feature type
        self.tcp.SetFilter(self._isElement)

    def _isElement(self, vectorName):
        """Check if element should be filtered out"""
        try:
            if int(grass.vector_info_topo(vectorName)[self.ftype]) < 1:
                return False
        except KeyError:
            return False

        return True


class ListCtrlComboPopup(ComboPopup):
    """Create a list ComboBox using TreeCtrl with hidden root.

    .. todo::

        use event.EventObject instead of hardcoding (see forms.py)
        https://groups.google.com/forum/#!topic/wxpython-users/pRz6bi0k0XY

    """
    # overridden ComboPopup methods

    def Init(self):
        self.value = []            # for multiple is False ->
        # len(self.value) in [0,1]
        self.curitem = None
        self.multiple = False
        self.updateOnPopup = True
        self.filterItems = []      # limit items based on this list,
        # see layerTree parameter

    def Create(self, parent):
        self.seltree = TreeCtrl(parent, style=wx.TR_HIDE_ROOT |
                                wx.TR_HAS_BUTTONS |
                                wx.TR_SINGLE |
                                wx.TR_LINES_AT_ROOT |
                                wx.SIMPLE_BORDER |
                                wx.TR_FULL_ROW_HIGHLIGHT)
        self.seltree.Bind(wx.EVT_MOTION, self.OnMotion)
        self.seltree.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        # the following dummy handler are needed to keep tree events
        # from propagating up to the parent GIS Manager layer tree
        self.seltree.Bind(wx.EVT_TREE_ITEM_EXPANDING, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_ITEM_COLLAPSED, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_SEL_CHANGED, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_DELETE_ITEM, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_BEGIN_DRAG, lambda x: None)
        self.seltree.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, lambda x: None)
        # navigation in list/tree is handled automatically since wxPython 3
        # for older versions, we have to workaround it and write our own
        # navigation
        if globalvar.CheckWxVersion(version=[3]):
            self.seltree.Bind(
                wx.EVT_TREE_ITEM_ACTIVATED,
                self._onItemConfirmed)
            self.seltree.Bind(wx.EVT_TREE_KEY_DOWN, self._onDismissPopup)
        else:
            self.seltree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, lambda x: None)
            self.seltree.Bind(wx.EVT_KEY_UP, self.OnKeyUp)

        return True

    def GetControl(self):
        return self.seltree

    def GetComboCtrl(self):
        if globalvar.wxPythonPhoenix:
            return super(ListCtrlComboPopup, self).GetComboCtrl()
        else:
            return self.GetCombo()

    def GetStringValue(self):
        """Get value as a string separated by commas
        """
        return ','.join(self.value)

    def SetStringValue(self, value):
        # this assumes that item strings are unique...
        root = self.seltree.GetRootItem()
        if not root:
            return
        winValue = self.GetComboCtrl().GetValue().strip(',')
        self.value = []
        if winValue:
            self.value = winValue.split(',')

    def OnPopup(self, force=False):
        """Limited only for first selected
        """
        if not force and not self.updateOnPopup:
            return

        # selects map starting according to written text
        inputText = self.GetComboCtrl().GetValue().strip()
        if inputText:
            root = self.seltree.GetRootItem()
            match = self.FindItem(root, inputText, startLetters=True)
            if match.IsOk():
                self.seltree.EnsureVisible(match)
                self.seltree.SelectItem(match)

    def GetAdjustedSize(self, minWidth, prefHeight, maxHeight):
        """Reads UserSettings to get height (which was 200 in old implementation).
        """
        height = UserSettings.Get(
            group='appearance',
            key='gSelectPopupHeight',
            subkey='value')
        return wx.Size(minWidth, min(height, maxHeight))

    def FindItem(self, parentItem, text, startLetters=False):
        """Finds item with given name or starting with given text
        """
        startletters = startLetters
        item, cookie = self.seltree.GetFirstChild(parentItem)
        while wx.TreeItemId.IsOk(item):
            if self.seltree.GetItemText(item) == text:
                return item
            if self.seltree.ItemHasChildren(item):
                item = self.FindItem(item, text, startLetters=startletters)
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
        self.seltree.AppendItem(root, text=value)

    def SetItems(self, items):
        root = self.seltree.GetRootItem()
        if not root:
            root = self.seltree.AddRoot("<hidden root>")
        for item in items:
            self.seltree.AppendItem(root, text=item)

    def OnKeyUp(self, event):
        """Enable to select items using keyboard.

        Unused with wxPython 3, can be removed in the future.
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
            self._selectTreeItem(item)
            self.Dismiss()

    def _onDismissPopup(self, event):
        """Hide popup without selecting item on Esc"""
        if event.GetKeyCode() == wx.WXK_ESCAPE:
            self.Dismiss()
        else:
            event.Skip()

    def _selectTreeItem(self, item):
        item_str = self.seltree.GetItemText(item)
        if self.multiple:
            if item_str not in self.value:
                self.value.append(item_str)
        else:
            self.value = [item_str]

    def _onItemConfirmed(self, event):
        item = event.GetItem()
        self._selectTreeItem(item)
        self.Dismiss()

    def OnMotion(self, evt):
        """Have the selection follow the mouse, like in a real combobox
        """
        item, flags = self.seltree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.seltree.SelectItem(item)
            self.curitem = item
        evt.Skip()

    def OnLeftDown(self, evt):
        """Do the combobox selection
        """
        if self.curitem is None:
            return

        self._selectTreeItem(self.curitem)
        self.Dismiss()

        evt.Skip()

    def SetData(self, **kargs):
        """Set object properties"""
        if 'multiple' in kargs:
            self.multiple = kargs['multiple']
        if 'onPopup' in kargs:
            self.onPopup = kargs['onPopup']
        if kargs.get('layerTree', None):
            self.filterItems = []  # reset
            ltype = kargs['type']
            for layer in kargs['layerTree'].GetVisibleLayers(
                    skipDigitized=True):
                if layer.GetType() != ltype:
                    continue
                self.filterItems.append(layer.GetName())

    def DeleteAllItems(self):
        """Delete all items in popup"""
        self.seltree.DeleteAllItems()


class TreeCtrlComboPopup(ListCtrlComboPopup):
    """Create a tree ComboBox for selecting maps and other GIS elements
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
        self.tgis_error = False

    def SetFilter(self, filter):
        """Set filter for GIS elements, see e.g. VectorSelect"""
        self.filterElements = filter

    def OnPopup(self, force=False):
        """Limited only for first selected"""
        if not force and not self.updateOnPopup:
            return
        if self.onPopup:
            selected, exclude = self.onPopup(self.type)
        else:
            selected = None
            exclude = False

        self.GetElementList(selected, exclude)

        ListCtrlComboPopup.OnPopup(self, force)

    def GetElementList(self, elements=None, exclude=False):
        """Get filtered list of GIS elements in accessible mapsets
        and display as tree with all relevant elements displayed
        beneath each mapset branch
        """
        # update list
        self.seltree.DeleteAllItems()
        if self.type:
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

    def _getElementList(self, element, mapsets=None,
                        elements=None, exclude=False):
        """Get list of GIS elements in accessible mapsets and display as tree
        with all relevant elements displayed beneath each mapset branch

        :param element: GIS element
        :param mapsets: list of acceptable mapsets (None for all mapsets in search path)
        :param elements: list of forced GIS elements
        :param exclude: True to exclude, False for forcing the list (elements)
        """
        # get current mapset
        curr_mapset = grass.gisenv()['MAPSET']

        # map element types to g.list types
        elementdict = {'cell': 'raster',
                       'raster': 'raster',
                       'grid3': 'raster_3d',
                       'raster_3d': 'raster_3d',
                       'vector': 'vector',
                       'paint/labels': 'label',
                       'label': 'label',
                       'windows': 'region',
                       'region': 'region',
                       'group': 'group',
                       'stds': 'stds',
                       'strds': 'strds',
                       'str3ds': 'str3ds',
                       'stvds': 'stvds'}

        # to support multiple elements
        element_list = element.split(',')
        renamed_elements = []
        for elem in element_list:
            if elem not in elementdict:
                self.AddItem(_('Not selectable element'), node=False)
                return
            else:
                renamed_elements.append(elementdict[elem])

        if element in ('stds', 'strds', 'str3ds', 'stvds'):
            if not self.tgis_error:
                import grass.temporal as tgis
                filesdict = tgis.tlist_grouped(
                    elementdict[element], element == 'stds')
            else:
                filesdict = None
        else:
            filesdict = grass.list_grouped(renamed_elements,
                                           check_search_path=False)

        # add extra items first
        if self.extraItems:
            for group, items in six.iteritems(self.extraItems):
                node = self.AddItem(group, node=True)
                self.seltree.SetItemTextColour(node, wx.Colour(50, 50, 200))
                for item in items:
                    self.AddItem(item, node=False, parent=node)
                self.seltree.ExpandAllChildren(node)

        # list of mapsets in current location
        if mapsets is None:
            mapsets = grass.mapsets(search_path=True)

        # current mapset first
        if curr_mapset in mapsets and mapsets[0] != curr_mapset:
            mapsets.remove(curr_mapset)
            mapsets.insert(0, curr_mapset)

        first_mapset = None
        for mapset in mapsets:
            mapset_node = self.AddItem(
                _('Mapset') + ': ' + mapset, node=True, mapset=mapset)
            node = mapset_node
            if not first_mapset:
                first_mapset = mapset_node

            self.seltree.SetItemTextColour(mapset_node, wx.Colour(50, 50, 200))
            if mapset not in filesdict:
                continue
            try:
                if isinstance(filesdict[mapset], dict):
                    for elementType in filesdict[mapset].keys():
                        node = self.AddItem(
                            _('Type: ') + elementType,
                            mapset=mapset,
                            node=True,
                            parent=mapset_node)
                        self.seltree.SetItemTextColour(
                            node, wx.Colour(50, 50, 200))
                        elem_list = filesdict[mapset][elementType]
                        self._addItems(
                            elist=elem_list,
                            elements=elements,
                            mapset=mapset,
                            exclude=exclude,
                            node=node)
                else:
                    elem_list = filesdict[mapset]
                    self._addItems(elist=elem_list, elements=elements,
                                   mapset=mapset, exclude=exclude, node=node)
            except Exception as e:
                sys.stderr.write(_("GSelect: invalid item: %s") % e)
                continue

            if self.seltree.ItemHasChildren(mapset_node):
                sel = UserSettings.Get(
                    group='appearance',
                    key='elementListExpand',
                    subkey='selection')
                collapse = True

                if sel == 0:  # collapse all except PERMANENT and current
                    if mapset in ('PERMANENT', curr_mapset):
                        collapse = False
                elif sel == 1:  # collapse all except PERMANENT
                    if mapset == 'PERMANENT':
                        collapse = False
                elif sel == 2:  # collapse all except current
                    if mapset == curr_mapset:
                        collapse = False
                elif sel == 3:  # collapse all
                    pass
                elif sel == 4:  # expand all
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
        """Helper function for adding multiple items (maps, stds).

        :param list elist: list of map/stds names
        :param list elements: list of forced elements
        :param str mapset:  mapset name
        :param exclude: True to exclude, False for forcing the list
        :param node: parent node
        """
        elist = grass.naturally_sorted(elist)
        for elem in elist:
            if elem != '':
                fullqElem = elem + '@' + mapset
                if self.filterItems and fullqElem not in self.filterItems:
                    continue  # skip items missed in self.filterItems

                if elements is not None:
                    if (exclude and fullqElem in elements) or \
                            (not exclude and fullqElem not in elements):
                        continue

                if self.filterElements:
                    if self.filterElements(fullqElem):
                        self.AddItem(
                            elem, mapset=mapset, node=False, parent=node)
                else:
                    self.AddItem(elem, mapset=mapset, node=False, parent=node)

    def AddItem(self, value, mapset=None, node=True, parent=None):
        if not parent:
            root = self.seltree.GetRootItem()
            if not root:
                root = self.seltree.AddRoot("<hidden root>")
            parent = root

        data = {'node': node, 'mapset': mapset}

        item = self.seltree.AppendItem(
            parent, text=value, data=data)
        return item

    def OnKeyUp(self, event):
        """Enables to select items using keyboard

        Unused with wxPython 3, can be removed in the future.
        """

        item = self.seltree.GetSelection()
        if event.GetKeyCode() == wx.WXK_DOWN:
            self.seltree.SelectItem(self.seltree.GetNextVisible(item))

        # problem with GetPrevVisible
        elif event.GetKeyCode() == wx.WXK_UP:
            if self.seltree.ItemHasChildren(item) and self.seltree.IsExpanded(
                    self.seltree.GetPrevSibling(item)):
                itemPrev = self.seltree.GetLastChild(
                    self.seltree.GetPrevSibling(item))
            else:
                itemPrev = self.seltree.GetPrevSibling(item)
                if not wx.TreeItemId.IsOk(itemPrev):
                    itemPrev = self.seltree.GetItemParent(item)
                    if item == self.seltree.GetFirstChild(
                            self.seltree.GetRootItem())[0]:
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
                match = self.FindItem(
                    nextSibling, self.GetCombo().GetValue().strip(), True)
            else:
                match = self.FindItem(
                    self.seltree.GetFirstChild(
                        self.seltree.GetItemParent(parent))[0],
                    self.GetCombo().GetValue().strip(),
                    True)
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
            if self.seltree.GetItemData(item)['node']:
                self.value = []
            else:
                self._selectTreeItem(item)

            self.Dismiss()

    def OnLeftDown(self, evt):
        """Do the combobox selection
        """
        item, flags = self.seltree.HitTest(evt.GetPosition())
        if item and flags & wx.TREE_HITTEST_ONITEMLABEL:
            self.curitem = item

            if self.seltree.GetItemData(item)['node']:
                evt.Skip()
                return

            self._selectTreeItem(item)
            self.Dismiss()

        evt.Skip()

    def _selectTreeItem(self, item):
        fullName = self.seltree.GetItemText(item)
        if self.fullyQualified and self.seltree.GetItemData(item)['mapset']:
            fullName += '@' + self.seltree.GetItemData(item)['mapset']

        if self.multiple:
            self.value.append(fullName)
        else:
            if self.nmaps > 1:  # see key_desc
                if len(self.value) >= self.nmaps:
                    self.value = [fullName]
                else:
                    self.value.append(fullName)
            else:
                self.value = [fullName]

    def _onItemConfirmed(self, event):
        item = event.GetItem()
        if self.seltree.GetItemData(item)['node']:
            self.value = []
        else:
            self._selectTreeItem(item)
        self.Dismiss()

    def SetData(self, **kargs):
        """Set object properties"""
        ListCtrlComboPopup.SetData(self, **kargs)
        if 'type' in kargs:
            self.type = kargs['type']
            if self.type in ('stds', 'strds', 'str3ds', 'stvds'):
                # Initiate the temporal framework. Catch database error
                # and set the error flag for the stds listing.
                try:
                    import grass.temporal as tgis
                    from grass.pygrass import messages
                    try:
                        tgis.init(True)
                    except messages.FatalError as e:
                        sys.stderr.write(_("Temporal GIS error:\n%s") % e)
                        self.tgis_error = True
                except ImportError as e:
                    # PyGRASS (ctypes) is the likely cause
                    sys.stderr.write(_(
                        "Unable to import pyGRASS: %s\n"
                        "Some functionality will be not accessible") % e)
                    self.tgis_error = True
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
        """Get element type
        """
        return self.type


class VectorDBInfo:
    """Class providing information about attribute tables
    linked to a vector map"""

    def __init__(self, map):
        self.map = map

        # dictionary of layer number and associated (driver, database, table)
        self.layers = {}
        # dictionary of table and associated columns (type, length, values,
        # ids)
        self.tables = {}

        if not self._CheckDBConnection():  # -> self.layers
            return

        self._DescribeTables()  # -> self.tables

    def _CheckDBConnection(self):
        """Check DB connection"""
        nuldev = open(os.devnull, 'w+')
        # if map is not defined (happens with vnet initialization) or it
        # doesn't exist
        try:
            self.layers = grass.vector_db(map=self.map, stderr=nuldev)
        except CalledModuleError:
            return False
        finally:  # always close nuldev
            nuldev.close()

        return bool(len(self.layers.keys()) > 0)

    def _DescribeTables(self):
        """Describe linked tables"""
        for layer in self.layers.keys():
            # determine column names and types
            table = self.layers[layer]["table"]
            columns = {}  # {name: {type, length, [values], [ids]}}
            i = 0
            Debug.msg(
                1,
                "gselect.VectorDBInfo._DescribeTables(): table=%s driver=%s database=%s" %
                (self.layers[layer]["table"],
                 self.layers[layer]["driver"],
                 self.layers[layer]["database"]))
            for item in grass.db_describe(
                    table=self.layers[layer]["table"],
                    driver=self.layers[layer]["driver"],
                    database=self.layers[layer]["database"])['cols']:
                name, type, length = item
                # FIXME: support more datatypes
                if type.lower() == "integer":
                    ctype = int
                elif type.lower() == "double precision":
                    ctype = float
                else:
                    ctype = str

                columns[name.strip()] = {'index': i,
                                         'type': type.lower(),
                                         'ctype': ctype,
                                         'length': int(length),
                                         'values': [],
                                         'ids': []}
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
        """Reset"""
        for layer in self.layers:
            table = self.layers[layer]["table"]  # get table desc
            for name in self.tables[table].keys():
                self.tables[table][name]['values'] = []
                self.tables[table][name]['ids'] = []

    def GetName(self):
        """Get vector name"""
        return self.map

    def GetKeyColumn(self, layer):
        """Get key column of given layer

        :param layer: vector layer number
        """
        return str(self.layers[layer]['key'])

    def GetTable(self, layer):
        """Get table name of given layer

        :param layer: vector layer number
        """
        if layer not in self.layers:
            raise GException(_("No table linked to layer <{}>.".format(layer)))
        return self.layers[layer]['table']

    def GetDbSettings(self, layer):
        """Get database settins

        :param layer: layer number

        :return: (driver, database)
        """
        return self.layers[layer]['driver'], self.layers[layer]['database']

    def GetTableDesc(self, table):
        """Get table columns

        :param table: table name
        """
        return self.tables[table]


class LayerSelect(wx.ComboBox):

    def __init__(self, parent, id=wx.ID_ANY,
                 size=globalvar.DIALOG_COMBOBOX_SIZE,
                 vector=None, dsn=None, choices=[], all=False, default=None):
        """Creates combo box for selecting vector map layer names

        :param str vector: vector map name (native or connected via v.external)
        :param str dsn: OGR data source name
        """
        super(
            LayerSelect,
            self).__init__(
            parent,
            id,
            size=size,
            choices=choices)

        self.all = all

        self.SetName("LayerSelect")

        # default value
        self.default = default

        self.InsertLayers(vector=vector, dsn=dsn)

    def InsertLayers(self, vector=None, dsn=None):
        """Insert layers for a vector into the layer combobox

        :param str vector: vector map name (native or connected via v.external)
        :param str dsn: OGR data source name
        """
        layers = list()

        if vector:
            layers = GetVectorNumberOfLayers(vector)
        elif dsn:
            ret = RunCommand('v.in.ogr',
                             read=True,
                             quiet=True,
                             flags='l',
                             input=dsn)
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
    """Creates combo box for selecting database driver.
    """

    def __init__(self, parent, choices, value,
                 id=wx.ID_ANY, pos=wx.DefaultPosition,
                 size=globalvar.DIALOG_LAYER_SIZE, **kargs):

        super(DriverSelect, self).__init__(parent, id, value, pos, size,
                                           choices, style=wx.CB_READONLY)

        self.SetName("DriverSelect")

        self.SetStringSelection(value)


class DatabaseSelect(TextCtrl):
    """Creates combo box for selecting database driver.
    """

    def __init__(self, parent, value='', id=wx.ID_ANY,
                 size=globalvar.DIALOG_TEXTCTRL_SIZE, **kargs):
        super(
            DatabaseSelect,
            self).__init__(
            parent,
            id,
            value,
            size=size,
            **kargs)
        self.SetName("DatabaseSelect")


class TableSelect(wx.ComboBox):
    """Creates combo box for selecting attribute tables from the database
    """

    def __init__(self, parent,
                 id=wx.ID_ANY, value='',
                 size=globalvar.DIALOG_COMBOBOX_SIZE, choices=[], **kargs):
        super(
            TableSelect,
            self).__init__(
            parent,
            id,
            value,
            size=size,
            choices=choices,
            style=wx.CB_READONLY,
            **kargs)
        self.SetName("TableSelect")

        if not choices:
            self.InsertTables()

    def InsertTables(self, driver=None, database=None):
        """Insert attribute tables into combobox"""
        items = []

        if not driver or not database:
            connect = grass.db_connection()

            driver = connect['driver']
            database = connect['database']

        ret = RunCommand('db.tables',
                         flags='p',
                         read=True,
                         driver=driver,
                         database=database)

        if ret:
            for table in ret.splitlines():
                items.append(table)

        self.SetItems(items)
        self.SetValue('')


class ColumnSelect(ComboCtrl):
    """Creates combo box for selecting columns in the attribute table
    for a vector map.

    :param parent: window parent
    :param id: window id
    :param value: default value
    :param size: window size
    :param str vector: vector map name
    :param layer: layer number
    :param multiple: True if it is possible to add multiple columns
    :param param: parameters list (see menuform.py)
    :param kwags: wx.ComboBox parameters
    """

    def __init__(self, parent, id=wx.ID_ANY, value='',
                 size=globalvar.DIALOG_COMBOBOX_SIZE,
                 vector=None, layer=1, multiple=False,
                 param=None, **kwargs):
        self.defaultValue = value
        self.param = param
        self.columns = []

        ComboCtrl.__init__(self, parent, id, size=size, **kwargs)
        self.GetChildren()[0].SetName("ColumnSelect")
        self.GetChildren()[0].type = type

        self.tcp = ListCtrlComboPopup()
        self.SetPopupControl(self.tcp)
        self.tcp.SetData(multiple=multiple)

        if vector:
            self.InsertColumns(vector, layer)
        self.GetChildren()[0].Bind(wx.EVT_KEY_UP, self.OnKeyUp)

    def GetColumns(self):
        return self.columns

    def OnKeyUp(self, event):
        """Shows popupwindow if down arrow key is released"""
        if event.GetKeyCode() == wx.WXK_DOWN and not self.IsPopupShown():
            self.ShowPopup()
        else:
            event.Skip()

    def Clear(self):
        self.tcp.DeleteAllItems()
        self.SetValue('')

    def InsertColumns(self, vector, layer, excludeKey=False,
                      excludeCols=None, type=None, dbInfo=None):
        """Insert columns for a vector attribute table into the columns combobox

        :param str vector: vector name
        :param int layer: vector layer number
        :param excludeKey: exclude key column from the list?
        :param excludeCols: list of columns to be removed from the list
        :param type: only columns of given type (given as list)
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
            self.columns = len(columnchoices.keys()) * ['']
            for key, val in six.iteritems(columnchoices):
                self.columns[val['index']] = key
            if excludeKey:  # exclude key column
                self.columns.remove(keyColumn)
            if excludeCols:  # exclude key column
                for key in six.iterkeys(columnchoices):
                    if key in excludeCols:
                        self.columns.remove(key)
            if type:  # only selected column types
                for key, value in six.iteritems(columnchoices):
                    if value['type'] not in type:
                        try:
                            self.columns.remove(key)
                        except ValueError:
                            pass
        except (KeyError, ValueError, GException):
            self.columns[:] = []

        # update list
        self.tcp.DeleteAllItems()
        for col in self.columns:
            self.tcp.AddItem(col)

        self.SetValue(self.defaultValue)

        if self.param:
            value = self.param.get('value', '')
            if value != '' and value in self.columns:
                self.SetValue(value)

    def InsertTableColumns(self, table, driver=None, database=None):
        """Insert table columns

        :param str table: table name
        :param str driver: driver name
        :param str database: database name
        """
        self.columns[:] = []

        ret = RunCommand('db.columns',
                         read=True,
                         driver=driver,
                         database=database,
                         table=table)

        if ret:
            self.columns = ret.splitlines()

        # update list
        self.tcp.DeleteAllItems()
        self.SetValue(self.defaultValue)

        for col in self.columns:
            self.tcp.AddItem(col)
        if self.param:
            value = self.param.get('value', '')
            if value != '' and value in self.columns:
                self.SetValue(value)


class DbaseSelect(wx.lib.filebrowsebutton.DirBrowseButton):
    """Widget for selecting GRASS Database"""

    def __init__(self, parent, **kwargs):
        super(
            DbaseSelect,
            self).__init__(
            parent,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            labelText='',
            dialogTitle=_('Choose GIS Data Directory'),
            buttonText=_('Browse'),
            startDirectory=grass.gisenv()['GISDBASE'],
            **kwargs)


class LocationSelect(wx.ComboBox):
    """Widget for selecting GRASS location"""

    def __init__(
            self, parent, id=wx.ID_ANY, size=globalvar.DIALOG_COMBOBOX_SIZE,
            gisdbase=None, **kwargs):
        super(LocationSelect, self).__init__(parent, id, size=size, **kwargs)
        self.SetName("LocationSelect")

        if not gisdbase:
            self.gisdbase = grass.gisenv()['GISDBASE']
        else:
            self.gisdbase = gisdbase

        self.SetItems(GetListOfLocations(self.gisdbase))

    def UpdateItems(self, dbase):
        """Update list of locations

        :param str dbase: path to GIS database
        """
        self.gisdbase = dbase
        if dbase:
            self.SetItems(GetListOfLocations(self.gisdbase))
        else:
            self.SetItems([])


class MapsetSelect(wx.ComboBox):
    """Widget for selecting GRASS mapset"""

    def __init__(self, parent, id=wx.ID_ANY,
                 size=globalvar.DIALOG_COMBOBOX_SIZE, gisdbase=None,
                 location=None, setItems=True, searchPath=False, new=False,
                 skipCurrent=False, multiple=False, **kwargs):
        style = 0
        # disabled, read-only widget has no TextCtrl children (TODO: rewrite)
        # if not new and not multiple:
        ###     style = wx.CB_READONLY

        wx.ComboBox.__init__(self, parent, id, size=size,
                             style=style, **kwargs)
        self.searchPath = searchPath
        self.skipCurrent = skipCurrent
        self.SetName("MapsetSelect")
        self.value = ''
        self.multiple = multiple
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

        if self.multiple:
            self.Bind(wx.EVT_COMBOBOX, self._onSelection)
            self.Bind(wx.EVT_TEXT, self._onSelection)

    def _onSelection(self, event):
        value = self.GetValue()
        if value:
            if self.value:
                self.value += ','
            self.value += value
            self.SetValue(self.value)
        else:
            self.value = value
        event.Skip()

    def UpdateItems(self, location, dbase=None):
        """Update list of mapsets for given location

        :param str dbase: path to GIS database (None to use currently
                          selected)
        :param str location: name of location
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
                               read=True, flags='p',
                               sep='newline').splitlines()
        else:
            mlist = GetListOfMapsets(self.gisdbase, self.location,
                                     selectable=False)

        gisenv = grass.gisenv()
        if self.skipCurrent and \
                gisenv['LOCATION_NAME'] == self.location and \
                gisenv['MAPSET'] in mlist:
            mlist.remove(gisenv['MAPSET'])

        return mlist


class SubGroupSelect(wx.ComboBox):
    """Widget for selecting subgroups"""

    def __init__(self, parent, id=wx.ID_ANY,
                 size=globalvar.DIALOG_GSELECT_SIZE, **kwargs):
        super(SubGroupSelect, self).__init__(parent, id, size=size,
                                             **kwargs)
        self.SetName("SubGroupSelect")

    def Insert(self, group):
        """Insert subgroups for defined group"""
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


class FormatSelect(wx.Choice):

    def __init__(self, parent, srcType, ogr=False,
                 size=globalvar.DIALOG_SPIN_SIZE,
                 **kwargs):
        """Widget for selecting external (GDAL/OGR) format

        :param parent: parent window
        :param srcType: source type ('file', 'database', 'protocol')
        :param ogr: True for OGR otherwise GDAL
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
        """Get file extension by format name"""
        formatToExt = dict()
        formatToExt.update(rasterFormatExtension)
        formatToExt.update(vectorFormatExtension)

        return formatToExt.get(name, '')


class GdalSelect(wx.Panel):

    def __init__(self, parent, panel, ogr=False, link=False, dest=False,
                 exclude=None, settings=True):
        """Widget for selecting GDAL/OGR datasource, format

        .. todo::
             Split into GdalSelect and OgrSelect and optionally to
             GdalSelectOutput, OgrSelectOutput

        :param parent: parent window
        :param bool ogr: use OGR selector instead of GDAL
        :param bool dest: True for output (destination)
        :param default: default type (ignored when dest == True)
        :param exclude: list of types to be excluded
        """
        self.parent = parent
        self.ogr = ogr
        self.link = link
        self.dest = dest
        self._sourceType = None

        wx.Panel.__init__(self, parent=panel, name='GdalSelect')

        self.reloadDataRequired = Signal('GdalSelect.reloadDataRequired')

        self.inputBox = StaticBox(parent=self)
        if dest:
            self.inputBox.SetLabel(" %s " % _("Output settings"))
        else:
            self.inputBox.SetLabel(" %s " % _("Source input"))

        # source type
        sources = list()
        self.sourceMap = {'file': -1,
                          'dir': -1,
                          'db': -1,
                          'pro': -1,
                          'native': -1}
        idx = 0
        if dest:
            sources.append(_("Native"))
            self.sourceMap['native'] = idx
            idx += 1
        if exclude is None:
            exclude = []
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
            idx += 1
        self.sourceMapByIdx = {}
        for name, idx in self.sourceMap.items():
            self.sourceMapByIdx[idx] = name

        self.source = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                  style=wx.RA_SPECIFY_COLS,
                                  choices=sources)
        if dest:
            self.source.SetLabel(" %s " % _('Output type'))
        else:
            self.source.SetLabel(" %s " % _('Source type'))

        self.source.SetSelection(0)
        self.source.Bind(
            wx.EVT_RADIOBOX,
            lambda evt: self.SetSourceType(
                self.sourceMapByIdx[
                    evt.GetInt()]))

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
        fileMask = '%(all)s (*)|*|' % {'all': _('All files')}
        if not ogr:
            extList = rasterFormatExtension
            fileMask += ('%(name)s (*.%(low1)s;*.%(low2)s;*.%(up1)s;*.%(up2)s)|'
                         '*.%(low1)s;*.%(low2)s;*.%(up1)s;*.%(up2)s|' %
                         {'name': 'GeoTIFF', 'low1': 'tif', 'low2': 'tiff', 'up1': 'TIF', 'up2': 'TIFF'})
        else:
            extList = vectorFormatExtension
            fileMask += '%(name)s (*.%(low)s;*.%(up)s)|*.%(low)s;*.%(up)s|' % {
                'name': 'ESRI Shapefile', 'low': 'shp', 'up': 'SHP'}

        for name, ext in sorted(extList.items()):
            if name in ('ESRI Shapefile', 'GeoTIFF'):
                continue
            fileMask += '%(name)s (*.%(low)s;*.%(up)s)|*.%(low)s;*.%(up)s|' % {
                'name': name, 'low': ext.lower(), 'up': ext.upper()}
        fileMask += '%s (*.zip;*.ZIP)|*.zip;*.ZIP|' % _('ZIP files')
        fileMask += '%s (*.gz;*.GZ)|*.gz;*.GZ|' % _('GZIP files')
        fileMask += '%s (*.tar;*.TAR)|*.tar;*.TAR|' % _('TAR files')
        # don't include last '|' - windows and mac throw error
        fileMask += '%s (*.tar.gz;*.TAR.GZ;*.tgz;*.TGZ)|*.tar.gz;*.TAR.GZ;*.tgz;*.TGZ' % _('TARGZ files')
        # only contains formats with extensions hardcoded

        self.filePanel = wx.Panel(parent=self)
        browse = filebrowse.FileBrowseButton(
            parent=self.filePanel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            labelText=_('File:'),
            dialogTitle=_('Choose file to import'),
            buttonText=_('Browse'),
            startDirectory=os.getcwd(),
            changeCallback=self.OnUpdate,
            fileMask=fileMask)
        browse.GetChildren()[1].SetName('GdalSelectDataSource')
        self.fileWidgets['browse'] = browse
        self.fileWidgets['options'] = TextCtrl(parent=self.filePanel)

        # directory
        self.dirPanel = wx.Panel(parent=self)
        browse = filebrowse.DirBrowseButton(
            parent=self.dirPanel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            labelText=_('Directory:'),
            dialogTitle=_('Choose input directory'),
            buttonText=_('Browse'),
            startDirectory=os.getcwd(),
            changeCallback=self.OnUpdate)
        browse.GetChildren()[1].SetName('GdalSelectDataSource')

        self.dirWidgets['browse'] = browse
        formatSelect = wx.Choice(parent=self.dirPanel)
        self.dirWidgets['format'] = formatSelect
        fileFormats = GetFormats(writableOnly=dest)[fType]['file']
        formatSelect.SetItems(sorted(list(fileFormats)))
        formatSelect.Bind(
            wx.EVT_CHOICE, lambda evt: self.SetExtension(
                self.dirWidgets['format'].GetStringSelection()))
        formatSelect.Bind(wx.EVT_CHOICE, self.OnUpdate)

        self.dirWidgets['extensionLabel'] = StaticText(
            parent=self.dirPanel, label=_("Extension:"))
        self.dirWidgets['extension'] = TextCtrl(parent=self.dirPanel)
        self.dirWidgets['extension'].Bind(wx.EVT_TEXT, self.ExtensionChanged)
        self.dirWidgets['options'] = TextCtrl(parent=self.dirPanel)
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
        dbChoice.Bind(
            wx.EVT_CHOICE,
            lambda evt: self.SetDatabase(
                db=dbChoice.GetStringSelection()))
        self.dbWidgets['format'] = dbChoice

        browse = filebrowse.FileBrowseButton(
            parent=self.dbPanel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            labelText=_("Name:"),
            dialogTitle=_('Choose file'),
            buttonText=_('Browse'),
            startDirectory=os.getcwd(),
            changeCallback=self.OnUpdate)
        browse.GetChildren()[1].SetName('GdalSelectDataSource')

        self.dbWidgets['browse'] = browse
        self.dbWidgets['choice'] = wx.Choice(
            parent=self.dbPanel, name='GdalSelectDataSource')
        self.dbWidgets['choice'].Bind(wx.EVT_CHOICE, self.OnUpdate)
        self.dbWidgets['text'] = TextCtrl(
            parent=self.dbPanel, name='GdalSelectDataSource')
        self.dbWidgets['text'].Bind(wx.EVT_TEXT, self.OnUpdate)
        self.dbWidgets['textLabel1'] = StaticText(
            parent=self.dbPanel, label=_("Name:"))
        self.dbWidgets['textLabel2'] = StaticText(
            parent=self.dbPanel, label=_("Name:"))
        self.dbWidgets['featType'] = wx.RadioBox(
            parent=self.dbPanel,
            id=wx.ID_ANY,
            label=" %s " %
            _("Feature type:"),
            choices=[
                _("simple features"),
                _("topological")],
            majorDimension=2,
            style=wx.RA_SPECIFY_COLS)
        if dest:
            self.dbWidgets['featType'].Disable()
        else:
            self.dbWidgets['featType'].Hide()
        browse = filebrowse.DirBrowseButton(
            parent=self.dbPanel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            labelText=_('Directory:'),
            dialogTitle=_('Choose input directory'),
            buttonText=_('Browse'),
            startDirectory=os.getcwd(),
            changeCallback=self.OnUpdate)
        self.dbWidgets['dirbrowse'] = browse
        self.dbWidgets['options'] = TextCtrl(parent=self.dbPanel)

        # protocol
        self.protocolPanel = wx.Panel(parent=self)
        protocolFormats = GetFormats(writableOnly=self.dest)[fType]['protocol']
        protocolChoice = wx.Choice(
            parent=self.protocolPanel,
            choices=protocolFormats)
        self.protocolWidgets['format'] = protocolChoice

        self.protocolWidgets['text'] = TextCtrl(parent=self.protocolPanel)
        self.protocolWidgets['text'].Bind(wx.EVT_TEXT, self.OnUpdate)
        self.protocolWidgets['options'] = TextCtrl(
            parent=self.protocolPanel)

        # native
        self.nativePanel = wx.Panel(parent=self)

        self._layout()
        sourceType = 'file'
        self.SetSourceType(sourceType)  # needed always to fit dialog size
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
            wx.CallAfter(self._postInit, sourceType, current)

    def _postInit(self, sourceType, data):
        """Fill in default values."""
        format = data.get('format', '')
        pg = 'conninfo' in data.keys()
        if pg:
            dsn = ''
            for item in data.get('conninfo').split(' '):
                k, v = item.split('=')
                if k == 'dbname':
                    dsn = v
                    break
            optList = list()
            for k, v in six.iteritems(data):
                if k in ('format', 'conninfo', 'topology'):
                    continue
                optList.append('%s=%s' % (k, v))
            options = ','.join(optList)
        else:
            dsn = data.get('dsn')
            options = data.get('options', '')

        self.SetSourceType(sourceType)
        self.source.SetSelection(self.sourceMap[sourceType])

        # v.external.out does not return dsn for the native format
        if dsn:
            dsn = os.path.expandvars(dsn)  # v.external.out uses $HOME
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
                    if 'topology' in data.keys():
                        self.dbWidgets['featType'].SetSelection(1)
                else:
                    self.dbWidgets[name].SetValue(dsn)

    def _layout(self):
        """Layout"""
        self.mainSizer = wx.BoxSizer(wx.VERTICAL)

        self.changingSizer = wx.StaticBoxSizer(self.inputBox, wx.VERTICAL)

        # file
        paddingSizer = wx.BoxSizer(wx.VERTICAL)
        sizer = wx.GridBagSizer(vgap=5, hgap=10)
        paddingSizer.Add(self.fileWidgets['browse'],
                         flag=wx.BOTTOM | wx.EXPAND,
                         border=35)
        sizer.Add(paddingSizer, flag=wx.EXPAND, pos=(0, 0), span=(1, 2))
        sizer.AddGrowableCol(0)
        if self.dest:
            sizer.Add(StaticText(parent=self.filePanel,
                                 label=_("Creation options:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(1, 0))
            sizer.Add(self.fileWidgets['options'],
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos=(1, 1))

        else:
            self.fileWidgets['options'].Hide()
        self.filePanel.SetSizer(sizer)

        # directory
        sizer = wx.GridBagSizer(vgap=3, hgap=10)
        sizer.Add(StaticText(parent=self.dirPanel,
                             label=_("Format:")),
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 0))
        sizer.Add(self.dirWidgets['format'],
                  flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                  pos=(0, 1))
        sizer.Add(self.dirWidgets['extensionLabel'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 2))
        sizer.Add(self.dirWidgets['extension'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 3))
        sizer.Add(self.dirWidgets['browse'],
                  flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                  pos=(1, 0), span=(1, 4))
        if self.dest:
            sizer.Add(StaticText(parent=self.dirPanel,
                                 label=_("Creation options:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(2, 0))
            sizer.Add(self.dirWidgets['options'],
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos=(2, 1))
            helpBtn = Button(parent=self.dirPanel, id=wx.ID_HELP)
            helpBtn.Bind(wx.EVT_BUTTON, self.OnHelp)
            sizer.Add(helpBtn,
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos=(2, 2))

            self.dirWidgets['extensionLabel'].Hide()
            self.dirWidgets['extension'].Hide()
        else:
            self.dirWidgets['options'].Hide()
        sizer.AddGrowableCol(1)
        self.dirPanel.SetSizer(sizer)

        # database
        sizer = wx.GridBagSizer(vgap=1, hgap=5)
        sizer.Add(StaticText(parent=self.dbPanel,
                             label=_("Format:")),
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 0))
        sizer.Add(self.dbWidgets['format'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 1))
        sizer.Add(self.dbWidgets['textLabel1'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(1, 0))
        sizer.Add(self.dbWidgets['text'],
                  flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                  pos=(1, 1), span=(1, 2))
        sizer.Add(self.dbWidgets['browse'],
                  flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                  pos=(2, 0), span=(1, 3))
        sizer.Add(self.dbWidgets['dirbrowse'],
                  flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                  pos=(3, 0), span=(1, 2))
        sizer.Add(self.dbWidgets['textLabel2'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(4, 0))
        sizer.Add(self.dbWidgets['choice'],
                  flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                  pos=(4, 1), span=(1, 2))
        if self.dest:
            sizer.Add(self.dbWidgets['featType'],
                      pos=(0, 2), flag=wx.EXPAND)

            sizer.Add(StaticText(parent=self.dbPanel,
                                 label=_("Creation options:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(5, 0))
            sizer.Add(self.dbWidgets['options'],
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos=(5, 1), span=(1, 2))

            # help button
            helpBtn = Button(parent=self.dbPanel, id=wx.ID_HELP)
            helpBtn.Bind(wx.EVT_BUTTON, self.OnHelp)
            sizer.Add(helpBtn,
                      pos=(5, 3))

        else:
            self.dbWidgets['options'].Hide()

        self.dbPanel.SetSizer(sizer)
        sizer.SetEmptyCellSize((0, 0))
        sizer.AddGrowableCol(1)

        # protocol
        sizer = wx.GridBagSizer(vgap=3, hgap=3)
        sizer.Add(StaticText(parent=self.protocolPanel,
                             label=_("Format:")),
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 0))
        sizer.Add(self.protocolWidgets['format'],
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(0, 1))
        sizer.Add(StaticText(parent=self.protocolPanel,
                             label=_("Protocol:")),
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  pos=(1, 0))
        sizer.Add(self.protocolWidgets['text'],
                  flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                  pos=(1, 1))
        if self.dest:
            sizer.Add(StaticText(parent=self.protocolPanel,
                                 label=_("Creation options:")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(2, 0))
            sizer.Add(self.protocolWidgets['options'],
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos=(2, 1))

        else:
            self.protocolWidgets['options'].Hide()
        sizer.AddGrowableCol(1)
        self.protocolPanel.SetSizer(sizer)

        # native
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(StaticText(parent=self.nativePanel,
                             label=_("No settings available")),
                  flag=wx.ALL | wx.EXPAND, border=5)
        self.nativePanel.SetSizer(sizer)

        for panel in (self.nativePanel, self.filePanel,
                      self.dirPanel, self.dbPanel,
                      self.protocolPanel):

            self.changingSizer.Add(panel, proportion=1,
                                   flag=wx.EXPAND)

        self.mainSizer.Add(self.source, proportion=0,
                           flag=wx.LEFT | wx.RIGHT | wx.EXPAND, border=5)
        self.mainSizer.Add(self.changingSizer, proportion=1,
                           flag=wx.ALL | wx.EXPAND, border=5)
        self.SetSizer(self.mainSizer)
        self.mainSizer.Fit(self)

    def _getExtension(self, name):
        """Get file extension by format name"""
        formatToExt = dict()
        formatToExt.update(rasterFormatExtension)
        formatToExt.update(vectorFormatExtension)

        return formatToExt.get(name, '')

    def SetSourceType(self, sourceType):
        """Set source type (db, file, dir, ...).
        Does not switch radioboxes."""
        self._sourceType = sourceType
        self.changingSizer.Show(
            self.filePanel, show=(
            sourceType == 'file'))
        self.changingSizer.Show(
            self.nativePanel, show=(
            sourceType == 'native'))
        self.changingSizer.Show(self.dirPanel, show=(sourceType == 'dir'))
        self.changingSizer.Show(
            self.protocolPanel, show=(
            sourceType == 'pro'))
        self.changingSizer.Show(self.dbPanel, show=(sourceType == 'db'))

        self.changingSizer.Layout()

        if sourceType == 'db':
            self.dbWidgets['format'].SetItems(self.dbFormats)
            if self.dbFormats:
                if 'PostgreSQL' in self.dbFormats:
                    self.dbWidgets['format'].SetStringSelection('PostgreSQL')
                else:
                    self.dbWidgets['format'].SetSelection(0)
            self.dbWidgets['format'].Enable()

        if sourceType == 'db':
            db = self.dbWidgets['format'].GetStringSelection()
            self.SetDatabase(db)

        if not self.dest:
            self.reloadDataRequired.emit(listData=None, data=None)
            self._reloadLayers()

    def OnSettingsChanged(self, data):
        """User changed setting"""
        # data list: [type, dsn, format, options]
        if len(data) == 3:
            data.append('')
        elif len(data) < 3:
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
        elif data[0] == 'db':
            name = self._getCurrentDbWidgetName()
            if name == 'choice':
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

        if not self.dest:
            self.reloadDataRequired.emit(listData=None, data=None)
            self._reloadLayers()

    def AttachSettings(self):
        if self.ogr:
            settingsFile = os.path.join(GetSettingsPath(), 'wxOGR')
        else:
            settingsFile = os.path.join(GetSettingsPath(), 'wxGDAL')

        self.settsManager = ManageSettingsWidget(parent=self,
                                                 settingsFile=settingsFile)
        self.settsManager.settingsChanged.connect(self.OnSettingsChanged)
        self.settsManager.settingsSaving.connect(self.OnSettingsSaving)

        # do layout
        self.mainSizer.Insert(0, self.settsManager,
                              flag=wx.ALL | wx.EXPAND, border=5)

    def OnSettingsSaving(self, name):
        """Saving data"""
        if not self.GetDsn():
            GMessage(parent=self, message=_(
                "No data source defined, settings are not saved."))
            return

        self.settsManager.SetDataToSave((self._sourceType, self.GetDsn(),
                                         self.GetFormat(), self.GetOptions()))
        self.settsManager.SaveSettings(name)

    def _getExtPatternGlob(self, ext):
        """Get pattern for case-insensitive globing"""
        pattern = '*.'
        for c in ext:
            pattern += '[%s%s]' % (c.lower(), c.upper())
        return pattern

    def _getCurrentDbWidgetName(self):
        """Returns active dns database widget name."""
        for widget in ('browse', 'dirbrowse', 'text', 'choice'):
            if self.dbWidgets[widget].IsShown():
                return widget

    def GetDsn(self):
        """Get datasource name
        """
        if self._sourceType == 'db':
            if self.dbWidgets['format'].GetStringSelection() in(
                    'PostgreSQL', 'PostGIS Raster driver'):

                dsn = 'PG:dbname=%s' % self.dbWidgets[
                    'choice'].GetStringSelection()
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
        """Update database panel."""
        sizer = self.dbPanel.GetSizer()
        showBrowse = db in ('SQLite', 'Rasterlite')
        showDirbrowse = db in ('FileGDB')
        showChoice = db in ('PostgreSQL', 'PostGIS WKT Raster driver',
                            'PostGIS Raster driver')
        enableFeatType = self.dest and self.ogr and db in ('PostgreSQL')
        showText = not(showBrowse or showChoice or showDirbrowse)

        sizer.Show(self.dbWidgets['browse'], show=showBrowse)
        sizer.Show(self.dbWidgets['dirbrowse'], show=showDirbrowse)
        sizer.Show(self.dbWidgets['choice'], show=showChoice)
        sizer.Show(self.dbWidgets['textLabel2'], show=showChoice)
        sizer.Show(self.dbWidgets['text'], show=showText)
        sizer.Show(self.dbWidgets['textLabel1'], show=showText)
        self.dbWidgets['featType'].Enable(enableFeatType)
        if showChoice:
            # try to get list of PG databases
            dbNames = RunCommand(
                'db.databases',
                parent=self,
                quiet=True,
                read=True,
                driver='pg').splitlines()
            if dbNames is not None:
                self.dbWidgets['choice'].SetItems(sorted(dbNames))
                self.dbWidgets['choice'].SetSelection(0)
            elif grass.find_program('psql', '--help'):
                if not self.dbWidgets['choice'].GetItems():
                    p = grass.Popen(['psql', '-ltA'], stdout=grass.PIPE)
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
        """Update required - load layers."""
        if not self.dest:
            self._reloadLayers()

        event.Skip()

    def _reloadLayers(self):
        """Reload list of layers"""

        def hasRastSameProjAsLocation(dsn):

            ret = RunCommand('r.external',
                             quiet=True,
                             read=True,
                             flags='t',
                             input=dsn)

            # v.external returns info for individual bands, however projection is shared by all bands ->
            # (it is possible to take first line)

            lines = ret.splitlines()
            projectionMatch = '0'
            if lines:
                bandNumber, bandType, projectionMatch = map(
                    lambda x: x.strip(), lines[0].split(','))

            return projectionMatch

        def getProjMatchCaption(projectionMatch):

            if projectionMatch == '0':
                projectionMatchCaption = _("No")
            else:
                projectionMatchCaption = _("Yes")

            return projectionMatchCaption

        dsn = self.GetDsn()
        if not dsn:
            return

        data = list()
        listData = list()
        layerId = 1

        if self.ogr:
            ret = RunCommand('v.external',
                             quiet=True,
                             read=True,
                             flags='t',
                             input=dsn)
            if not ret:
                self.reloadDataRequired.emit(listData=None, data=None)
                return

            layerId = 1
            for line in ret.splitlines():
                layerName, featureType, projectionMatch, geometryColumn = map(
                    lambda x: x.strip(), line.split(','))
                projectionMatchCaption = getProjMatchCaption(projectionMatch)
                grassName = GetValidLayerName(layerName)
                if geometryColumn:
                    featureType = geometryColumn + '/' + featureType
                listData.append(
                    (layerId,
                     layerName,
                     featureType,
                     projectionMatchCaption,
                     grassName))
                data.append(
                    (layerId,
                     layerName,
                     featureType,
                     int(projectionMatch),
                        grassName))
                layerId += 1
        else:
            if self._sourceType == 'file':
                baseName = os.path.basename(dsn)
                grassName = GetValidLayerName(baseName.split('.', -1)[0])
                projectionMatch = hasRastSameProjAsLocation(dsn)
                projectionMatchCaption = getProjMatchCaption(projectionMatch)
                listData.append(
                    (layerId, baseName, projectionMatchCaption, grassName))
                data.append(
                    (layerId, baseName, int(projectionMatch), grassName))
            elif self._sourceType == 'dir':
                ext = self.dirWidgets['extension'].GetValue()
                for filename in glob.glob(os.path.join(
                        dsn, "%s") % self._getExtPatternGlob(ext)):
                    baseName = os.path.basename(filename)
                    grassName = GetValidLayerName(baseName.split('.', -1)[0])
                    projectionMatch = hasRastSameProjAsLocation(filename)
                    projectionMatchCaption = getProjMatchCaption(
                        projectionMatch)
                    listData.append(
                        (layerId, baseName, projectionMatchCaption, grassName))
                    data.append(
                        (layerId, baseName, int(projectionMatch), grassName))
                    layerId += 1

        # emit signal
        self.reloadDataRequired.emit(listData=listData, data=data)

    def ExtensionChanged(self, event):
        if not self.dest:
            # reload layers
            self._reloadLayers()

    def SetExtension(self, name):
        """Extension changed"""
        ext = self._getExtension(name)
        self.dirWidgets['extension'].SetValue(ext)

    def GetType(self):
        """Get source type"""
        return self._sourceType

    def GetFormat(self):
        """Get format as string"""
        if self._sourceType == 'dir':
            format = self.dirWidgets['format'].GetStringSelection()
        elif self._sourceType == 'pro':
            format = self.protocolWidgets['format'].GetStringSelection()
        elif self._sourceType == 'db':
            format = self.dbWidgets['format'].GetStringSelection()
        else:
            format = ''

        return format.replace(' ', '_')

    def GetFormatExt(self):
        """Get format extension"""
        return self._getExtension(self.GetFormat())

    def GetOptions(self):
        """Get creation options"""
        if self._sourceType == 'file':
            options = self.fileWidgets['options'].GetValue()
        elif self._sourceType == 'dir':
            options = self.dirWidgets['options'].GetValue()
        elif self._sourceType == 'pro':
            options = self.protocolWidgets['options'].GetValue()
        elif self._sourceType == 'db':
            if self.dbWidgets['featType'].GetSelection() == 1:
                options = 'topology=yes '
            else:
                options = ''
            options += self.dbWidgets['options'].GetValue()

        return options.strip()

    def OnHelp(self, event):
        """Show related manual page"""
        cmd = ''
        if self.dest:
            if self.ogr:
                cmd = 'v.external.out'
            else:
                cmd = 'r.external.out'
        else:
            if self.link:
                if self.ogr:
                    cmd = 'v.external'
                else:
                    cmd = 'r.external'
            else:
                if self.ogr:
                    cmd = 'v.in.ogr'
                else:
                    cmd = 'r.in.gdal'

        RunCommand('g.manual', entry=cmd)


class ProjSelect(wx.ComboBox):
    """Widget for selecting input raster/vector map used by
    r.proj/v.proj modules."""

    def __init__(self, parent, isRaster, id=wx.ID_ANY,
                 size=globalvar.DIALOG_COMBOBOX_SIZE, **kwargs):
        super(ProjSelect, self).__init__(parent, id, size=size, **kwargs)
        self.SetName("ProjSelect")
        self.isRaster = isRaster

    def UpdateItems(self, dbase, location, mapset):
        """Update list of maps

        """
        if not dbase:
            dbase = grass.gisenv()['GISDBASE']
        if not mapset:
            mapset = grass.gisenv()['MAPSET']
        if self.isRaster:
            ret = RunCommand('r.proj',
                             quiet=True,
                             read=True,
                             flags='l',
                             dbase=dbase,
                             location=location,
                             mapset=mapset)
        else:
            ret = RunCommand('v.proj',
                             quiet=True,
                             read=True,
                             flags='l',
                             dbase=dbase,
                             location=location,
                             mapset=mapset)
        listMaps = list()
        if ret:
            for line in ret.splitlines():
                listMaps.append(line.strip())
        ListSortLower(listMaps)

        self.SetItems(listMaps)
        self.SetValue('')


class ElementSelect(wx.Choice):

    def __init__(self, parent, id=wx.ID_ANY, elements=None,
                 size=globalvar.DIALOG_COMBOBOX_SIZE,
                 **kwargs):
        """Widget for selecting GIS element

        :param parent: parent window
        :param elements: filter elements
        """
        super(ElementSelect, self).__init__(parent, id, size=size,
                                            **kwargs)
        self.SetName("ElementSelect")

        task = gtask.parse_interface('g.list')
        p = task.get_param(value='type')
        self.values = p.get('values', [])
        self.valuesDesc = p.get('values_desc', [])

        if elements:
            values = []
            valuesDesc = []
            for idx in range(0, len(self.values)):
                value = self.values[idx]
                if value in elements:
                    values.append(value)
                    valuesDesc.append(self.valuesDesc[idx])
            self.values = values
            self.valuesDesc = valuesDesc

        self.SetItems(self.valuesDesc)

    def GetValue(self, name):
        """Translate value

        :param name: element name
        """
        idx = self.valuesDesc.index(name)
        if idx > -1:
            return self.values[idx]
        return ''


class OgrTypeSelect(wx.Panel):

    def __init__(self, parent, panel, **kwargs):
        """Widget to choose OGR feature type

        :param parent: parent window
        :param panel: wx.Panel instance used as parent window
        """
        wx.Panel.__init__(self, parent=panel, id=wx.ID_ANY)

        self.ftype = wx.Choice(parent=self, id=wx.ID_ANY, size=(
            200, -1), choices=(_("Point"), _("LineString"), _("Polygon")))
        self._layout()

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.HORIZONTAL)
        sizer.Add(StaticText(parent=self,
                             id=wx.ID_ANY,
                             label=_("Feature type:")),
                  proportion=1,
                  flag=wx.ALIGN_CENTER_VERTICAL,
                  border=5)
        sizer.Add(self.ftype,
                  proportion=0,
                  flag=wx.EXPAND | wx.ALIGN_RIGHT)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def GetType(self):
        """Get selected type as string

        :return: feature type as string
        """
        sel = self.ftype.GetSelection()
        if sel == 0:
            return 'point'
        elif sel == 1:
            return 'line'
        elif sel == 2:
            return 'boundary'


class CoordinatesSelect(Panel):

    def __init__(self, parent, giface, multiple=False, **kwargs):
        """Widget to get coordinates from map window  by mouse click

        :param parent: parent window
        :param giface: GRASS interface
        :param multiple: - True if it is possible to insert more coordinates
        """
        self._giface = giface
        self.multiple = multiple
        self.mapWin = None
        self.drawMapWin = None

        super(CoordinatesSelect, self).__init__(parent=parent, id=wx.ID_ANY)

        self.coordsField = TextCtrl(parent=self, id=wx.ID_ANY,
                                    size=globalvar.DIALOG_TEXTCTRL_SIZE,
                                    validator=CoordinatesValidator())

        icon = wx.Bitmap(
            os.path.join(
                globalvar.ICONDIR,
                "grass",
                "pointer.png"))
        self.buttonInsCoords = buttons.ThemedGenBitmapToggleButton(
            parent=self, id=wx.ID_ANY, bitmap=icon, size=globalvar.DIALOG_COLOR_SIZE)
        self.registered = False
        self.buttonInsCoords.Bind(wx.EVT_BUTTON, self._onClick)

        mapdisp = self._giface.GetMapDisplay()
        if mapdisp:
            switcher = mapdisp.GetToolSwitcher()
            switcher.AddCustomToolToGroup(
                group='mouseUse',
                btnId=self.buttonInsCoords.GetId(),
                toggleHandler=self.buttonInsCoords.SetValue)
        self._doLayout()
        self.coordsField.Bind(wx.EVT_TEXT, lambda event: self._draw(delay=1))

    def _doLayout(self):
        self.dialogSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.dialogSizer.Add(self.coordsField,
                             proportion=1,
                             flag=wx.EXPAND)
        self.dialogSizer.Add(self.buttonInsCoords)
        self.SetSizer(self.dialogSizer)

    def _onClick(self, event):
        """Button for interacitve inserting of coordinates clicked"""

        self.mapWin = self._giface.GetMapWindow()
        if self.buttonInsCoords.GetToggle() and self.mapWin:
            switcher = self._giface.GetMapDisplay().GetToolSwitcher()
            switcher.ToolChanged(self.buttonInsCoords.GetId())
            if not self.mapWin.RegisterMouseEventHandler(wx.EVT_LEFT_DOWN,
                                                         self._onMapClickHandler,
                                                         'cross'):
                return

            self.registered = True
            self._giface.GetMapDisplay().Raise()
        else:
            if self.mapWin and self.mapWin.UnregisterMouseEventHandler(
                    wx.EVT_LEFT_DOWN, self._onMapClickHandler):
                self.registered = False
                return

    def drawCleanUp(self):
        if self.drawMapWin:
            self.drawMapWin.UnregisterGraphicsToDraw(self.pointsToDraw)

    def _draw(self, delay):
        """Draws points representing inserted coordinates in mapwindow."""
        if self.drawMapWin != self.mapWin:
            self.drawCleanUp()
            if self.mapWin:
                self.drawMapWin = self.mapWin
                self.pointsToDraw = self.drawMapWin.RegisterGraphicsToDraw(
                    graphicsType="point")

        if self.drawMapWin:
            items = self.pointsToDraw.GetAllItems()
            for i in items:
                self.pointsToDraw.DeleteItem(i)

            coords = self._getCoords()
            if coords is not None:
                for i in range(len(coords) // 2):
                    i = i * 2
                    self.pointsToDraw.AddItem(
                        coords=(coords[i], coords[i + 1]))

            self._giface.updateMap.emit(
                render=False, renderVector=False, delay=delay)

    def _getCoords(self):
        """Get list of coordinates.

        :return: None if values are not valid
        """
        if self.coordsField.GetValidator().Validate():
            return self.coordsField.GetValue().split(',')

        return None

    def _onMapClickHandler(self, event):
        """Gets coordinates from mapwindow"""
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

        self._draw(delay=0)

    def OnClose(self):
        """Unregistrates _onMapClickHandler from mapWin"""
        self.drawCleanUp()
        self._giface.updateMap.emit(render=False, renderVector=False)

        mapdisp = self._giface.GetMapDisplay()
        if mapdisp:
            switcher = mapdisp.GetToolSwitcher()
            switcher.RemoveCustomToolFromGroup(self.buttonInsCoords.GetId())

        if self.mapWin and self.registered:
            self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN,
                                                    self._onMapClickHandler)

    def GetTextWin(self):
        """Get TextCtrl widget"""
        return self.coordsField


class VectorCategorySelect(wx.Panel):
    """Widget that allows interactive selection of vector features"""

    def __init__(self, parent, giface, task=None):
        super(VectorCategorySelect, self).__init__(parent=parent, id=wx.ID_ANY)
        self.task = task
        self.parent = parent
        self.giface = giface

        self.selectedFeatures = None
        self.registered = False
        self._vectorSelect = None

        self.mapdisp = self.giface.GetMapDisplay()

        self.catsField = TextCtrl(parent=self, id=wx.ID_ANY,
                                  size=globalvar.DIALOG_TEXTCTRL_SIZE)

        icon = wx.Bitmap(
            os.path.join(
                globalvar.ICONDIR,
                "grass",
                "select.png"))
        self.buttonVecSelect = buttons.ThemedGenBitmapToggleButton(
            parent=self, id=wx.ID_ANY, bitmap=icon, size=globalvar.DIALOG_COLOR_SIZE)
        self.buttonVecSelect.Bind(wx.EVT_BUTTON, self._onClick)

        if self.mapdisp:
            switcher = self.mapdisp.GetToolSwitcher()
            switcher.AddCustomToolToGroup(
                group='mouseUse',
                btnId=self.buttonVecSelect.GetId(),
                toggleHandler=self.buttonVecSelect.SetValue)

        self._layout()

    def _isMapSelected(self):
        """Check if layer list contains at least one selected map
        """
        layerList = self.giface.GetLayerList()
        layerSelected = layerList.GetSelectedLayer()
        if layerSelected is None:
            GWarning(
                _("No vector map selected in layer manager. Operation canceled."))
            return False

        return True

    def _chckMap(self):
        """Check if selected map in 'input' widget is the same as selected map in lmgr """
        if self._isMapSelected():
            layerList = self.giface.GetLayerList()
            layerSelected = layerList.GetSelectedLayer()
            # d.vect module
            inputName = self.task.get_param(value='map', raiseError=False)
            if not inputName:
                inputName = self.task.get_param('input')
            if inputName['value'] != str(layerSelected):
                if inputName['value'] == '' or inputName['value'] is None:
                    GWarning(_("Input vector map is not selected"))
                    return False
                GWarning(
                    _(
                        "Input vector map <%s> and selected map <%s> in layer manager are different. "
                        "Operation canceled.") %
                    (inputName['value'], str(layerSelected)))
                return False
            return True
        return False

    def _onClick(self, evt=None):
        if self.task is not None:
            if not self._chckMap():
                self.buttonVecSelect.SetValue(False)
                return
        else:
            if not self._isMapSelected():
                self.buttonVecSelect.SetValue(False)
                return
        if self._vectorSelect is None:

            if self.mapdisp:
                if self.buttonVecSelect.IsEnabled():
                    switcher = self.mapdisp.GetToolSwitcher()
                    switcher.ToolChanged(self.buttonVecSelect.GetId())

                self._vectorSelect = VectorSelectBase(
                    self.mapdisp, self.giface)
                if not self.mapdisp.GetWindow().RegisterMouseEventHandler(
                        wx.EVT_LEFT_DOWN, self._onMapClickHandler, 'cross'):
                    return
                self.registered = True
                self.mapdisp.Raise()
        else:
            self.OnClose()

    def OnClose(self, event=None):
        if not self.mapdisp:
            return

        switcher = self.mapdisp.GetToolSwitcher()
        switcher.RemoveCustomToolFromGroup(self.buttonVecSelect.GetId())
        if self._vectorSelect is not None:
            tmp = self._vectorSelect.GetLineStringSelectedCats()
            self._vectorSelect.OnClose()
            self.catsField.SetValue(tmp)
        self._vectorSelect = None

    def _onMapClickHandler(self, event):
        """Update category text input widget"""
        if event == "unregistered":
            return

        if self.task is None:
            if not self._isMapSelected():
                self.OnClose()
            else:
                self.catsField.SetValue(
                    self._vectorSelect.GetLineStringSelectedCats())
        else:
            if not self._chckMap():
                self.OnClose()
            else:
                self.catsField.SetValue(
                    self._vectorSelect.GetLineStringSelectedCats())

    def GetTextWin(self):
        return self.catsField

    def GetValue(self):
        return self.catsField.GetValue()

    def SetValue(self, value):
        self.catsField.SetValue(value)

    def _layout(self):
        self.dialogSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.dialogSizer.Add(self.catsField,
                             proportion=1,
                             flag=wx.EXPAND)

        self.dialogSizer.Add(self.buttonVecSelect)
        self.SetSizer(self.dialogSizer)


class SignatureSelect(wx.ComboBox):
    """Widget for selecting signatures"""

    def __init__(self, parent, element, id=wx.ID_ANY,
                 size=globalvar.DIALOG_GSELECT_SIZE, **kwargs):
        super(SignatureSelect, self).__init__(parent, id, size=size,
                                              **kwargs)
        self.element = element
        self.SetName("SignatureSelect")

    def Insert(self, group, subgroup=None):
        """Insert signatures for defined group/subgroup

        :param group: group name (can be fully-qualified)
        :param subgroup: non fully-qualified name of subgroup
        """
        if not group:
            return
        gisenv = grass.gisenv()
        try:
            name, mapset = group.split('@', 1)
        except ValueError:
            name = group
            mapset = gisenv['MAPSET']

        path = os.path.join(
            gisenv['GISDBASE'],
            gisenv['LOCATION_NAME'],
            mapset, 'group', name)

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


class SeparatorSelect(wx.ComboBox):
    """Widget for selecting seperator"""

    def __init__(self, parent, id=wx.ID_ANY,
                 size=globalvar.DIALOG_GSELECT_SIZE, **kwargs):
        super(SeparatorSelect, self).__init__(parent, id, size=size,
                                              **kwargs)
        self.SetName("SeparatorSelect")
        self.SetItems(['pipe', 'comma', 'space', 'tab', 'newline'])


class SqlWhereSelect(wx.Panel):

    def __init__(self, parent, **kwargs):
        """Widget to define SQL WHERE condition.

        :param parent: parent window
        """
        super(SqlWhereSelect, self).__init__(parent=parent, id=wx.ID_ANY)
        self.parent = parent
        self.vector_map = None

        self.sqlField = TextCtrl(parent=self, id=wx.ID_ANY,
                                 size=globalvar.DIALOG_TEXTCTRL_SIZE)
        self.GetChildren()[0].SetName("SqlWhereSelect")
        icon = wx.Bitmap(
            os.path.join(
                globalvar.ICONDIR,
                "grass",
                "table.png"))
        self.buttonInsSql = buttons.ThemedGenBitmapButton(
            parent=self, id=wx.ID_ANY, bitmap=icon, size=globalvar.DIALOG_COLOR_SIZE)
        self.buttonInsSql.Bind(wx.EVT_BUTTON, self._onClick)

        self._doLayout()


    def _doLayout(self):
        self.dialogSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.dialogSizer.Add(self.sqlField,
                             proportion=1,
                             flag=wx.EXPAND)
        self.dialogSizer.Add(self.buttonInsSql)
        self.SetSizer(self.dialogSizer)

    def GetTextWin(self):
        return self.sqlField

    def _onClick(self, event):
        from dbmgr.sqlbuilder import SQLBuilderWhere
        try:
            if not self.vector_map:
                raise GException(_('No vector map selected'))
            win = SQLBuilderWhere(parent=self,
                                  vectmap=self.vector_map,
                                  layer=self.vector_layer)
            win.Show()
        except GException as e:
            GMessage(parent=self.parent, message='{}'.format(e))

    def SetData(self, vector, layer):
        self.vector_map = vector
        self.vector_layer = int(layer) # TODO: support layer names

    def SetValue(self, value):
        self.sqlField.SetValue(value)
