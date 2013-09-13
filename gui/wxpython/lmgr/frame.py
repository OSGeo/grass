"""!
@package lmgr::frame

@brief Layer Manager - main menu, layer management toolbar, notebook
control for display management and access to command console.

Classes:
 - frame::GMFrame

(C) 2006-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Jachym Cepicky (Mendel University of Agriculture)
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
"""

import sys
import os
import tempfile
import stat
import platform
import re
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

from core import globalvar
import wx
import wx.aui
try:
    import wx.lib.agw.flatnotebook   as FN
except ImportError:
    import wx.lib.flatnotebook   as FN

if os.path.join(globalvar.ETCDIR, "python") not in sys.path:
    sys.path.append(os.path.join(globalvar.ETCDIR, "python"))

from grass.script          import core as grass

from core.gcmd             import RunCommand, GError, GMessage, GException
from core.settings         import UserSettings, GetDisplayVectSettings
from core.utils            import SetAddOnPath, GetLayerNameFromCmd, command2ltype, _
from gui_core.preferences  import MapsetAccess, PreferencesDialog
from lmgr.layertree        import LayerTree, LMIcons
from lmgr.menudata         import LayerManagerMenuData, LayerManagerModuleTree
from gui_core.widgets      import GNotebook
from modules.mcalc_builder import MapCalcFrame
from dbmgr.manager         import AttributeManager
from core.workspace        import ProcessWorkspaceFile, ProcessGrcFile, WriteWorkspaceFile
from core.gconsole         import GConsole, \
    EVT_CMD_OUTPUT, EVT_IGNORED_CMD_RUN, EVT_IMPORTANT_CMD_RUN, \
    EVT_WRITE_LOG, EVT_WRITE_WARNING, EVT_WRITE_ERROR
from gui_core.goutput      import GConsoleWindow, EVT_GC_CONTENT_CHANGED, GC_SEARCH, GC_PROMPT
from gui_core.dialogs      import GdalOutputDialog, DxfImportDialog, GdalImportDialog, MapLayersDialog
from gui_core.dialogs      import LocationDialog, MapsetDialog, CreateNewVector, GroupDialog
from modules.colorrules    import RasterColorTable, VectorColorTable
from gui_core.menu         import Menu, SearchModuleWindow
from gmodeler.model        import Model
from gmodeler.frame        import ModelFrame
from psmap.frame           import PsMapFrame
from core.debug            import Debug
from gui_core.ghelp        import AboutWindow
from modules.extensions    import InstallExtensionWindow, UninstallExtensionWindow
from lmgr.toolbars         import LMWorkspaceToolbar, LMDataToolbar, LMToolsToolbar
from lmgr.toolbars         import LMMiscToolbar, LMVectorToolbar, LMNvizToolbar
from lmgr.pyshell          import PyShellWindow
from lmgr.giface           import LayerManagerGrassInterface
from gui_core.forms        import GUI
from gcp.manager           import GCPWizard
from nviz.main             import haveNviz
from nviz.preferences      import NvizPreferencesDialog
from mapswipe.frame        import SwipeMapFrame
from rlisetup.frame        import RLiSetupFrame

class GMFrame(wx.Frame):
    """!Layer Manager frame with notebook widget for controlling GRASS
    GIS. Includes command console page for typing GRASS (and other)
    commands, tree widget page for managing map layers.
    """
    def __init__(self, parent, id = wx.ID_ANY, title = _("GRASS GIS Layer Manager"),
                 workspace = None,
                 size = globalvar.GM_WINDOW_SIZE, style = wx.DEFAULT_FRAME_STYLE, **kwargs):
        self.parent    = parent
        self.baseTitle = title
        self.iconsize  = (16, 16)

        self.displayIndex    = 0          # index value for map displays and layer trees
        self.currentPage     = None       # currently selected page for layer tree notebook
        self.currentPageNum  = None       # currently selected page number for layer tree notebook
        self.workspaceFile = workspace    # workspace file
        self.workspaceChanged = False     # track changes in workspace

        wx.Frame.__init__(self, parent = parent, id = id, size = size,
                          style = style, **kwargs)
        self._setTitle()
        self.SetName("LayerManager")
        
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self._giface = LayerManagerGrassInterface(self)
        
        # the main menu bar
        self._menuTreeBuilder = LayerManagerMenuData()
        # the search tree and command console
        self._moduleTreeBuilder = LayerManagerModuleTree()
        self._auimgr = wx.aui.AuiManager(self)
        
        
        # list of open dialogs
        self.dialogs        = dict()
        self.dialogs['preferences'] = None
        self.dialogs['nvizPreferences'] = None
        self.dialogs['atm'] = list()
        
        # create widgets
        self._createMenuBar()
        self.statusbar = self.CreateStatusBar(number = 1)
        self.notebook  = self._createNoteBook()
        self.toolbars  = { 'workspace' : LMWorkspaceToolbar(parent = self),
                           'data'      : LMDataToolbar(parent = self),
                           'tools'     : LMToolsToolbar(parent = self),
                           'misc'      : LMMiscToolbar(parent = self),
                           'vector'    : LMVectorToolbar(parent = self),
                           'nviz'      : LMNvizToolbar(parent = self)}
        self._toolbarsData = { 'workspace' : ("toolbarWorkspace",     # name
                                              _("Workspace Toolbar"), # caption
                                              1),                     # row
                               'data'      : ("toolbarData",
                                              _("Data Toolbar"),
                                              1),
                               'misc'      : ("toolbarMisc",
                                              _("Misc Toolbar"),
                                              2),
                               'tools'     : ("toolbarTools",
                                              _("Tools Toolbar"),
                                              2),
                               'vector'    : ("toolbarVector",
                                              _("Vector Toolbar"),
                                              2),
                               'nviz'      : ("toolbarNviz",
                                              _("3D view Toolbar"),
                                              2),                                            
                               }
        if sys.platform == 'win32':
            self._toolbarsList = ('workspace', 'data',
                                  'vector', 'tools', 'misc', 'nviz')
        else:
            self._toolbarsList = ('data', 'workspace',
                                  'nviz', 'misc', 'tools', 'vector')
        for toolbar in self._toolbarsList:
            name, caption, row = self._toolbarsData[toolbar]
            self._auimgr.AddPane(self.toolbars[toolbar],
                                 wx.aui.AuiPaneInfo().
                                 Name(name).Caption(caption).
                                 ToolbarPane().Top().Row(row).
                                 LeftDockable(False).RightDockable(False).
                                 BottomDockable(False).TopDockable(True).
                                 CloseButton(False).Layer(2).
                                 BestSize((self.toolbars[toolbar].GetBestSize())))
            
        self._auimgr.GetPane('toolbarNviz').Hide()
        # bindings
        self.Bind(wx.EVT_CLOSE,    self.OnCloseWindow)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)

        self._giface.mapCreated.connect(self.OnMapCreated)
        self._giface.updateMap.connect(self._updateCurrentMap)

        # minimal frame size
        self.SetMinSize(globalvar.GM_WINDOW_MIN_SIZE)

        # AUI stuff
        self._auimgr.AddPane(self.notebook, wx.aui.AuiPaneInfo().
                             Left().CentrePane().BestSize((-1,-1)).Dockable(False).
                             CloseButton(False).DestroyOnClose(True).Row(1).Layer(0))

        self._auimgr.Update()

        wx.CallAfter(self.notebook.SetSelectionByName, 'layers')
        
        # use default window layout ?
        if UserSettings.Get(group = 'general', key = 'defWindowPos', subkey = 'enabled'):
            dim = UserSettings.Get(group = 'general', key = 'defWindowPos', subkey = 'dim')
            try:
               x, y = map(int, dim.split(',')[0:2])
               w, h = map(int, dim.split(',')[2:4])
               self.SetPosition((x, y))
               self.SetSize((w, h))
            except:
                pass
        else:
            self.Centre()
        
        self.Layout()
        self.Show()
        
        # load workspace file if requested
        if self.workspaceFile:
            # load given workspace file
            if self.LoadWorkspaceFile(self.workspaceFile):
                self._setTitle()
            else:
                self.workspaceFile = None
        else:
            # start default initial display
            self.NewDisplay(show = False)

        # show map display widnow
        # -> OnSize() -> UpdateMap()
        for mapdisp in self.GetMapDisplay(onlyCurrent = False):
            mapdisp.Show()
        
        # redirect stderr to log area
        self._gconsole.Redirect()
        
        # fix goutput's pane size (required for Mac OSX)`
        self.goutput.SetSashPosition(int(self.GetSize()[1] * .8))
        
        self.workspaceChanged = False
        
        # start with layer manager on top
        if self.currentPage:
            self.GetMapDisplay().Raise()
        wx.CallAfter(self.Raise)

    def _setTitle(self):
        """!Set frame title"""
        if self.workspaceFile:
            self.SetTitle(self.baseTitle + " - " +  os.path.splitext(os.path.basename(self.workspaceFile))[0])
        else:
            self.SetTitle(self.baseTitle)
        
    def _createMenuBar(self):
        """!Creates menu bar"""
        self.menubar = Menu(parent=self, model=self._menuTreeBuilder.GetModel(separators=True))
        self.SetMenuBar(self.menubar)
        self.menucmd = self.menubar.GetCmd()
        
    def _createTabMenu(self):
        """!Creates context menu for display tabs.
        
        Used to rename display.
        """
        menu = wx.Menu()
        item = wx.MenuItem(menu, id = wx.ID_ANY, text = _("Rename Map Display"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameDisplay, item)
        
        return menu
        
    def _setCopyingOfSelectedText(self):
        copy = UserSettings.Get(group = 'manager', key = 'copySelectedTextToClipboard', subkey = 'enabled')
        self.goutput.SetCopyingOfSelectedText(copy)
    
    def IsPaneShown(self, name):
        """!Check if pane (toolbar, ...) of given name is currently shown"""
        if self._auimgr.GetPane(name).IsOk():
            return self._auimgr.GetPane(name).IsShown()
        return False
    
    def _createNoteBook(self):
        """!Creates notebook widgets"""
        self.notebook = GNotebook(parent = self, style = globalvar.FNPageDStyle)
        # create displays notebook widget and add it to main notebook page
        cbStyle = globalvar.FNPageStyle
        if globalvar.hasAgw:
            self.notebookLayers = FN.FlatNotebook(self, id = wx.ID_ANY, agwStyle = cbStyle)
        else:
            self.notebookLayers = FN.FlatNotebook(self, id = wx.ID_ANY, style = cbStyle)
        self.notebookLayers.SetTabAreaColour(globalvar.FNPageColor)
        menu = self._createTabMenu()
        self.notebookLayers.SetRightClickMenu(menu)
        self.notebook.AddPage(page = self.notebookLayers, text = _("Map layers"), name = 'layers')
        
        # create 'command output' text area
        self._gconsole = GConsole(guiparent = self, giface = self._giface,
                                  ignoredCmdPattern = '^d\..*|^r[3]?\.mapcalc$|^i.group$|^r.in.gdal$|'
                                                      '^r.external$|^r.external.out$|'
                                                      '^v.in.ogr$|^v.external$|^v.external.out$')
        self.goutput = GConsoleWindow(parent = self, gconsole = self._gconsole,
                                      menuModel=self._moduleTreeBuilder.GetModel(),
                                      gcstyle = GC_PROMPT)
        self.notebook.AddPage(page = self.goutput, text = _("Command console"), name = 'output')

        self.goutput.showNotification.connect(lambda message: self.SetStatusText(message))

        self._gconsole.mapCreated.connect(self.OnMapCreated)

        # EVT_CMD_OUTPUT and EVT_GC_CONTENT_CHANGED are similar but should be distinct
        # (logging/messages may be splited from GConsole commad running interface)
        # thus, leaving this bind here
        self._gconsole.Bind(EVT_CMD_OUTPUT,
                                lambda event:
                                    self._switchPageHandler(event = event, priority = 1))
        self._gconsole.Bind(EVT_IMPORTANT_CMD_RUN,
                            lambda event:
                                self._switchPageHandler(event = event, priority = 2))
        self._gconsole.Bind(EVT_IGNORED_CMD_RUN,
                            lambda event: self.RunSpecialCmd(event.cmd))
        # if you are chnaging GConsoleWindow to GConsole and
        # EVT_GC_CONTENT_CHANGED to somthing like EVT_LOG_OUTPUT
        # you are doing right
        self.goutput.Bind(EVT_GC_CONTENT_CHANGED,
                          lambda event:
                              self._switchPageHandler(event = event, priority = 1))
        self._gconsole.Bind(EVT_WRITE_LOG,
                            lambda event:
                                self._switchPageHandler(event = event, priority = event.priority))
        self._gconsole.Bind(EVT_WRITE_WARNING,
                            lambda event:
                                self._switchPageHandler(event = event, priority = 2))
        self._gconsole.Bind(EVT_WRITE_ERROR,
                            lambda event:
                                self._switchPageHandler(event = event, priority = 2))
        self._setCopyingOfSelectedText()
        
        # create 'search module' notebook page
        if not UserSettings.Get(group = 'manager', key = 'hideTabs', subkey = 'search'):
            self.search = SearchModuleWindow(parent = self, model=self._moduleTreeBuilder.GetModel())
            self.search.showNotification.connect(lambda message: self.SetStatusText(message))
            self.notebook.AddPage(page = self.search, text = _("Search modules"), name = 'search')
        else:
            self.search = None
        
        # create 'python shell' notebook page
        if not UserSettings.Get(group = 'manager', key = 'hideTabs', subkey = 'pyshell'):
            self.pyshell = PyShellWindow(parent = self)
            self.notebook.AddPage(page = self.pyshell, text = _("Python shell"), name = 'pyshell')
        else:
            self.pyshell = None
        
        # bindings
        self.notebookLayers.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED,    self.OnCBPageChanged)
        self.notebook.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.notebookLayers.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CLOSING,    self.OnCBPageClosed)
        
        return self.notebook
            
    def AddNvizTools(self, firstTime):
        """!Add nviz notebook page

        @param firstTime if a mapdisplay is starting 3D mode for the first time
        """
        Debug.msg(5, "GMFrame.AddNvizTools()")
        if not haveNviz:
            return
        
        from nviz.main import NvizToolWindow
        
        # show toolbar
        self._auimgr.GetPane('toolbarNviz').Show()
        # reorder other toolbars
        for pos, toolbar in enumerate(('toolbarVector', 'toolbarTools', 'toolbarMisc','toolbarNviz')):
            self._auimgr.GetPane(toolbar).Row(2).Position(pos)
        self._auimgr.Update()
        
        # create nviz tools tab
        self.nviz = NvizToolWindow(parent = self,
                                   display = self.GetMapDisplay())
        idx = self.notebook.GetPageIndexByName('layers')
        self.notebook.InsertPage(indx = idx + 1, page = self.nviz, text = _("3D view"), name = 'nviz')
        self.notebook.SetSelectionByName('nviz')

        # this is a bit strange here since a new window is created everytime
        if not firstTime:
            for page in ('view', 'light', 'fringe', 'constant', 'cplane', 'animation'):
                self.nviz.UpdatePage(page)

    def RemoveNvizTools(self):
        """!Remove nviz notebook page"""
        # if more mapwindow3D were possible, check here if nb page should be removed
        self.notebook.SetSelectionByName('layers')
        self.notebook.DeletePage('nviz')

        # hide toolbar
        self._auimgr.GetPane('toolbarNviz').Hide()
        for pos, toolbar in enumerate(('toolbarVector', 'toolbarTools', 'toolbarMisc')):
            self._auimgr.GetPane(toolbar).Row(2).Position(pos)
        self._auimgr.Update()
    
    def WorkspaceChanged(self):
        """!Update window title"""
        if not self.workspaceChanged:
            self.workspaceChanged = True
        
        if self.workspaceFile:
            self._setTitle()
        
    def OnLocationWizard(self, event):
        """!Launch location wizard"""
        from location_wizard.wizard import LocationWizard
        from location_wizard.dialogs import RegionDef
        
        gWizard = LocationWizard(parent = self,
                                 grassdatabase = grass.gisenv()['GISDBASE'])
        location = gWizard.location
        
        if location !=  None:
            dlg = wx.MessageDialog(parent = self,
                                   message = _('Location <%s> created.\n\n'
                                               'Do you want to switch to the '
                                               'new location?') % location,
                                   caption=_("Switch to new location?"),
                                   style = wx.YES_NO | wx.NO_DEFAULT |
                                   wx.ICON_QUESTION | wx.CENTRE)
            
            ret = dlg.ShowModal()
            dlg.Destroy()
            if ret == wx.ID_YES:
                if RunCommand('g.mapset', parent = self,
                              location = location,
                              mapset = 'PERMANENT') != 0:
                    return
                
                GMessage(parent = self,
                         message = _("Current location is <%(loc)s>.\n"
                                     "Current mapset is <%(mapset)s>.") % \
                             { 'loc' : location, 'mapset' : 'PERMANENT' })

                # code duplication with gis_set.py
                dlg = wx.MessageDialog(parent = self,
                               message = _("Do you want to set the default "
                                           "region extents and resolution now?"),
                               caption = _("Location <%s> created") % location,
                               style = wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)
                dlg.CenterOnScreen()
                if dlg.ShowModal() == wx.ID_YES:
                    dlg.Destroy()
                    defineRegion = RegionDef(self, location = location)
                    defineRegion.CenterOnScreen()
                    defineRegion.ShowModal()
                    defineRegion.Destroy()
                else:
                    dlg.Destroy()
        
    def OnSettingsChanged(self):
        """!Here can be functions which have to be called
        after receiving settingsChanged signal. 
        Now only set copying of selected text to clipboard (in goutput).
        """
        ### self._createMenuBar() # bug when menu is re-created on the fly
        self._setCopyingOfSelectedText()
        
    def OnGCPManager(self, event):
        """!Launch georectifier module
        """
        GCPWizard(self, self._giface)

    def OnGModeler(self, event):
        """!Launch Graphical Modeler"""
        win = ModelFrame(parent = self, giface = self._giface)
        win.CentreOnScreen()
        
        win.Show()
        
    def OnPsMap(self, event):
        """!Launch Cartographic Composer
        """
        win = PsMapFrame(parent = self)
        win.CentreOnScreen()
        
        win.Show()

    def OnMapSwipe(self, event):
        """!Launch Map Swipe"""
        win = SwipeMapFrame(parent = self)

        rasters = []
        tree = self.GetLayerTree()
        if tree:
            for layer in tree.GetSelections():
                if tree.GetLayerInfo(layer, key = 'maplayer').GetType() != 'raster':
                    continue
                rasters.append(tree.GetLayerInfo(layer, key = 'maplayer').GetName())

        if len(rasters) >= 1:
            win.SetFirstRaster(rasters[0])
        if len(rasters) >= 2:
            win.SetSecondRaster(rasters[1])
            win.SetRasterNames()

        win.CentreOnScreen()
        win.Show()

    def OnRLiSetup(self, event):
        """!Launch r.li Setup"""
        win = RLiSetupFrame(parent = self)
        win.CentreOnScreen()
        
        win.Show()

    def OnDone(self, cmd, returncode):
        """Command execution finished"""
        if hasattr(self, "model"):
            self.model.DeleteIntermediateData(log = self._gconsole)
            del self.model
        self.SetStatusText('')
        
    def OnRunModel(self, event):
        """!Run model"""
        filename = ''
        dlg = wx.FileDialog(parent = self, message =_("Choose model to run"),
                            defaultDir = os.getcwd(),
                            wildcard = _("GRASS Model File (*.gxm)|*.gxm"))
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
        
        if not filename:
            dlg.Destroy()
            return
        
        self.model = Model()
        self.model.LoadModel(filename)
        self.model.Run(log = self._goutput, onDone = self.OnDone, parent = self)
        
        dlg.Destroy()
        
    def OnMapsets(self, event):
        """!Launch mapset access dialog
        """
        dlg = MapsetAccess(parent = self, id = wx.ID_ANY)
        dlg.CenterOnScreen()
        
        if dlg.ShowModal() == wx.ID_OK:
            ms = dlg.GetMapsets()
            RunCommand('g.mapsets',
                       parent = self,
                       mapset = '%s' % ','.join(ms),
                       operation = 'set')
        
    def OnCBPageChanged(self, event):
        """!Page in notebook (display) changed"""
        self.currentPage    = self.notebookLayers.GetCurrentPage()
        self.currentPageNum = self.notebookLayers.GetSelection()
        try:
            self.GetMapDisplay().SetFocus()
            self.GetMapDisplay().Raise()
        except:
            pass
        
        event.Skip()

    def OnPageChanged(self, event):
        """!Page in notebook changed"""
        page = event.GetSelection()
        if page == self.notebook.GetPageIndexByName('output'):
            wx.CallAfter(self.goutput.ResetFocus)
        self.SetStatusText('', 0)
        
        event.Skip()

    def OnCBPageClosed(self, event):
        """!Page of notebook closed
        Also close associated map display
        """
        if UserSettings.Get(group = 'manager', key = 'askOnQuit', subkey = 'enabled'):
            maptree = self.GetLayerTree()
            
            if self.workspaceFile:
                message = _("Do you want to save changes in the workspace?")
            else:
                message = _("Do you want to store current settings "
                            "to workspace file?")
            
            # ask user to save current settings
            if maptree.GetCount() > 0:
                name = self.notebookLayers.GetPageText(self.currentPageNum)
                dlg = wx.MessageDialog(self,
                                       message = message,
                                       caption = _("Close Map Display %s") % name,
                                       style = wx.YES_NO | wx.YES_DEFAULT |
                                       wx.CANCEL | wx.ICON_QUESTION | wx.CENTRE)
                ret = dlg.ShowModal()
                if ret == wx.ID_YES:
                    if not self.workspaceFile:
                        self.OnWorkspaceSaveAs()
                    else:
                        self.SaveToWorkspaceFile(self.workspaceFile)
                elif ret == wx.ID_CANCEL:
                    event.Veto()
                    dlg.Destroy()
                    return
                dlg.Destroy()

        self.notebookLayers.GetPage(event.GetSelection()).maptree.Map.Clean()
        self.notebookLayers.GetPage(event.GetSelection()).maptree.Close(True)

        self.currentPage = None

        event.Skip()

    def _switchPageHandler(self, event, priority):
        self._switchPage(priority = priority)
        event.Skip()

    def _switchPage(self, priority):
        """!Manages @c 'output' notebook page according to event priority."""
        if priority == 1:
            self.notebook.HighlightPageByName('output')
        if priority >= 2:
            self.notebook.SetSelectionByName('output')
        if priority >= 3:
            self.SetFocus()
            self.Raise()

    def RunSpecialCmd(self, command):
        """!Run command from command line, check for GUI wrappers"""
        if re.compile('^d\..*').search(command[0]):
            self.RunDisplayCmd(command)
        elif re.compile('r[3]?\.mapcalc').search(command[0]):
            self.OnMapCalculator(event = None, cmd = command)
        elif command[0] == 'i.group':
            self.OnEditImageryGroups(event = None, cmd = command)
        elif command[0] == 'r.in.gdal':
            self.OnImportGdalLayers(event = None, cmd = command)
        elif command[0] == 'r.external':
            self.OnLinkGdalLayers(event = None, cmd = command)
        elif command[0] == 'r.external.out':
             self.OnRasterOutputFormat(event = None)
        elif command[0] == 'v.in.ogr':
            self.OnImportOgrLayers(event = None, cmd = command)
        elif command[0] == 'v.external':
            self.OnLinkOgrLayers(event = None, cmd = command)
        elif command[0] == 'v.external.out':
             self.OnVectorOutputFormat(event = None)

        else:
            raise ValueError('Layer Manager special command (%s)'
                             ' not supported.' % ' '.join(command))

    def RunDisplayCmd(self, command):
        """!Handles display commands.

        @param command command in a list
        """
        if not self.currentPage:
            self.NewDisplay(show = True)
        try:
            # display GRASS commands
            layertype = command2ltype[command[0]]
        except KeyError:
            GMessage(parent = self,
                     message = _("Command '%s' not yet implemented in the WxGUI. "
                                 "Try adding it as a command layer instead.") % command[0])
            return
        
        if layertype == 'barscale':
            if len(command) > 1:
                self.GetMapDisplay().AddBarscale(cmd = command, showDialog = False)
            else:
                self.GetMapDisplay().AddBarscale(showDialog = True)
        elif layertype == 'rastleg':
            if len(command) > 1:
                self.GetMapDisplay().AddLegend(cmd = command, showDialog = False)
            else:
                self.GetMapDisplay().AddLegend(showDialog = True)
        elif layertype == 'redraw':
            self.GetMapDisplay().OnRender(None)
        else:
            # add layer into layer tree
            lname, found = GetLayerNameFromCmd(command, fullyQualified = True,
                                               layerType = layertype)
            self.GetLayerTree().AddLayer(ltype = layertype,
                                         lname = lname,
                                         lcmd = command)

    def GetLayerNotebook(self):
        """!Get Layers Notebook"""
        return self.notebookLayers
    
    def GetLayerTree(self):
        """!Get current layer tree

        @return LayerTree instance
        @return None no layer tree selected
        """
        if self.currentPage:
            return self.currentPage.maptree
        return None
    
    def GetMapDisplay(self, onlyCurrent = True):
        """!Get current map display

        @param onlyCurrent True to return only active mapdisplay
                           False for list of all mapdisplays

        @return MapFrame instance (or list)
        @return None no mapdisplay selected
        """
        if onlyCurrent:
            if self.currentPage:
                return self.GetLayerTree().GetMapDisplay()
            else:
                return None
        else: # -> return list of all mapdisplays
            mlist = list()
            for idx in range(0, self.notebookLayers.GetPageCount()):
                mlist.append(self.notebookLayers.GetPage(idx).maptree.GetMapDisplay())
            
            return mlist

    def GetLogWindow(self):
        """!Gets console for command output and messages"""
        return self._gconsole
    
    def GetToolbar(self, name):
        """!Returns toolbar if exists else None"""
        if name in self.toolbars:
            return self.toolbars[name]
        
        return None
        
    def GetMenuCmd(self, event):
        """!Get GRASS command from menu item

        Return command as a list"""
        layer = None
        if event:
            cmd = self.menucmd[event.GetId()]
        else:
            cmd = ''

        try:
            cmdlist = cmd.split(' ')
        except: # already list?
            cmdlist = cmd
        
        # check list of dummy commands for GUI modules that do not have GRASS
        # bin modules or scripts. 
        if cmd in ['vcolors', 'r.mapcalc', 'r3.mapcalc']:
            return cmdlist

        try:
            layer = self.GetLayerTree().layer_selected
            name = self.GetLayerTree().GetLayerInfo(layer, key = 'maplayer').name
            type = self.GetLayerTree().GetLayerInfo(layer, key = 'type')
        except:
            layer = None

        if layer and len(cmdlist) == 1: # only if no paramaters given
            if (type == 'raster' and cmdlist[0][0] == 'r' and cmdlist[0][1] != '3') or \
                    (type == 'vector' and cmdlist[0][0] == 'v'):
                input = GUI().GetCommandInputMapParamKey(cmdlist[0])
                if input:
                    cmdlist.append("%s=%s" % (input, name))
        
        return cmdlist

    def RunMenuCmd(self, event = None, cmd = []):
        """!Run command selected from menu"""
        if event:       
            cmd = self.GetMenuCmd(event)
        self._gconsole.RunCmd(cmd)

    def OnMenuCmd(self, event = None, cmd = []):
        """!Parse command selected from menu"""
        if event:       
            cmd = self.GetMenuCmd(event)
        GUI(parent=self, giface=self._giface).ParseCommand(cmd)
        
    def OnVNet(self, event):
        """Vector network analysis tool"""
        if self.GetMapDisplay():
            self.GetMapDisplay().OnVNet(event)
        else:
            self.NewDisplay(show = True).OnVNet(event)
        
    def OnVDigit(self, event):
        """!Start vector digitizer
        """
        if not self.currentPage:
            self.MsgNoLayerSelected()
            return
        
        tree = self.GetLayerTree()
        layer = tree.layer_selected
        # no map layer selected
        if not layer:
            self.MsgNoLayerSelected()
            return
        
        # available only for vector map layers
        try:
            mapLayer = tree.GetLayerInfo(layer, key = 'maplayer')
        except:
            mapLayer = None
        
        if not mapLayer or mapLayer.GetType() != 'vector':
            GMessage(parent = self,
                     message = _("Selected map layer is not vector."))
            return
        
        if mapLayer.GetMapset() != grass.gisenv()['MAPSET']:
            GMessage(parent = self,
                     message = _("Editing is allowed only for vector maps from the "
                                 "current mapset."))
            return
        
        if not tree.GetLayerInfo(layer):
            return
        dcmd = tree.GetLayerInfo(layer, key = 'cmd')
        if not dcmd:
            return
        
        tree.OnStartEditing(None)
        
    def OnRunScript(self, event):
        """!Run script"""
        # open dialog and choose script file
        dlg = wx.FileDialog(parent = self, message = _("Choose script file to run"),
                            defaultDir = os.getcwd(),
                            wildcard = _("Python script (*.py)|*.py|Bash script (*.sh)|*.sh"))
        
        filename = None
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
        
        if not filename:
            return False
        
        if not os.path.exists(filename):
            GError(parent = self,
                   message = _("Script file '%s' doesn't exist. "
                               "Operation canceled.") % filename)
            return

        # check permission
        if not os.access(filename, os.X_OK):
            dlg = wx.MessageDialog(self,
                                   message = _("Script <%s> is not executable. "
                                               "Do you want to set the permissions "
                                               "that allows you to run this script "
                                               "(note that you must be the owner of the file)?" % \
                                                   os.path.basename(filename)),
                                   caption = _("Set permission?"),
                                   style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() != wx.ID_YES:
                return
            dlg.Destroy()
            try:
                mode = stat.S_IMODE(os.lstat(filename)[stat.ST_MODE])
                os.chmod(filename, mode | stat.S_IXUSR)
            except OSError:
                GError(_("Unable to set permission. Operation canceled."), parent = self)
                return
        
        # check GRASS_ADDON_PATH
        addonPath = os.getenv('GRASS_ADDON_PATH', [])
        if addonPath:
            addonPath = addonPath.split(os.pathsep)
        dirName = os.path.dirname(filename)
        if dirName not in addonPath:
            addonPath.append(dirName)
            dlg = wx.MessageDialog(self,
                                   message = _("Directory '%s' is not defined in GRASS_ADDON_PATH. "
                                               "Do you want add this directory to GRASS_ADDON_PATH?") % \
                                       dirName,
                                   caption = _("Update Addons path?"),
                                   style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_YES:
                SetAddOnPath(os.pathsep.join(addonPath), key = 'PATH')
        
        self._gconsole.WriteCmdLog(_("Launching script '%s'...") % filename)
        self._gconsole.RunCmd([filename], switchPage = True)
        
    def OnChangeLocation(self, event):
        """Change current location"""
        dlg = LocationDialog(parent = self)
        if dlg.ShowModal() == wx.ID_OK:
            location, mapset = dlg.GetValues()
            dlg.Destroy()
            
            if not location or not mapset:
                GError(parent = self,
                       message = _("No location/mapset provided. Operation canceled."))
                return # this should not happen
            
            if RunCommand('g.mapset', parent = self,
                          location = location,
                          mapset = mapset) != 0:
                return # error reported
            
            # close workspace
            self.OnWorkspaceClose()
            self.OnWorkspaceNew()
            GMessage(parent = self,
                     message = _("Current location is <%(loc)s>.\n"
                                 "Current mapset is <%(mapset)s>.") % \
                         { 'loc' : location, 'mapset' : mapset })
        
    def OnCreateMapset(self, event):
        """!Create new mapset"""
        dlg = wx.TextEntryDialog(parent = self,
                                 message = _('Enter name for new mapset:'),
                                 caption = _('Create new mapset'))
        
        if dlg.ShowModal() ==  wx.ID_OK:
            mapset = dlg.GetValue()
            if not mapset:
                GError(parent = self,
                       message = _("No mapset provided. Operation canceled."))
                return
            
            ret = RunCommand('g.mapset',
                             parent = self,
                             flags = 'c',
                             mapset = mapset)
            if ret == 0:
                GMessage(parent = self,
                         message = _("Current mapset is <%s>.") % mapset)
                
    def OnChangeMapset(self, event):
        """Change current mapset"""
        dlg = MapsetDialog(parent = self)
        
        if dlg.ShowModal() == wx.ID_OK:
            mapset = dlg.GetMapset()
            dlg.Destroy()
            
            if not mapset:
                GError(parent = self,
                       message = _("No mapset provided. Operation canceled."))
                return
            
            if RunCommand('g.mapset',
                          parent = self,
                          mapset = mapset) == 0:
                GMessage(parent = self,
                         message = _("Current mapset is <%s>.") % mapset)
        
    def OnChangeCWD(self, event):
        """!Change current working directory
        """
        dlg = wx.DirDialog(parent = self, message = _("Choose a working directory"),
                            defaultPath = os.getcwd(), style = wx.DD_CHANGE_DIR)

        cwd_path = ''
        if dlg.ShowModal() == wx.ID_OK:
            cwd_path = dlg.GetPath()

        # save path to somewhere ?

    def OnNewVector(self, event):
        """!Create new vector map layer"""
        dlg = CreateNewVector(self, giface=self._giface,
                              cmd=(('v.edit',
                                    {'tool': 'create'},
                                    'map')))
        
        if not dlg:
            return
        
        name = dlg.GetName(full = True)
        if name and dlg.IsChecked('add'):
            # add layer to map layer tree
            self.GetLayerTree().AddLayer(ltype = 'vector',
                                            lname = name,
                                            lcmd = ['d.vect', 'map=%s' % name])
        dlg.Destroy()
        
    def OnSystemInfo(self, event):
        """!Print system information"""
        vInfo = grass.version()
        
        # check also OSGeo4W on MS Windows
        if sys.platform == 'win32' and \
                not os.path.exists(os.path.join(os.getenv("GISBASE"), "WinGRASS-README.url")):
            osgeo4w = ' (OSGeo4W)'
        else:
            osgeo4w = ''
        
        self._gconsole.WriteCmdLog(_("System Info"))
        self._gconsole.WriteLog("%s: %s\n"
                                "%s: %s\n"
                                "%s: %s\n"
                                "%s: %s (%s)\n"
                                "GDAL/OGR: %s\n"
                                "PROJ.4: %s\n"
                                "GEOS: %s\n"
                                "SQLite: %s\n"
                                "Python: %s\n"
                                "wxPython: %s\n"
                                "%s: %s%s\n"% (_("GRASS version"), vInfo['version'],
                                               _("GRASS SVN Revision"), vInfo['revision'],
                                               _("Build Date"), vInfo['build_date'],
                                               _("GIS Library Revision"), vInfo['libgis_revision'], vInfo['libgis_date'].split(' ', 1)[0],
                                               vInfo['gdal'], vInfo['proj4'], vInfo['geos'], vInfo['sqlite'],
                                               platform.python_version(),
                                               wx.__version__,
                                               _("Platform"), platform.platform(), osgeo4w),
                                priority = 2)
        self._gconsole.WriteCmdLog(' ')
    
    def OnAboutGRASS(self, event):
        """!Display 'About GRASS' dialog"""
        win = AboutWindow(self)
        win.CentreOnScreen()
        win.Show(True)  

    def _popupMenu(self, data):
        """!Create popup menu
        """
        menu = wx.Menu()
        
        for key, handler in data:
            if key is None:
                menu.AppendSeparator()
                continue
            item = wx.MenuItem(menu, wx.ID_ANY, LMIcons[key].GetLabel())
            item.SetBitmap(LMIcons[key].GetBitmap(self.iconsize))
            menu.AppendItem(item)
            self.Bind(wx.EVT_MENU, handler, item)
        
        # create menu
        self.PopupMenu(menu)
        menu.Destroy()

    def OnImportMenu(self, event):
        """!Import maps menu (import, link)
        """
        self._popupMenu((('rastImport',    self.OnImportGdalLayers),
                         ('rastLink',      self.OnLinkGdalLayers),
                         ('rastOut',       self.OnRasterOutputFormat),
                         (None, None),
                         ('vectImport',    self.OnImportOgrLayers),
                         ('vectLink',      self.OnLinkOgrLayers),
                         ('vectOut',       self.OnVectorOutputFormat)))
        
    def OnWorkspaceNew(self, event = None):
        """!Create new workspace file

        Erase current workspace settings first
        """
        Debug.msg(4, "GMFrame.OnWorkspaceNew():")
        
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay()
        
        maptree = self.GetLayerTree()
        
        # ask user to save current settings
        if self.workspaceFile and self.workspaceChanged:
            self.OnWorkspaceSave()
        elif self.workspaceFile is None and maptree.GetCount() > 0:
             dlg = wx.MessageDialog(self, message = _("Current workspace is not empty. "
                                                    "Do you want to store current settings "
                                                    "to workspace file?"),
                                    caption = _("Create new workspace?"),
                                    style = wx.YES_NO | wx.YES_DEFAULT | \
                                        wx.CANCEL | wx.ICON_QUESTION)
             ret = dlg.ShowModal()
             if ret == wx.ID_YES:
                 self.OnWorkspaceSaveAs()
             elif ret == wx.ID_CANCEL:
                 dlg.Destroy()
                 return
             
             dlg.Destroy()
        
        # delete all items
        maptree.DeleteAllItems()
        
        # add new root element
        maptree.root = maptree.AddRoot("Map Layers")
        self.GetLayerTree().SetPyData(maptree.root, (None,None))
        
        # no workspace file loaded
        self.workspaceFile = None
        self.workspaceChanged = False
        self._setTitle()
        
    def OnWorkspaceOpen(self, event = None):
        """!Open file with workspace definition"""
        dlg = wx.FileDialog(parent = self, message = _("Choose workspace file"),
                            defaultDir = os.getcwd(), wildcard = _("GRASS Workspace File (*.gxw)|*.gxw"))

        filename = ''
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if filename == '':
            return

        Debug.msg(4, "GMFrame.OnWorkspaceOpen(): filename=%s" % filename)
        
        # delete current layer tree content
        self.OnWorkspaceClose()
        
        self.LoadWorkspaceFile(filename)

        self.workspaceFile = filename
        self._setTitle()

    def LoadWorkspaceFile(self, filename):
        """!Load layer tree definition stored in GRASS Workspace XML file (gxw)

        @todo Validate against DTD
        
        @return True on success
        @return False on error
        """
        # dtd
        # dtdFilename = os.path.join(globalvar.ETCWXDIR, "xml", "grass-gxw.dtd")
        
        # parse workspace file
        try:
            gxwXml = ProcessWorkspaceFile(etree.parse(filename))
        except Exception, e:
            GError(parent = self,
                   message = _("Reading workspace file <%s> failed.\n"
                               "Invalid file, unable to parse XML document.") % filename)
            return
        
        busy = wx.BusyInfo(message = _("Please wait, loading workspace..."),
                           parent = self)
        wx.Yield()

        #
        # load layer manager window properties
        #
        if UserSettings.Get(group = 'general', key = 'workspace', subkey = ['posManager', 'enabled']) is False:
            if gxwXml.layerManager['pos']:
                self.SetPosition(gxwXml.layerManager['pos'])
            if gxwXml.layerManager['size']:
                self.SetSize(gxwXml.layerManager['size'])
        
        #
        # start map displays first (list of layers can be empty)
        #
        displayId = 0
        mapdisplay = list()
        for display in gxwXml.displays:
            mapdisp = self.NewDisplay(name = display['name'], show = False)
            mapdisplay.append(mapdisp)
            maptree = self.notebookLayers.GetPage(displayId).maptree
            
            # set windows properties
            mapdisp.SetProperties(render = display['render'],
                                  mode = display['mode'],
                                  showCompExtent = display['showCompExtent'],
                                  alignExtent = display['alignExtent'],
                                  constrainRes = display['constrainRes'],
                                  projection = display['projection']['enabled'])

            if display['projection']['enabled']:
                if display['projection']['epsg']:
                    UserSettings.Set(group = 'display', key = 'projection', subkey = 'epsg',
                                     value = display['projection']['epsg'])
                    if display['projection']['proj']:
                        UserSettings.Set(group = 'display', key = 'projection', subkey = 'proj4',
                                         value = display['projection']['proj'])
            
            # set position and size of map display
            if not UserSettings.Get(group = 'general', key = 'workspace', subkey = ['posDisplay', 'enabled']):
                if display['pos']:
                    mapdisp.SetPosition(display['pos'])
                if display['size']:
                    mapdisp.SetSize(display['size'])
                    
            # set extent if defined
            if display['extent']:
                w, s, e, n = display['extent']
                region = maptree.Map.region = maptree.Map.GetRegion(w = w, s = s, e = e, n = n)
                mapdisp.GetWindow().ResetZoomHistory()
                mapdisp.GetWindow().ZoomHistory(region['n'],
                                                region['s'],
                                                region['e'],
                                                region['w'])
            
            displayId += 1
            mapdisp.Show() # show mapdisplay
    
        maptree = None 
        selected = [] # list of selected layers
        # 
        # load list of map layers
        #
        for layer in gxwXml.layers:
            display = layer['display']
            maptree = self.notebookLayers.GetPage(display).maptree
            
            newItem = maptree.AddLayer(ltype = layer['type'],
                                       lname = layer['name'],
                                       lchecked = layer['checked'],
                                       lopacity = layer['opacity'],
                                       lcmd = layer['cmd'],
                                       lgroup = layer['group'],
                                       lnviz = layer['nviz'],
                                       lvdigit = layer['vdigit'])
            
            if layer.has_key('selected'):
                if layer['selected']:
                    selected.append((maptree, newItem))
                else:
                    maptree.SelectItem(newItem, select = False)
            
        for maptree, layer in selected:
            if not maptree.IsSelected(layer):
                maptree.SelectItem(layer, select = True)
                
        busy.Destroy()
            
        for idx, mdisp in enumerate(mapdisplay):
            mdisp.MapWindow2D.UpdateMap()
            # nviz
            if gxwXml.displays[idx]['viewMode'] == '3d':
                mdisp.AddNviz()
                self.nviz.UpdateState(view = gxwXml.nviz_state['view'],
                                              iview = gxwXml.nviz_state['iview'],
                                              light = gxwXml.nviz_state['light'])
                mdisp.MapWindow3D.constants = gxwXml.nviz_state['constants']
                for idx, constant in enumerate(mdisp.MapWindow3D.constants):
                    mdisp.MapWindow3D.AddConstant(constant, idx + 1)
                for page in ('view', 'light', 'fringe', 'constant', 'cplane'):
                    self.nviz.UpdatePage(page)
                self.nviz.UpdateSettings()
                mdisp.toolbars['map'].combo.SetSelection(1)
            
        return True
    
    def OnWorkspaceLoadGrcFile(self, event):
        """!Load map layers from GRC file (Tcl/Tk GUI) into map layer tree"""
        dlg = wx.FileDialog(parent = self, message = _("Choose GRC file to load"),
                            defaultDir = os.getcwd(), wildcard = _("Old GRASS Workspace File (*.grc)|*.grc"))

        filename = ''
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if filename == '':
            return

        Debug.msg(4, "GMFrame.OnWorkspaceLoadGrcFile(): filename=%s" % filename)

        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay()

        busy = wx.BusyInfo(message = _("Please wait, loading workspace..."),
                           parent = self)
        wx.Yield()

        maptree = None
        for layer in ProcessGrcFile(filename).read(self):
            maptree = self.notebookLayers.GetPage(layer['display']).maptree
            newItem = maptree.AddLayer(ltype = layer['type'],
                                       lname = layer['name'],
                                       lchecked = layer['checked'],
                                       lopacity = layer['opacity'],
                                       lcmd = layer['cmd'],
                                       lgroup = layer['group'])

            busy.Destroy()
            
        if maptree:
            # reverse list of map layers
            maptree.Map.ReverseListOfLayers()

    def OnWorkspaceSaveAs(self, event = None):
        """!Save workspace definition to selected file"""
        dlg = wx.FileDialog(parent = self, message = _("Choose file to save current workspace"),
                            defaultDir = os.getcwd(), wildcard = _("GRASS Workspace File (*.gxw)|*.gxw"), style = wx.FD_SAVE)

        filename = ''
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if filename == '':
            return False

        # check for extension
        if filename[-4:] != ".gxw":
            filename += ".gxw"

        if os.path.exists(filename):
            dlg = wx.MessageDialog(self, message = _("Workspace file <%s> already exists. "
                                                     "Do you want to overwrite this file?") % filename,
                                   caption = _("Save workspace"), style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() != wx.ID_YES:
                dlg.Destroy()
                return False

        Debug.msg(4, "GMFrame.OnWorkspaceSaveAs(): filename=%s" % filename)

        self.SaveToWorkspaceFile(filename)
        self.workspaceFile = filename
        self._setTitle()

    def OnWorkspaceSave(self, event = None):
        """!Save file with workspace definition"""
        if self.workspaceFile:
            dlg = wx.MessageDialog(self, message = _("Workspace file <%s> already exists. "
                                                   "Do you want to overwrite this file?") % \
                                       self.workspaceFile,
                                   caption = _("Save workspace"), style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
            else:
                Debug.msg(4, "GMFrame.OnWorkspaceSave(): filename=%s" % self.workspaceFile)
                self.SaveToWorkspaceFile(self.workspaceFile)
                self._setTitle()
                self.workspaceChanged = False
        else:
            self.OnWorkspaceSaveAs()

    def SaveToWorkspaceFile(self, filename):
        """!Save layer tree layout to workspace file
        
        Return True on success, False on error
        """
        tmpfile = tempfile.TemporaryFile(mode = 'w+b')
        try:
            WriteWorkspaceFile(lmgr = self, file = tmpfile)
        except StandardError, e:
            GError(parent = self,
                   message = _("Writing current settings to workspace file "
                               "failed."))
            return False
        
        try:
            mfile = open(filename, "w")
            tmpfile.seek(0)
            for line in tmpfile.readlines():
                mfile.write(line)
        except IOError:
            GError(parent = self,
                   message = _("Unable to open file <%s> for writing.") % filename)
            return False
        
        mfile.close()
        
        return True
    
    def OnWorkspaceClose(self, event = None):
        """!Close file with workspace definition
        
        If workspace has been modified ask user to save the changes.
        """
        Debug.msg(4, "GMFrame.OnWorkspaceClose(): file=%s" % self.workspaceFile)
        
        self.OnDisplayCloseAll()
        self.workspaceFile = None
        self.workspaceChanged = False
        self._setTitle()
        self.displayIndex = 0
        self.currentPage = None
        
    def OnDisplayClose(self, event = None):
        """!Close current map display window
        """
        if self.currentPage and self.GetMapDisplay():
            self.GetMapDisplay().OnCloseWindow(event)
        
    def OnDisplayCloseAll(self, event = None):
        """!Close all open map display windows
        """
        for display in self.GetMapDisplay(onlyCurrent = False):
            display.OnCloseWindow(event)
        
    def OnRenameDisplay(self, event):
        """!Change Map Display name"""
        name = self.notebookLayers.GetPageText(self.currentPageNum)
        dlg = wx.TextEntryDialog(self, message = _("Enter new name:"),
                                 caption = _("Rename Map Display"), defaultValue = name)
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            self.notebookLayers.SetPageText(page = self.currentPageNum, text = name)
            mapdisplay = self.GetMapDisplay()
            mapdisplay.SetTitle(_("GRASS GIS Map Display: %(name)s  - Location: %(loc)s") % \
                                     { 'name' : name,
                                       'loc' : grass.gisenv()["LOCATION_NAME"] })
        dlg.Destroy()
        
    def OnRasterRules(self, event):
        """!Launches dialog for raster color rules
        """
        ctable = RasterColorTable(self, layerTree = self.GetLayerTree())
        ctable.Show()
        ctable.CentreOnScreen()

    def OnVectorRules(self, event):
        """!Launches dialog for vector color rules
        """
        ctable = VectorColorTable(self, layerTree = self.GetLayerTree(),
                                  attributeType = 'color')
        ctable.Show()
        ctable.CentreOnScreen()
        
    def OnEditImageryGroups(self, event, cmd = None):
        """!Show dialog for creating and editing groups.
        """
        dlg = GroupDialog(self)
        dlg.CentreOnScreen()
        dlg.Show()
        
    def OnInstallExtension(self, event):
        """!Install extension from GRASS Addons SVN repository"""
        win = InstallExtensionWindow(self, giface=self._giface, size = (650, 550))
        win.CentreOnScreen()
        win.Show()
        
    def OnUninstallExtension(self, event):
        """!Uninstall extension"""
        win = UninstallExtensionWindow(self, size = (650, 300))
        win.CentreOnScreen()
        win.Show()

    def OnPreferences(self, event):
        """!General GUI preferences/settings
        """
        if not self.dialogs['preferences']:
            dlg = PreferencesDialog(parent = self, giface = self._giface)
            self.dialogs['preferences'] = dlg
            self.dialogs['preferences'].CenterOnScreen()
            
            dlg.settingsChanged.connect(self.OnSettingsChanged)
        
        self.dialogs['preferences'].ShowModal()
        
    def OnNvizPreferences(self, event):
        """!Show nviz preferences"""
        if not self.dialogs['nvizPreferences']:
            dlg = NvizPreferencesDialog(parent = self, giface = self._giface)
            self.dialogs['nvizPreferences'] = dlg
            self.dialogs['nvizPreferences'].CenterOnScreen()

        self.dialogs['nvizPreferences'].Show()

    def OnHelp(self, event):
        """!Show help
        """
        self._gconsole.RunCmd(['g.manual','-i'])
        
    def OnIClass(self, event=None, cmd=None):
        """!Start wxIClass tool

        The parameters of all handlers which are associated with module
        and contained in menu/toolboxes must be event and cmd.
        When called from menu event is always None and cmd is the associated
        command (list containing a module name and paremeters).
        @todo This documentation is actually documentation of some component related
        to gui_core/menu.py file.
        """
        from iclass.frame import IClassMapFrame, haveIClass, errMsg
        if not haveIClass:
            GError(_('Unable to launch "Supervised Classification Tool".\n\n'
                     'Reason: %s') % errMsg)
            return
        
        win = IClassMapFrame(parent = self, giface = self._giface)
        win.CentreOnScreen()
        
        win.Show()

    def OnAnimationTool(self, event):
        """!Launch Animation tool
        """
        from animation.frame import AnimationFrame

        frame = AnimationFrame(parent = self)
        frame.CentreOnScreen()
        frame.Show()

        tree = self.GetLayerTree()
        if tree:
            rasters = []
            for layer in tree.GetSelectedLayers(checkedOnly = False):
                if tree.GetLayerInfo(layer, key = 'type') == 'raster':
                    rasters.append(tree.GetLayerInfo(layer, key = 'maplayer').GetName())
            if len(rasters) >= 2:
                frame.SetAnimations(raster = [rasters, None, None, None])


    def OnHistogram(self, event):
        """!Init histogram display canvas and tools
        """
        from modules.histogram import HistogramFrame
        win = HistogramFrame(self, giface=self._giface)
        
        win.CentreOnScreen()
        win.Show()
        win.Refresh()
        win.Update()

    def OnMapCalculator(self, event, cmd = ''):
        """!Init map calculator for interactive creation of mapcalc statements
        """
        if event:
            try:
                cmd = self.GetMenuCmd(event)
            except KeyError:
                cmd = ['r.mapcalc']
        
        win = MapCalcFrame(parent = self,
                           giface = self._giface,
                           cmd = cmd[0])
        win.CentreOnScreen()
        win.Show()
    
    def OnVectorCleaning(self, event, cmd = ''):
        """!Init interactive vector cleaning
        """
        from modules.vclean import VectorCleaningFrame
        win = VectorCleaningFrame(parent = self)
        win.CentreOnScreen()
        win.Show()

    def OnRasterOutputFormat(self, event):
        """!Set raster output format handler"""
        self.OnMenuCmd(cmd = ['r.external.out'])

    def OnVectorOutputFormat(self, event):
        """!Set vector output format handler"""
        dlg = GdalOutputDialog(parent = self, ogr = True)
        dlg.CentreOnScreen()
        dlg.Show()
    
    def OnImportDxfFile(self, event, cmd = None):
        """!Convert multiple DXF layers to GRASS vector map layers"""
        dlg = DxfImportDialog(parent = self, giface = self._giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnImportGdalLayers(self, event, cmd = None):
        """!Convert multiple GDAL layers to GRASS raster map layers"""
        dlg = GdalImportDialog(parent = self, giface = self._giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnLinkGdalLayers(self, event, cmd = None):
        """!Link multiple GDAL layers to GRASS raster map layers"""
        dlg = GdalImportDialog(parent = self, giface = self._giface, link = True)
        dlg.CentreOnScreen()
        dlg.Show()
        
    def OnImportOgrLayers(self, event, cmd = None):
        """!Convert multiple OGR layers to GRASS vector map layers"""
        dlg = GdalImportDialog(parent = self, giface = self._giface, ogr = True)
        dlg.CentreOnScreen()
        dlg.Show()
        
    def OnLinkOgrLayers(self, event, cmd = None):
        """!Links multiple OGR layers to GRASS vector map layers"""
        dlg = GdalImportDialog(parent = self, giface = self._giface, ogr = True, link = True)
        dlg.CentreOnScreen()
        dlg.Show()
        
    def OnAddWS(self, event, cmd = None):
        """!Add web services layer"""
        from web_services.dialogs import AddWSDialog
        dlg = AddWSDialog(parent = self, giface = self._giface)
        dlg.CentreOnScreen()
        x, y = dlg.GetPosition()
        dlg.SetPosition((x, y - 200))
        dlg.Show()

    def OnShowAttributeTable(self, event, selection = None):
        """!Show attribute table of the given vector map layer
        """
        if not self.currentPage:
            self.MsgNoLayerSelected()
            return
        
        tree = self.GetLayerTree()
        layer = tree.layer_selected
        # no map layer selected
        if not layer:
            self.MsgNoLayerSelected()
            return
        
        # available only for vector map layers
        try:
            maptype = tree.GetLayerInfo(layer, key = 'maplayer').type
        except:
            maptype = None
        
        if not maptype or maptype != 'vector':
            GMessage(parent = self,
                     message = _("Selected map layer is not vector."))
            return
        
        if not tree.GetLayerInfo(layer):
            return
        dcmd = tree.GetLayerInfo(layer, key = 'cmd')
        if not dcmd:
            return
        
        busy = wx.BusyInfo(message = _("Please wait, loading attribute data..."),
                           parent = self)
        wx.Yield()
        
        dbmanager = AttributeManager(parent = self, id = wx.ID_ANY,
                                     size = wx.Size(500, 300),
                                     item = layer, log = self._gconsole,
                                     selection = selection)
        
        busy.Destroy()
        
        # register ATM dialog
        self.dialogs['atm'].append(dbmanager)
        
        # show ATM window
        dbmanager.Show()
        
    def OnNewDisplay(self, event = None):
        """!Create new layer tree and map display instance"""
        self.NewDisplay()

    def NewDisplay(self, name = None, show = True):
        """!Create new layer tree, which will
        create an associated map display frame

        @param name name of new map display
        @param show show map display window if True

        @return reference to mapdisplay intance
        """
        Debug.msg(1, "GMFrame.NewDisplay(): idx=%d" % self.displayIndex)
        
        # make a new page in the bookcontrol for the layer tree (on page 0 of the notebook)
        self.pg_panel = wx.Panel(self.notebookLayers, id = wx.ID_ANY, style = wx.EXPAND)
        if name:
            dispName = name
        else:
            dispName = "Display " + str(self.displayIndex + 1)
        self.notebookLayers.AddPage(self.pg_panel, text = dispName, select = True)
        self.currentPage = self.notebookLayers.GetCurrentPage()
        
        # create layer tree (tree control for managing GIS layers)  and put on new notebook page
        self.currentPage.maptree = LayerTree(self.currentPage, giface = self._giface,
                                             id = wx.ID_ANY, pos = wx.DefaultPosition,
                                             size = wx.DefaultSize, style = wx.TR_HAS_BUTTONS |
                                             wx.TR_LINES_AT_ROOT| wx.TR_HIDE_ROOT |
                                             wx.TR_DEFAULT_STYLE| wx.NO_BORDER | wx.FULL_REPAINT_ON_RESIZE,
                                             idx = self.displayIndex, lmgr = self, notebook = self.notebookLayers,
                                             showMapDisplay = show)
        
        # layout for controls
        cb_boxsizer = wx.BoxSizer(wx.VERTICAL)
        cb_boxsizer.Add(self.GetLayerTree(), proportion = 1, flag = wx.EXPAND, border = 1)
        self.currentPage.SetSizer(cb_boxsizer)
        cb_boxsizer.Fit(self.GetLayerTree())
        self.currentPage.Layout()
        self.GetLayerTree().Layout()

        mapdisplay = self.currentPage.maptree.mapdisplay
        mapdisplay.Bind(wx.EVT_ACTIVATE,
                        lambda event, page=self.currentPage:
                        self._onMapDisplayFocus(page))
        mapdisplay.starting3dMode.connect(
            lambda firstTime, mapDisplayPage=self.currentPage:
            self._onMapDisplayStarting3dMode(mapDisplayPage))
        mapdisplay.starting3dMode.connect(self.AddNvizTools)
        mapdisplay.ending3dMode.connect(self.RemoveNvizTools)

        # use default window layout
        if UserSettings.Get(group = 'general', key = 'defWindowPos', subkey = 'enabled'):
            dim = UserSettings.Get(group = 'general', key = 'defWindowPos', subkey = 'dim')
            idx = 4 + self.displayIndex * 4
            try:
                x, y = map(int, dim.split(',')[idx:idx + 2])
                w, h = map(int, dim.split(',')[idx + 2:idx + 4])
                self.GetMapDisplay().SetPosition((x, y))
                self.GetMapDisplay().SetSize((w, h))
            except:
                pass
        
        self.displayIndex += 1
        
        return self.GetMapDisplay()

    def _onMapDisplayFocus(self, notebookLayerPage):
        """Changes bookcontrol page to page associated with display."""
        # moved from mapdisp/frame.py
        # TODO: why it is called 3 times when getting focus?
        # and one times when loosing focus?
        pgnum = self.notebookLayers.GetPageIndex(notebookLayerPage)
        if pgnum > -1:
            self.notebookLayers.SetSelection(pgnum)
            self.currentPage = self.notebookLayers.GetCurrentPage()

    def _onMapDisplayStarting3dMode(self, mapDisplayPage):
        """!Disables 3D mode for all map displays except for @p mapDisplay"""
        # TODO: it should be disabled also for newly created map windows
        # moreover mapdisp.Disable3dMode() does not work properly
        for page in range(0, self.GetLayerNotebook().GetPageCount()):
            mapdisp = self.GetLayerNotebook().GetPage(page).maptree.GetMapDisplay()
            if self.GetLayerNotebook().GetPage(page) != mapDisplayPage:
                mapdisp.Disable3dMode()

    def OnAddMaps(self, event = None):
        """!Add selected map layers into layer tree"""
        dialog = MapLayersDialog(parent = self, title = _("Add selected map layers into layer tree"))
        dialog.applyAddingMapLayers.connect(self.AddMaps)
        val = dialog.ShowModal()
        
        if val == wx.ID_OK:
            self.AddMaps(dialog.GetMapLayers(), dialog.GetLayerType(cmd = True))
        dialog.Destroy()

    def AddMaps(self, mapLayers, ltype, check = False):
        """!Add map layers to layer tree.

        @param mapLayers list of map names
        @param ltype layer type ('rast', 'rast3d', 'vect')
        @param check @c True if new layers should be checked in layer tree
        @c False otherwise
        """
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay()
            
        maptree = self.GetLayerTree()
        
        for layerName in mapLayers:
            if ltype == 'rast':
                cmd = ['d.rast', 'map=%s' % layerName]
                wxType = 'raster'
            elif ltype == 'rast3d':
                cmd = ['d.rast3d', 'map=%s' % layerName]
                wxType = '3d-raster'
            elif ltype == 'vect':
                cmd = ['d.vect', 'map=%s' % layerName] + GetDisplayVectSettings()
                wxType = 'vector'
            else:
                GError(parent = self,
                       message = _("Unsupported map layer type <%s>.") % ltype)
                return
            
            newItem = maptree.AddLayer(ltype = wxType,
                                       lname = layerName,
                                       lchecked = check,
                                       lopacity = 1.0,
                                       lcmd = cmd,
                                       lgroup = None)

    def _updateCurrentMap(self):
        """!Updates map of the current map window."""
        self.GetMapDisplay().GetWindow().UpdateMap()

    def OnMapCreated(self, name, ltype, add=None):
        """!Decides wheter the map should be added to layer tree."""
        if add is None:
            if UserSettings.Get(group = 'cmd',
                                key = 'addNewLayer', subkey = 'enabled'):
                self.AddOrUpdateMap(name, ltype)
        elif add:
            self.AddOrUpdateMap(name, ltype)

    def AddOrUpdateMap(self, mapName, ltype):
        """!Add map layer or update"""
        # start new map display if no display is available

        # TODO: standardize type identifiers
        convertType = {'raster': 'rast',
                       '3d-raster': 'rast3d',
                       'vector': 'vect'}
        try:
            grassType = convertType[ltype]
        except KeyError:
            if ltype in convertType.values():
                grassType = ltype
            else:
                GError(parent = self,
                       message = _("Unsupported map layer type <%s>.") % ltype)
                return

        if not self.currentPage:
            self.AddMaps([mapName], grassType, check = True)
        else:
            display = self.GetMapDisplay()
            mapLayers = map(lambda x: x.GetName(),
                            display.GetMap().GetListOfLayers(ltype = ltype))
            if mapName in mapLayers:
                display.GetWindow().UpdateMap(render = True)
            else:
                self.AddMaps([mapName], grassType, check = True)

    def OnAddRaster(self, event):
        """!Add raster map layer"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show = True)
        
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('raster')
        
    def OnAddRasterMisc(self, event):
        """!Create misc raster popup-menu"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show = True)
        
        self._popupMenu((('addRast3d', self.OnAddRaster3D),
                         (None, None),
                         ('addRgb',    self.OnAddRasterRGB),
                         ('addHis',    self.OnAddRasterHIS),
                         (None, None),
                         ('addShaded', self.OnAddRasterShaded),
                         (None, None),
                         ('addRArrow', self.OnAddRasterArrow),
                         ('addRNum',   self.OnAddRasterNum)))
        
        # show map display
        self.GetMapDisplay().Show()
        
    def OnAddVector(self, event):
        """!Add vector map to the current layer tree"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show = True)
        
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('vector')

    def OnAddVectorMisc(self, event):
        """!Create misc vector popup-menu"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show = True)

        self._popupMenu((('addThematic', self.OnAddVectorTheme),
                         ('addChart',    self.OnAddVectorChart)))
        
        # show map display
        self.GetMapDisplay().Show()

    def OnAddVectorTheme(self, event):
        """!Add thematic vector map to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('thememap')

    def OnAddVectorChart(self, event):
        """!Add chart vector map to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('themechart')

    def OnAddOverlay(self, event):
        """!Create decoration overlay menu""" 
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show = True)

        self._popupMenu((('addGrid',     self.OnAddGrid),
                         ('addLabels',   self.OnAddLabels),
                         ('addGeodesic', self.OnAddGeodesic),
                         ('addRhumb',    self.OnAddRhumb),
                         (None, None),
                         ('addCmd',      self.OnAddCommand)))
        
        # show map display
        self.GetMapDisplay().Show()
        
    def OnAddRaster3D(self, event):
        """!Add 3D raster map to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('3d-raster')

    def OnAddRasterRGB(self, event):
        """!Add RGB raster map to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('rgb')

    def OnAddRasterHIS(self, event):
        """!Add HIS raster map to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('his')

    def OnAddRasterShaded(self, event):
        """!Add shaded relief raster map to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('shaded')

    def OnAddRasterArrow(self, event):
        """!Add flow arrows raster map to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        # here it seems that it should be retrieved from the mapwindow
        mapdisplay = self.GetMapDisplay()
        resolution = mapdisplay.mapWindowProperties.resolution
        if not resolution:
            dlg = self.MsgDisplayResolution()
            if dlg.ShowModal() == wx.ID_YES:
                mapdisplay.mapWindowProperties.resolution = True
            dlg.Destroy()

        self.GetLayerTree().AddLayer('rastarrow')

    def OnAddRasterNum(self, event):
        """!Add cell number raster map to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        mapdisplay = self.GetMapDisplay()
        resolution = mapdisplay.mapWindowProperties.resolution
        if not resolution:
            limitText = _("Note that cell values can only be displayed for "
                          "regions of less than 10,000 cells.")
            dlg = self.MsgDisplayResolution(limitText)
            if dlg.ShowModal() == wx.ID_YES:
                mapdisplay.mapWindowProperties.resolution = True
            dlg.Destroy()

        # region = tree.GetMap().GetCurrentRegion()
        # if region['cells'] > 10000:
        #   GMessage(message = "Cell values can only be displayed for regions of < 10,000 cells.", parent = self)
        self.GetLayerTree().AddLayer('rastnum')

    def OnAddCommand(self, event):
        """!Add command line map layer to the current layer tree"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show = True)

        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('command')

        # show map display
        self.GetMapDisplay().Show()

    def OnAddGroup(self, event):
        """!Add layer group"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show = True)

        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('group')

        # show map display
        self.GetMapDisplay().Show()

    def OnAddGrid(self, event):
        """!Add grid map layer to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('grid')

    def OnAddGeodesic(self, event):
        """!Add geodesic line map layer to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('geodesic')

    def OnAddRhumb(self, event):
        """!Add rhumb map layer to the current layer tree"""
        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('rhumb')

    def OnAddLabels(self, event):
        """!Add vector labels map layer to the current layer tree"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show = True)

        self.notebook.SetSelectionByName('layers')
        self.GetLayerTree().AddLayer('labels')

        # show map display
        self.GetMapDisplay().Show()

    def OnDeleteLayer(self, event):
        """!Remove selected map layer from the current layer Tree
        """
        if not self.currentPage or not self.GetLayerTree().layer_selected:
            self.MsgNoLayerSelected()
            return

        if UserSettings.Get(group = 'manager', key = 'askOnRemoveLayer', subkey = 'enabled'):
            layerName = ''
            for item in self.GetLayerTree().GetSelections():
                name = str(self.GetLayerTree().GetItemText(item))
                idx = name.find('(opacity')
                if idx > -1:
                    layerName += '<' + name[:idx].strip(' ') + '>,\n'
                else:
                    layerName += '<' + name + '>,\n'
            layerName = layerName.rstrip(',\n')
            
            if len(layerName) > 2: # <>
                message = _("Do you want to remove map layer(s)\n%s\n"
                            "from layer tree?") % layerName
            else:
                message = _("Do you want to remove selected map layer(s) "
                            "from layer tree?")

            dlg = wx.MessageDialog (parent = self, message = message,
                                    caption = _("Remove map layer"),
                                    style  =  wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)

            if dlg.ShowModal() != wx.ID_YES:
                dlg.Destroy()
                return
            
            dlg.Destroy()

        for layer in self.GetLayerTree().GetSelections():
            if self.GetLayerTree().GetLayerInfo(layer, key = 'type') == 'group':
                self.GetLayerTree().DeleteChildren(layer)
            self.GetLayerTree().Delete(layer)
        
    def OnKeyDown(self, event):
        """!Key pressed"""
        kc = event.GetKeyCode()
        
        if event.ControlDown():
            if kc == wx.WXK_TAB:
                # switch layer list / command output
                if self.notebook.GetSelection() == self.notebook.GetPageIndexByName('layers'):
                    self.notebook.SetSelectionByName('output')
                else:
                    self.notebook.SetSelectionByName('layers')
        
        try:
            ckc = chr(kc)
        except ValueError:
            event.Skip()
            return
        
        if event.CtrlDown():
            if kc == 'R':
                self.OnAddRaster(None)
            elif kc == 'V':
                self.OnAddVector(None)
        
        event.Skip()

    def OnCloseWindow(self, event):
        """!Cleanup when wxGUI is quitted"""
        # save command protocol if actived
        if self.goutput.btnCmdProtocol.GetValue():
            self.goutput.CmdProtocolSave()
        
        if not self.currentPage:
            self._auimgr.UnInit()
            self.Destroy()
            return
        
        # save changes in the workspace
        maptree = self.GetLayerTree()
        if self.workspaceChanged and \
                UserSettings.Get(group = 'manager', key = 'askOnQuit', subkey = 'enabled'):
            if self.workspaceFile:
                message = _("Do you want to save changes in the workspace?")
            else:
                message = _("Do you want to store current settings "
                            "to workspace file?")
            
            # ask user to save current settings
            if maptree.GetCount() > 0:
                dlg = wx.MessageDialog(self,
                                       message = message,
                                       caption = _("Quit GRASS GUI"),
                                       style = wx.YES_NO | wx.YES_DEFAULT |
                                       wx.CANCEL | wx.ICON_QUESTION | wx.CENTRE)
                ret = dlg.ShowModal()
                if ret == wx.ID_YES:
                    if not self.workspaceFile:
                        self.OnWorkspaceSaveAs()
                    else:
                        self.SaveToWorkspaceFile(self.workspaceFile)
                elif ret == wx.ID_CANCEL:
                    # when called from menu, it gets CommandEvent and not CloseEvent
                    if hasattr(event, 'Veto'):
                        event.Veto()
                    dlg.Destroy()
                    return
                dlg.Destroy()
        
        # don't ask any more...
        UserSettings.Set(group = 'manager', key = 'askOnQuit', subkey = 'enabled',
                         value = False)
        
        self.OnDisplayCloseAll()
        
        self.notebookLayers.DeleteAllPages()
        
        self._auimgr.UnInit()
        self.Destroy()
        
    def MsgNoLayerSelected(self):
        """!Show dialog message 'No layer selected'"""
        wx.MessageBox(parent = self,
                      message = _("No map layer selected. Operation canceled."),
                      caption = _("Message"),
                      style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
                      
    def MsgDisplayResolution(self, limitText = None):
        """!Returns dialog for d.rast.num, d.rast.arrow
            when display resolution is not constrained
            
        @param limitText adds a note about cell limit
        """
        message = _("Display resolution is currently not constrained to "
                    "computational settings. "
                    "It's suggested to constrain map to region geometry. "
                    "Do you want to constrain "
                    "the resolution?")
        if limitText:
            message += "\n\n%s" % _(limitText)
        dlg = wx.MessageDialog(parent = self,
                               message = message,
                               caption = _("Constrain map to region geometry?"),
                               style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)
        return dlg
