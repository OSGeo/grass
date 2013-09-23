"""!
@package iclass.dialogs

@brief wxIClass dialogs

Classes:
 - dialogs::IClassGroupDialog
 - dialogs::IClassMapDialog
 - dialogs::IClassCategoryManagerDialog
 - dialogs::CategoryListCtrl
 - dialogs::IClassSignatureFileDialog
 - dialogs::IClassExportAreasDialog

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""
import os
import wx

import wx.lib.mixins.listctrl as listmix
import wx.lib.scrolledpanel as scrolled

from core               import globalvar
from core.utils import _
from core.settings      import UserSettings
from core.gcmd          import GError, RunCommand, GMessage
from gui_core.dialogs   import SimpleDialog, GroupDialog
from gui_core           import gselect
from gui_core.widgets   import SimpleValidator
from iclass.statistics  import Statistics, BandStatistics

import grass.script as grass

class IClassGroupDialog(SimpleDialog):
    """!Dialog for imagery group selection"""
    def __init__(self, parent, group = None, subgroup = None, 
                 title = _("Select imagery group"), id = wx.ID_ANY):
        """!
        Does post init and layout.
        
        @param gui parent
        @param title dialog window title
        @param id wx id
        """
        SimpleDialog.__init__(self, parent, title)
        
        self.use_subg = True

        self.groupSelect = gselect.Select(parent = self.panel, type = 'group',
                                      mapsets = [grass.gisenv()['MAPSET']],
                                      size = globalvar.DIALOG_GSELECT_SIZE,
                                      validator = SimpleValidator(callback = self.ValidatorCallback))

        self.subg_panel = wx.Panel(self.panel)
        # TODO use when subgroup will be optional
        #self.subg_chbox = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
        #                              label = _("Use subgroup"))
        self.subGroupSelect = gselect.SubGroupSelect(parent = self.subg_panel)

        self.groupSelect.SetFocus()
        if group:
            self.groupSelect.SetValue(group)
            self.GroupSelected()
        if subgroup:
            self.subGroupSelect.SetValue(subgroup)

        self.editGroup = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                   label = _("Create/edit group..."))

        self.editGroup.Bind(wx.EVT_BUTTON, self.OnEditGroup)
        self.groupSelect.GetTextCtrl().Bind(wx.EVT_TEXT, 
                                           lambda event : wx.CallAfter(self.GroupSelected))

        self.warning = _("Name of imagery group is missing.")
        self._layout()
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """!Do layout"""
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        self.dataSizer.Add(wx.StaticText(self.panel, id = wx.ID_ANY,
                                         label = _("Name of imagery group:")),
                                         proportion = 0, 
                                         flag = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT,
                                         border = 5)
        self.dataSizer.Add(self.groupSelect, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        
        # TODO use when subgroup will be optional
        #self.dataSizer.Add(self.subg_chbox, proportion = 0,
        #              flag = wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border = 5)

        subg_sizer = wx.BoxSizer(wx.VERTICAL)
        subg_sizer.Add(wx.StaticText(self.subg_panel, id = wx.ID_ANY,
                                    label = _("Name of imagery subgroup:")),
                                    proportion = 0, flag = wx.EXPAND | wx.BOTTOM, 
                                    border = 5)
        subg_sizer.Add(self.subGroupSelect, proportion = 0,
                       flag = wx.EXPAND)

        self.subg_panel.SetSizer(subg_sizer)
        self.dataSizer.Add(self.subg_panel, proportion = 0,
                           flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 5)

        self.dataSizer.Add(self.editGroup, proportion = 0,
                      flag = wx.ALL, border = 5)

        self.panel.SetSizer(self.sizer)

        mainSizer.Add(self.panel, proportion = 1,
                     flag = wx.ALL, border = 5)

        self.SetSizer(mainSizer)
        self.sizer.Fit(self)

        #TODO use when subgroup will be optional
        #self.subg_panel.Show(False)
        #self.subg_chbox.Bind(wx.EVT_CHECKBOX, self.OnSubgChbox)

    def OnSubgChbox(self, event):
        self.use_subg = self.subg_chbox.GetValue()

        if self.use_subg:
            self.subg_panel.Show()
            #self.SubGroupSelected()
        else:
            self.subg_panel.Hide()
            #self.GroupSelected()
        self.SetMinSize(self.GetBestSize())
        self.Layout()

    def GetData(self):
        """!Returns selected group and subgroup"""

        if self.use_subg:
            ret = (self.groupSelect.GetValue(), self.subGroupSelect.GetValue())
        else:
            ret = (self.groupSelect.GetValue(), None)

        return ret
        
    def OnEditGroup(self, event):
        """!Launch edit group dialog"""
        g, s = self.GetData()
        dlg = GroupDialog(parent=self, defaultGroup=g, defaultSubgroup=s)

        dlg.ShowModal()
        gr, s = dlg.GetSelectedGroup()
        if gr in dlg.GetExistGroups():
            self.groupSelect.SetValue(gr)
            self.GroupSelected()
            wx.CallAfter(self.subGroupSelect.SetValue, s)
        dlg.Destroy()

    def GroupSelected(self):
        group = self.GetSelectedGroup()
        self.subGroupSelect.Insert(group)

    def GetSelectedGroup(self):
        """!Return currently selected group (without mapset)"""
        return self.groupSelect.GetValue().split('@')[0]

    def GetGroupBandsErr(self, parent):
        """!Get list of raster bands which are in the soubgroup of group with both having same name.
           If the group does not exists or it does not contain any bands in subgoup with same name, 
           error dialog is shown.
        """
        gr, s = self.GetData()

        group = grass.find_file(name=gr, element='group')
    
        bands = []
        g = group['name']
    
        if g:
            if self.use_subg:
                if s == '':
                    GError(_("Please choose a subgroup."), parent=parent)
                    return bands
                if s not in self.GetSubgroups(g):
                    GError(_("Subgroup <%s> not found in group <%s>") % (s, g), parent=parent)
                    return bands

            bands = self.GetGroupBands(g, s)
            if not bands:
                if self.use_subg:
                    GError(_("No data found in subgroup <%s> of group <%s>.\n" \
                             ".") \
                               % (s, g), parent=parent)
        
                else:
                    GError(_("No data found in group <%s>.\n" \
                             ".") \
                               % g, parent=parent)
        else:
            GError(_("Group <%s> not found") % gr, parent=parent)

        return bands

    def GetGroupBands(self, group, subgroup):
        """!Get list of raster bands which are in the soubgroup of group with both having same name."""

        kwargs = {}
        if subgroup:
            kwargs['subgroup'] = subgroup

        res = RunCommand('i.group',
                         flags='g',
                         group=group,
                         read = True, **kwargs).strip()
        bands = None
        if res.split('\n')[0]:
            bands = res.split('\n')
            
        return bands

    def GetSubgroups(self, group):
        return RunCommand('i.group', group=group,
                           read=True, flags='sg').splitlines()

class IClassMapDialog(SimpleDialog):
    """!Dialog for adding raster/vector map"""
    def __init__(self, parent, title, element):
        """!
        
        @param parent gui parent
        @param title dialog title
        @param element element type ('raster', 'vector')
        """
        
        SimpleDialog.__init__(self, parent, title = title)
        
        self.elementType = element
        self.element = gselect.Select(parent = self.panel, type = element,
                                      size = globalvar.DIALOG_GSELECT_SIZE,
                                      validator = SimpleValidator(callback = self.ValidatorCallback))
        self.element.SetFocus()

        self.warning = _("Name of map is missing.")
        self._layout()
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """!Do layout"""
        if self.elementType == 'raster':
            label = _("Name of raster map:")
        elif self.elementType == 'vector':
            label = _("Name of vector map:")
        self.dataSizer.Add(wx.StaticText(self.panel, id = wx.ID_ANY,
                                         label = label),
                           proportion = 0, flag = wx.EXPAND | wx.ALL, border = 5)
        self.dataSizer.Add(self.element, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetMap(self):
        """!Returns selected raster/vector map"""
        return self.element.GetValue()


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
        
        self.parent = parent
        panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox(panel, id = wx.ID_ANY,
                           label = " %s " % _("Classes"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        self.catList = CategoryListCtrl(panel, mapwindow = parent, 
                                               stats_data = parent.stats_data)
        addButton = wx.Button(panel, id = wx.ID_ADD)
        deleteButton = wx.Button(panel, id = wx.ID_DELETE)
        
        gridSizer.Add(item = self.catList, pos = (0, 0), span = (3, 1), flag = wx.EXPAND)
        gridSizer.Add(item = addButton, pos = (0, 1), flag = wx.EXPAND)
        gridSizer.Add(item = deleteButton, pos = (1, 1), flag = wx.EXPAND)
                
        gridSizer.AddGrowableCol(0)
        gridSizer.AddGrowableRow(2)
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
        mainSizer.Fit(self)

        self.SetSize((400, 250))

        self.Layout()

    def OnAddCategory(self, event):
        if self.parent.stats_data.GetCategories():
            cat = max(self.parent.stats_data.GetCategories()) + 1
        else:
            cat = 1
        defaultName = 'class' + '_' + str(cat) # intentionally not translatable
        defaultColor = '0:0:0'
        self.catList.AddCategory(cat = cat, name = defaultName, color = defaultColor)
        
    def OnDeleteCategory(self, event):
        self.catList.DeleteCategory()
        
    def OnClose(self, event):
        self.catList.DeselectAll()
        
        self.Hide()
        #if not isinstance(event, wx.CloseEvent):
            #self.Destroy()
            
        #event.Skip()
        
    def GetListCtrl(self):
        """!Returns list widget"""
        return self.catList
        
class CategoryListCtrl(wx.ListCtrl,
                       listmix.ListCtrlAutoWidthMixin,
                       listmix.TextEditMixin):
    """! Widget for controling list of classes (categories).
    
    CategoryListCtrl updates choice in mapwindow and removes raster map
    when deleting class (category).
    It uses virtual data in the terms of @c wx.ListCtrl.
    
    @todo delete vector features after deleting class
    """
    def __init__(self, parent, mapwindow, stats_data, id = wx.ID_ANY):
        """!
        @param parent gui parent
        @param mapwindow mapwindow instance with iclass toolbar and remove raster method
        @param stats_data StatisticsData instance (defined in statistics.py)
        @param id wx id
        """
        wx.ListCtrl.__init__(self, parent, id,
                             style = wx.LC_REPORT|wx.LC_VIRTUAL|wx.LC_HRULES|wx.LC_VRULES)
        self.columns = ((_('Class name'), 'name'),
                        (_('Color'), 'color'))
        self.Populate(columns = self.columns)
        self.mapWindow = mapwindow
        self.stats_data = stats_data
        self.SetItemCount(len(self.stats_data.GetCategories()))
        
        self.rightClickedItemIdx = wx.NOT_FOUND
        
        listmix.ListCtrlAutoWidthMixin.__init__(self)

        listmix.TextEditMixin.__init__(self)
        
        self.Bind(wx.EVT_LIST_BEGIN_LABEL_EDIT, self.OnEdit)
        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnCategorySelected)
        
        self.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnClassRightUp) #wxMSW
        self.Bind(wx.EVT_RIGHT_UP,            self.OnClassRightUp) #wxGTK
    
        self.stats_data.statisticsAdded.connect(self.Update)
        self.stats_data.statisticsDeleted.connect(self.Update)
        self.stats_data.allStatisticsDeleted.connect(self.Update)
        self.stats_data.statisticsSet.connect(self.Update)

    def Update(self):
        self.SetItemCount(len(self.stats_data.GetCategories()))

    def SetVirtualData(self, row, column, text):
        attr = self.columns[column][1]
        if attr == 'name':
            try:
                text.encode('ascii')
            except UnicodeEncodeError:
                GMessage(parent = self, message = _("Please use only ASCII characters."))
                return

        cat = self.stats_data.GetCategories()[row]
        self.stats_data.GetStatistics(cat).SetStatistics(stats = {attr : text})
        
        toolbar = self.mapWindow.toolbars['iClass']
        toolbar.choice.SetSelection(row)
        self.Select(row)
        
        if attr == 'name':
            self.mapWindow.UpdateRasterName(text, toolbar.GetSelectedCategoryIdx())
            
        self.mapWindow.UpdateChangeState(changes = True)
        
    def Populate(self, columns):
        for i, col in enumerate(columns):
            self.InsertColumn(i, col[0])#wx.LIST_FORMAT_RIGHT

        self.SetColumnWidth(0, 100)
        self.SetColumnWidth(1, 100)
        
    def AddCategory(self, cat, name, color):
        """!Add category record (used when importing areas)"""

        self.stats_data.AddStatistics(cat, name, color)
        self.SetItemCount(len(self.stats_data.GetCategories()))
        
        self.mapWindow.UpdateChangeState(changes = True)
                
    def DeleteCategory(self):
        indexList = sorted(self.GetSelectedIndices(), reverse = True)
        del_cats = []
        cats = self.stats_data.GetCategories()

        for i in indexList:
            # remove temporary raster
            cat = cats[i]
            stat = self.stats_data.GetStatistics(cat)

            name = stat.rasterName
            self.mapWindow.RemoveTempRaster(name)
            
            del_cats.append(cat)
            self.stats_data.DeleteStatistics(cat)
            
        self.SetItemCount(len(self.stats_data.GetCategories()))
        
        self.mapWindow.UpdateChangeState(changes = True)
        
        self.mapWindow.DeleteAreas(cats = del_cats)
            
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
            col = self.OnGetItemText(currentItem, currentCol)
            col = map(int, col.split(':'))

            col_data =  wx.ColourData()
            col_data.SetColour(wx.Colour(*col))

            dlg = wx.ColourDialog(self, col_data)
            dlg.GetColourData().SetChooseFull(True)

            if dlg.ShowModal() == wx.ID_OK:
                color = dlg.GetColourData().GetColour().Get()
                color = ':'.join(map(str, color))
                self.SetVirtualData(currentItem, currentCol, color)
            dlg.Destroy()
            wx.CallAfter(self.SetFocus)
        
        event.Skip()
        
    def OnCategorySelected(self, event):
        """!Highlight selected areas"""
        indexList = self.GetSelectedIndices()
        sel_cats = []
        cats = self.stats_data.GetCategories()
        for i in indexList:
            sel_cats.append(cats[i])
        
        self.mapWindow.HighlightCategory(sel_cats)
        if event:
            event.Skip()
        
    def OnClassRightUp(self, event):
        """!Show context menu on right click"""
        item, flags = self.HitTest((event.GetX(), event.GetY()))
        if item != wx.NOT_FOUND and flags & wx.LIST_HITTEST_ONITEM:
            self.rightClickedItemIdx = item
           
        if not hasattr(self, "popupZoomtoAreas"):
            self.popupZoomtoAreas = wx.NewId()
            self.Bind(wx.EVT_MENU, self.OnZoomToAreasByCat, id = self.popupZoomtoAreas)

        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupZoomtoAreas, _("Zoom to training areas of selected class"))
        
        self.PopupMenu(menu)
        menu.Destroy()
    
    def OnZoomToAreasByCat(self, event):
        """!Zoom to areas of given category"""
        cat = self.stats_data.GetCategories()[self.rightClickedItemIdx]
        self.mapWindow.ZoomToAreasByCat(cat)
        
    def DeselectAll(self):
        """!Deselect all items"""
        indexList = self.GetSelectedIndices()
        for i in indexList:
            self.Select(i, on = 0)
         
        # no highlight
        self.OnCategorySelected(None)
        
    def OnGetItemText(self, item, col):
        cat = self.stats_data.GetCategories()[item]
        stat = self.stats_data.GetStatistics(cat)
        return getattr(stat, self.columns[col][1]) 

    def OnGetItemImage(self, item):
        return -1

    def OnGetItemAttr(self, item):
        return None

    def OnGetItemAttr(self, item):
        """!Set correct class color for a item"""
        back_c = wx.Colour(*map(int, self.OnGetItemText(item, 1).split(':')))
        text_c = wx.Colour(*ContrastColor(back_c))

        # if it is in scope of the method, gui falls, using self solved it 
        self.l = wx.ListItemAttr(colText = text_c, colBack =  back_c)
        return self.l
    
def ContrastColor(color):
    """!Decides which value shoud have text to be contrast with backgroud color 
        (bright bg -> black, dark bg -> white)

    @todo could be useful by other apps, consider moving it into gui_core 
    """
    #gacek, http://stackoverflow.com/questions/1855884/determine-font-color-based-on-background-color
    a = 1 - ( 0.299 * color[0] + 0.587 * color[1] + 0.114 * color[2])/255;

    if a < 0.5:
        d = 0
    else:
        d = 255 
    # maybe return just bool if text shoud be dark or bright 
    return (d, d, d)

class IClassSignatureFileDialog(wx.Dialog):
    def __init__(self, parent, group, subgroup, 
                 file = None, title = _("Save signature file"), id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!Dialog for saving signature file
        
        @param parent window
        @param group group name
        @param file signature file name
        @param title window title
        """
        wx.Dialog.__init__(self, parent, id, title, style = style, **kwargs)
        
        self.fileName = file
        
        env = grass.gisenv()
        
        # inconsistent group and subgroup name
        # path: grassdata/nc_spm_08/landsat/group/test_group/subgroup/test_group/sig/sigFile
        self.baseFilePath = os.path.join(env['GISDBASE'],
                                         env['LOCATION_NAME'],
                                         env['MAPSET'],
                                         'group', group,
                                         'subgroup', subgroup,
                                         'sig')
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btnOK     = wx.Button(parent = self.panel, id = wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)
        
        self.__layout()
        
        self.fileNameCtrl.Bind(wx.EVT_TEXT, self.OnTextChanged)
        self.OnTextChanged(None)
        
    def OnTextChanged(self, event):
        """!Name for signature file given"""
        file = self.fileNameCtrl.GetValue()
        if len(file) > 0:
            self.btnOK.Enable(True)
        else:
            self.btnOK.Enable(False)
            
        path = os.path.join(self.baseFilePath, file)
        self.filePathText.SetLabel(path)
        bestSize = self.pathPanel.GetBestVirtualSize() 
        self.pathPanel.SetVirtualSize(bestSize)
        self.pathPanel.Scroll(*bestSize)
        
    def __layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        dataSizer = wx.BoxSizer(wx.VERTICAL)
        
        dataSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                           label = _("Enter name of signature file:")),
                      proportion = 0, flag = wx.ALL, border = 3)
        self.fileNameCtrl = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY, size = (400, -1))
        if self.fileName:
            self.fileNameCtrl.SetValue(self.fileName)
        dataSizer.Add(item = self.fileNameCtrl,
                      proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)
    
        dataSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                                label = _("Signature file path:")),
                           proportion = 0, flag = wx.ALL, border = 3)
        
        self.pathPanel = scrolled.ScrolledPanel(self.panel, size = (-1, 40))
        pathSizer = wx.BoxSizer()
        self.filePathText = wx.StaticText(parent = self.pathPanel, id = wx.ID_ANY,
                                                label = self.baseFilePath)
        pathSizer.Add(self.filePathText, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        self.pathPanel.SetupScrolling(scroll_x = True, scroll_y = False)
        self.pathPanel.SetSizer(pathSizer)

        dataSizer.Add(item = self.pathPanel,
                      proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)
                      
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()
        
        sizer.Add(item = dataSizer, proportion = 1,
                       flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        sizer.Add(item = btnSizer, proportion = 0,
                       flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self)
        
        self.SetMinSize(self.GetSize())
        
    def GetFileName(self, fullPath = False):
        """!Returns signature file name
        
        @param fullPath return full path of sig. file
        """
        if fullPath:
            return os.path.join(self.baseFilePath, self.fileNameCtrl.GetValue())
            
        return self.fileNameCtrl.GetValue()
        
class IClassExportAreasDialog(wx.Dialog):
    def __init__(self, parent, vectorName = None, title = _("Export training areas"), id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!Dialog for export of training areas to vector layer
        
        @param parent window
        @param vectorName name of vector layer for export
        @param title window title
        """
        wx.Dialog.__init__(self, parent, id, title, style = style, **kwargs)
        
        self.vectorName = vectorName
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btnOK     = wx.Button(parent = self.panel, id = wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)
        self.btnOK.Bind(wx.EVT_BUTTON, self.OnOK)
        
        self.__layout()
        
        self.vectorNameCtrl.Bind(wx.EVT_TEXT, self.OnTextChanged)
        self.OnTextChanged(None)
        wx.CallAfter(self.vectorNameCtrl.SetFocus)

    def OnTextChanged(self, event):
        """!Name of new vector map given.
        
        Enable/diable OK button.
        """
        file = self.vectorNameCtrl.GetValue()
        if len(file) > 0:
            self.btnOK.Enable(True)
        else:
            self.btnOK.Enable(False)
        
    def __layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        dataSizer = wx.BoxSizer(wx.VERTICAL)
        
        dataSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                           label = _("Enter name of new vector map:")),
                      proportion = 0, flag = wx.ALL, border = 3)
        self.vectorNameCtrl = gselect.Select(parent = self.panel, type = 'vector',
                                             mapsets = [grass.gisenv()['MAPSET']],
                                             size = globalvar.DIALOG_GSELECT_SIZE)
        if self.vectorName:
            self.vectorNameCtrl.SetValue(self.vectorName)
        dataSizer.Add(item = self.vectorNameCtrl,
                      proportion = 0, flag = wx.ALL | wx.EXPAND, border = 3)
        self.withTableCtrl = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                       label = _("Export attribute table"))
        self.withTableCtrl.SetValue(True)
        self.withTableCtrl.SetToolTipString(_("Export attribute table containing" 
                                              " computed statistical data"))
        
        dataSizer.Add(item = self.withTableCtrl,
                      proportion = 0, flag = wx.ALL, border = 3)
                      
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()
        
        sizer.Add(item = dataSizer, proportion = 1,
                       flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        sizer.Add(item = btnSizer, proportion = 0,
                       flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self)
        
        self.SetMinSize(self.GetSize())
        
    def GetVectorName(self):
        """!Returns vector name"""
        return self.vectorNameCtrl.GetValue()
        
    def WithTable(self):
        """!Returns true if attribute table should be exported too"""
        return self.withTableCtrl.IsChecked()
        
    def OnOK(self, event):
        """!Checks if map exists and can be overwritten."""
        overwrite = UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled')
        vName = self.GetVectorName()
        res = grass.find_file(vName, element = 'vector')
        if res['fullname'] and overwrite is False:
            qdlg = wx.MessageDialog(parent = self,
                                        message = _("Vector map <%s> already exists."
                                                    " Do you want to overwrite it?" % vName) ,
                                        caption = _("Vector <%s> exists" % vName),
                                        style = wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)
            if qdlg.ShowModal() == wx.ID_YES:
                event.Skip()
            qdlg.Destroy()
        else:
            event.Skip()
            
