"""!
@package iclass::toolbars

@brief wxIClass dialogs

Classes:
 - dialogs::IClassGroupDialog
 - dialogs::IClassMapDialog
 - dialogs::IClassCategoryManagerDialog
 - dialogs::CategoryListCtrl

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

import  wx.lib.mixins.listctrl as listmix

from core               import globalvar
from gui_core.dialogs   import ElementDialog, GroupDialog
from gui_core           import gselect
from iclass.statistics  import Statistics, BandStatistics

import grass.script as grass

class IClassGroupDialog(ElementDialog):
    """!Dialog for imagery group selection"""
    def __init__(self, parent, group = None, title = _("Select imagery group"), id = wx.ID_ANY):
        """!
        Does post init and layout.
        
        @param gui parent
        @param title dialog window title
        @param id wx id
        """
        ElementDialog.__init__(self, parent, title, label = _("Name of imagery group:"))
        
        self.element = gselect.Select(parent = self.panel, type = 'group',
                                          mapsets = [grass.gisenv()['MAPSET']],
                                          size = globalvar.DIALOG_GSELECT_SIZE)
        if group:
            self.element.SetValue(group)
        self.editGroup = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                   label = _("Create/edit group..."))
        self.editGroup.Bind(wx.EVT_BUTTON, self.OnEditGroup)
        self.PostInit()
        
        self.__Layout()
        self.SetMinSize(self.GetSize())

    def __Layout(self):
        """!Do layout"""
        self.dataSizer.Add(self.element, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        self.dataSizer.Add(self.editGroup, proportion = 0,
                      flag = wx.ALL, border = 5)
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetGroup(self):
        """!Returns selected group"""
        return self.GetElement()
        
    def OnEditGroup(self, event):
        """!Launch edit group dialog"""
        dlg = GroupDialog(parent = self, defaultGroup = self.element.GetValue())

        dlg.ShowModal()
        gr = dlg.GetSelectedGroup()
        if gr in dlg.GetExistGroups():
            self.element.SetValue(gr)
        dlg.Destroy()
        
class IClassMapDialog(ElementDialog):
    """!Dialog for adding raster map"""
    def __init__(self, parent, title = _("Add raster map"), id = wx.ID_ANY):
        """!
        Does post init and layout.
        
        @param gui parent
        @param title dialog window title
        @param id wx id
        """
        ElementDialog.__init__(self, parent, title, label = _("Name of raster map:"))
        
        self.element = gselect.Select(parent = self.panel, type = 'raster',
                                      size = globalvar.DIALOG_GSELECT_SIZE)
        #self.firstMapCheck = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                         #label = _("Add raster map to Training display"))
        #self.secondMapCheck = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                         #label = _("Add raster map to Preview display"))
        # in user settings
        #self.firstMapCheck.SetValue(True)
        #self.secondMapCheck.SetValue(True)
        
        self.PostInit()
        
        self.__Layout()
        self.SetMinSize(self.GetSize())

    def __Layout(self):
        """!Do layout"""
        self.dataSizer.Add(self.element, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        #self.dataSizer.Add(self.firstMapCheck, proportion = 0,
                      #flag = wx.ALL, border = 5)
        #self.dataSizer.Add(self.secondMapCheck, proportion = 0,
                      #flag = wx.ALL, border = 5)
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetRasterMap(self):
        """!Returns selected raster map"""
        return self.GetElement()
        
    #def IfAddToFirstMap(self):
        #return self.firstMapCheck.IsChecked()
        
    #def IfAddToSecondMap(self):
        #return self.secondMapCheck.IsChecked()
        
        
class IClassCategoryManagerDialog(wx.Dialog):
    """!Dialog for managing categories (classes).
    
    Alows adding, deleting class and changing its name and color.
    """
    def __init__(self, parent, title = _("Class manager"), id = wx.ID_ANY):
        """!
        Does post init and layout.
        
        @param gui parent
        @param title dialog window title
        @param id wx id
        """
        wx.Dialog.__init__(self, parent = parent, title = title, id = id)
        
        panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox(panel, id = wx.ID_ANY,
                           label = " %s " % _("Classes"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        gridSizer.AddGrowableCol(0)
        gridSizer.AddGrowableRow(2)
        self.catList = CategoryListCtrl(panel, mapwindow = parent, statistics = parent.statisticsDict,
                                         statisticsList = parent.statisticsList)
        addButton = wx.Button(panel, id = wx.ID_ADD)
        deleteButton = wx.Button(panel, id = wx.ID_DELETE)
        
        gridSizer.Add(item = self.catList, pos = (0, 0), span = (3, 1), flag = wx.EXPAND)
        gridSizer.Add(item = addButton, pos = (0, 1), flag = wx.EXPAND)
        gridSizer.Add(item = deleteButton, pos = (1, 1), flag = wx.EXPAND)
        
        
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = sizer, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        closeButton = wx.Button(panel, id = wx.ID_CLOSE)
        btnSizer.Add(item = wx.Size(-1, -1), proportion = 1, flag = wx.EXPAND)
        btnSizer.Add(item = closeButton, proportion = 0, flag = wx.ALIGN_RIGHT)
        mainSizer.Add(item = btnSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = 5)
        
        addButton.Bind(wx.EVT_BUTTON, self.OnAddCategory)
        deleteButton.Bind(wx.EVT_BUTTON, self.OnDeleteCategory)
        closeButton.Bind(wx.EVT_BUTTON, self.OnClose)
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        
        panel.SetSizer(mainSizer)
        mainSizer.Fit(panel)

        self.SetSize((-1, 250))

        self.Layout()

    def OnAddCategory(self, event):
        self.catList.AddCategory()
        
    def OnDeleteCategory(self, event):
        self.catList.DeleteCategory()
        
    def OnClose(self, event):
        self.catList.UpdateChoice()
        if not isinstance(event, wx.CloseEvent):
            self.Destroy()
            
        event.Skip()
        
class CategoryListCtrl(wx.ListCtrl,
                       listmix.ListCtrlAutoWidthMixin,
                       listmix.TextEditMixin):
    """! Widget for controling list of classes (categories).
    
    CategoryListCtrl updates choice in mapwindow and removes raster map
    when deleting class (category).
    It uses virtual data in the terms of @c wx.ListCtrl.
    
    @todo statistics and categories are managed here directly,
    it could be better to use some interface
    @todo delete vector features after deleting class
    """
    def __init__(self, parent, mapwindow, statistics, statisticsList, id = wx.ID_ANY):
        """!
        @param parent gui parent
        @param mapwindow mapwindow instance with iclass toolbar and remove raster method
        @param statistics dictionary of statistics (defined in statistics.py)
        @param statisticsList list of statistics
        @param id wx id
        """
        wx.ListCtrl.__init__(self, parent, id,
                             style = wx.LC_REPORT|wx.LC_VIRTUAL|wx.LC_HRULES|wx.LC_VRULES)
        self.columns = ((_('Class name'), 'name'),
                        (_('Color'), 'color'))
        self.Populate(columns = self.columns)
        self.mapWindow = mapwindow
        self.statisticsDict = statistics
        self.statisticsList = statisticsList
        self.SetItemCount(len(statisticsList))
        
        listmix.ListCtrlAutoWidthMixin.__init__(self)

        listmix.TextEditMixin.__init__(self)
        
        self.Bind(wx.EVT_LIST_BEGIN_LABEL_EDIT, self.OnEdit)
        
    def SetVirtualData(self, row, column, text):
        attr = self.columns[column][1]
        setattr(self.statisticsDict[self.statisticsList[row]], attr, text)
        
        self.UpdateChoice()
        toolbar = self.mapWindow.toolbars['iClass']
        toolbar.choice.SetSelection(row)
        
        if attr == 'name':
            self.mapWindow.UpdateRasterName(text, toolbar.GetSelectedCategoryIdx())
            
    def Populate(self, columns):
        for i, col in enumerate(columns):
            self.InsertColumn(i, col[0])#wx.LIST_FORMAT_RIGHT

        self.SetColumnWidth(0, 100)
        self.SetColumnWidth(1, 100)
        
    def AddCategory(self):
        st = Statistics()
        if self.statisticsList:
            cat = max(self.statisticsList) + 1
        else:
            cat = 1
        defaultName = 'class' + '_' + str(cat) # intentionally not translatable
        defaultColor = '0:0:0'
        st.SetBaseStatistics(cat = cat, name = defaultName, color = defaultColor)
        self.statisticsDict[cat] = st
        self.statisticsList.append(cat)
        self.SetItemCount(len(self.statisticsList))
        
        self.UpdateChoice()
                
    def DeleteCategory(self):
        indexList = sorted(self.GetSelectedIndices(), reverse = True)
        for i in indexList:
            # remove temporary raster
            name = self.statisticsDict[self.statisticsList[i]].rasterName
            self.mapWindow.RemoveTempRaster(name)
            
            del self.statisticsDict[self.statisticsList[i]]
            del self.statisticsList[i]
        self.SetItemCount(len(self.statisticsList))
        
        self.UpdateChoice()
        
        
        # delete vector items!
    
    def UpdateChoice(self):
        toolbar = self.mapWindow.toolbars['iClass']
        name = toolbar.GetSelectedCategoryName()
        catNames = []
        for cat in self.statisticsList:
            catNames.append(self.statisticsDict[cat].name)
        toolbar.SetCategories(catNames = catNames, catIdx = self.statisticsList)
        if name in catNames:
            toolbar.choice.SetStringSelection(name)
        elif catNames:
            toolbar.choice.SetSelection(0)
            
        if toolbar.choice.IsEmpty():
            toolbar.EnableControls(False)
        else:
            toolbar.EnableControls(True)
        # don't forget to update maps, histo, ...
        
    def GetSelectedIndices(self, state =  wx.LIST_STATE_SELECTED):
        indices = []
        lastFound = -1
        while True:
            index = self.GetNextItem(lastFound, wx.LIST_NEXT_ALL, state)
            if index == -1:
                break
            else:
                lastFound = index
                indices.append(index)
        return indices
        
    def OnEdit(self, event):
        currentItem = event.m_itemIndex
        currentCol = event.m_col
        if currentCol == 1:
            dlg = wx.ColourDialog(self)
            dlg.GetColourData().SetChooseFull(True)

            if dlg.ShowModal() == wx.ID_OK:
                color = dlg.GetColourData().GetColour().Get()
                color = ':'.join(map(str, color))
                self.SetVirtualData(currentItem, currentCol, color)
            dlg.Destroy()
            
        event.Skip()
        
    def OnGetItemText(self, item, col):
        cat = self.statisticsList[item]
        return getattr(self.statisticsDict[cat], self.columns[col][1]) 

    def OnGetItemImage(self, item):
        return -1

    def OnGetItemAttr(self, item):
        return None
