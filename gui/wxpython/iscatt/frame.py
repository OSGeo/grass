"""!
@package iscatt.frame

@brief Main scatter plot widgets.

Classes:
 - frame::IClassIScattPanel
 - frame::IScattDialog
 - frame::MapDispIScattPanel
 - frame::ScatterPlotsPanel
 - frame::CategoryListCtrl

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""
import os
import sys

import wx
import wx.lib.scrolledpanel as scrolled
import wx.lib.mixins.listctrl as listmix

from core import globalvar
from core.gcmd import GException, GError, RunCommand

from gui_core.gselect import Select
from gui_core.dialogs import SetOpacityDialog
from iscatt.controllers import ScattsManager
from iscatt.toolbars import MainToolbar, EditingToolbar, CategoryToolbar
from iscatt.iscatt_core import idScattToidBands
from iscatt.dialogs import ManageBusyCursorMixin, RenameClassDialog
from iscatt.plots import ScatterPlotWidget
from iclass.dialogs import ContrastColor

try:
    from agw import aui
except ImportError:
    import wx.lib.agw.aui as aui

class IClassIScattPanel(wx.Panel, ManageBusyCursorMixin):
    def __init__(self, parent, giface, iclass_mapwin = None,
                 id = wx.ID_ANY):

        #wx.SplitterWindow.__init__(self, parent = parent, id = id,
        #                           style = wx.SP_LIVE_UPDATE)
        wx.Panel.__init__(self, parent = parent, id = wx.ID_ANY)
        ManageBusyCursorMixin.__init__(self, window=self)

        self.scatt_mgr = self._createScattMgr(guiparent=parent, giface=giface, 
                                              iclass_mapwin=iclass_mapwin)

        # toobars
        self.toolbars = {}
        self.toolbars['mainToolbar'] = self._createMainToolbar()
        self.toolbars['editingToolbar'] = EditingToolbar(parent = self, scatt_mgr = self.scatt_mgr)
        
        self._createCategoryPanel(self)

        self.plot_panel = ScatterPlotsPanel(self, self.scatt_mgr)

        self.mainsizer = wx.BoxSizer(wx.VERTICAL)
        self.mainsizer.Add(item = self.toolbars['mainToolbar'], proportion = 0, flag = wx.EXPAND)
        self.mainsizer.Add(item = self.toolbars['editingToolbar'], proportion = 0, flag = wx.EXPAND)
        self.mainsizer.Add(item = self.catsPanel, proportion = 0, 
                           flag = wx.EXPAND | wx.LEFT | wx.RIGHT , border = 5)
        self.mainsizer.Add(item = self.plot_panel, proportion = 1, flag = wx.EXPAND)

        self.catsPanel.Hide()
        self.toolbars['editingToolbar'].Hide()

        self.SetSizer(self.mainsizer)
        
        self.scatt_mgr.computingStarted.connect(lambda : self.UpdateCur(busy=True))
        self.scatt_mgr.renderingStarted.connect(lambda : self.UpdateCur(busy=True))
        self.scatt_mgr.renderingFinished.connect(lambda : self.UpdateCur(busy=False))

        #self.SetSashGravity(0.5)
        #self.SplitHorizontally(self.head_panel, self.plot_panel, -50)
        self.Layout()

    def CloseWindow(self):
        self.scatt_mgr.CleanUp()

    def UpdateCur(self, busy):
        self.plot_panel.SetBusy(busy)
        ManageBusyCursorMixin.UpdateCur(self, busy)

    def _selCatInIScatt(self):
         return False

    def _createMainToolbar(self):
         return MainToolbar(parent = self, scatt_mgr = self.scatt_mgr)

    def _createScattMgr(self, guiparent, giface, iclass_mapwin):
        return ScattsManager(guiparent=self, giface=giface, iclass_mapwin=iclass_mapwin)


    def NewScatterPlot(self, scatt_id, transpose):
        return self.plot_panel.NewScatterPlot(scatt_id, transpose)

    def ShowPlotEditingToolbar(self, show):
        self.toolbars["editingToolbar"].Show(show)
        self.Layout()

    def ShowCategoryPanel(self, show):
        self.catsPanel.Show(show)
        
        #if show:
        #    self.SetSashSize(5) 
        #else:
        #    self.SetSashSize(0)
        self.plot_panel.SetVirtualSize(self.plot_panel.GetBestVirtualSize())
        self.Layout()

    def _createCategoryPanel(self, parent):
        self.catsPanel = wx.Panel(parent=parent)
        self.cats_list = CategoryListCtrl(parent=self.catsPanel, 
                                          cats_mgr=self.scatt_mgr.GetCategoriesManager(),
                                          sel_cats_in_iscatt=self._selCatInIScatt())

        self.catsPanel.SetMinSize((-1, 100))
        self.catsPanel.SetInitialSize((-1, 150))

        box_capt = wx.StaticBox(parent=self.catsPanel, id=wx.ID_ANY,
                             label=' %s ' % _("Classes"),)
        catsSizer = wx.StaticBoxSizer(box_capt, wx.VERTICAL)

        self.toolbars['categoryToolbar'] = self._createCategoryToolbar(self.catsPanel)

        catsSizer.Add(item=self.cats_list, proportion=1,  flag=wx.EXPAND | wx.TOP, border = 5)
        if self.toolbars['categoryToolbar']:
            catsSizer.Add(item=self.toolbars['categoryToolbar'], proportion=0)

        self.catsPanel.SetSizer(catsSizer)
    
    def _createCategoryToolbar(self, parent):
        return CategoryToolbar(parent=parent,
                               scatt_mgr=self.scatt_mgr, 
                               cats_list=self.cats_list)

class IScattDialog(wx.Dialog):
    def __init__(self, parent, giface, title=_("GRASS GIS Interactive Scatter Plot Tool"),
                 id=wx.ID_ANY, style=wx.DEFAULT_FRAME_STYLE, **kwargs):
        wx.Dialog.__init__(self, parent, id, style=style, title = title, **kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))

        self.iscatt_panel = MapDispIScattPanel(self, giface)

        mainsizer = wx.BoxSizer(wx.VERTICAL)
        mainsizer.Add(item=self.iscatt_panel, proportion=1, flag=wx.EXPAND)

        self.SetSizer(mainsizer)

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        self.SetMinSize((300, 300))

    def OnCloseWindow(self, event):
        event.Skip()
        #self.

class MapDispIScattPanel(IClassIScattPanel):
    def __init__(self, parent, giface,
                 id=wx.ID_ANY, **kwargs):
        IClassIScattPanel.__init__(self, parent=parent, giface=giface,
                                         id=id, **kwargs)

    def _createScattMgr(self, guiparent, giface, iclass_mapwin):
        return ScattsManager(guiparent = self, giface = giface)

    def _createMainToolbar(self):
         return MainToolbar(parent = self, scatt_mgr = self.scatt_mgr, opt_tools=['add_group'])

    def _selCatInIScatt(self):
         return True

class ScatterPlotsPanel(scrolled.ScrolledPanel):
    def __init__(self, parent, scatt_mgr, id=wx.ID_ANY):
    
        scrolled.ScrolledPanel.__init__(self, parent)
        self.SetupScrolling(scroll_x=False, scroll_y=True, scrollToTop=False)

        self.scatt_mgr = scatt_mgr

        self.mainPanel = wx.Panel(parent=self, id=wx.ID_ANY)

        #self._createCategoryPanel()
        # Fancy gui
        self._mgr = aui.AuiManager(self.mainPanel)
        #self._mgr.SetManagedWindow(self)

        self._mgr.Update()

        self._doLayout()
        self.Bind(wx.EVT_SCROLLWIN, self.OnScroll)
        self.Bind(wx.EVT_SCROLL_CHANGED, self.OnScrollChanged)

        self.Bind(aui.EVT_AUI_PANE_CLOSE, self.OnPlotPaneClosed)

        dlgSize = (-1, 400)
        #self.SetBestSize(dlgSize)
        #self.SetInitialSize(dlgSize)
        self.SetAutoLayout(1)
        #fix goutput's pane size (required for Mac OSX)
        #if self.gwindow:         
        #    self.gwindow.SetSashPosition(int(self.GetSize()[1] * .75))
        self.ignore_scroll = 0
        self.Bind(wx.EVT_MOUSEWHEEL, self.OnMouseWheel)

        self.scatt_i = 1
        self.scatt_id_scatt_i = {}
        self.transpose = {}
        self.scatts = {}

        self.Bind(wx.EVT_CLOSE, self.OnClose)
        
        self.scatt_mgr.cursorPlotMove.connect(self.CursorPlotMove)

    def SetBusy(self, busy):
        for scatt in self.scatts.itervalues():
            scatt.UpdateCur(busy)

    def CursorPlotMove(self, x, y, scatt_id):

        try:
            x = int(round(x))
            y = int(round(y))
            coords = True
        except:
            coords = False

        pane = self._getPane(scatt_id)
        caption = self._creteCaption(scatt_id)
        if coords:
            caption += "   %d, %d" % (x, y)

        pane.Caption(caption)
        self._mgr.RefreshCaptions()

    def _getPane(self, scatt_id):
        scatt_i = self.scatt_id_scatt_i[scatt_id]
        name = self._getScatterPlotName(scatt_i)
        return self._mgr.GetPane(name)

    def ScatterPlotClosed(self, scatt_id):

        scatt_i = self.scatt_id_scatt_i[scatt_id]

        name = self._getScatterPlotName(scatt_i)
        pane = self._mgr.GetPane(name)

        del self.scatt_id_scatt_i[scatt_id]
        del self.scatts[scatt_id]

        if pane.IsOk(): 
          self._mgr.ClosePane(pane) 
        self._mgr.Update() 

    def OnMouseWheel(self, event):
        #TODO very ugly find some better solution        
        self.ignore_scroll = 3
        event.Skip()

    def ScrollChildIntoView(self, child):
        #For aui manager it does not work and returns position always to the top -> deactivated.
        pass

    def OnPlotPaneClosed(self, event):
        if isinstance(event.pane.window, ScatterPlotWidget):
            event.pane.window.CleanUp()

    def OnScrollChanged(self, event):
        wx.CallAfter(self.Layout)

    def OnScroll(self, event):
        if self.ignore_scroll > 0:
            self.ignore_scroll -= 1
        else:
            event.Skip()

        #wx.CallAfter(self._mgr.Update)
        #wx.CallAfter(self.Layout)

    def _doLayout(self):

        mainsizer = wx.BoxSizer(wx.VERTICAL)
        mainsizer.Add(item = self.mainPanel, proportion = 1, flag = wx.EXPAND)
        self.SetSizer(mainsizer)

        self.Layout()
        self.SetupScrolling()

    def OnClose(self, event):
        """!Close dialog"""
        #TODO
        print "closed"
        self.scatt_mgr.CleanUp()
        self.Destroy()

    def OnSettings(self, event):
        pass

    def _newScatterPlotName(self, scatt_id):
        name = self._getScatterPlotName(self.scatt_i) 
        self.scatt_id_scatt_i[scatt_id] = self.scatt_i
        self.scatt_i += 1
        return name

    def _getScatterPlotName(self, i):
        name = "scatter plot %d" % i
        return name

    def NewScatterPlot(self, scatt_id, transpose):
        #TODO needs to be resolved (should be in this class)

        scatt = ScatterPlotWidget(parent = self.mainPanel, 
                                  scatt_mgr = self.scatt_mgr, 
                                  scatt_id = scatt_id, 
                                  transpose = transpose)
        scatt.plotClosed.connect(self.ScatterPlotClosed)
        self.transpose[scatt_id] = transpose
        
        caption = self._creteCaption(scatt_id)
        self._mgr.AddPane(scatt,
                           aui.AuiPaneInfo().Dockable(True).Floatable(True).
                           Name(self._newScatterPlotName(scatt_id)).MinSize((-1, 300)).
                           Caption(caption).
                           Center().Position(1).MaximizeButton(True).
                           MinimizeButton(True).CaptionVisible(True).
                           CloseButton(True).Layer(0))

        self._mgr.Update()
  
        self.SetVirtualSize(self.GetBestVirtualSize())
        self.Layout()

        self.scatts[scatt_id] = scatt

        return scatt

    def _creteCaption(self, scatt_id):

        transpose = self.transpose[scatt_id]
        bands = self.scatt_mgr.GetBands()

        #TODO too low level
        b1_id, b2_id = idScattToidBands(scatt_id, len(bands))

        x_b =  bands[b1_id].split('@')[0]
        y_b = bands[b2_id].split('@')[0]

        if transpose:
            tmp = x_b
            x_b = y_b
            y_b = tmp

        return "%s x: %s y: %s" % (_("scatter plot"), x_b, y_b)

    def GetScattMgr(self):
        return  self.scatt_mgr

class CategoryListCtrl(wx.ListCtrl,
                       listmix.ListCtrlAutoWidthMixin):
                       #listmix.TextEditMixin):

    def __init__(self, parent, cats_mgr, sel_cats_in_iscatt, id = wx.ID_ANY):

        wx.ListCtrl.__init__(self, parent, id,
                             style = wx.LC_REPORT|wx.LC_VIRTUAL|wx.LC_HRULES|
                                     wx.LC_VRULES|wx.LC_SINGLE_SEL|wx.LC_NO_HEADER)
        self.columns = ((_('Class name'), 'name'), )
                        #(_('Color'), 'color'))

        self.sel_cats_in_iscatt = sel_cats_in_iscatt

        self.Populate(columns = self.columns)
        
        self.cats_mgr = cats_mgr
        self.SetItemCount(len(self.cats_mgr.GetCategories()))

        self.rightClickedItemIdx = wx.NOT_FOUND
        
        listmix.ListCtrlAutoWidthMixin.__init__(self)

        #listmix.TextEditMixin.__init__(self)
      
        self.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnCategoryRightUp) #wxMSW
        self.Bind(wx.EVT_RIGHT_UP,            self.OnCategoryRightUp) #wxGTK

        #self.Bind(wx.EVT_LIST_BEGIN_LABEL_EDIT, self.OnEdit)
        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnSel)
             
        self.cats_mgr.setCategoryAttrs.connect(self.Update)
        self.cats_mgr.deletedCategory.connect(self.Update)
        self.cats_mgr.addedCategory.connect(self.Update)

    def Update(self, **kwargs):
        self.SetItemCount(len(self.cats_mgr.GetCategories()))
        self.RefreshItems(0, len(self.cats_mgr.GetCategories()))

    def InitCoreCats(self):
        self.SetItemCount(len(self.cats_mgr.GetCategories()))
        self.RefreshItems(0, len(self.cats_mgr.GetCategories()))

    def SetVirtualData(self, row, column, text):
        attr = self.columns[column][1]
        if attr == 'name':
            try:
                text.encode('ascii')
            except UnicodeEncodeError:
                GMessage(parent = self, message = _("Please use only ASCII characters."))
                return

        cat_id = self.cats_mgr.GetCategories()[row]

        self.cats_mgr.setCategoryAttrs.disconnect(self.Update)
        self.cats_mgr.SetCategoryAttrs(cat_id, {attr : text})
        self.cats_mgr.setCategoryAttrs.connect(self.Update)
        
        self.Select(row)
        
    def Populate(self, columns):
        for i, col in enumerate(columns):
            self.InsertColumn(i, col[0])#wx.LIST_FORMAT_RIGHT

        #self.SetColumnWidth(0, 100)
        #self.SetColumnWidth(1, 100)
        
    def AddCategory(self):

        self.cats_mgr.addedCategory.disconnect(self.Update)
        cat_id = self.cats_mgr.AddCategory()
        self.cats_mgr.addedCategory.connect(self.Update)

        if cat_id < 0:
            GError(_("Maximum limit of categories number was reached."))
            return
        self.SetItemCount(len(self.cats_mgr.GetCategories()))
                        
    def DeleteCategory(self):
        indexList = sorted(self.GetSelectedIndices(), reverse = True)
        cats = []
        for i in indexList:
            # remove temporary raster
            cat_id = self.cats_mgr.GetCategories()[i]
            
            cats.append(cat_id)

            self.cats_mgr.deletedCategory.disconnect(self.Update)
            self.cats_mgr.DeleteCategory(cat_id)
            self.cats_mgr.deletedCategory.connect(self.Update)
            
        self.SetItemCount(len(self.cats_mgr.GetCategories()))
        
    def OnSel(self, event):
        if self.sel_cats_in_iscatt:
            indexList = self.GetSelectedIndices()
            sel_cats = []
            cats = self.cats_mgr.GetCategories()
            for i in indexList:
                sel_cats.append(cats[i])       

            if sel_cats:
                self.cats_mgr.SetSelectedCat(sel_cats[0])
        event.Skip()

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

    def DeselectAll(self):
        """!Deselect all items"""
        indexList = self.GetSelectedIndices()
        for i in indexList:
            self.Select(i, on = 0)
         
        # no highlight
        self.OnCategorySelected(None)
        
    def OnGetItemText(self, item, col):
        attr = self.columns[col][1]
        cat_id = self.cats_mgr.GetCategories()[item]

        return self.cats_mgr.GetCategoryAttrs(cat_id)[attr]

    def OnGetItemImage(self, item):
        return -1

    def OnGetItemAttr(self, item):
        cat_id = self.cats_mgr.GetCategories()[item]

        cattr = self.cats_mgr.GetCategoryAttrs(cat_id)
        
        if cattr['show']:
            c = cattr['color']
            
            back_c = wx.Colour(*map(int, c.split(':')))
            text_c = wx.Colour(*ContrastColor(back_c))
        else:
            back_c = wx.SystemSettings.GetColour(wx.SYS_COLOUR_INACTIVECAPTION)
            text_c = wx.SystemSettings.GetColour(wx.SYS_COLOUR_INACTIVECAPTIONTEXT)

        # if it is in scope of the method, gui falls, using self solved it 
        self.l = wx.ListItemAttr(colText=text_c, colBack=back_c)
        return self.l

    def OnCategoryRightUp(self, event):
        """!Show context menu on right click"""
        item, flags = self.HitTest((event.GetX(), event.GetY()))
        if item != wx.NOT_FOUND and flags & wx.LIST_HITTEST_ONITEM:
            self.rightClickedItemIdx = item

        # generate popup-menu
        cat_idx = self.rightClickedItemIdx

        cats = self.cats_mgr.GetCategories()
        cat_id = cats[cat_idx]
        showed = self.cats_mgr.GetCategoryAttrs(cat_id)['show']
        
        menu = wx.Menu()

        item_id = wx.NewId()
        menu.Append(item_id, text=_("Rename class"))
        self.Bind(wx.EVT_MENU, self.OnRename, id=item_id)

        item_id = wx.NewId()
        menu.Append(item_id, text=_("Set color"))
        self.Bind(wx.EVT_MENU, self.OnSetColor, id=item_id)

        item_id = wx.NewId()
        menu.Append(item_id, text=_("Change opacity level"))
        self.Bind(wx.EVT_MENU, self.OnPopupOpacityLevel, id=item_id)

        if showed:
            text = _("Hide")
        else:
            text = _("Show")

        item_id = wx.NewId()
        menu.Append(item_id, text = text)
        self.Bind(wx.EVT_MENU, lambda event : self._setCatAttrs(cat_id=cat_id,
                                                                attrs={'show' : not showed}), 
                                                                id=item_id) 
        
        menu.AppendSeparator()
        
        item_id = wx.NewId()
        menu.Append(item_id, text=_("Move to top"))
        self.Bind(wx.EVT_MENU, self.OnMoveTop, id=item_id)
        if cat_idx == 0:
            menu.Enable(item_id, False)

        item_id = wx.NewId()
        menu.Append(item_id, text=_("Move to bottom"))
        self.Bind(wx.EVT_MENU, self.OnMoveBottom, id=item_id)
        if cat_idx == len(cats) - 1:
            menu.Enable(item_id, False)

        menu.AppendSeparator()

        item_id = wx.NewId()
        menu.Append(item_id, text=_("Move category up"))
        self.Bind(wx.EVT_MENU, self.OnMoveUp, id=item_id)
        if cat_idx == 0:
            menu.Enable(item_id, False)

        item_id = wx.NewId()
        menu.Append(item_id, text=_("Move category down"))
        self.Bind(wx.EVT_MENU, self.OnMoveDown, id=item_id)
        if cat_idx == len(cats) - 1:
            menu.Enable(item_id, False)

        menu.AppendSeparator()

        item_id = wx.NewId()
        menu.Append(item_id, text=_("Export class raster"))
        self.Bind(wx.EVT_MENU, self.OnExportCatRast, id=item_id)

        self.PopupMenu(menu)
        menu.Destroy()

    def OnExportCatRast(self, event):
        """!Export training areas"""
        #TODO
        #   GMessage(parent=self, message=_("No class raster to export."))
        #    return

        cat_idx = self.rightClickedItemIdx
        cat_id = self.cats_mgr.GetCategories()[cat_idx]

        self.cats_mgr.ExportCatRast(cat_id)

    def OnMoveUp(self, event):
        cat_idx = self.rightClickedItemIdx
        cat_id = self.cats_mgr.GetCategories()[cat_idx]
        self.cats_mgr.ChangePosition(cat_id, cat_idx - 1)
        self.RefreshItems(0, len(self.cats_mgr.GetCategories()))

    def OnMoveDown(self, event):
        cat_idx = self.rightClickedItemIdx
        cat_id = self.cats_mgr.GetCategories()[cat_idx]
        self.cats_mgr.ChangePosition(cat_id, cat_idx + 1)
        self.RefreshItems(0, len(self.cats_mgr.GetCategories()))

    def OnMoveTop(self, event):
        cat_idx = self.rightClickedItemIdx
        cat_id = self.cats_mgr.GetCategories()[cat_idx]
        self.cats_mgr.ChangePosition(cat_id, 0)
        self.RefreshItems(0, len(self.cats_mgr.GetCategories()))

    def OnMoveBottom(self, event):
        cat_idx = self.rightClickedItemIdx
        cats = self.cats_mgr.GetCategories()
        cat_id = cats[cat_idx]
        self.cats_mgr.ChangePosition(cat_id, len(cats) - 1)
        self.RefreshItems(0, len(self.cats_mgr.GetCategories()))

    def OnSetColor(self, event):
        """!Popup opacity level indicator"""
        cat_idx = self.rightClickedItemIdx
        cat_id = self.cats_mgr.GetCategories()[cat_idx]

        col = self.cats_mgr.GetCategoryAttrs(cat_id)['color']
        col = map(int, col.split(':'))

        col_data =  wx.ColourData()
        col_data.SetColour(wx.Colour(*col))

        dlg = wx.ColourDialog(self, col_data)
        dlg.GetColourData().SetChooseFull(True)

        if dlg.ShowModal() == wx.ID_OK:
            color = dlg.GetColourData().GetColour().Get()
            color = ':'.join(map(str, color))
            self.cats_mgr.SetCategoryAttrs(cat_id, {"color" : color})

        dlg.Destroy()

    def OnPopupOpacityLevel(self, event):
        """!Popup opacity level indicator"""

        cat_id = self.cats_mgr.GetCategories()[self.rightClickedItemIdx]
        cat_attrs = self.cats_mgr.GetCategoryAttrs(cat_id)
        value = cat_attrs['opacity'] * 100
        name = cat_attrs['name']

        dlg = SetOpacityDialog(self, opacity = value,
                               title = _("Change opacity of class <%s>" % name))

        dlg.applyOpacity.connect(lambda value:
                                 self._setCatAttrs(cat_id=cat_id, attrs={'opacity' : value}))
        dlg.CentreOnParent()

        if dlg.ShowModal() == wx.ID_OK:
            self._setCatAttrs(cat_id=cat_id, attrs={'opacity' : value})
        
        dlg.Destroy()

    def OnRename(self, event):
        cat_id = self.cats_mgr.GetCategories()[self.rightClickedItemIdx]
        cat_attrs = self.cats_mgr.GetCategoryAttrs(cat_id)

        dlg = RenameClassDialog(self, old_name=cat_attrs['name'])
        dlg.CentreOnParent()

        while True:
            if dlg.ShowModal() == wx.ID_OK:
                name = dlg.GetNewName().strip()
                if not name:
                    GMessage(parent=self, message=_("Empty name was inserted."))
                else:
                    self.cats_mgr.SetCategoryAttrs(cat_id, {"name" : name})
                    break
            else:
                break

        dlg.Destroy()

    def _setCatAttrs(self, cat_id, attrs):
        self.cats_mgr.setCategoryAttrs.disconnect(self.Update)
        self.cats_mgr.SetCategoryAttrs(cat_id, attrs)
        self.cats_mgr.setCategoryAttrs.connect(self.Update)
