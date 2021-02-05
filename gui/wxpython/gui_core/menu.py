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
@author Tomas Zigo <tomas.zigo slovanet.sk> RecentFilesMenu
"""
import re
import os
import wx

from core import globalvar
from core import utils
from core.gcmd import EncodeString
from gui_core.treeview import CTreeView
from gui_core.wrap import Button, SearchCtrl
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
        self._model = model

        self.showNotification = Signal('SearchModuleWindow.showNotification')
        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)

        # search widget
        self._search = SearchCtrl(self)
        self._search.SetDescriptiveText(_('Search'))
        self._search.ShowCancelButton(True)
        self._btnAdvancedSearch = Button(self, id=wx.ID_ANY,
                                         label=_("Adva&nced search..."))
        self._btnAdvancedSearch.SetToolTip(
            _("Do advanced search using %s module") % 'g.search.module')
        # tree
        self._tree = CTreeView(model=model, parent=self)
        self._tree.SetToolTip(
            _("Double-click to run selected module"))

        # buttons
        self._btnRun = Button(self, id=wx.ID_OK, label=_("&Run..."))
        self._btnRun.SetToolTip(_("Run selected module from the tree"))
        self._btnHelp = Button(self, id=wx.ID_ANY, label=_("H&elp"))
        self._btnHelp.SetToolTip(
            _("Show manual for selected module from the tree"))

        # bindings
        self._search.Bind(wx.EVT_TEXT, lambda evt: self.Filter(evt.GetString()))
        self._search.Bind(wx.EVT_SEARCHCTRL_CANCEL_BTN,
                          lambda evt: self.Filter(''))
        self._btnRun.Bind(wx.EVT_BUTTON, lambda evt: self.Run())
        self._btnHelp.Bind(wx.EVT_BUTTON, lambda evt: self.Help())
        self._btnAdvancedSearch.Bind(wx.EVT_BUTTON,
                                     lambda evt: self.AdvancedSearch())

        self._tree.selectionChanged.connect(self.OnItemSelected)
        self._tree.itemActivated.connect(lambda node: self.Run(node))

        self._layout()

        self._search.SetFocus()

    def _layout(self):
        """Do dialog layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        # search
        searchSizer = wx.BoxSizer(wx.HORIZONTAL)
        searchSizer.Add(self._search, proportion=1,
                        flag=wx.EXPAND | wx.RIGHT, border=5)
        searchSizer.Add(self._btnAdvancedSearch, proportion=0,
                        flag=wx.EXPAND)
        sizer.Add(searchSizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL, border=5)
        # body
        sizer.Add(self._tree, proportion=1,
                  flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5)

        # buttons
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.AddStretchSpacer()
        btnSizer.Add(self._btnHelp, proportion=0,
                     flag=wx.EXPAND | wx.RIGHT, border=5)
        btnSizer.Add(self._btnRun, proportion=0)

        sizer.Add(btnSizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL, border=5)

        self.SetSizerAndFit(sizer)
        self.SetAutoLayout(True)
        self.Layout()

    def Filter(self, text):
        if text:
            model = self._model.Filtered(key=['command', 'keywords', 'description'],
                                         value=text)
            self._tree.SetModel(model)
            self._tree.ExpandAll()
        else:
            self._tree.SetModel(self._model)

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
            # expand/collapse location/mapset...
            if self._tree.IsNodeExpanded(node):
                self._tree.CollapseNode(node, recursive=False)
            else:
                self._tree.ExpandNode(node, recursive=False)
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


class RecentFilesMenu:
    """Add recent files history menu

    Signal FileRequested is emitted if you request file from recent
    files menu
    :param str path: file path you requested
    :param bool file_exists: file path exists or not
    :param obj file_history: file history obj instance


    :param str app_name: required for group name of recent files path
    written into the .recent_files file
    :param obj parent_menu: menu widget instance where be inserted
    recent files menu on the specified position
    :param int pos: position (index) where insert recent files menu in
    the parent menu
    :param int history_len: the maximum number of file paths written
    into the .recent_files file to app name group
    """

    recent_files = '.recent_files'

    def __init__(self, app_name, parent_menu, pos, history_len=10):
        self._history_len = history_len
        self._parent_menu = parent_menu
        self._pos = pos

        self.file_requested = Signal('RecentFilesMenu.FileRequested')

        self._filehistory = wx.FileHistory(maxFiles=history_len)
        # Recent files path stored in GRASS GIS config dir in the
        # .recent_files file in the group by application name
        self._config = wx.FileConfig(
            style=wx.CONFIG_USE_LOCAL_FILE,
            localFilename=os.path.join(
                utils.GetSettingsPath(), self.recent_files,
            ),
        )
        self._config.SetPath(strPath=app_name)
        self._filehistory.Load(self._config)
        self.RemoveNonExistentFiles()

        self.recent = wx.Menu()
        self._filehistory.UseMenu(self.recent)
        self._filehistory.AddFilesToMenu()

        # Show recent files menu if count of items in menu > 0
        if self._filehistory.GetCount() > 0:
            self._insertMenu()

    def _insertMenu(self):
        """Insert recent files menu into the parent menu on the
        specified position if count of menu items > 0"""

        self._parent_menu.Insert(
            pos=self._pos, id=wx.ID_ANY, text=_('&Recent Files'),
            submenu=self.recent,
        )
        self.recent.Bind(
            wx.EVT_MENU_RANGE, self._onFileHistory,
            id=wx.ID_FILE1, id2=wx.ID_FILE + self._history_len,
        )

    def _onFileHistory(self, event):
        """Choose recent file from menu event"""

        file_exists = True
        file_index = event.GetId() - wx.ID_FILE1
        path = self._filehistory.GetHistoryFile(file_index)

        if not os.path.exists(path):
            self.RemoveFileFromHistory(file_index)
            file_exists = False
        self.file_requested.emit(
            path=path, file_exists=file_exists,
            file_history=self._filehistory,
        )

    def AddFileToHistory(self, filename):
        """Add file to history, and save history into '.recent_files'
        file

        :param str filename: file path

        :return None
        """

        if self._filehistory.GetCount() == 0:
            self._insertMenu()
        if filename:
            self._filehistory.AddFileToHistory(filename)
            self._filehistory.Save(self._config)
            self._config.Flush()

    def RemoveFileFromHistory(self, file_index):
        """Remove file from the history.

        :param int file_index: filed index

        :return: None
        """
        self._filehistory.RemoveFileFromHistory(i=file_index)
        self._filehistory.Save(self._config)
        self._config.Flush()

    def RemoveNonExistentFiles(self):
        """Remove non existent files from the history"""
        for i in reversed(range(0, self._filehistory.GetCount())):
            file = self._filehistory.GetHistoryFile(index=i)
            if not os.path.exists(file):
                self._filehistory.RemoveFileFromHistory(i=i)

        self._filehistory.Save(self._config)
        self._config.Flush()
