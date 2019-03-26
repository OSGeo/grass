"""
@package gui_core.menu

@brief Menu classes for wxGUI

Classes:
 - menu::Menu
 - menu::SearchModuleWindow

(C) 2010-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Pawel Netzel (menu customization)
@author Milena Nowotarska (menu customization)
@author Robert Szczepanek (menu customization)
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
"""
import re
import wx

from core import globalvar
from core import utils
from core.gcmd import EncodeString
from gui_core.widgets import SearchModuleWidget
from gui_core.treeview import CTreeView
from gui_core.wrap import Button, StaticText
from gui_core.wrap import Menu as MenuWidget
from icons.icon import MetaIcon

from grass.pydispatch.signal import Signal


class Menu(wx.MenuBar):

    def __init__(self, parent, model):
        """Creates menubar"""
        wx.MenuBar.__init__(self)
        self.parent = parent
        self.model = model
        self.menucmd = dict()
        self.bmpsize = (16, 16)

        for child in self.model.root.children:
            self.Append(self._createMenu(child), child.label)

    def _createMenu(self, node):
        """Creates menu"""
        menu = MenuWidget()
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

    def _createMenuItem(
            self, menu, label, description, handler, command, keywords,
            shortcut='', icon='', wxId=wx.ID_ANY, kind=wx.ITEM_NORMAL):
        """Creates menu items
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

        menuItem = wx.MenuItem(menu, wxId, label, helpString, kind)
        if icon:
            menuItem.SetBitmap(MetaIcon(img=icon).GetBitmap(self.bmpsize))
        menu.AppendItem(menuItem)

        self.menucmd[menuItem.GetId()] = command

        if command:
            try:
                cmd = utils.split(str(command))
            except UnicodeError:
                cmd = utils.split(EncodeString((command)))
            # disable only grass commands which are not present (e.g.
            # r.in.lidar)
            if cmd and cmd[0] not in globalvar.grassCmd and \
               re.match('[rvdipmgt][3bs]?\.([a-z0-9\.])+', cmd[0]):
                menuItem.Enable(False)

        rhandler = eval('self.parent.' + handler)
        self.parent.Bind(wx.EVT_MENU, rhandler, menuItem)

    def GetData(self):
        """Get menu data"""
        return self.model

    def GetCmd(self):
        """Get dictionary of commands (key is id)

        :return: dictionary of commands
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
    """Menu tree and search widget for searching modules.

    Signal:
        showNotification - attribute 'message'
    """

    def __init__(self, parent, handlerObj, giface, model, id=wx.ID_ANY,
                 **kwargs):
        self.parent = parent
        self._handlerObj = handlerObj
        self._giface = giface

        self.showNotification = Signal('SearchModuleWindow.showNotification')
        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)

        # tree
        self._tree = CTreeView(model=model, parent=self)
        self._tree.SetToolTip(
            _("Double-click or Ctrl-Enter to run selected module"))

#        self._dataBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
#                                     label = " %s " % _("Module tree"))

        # search widget
        self._search = SearchModuleWidget(parent=self,
                                          model=model,
                                          showChoice=False)
        self._search.showSearchResult.connect(
            lambda result: self._tree.Select(result))
        self._search.showNotification.connect(self.showNotification)

        self._helpText = StaticText(
            parent=self, id=wx.ID_ANY,
            label="Press Enter for next match, Ctrl+Enter to run command")
        self._helpText.SetForegroundColour(
            wx.SystemSettings.GetColour(
                wx.SYS_COLOUR_GRAYTEXT))

        # buttons
        self._btnRun = Button(self, id=wx.ID_OK, label=_("&Run"))
        self._btnRun.SetToolTip(_("Run selected module from the tree"))
        self._btnHelp = Button(self, id=wx.ID_ANY, label=_("H&elp"))
        self._btnHelp.SetToolTip(
            _("Show manual for selected module from the tree"))
        self._btnAdvancedSearch = Button(self, id=wx.ID_ANY,
                                         label=_("Adva&nced search..."))
        self._btnAdvancedSearch.SetToolTip(
            _("Do advanced search using %s module") % 'g.search.module')

        # bindings
        self._btnRun.Bind(wx.EVT_BUTTON, lambda evt: self.Run())
        self._btnHelp.Bind(wx.EVT_BUTTON, lambda evt: self.Help())
        self._btnAdvancedSearch.Bind(wx.EVT_BUTTON,
                                     lambda evt: self.AdvancedSearch())
        self.Bind(wx.EVT_KEY_UP, self.OnKeyUp)

        self._tree.selectionChanged.connect(self.OnItemSelected)
        self._tree.itemActivated.connect(lambda node: self.Run(node))

        self._layout()

        self._search.SetFocus()

    def _layout(self):
        """Do dialog layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        # body
        dataSizer = wx.BoxSizer(wx.HORIZONTAL)
        dataSizer.Add(self._tree, proportion=1,
                      flag=wx.EXPAND)

        # buttons
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self._btnAdvancedSearch, proportion=0)
        btnSizer.AddStretchSpacer()
        btnSizer.Add(self._btnHelp, proportion=0)
        btnSizer.Add(self._btnRun, proportion=0)

        sizer.Add(dataSizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL, border=5)

        sizer.Add(self._search, proportion=0,
                  flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        sizer.Add(btnSizer, proportion=0,
                  flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        sizer.Add(self._helpText,
                  proportion=0, flag=wx.EXPAND | wx.LEFT, border=5)

        sizer.Fit(self)
        sizer.SetSizeHints(self)

        self.SetSizer(sizer)

        self.Fit()
        self.SetAutoLayout(True)
        self.Layout()

    def _GetSelectedNode(self):
        selection = self._tree.GetSelected()
        if not selection:
            return None
        return selection[0]

    def Run(self, node=None):
        """Run selected command.

        :param node: a tree node associated with the module or other item
        """
        if not node:
            node = self._GetSelectedNode()
        # nothing selected
        if not node:
            return
        data = node.data
        # non-leaf nodes
        if not data:
            return

        # extract name of the handler and create a new call
        handler = 'self._handlerObj.' + data['handler'].lstrip('self.')

        if data['command']:
            eval(handler)(event=None, cmd=data['command'].split())
        else:
            eval(handler)(event=None)

    def Help(self, node=None):
        """Show documentation for a module"""
        if not node:
            node = self._GetSelectedNode()
        # nothing selected
        if not node:
            return
        data = node.data
        # non-leaf nodes
        if not data:
            return

        if not data['command']:
            # showing nothing for non-modules
            return
        # strip parameters from command if present
        name = data['command'].split()[0]
        self._giface.Help(name)
        self.showNotification.emit(
            message=_("Documentation for %s is now open in the web browser")
            % name)

    def AdvancedSearch(self):
        """Show advanced search window"""
        self._handlerObj.RunMenuCmd(cmd=['g.search.modules'])

    def OnKeyUp(self, event):
        """Key or key combination pressed"""
        if event.ControlDown() and \
                event.GetKeyCode() in (wx.WXK_RETURN, wx.WXK_NUMPAD_ENTER):
            self.Run()

    def OnItemSelected(self, node):
        """Item selected"""
        data = node.data
        if not data or 'command' not in data:
            return

        if data['command']:
            label = data['command']
            if data['description']:
                label += ' -- ' + data['description']
        else:
            label = data['description']

        self.showNotification.emit(message=label)
