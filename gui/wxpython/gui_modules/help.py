"""!
@package help.py

@brief Help window

@todo Needs improvements...

Classes:
 - HelpWindow
 - MenuTreeWindow
 - AboutWindow

(C) 2008-2009 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os

import wx
import wx.lib.customtreectrl as CT
import wx.lib.flatnotebook as FN
import  wx.lib.scrolledpanel as scrolled
from wx.lib.wordwrap import wordwrap

import menudata
import gcmd
import globalvar

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
        if not data:
            return

        handler = 'self.parent.' + data['handler'].lstrip('self.')
        if data['command']:
            eval(handler)(event = None, cmd = data['command'].split())
        else:
            eval(handler)(None)
        
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
        
        if data['command']:
            label = data['command'] + ' -- ' + data['help']
        else:
            label = data['help']
        
        self.statusbar.SetStatusText(label, 0)
        
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

        if self.tree.itemSelected:
            self.btnRun.Enable()
        else:
            self.btnRun.Enable(False)

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

class AboutWindow(wx.Frame):
    def __init__(self, parent):
        """!Create custom About Window

        @todo improve styling
        """
        wx.Frame.__init__(self, parent=parent, id=wx.ID_ANY, size=(550,400), 
                          title=_('About GRASS GIS'))
        
        # version and web site
        version, svn_gis_h_rev, svn_gis_h_date = gcmd.RunCommand('g.version',
                                                                 flags = 'r',
                                                                 read = True).splitlines()

        infoTxt = wx.Panel(parent = self, id = wx.ID_ANY)
        infoSizer = wx.BoxSizer(wx.VERTICAL)
        logo = os.path.join(globalvar.ETCDIR, "gui", "icons", "grass.ico")
        logoBitmap = wx.StaticBitmap(parent = infoTxt, id = wx.ID_ANY,
                                     bitmap = wx.Bitmap(name = logo,
                                                        type = wx.BITMAP_TYPE_ICO))
        infoSizer.Add(item = logoBitmap, proportion = 0,
                      flag = wx.ALL | wx.ALIGN_CENTER, border = 10)
        
        i = 0
        for label in [version.replace('GRASS', 'GRASS GIS').strip() + '\n\n',
                      _('Official GRASS site: http://grass.osgeo.org') + '\n\n',
                      _('GIS Library') + ' ' + svn_gis_h_rev + '(' + svn_gis_h_date.split(' ')[1] + ')']:
            info = wx.StaticText(parent = infoTxt,
                                 id = wx.ID_ANY,
                                 label = label)
            if i == 0:
                info.SetFont(wx.Font(13, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            infoSizer.Add(item = info, proportion = 0,
                          flag = wx.TOP | wx.ALIGN_CENTER, border = 5)
            i += 1
            
        # copyright information
        copyfile = os.path.join(os.getenv("GISBASE"), "COPYING")
        if os.path.exists(copyfile):
            copyrightFile = open(copyfile, 'r')
            copyrightOut = []
            copyright = copyrightFile.readlines()
            copytext = wordwrap(''.join(copyright[:11] + copyright[26:-3]),
                                575, wx.ClientDC(self))
            copyrightFile.close()
        else:
            copytext = _('COPYING file missing')
        # put text into a scrolling panel
        copyrightwin = scrolled.ScrolledPanel(self, id=wx.ID_ANY, 
                                              size=wx.DefaultSize,
                                              style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        copyrighttxt = wx.StaticText(copyrightwin, id=wx.ID_ANY, label=copytext)
        copyrightwin.SetAutoLayout(1)
        copyrightwin.SetupScrolling()
        copyrightwin.sizer = wx.BoxSizer(wx.VERTICAL)
        copyrightwin.sizer.Add(item=copyrighttxt, proportion=1,
                               flag=wx.EXPAND | wx.ALL, border=1)
        copyrightwin.SetSizer(copyrightwin.sizer)
        copyrightwin.Layout()

        # license
        licfile = os.path.join(os.getenv("GISBASE"), "GPL.TXT")
        if os.path.exists(licfile):
            licenceFile = open(licfile, 'r')
            license = ''.join(licenceFile.readlines())
            licenceFile.close()
        else:
            license = _('GPL.TXT file missing')
        # put text into a scrolling panel
        licensewin = scrolled.ScrolledPanel(self, id=wx.ID_ANY, 
                                            size=wx.DefaultSize,
                                            style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        licensetxt = wx.StaticText(licensewin, id=wx.ID_ANY, label=license)
        licensewin.SetAutoLayout(1)
        licensewin.SetupScrolling()
        licensewin.sizer = wx.BoxSizer(wx.VERTICAL)
        licensewin.sizer.Add(item=licensetxt, proportion=1,
                flag=wx.EXPAND | wx.ALL, border=1)
        licensewin.SetSizer(licensewin.sizer)
        licensewin.Layout()
        
        # credits
        authfile = os.path.join(os.getenv("GISBASE"), "AUTHORS")
        if os.path.exists(authfile):
            authorsFile = open(authfile, 'r')
            authors = unicode(''.join(authorsFile.readlines()), "utf-8")
            authorsFile.close()
        else:
            authors = _('AUTHORS file missing')
        authorwin = scrolled.ScrolledPanel(self, id=wx.ID_ANY, 
                                           size=wx.DefaultSize,
                                           style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        authortxt = wx.StaticText(authorwin, id=wx.ID_ANY, label=str(authors))
        authorwin.SetAutoLayout(1)
        authorwin.SetupScrolling()
        authorwin.sizer = wx.BoxSizer(wx.VERTICAL)
        authorwin.sizer.Add(item=authortxt, proportion=1,
                flag=wx.EXPAND | wx.ALL, border=1)
        authorwin.SetSizer(authorwin.sizer)
        authorwin.Layout()      
        
        # create a flat notebook for displaying information about GRASS
        nbstyle = FN.FNB_VC8 | \
                FN.FNB_BACKGROUND_GRADIENT | \
                FN.FNB_TABS_BORDER_SIMPLE | \
                FN.FNB_NO_X_BUTTON | \
                FN.FNB_NO_NAV_BUTTONS
                
        aboutNotebook = FN.FlatNotebook(self, id=wx.ID_ANY, style=nbstyle)
        aboutNotebook.SetTabAreaColour(globalvar.FNPageColor)
        
        # make pages for About GRASS notebook
        pg1 = aboutNotebook.AddPage(infoTxt,    text=_("Info"))
        pg2 = aboutNotebook.AddPage(copyrightwin, text=_("Copyright"))
        pg3 = aboutNotebook.AddPage(licensewin,   text=_("License"))
        pg4 = aboutNotebook.AddPage(authorwin,    text=_("Authors"))

        # buttons
        btnClose = wx.Button(parent = self, id = wx.ID_CLOSE)
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = btnClose, proportion = 1,
                     flag = wx.ALL | wx.EXPAND | wx.ALIGN_RIGHT,
                     border = 5)
        # bindings
        # self.aboutNotebook.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnAGPageChanged)
        btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)

        infoTxt.SetSizer(infoSizer)
        infoSizer.Fit(infoTxt)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(item=aboutNotebook, proportion=1,
                  flag=wx.EXPAND | wx.ALL, border=1)
        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT, border=1)
        self.SetSizer(sizer)
        self.Layout()
    
    def OnCloseWindow(self, event):
        """!Close window"""
        self.Close()
