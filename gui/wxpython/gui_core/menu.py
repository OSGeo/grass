"""!
@package gui_core.menu

@brief Menu classes for wxGUI

Classes:
 - menu::Menu
 - menu::SearchModuleWindow

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
from core.gcmd         import EncodeString
from core.utils import _
from gui_core.widgets  import SearchModuleWidget
from gui_core.treeview import CTreeView

from grass.pydispatch.signal import Signal

class Menu(wx.MenuBar):
    def __init__(self, parent, model):
        """!Creates menubar"""
        wx.MenuBar.__init__(self)
        self.parent = parent
        self.model = model
        self.menucmd = dict()

        for child in self.model.root.children:
            self.Append(self._createMenu(child), child.label)
        
    def _createMenu(self, node):
        """!Creates menu"""
        menu = wx.Menu()
        for child in node.children:
            if child.children:
                label = child.label
                subMenu = self._createMenu(child)
                menu.AppendMenu(wx.ID_ANY, label, subMenu)
            else:
                data = child.data.copy()
                data.pop('label')
                self._createMenuItem(menu, label=child.label, **data)
        
        self.parent.Bind(wx.EVT_MENU_HIGHLIGHT_ALL, self.OnMenuHighlight)
        
        return menu

    def _createMenuItem(self, menu, label, description, handler, command, keywords,
                        shortcut = '', wxId = wx.ID_ANY, kind = wx.ITEM_NORMAL):
        """!Creates menu items
        There are three menu styles (menu item text styles).
        1 -- label only, 2 -- label and cmd name, 3 -- cmd name only
        """
        if not label:
            menu.AppendSeparator()
            return
        
        if command:
            helpString = command + ' -- ' + description
        else:
            helpString = description
        
        if shortcut:
            label += '\t' + shortcut
        
        menuItem = menu.Append(wxId, label, helpString, kind)
        
        self.menucmd[menuItem.GetId()] = command
        
        if command: 
            try: 
                cmd = utils.split(str(command)) 
            except UnicodeError: 
                cmd = utils.split(EncodeString((command))) 
            if cmd and cmd[0] not in globalvar.grassCmd: 
                menuItem.Enable(False)

        rhandler = eval('self.parent.' + handler)
        self.parent.Bind(wx.EVT_MENU, rhandler, menuItem)
        
    def GetData(self):
        """!Get menu data"""
        return self.model
    
    def GetCmd(self):
        """!Get dictionary of commands (key is id)

        @return dictionary of commands
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
    """!Menu tree and search widget for searching modules.
    
        Signal:
            showNotification - attribute 'message'
    """
    def __init__(self, parent, model, id = wx.ID_ANY, **kwargs):
        self.parent = parent # LayerManager
        
        self.showNotification = Signal('SearchModuleWindow.showNotification')
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        # tree
        self._tree = CTreeView(model=model, parent=self)
        self._tree.SetToolTipString(_("Double-click or Ctrl-Enter to run selected module"))

#        self._dataBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
#                                     label = " %s " % _("Module tree"))

        # search widget
        self._search = SearchModuleWidget(parent=self,
                                          model=model,
                                          showChoice=False)
        self._search.showSearchResult.connect(lambda result: self._tree.Select(result))
        self._search.showNotification.connect(self.showNotification)
        
        self._helpText = wx.StaticText(parent=self, id=wx.ID_ANY,
                                       label="Press Enter for next match, Ctrl+Enter to run command")
        self._helpText.SetForegroundColour(wx.SystemSettings_GetColour(wx.SYS_COLOUR_GRAYTEXT))
        
        # buttons
        self._btnRun = wx.Button(self, id=wx.ID_OK, label=_("&Run"))
        self._btnRun.SetToolTipString(_("Run selected module from the tree"))
        
        # bindings
        self._btnRun.Bind(wx.EVT_BUTTON, lambda evt: self.Run())
        self.Bind(wx.EVT_KEY_UP,  self.OnKeyUp)
        
        self._tree.selectionChanged.connect(self.OnItemSelected)
        self._tree.itemActivated.connect(lambda node: self.Run(node))

        self._layout()
        
        self._search.SetFocus()
        
    def _layout(self):
        """!Do dialog layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        # body
        dataSizer = wx.BoxSizer(wx.HORIZONTAL)
        dataSizer.Add(item = self._tree, proportion =1,
                      flag = wx.EXPAND)
        
        # buttons
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = self._btnRun, proportion = 0)
        
        sizer.Add(item = dataSizer, proportion = 1,
                  flag = wx.EXPAND | wx.ALL, border = 5)

        sizer.Add(item = self._search, proportion = 0,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALIGN_RIGHT | wx.BOTTOM | wx.RIGHT, border = 5)
        
        sizer.Add(item=self._helpText,
                  proportion=0, flag=wx.EXPAND | wx.LEFT, border=5)
        
        sizer.Fit(self)
        sizer.SetSizeHints(self)
        
        self.SetSizer(sizer)
        
        self.Fit()
        self.SetAutoLayout(True)        
        self.Layout()
        
    def Run(self, module=None):
        """!Run selected command.
        
        @param module module (represented by tree node)
        """
        if module is None:
            if not self._tree.GetSelected():
                return

            module = self._tree.GetSelected()[0]
        data = module.data
        if not data:
            return

        handler = 'self.parent.' + data['handler'].lstrip('self.')

        if data['command']:
            eval(handler)(event=None, cmd=data['command'].split())
        else:
            eval(handler)(event=None)

    def OnKeyUp(self, event):
        """!Key or key combination pressed"""
        if event.ControlDown() and \
                event.GetKeyCode() in (wx.WXK_RETURN, wx.WXK_NUMPAD_ENTER):
            self.Run()
        
    def OnItemSelected(self, node):
        """!Item selected"""      
        data = node.data
        if not data or 'command' not in data:
            return
        
        if data['command']:
            label = data['command'] + ' -- ' + data['description']
        else:
            label = data['description']
        
        self.showNotification.emit(message=label)
