import wx

import globalvar

class Menu(wx.MenuBar):
    def __init__(self, parent, data):
        """!Creates menubar"""
        wx.MenuBar.__init__(self)
        self.parent   = parent
        self.menudata = data
        self.menucmd  = dict()
        
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
                self._createMenuItem(menu, *eachItem)
        
        self.parent.Bind(wx.EVT_MENU_HIGHLIGHT_ALL, self.parent.OnMenuHighlight)
        
        return menu

    def _createMenuItem(self, menu, label, help, handler, gcmd, keywords,
                        shortcut = '', kind = wx.ITEM_NORMAL):
        """!Creates menu items"""
        if not label:
            menu.AppendSeparator()
            return
        
        if len(gcmd) > 0:
            helpString = gcmd + ' -- ' + help
        else:
            helpString = help
        
        if shortcut:
            label += '\t' + shortcut
        
        menuItem = menu.Append(wx.ID_ANY, label, helpString, kind)
        
        self.menucmd[menuItem.GetId()] = gcmd
        
        if len(gcmd) > 0 and \
                gcmd.split()[0] not in globalvar.grassCmd['all']:
            menuItem.Enable (False)
        
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
        
