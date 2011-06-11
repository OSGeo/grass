"""!
@package menu.py

@brief Menu classes for wxGUI

Classes:
 - Menu

(C) 2010 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Pawel Netzel (menu customization)
@author Milena Nowotarska (menu customization)
@author Robert Szczepanek (menu customization)
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
"""

import wx

import globalvar
import utils

from preferences import globalSettings as UserSettings

class Menu(wx.MenuBar):
    def __init__(self, parent, data):
        """!Creates menubar"""
        wx.MenuBar.__init__(self)
        self.parent   = parent
        self.menudata = data
        self.menucmd  = dict()
        
        self.menustyle = UserSettings.Get(group='appearance', key='menustyle', subkey='selection')
        
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
                        shortcut = '', kind = wx.ITEM_NORMAL):
        """!Creates menu items
        There are three menu styles (menu item text styles).
        1 -- label only, 2 -- label and cmd name, 3 -- cmd name only
        """
        if not label:
            menu.AppendSeparator()
            return
        
        if len(gcmd) > 0:
            helpString = gcmd + ' -- ' + help
            if menustyle == 1:
                label += '   [' + gcmd + ']'
            elif menustyle == 2:
                label = '      [' + gcmd + ']'
        else:
            helpString = help
        
        if shortcut:
            label += '\t' + shortcut
        
        menuItem = menu.Append(wx.ID_ANY, label, helpString, kind)
        
        self.menucmd[menuItem.GetId()] = gcmd
        
        if gcmd: 
            try: 
                cmd = utils.split(str(gcmd)) 
            except UnicodeError: 
                cmd = utils.split(utils.EncodeString((gcmd))) 
            if cmd and cmd[0] not in globalvar.grassCmd['all']: 
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
