"""!
@package gui_core.ghelp

@brief Help/about window, menu tree, search module tree

Classes:
 - ghelp::SearchModuleWindow
 - ghelp::MenuTreeWindow
 - ghelp::MenuTree
 - ghelp::AboutWindow
 - ghelp::HelpFrame
 - ghelp::HelpWindow
 - ghelp::HelpPanel

(C) 2008-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import codecs
import platform

import wx
from wx.html import HtmlWindow
try:
    import wx.lib.agw.customtreectrl as CT
    from wx.lib.agw.hyperlink import HyperLinkCtrl
except ImportError:
    import wx.lib.customtreectrl as CT
    from wx.lib.hyperlink import HyperLinkCtrl
import wx.lib.flatnotebook as FN

import grass.script as grass

from core             import globalvar
from core             import utils
from lmgr.menudata    import ManagerData
from core.gcmd        import GError, DecodeString
from gui_core.widgets import FormListbook, StaticWrapText, ItemTree, ScrolledPanel
from core.debug       import Debug
from core.settings    import UserSettings

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
            self.searchTip = StaticWrapText(parent = self, id = wx.ID_ANY,
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
        
        gridSizer.AddGrowableCol(1)

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
                if text in ','.join(data['keywords']):
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
        if self.showTip:
            self.searchTip.SetLabel('')
        
class MenuTreeWindow(wx.Panel):
    """!Show menu tree"""
    def __init__(self, parent, id = wx.ID_ANY, **kwargs):
        self.parent = parent # LayerManager
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self.dataBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                    label = " %s " % _("Menu tree (double-click to run command)"))
        # tree
        self.tree = MenuTree(parent = self, data = ManagerData())
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
        
class AboutWindow(wx.Frame):
    """!Create custom About Window
    """
    def __init__(self, parent, size = (650, 460), 
                 title = _('About GRASS GIS'), **kwargs):
        wx.Frame.__init__(self, parent = parent, id = wx.ID_ANY, title = title, size = size, **kwargs)
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        # icon
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))

        # notebook
        self.aboutNotebook = FormListbook(self.panel, style = wx.BK_LEFT)
        
        for title, win in ((_("Info"), self._pageInfo()),
                           (_("Copyright"), self._pageCopyright()),
                           (_("License"), self._pageLicense()),
                           (_("Authors"), self._pageCredit()),
                           (_("Contributors"), self._pageContributors()),
                           (_("Extra contributors"), self._pageContributors(extra = True)),
                           (_("Translators"), self._pageTranslators()),
                           (_("Translation status"), self._pageStats())):
            self.aboutNotebook.AddPage(page = win, text = title)
        self.aboutNotebook.Refresh()
        wx.CallAfter(self.aboutNotebook.SetSelection, 0)
        
        # buttons
        self.btnClose = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btnClose.Bind(wx.EVT_BUTTON, self.OnCloseWindow)

        self._doLayout()
        
    def _doLayout(self):
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item = self.btnClose, proportion = 0,
                     flag = wx.ALL | wx.ALIGN_RIGHT,
                     border = 5)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(item = self.aboutNotebook, proportion = 1,
                  flag = wx.EXPAND | wx.ALL, border = 1)
        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALL | wx.ALIGN_RIGHT, border = 1)
        
        self.SetMinSize((400, 400))
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
                
        self.Layout()
        
    def _pageInfo(self):
        """!Info page"""
        # get version and web site
        vInfo = grass.version()
        
        infoTxt = ScrolledPanel(self.aboutNotebook)
        infoTxt.SetBackgroundColour('WHITE')
        infoTxt.SetupScrolling()
        infoSizer = wx.BoxSizer(wx.VERTICAL)
        infoGridSizer = wx.GridBagSizer(vgap = 5, hgap = 5)
        infoGridSizer.AddGrowableCol(0)
        infoGridSizer.AddGrowableCol(1)
        logo = os.path.join(globalvar.ETCDIR, "gui", "icons", "grass-64x64.png")
        logoBitmap = wx.StaticBitmap(parent = infoTxt, id = wx.ID_ANY,
                                     bitmap = wx.Bitmap(name = logo,
                                                        type = wx.BITMAP_TYPE_PNG))
        infoSizer.Add(item = logoBitmap, proportion = 0,
                      flag = wx.ALL | wx.ALIGN_CENTER, border = 20)
        
        info = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                             label = 'GRASS GIS ' + vInfo['version'] + '\n\n')
        info.SetFont(wx.Font(13, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
        info.SetForegroundColour(wx.Colour(35, 142, 35))
        infoSizer.Add(item = info, proportion = 0,
                      flag = wx.BOTTOM | wx.ALIGN_CENTER, border = 1)
        
        row = 0
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = _('Official GRASS site:')),
                          pos = (row, 0),
                          flag = wx.ALIGN_RIGHT)

        infoGridSizer.Add(item = HyperLinkCtrl(parent = infoTxt, id = wx.ID_ANY,
                                               label = 'http://grass.osgeo.org'),
                          pos = (row, 1),
                          flag = wx.ALIGN_LEFT)

        row += 2
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = '%s:' % _('SVN Revision')),
                          pos = (row, 0),
                          flag = wx.ALIGN_RIGHT)
        
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = vInfo['revision']),
                          pos = (row, 1),
                          flag = wx.ALIGN_LEFT)
        
        row += 1
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = '%s:' % _('GIS Library Revision')),
                          pos = (row, 0),
                          flag = wx.ALIGN_RIGHT)
        
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = vInfo['libgis_revision'] + ' (' +
                                               vInfo['libgis_date'].split(' ')[0] + ')'),
                          pos = (row, 1),
                          flag = wx.ALIGN_LEFT)

        row += 2
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = 'Python:'),
                          pos = (row, 0),
                          flag = wx.ALIGN_RIGHT)
        
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = platform.python_version()),
                          pos = (row, 1),
                          flag = wx.ALIGN_LEFT)

        row += 1
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label =  'wxPython:'),
                          pos = (row, 0),
                          flag = wx.ALIGN_RIGHT)
        
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = wx.__version__),
                          pos = (row, 1),
                          flag = wx.ALIGN_LEFT)
        
        infoSizer.Add(item = infoGridSizer,
                      proportion = 1,
                      flag = wx.EXPAND | wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL)
        
        row += 2
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = "%s:" % _('Language')),
                          pos = (row, 0),
                          flag = wx.ALIGN_RIGHT)
        self.langUsed = grass.gisenv().get('LANG', None)
        if not self.langUsed:
            import locale
            loc = locale.getdefaultlocale()
            if loc == (None, None):
                self.langUsed = _('unknown')
            else:
                self.langUsed = u'%s.%s' % (loc[0], loc[1])
        infoGridSizer.Add(item = wx.StaticText(parent = infoTxt, id = wx.ID_ANY,
                                               label = self.langUsed),
                          pos = (row, 1),
                          flag = wx.ALIGN_LEFT)        
        
        infoTxt.SetSizer(infoSizer)
        infoSizer.Fit(infoTxt)
       
        return infoTxt
    
    def _pageCopyright(self):
        """Copyright information"""
        copyfile = os.path.join(os.getenv("GISBASE"), "COPYING")
        if os.path.exists(copyfile):
            copyrightFile = open(copyfile, 'r')
            copytext = copyrightFile.read()
            copyrightFile.close()
        else:
            copytext = _('%s file missing') % 'COPYING'
        
        # put text into a scrolling panel
        copyrightwin = ScrolledPanel(self.aboutNotebook)
        copyrightwin.SetBackgroundColour('WHITE')
        copyrighttxt = wx.StaticText(copyrightwin, id = wx.ID_ANY, label = copytext)
        copyrightwin.SetAutoLayout(True)
        copyrightwin.sizer = wx.BoxSizer(wx.VERTICAL)
        copyrightwin.sizer.Add(item = copyrighttxt, proportion = 1,
                               flag = wx.EXPAND | wx.ALL, border = 3)
        copyrightwin.SetSizer(copyrightwin.sizer)
        copyrightwin.Layout()
        copyrightwin.SetupScrolling()
        
        return copyrightwin
    
    def _pageLicense(self):
        """Licence about"""
        licfile = os.path.join(os.getenv("GISBASE"), "GPL.TXT")
        if os.path.exists(licfile):
            licenceFile = open(licfile, 'r')
            license = ''.join(licenceFile.readlines())
            licenceFile.close()
        else:
            license = _('%s file missing') % 'GPL.TXT'
        # put text into a scrolling panel
        licensewin = ScrolledPanel(self.aboutNotebook)
        licensewin.SetBackgroundColour('WHITE')
        licensetxt = wx.StaticText(licensewin, id = wx.ID_ANY, label = license)
        licensewin.SetAutoLayout(True)
        licensewin.sizer = wx.BoxSizer(wx.VERTICAL)
        licensewin.sizer.Add(item = licensetxt, proportion = 1,
                flag = wx.EXPAND | wx.ALL, border = 3)
        licensewin.SetSizer(licensewin.sizer)
        licensewin.Layout()
        licensewin.SetupScrolling()
        
        return licensewin
    
    def _pageCredit(self):
        """Credit about"""
                # credits
        authfile = os.path.join(os.getenv("GISBASE"), "AUTHORS")
        if os.path.exists(authfile):
            authorsFile = open(authfile, 'r')
            authors = unicode(''.join(authorsFile.readlines()), "utf-8")
            authorsFile.close()
        else:
            authors = _('%s file missing') % 'AUTHORS'
        authorwin = ScrolledPanel(self.aboutNotebook)
        authorwin.SetBackgroundColour('WHITE')
        authortxt = wx.StaticText(authorwin, id = wx.ID_ANY, label = authors)
        authorwin.SetAutoLayout(True)
        authorwin.SetupScrolling()
        authorwin.sizer = wx.BoxSizer(wx.VERTICAL)
        authorwin.sizer.Add(item = authortxt, proportion = 1,
                flag = wx.EXPAND | wx.ALL, border = 3)
        authorwin.SetSizer(authorwin.sizer)
        authorwin.Layout()      
        
        return authorwin

    def _pageContributors(self, extra = False):
        """Contributors info"""
        if extra:
            contribfile = os.path.join(os.getenv("GISBASE"), "contributors_extra.csv")
        else:
            contribfile = os.path.join(os.getenv("GISBASE"), "contributors.csv")
        if os.path.exists(contribfile):
            contribFile = codecs.open(contribfile, encoding = 'utf-8', mode = 'r')
            contribs = list()
            errLines = list()
            for line in contribFile.readlines()[1:]:
                line = line.rstrip('\n')
                try:
                    if extra:
                        name, email, country, rfc2_agreed = line.split(',')
                    else:
                        cvs_id, name, email, country, osgeo_id, rfc2_agreed = line.split(',')
                except ValueError:
                    errLines.append(line)
                    continue
                if extra:
                    contribs.append((name, email, country))
                else:
                    contribs.append((name, email, country, osgeo_id))
            
            contribFile.close()
            
            if errLines:
                GError(parent = self,
                       message = _("Error when reading file '%s'.") % contribfile + \
                           "\n\n" + _("Lines:") + " %s" % \
                           os.linesep.join(map(DecodeString, errLines)))
        else:
            contribs = None
        
        contribwin = ScrolledPanel(self.aboutNotebook)
        contribwin.SetBackgroundColour('WHITE')
        contribwin.SetAutoLayout(True)
        contribwin.SetupScrolling()
        contribwin.sizer = wx.BoxSizer(wx.VERTICAL)
        
        if not contribs:
            contribtxt = wx.StaticText(contribwin, id = wx.ID_ANY,
                                       label = _('%s file missing') % contribfile)
            contribwin.sizer.Add(item = contribtxt, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        else:
            if extra:
                items = (_('Name'), _('E-mail'), _('Country'))
            else:
                items = (_('Name'), _('E-mail'), _('Country'), _('OSGeo_ID'))
            contribBox = wx.FlexGridSizer(cols = len(items), vgap = 5, hgap = 5)
            for item in items:
                text = wx.StaticText(parent = contribwin, id = wx.ID_ANY,
                                        label = item)
                text.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
                contribBox.Add(item = text)
            for vals in sorted(contribs, key = lambda x: x[0]):
                for item in vals:
                    contribBox.Add(item = wx.StaticText(parent = contribwin, id = wx.ID_ANY,
                                                        label = item))
            contribwin.sizer.Add(item = contribBox, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        
        contribwin.SetSizer(contribwin.sizer)
        contribwin.Layout()      
        
        return contribwin

    def _pageTranslators(self):
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
                GError(parent = self,
                       message = _("Error when reading file '%s'.") % translatorsfile + \
                           "\n\n" + _("Lines:") + " %s" % \
                           os.linesep.join(map(DecodeString, errLines)))
        else:
            translators = None
        
        translatorswin = ScrolledPanel(self.aboutNotebook)
        translatorswin.SetBackgroundColour('WHITE')
        translatorswin.SetAutoLayout(True)
        translatorswin.SetupScrolling()
        translatorswin.sizer = wx.BoxSizer(wx.VERTICAL)
        
        if not translators:
            translatorstxt = wx.StaticText(translatorswin, id = wx.ID_ANY,
                                           label = _('%s file missing') % 'translators.csv')
            translatorswin.sizer.Add(item = translatorstxt, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        else:
            translatorsBox = wx.FlexGridSizer(cols = 4, vgap = 5, hgap = 5)
            languages = translators.keys()
            languages.sort()
            tname = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                label = _('Name'))
            tname.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            translatorsBox.Add(item = tname)
            temail = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                label = _('E-mail'))
            temail.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            translatorsBox.Add(item = temail)
            tlang = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                label = _('Language'))
            tlang.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            translatorsBox.Add(item = tlang)
            tnat = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                label = _('Nation'))
            tnat.SetFont(wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))
            translatorsBox.Add(item = tnat)           
            for lang in languages:
                for translator in translators[lang]:
                    name, email = translator
                    translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                            label =  unicode(name, "utf-8")))
                    translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                            label = email))
                    translatorsBox.Add(item = wx.StaticText(parent = translatorswin, id = wx.ID_ANY,
                                                            label = lang))                                                            
                    flag = os.path.join(os.getenv("GISBASE"), "etc", "gui", 
                            "icons", "flags", "%s.png" % lang.lower())
                    if os.path.exists(flag):
                        flagBitmap = wx.StaticBitmap(parent = translatorswin, id = wx.ID_ANY,
                                     bitmap = wx.Bitmap(name = flag,
                                                        type = wx.BITMAP_TYPE_PNG))
                        translatorsBox.Add(item = flagBitmap)
                    else:
                        translatorsBox.Add(item = wx.StaticText(parent = translatorswin, 
                                        id = wx.ID_ANY, label = lang))
            
            translatorswin.sizer.Add(item = translatorsBox, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        
        translatorswin.SetSizer(translatorswin.sizer)
        translatorswin.Layout()      
        
        return translatorswin

    def _langString(self, k, v):
        """Return string for the status of translation"""
        allStr = "%s :" % k.upper()
        try:
            allStr += _("   %d translated" % v['good'])
        except:
            pass
        try:
            allStr += _("   %d fuzzy" % v['fuzzy'])
        except:
            pass
        try:
            allStr += _("   %d untranslated" % v['bad'])
        except:
            pass
        return allStr
 
    def _langBox(self, par, k, v):
        """Return box"""
        langBox = wx.FlexGridSizer(cols = 4, vgap = 5, hgap = 5)
        tkey = wx.StaticText(parent = par, id = wx.ID_ANY,
                            label = k.upper())
        langBox.Add(item = tkey)
        try:
            tgood = wx.StaticText(parent = par, id = wx.ID_ANY,
                                label = _("%d translated" % v['good']))
            tgood.SetForegroundColour(wx.Colour(35, 142, 35))
            langBox.Add(item = tgood)           
        except:
            tgood = wx.StaticText(parent = par, id = wx.ID_ANY,
                                label = "")
            langBox.Add(item = tgood)
        try:
            tfuzzy = wx.StaticText(parent = par, id = wx.ID_ANY,
                                label = _("   %d fuzzy" % v['fuzzy']))
            tfuzzy.SetForegroundColour(wx.Colour(255, 142, 0))                    
            langBox.Add(item = tfuzzy)
        except:
            tfuzzy = wx.StaticText(parent = par, id = wx.ID_ANY,
                                label = "")
            langBox.Add(item = tfuzzy)
        try:
            tbad = wx.StaticText(parent = par, id = wx.ID_ANY,
                                label = _("   %d untranslated" % v['bad']))
            tbad.SetForegroundColour(wx.Colour(255, 0, 0))
            langBox.Add(item = tbad)
        except:
            tbad = wx.StaticText(parent = par, id = wx.ID_ANY,
                                label = "")
            langBox.Add(item = tbad)                           
        return langBox
        
    def _langPanel(self, lang, js):
        """Create panel for each languages"""
        text = self._langString(lang, js['total'])
        panel = wx.CollapsiblePane(self.statswin, -1, label=text, style=wx.CP_DEFAULT_STYLE|wx.CP_NO_TLW_RESIZE)
        panel.Bind(wx.EVT_COLLAPSIBLEPANE_CHANGED, self.OnPaneChanged)
        win = panel.GetPane()
        # TODO IT DOESN'T WORK
        # TO ADD ONLY WHEN TAB IS OPENED
        #if lang == self.langUsed.split('_')[0]:
            #panel.Collapse(False)
        #else:
            #panel.Collapse(True)        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        for k,v in js.iteritems():
            if k != 'total' and k!= 'name':
                box = self._langBox(win, k,v)
                pageSizer.Add(item = box, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)

        win.SetSizer(pageSizer)
        pageSizer.SetSizeHints(win)

        return panel

    def OnPaneChanged(self, evt):
        """Redo the layout"""
        # TODO better to test on Windows
        self.statswin.SetupScrolling(scrollToTop = False)

    def _pageStats(self):
        """Translation statistics info"""
        fname = "translation_status.json"
        statsfile = os.path.join(os.getenv("GISBASE"), fname)
        if os.path.exists(statsfile):
            statsFile = open(statsfile)
            import json
            jsStats = json.load(statsFile)
        else:
            jsStats = None
        self.statswin = ScrolledPanel(self.aboutNotebook)
        self.statswin.SetBackgroundColour('WHITE')
        self.statswin.SetAutoLayout(True)

        if not jsStats:
            Debug.msg(5, _("File <%s> not found") % fname)
            statsSizer = wx.BoxSizer(wx.VERTICAL)
            statstext = wx.StaticText(self.statswin, id = wx.ID_ANY,
                                           label = _('%s file missing') % fname)
            statsSizer.Add(item = statstext, proportion = 1,
                                 flag = wx.EXPAND | wx.ALL, border = 3)
        else:
            languages = jsStats['langs'].keys()
            languages.sort()
            
            statsSizer = wx.BoxSizer(wx.VERTICAL)
            for lang in languages:
                v = jsStats['langs'][lang]
                panel = self._langPanel(lang, v)
                statsSizer.Add(panel)
        
        self.statswin.SetSizer(statsSizer)
        self.statswin.SetupScrolling(scroll_x = False, scroll_y = True)
        self.statswin.Layout()
        self.statswin.Fit()
        return self.statswin
    
    def OnCloseWindow(self, event):
        """!Close window"""
        self.Close()

class HelpFrame(wx.Dialog):
    """!GRASS Quickstart help window

    As a base class wx.Dialog is used, because of not working
    close button with wx.Frame when dialog is called from wizard.
    If parent is None, application TopLevelWindow is used (wxPython standard behaviour).

    Currently not used (was in location wizard before)
    due to unsolved problems - window sometimes does not respond.
    """
    def __init__(self, parent, id, title, size, file):
        wx.Dialog.__init__(self, parent = parent, id = id, title = title,
                           size = size, style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER | wx.MINIMIZE_BOX)
        
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        # text
        content = HelpPanel(parent = self)
        content.LoadPage(file)
        
        sizer.Add(item = content, proportion = 1, flag = wx.EXPAND)
        
        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        self.Layout()

class HelpWindow(HtmlWindow):
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
        HtmlWindow.__init__(self, parent = parent, **kwargs)
        
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
