"""!
@package help.py

@brief Help window

@todo Needs improvements...

Classes:
 - HelpWindow
 - MenuTreeWindow

(C) 2008-2009 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import wx
import wx.lib.customtreectrl as CT

import menudata
import gcmd

class HelpWindow(wx.Frame):
    """!GRASS Quickstart help window"""
    def __init__(self, parent, id, title, size, file):

        wx.Frame.__init__(self, parent=parent, id=id, title=title, size=size)

        sizer = wx.BoxSizer(wx.VERTICAL)

        # text
        helpFrame = wx.html.HtmlWindow(parent=self, id=wx.ID_ANY)
        helpFrame.SetStandardFonts (size = 10)
        helpFrame.SetBorders(10)
        wx.InitAllImageHandlers()

        helpFrame.LoadFile(file)
        self.Ok = True

        sizer.Add(item=helpFrame, proportion=1, flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        #        sizer.Fit(self)
        #        sizer.SetSizeHints(self)
        self.Layout()

class MenuTreeWindow(wx.Frame):
    """!Show menu tree"""
    def __init__(self, parent, id = wx.ID_ANY, title = _("Menu tree window"), **kwargs):
        self.parent = parent # LayerManager
        
        wx.Frame.__init__(self, parent = parent, id = id, title = title, **kwargs)

        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        # tree
        self.tree = MenuTree(parent = self.panel, data = menudata.Data())
        self.tree.Load()
        
        self.searchDict = { _('label')    : 'label', # i18n workaround
                            _('help')     : 'help',
                            _('command')  : 'command',
                            _('keywords') : 'keywords' }
        # search
        self.searchBy = wx.Choice(parent = self.panel, id = wx.ID_ANY,
                                  choices = [_('label'),
                                             _('help'),
                                             _('command'),
                                             _('keywords')])
        self.search = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY,
                                  value = "", size = (-1, 25),
                                  style = wx.TE_PROCESS_ENTER)
        
        # statusbar
        self.statusbar = self.CreateStatusBar(number=1)

        # close on run
        self.closeOnRun = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                      label = _("Close dialog on run"))
        self.closeOnRun.SetValue(True)
        
        # buttons
        self.btnRun   = wx.Button(self.panel, id = wx.ID_OK, label = _("Run"))
        self.btnRun.SetToolTipString(_("Run selected command"))
        self.btnRun.Enable(False)
        self.btnClose = wx.Button(self.panel, id = wx.ID_CLOSE)
        self.btnClose.SetToolTipString(_("Close dialog"))

        # bindings
        self.btnClose.Bind(wx.EVT_BUTTON,          self.OnCloseWindow)
        self.btnRun.Bind(wx.EVT_BUTTON,            self.OnRun)
        self.search.Bind(wx.EVT_TEXT_ENTER,        self.OnShowItem)
        self.search.Bind(wx.EVT_TEXT,              self.OnUpdateStatusBar)
        self.tree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.OnItemActivated)
        self.tree.Bind(wx.EVT_TREE_SEL_CHANGED,    self.OnItemSelected)

        self.__Layout()

        self.search.SetFocus()
        
    def __Layout(self):
        """!Do dialog layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        # body
        dataBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                               label=" %s " % _("Menu tree (double-click to run command)"))
        
        dataSizer = wx.StaticBoxSizer(dataBox, wx.HORIZONTAL)
        dataSizer.Add(item = self.tree, proportion =1,
                      flag = wx.EXPAND)
        
        # search
        searchSizer = wx.BoxSizer(wx.HORIZONTAL)
        searchSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                             label = _("Search:")),
                        proportion = 0,
                        flag = wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                        border = 3)
        searchSizer.Add(item = self.searchBy, proportion = 0,
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT | wx.RIGHT,
                        border = 5)
        searchSizer.Add(item = self.search, proportion = 1,
                        flag = wx.EXPAND | wx.RIGHT | wx.ALIGN_CENTER_VERTICAL,
                        border = 5)
        
        # buttons
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = self.btnRun, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        btnSizer.Add(item = self.btnClose, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        
        sizer.Add(item = dataSizer, proportion = 1,
                  flag = wx.EXPAND | wx.ALL, border = 5)

        sizer.Add(item = searchSizer, proportion=0,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        sizer.Add(item = btnSizer, proportion=0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        sizer.Add(item = self.closeOnRun, proportion=0,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        
        self.Layout()
        self.SetSize((530, 370))
        
    def OnCloseWindow(self, event):
        """!Close window"""
        self.Destroy()
        
    def OnRun(self, event):
        """!Run selected command"""
        if not self.tree.GetSelected():
            return # should not happen
        
        data = self.tree.GetPyData(self.tree.GetSelected())
        if not data or not data.has_key('command'):
            return
        
        gcmd.RunCommand(data['command'])
        
        if self.closeOnRun.IsChecked():
            self.OnCloseWindow(None)
        
    def OnItemActivated(self, event):
        """!Item activated (double-click)"""
        item = event.GetItem()
        if not item or not item.IsOk():
            return
        
        data = self.tree.GetPyData(item)
        if not data or not data.has_key('command'):
            return
        
        self.tree.itemSelected = item
        
        self.OnRun(None)
        
    def OnItemSelected(self, event):
        """!Item selected"""
        item = event.GetItem()
        if not item or not item.IsOk():
            return
        
        data = self.tree.GetPyData(item)
        if not data or not data.has_key('command'):
            return
        
        self.statusbar.SetStatusText(data['command'] + ' -- ' + data['help'], 0)
        
    def OnShowItem(self, event):
        """!Highlight first found item in menu tree"""
        if len(self.tree.itemsMarked) > 0:
            if self.tree.GetSelected():
                self.tree.ToggleItemSelection(self.tree.GetSelected())
                idx = self.tree.itemsMarked.index(self.tree.GetSelected()) + 1
            else:
                idx = 0
            try:
                self.tree.ToggleItemSelection(self.tree.itemsMarked[idx])
                self.tree.itemSelected = self.tree.itemsMarked[idx]
                self.tree.EnsureVisible(self.tree.itemsMarked[idx])
            except IndexError:
                self.tree.ToggleItemSelection(self.tree.itemsMarked[0]) # reselect first item
                self.tree.EnsureVisible(self.tree.itemsMarked[0])
                self.tree.itemSelected = self.tree.itemsMarked[0]
        else:
            for item in self.tree.root.GetChildren():
                self.tree.Collapse(item)
            itemSelected = self.tree.GetSelection()
            if itemSelected:
                self.tree.ToggleItemSelection(itemSelected)
            self.tree.itemSelected = None

    def OnUpdateStatusBar(self, event):
        """!Update statusbar text"""
        element = self.searchDict[self.searchBy.GetStringSelection()]
        self.tree.itemsMarked = self.SearchItems(element = element,
                                                 value = event.GetString())
        self.tree.itemSelected = None
        
        nItems = len(self.tree.itemsMarked)
        if event.GetString():
            self.statusbar.SetStatusText(_("%d items match") % nItems, 0)
        else:
            self.statusbar.SetStatusText("", 0)
        
        if nItems > 0:
            self.tree.itemSelected = self.tree.itemsMarked[0]
            self.btnRun.Enable()
        else:
            self.btnRun.Enable(False)
        
        event.Skip()
        
    def SearchItems(self, element, value):
        """!Search item 

        @param element element index (see self.searchBy)
        @param value

        @return list of found tree items
        """
        items = list()
        if not value:
            return items
        
        item = self.tree.GetFirstChild(self.tree.root)[0]
        self.__ProcessItem(item, element, value, items)
        
        return items
    
    def __ProcessItem(self, item, element, value, listOfItems):
        """!Search items (used by SearchItems)

        @param item reference item
        @param listOfItems list of found items
        """
        while item and item.IsOk():
            subItem = self.tree.GetFirstChild(item)[0]
            if subItem:
                self.__ProcessItem(subItem, element, value, listOfItems)
            data = self.tree.GetPyData(item)
            
            if data and data.has_key(element) and \
                    value.lower() in data[element].lower():
                listOfItems.append(item)
            
            item = self.tree.GetNextSibling(item)
        
class MenuTree(CT.CustomTreeCtrl):
    """!Menu tree class"""
    def __init__(self, parent, data, id = wx.ID_ANY,
                 ctstyle = CT.TR_HIDE_ROOT | CT.TR_FULL_ROW_HIGHLIGHT | CT.TR_HAS_BUTTONS | \
                     CT.TR_LINES_AT_ROOT | CT.TR_SINGLE,
                 **kwargs):
        self.parent   = parent
        self.menudata = data

        super(MenuTree, self).__init__(parent, id, ctstyle = ctstyle, **kwargs)

        self.root = self.AddRoot(_("Menu tree"))
        self.itemsMarked = [] # list of marked items
        self.itemSelected = None
        
    def Load(self, data = None):
        """!Load menu data tree

        @param data menu data (None to use self.menudata)
        """
        if not data:
            data = self.menudata
        
        self.itemsMarked = [] # list of marked items
        for eachMenuData in data.GetMenu():
            for label, items in eachMenuData:
                item = self.AppendItem(parentId = self.root,
                                       text = label)
                self.__AppendItems(item, items)
        
    def __AppendItems(self, item, data):
        """!Append items into tree (used by Load()
        
        @param item tree item (parent)
        @parent data menu data"""
        for eachItem in data:
            if len(eachItem) == 2:
                if eachItem[0]:
                    itemSub = self.AppendItem(parentId = item,
                                    text = eachItem[0])
                self.__AppendItems(itemSub, eachItem[1])
            else:
                if eachItem[0]:
                    itemNew = self.AppendItem(parentId = item,
                                              text = eachItem[0])
                    
                    data = { 'label'    : eachItem[0],
                             'help'     : eachItem[1],
                             'handler'  : eachItem[2],
                             'command'  : eachItem[3],
                             'keywords' : eachItem[4] }
                    
                    self.SetPyData(itemNew, data)
        
    def GetSelected(self):
        """!Get selected item"""
        return self.itemSelected
