"""!
@package vnet.widgets

@brief Base class for list of points. 

Classes:
 - widgets::PointsList
 - widgets::EditItem

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Original author Michael Barton
@author Original version improved by Martin Landa <landa.martin gmail.com>
@author Rewritten by Markus Metz redesign georectfier -> GCP Manage
@author Stepan Turek <stepan.turek seznam.cz> (Created PointsList from GCPList) (GSoC 2012, mentor: Martin Landa)
"""

import os
import wx
from copy import copy, deepcopy

import wx
from wx.lib.mixins.listctrl import CheckListCtrlMixin, ColumnSorterMixin, ListCtrlAutoWidthMixin, TextEditMixin

from core import globalvar


class PointsList(wx.ListCtrl,
                 CheckListCtrlMixin,
                 ListCtrlAutoWidthMixin,
                 ColumnSorterMixin):

    def __init__(self, parent, cols, id=wx.ID_ANY,
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.LC_REPORT | wx.SUNKEN_BORDER | wx.LC_HRULES |
                 wx.LC_SINGLE_SEL):
        """!Creates list for points. 

        PointsList class was created from GCPList class in GCP manager. It is possible 
        to be shared by GCP and VNET front end.

        Important parameters:
        @param cols is list containing list items. which represents columns.
                This columns will be added in order as they are in list. 
                Class will add as first column "use" with number of point and checkbox.
                Structure of list item must be this:
               -1. item: column name 
               -2. item: column label                
               -3. item: If column is editable by user, it must contain convert function to convert
                         inserted string to it's type for sorting. Use None for not editable 
                         columns. Values for insertion can be in list. This allows insert
                         just values in the list. 
               -4. item: Default value for column cell. Value should be given in it's  type 
                         in order to sorting would work properly. If 3. item is list, it must be index
                         of some item in the list.
  
        Example of cols parameter:
                 column name, column label, convert function, default val
        @code
         cols =   [
                   ['E', _('source E'), float, 0.0],
                   ['N', _('source N'), float, 0.0],
                   ['E', _('target E'), float, 0.0],
                   ['N', _('target N'), float, 0.0],
                   ['F_Err', _('Forward error'), None, 0],
                   ['B_Err', _(Backward error'), None, 0]
                   ['type', _('type'), [_(""), _("Start point"), _("End point")], 0] # Select from 3 choices ("Start point", "End point"), 
                                                                                     # Choice with index 0 ("") is default.
                  ]
        @endcode 
        """

        wx.ListCtrl.__init__(self, parent, id, pos, size, style)

        # Mixin settings
        CheckListCtrlMixin.__init__(self)
        ListCtrlAutoWidthMixin.__init__(self)
        # TextEditMixin.__init__(self)

        # inserts first column with points numbers and checkboxes
        cols.insert(0, ['use', _('use'), False, 0])

        self.colsData = cols
        self.dataTypes = {"colName" : 0,
                          "colLabel" : 1,
                          "colEditable" : 2,
                          "itemDefaultValue" : 3} # just for better understanding

        # information whether list items are checked or not
        self.CheckList = [] 

        self._createCols()
        self.hiddenCols = {}

        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnItemActivated)
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColClick)

        self.selected = wx.NOT_FOUND
        self.selectedkey = -1


        # CheckListCtrlMixin must set an ImageList first
        self.il = self.GetImageList(wx.IMAGE_LIST_SMALL)

        # images for column sorting
        SmallUpArrow = wx.BitmapFromImage(self.getSmallUpArrowImage())            
        SmallDnArrow = wx.BitmapFromImage(self.getSmallDnArrowImage())            
        self.sm_dn = self.il.Add(SmallDnArrow)
        self.sm_up = self.il.Add(SmallUpArrow)

        # initialize column sorter
        self.itemDataMap = [] 
        ncols = self.GetColumnCount()
        ColumnSorterMixin.__init__(self, ncols)

        # init to ascending sort on first click
        self._colSortFlag = [1] * ncols

        # same structure as itemDataMap, information about choice index selected
        # if cell is in column without values to choose then is -1 
        self.selIdxs = []

        self.ResizeColumns()
        self.SetColumnWidth(0, 50)

    def _createCols(self):
        """!Creates columns in list"""
        if 0:
            # normal, simple columns
            for col in enumerate(self.colsData):
                iLabel = self.dataTypes["colLabel"]
                self.InsertColumn(col[0], col[1][iLabel])
        else:
            # the hard way: we want images on the column header
            info = wx.ListItem()
            info.SetMask(wx.LIST_MASK_TEXT | wx.LIST_MASK_IMAGE | wx.LIST_MASK_FORMAT)
            info.SetImage(-1)
            info.m_format = wx.LIST_FORMAT_LEFT

            for col in enumerate(self.colsData):
                iLabel = self.dataTypes["colLabel"]
                info.SetText(col[1][iLabel]) 
                self.InsertColumnInfo(col[0], info)

    def AddItem(self, event):
        """!Appends an item to list with default values"""
        iDefVal = self.dataTypes["itemDefaultValue"]
        iColEd = self.dataTypes["colEditable"]
        itemData = []
        itemIndexes = []
        for col in self.colsData:
            if type(col[iColEd]).__name__ == "list":
                itemData.append(col[iColEd][col[iDefVal]])
                itemIndexes.append(col[iDefVal])
            else:
                itemData.append(col[iDefVal])
                itemIndexes.append(-1)# not a choise column 

                
        self.selIdxs.append(itemIndexes) 

        for hCol in self.hiddenCols.itervalues():
            defVal = hCol['colsData'][iDefVal]
            if type(hCol['colsData'][iColEd]).__name__ == "list":
                hCol['itemDataMap'].append(hCol['colsData'][iColEd][defVal])
                hCol['selIdxs'].append(defVal)
            else:
                hCol['itemDataMap'].append(defVal)
                hCol['selIdxs'].append(-1)

        self.selectedkey = self.GetItemCount()

        itemData[0] = self.selectedkey + 1
        self.itemDataMap.append(copy(itemData))

        self.Append(map(str, itemData))             

        self.selected = self.GetItemCount() - 1
        self.SetItemData(self.selected, self.selectedkey)

        self.SetItemState(self.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)
        self.ResizeColumns()

        return self.selected

    def GetCellValue(self, key, colName):
        """!Get value in cell of list using key (same regardless of sorting)"""
        colNum = self._getColumnNum(colName)
        iColEd = self.dataTypes["colEditable"]      
        return self.itemDataMap[key][colNum]

    def GetCellSelIdx(self, key, colName):
        """!Get selected index in cell of list using key (same regardless of sorting)

            @return number of chosen value, if column has values to choose
            @return -1 if column does not has values to choose
        """
        colNum = self._getColumnNum(colName)
        iColEd = self.dataTypes["colEditable"]
        return self.selIdxs[key][colNum]

    def EditCellIndex(self, index, colName, cellData):
        """!Changes value in list using key (same regardless of sorting)"""
        colNum = self._getColumnNum(colName) 
        key = self.GetItemData(index)

        iColEd = self.dataTypes["colEditable"]
        if type(self.colsData[colNum][iColEd]).__name__ == "list":
            cellVal = self.colsData[colNum][iColEd][cellData]
            self.selIdxs[key][colNum] = cellData
        else:
            cellVal = cellData  
            self.selIdxs[key][colNum] = -1

        self.itemDataMap[key][colNum] = cellVal
        self.SetStringItem(index, colNum, str(cellVal))

    def EditCellKey(self, key, colName, cellData):
        """!Changes value in list using index (changes during sorting)"""
        colNum = self._getColumnNum(colName)   

        iColEd = self.dataTypes["colEditable"]
        if type(self.colsData[colNum][iColEd]).__name__ == "list":
            cellVal = self.colsData[colNum][iColEd][cellData]
            self.selIdxs[key][colNum] = cellData
        else:
            cellVal = cellData  
            self.selIdxs[key][colNum] = -1

        self.itemDataMap[key][colNum] = cellVal
        index = self._findIndex(key)

        self.SetStringItem(index, colNum, str(cellVal))

    def _findIndex(self, key):
        """!Find index for key"""
        index = -1
        while True:
            index = self.GetNextItem(index,
                                     wx.LIST_NEXT_BELOW)
            if key == self.GetItemData(index):
                return index
            if index == -1:
                break
        return -1

    def ChangeColEditable(self, colName, colType):
        """!Change 3. item in constructor parameter cols (see the class constructor hint)"""     
        colNum = self._getColumnNum(colName)
        iColEd = self.dataTypes["colEditable"]
        self.colsData[colNum][iColEd] = colType

    def DeleteItem(self, event = None):
        """!Delete selected item in list"""
        if self.selected == wx.NOT_FOUND:
            return

        key = self.GetItemData(self.selected)
        wx.ListCtrl.DeleteItem(self, self.selected)

        del self.itemDataMap[key]
        self.selIdxs.pop(key)

        # update hidden columns
        for hCol in self.hiddenCols.itervalues():    
            hCol['itemDataMap'].pop(key) 
            hCol['selIdxs'].pop(key)

        # update key and point number
        for newkey in range(key, len(self.itemDataMap)):
            index = self.FindItemData(-1, newkey + 1)
            self.itemDataMap[newkey][0] = newkey
            self.SetStringItem(index, 0, str(newkey + 1))
            self.SetItemData(index, newkey)

        # update selected
        if self.GetItemCount() > 0:
            if self.selected < self.GetItemCount():
                self.selectedkey = self.GetItemData(self.selected)
            else:
                self.selected = self.GetItemCount() - 1
                self.selectedkey = self.GetItemData(self.selected)
                
            self.SetItemState(self.selected,
                              wx.LIST_STATE_SELECTED,
                              wx.LIST_STATE_SELECTED)
        else:
            self.selected = wx.NOT_FOUND
            self.selectedkey = -1

    def ClearItem(self, event):
        """"!Set all values to default in selected item of points list and uncheck it."""
        if self.selected == wx.NOT_FOUND:
            return
        index = self.selected

        iDefVal = self.dataTypes["itemDefaultValue"]
        iColEd = self.dataTypes["colEditable"]

        i = 0
        for col in self.colsData:
            if i == 0:
                i  += 1
                continue
            if type(col[iColEd]).__name__ == "list":
                self.EditCell(index, i, col[iColEd][col[iDefVal]])
            else:
                self.EditCell(index, i, col[iDefVal])
            i  += 1
        self.CheckItem(index, False)

    def ResizeColumns(self, minWidth = [90, 120]):
        """!Resize columns"""
        for i in range(self.GetColumnCount()):
            self.SetColumnWidth(i, wx.LIST_AUTOSIZE)
            # first column is checkbox, don't set to minWidth
            if i > 0 and self.GetColumnWidth(i) < minWidth[i > 4]:
                self.SetColumnWidth(i, minWidth[i > 4])

        self.SendSizeEvent()

    def GetSelected(self):
        """!Get index of selected item."""
        return self.selected

    # Used by the ColumnSorterMixin, see wx/lib/mixins/listctrl.py
    def GetSortImages(self):
        return (self.sm_dn, self.sm_up)

    # Used by the ColumnSorterMixin, see wx/lib/mixins/listctrl.py
    def GetListCtrl(self):
        return self

    def OnItemActivated(self, event):
        """!When item is double clicked, open editor to edit editable columns."""
        data = []

        index = event.GetIndex()
        key = self.GetItemData(index)       
        changed = False
        iColEd = self.dataTypes["colEditable"]

        for col in  enumerate(self.colsData):
            if col[1][iColEd]:
                data.append([col[0], #culumn number
                             self.GetItem(index, col[0]).GetText(), #cell value 
                             col[1][iColEd]]) #convert function for type check

        if not data:
            return
        dlg = self.CreateEditDialog(data = data, pointNo = key)

        if dlg.ShowModal() == wx.ID_OK:
            editedData = dlg.GetValues() # string
            
            if len(editedData) == 0:
                GError(parent = self,
                       message=_("Invalid value inserted. Operation canceled."))
            else:
                i = 0
                for editedCell in editedData:
                    if editedCell[1] != data[i][1]:
                        self.SetStringItem(index, editedCell[0], str(editedCell[1]))
                        self.itemDataMap[key][editedCell[0]] = editedCell[1]
                        changed = True
                    i += 1 

                self.selIdxs[key] = dlg.GetSelectionIndexes()
        dlg.Destroy()
        return changed
        
    def CreateEditDialog(self, data, pointNo):
        """!Helper function
        It is possible to define in child derived class
        and adapt created dialog (e. g. it's title...) 
        """

        return  EditItem(parent=self, id=wx.ID_ANY, data = data, pointNo=pointNo)

    def OnColClick(self, event):
        """!ListCtrl forgets selected item..."""
        self.selected = self.FindItemData(-1, self.selectedkey)
        self.SetItemState(self.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)
        event.Skip()

    def OnItemSelected(self, event):
        """!Updates class attributes holding information about selected item"""
        if self.selected != event.GetIndex():
            self.selected = event.GetIndex()
            self.selectedkey = self.GetItemData(self.selected)

        event.Skip()

    def getSmallUpArrowImage(self):
        """!Get arrow up symbol for indication of sorting"""
        stream = open(os.path.join(globalvar.ETCIMGDIR, 'small_up_arrow.png'), 'rb')
        try:
            img = wx.ImageFromStream(stream)
        finally:
            stream.close()
        return img

    def getSmallDnArrowImage(self):
        """!Get arrow down symbol for indication of sorting"""
        stream = open(os.path.join(globalvar.ETCIMGDIR, 'small_down_arrow.png'), 'rb')
        try:
            img = wx.ImageFromStream(stream)
        finally:
            stream.close()
        return img

    def _getColumnNum(self, colName):
        """!Get position of column among showed columns 

        @param colName - name of column

        @return index of columns or -1 if col was not found
        """

        for iCol, col in enumerate(self.colsData):
             if colName == col[0]:
                 return iCol
 
        return -1

    def HideColumn(self, colName):
        """!Hide column (hidden columns are not editable)

        @param colName - name of column

        @return True - if column was hidden
        @return False - if position is not valid or column is not showed
        """
        colNum = self._getColumnNum(colName)
        if colNum == -1:
            return False

        hiddenCol = self.GetColumn(colNum)
        self.DeleteColumn(colNum) 

        self.hiddenCols[colName] = {}
        self.hiddenCols[colName]['wxCol'] = hiddenCol
        hiddenMaps = []
        hiddenSelIdxs = []
        for item in self.itemDataMap:
            hiddenMaps.append(item.pop(colNum))
        for item in self.selIdxs:
            hiddenSelIdxs.append(item.pop(colNum))

        self.hiddenCols[colName]['itemDataMap'] = hiddenMaps
        self.hiddenCols[colName]['selIdxs'] = hiddenSelIdxs
        self.hiddenCols[colName]['colsData'] = self.colsData.pop(colNum)
        self.ResizeColumns()

        return True

    def ShowColumn(self, colName, pos):
        """!Show column

        @param colName - name of column
        @param pos - zero based index of position among showed columns (including added 'use' column)

        @return True - if column was shown
        @return False - if position is not valid or column is not hidden
        """
        if pos < 0 and pos >= self.self.GetColumnCount():
            return False
        if self.hiddenCols.has_key(colName):
            col = self.hiddenCols[colName]

            for item in enumerate(self.itemDataMap):
                item[1].insert(pos, col['itemDataMap'][item[0]])
            for item in enumerate(self.selIdxs):
                item[1].insert(pos, col['selIdxs'][item[0]])

            self.colsData.insert(pos, col['colsData'])

            self.InsertColumnItem(pos, col['wxCol'])
            self.ResizeColumns()
            del self.hiddenCols[colName]  
            return True

        return False

    def IsShown(self, colName):
        """!Is column shown

        @param colName - name of column

        @return True - if is shown
        @return False - if is not shown
        """

        if self._getColumnNum(colName) == -1:
            return False
        else:
            return True

class EditItem(wx.Dialog):
    
    def __init__(self, parent, data, pointNo, itemCap = "Point No." ,id=wx.ID_ANY,
                 title =_("Edit point"), style=wx.DEFAULT_DIALOG_STYLE):
        """!Dialog for editing item cells in list"""

        wx.Dialog.__init__(self, parent, id, title=_(title), style=style)

        self.parent = parent
        panel = wx.Panel(parent=self)
        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s %s " % (_(itemCap), str(pointNo + 1)))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        # source coordinates
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
       
        self.fields = [] 
        self.data = deepcopy(data)


        col = 0
        row = 0
        iField = 0
        for cell in self.data:

            # Select
            if type(cell[2]).__name__ == "list":
                self.fields.append(wx.ComboBox(parent = panel, id = wx.ID_ANY,
                                               choices = cell[2],
                                               style = wx.CB_READONLY, 
                                               size = (110, -1)))
            # Text field
            else:
                if cell[2] == float:
                    validator = FloatValidator()
                elif cell[2] == int:
                    validator = IntegerValidator()
                else:
                    validator = None

                if validator:
                    self.fields.append(wx.TextCtrl(parent=panel, id=wx.ID_ANY, 
                                                   validator = validator, size=(150, -1)))
                else:
                    self.fields.append(wx.TextCtrl(parent=panel, id=wx.ID_ANY, 
                                                   size=(150, -1)))
                    self.fields[iField].SetValue(str(cell[1]))

            label = wx.StaticText(parent = panel, id=wx.ID_ANY,
                                  label = _(parent.GetColumn(cell[0]).GetText()) + ":") # name of column)

            gridSizer.Add(item=label,
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(row, col))

            col += 1

            gridSizer.Add(item=self.fields[iField],
                          pos=(row, col))


            if col%3 == 0:
                col = 0
                row += 1
            else:
                col += 1

            iField += 1

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL, border=5)

        sizer.Add(item=boxSizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL, border=5)

        #
        # buttons
        #
        self.btnCancel = wx.Button(panel, wx.ID_CANCEL)
        self.btnOk = wx.Button(panel, wx.ID_OK)
        self.btnOk.SetDefault()

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()

        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

        panel.SetSizer(sizer)
        sizer.Fit(self)

    def GetValues(self):
        """!Return list of values (as strings).
        """

        iField = 0
        for cell in self.data:
            value = self.fields[iField].GetValue()
            
            if type(cell[2]).__name__ == "list":
                    cell[1] = value
            else:
                try:
                    cell[1] = cell[2](value)
                except ValueError:
                    return []
            iField += 1

        return self.data

    def GetSelectionIndexes(self):
        """!Return indexes of selected values (works just for choice columns).
        """
        iField = 0
        itemIndexes = []
        for cell in self.parent.colsData:            
            if type(cell[2]).__name__ == "list":
                itemIndexes.append(self.fields[iField].GetSelection())
            else:
                itemIndexes.append(-1) # not a choise column 
            if cell[2]:   
                iField += 1

        return itemIndexes



