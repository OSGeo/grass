"""!
@package gui_core.menu

@brief Menu classes for wxGUI

Classes:
 - menu::Menu
 - menu::SearchModuleWindow
 - menu::MenuTree

(C) 2010-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Pawel Netzel (menu customization)
@author Milena Nowotarska (menu customization)
@author Robert Szczepanek (menu customization)
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
"""

import wx

from core              import globalvar
from core              import utils
from core.modulesdata  import ModulesData
from core.gcmd         import EncodeString
from core.settings     import UserSettings
from gui_core.widgets  import ItemTree, SearchModuleWidget
from lmgr.menudata     import LayerManagerMenuData

class Menu(wx.MenuBar):
    def __init__(self, parent, data):
        """!Creates menubar"""
        wx.MenuBar.__init__(self)
        self.parent   = parent
        self.menudata = data
        self.menucmd  = dict()
        
        self.menustyle = UserSettings.Get(group = 'appearance', key = 'menustyle', subkey = 'selection')
        
        for eachMenuData in self.menudata.GetMenu():
            for eachHeading in eachMenuData:
                menuLabel = eachHeading[0]
                menuItems = eachHeading[1]
                self.Append(self._createMenu(menuItems), menuLabel)
        
    def _createMenu(self, menuData):
        """!Creates menu"""
        menu = wx.Menu()
        for eachItem in menuData:
            if len(eachItem) == 2:
                label = eachItem[0]
                subMenu = self._createMenu(eachItem[1])
                menu.AppendMenu(wx.ID_ANY, label, subMenu)
            else:
                self._createMenuItem(menu, self.menustyle, *eachItem)
        
        self.parent.Bind(wx.EVT_MENU_HIGHLIGHT_ALL, self.OnMenuHighlight)
        
        return menu

    def _createMenuItem(self, menu, menustyle, label, help, handler, gcmd, keywords,
                        shortcut = '', wxId = wx.ID_ANY, kind = wx.ITEM_NORMAL):
        """!Creates menu items
        There are three menu styles (menu item text styles).
        1 -- label only, 2 -- label and cmd name, 3 -- cmd name only
        """
        if not label:
            menu.AppendSeparator()
            return
        
        if gcmd:
            helpString = gcmd + ' -- ' + help
            if menustyle == 1:
                label += '   [' + gcmd + ']'
            elif menustyle == 2:
                label = '      [' + gcmd + ']'
        else:
            helpString = help
        
        if shortcut:
            label += '\t' + shortcut
        
        menuItem = menu.Append(wxId, label, helpString, kind)
        
        self.menucmd[menuItem.GetId()] = gcmd
        
        if gcmd: 
            try: 
                cmd = utils.split(str(gcmd)) 
            except UnicodeError: 
                cmd = utils.split(EncodeString((gcmd))) 
            if cmd and cmd[0] not in globalvar.grassCmd: 
                menuItem.Enable(False)
        
        rhandler = eval('self.parent.' + handler)
        
        self.parent.Bind(wx.EVT_MENU, rhandler, menuItem)

    def GetData(self):
        """!Get menu data"""
        return self.menudata
    
    def GetCmd(self):
        """!Get list of commands

        @return list of commands
        """
        return self.menucmd
        
    def OnMenuHighlight(self, event):
        """
        Default menu help handler
        """
         # Show how to get menu item info from this event handler
        id = event.GetMenuId()
        item = self.FindItemById(id)
        if item:
            text = item.GetText()
            help = item.GetHelp()

        # but in this case just call Skip so the default is done
        event.Skip()

class SearchModuleWindow(wx.Panel):
    """!Show menu tree"""
    def __init__(self, parent, id = wx.ID_ANY, **kwargs):
        self.parent = parent # LayerManager
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self.dataBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                    label = " %s " % _("Menu tree (double-click or Ctrl-Enter to run command)"))
        # tree
        menuData = LayerManagerMenuData()
        self.tree = MenuTree(parent = self, data = menuData)
        self.tree.Load()

        # search widget
        self.search = SearchModuleWidget(parent = self,
                                         modulesData = ModulesData(menuData.GetModules()),
                                         showChoice = False)
        
        # buttons
        self.btnRun   = wx.Button(self, id = wx.ID_OK, label = _("&Run"))
        self.btnRun.SetToolTipString(_("Run selected command from the menu tree"))
        self.btnRun.Enable(False)
        
        # bindings
        self.btnRun.Bind(wx.EVT_BUTTON,            self.OnRun)
        self.tree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.OnItemActivated)
        self.tree.Bind(wx.EVT_TREE_SEL_CHANGED,    self.OnItemSelected)
        self.search.GetCtrl().Bind(wx.EVT_TEXT,    self.OnUpdateStatusBar)
        self.search.GetCtrl().Bind(wx.EVT_KEY_UP,  self.OnKeyUp)

        # because number of matched items differs
        # from number of matched items in tree
        # TODO: find the reason for this difference
        # TODO: use this event for updating statusbar
        # TODO: some showNotification usage?

        self._layout()
        
        self.search.SetFocus()
        
    def _layout(self):
        """!Do dialog layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        # body
        dataSizer = wx.StaticBoxSizer(self.dataBox, wx.HORIZONTAL)
        dataSizer.Add(item = self.tree, proportion =1,
                      flag = wx.EXPAND)
        
        # buttons
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = self.btnRun, proportion = 0)
        
        sizer.Add(item = dataSizer, proportion = 1,
                  flag = wx.EXPAND | wx.ALL, border = 5)

        sizer.Add(item = self.search, proportion = 0,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALIGN_RIGHT | wx.BOTTOM | wx.RIGHT, border = 5)
        
        sizer.Fit(self)
        sizer.SetSizeHints(self)
        
        self.SetSizer(sizer)
        
        self.Fit()
        self.SetAutoLayout(True)        
        self.Layout()
        
    def OnCloseWindow(self, event):
        """!Close window"""
        self.Destroy()
        
    def OnRun(self, event):
        """!Run selected command"""
        if not self.tree.GetSelected():
            return # should not happen
        
        data = self.tree.GetPyData(self.tree.GetSelected())
        if not data:
            return

        handler = 'self.parent.' + data['handler'].lstrip('self.')
        if data['handler'] == 'self.OnXTerm':
            wx.MessageBox(parent = self,
                          message = _('You must run this command from the menu or command line',
                                      'This command require an XTerm'),
                          caption = _('Message'), style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
        elif data['command']:
            eval(handler)(event = None, cmd = data['command'].split())
        else:
            eval(handler)(None)

    def OnKeyUp(self, event):
        if event.GetKeyCode() == wx.WXK_RETURN:
            if event.ControlDown():
                self.OnRun(event)
            else:
                self.OnShowItem(event)
        
    def OnShowItem(self, event):
        """!Show selected item"""
        self.tree.OnShowItem(event)
        if self.tree.GetSelected():
            self.btnRun.Enable()
        else:
            self.btnRun.Enable(False)
        
    def OnItemActivated(self, event):
        """!Item activated (double-click)"""
        item = event.GetItem()
        if not item or not item.IsOk():
            return
        
        data = self.tree.GetPyData(item)
        if not data or 'command' not in data:
            return
        
        self.tree.itemSelected = item
        
        self.OnRun(None)
        
    def OnItemSelected(self, event):
        """!Item selected"""
        item = event.GetItem()
        if not item or not item.IsOk():
            return
        
        data = self.tree.GetPyData(item)
        if not data or 'command' not in data:
            return
        
        if data['command']:
            label = data['command'] + ' -- ' + data['description']
        else:
            label = data['description']
        
        self.parent.SetStatusText(label, 0)
        
    def OnUpdateStatusBar(self, event):
        """!Update statusbar text"""
        element = self.search.GetSelection()
        value = event.GetEventObject().GetValue()
        self.tree.SearchItems(element = element, value = value)
        
        nItems = len(self.tree.itemsMarked)
        if value:
            self.parent.SetStatusText(_("%d modules match") % nItems, 0)
        else:
            self.parent.SetStatusText("", 0)
        
        event.Skip()
        
class MenuTree(ItemTree):
    """!Menu tree class"""
    def __init__(self, parent, data, **kwargs):
        self.parent   = parent
        self.menudata = data

        super(MenuTree, self).__init__(parent, **kwargs)
        
        self.menustyle = UserSettings.Get(group = 'appearance', key = 'menustyle', subkey = 'selection')
        
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
                                       text = label.replace('&', ''))
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
                    label = eachItem[0]
                    if eachItem[3]:
                        if self.menustyle == 1:
                            label += ' [' + eachItem[3] + ']'
                        elif self.menustyle == 2:
                            label = '[' + eachItem[3] + ']'
                    
                    itemNew = self.AppendItem(parentId = item,
                                              text = label)
                    
                    data = { 'item'        : eachItem[0],
                             'description' : eachItem[1],
                             'handler'  : eachItem[2],
                             'command'  : eachItem[3],
                             'keywords' : eachItem[4] }
                    
                    self.SetPyData(itemNew, data)
