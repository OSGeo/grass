"""!
@package help.py

@brief Help window

Classes:
 - HelpWindow
 - SearchModuleWindow
 - ItemTree
 - MenuTreeWindow
 - MenuTree
 - AboutWindow
 - InstallExtensionWindow
 - ExtensionTree

(C) 2008-2010 by the GRASS Development Team
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

class SearchModuleWindow(wx.Panel):
    """!Search module window (used in MenuTreeWindow)"""
    def __init__(self, parent, id = wx.ID_ANY, showLabel = True, **kwargs):
        self.showLabel = showLabel
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self._searchDict = { _('description') : 'description',
                             _('command')     : 'command',
                             _('keywords')    : 'keywords' }
        
        self.searchBy = wx.Choice(parent = self, id = wx.ID_ANY,
                                  choices = [_('description'),
                                             _('keywords'),
                                             _('command')])
        self.searchBy.SetSelection(0)
        
        self.search = wx.TextCtrl(parent = self, id = wx.ID_ANY,
                                  value = "", size = (-1, 25),
                                  style = wx.TE_PROCESS_ENTER)
        
        self._layout()

    def _layout(self):
        """!Do layout"""
                # search
        sizer = wx.BoxSizer(wx.HORIZONTAL)
        if self.showLabel:
            sizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                           label = _("Find module by:")),
                      proportion = 0,
                      flag = wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                      border = 3)
        sizer.Add(item = self.searchBy, proportion = 0,
                  flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT | wx.RIGHT,
                  border = 5)
        sizer.Add(item = self.search, proportion = 1,
                  flag = wx.EXPAND | wx.RIGHT | wx.ALIGN_CENTER_VERTICAL,
                  border = 5)
        
        self.SetSizer(sizer)
        sizer.Fit(self)

    def GetSelection(self):
        """!Get selected element"""
        selection = self.searchBy.GetStringSelection()
        
        return self._searchDict[selection]
    
class MenuTreeWindow(wx.Panel):
    """!Show menu tree"""
    def __init__(self, parent, id = wx.ID_ANY, **kwargs):
        self.parent = parent # LayerManager
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self.dataBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                    label=" %s " % _("Menu tree (double-click to run command)"))
        # tree
        self.tree = MenuTree(parent = self, data = menudata.ManagerData())
        self.tree.Load()

        # search widget
        self.search = SearchModuleWindow(parent = self)
        
        # buttons
        self.btnRun   = wx.Button(self, id = wx.ID_OK, label = _("Run"))
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
        btnSizer.Add(item = self.btnRun, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        
        sizer.Add(item = dataSizer, proportion = 1,
                  flag = wx.EXPAND | wx.ALL, border = 5)

        sizer.Add(item = self.search, proportion=0,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        sizer.Add(item = btnSizer, proportion=0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
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
                          caption = _('Message'), style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
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
            self.parent.SetStatusText(_("%d items match") % nItems, 0)
        else:
            self.parent.SetStatusText("", 0)
        
        event.Skip()
        
class ItemTree(CT.CustomTreeCtrl):
    def __init__(self, parent, id = wx.ID_ANY,
                 ctstyle = CT.TR_HIDE_ROOT | CT.TR_FULL_ROW_HIGHLIGHT | CT.TR_HAS_BUTTONS |
                 CT.TR_LINES_AT_ROOT | CT.TR_SINGLE, **kwargs):
        super(ItemTree, self).__init__(parent, id, ctstyle = ctstyle, **kwargs)
        
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
            
            if data and data.has_key(element) and \
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
        wx.Frame.__init__(self, parent=parent, id=wx.ID_ANY, size=(550,400), 
                          title=_('About GRASS GIS'))
        
        panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        # icon
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))

        # get version and web site
        version, svn_gis_h_rev, svn_gis_h_date = gcmd.RunCommand('g.version',
                                                                 flags = 'r',
                                                                 read = True).splitlines()
        
        infoTxt = wx.Panel(parent = panel, id = wx.ID_ANY)
        infoSizer = wx.BoxSizer(wx.VERTICAL)
        infoGridSizer = wx.GridBagSizer(vgap=5, hgap=5)
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
                                               label = svn_gis_h_rev.split(' ')[1] + ' (' +
                                               svn_gis_h_date.split(' ')[1] + ')'),
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
        
        aboutNotebook = FN.FlatNotebook(panel, id=wx.ID_ANY, style=nbstyle)
        aboutNotebook.SetTabAreaColour(globalvar.FNPageColor)
        
        # make pages for About GRASS notebook
        pg1 = aboutNotebook.AddPage(infoTxt,      text=_("Info"))
        pg2 = aboutNotebook.AddPage(copyrightwin, text=_("Copyright"))
        pg3 = aboutNotebook.AddPage(licensewin,   text=_("License"))
        pg4 = aboutNotebook.AddPage(authorwin,    text=_("Authors"))
        pg5 = aboutNotebook.AddPage(contribwin,   text=_("Contributors"))
        pg5 = aboutNotebook.AddPage(transwin,     text=_("Translators"))
        
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
        sizer.Add(item=aboutNotebook, proportion=1,
                  flag=wx.EXPAND | wx.ALL, border=1)
        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.ALL | wx.ALIGN_RIGHT, border=1)
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
        copyrightwin = scrolled.ScrolledPanel(self, id=wx.ID_ANY, 
                                              size=wx.DefaultSize,
                                              style = wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        copyrighttxt = wx.StaticText(copyrightwin, id=wx.ID_ANY, label=copytext)
        copyrightwin.SetAutoLayout(True)
        copyrightwin.sizer = wx.BoxSizer(wx.VERTICAL)
        copyrightwin.sizer.Add(item=copyrighttxt, proportion=1,
                               flag=wx.EXPAND | wx.ALL, border=3)
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
        licensewin = scrolled.ScrolledPanel(self, id=wx.ID_ANY, 
                                            style = wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        licensetxt = wx.StaticText(licensewin, id=wx.ID_ANY, label=license)
        licensewin.SetAutoLayout(True)
        licensewin.sizer = wx.BoxSizer(wx.VERTICAL)
        licensewin.sizer.Add(item=licensetxt, proportion=1,
                flag=wx.EXPAND | wx.ALL, border=3)
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
        authorwin = scrolled.ScrolledPanel(self, id=wx.ID_ANY, 
                                           style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        authortxt = wx.StaticText(authorwin, id=wx.ID_ANY, label=authors)
        authorwin.SetAutoLayout(1)
        authorwin.SetupScrolling()
        authorwin.sizer = wx.BoxSizer(wx.VERTICAL)
        authorwin.sizer.Add(item=authortxt, proportion=1,
                flag=wx.EXPAND | wx.ALL, border=3)
        authorwin.SetSizer(authorwin.sizer)
        authorwin.Layout()      
        
        return authorwin

    def PageContributors(self):
        """Contributors info"""
        contribfile = os.path.join(os.getenv("GISBASE"), "contributors.csv")
        if os.path.exists(contribfile):
            contribFile = open(contribfile, 'r')
            contribs = list()
            for line in contribFile.readlines():
                cvs_id, name, email, country, osgeo_id, rfc2_agreed = line.split(',')
                contribs.append((name, email, country, osgeo_id))
            contribs[0] = (_('Name'), _('E-mail'), _('Country'), _('OSGeo_ID'))
            contribFile.close()
        else:
            contribs = None
        
        contribwin = scrolled.ScrolledPanel(self, id=wx.ID_ANY, 
                                           style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        contribwin.SetAutoLayout(1)
        contribwin.SetupScrolling()
        contribwin.sizer = wx.BoxSizer(wx.VERTICAL)
        
        if not contribs:
            contribtxt = wx.StaticText(contribwin, id=wx.ID_ANY,
                                       label=_('%s file missing') % 'contibutors.csv')
            contribwin.sizer.Add(item=contribtxt, proportion=1,
                                 flag=wx.EXPAND | wx.ALL, border=3)
        else:
            contribBox = wx.FlexGridSizer(cols=4, vgap=5, hgap=5)
            for developer in contribs:
                for item in developer:
                    contribBox.Add(item = wx.StaticText(parent = contribwin, id = wx.ID_ANY,
                                                        label = item))
            contribwin.sizer.Add(item=contribBox, proportion=1,
                                 flag=wx.EXPAND | wx.ALL, border=3)
        
        contribwin.SetSizer(contribwin.sizer)
        contribwin.Layout()      
        
        return contribwin

    def PageTranslators(self):
        """Translators info"""
        translatorsfile = os.path.join(os.getenv("GISBASE"), "translators.csv")
        if os.path.exists(translatorsfile):
            translatorsFile = open(translatorsfile, 'r')
            translators = dict()
            for line in translatorsFile.readlines()[1:]:
                name, email, languages = line.rstrip('\n').split(',')
                for language in languages.split(' '):
                    if not translators.has_key(language):
                        translators[language] = list()
                    translators[language].append((name, email))
            translatorsFile.close()
        else:
            translators = None
        
        translatorswin = scrolled.ScrolledPanel(self, id=wx.ID_ANY, 
                                           style = wx.TAB_TRAVERSAL|wx.SUNKEN_BORDER)
        translatorswin.SetAutoLayout(1)
        translatorswin.SetupScrolling()
        translatorswin.sizer = wx.BoxSizer(wx.VERTICAL)
        
        if not translators:
            translatorstxt = wx.StaticText(translatorswin, id=wx.ID_ANY,
                                           label=_('%s file missing') % 'translators.csv')
            translatorswin.sizer.Add(item=translatorstxt, proportion=1,
                                 flag=wx.EXPAND | wx.ALL, border=3)
        else:
            translatorsBox = wx.FlexGridSizer(cols=3, vgap=5, hgap=5)
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
            
            translatorswin.sizer.Add(item=translatorsBox, proportion=1,
                                 flag=wx.EXPAND | wx.ALL, border=3)
        
        translatorswin.SetSizer(translatorswin.sizer)
        translatorswin.Layout()      
        
        return translatorswin
    
    def OnCloseWindow(self, event):
        """!Close window"""
        self.Close()

class InstallExtensionWindow(wx.Frame):
    def __init__(self, parent, id = wx.ID_ANY,
                 title = _("Fetch & install new extension from GRASS Addons"), **kwargs):
        self.parent = parent
        
        wx.Frame.__init__(self, parent = parent, id = id, title = title, **kwargs)
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)

        self.findBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                    label=" %s " % _("Find extension by"))
        self.treeBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                    label=" %s " % _("List of extensions"))
        
        self.search = SearchModuleWindow(parent = self.panel, showLabel = False)
        
        self.tree   = ExtensionTree(parent = self.panel, log = parent.GetLogWindow())
        
        self.statusbar = self.CreateStatusBar(0)
        
        self.btnFetch = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                  label = _("&Fetch"))
        self.btnFetch.SetToolTipString(_("Fetch list of available modules from GRASS Addons SVN repository"))
        self.btnClose = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btnInstall = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                    label = _("&Install"))
        self.btnInstall.SetToolTipString(_("Install selected add-ons GRASS module"))
        self.btnInstall.Enable(False)
        
        self.btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)
        self.btnFetch.Bind(wx.EVT_BUTTON, self.OnFetch)
        self.btnInstall.Bind(wx.EVT_BUTTON, self.OnInstall)
        self.tree.Bind(wx.EVT_TREE_ITEM_ACTIVATED, self.OnItemActivated)
        self.tree.Bind(wx.EVT_TREE_SEL_CHANGED,    self.OnItemSelected)
        self.search.Bind(wx.EVT_TEXT_ENTER,        self.OnShowItem)
        self.search.Bind(wx.EVT_TEXT,              self.OnUpdateStatusBar)

        self._layout()

    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        findSizer = wx.StaticBoxSizer(self.findBox, wx.HORIZONTAL)
        findSizer.Add(item = self.search, proportion = 1,
                      flag = wx.ALL, border = 1)
        findSizer.Add(item = self.btnFetch, proportion = 0,
                      flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL, border = 1)
        
        treeSizer = wx.StaticBoxSizer(self.treeBox, wx.HORIZONTAL)
        treeSizer.Add(item = self.tree, proportion = 1,
                      flag = wx.ALL | wx.EXPAND, border = 1)
        
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = self.btnClose, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        btnSizer.Add(item = self.btnInstall, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT, border = 5)
        
        sizer.Add(item = findSizer, proportion = 0,
                  flag = wx.ALL | wx.EXPAND, border = 3)
        sizer.Add(item = treeSizer, proportion = 1,
                  flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 3)
        sizer.Add(item = btnSizer, proportion=0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        
        self.Layout()

    def _install(self, name):
        if not name:
            return
        log = self.parent.GetLogWindow()
        log.RunCmd(['g.extension', 'extension=' + name])
        self.OnCloseWindow(None)
        
    def OnUpdateStatusBar(self, event):
        """!Update statusbar text"""
        element = self.search.GetSelection()
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
        self.tree.Load()
        self.SetStatusText("", 0)

    def OnItemActivated(self, event):
        item = event.GetItem()
        data = self.tree.GetPyData(item)
        if data and data.has_key('command'):
            self._install(data['command'])
            
    def OnInstall(self, event):
        """!Install selected extension"""
        item = self.tree.GetSelected()
        if not item.IsOk():
            return
        self._install(self.tree.GetItemText(item))
        
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
                       'raster', 'raster3D', 'sites', 'vector'):
            self.AppendItem(parentId = self.root,
                            text = prefix)
        
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
                 'v'  : 'vector' }
        
        if name.has_key(c):
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
    
    def Load(self):
        """!Load list of extensions"""
        self.DeleteAllItems()
        self.root = self.AddRoot(_("Menu tree"))
        self._initTree()
        
        ret = gcmd.RunCommand('g.extension', read = True,
                                 flags = 'g', quiet = True)
        if not ret:
            return
        
        mdict = dict()
        for line in ret.splitlines():
            key, value = line.split('=', 1)
            if key == 'name':
                prefix, name = value.split('.', 1)
                if not mdict.has_key(prefix):
                    mdict[prefix] = dict()
                mdict[prefix][name] = dict()
            else:
                mdict[prefix][name][key] = value
                
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
        
