"""!
@package help.py

@brief Help window

Classes:
 - SearchModuleWindow
 - ItemTree
 - MenuTreeWindow
 - MenuTree
 - AboutWindow
 - InstallExtensionWindow
 - ExtensionTree
 - HelpFrame
 - HelpWindow
 - HelpPanel

(C) 2008-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os

import wx
try:
    import wx.lib.agw.customtreectrl as CT
#    import wx.lib.agw.hyperlink as hl
except ImportError:
    import wx.lib.customtreectrl as CT
#    import wx.lib.hyperlink as hl
import wx.lib.flatnotebook as FN
import  wx.lib.scrolledpanel as scrolled

from grass.script import task as gtask

import menudata
import gcmd
import globalvar
import gdialogs
import utils
import menuform

class HelpFrame(wx.Frame):
    """!GRASS Quickstart help window"""
    def __init__(self, parent, id, title, size, file):
        wx.Frame.__init__(self, parent = parent, id = id, title = title, size = size)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        # text
        content = HelpPanel(parent = self)
        content.LoadPage(file)
        
        sizer.Add(item = content, proportion = 1, flag = wx.EXPAND)
        
        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        self.Layout()

class SearchModuleWindow(wx.Panel):
    """!Search module window (used in MenuTreeWindow)"""
    def __init__(self, parent, id = wx.ID_ANY, cmdPrompt = None,
                 showChoice = True, showTip = False, **kwargs):
        self.showTip    = showTip
        self.showChoice = showChoice
        self.cmdPrompt  = cmdPrompt
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self._searchDict = { _('description') : 'description',
                             _('command')     : 'command',
                             _('keywords')    : 'keywords' }
        
        self.box = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                label = " %s " % _("Find module(s)"))
        
        self.searchBy = wx.Choice(parent = self, id = wx.ID_ANY,
                                  choices = [_('description'),
                                             _('keywords'),
                                             _('command')])
        self.searchBy.SetSelection(0)
        
        self.search = wx.TextCtrl(parent = self, id = wx.ID_ANY,
                                  value = "", size = (-1, 25),
                                  style = wx.TE_PROCESS_ENTER)
        self.search.Bind(wx.EVT_TEXT, self.OnSearchModule)
        
        if self.showTip:
            self.searchTip = gdialogs.StaticWrapText(parent = self, id = wx.ID_ANY,
                                                     size = (-1, 35))
        
        if self.showChoice:
            self.searchChoice = wx.Choice(parent = self, id = wx.ID_ANY)
            if self.cmdPrompt:
                self.searchChoice.SetItems(self.cmdPrompt.GetCommandItems())
            self.searchChoice.Bind(wx.EVT_CHOICE, self.OnSelectModule)
        
        self._layout()

    def _layout(self):
        """!Do layout"""
        sizer = wx.StaticBoxSizer(self.box, wx.HORIZONTAL)
        gridSizer = wx.GridBagSizer(hgap = 3, vgap = 3)
        gridSizer.AddGrowableCol(1)
        
        gridSizer.Add(item = self.searchBy,
                      flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 0))
        gridSizer.Add(item = self.search,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos = (0, 1))
        row = 1
        if self.showTip:
            gridSizer.Add(item = self.searchTip,
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos = (row, 0), span = (1, 2))
            row += 1
        
        if self.showChoice:
            gridSizer.Add(item = self.searchChoice,
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos = (row, 0), span = (1, 2))
        
        sizer.Add(item = gridSizer, proportion = 1)
        
        self.SetSizer(sizer)
        sizer.Fit(self)

    def GetSelection(self):
        """!Get selected element"""
        selection = self.searchBy.GetStringSelection()
        
        return self._searchDict[selection]

    def SetSelection(self, i):
        """!Set selection element"""
        self.searchBy.SetSelection(i)

    def OnSearchModule(self, event):
        """!Search module by keywords or description"""
        if not self.cmdPrompt:
            event.Skip()
            return
        
        text = event.GetString()
        if not text:
            self.cmdPrompt.SetFilter(None)
            mList = self.cmdPrompt.GetCommandItems()
            self.searchChoice.SetItems(mList)
            if self.showTip:
                self.searchTip.SetLabel(_("%d modules found") % len(mList))
            event.Skip()
            return
        
        modules = dict()
        iFound = 0
        for module, data in self.cmdPrompt.moduleDesc.iteritems():
            found = False
            sel = self.searchBy.GetSelection()
            if sel == 0: # -> description
                if text in data['desc']:
                    found = True
            elif sel == 1: # keywords
                if self.cmdPrompt.CheckKey(text, data['keywords']):
                    found = True
            else: # command
                if module[:len(text)] == text:
                    found = True
            
            if found:
                iFound += 1
                try:
                    group, name = module.split('.')
                except ValueError:
                    continue # TODO
                
                if group not in modules:
                    modules[group] = list()
                modules[group].append(name)
                
        self.cmdPrompt.SetFilter(modules)
        self.searchChoice.SetItems(self.cmdPrompt.GetCommandItems())
        if self.showTip:
            self.searchTip.SetLabel(_("%d modules found") % iFound)
        
        event.Skip()
        
    def OnSelectModule(self, event):
        """!Module selected from choice, update command prompt"""
        cmd  = event.GetString().split(' ', 1)[0]
        text = cmd + ' '
        pos = len(text)

        if self.cmdPrompt:
            self.cmdPrompt.SetText(text)
            self.cmdPrompt.SetSelectionStart(pos)
            self.cmdPrompt.SetCurrentPos(pos)
            self.cmdPrompt.SetFocus()
        
        desc = self.cmdPrompt.GetCommandDesc(cmd)
        if self.showTip:
            self.searchTip.SetLabel(desc)
    
    def Reset(self):
        """!Reset widget"""
        self.searchBy.SetSelection(0)
        self.search.SetValue('')
        
class MenuTreeWindow(wx.Panel):
    """!Show menu tree"""
    def __init__(self, parent, id = wx.ID_ANY, **kwargs):
        self.parent = parent # LayerManager
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self.dataBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                    label = " %s " % _("Menu tree (double-click to run command)"))
        # tree
        self.tree = MenuTree(parent = self, data = menudata.ManagerData())
        self.tree.Load()

        # search widget
        self.search = SearchModuleWindow(parent = self, showChoice = False)
        
        # buttons
        self.btnRun   = wx.Button(self, id = wx.ID_OK, label = _("&Run"))
        self.btnRun.SetToolTipString(_("Run selected command"))
        self.btnRun.Enable(False)
        
        # bindings
        self.btnRun.Bind(wx.EVT_BUTTON,            self.OnRun)
        self.tree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.OnItemActivated)
        self.tree.Bind(wx.EVT_TREE_SEL_CHANGED,    self.OnItemSelected)
        self.search.Bind(wx.EVT_TEXT_ENTER,        self.OnShowItem)
        self.search.Bind(wx.EVT_TEXT,              self.OnUpdateStatusBar)
        
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
        self.tree.SearchItems(element = element,
                              value = event.GetString())
        
        nItems = len(self.tree.itemsMarked)
        if event.GetString():
            self.parent.SetStatusText(_("%d modules match") % nItems, 0)
        else:
            self.parent.SetStatusText("", 0)
        
        event.Skip()
        
class ItemTree(CT.CustomTreeCtrl):
    def __init__(self, parent, id = wx.ID_ANY,
                 ctstyle = CT.TR_HIDE_ROOT | CT.TR_FULL_ROW_HIGHLIGHT | CT.TR_HAS_BUTTONS |
                 CT.TR_LINES_AT_ROOT | CT.TR_SINGLE, **kwargs):
        if globalvar.hasAgw:
            super(ItemTree, self).__init__(parent, id, agwStyle = ctstyle, **kwargs)
        else:
            super(ItemTree, self).__init__(parent, id, style = ctstyle, **kwargs)
        
        self.root = self.AddRoot(_("Menu tree"))
        self.itemsMarked = [] # list of marked items
        self.itemSelected = None

    def SearchItems(self, element, value):
        """!Search item 

        @param element element index (see self.searchBy)
        @param value

        @return list of found tree items
        """
        items = list()
        if not value:
            return items
        
        item = self.GetFirstChild(self.root)[0]
        self._processItem(item, element, value, items)
        
        self.itemsMarked  = items
        self.itemSelected = None
        
        return items
    
    def _processItem(self, item, element, value, listOfItems):
        """!Search items (used by SearchItems)
        
        @param item reference item
        @param listOfItems list of found items
        """
        while item and item.IsOk():
            subItem = self.GetFirstChild(item)[0]
            if subItem:
                self._processItem(subItem, element, value, listOfItems)
            data = self.GetPyData(item)
            
            if data and element in data and \
                    value.lower() in data[element].lower():
                listOfItems.append(item)
            
            item = self.GetNextSibling(item)
            
    def GetSelected(self):
        """!Get selected item"""
        return self.itemSelected

    def OnShowItem(self, event):
        """!Highlight first found item in menu tree"""
        if len(self.itemsMarked) > 0:
            if self.GetSelected():
                self.ToggleItemSelection(self.GetSelected())
                idx = self.itemsMarked.index(self.GetSelected()) + 1
            else:
                idx = 0
            try:
                self.ToggleItemSelection(self.itemsMarked[idx])
                self.itemSelected = self.itemsMarked[idx]
                self.EnsureVisible(self.itemsMarked[idx])
            except IndexError:
                self.ToggleItemSelection(self.itemsMarked[0]) # reselect first item
                self.EnsureVisible(self.itemsMarked[0])
                self.itemSelected = self.itemsMarked[0]
        else:
            for item in self.root.GetChildren():
                self.Collapse(item)
            itemSelected = self.GetSelection()
            if itemSelected:
                self.ToggleItemSelection(itemSelected)
            self.itemSelected = None
    
class MenuTree(ItemTree):
    """!Menu tree class"""
    def __init__(self, parent, data, **kwargs):
        self.parent   = parent
        self.menudata = data

        super(MenuTree, self).__init__(parent, **kwargs)
        
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
                    itemNew = self.AppendItem(parentId = item,
                                              text = eachItem[0])
                    
                    data = { 'item'        : eachItem[0],
                             'description' : eachItem[1],
                             'handler'  : eachItem[2],
                             'command'  : eachItem[3],
                             'keywords' : eachItem[4] }
                    
                    self.SetPyData(itemNew, data)
        
class AboutWindow(wx.Frame):
    def __init__(self, parent):
        """!Create custom About Window

        @todo improve styling
        """
        wx.Frame.__init__(self, parent = parent, id = wx.ID_ANY, size = (550,400), 
                          title = _('About GRASS GIS'))
        
        panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        # icon
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))

        # get version and web site
        version, svn_gis_h_rev, svn_gis_h_date = gcmd.RunCommand('g.version',
                                                                 flags = 'r',
                                                                 read = True).splitlines()
        
        infoTxt = wx.Panel(parent = panel, id = wx.ID_ANY)
        infoSizer = wx.BoxSizer(wx.VERTICAL)
        infoGridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        infoGridSizer.AddGrowableCol(0)
        infoGridSizer.AddGrowableCol(1)
        logo = os.path.join(globalvar.ETCDIR, "gui", "icons", "grass.ico")
        logoBitmap = wx.StaticBitmap(parent = infoTxt, id = wx.ID_ANY,
                                     bitmap = wx.Bitmap(name = logo,
                                                        type = wx.BITMAP_TYPE_ICO))
        infoSizer.Add(item = logoBitmap, proportion = 0,
                      flag = wx.ALL | wx.ALIGN_CENTER, border = 25)
        
        info = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                             label = version.replace('GRASS', 'GRASS GIS').strip() + '\n\n')
        info.SetFont(wx.Font(13, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
        infoSizer.Add(item = info, proportion = 0,
                          flag = wx.BOTTOM | wx.ALIGN_CENTER, border = 15)

        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = _('Official GRASS site:')),
                          pos = (0, 0),
                          flag = wx.ALIGN_RIGHT)

        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = 'http://grass.osgeo.org'),
                          pos = (0, 1),
                          flag = wx.ALIGN_LEFT)

        # infoGridSizer.Add(item = hl.HyperLinkCtrl(parent = self, id = wx.ID_ANY,
        #                                           label = 'http://grass.osgeo.org',
        #                                           URL = 'http://grass.osgeo.org'),
        #                   pos = (0, 1),
        #                   flag = wx.LEFT)
        
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = _('GIS Library Revision:')),
                          pos = (2, 0),
                          flag = wx.ALIGN_RIGHT)
        
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = svn_gis_h_rev.split(' ')[2] + ' (' +
                                               svn_gis_h_date.split(' ')[2] + ')'),
                          pos = (2, 1),
                          flag = wx.ALIGN_LEFT)

        infoSizer.Add(item = infoGridSizer,
                      proportion = 1,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL,
                      border = 25)
        
        #
        # create pages
        #
        copyrightwin = self.PageCopyright()
        licensewin   = self.PageLicense()
        authorwin    = self.PageCredit()
        contribwin   = self.PageContributors()
        transwin     = self.PageTranslators()

        # create a flat notebook for displaying information about GRASS
        nbstyle = FN.FNB_VC8 | \
                FN.FNB_BACKGROUND_GRADIENT | \
                FN.FNB_TABS_BORDER_SIMPLE | \
                FN.FNB_NO_X_BUTTON
        
        if globalvar.hasAgw:
            aboutNotebook = FN.FlatNotebook(panel, id = wx.ID_ANY, agwStyle = nbstyle)
        else:
            aboutNotebook = FN.FlatNotebook(panel, id = wx.ID_ANY, style = nbstyle)
        aboutNotebook.SetTabAreaColour(globalvar.FNPageColor)
        
        # make pages for About GRASS notebook
        pg1 = aboutNotebook.AddPage(infoTxt,      text = _("Info"))
        pg2 = aboutNotebook.AddPage(copyrightwin, text = _("Copyright"))
        pg3 = aboutNotebook.AddPage(licensewin,   text = _("License"))
        pg4 = aboutNotebook.AddPage(authorwin,    text = _("Authors"))
        pg5 = aboutNotebook.AddPage(contribwin,   text = _("Contributors"))
        pg5 = aboutNotebook.AddPage(transwin,     text = _("Translators"))
        
        wx.CallAfter(aboutNotebook.SetSelection, 0)
        
        # buttons
        btnClose = wx.Button(parent = panel, id = wx.ID_CLOSE)
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = btnClose, proportion = 0,
                     flag = wx.ALL | wx.ALIGN_RIGHT,
                     border = 5)
        # bindings
        # self.aboutNotebook.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnAGPageChanged)
        btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)

        infoTxt.SetSizer(infoSizer)
        infoSizer.Fit(infoTxt)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(item = aboutNotebook, proportion = 1,
                  flag = wx.EXPAND | wx.ALL, border = 1)
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALL | wx.ALIGN_RIGHT, border = 1)
        panel.SetSizer(sizer)
        self.Layout()
    
    def PageCopyright(self):
        """Copyright information"""
        copyfile = os.path.join(os.getenv("GISBASE"), "COPYING")
        if os.path.exists(copyfile):
            copyrightFile = open(copyfile, 'r')
            copytext = copyrightFile.read()
            copyrightFile.close()
        else:
            copytext = _('%s file missing') % 'COPYING'
        
        # put text into a scrolling panel
        copyrightwin = scrolled.ScrolledPanel(self, id = wx.ID_ANY, 
                                              size = wx.DefaultSize,
                                              style = wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        copyrighttxt = wx.StaticText(copyrightwin, id = wx.ID_ANY, label = copytext)
        copyrightwin.SetAutoLayout(True)
        copyrightwin.sizer = wx.BoxSizer(wx.VERTICAL)
        copyrightwin.sizer.Add(item = copyrighttxt, proportion = 1,
                               flag = wx.EXPAND | wx.ALL, border = 3)
        copyrightwin.SetSizer(copyrightwin.sizer)
        copyrightwin.Layout()
        copyrightwin.SetupScrolling()
        
        return copyrightwin
    
    def PageLicense(self):
        """Licence about"""
        licfile = os.path.join(os.getenv("GISBASE"), "GPL.TXT")
        if os.path.exists(licfile):
            licenceFile = open(licfile, 'r')
            license = ''.join(licenceFile.readlines())
            licenceFile.close()
        else:
            license = _('%s file missing') % 'GPL.TXT'
        # put text into a scrolling panel
        licensewin = scrolled.ScrolledPanel(self, id = wx.ID_ANY, 
                                            style = wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        licensetxt = wx.StaticText(licensewin, id = wx.ID_ANY, label = license)
        licensewin.SetAutoLayout(True)
        licensewin.sizer = wx.BoxSizer(wx.VERTICAL)
        licensewin.sizer.Add(item = licensetxt, proportion = 1,
                flag = wx.EXPAND | wx.ALL, border = 3)
        licensewin.SetSizer(licensewin.sizer)
        licensewin.Layout()
        licensewin.SetupScrolling()
        
        return licensewin
    
    def PageCredit(self):
        """Credit about"""
                # credits
        authfile = os.path.join(os.getenv("GISBASE"), "AUTHORS")
        if os.path.exists(authfile):
            authorsFile = open(authfile, 'r')
            authors = unicode(''.join(authorsFile.readlines()), "utf-8")
            authorsFile.close()
        else:
            authors = _('%s file missing') % 'AUTHORS'
        authorwin = scrolled.ScrolledPanel(self, id = wx.ID_ANY, 
                                           style  =  wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        authortxt = wx.StaticText(authorwin, id = wx.ID_ANY, label = authors)
        authorwin.SetAutoLayout(1)
        authorwin.SetupScrolling()
        authorwin.sizer = wx.BoxSizer(wx.VERTICAL)
        authorwin.sizer.Add(item = authortxt, proportion = 1,
                flag = wx.EXPAND | wx.ALL, border = 3)
        authorwin.SetSizer(authorwin.sizer)
        authorwin.Layout()      
        
        return authorwin

    def PageContributors(self):
        """Contributors info"""
        contribfile = os.path.join(os.getenv("GISBASE"), "contributors.csv")
        if os.path.exists(contribfile):
            contribFile = open(contribfile, 'r')
            contribs = dict()
            errLines = list()
            for line in contribFile.readlines():
                line = line.rstrip('\n')
                try:
                    cvs_id, name, email, country, osgeo_id, rfc2_agreed = line.split(',')
                except ValueError:
                    errLines.append(line)
                    continue
                contribs[osgeo_id] = [name, email, country]
            contribFile.close()
            
            if errLines:
                gcmd.GError(parent = self,
                            message = _("Error when reading file '%s'.") % translatorsfile + \
                                "\n\n" + _("Lines:") + " %s" % \
                                os.linesep.join(map(utils.DecodeString, errLines)))
        else:
            contribs = None
        
        contribwin = scrolled.ScrolledPanel(self, id = wx.ID_ANY, 
                                           style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        contribwin.SetAutoLayout(1)
        contribwin.SetupScrolling()
        contribwin.sizer = wx.BoxSizer(wx.VERTICAL)
        
        if not contribs:
            contribtxt = wx.StaticText(contribwin, id = wx.ID_ANY,
                                       label = _('%s file missing') % 'contibutors.csv')
            contribwin.sizer.Add(item = contribtxt, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        else:
            contribBox = wx.FlexGridSizer(cols = 4, vgap = 5, hgap = 5)
            for item in (_('Name'), _('E-mail'), _('Country'), _('OSGeo_ID')):
                contribBox.Add(item = wx.StaticText(parent = contribwin, id = wx.ID_ANY,
                                                    label = item))
            for osgeo_id in sorted(contribs.keys()):
                for item in contribs[osgeo_id] + [osgeo_id]:
                    contribBox.Add(item = wx.StaticText(parent = contribwin, id = wx.ID_ANY,
                                                        label = item))
            contribwin.sizer.Add(item = contribBox, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        
        contribwin.SetSizer(contribwin.sizer)
        contribwin.Layout()      
        
        return contribwin

    def PageTranslators(self):
        """Translators info"""
        translatorsfile = os.path.join(os.getenv("GISBASE"), "translators.csv")
        if os.path.exists(translatorsfile):
            translatorsFile = open(translatorsfile, 'r')
            translators = dict()
            errLines = list()
            for line in translatorsFile.readlines()[1:]:
                line = line.rstrip('\n')
                try:
                    name, email, languages = line.split(',')
                except ValueError:
                    errLines.append(line)
                    continue
                for language in languages.split(' '):
                    if language not in translators:
                        translators[language] = list()
                    translators[language].append((name, email))
            translatorsFile.close()
            
            if errLines:
                gcmd.GError(parent = self,
                            message = _("Error when reading file '%s'.") % translatorsfile + \
                                "\n\n" + _("Lines:") + " %s" % \
                                os.linesep.join(map(utils.DecodeString, errLines)))
        else:
            translators = None
        
        translatorswin = scrolled.ScrolledPanel(self, id = wx.ID_ANY, 
                                           style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        translatorswin.SetAutoLayout(1)
        translatorswin.SetupScrolling()
        translatorswin.sizer = wx.BoxSizer(wx.VERTICAL)
        
        if not translators:
            translatorstxt = wx.StaticText(translatorswin, id = wx.ID_ANY,
                                           label = _('%s file missing') % 'translators.csv')
            translatorswin.sizer.Add(item = translatorstxt, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        else:
            translatorsBox = wx.FlexGridSizer(cols = 3, vgap = 5, hgap = 5)
            languages = translators.keys()
            languages.sort()
            translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                    label = _('Name')))
            translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                    label = _('E-mail')))
            translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                    label = _('Language')))
            for lang in languages:
                for translator in translators[lang]:
                    name, email = translator
                    translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                            label =  unicode(name, "utf-8")))
                    translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                            label = email))
                    translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                            label = lang))
            
            translatorswin.sizer.Add(item = translatorsBox, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        
        translatorswin.SetSizer(translatorswin.sizer)
        translatorswin.Layout()      
        
        return translatorswin
    
    def OnCloseWindow(self, event):
        """!Close window"""
        self.Close()

class InstallExtensionWindow(wx.Frame):
    def __init__(self, parent, id = wx.ID_ANY,
                 title = _("Fetch & install extension from GRASS Addons"), **kwargs):
        self.parent = parent
        self.options = dict() # list of options
        
        wx.Frame.__init__(self, parent = parent, id = id, title = title, **kwargs)
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)

        self.repoBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                    label = " %s " % _("Repository"))
        self.treeBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                    label = " %s " % _("List of extensions"))
        
        self.repo = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY)
        self.fullDesc = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                    label = _("Fetch full info including description and keywords (takes time)"))
        self.fullDesc.SetValue(False)
        
        self.search = SearchModuleWindow(parent = self.panel)
        self.search.SetSelection(2) 
        
        self.tree   = ExtensionTree(parent = self.panel, log = parent.GetLogWindow())
        
        self.optionBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                      label = " %s " % _("Options"))
        task = gtask.parse_interface('g.extension')
        for f in task.get_options()['flags']:
            name = f.get('name', '')
            desc = f.get('label', '')
            if not desc:
                desc = f.get('description', '')
            if not name and not desc:
                continue
            if name in ('l', 'f', 'g', 'quiet', 'verbose'):
                continue
            self.options[name] = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                             label = desc)
        self.repo.SetValue(task.get_param(value = 'svnurl').get('default',
                                                                'https://svn.osgeo.org/grass/grass-addons'))
        
        self.statusbar = self.CreateStatusBar(number = 1)
        
        self.btnFetch = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                  label = _("&Fetch"))
        self.btnFetch.SetToolTipString(_("Fetch list of available modules from GRASS Addons SVN repository"))
        self.btnClose = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btnInstall = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                    label = _("&Install"))
        self.btnInstall.SetToolTipString(_("Install selected add-ons GRASS module"))
        self.btnInstall.Enable(False)
        self.btnCmd = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                label = _("Command dialog"))
        self.btnCmd.SetToolTipString(_('Open %s dialog') % 'g.extension')

        self.btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)
        self.btnFetch.Bind(wx.EVT_BUTTON, self.OnFetch)
        self.btnInstall.Bind(wx.EVT_BUTTON, self.OnInstall)
        self.btnCmd.Bind(wx.EVT_BUTTON, self.OnCmdDialog)
        self.tree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.OnItemActivated)
        self.tree.Bind(wx.EVT_TREE_SEL_CHANGED,    self.OnItemSelected)
        self.search.Bind(wx.EVT_TEXT_ENTER,        self.OnShowItem)
        self.search.Bind(wx.EVT_TEXT,              self.OnUpdateStatusBar)

        self._layout()

    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        repoSizer = wx.StaticBoxSizer(self.repoBox, wx.VERTICAL)
        repo1Sizer = wx.BoxSizer(wx.HORIZONTAL)
        repo1Sizer.Add(item = self.repo, proportion = 1,
                      flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL, border = 1)
        repo1Sizer.Add(item = self.btnFetch, proportion = 0,
                      flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL, border = 1)
        repoSizer.Add(item = repo1Sizer,
                      flag = wx.EXPAND)
        repoSizer.Add(item = self.fullDesc)
        
        findSizer = wx.BoxSizer(wx.HORIZONTAL)
        findSizer.Add(item = self.search, proportion = 1)
        
        treeSizer = wx.StaticBoxSizer(self.treeBox, wx.HORIZONTAL)
        treeSizer.Add(item = self.tree, proportion = 1,
                      flag = wx.ALL | wx.EXPAND, border = 1)

        # options
        optionSizer = wx.StaticBoxSizer(self.optionBox, wx.VERTICAL)
        for key in self.options.keys():
            optionSizer.Add(item = self.options[key], proportion = 0)
        
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = self.btnCmd, proportion = 0,
                     flag = wx.RIGHT, border = 5)
        btnSizer.AddSpacer(10)
        btnSizer.Add(item = self.btnClose, proportion = 0,
                     flag = wx.RIGHT, border = 5)
        btnSizer.Add(item = self.btnInstall, proportion = 0)
        
        sizer.Add(item = repoSizer, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        sizer.Add(item = findSizer, proportion = 0,
                  flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        sizer.Add(item = treeSizer, proportion = 1,
                  flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        sizer.Add(item = optionSizer, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        
        self.Layout()

    def _getCmd(self):
        item = self.tree.GetSelected()
        if not item or not item.IsOk():
            return ['g.extension']
        
        name = self.tree.GetItemText(item)
        if not name:
            gcmd.GError(_("Extension not defined"), parent = self)
            return
        
        flags = list()
        for key in self.options.keys():
            if self.options[key].IsChecked():
                flags.append('-%s' % key)
        
        return ['g.extension'] + flags + ['extension=' + name,
                                          'svnurl=' + self.repo.GetValue().strip()]
    
    def OnUpdateStatusBar(self, event):
        """!Update statusbar text"""
        element = self.search.GetSelection()
        if not self.tree.IsLoaded():
            self.SetStatusText(_("Fetch list of available extensions by clicking on 'Fetch' button"), 0)
            return
        
        self.tree.SearchItems(element = element,
                              value = event.GetString())
        
        nItems = len(self.tree.itemsMarked)
        if event.GetString():
            self.SetStatusText(_("%d items match") % nItems, 0)
        else:
            self.SetStatusText("", 0)
        
        event.Skip()
    
    def OnCloseWindow(self, event):
        """!Close window"""
        self.Destroy()

    def OnFetch(self, event):
        """!Fetch list of available extensions"""
        self.SetStatusText(_("Fetching list of modules from GRASS-Addons SVN (be patient)..."), 0)
        self.tree.Load(url = self.repo.GetValue().strip(), full = self.fullDesc.IsChecked())
        self.SetStatusText("", 0)

    def OnItemActivated(self, event):
        item = event.GetItem()
        data = self.tree.GetPyData(item)
        if data and 'command' in data:
            self.OnInstall(event = None)
        
    def OnInstall(self, event):
        """!Install selected extension"""
        log = self.parent.GetLogWindow()
        log.RunCmd(self._getCmd())
        
        ### self.OnCloseWindow(None)
                
    def OnItemSelected(self, event):
        """!Item selected"""
        item = event.GetItem()
        self.tree.itemSelected = item
        data = self.tree.GetPyData(item)
        if not data:
            self.SetStatusText('', 0)
            self.btnInstall.Enable(False)
        else:
            self.SetStatusText(data.get('description', ''), 0)
            self.btnInstall.Enable(True)

    def OnShowItem(self, event):
        """!Show selected item"""
        self.tree.OnShowItem(event)
        if self.tree.GetSelected():
            self.btnInstall.Enable()
        else:
            self.btnInstall.Enable(False)

    def OnCmdDialog(self, event):
        """!Shows command dialog"""
        menuform.GUI(parent = self).ParseCommand(cmd = self._getCmd())
        
class ExtensionTree(ItemTree):
    """!List of available extensions"""
    def __init__(self, parent, log, id = wx.ID_ANY,
                 ctstyle = CT.TR_HIDE_ROOT | CT.TR_FULL_ROW_HIGHLIGHT | CT.TR_HAS_BUTTONS |
                 CT.TR_LINES_AT_ROOT | CT.TR_SINGLE,
                 **kwargs):
        self.parent = parent # GMFrame
        self.log    = log
        
        super(ExtensionTree, self).__init__(parent, id, ctstyle = ctstyle, **kwargs)
        
        self._initTree()
        
    def _initTree(self):
        for prefix in ('display', 'database',
                       'general', 'imagery',
                       'misc', 'postscript', 'paint',
                       'raster', 'raster3D', 'sites', 'vector', 'wxGUI'):
            self.AppendItem(parentId = self.root,
                            text = prefix)
        self._loaded = False
        
    def _expandPrefix(self, c):
        name = { 'd'  : 'display',
                 'db' : 'database',
                 'g'  : 'general',
                 'i'  : 'imagery',
                 'm'  : 'misc',
                 'ps' : 'postscript',
                 'p'  : 'paint',
                 'r'  : 'raster',
                 'r3' : 'raster3D',
                 's'  : 'sites',
                 'v'  : 'vector',
                 'wx' : 'wxGUI' }
        
        if c in name:
            return name[c]
        
        return c
    
    def _findItem(self, text):
        """!Find item"""
        item = self.GetFirstChild(self.root)[0]
        while item and item.IsOk():
            if text == self.GetItemText(item):
                return item
            
            item = self.GetNextSibling(item)
        
        return None
    
    def Load(self, url, full = False):
        """!Load list of extensions"""
        self.DeleteAllItems()
        self.root = self.AddRoot(_("Menu tree"))
        self._initTree()
        
        if full:
            flags = 'g'
        else:
            flags = 'l'
        ret = gcmd.RunCommand('g.extension', read = True,
                              svnurl = url,
                              flags = flags, quiet = True)
        if not ret:
            return
        
        mdict = dict()
        for line in ret.splitlines():
            if full:
                key, value = line.split('=', 1)
                if key == 'name':
                    prefix, name = value.split('.', 1)
                    if prefix not in mdict:
                        mdict[prefix] = dict()
                    mdict[prefix][name] = dict()
                else:
                    mdict[prefix][name][key] = value
            else:
                try:
                    prefix, name = line.strip().split('.', 1)
                except:
                    prefix = 'unknown'
                    name = line.strip()
                
                if self._expandPrefix(prefix) == prefix:
                    prefix = 'unknown'
                    
                if prefix not in mdict:
                    mdict[prefix] = dict()
                    
                mdict[prefix][name] = { 'command' : prefix + '.' + name }
        
        for prefix in mdict.keys():
            prefixName = self._expandPrefix(prefix)
            item = self._findItem(prefixName)
            names = mdict[prefix].keys()
            names.sort()
            for name in names:
                new = self.AppendItem(parentId = item,
                                      text = prefix + '.' + name)
                data = dict()
                for key in mdict[prefix][name].keys():
                    data[key] = mdict[prefix][name][key]
                
                self.SetPyData(new, data)
        
        self._loaded = True

    def IsLoaded(self):
        """Check if items are loaded"""
        return self._loaded

class HelpWindow(wx.html.HtmlWindow):
    """!This panel holds the text from GRASS docs.
    
    GISBASE must be set in the environment to find the html docs dir.
    The SYNOPSIS section is skipped, since this Panel is supposed to
    be integrated into the cmdPanel and options are obvious there.
    """
    def __init__(self, parent, grass_command, text, skip_description,
                 **kwargs):
        """!If grass_command is given, the corresponding HTML help
        file will be presented, with all links pointing to absolute
        paths of local files.

        If 'skip_description' is True, the HTML corresponding to
        SYNOPSIS will be skipped, thus only presenting the help file
        from the DESCRIPTION section onwards.

        If 'text' is given, it must be the HTML text to be presented
        in the Panel.
        """
        self.parent = parent
        wx.InitAllImageHandlers()
        wx.html.HtmlWindow.__init__(self, parent = parent, **kwargs)
        
        gisbase = os.getenv("GISBASE")
        self.loaded = False
        self.history = list()
        self.historyIdx = 0
        self.fspath = os.path.join(gisbase, "docs", "html")
        
        self.SetStandardFonts (size = 10)
        self.SetBorders(10)
        
        if text is None:
            if skip_description:
                url = os.path.join(self.fspath, grass_command + ".html")
                self.fillContentsFromFile(url,
                                          skip_description = skip_description)
                self.history.append(url)
                self.loaded = True
            else:
                ### FIXME: calling LoadPage() is strangely time-consuming (only first call)
                # self.LoadPage(self.fspath + grass_command + ".html")
                self.loaded = False
        else:
            self.SetPage(text)
            self.loaded = True
        
    def OnLinkClicked(self, linkinfo):
        url = linkinfo.GetHref()
        if url[:4] != 'http':
            url = os.path.join(self.fspath, url)
        self.history.append(url)
        self.historyIdx += 1
        self.parent.OnHistory()
        
        super(HelpWindow, self).OnLinkClicked(linkinfo)
        
    def fillContentsFromFile(self, htmlFile, skip_description = True):
        """!Load content from file"""
        aLink = re.compile(r'(<a href="?)(.+\.html?["\s]*>)', re.IGNORECASE)
        imgLink = re.compile(r'(<img src="?)(.+\.[png|gif])', re.IGNORECASE)
        try:
            contents = []
            skip = False
            for l in file(htmlFile, "rb").readlines():
                if "DESCRIPTION" in l:
                    skip = False
                if not skip:
                    # do skip the options description if requested
                    if "SYNOPSIS" in l:
                        skip = skip_description
                    else:
                        # FIXME: find only first item
                        findALink = aLink.search(l)
                        if findALink is not None: 
                            contents.append(aLink.sub(findALink.group(1)+
                                                      self.fspath+findALink.group(2),l))
                        findImgLink = imgLink.search(l)
                        if findImgLink is not None: 
                            contents.append(imgLink.sub(findImgLink.group(1)+
                                                        self.fspath+findImgLink.group(2),l))
                        
                        if findALink is None and findImgLink is None:
                            contents.append(l)
            self.SetPage("".join(contents))
            self.loaded = True
        except: # The Manual file was not found
            self.loaded = False
        
class HelpPanel(wx.Panel):
    def __init__(self, parent, grass_command = "index", text = None,
                 skip_description = False, **kwargs):
        self.grass_command = grass_command
        wx.Panel.__init__(self, parent = parent, id = wx.ID_ANY)
        
        self.content = HelpWindow(self, grass_command, text,
                                  skip_description)
        
        self.btnNext = wx.Button(parent = self, id = wx.ID_ANY,
                                 label = _("&Next"))
        self.btnNext.Enable(False)
        self.btnPrev = wx.Button(parent = self, id = wx.ID_ANY,
                                 label = _("&Previous"))
        self.btnPrev.Enable(False)
        
        self.btnNext.Bind(wx.EVT_BUTTON, self.OnNext)
        self.btnPrev.Bind(wx.EVT_BUTTON, self.OnPrev)
        
        self._layout()

    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        btnSizer.Add(item = self.btnPrev, proportion = 0,
                     flag = wx.ALL, border = 5)
        btnSizer.Add(item = wx.Size(1, 1), proportion = 1)
        btnSizer.Add(item = self.btnNext, proportion = 0,
                     flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        sizer.Add(item = self.content, proportion = 1,
                  flag = wx.EXPAND)
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.EXPAND)
        
        self.SetSizer(sizer)
        sizer.Fit(self)

    def LoadPage(self, path = None):
        """!Load page"""
        if not path:
            path = os.path.join(self.content.fspath, self.grass_command + ".html")
        self.content.history.append(path)
        self.content.LoadPage(path)
        
    def IsFile(self):
        """!Check if file exists"""
        return os.path.isfile(os.path.join(self.content.fspath, self.grass_command + ".html"))

    def IsLoaded(self):
        return self.content.loaded

    def OnHistory(self):
        """!Update buttons"""
        nH = len(self.content.history)
        iH = self.content.historyIdx
        if iH == nH - 1:
            self.btnNext.Enable(False)
        elif iH > -1:
            self.btnNext.Enable(True)
        if iH < 1:
            self.btnPrev.Enable(False)
        else:
            self.btnPrev.Enable(True)

    def OnNext(self, event):
        """Load next page"""
        self.content.historyIdx += 1
        idx = self.content.historyIdx
        path = self.content.history[idx]
        self.content.LoadPage(path)
        self.OnHistory()
        
        event.Skip()
        
    def OnPrev(self, event):
        """Load previous page"""
        self.content.historyIdx -= 1
        idx = self.content.historyIdx
        path = self.content.history[idx]
        self.content.LoadPage(path)
        self.OnHistory()
        
        event.Skip()
