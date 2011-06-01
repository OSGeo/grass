"""!
@package wxgui.py

@brief Main Python app for GRASS wxPython GUI. Main menu, layer management
toolbar, notebook control for display management and access to
command console.

Classes:
 - GMFrame
 - GMApp

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton (Arizona State University)
@author Jachym Cepicky (Mendel University of Agriculture)
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
"""

import sys
import os
import time
import string
import getopt
import platform
import signal
import tempfile

### XML
try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

### i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

from gui_modules import globalvar
import wx
import wx.aui
import wx.combo
import wx.html
import wx.stc
try:
    import wx.lib.agw.customtreectrl as CT
    import wx.lib.agw.flatnotebook   as FN
except ImportError:
    import wx.lib.customtreectrl as CT
    import wx.lib.flatnotebook   as FN

try:
    import wx.lib.agw.advancedsplash as SC
except ImportError:
    SC = None

sys.path.append(os.path.join(globalvar.ETCDIR, "python"))
from grass.script import core as grass

from gui_modules import utils
from gui_modules import preferences
from gui_modules import layertree
from gui_modules import mapdisp
from gui_modules import menudata
from gui_modules import menuform
from gui_modules import histogram
from gui_modules import profile
from gui_modules import mcalc_builder as mapcalculator
from gui_modules import gcmd
from gui_modules import dbm
from gui_modules import workspace
from gui_modules import goutput
from gui_modules import gdialogs
from gui_modules import colorrules
from gui_modules import ogc_services
from gui_modules import prompt
from gui_modules import menu
from gui_modules import gmodeler
from gui_modules import vclean
from gui_modules import nviz_tools
from gui_modules.debug    import Debug
from gui_modules.ghelp    import MenuTreeWindow, AboutWindow, InstallExtensionWindow
from gui_modules.toolbars import LayerManagerToolbar, ToolsToolbar
from gui_modules.gpyshell import PyShellWindow
from icons.icon           import Icons

UserSettings = preferences.globalSettings

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
        
        wx.Frame.__init__(self, parent = parent, id = id, size = size,
                          style = style, **kwargs)
                          
        self.SetTitle(self.baseTitle)
        self.SetName("LayerManager")

        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))

        self._auimgr = wx.aui.AuiManager(self)

        # initialize variables
        self.disp_idx      = 0            # index value for map displays and layer trees
        self.curr_page     = None         # currently selected page for layer tree notebook
        self.curr_pagenum  = None         # currently selected page number for layer tree notebook
        self.workspaceFile = workspace    # workspace file
        self.workspaceChanged = False     # track changes in workspace
        self.georectifying = None         # reference to GCP class or None
        self.gcpmanagement = None         # reference to GCP class or None
        # list of open dialogs
        self.dialogs        = dict()
        self.dialogs['preferences'] = None
        self.dialogs['atm'] = list()
        
        # creating widgets
        self._createMenuBar()
        self.statusbar = self.CreateStatusBar(number=1)
        self.notebook  = self._createNoteBook()
        self.toolbars = { 'main'  : LayerManagerToolbar(parent = self),
                          'tools' : ToolsToolbar(parent = self) }
        
        # self.SetToolBar(self.toolbar)
        self._auimgr.AddPane(self.toolbars['main'],
                             wx.aui.AuiPaneInfo().
                             Name("toolbarMain").Caption(_("Main Toolbar")).
                             ToolbarPane().Top().
                             LeftDockable(False).RightDockable(False).
                             BottomDockable(False).TopDockable(True).
                             CloseButton(False).Layer(3).
                             BestSize((self.toolbars['main'].GetSize())))
        self._auimgr.AddPane(self.toolbars['tools'],
                             wx.aui.AuiPaneInfo().
                             Name("toolbarTools").Caption(_("Tools Toolbar")).
                             ToolbarPane().Top().
                             LeftDockable(False).RightDockable(False).
                             BottomDockable(False).TopDockable(True).
                             CloseButton(False).Layer(2).
                             BestSize((self.toolbars['tools'].GetSize())))
        
        # bindings
        self.Bind(wx.EVT_CLOSE,    self.OnCloseWindow)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)

        # minimal frame size
        self.SetMinSize((500, 400))

        # AUI stuff
        self._auimgr.AddPane(self.notebook, wx.aui.AuiPaneInfo().
                             Left().CentrePane().BestSize((-1,-1)).Dockable(False).
                             CloseButton(False).DestroyOnClose(True).Row(1).Layer(0))

        self._auimgr.Update()

        wx.CallAfter(self.notebook.SetSelection, 0)
        
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
                self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.workspaceFile))
            else:
                self.workspaceFile = None
        else:
            # start default initial display
            self.NewDisplay(show = False)

        # show map display widnow
        # -> OnSize() -> UpdateMap()
        if self.curr_page and not self.curr_page.maptree.mapdisplay.IsShown():
            self.curr_page.maptree.mapdisplay.Show()
        
        # redirect stderr to log area    
        self.goutput.Redirect()
        # fix goutput's pane size
        self.goutput.SetSashPosition(int(self.GetSize()[1] * .45))

        self.workspaceChanged = False
        
        # start with layer manager on top
        if self.curr_page:
            self.curr_page.maptree.mapdisplay.Raise()
        wx.CallAfter(self.Raise)
        
    def _createMenuBar(self):
        """!Creates menu bar"""
        self.menubar = menu.Menu(parent = self, data = menudata.ManagerData())
        self.SetMenuBar(self.menubar)
        self.menucmd = self.menubar.GetCmd()
        
    def _createNoteBook(self):
        """!Creates notebook widgets"""
        if globalvar.hasAgw:
            self.notebook = FN.FlatNotebook(parent = self, id = wx.ID_ANY, agwStyle = globalvar.FNPageDStyle)
        else:
            self.notebook = FN.FlatNotebook(parent = self, id = wx.ID_ANY, style = globalvar.FNPageDStyle)
        
        # create displays notebook widget and add it to main notebook page
        cbStyle = globalvar.FNPageStyle
        if globalvar.hasAgw:
            self.gm_cb = FN.FlatNotebook(self, id = wx.ID_ANY, agwStyle = cbStyle)
        else:
            self.gm_cb = FN.FlatNotebook(self, id = wx.ID_ANY, style = cbStyle)
        self.gm_cb.SetTabAreaColour(globalvar.FNPageColor)
        self.notebook.AddPage(self.gm_cb, text = _("Map layers"))
        
        # create 'command output' text area
        self.goutput = goutput.GMConsole(self, pageid = 1)
        self.notebook.AddPage(self.goutput, text = _("Command console"))
        
        # create 'search module' notebook page
        self.search = MenuTreeWindow(parent = self)
        self.notebook.AddPage(self.search, text = _("Search module"))
        
        # create 'python shell' notebook page
        self.pyshell = PyShellWindow(parent = self)
        self.notebook.AddPage(self.pyshell, text = _("Python shell"))
        
        # bindings
        self.gm_cb.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED,    self.OnCBPageChanged)
        self.notebook.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.gm_cb.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CLOSING,    self.OnCBPageClosed)
        
        return self.notebook

    def AddNviz(self):
        """!Add nviz notebook page"""
        self.nviz = nviz_tools.NvizToolWindow(parent = self,
                                              display = self.curr_page.maptree.GetMapDisplay()) 
        self.notebook.AddPage(self.nviz, text = _("3D view"))
        self.notebook.SetSelection(self.notebook.GetPageCount() - 1)
        
    def RemoveNviz(self):
        """!Remove nviz notebook page"""
        # print self.notebook.GetPage(1)
        self.notebook.RemovePage(3)
        del self.nviz
        self.notebook.SetSelection(0)
        
    def WorkspaceChanged(self):
        """!Update window title"""
        if not self.workspaceChanged:
            self.workspaceChanged = True
        
        if self.workspaceFile:
            self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.workspaceFile) + '*')
        
    def OnSettingsChanged(self, event):
        """!Here can be functions which have to be called after EVT_SETTINGS_CHANGED. 
        Now recreates menu only.
        """
        self._createMenuBar()
        
    def OnGCPManager(self, event):
        """!Launch georectifier module
        """
        from gui_modules import gcpmanager
        gcpmanager.GCPWizard(self)

    def OnGModeler(self, event):
        """!Launch Graphical Modeler"""
        win = gmodeler.ModelFrame(parent = self)
        win.CentreOnScreen()
        
        win.Show()
        
    def OnPsMap(self, event):
        """!Launch Hardcopy Map Output Utility
        """
        try:
            from gui_modules import psmap
        except:
            gcmd.GError(parent = self.parent,
                        message = _("Hardcopy Map Output Utility is not available. You can install it by %s") % \
                            'g.extension -s svnurl=https://svn.osgeo.org/grass/grass-addons extension=wx.psmap')
            return
        
        win = psmap.PsMapFrame(parent = self)
        win.CentreOnScreen()
        
        win.Show()
        
    def OnDone(self, returncode):
        """Command execution finised"""
        if hasattr(self, "model"):
            self.model.DeleteIntermediateData(log = self.goutput)
            del self.model
        self.SetStatusText('')
        
    def OnRunModel(self, event):
        """!Run model"""
        filename = ''
        dlg = wx.FileDialog(parent = self, message=_("Choose model to run"),
                            defaultDir = os.getcwd(),
                            wildcard=_("GRASS Model File (*.gxm)|*.gxm"))
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
        
        if not filename:
            return
        
        self.model = gmodeler.Model()
        self.model.LoadModel(filename)
        self.SetStatusText(_('Validating model...'), 0)
        result =  self.model.Validate()
        if result:
            dlg = wx.MessageDialog(parent = self,
                                   message = _('Model is not valid. Do you want to '
                                               'run the model anyway?\n\n%s') % '\n'.join(errList),
                                   caption=_("Run model?"),
                                   style = wx.YES_NO | wx.NO_DEFAULT |
                                   wx.ICON_QUESTION | wx.CENTRE)
            ret = dlg.ShowModal()
            if ret != wx.ID_YES:
                return
        
        self.SetStatusText(_('Running model...'), 0)
        self.model.Run(log = self.goutput,
                       onDone = self.OnDone)
        
    def OnMapsets(self, event):
        """!Launch mapset access dialog
        """
        dlg = preferences.MapsetAccess(parent = self, id = wx.ID_ANY)
        dlg.CenterOnScreen()
        
        if dlg.ShowModal() == wx.ID_OK:
            ms = dlg.GetMapsets()
            gcmd.RunCommand('g.mapsets',
                            parent = self,
                            mapset = '%s' % ','.join(ms))
        
    def OnCBPageChanged(self, event):
        """!Page in notebook (display) changed"""
        old_pgnum = event.GetOldSelection()
        new_pgnum = event.GetSelection()
        
        self.curr_page   = self.gm_cb.GetCurrentPage()
        self.curr_pagenum = self.gm_cb.GetSelection()
        try:
            self.curr_page.maptree.mapdisplay.SetFocus()
            self.curr_page.maptree.mapdisplay.Raise()
        except:
            pass
        
        event.Skip()

    def OnPageChanged(self, event):
        """!Page in notebook changed"""
        page = event.GetSelection()
        if page == self.goutput.pageid:
            # remove '(...)'
            self.notebook.SetPageText(page, _("Command console"))
            wx.CallAfter(self.goutput.cmd_prompt.SetFocus)
        self.SetStatusText('', 0)
        
        event.Skip()

    def OnCBPageClosed(self, event):
        """!Page of notebook closed
        Also close associated map display
        """
        if UserSettings.Get(group = 'manager', key = 'askOnQuit', subkey = 'enabled'):
            maptree = self.curr_page.maptree
            
            if self.workspaceFile:
                message = _("Do you want to save changes in the workspace?")
            else:
                message = _("Do you want to store current settings "
                            "to workspace file?")
            
            # ask user to save current settings
            if maptree.GetCount() > 0:
                dlg = wx.MessageDialog(self,
                                       message = message,
                                       caption = _("Close Map Display %d") % (self.curr_pagenum + 1),
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

        self.gm_cb.GetPage(event.GetSelection()).maptree.Map.Clean()
        self.gm_cb.GetPage(event.GetSelection()).maptree.Close(True)

        self.curr_page = None

        event.Skip()

    def GetLayerTree(self):
        """!Get current layer tree"""
        return self.curr_page.maptree
    
    def GetLogWindow(self):
        """!Get widget for command output"""
        return self.goutput
    
    def GetMenuCmd(self, event):
        """!Get GRASS command from menu item

        Return command as a list"""
        layer = None
        
        if event:
            cmd = self.menucmd[event.GetId()]
        
        try:
            cmdlist = cmd.split(' ')
        except: # already list?
            cmdlist = cmd
        
        # check list of dummy commands for GUI modules that do not have GRASS
        # bin modules or scripts. 
        if cmd in ['vcolors', 'r.mapcalc', 'r3.mapcalc']:
            return cmdlist

        try:
            layer = self.curr_page.maptree.layer_selected
            name = self.curr_page.maptree.GetPyData(layer)[0]['maplayer'].name
            type = self.curr_page.maptree.GetPyData(layer)[0]['type']
        except:
            layer = None

        if layer and len(cmdlist) == 1: # only if no paramaters given
            if (type == 'raster' and cmdlist[0][0] == 'r' and cmdlist[0][1] != '3') or \
                    (type == 'vector' and cmdlist[0][0] == 'v'):
                input = menuform.GUI().GetCommandInputMapParamKey(cmdlist[0])
                if input:
                    cmdlist.append("%s=%s" % (input, name))
        
        return cmdlist

    def RunMenuCmd(self, event = None, cmd = []):
        """!Run command selected from menu"""
        if event:
            cmd = self.GetMenuCmd(event)
        self.goutput.RunCmd(cmd, switchPage = False)

    def OnMenuCmd(self, event = None, cmd = []):
        """!Parse command selected from menu"""
        if event:
            cmd = self.GetMenuCmd(event)
        menuform.GUI(parent = self).ParseCommand(cmd)
        
    def OnVDigit(self, event):
        """!Start vector digitizer
        """
        if not self.curr_page:
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
            mapLayer = tree.GetPyData(layer)[0]['maplayer']
        except:
            mapLayer = None
        
        if not mapLayer or mapLayer.GetType() != 'vector':
            gcmd.GMessage(parent = self,
                          message = _("Selected map layer is not vector."))
            return
        
        if mapLayer.GetMapset() != grass.gisenv()['MAPSET']:
            gcmd.GMessage(parent = self,
                          message = _("Editing is allowed only for vector maps from the "
                                      "current mapset."))
            return
        
        if not tree.GetPyData(layer)[0]:
            return
        dcmd = tree.GetPyData(layer)[0]['cmd']
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
            gcmd.GError(parent = self,
                        message = _("Script file '%s' doesn't exist. "
                                    "Operation cancelled.") % filename)
            return
        
        self.goutput.WriteCmdLog(_("Launching script '%s'...") % filename)
        self.goutput.RunCmd([filename], switchPage = True)
        
    def OnChangeLocation(self, event):
        """Change current location"""
        dlg = gdialogs.LocationDialog(parent = self)
        if dlg.ShowModal() == wx.ID_OK:
            location, mapset = dlg.GetValues()
            if location and mapset:
                ret = gcmd.RunCommand("g.gisenv",
                                      set = "LOCATION_NAME=%s" % location)
                ret += gcmd.RunCommand("g.gisenv",
                                       set = "MAPSET=%s" % mapset)
                if ret > 0:
                    wx.MessageBox(parent = self,
                                  message = _("Unable to switch to location <%(loc)s> mapset <%(mapset)s>.") % \
                                      { 'loc' : location, 'mapset' : mapset },
                                  caption = _("Error"), style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
                else:
                    # close workspace
                    self.OnWorkspaceClose()
                    self.OnWorkspaceNew()
                    wx.MessageBox(parent = self,
                                  message = _("Current location is <%(loc)s>.\n"
                                              "Current mapset is <%(mapset)s>.") % \
                                      { 'loc' : location, 'mapset' : mapset },
                                  caption = _("Info"), style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
                    
    def OnChangeMapset(self, event):
        """Change current mapset"""
        dlg = gdialogs.MapsetDialog(parent = self)
        if dlg.ShowModal() == wx.ID_OK:
            mapset = dlg.GetMapset()
            if mapset:
                if gcmd.RunCommand("g.gisenv",
                                   set = "MAPSET=%s" % mapset) != 0:
                    wx.MessageBox(parent = self,
                                  message = _("Unable to switch to mapset <%s>.") % mapset,
                                  caption = _("Error"), style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
                else:
                    wx.MessageBox(parent = self,
                                  message = _("Current mapset is <%s>.") % mapset,
                                  caption = _("Info"), style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
        
    def OnNewVector(self, event):
        """!Create new vector map layer"""
        name, add = gdialogs.CreateNewVector(self, log = self.goutput,
                                             cmd = (('v.edit',
                                                     { 'tool' : 'create' },
                                                     'map')))
        
        if name and add:
            # add layer to map layer tree
            self.curr_page.maptree.AddLayer(ltype = 'vector',
                                            lname = name,
                                            lchecked = True,
                                            lopacity = 1.0,
                                            lcmd = ['d.vect', 'map=%s' % name])
        
    def OnAboutGRASS(self, event):
        """!Display 'About GRASS' dialog"""
        win = AboutWindow(self)
        win.CentreOnScreen()
        win.Show(True)  
        
    def _popupMenu(self, data):
        """!Create popup menu
        """
        point = wx.GetMousePosition()
        menu = wx.Menu()
        
        for key, handler in data:
            if key is None:
                menu.AppendSeparator()
                continue
            item = wx.MenuItem(menu, wx.ID_ANY, Icons['layerManager'][key].GetLabel())
            item.SetBitmap(Icons['layerManager'][key].GetBitmap(self.iconsize))
            menu.AppendItem(item)
            self.Bind(wx.EVT_MENU, handler, item)
        
        # create menu
        self.PopupMenu(menu)
        menu.Destroy()

    def OnNewMenu(self, event):
        """!New display/workspace menu
        """
        self._popupMenu((('newdisplay', self.OnNewDisplay),
                         ('workspaceNew',  self.OnWorkspaceNew)))

    def OnLoadMenu(self, event):
        """!Load maps menu (new, load, import, link)
        """
        self._popupMenu((('workspaceLoad', self.OnWorkspaceLoad),
                         (None, None),
                         ('rastImport',    self.OnImportGdalLayers),
                         ('rastLink',      self.OnLinkGdalLayers),
                         (None, None),
                         ('vectImport',    self.OnImportOgrLayers),
                         ('vectLink',      self.OnLinkOgrLayers)))
        
    def OnWorkspaceNew(self, event = None):
        """!Create new workspace file

        Erase current workspace settings first
        """
        Debug.msg(4, "GMFrame.OnWorkspaceNew():")
        
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay()
        
        maptree = self.curr_page.maptree
        
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
        self.curr_page.maptree.SetPyData(maptree.root, (None,None))
        
        # no workspace file loaded
        self.workspaceFile = None
        self.workspaceChanged = False
        self.SetTitle(self.baseTitle)
        
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
        self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.workspaceFile))

    def LoadWorkspaceFile(self, filename):
        """!Load layer tree definition stored in GRASS Workspace XML file (gxw)

        @todo Validate against DTD
        
        @return True on success
        @return False on error
        """
        # dtd
        dtdFilename = os.path.join(globalvar.ETCWXDIR, "xml", "grass-gxw.dtd")
        
        # parse workspace file
        try:
            gxwXml = workspace.ProcessWorkspaceFile(etree.parse(filename))
        except Exception, e:
            gcmd.GError(parent = self,
                        message = _("Reading workspace file <%s> failed.\n"
                                    "Invalid file, unable to parse XML document.") % filename)
            return
        
        busy = wx.BusyInfo(message = _("Please wait, loading workspace..."),
                           parent = self)
        wx.Yield()

        #
        # load layer manager window properties
        #
        if UserSettings.Get(group = 'workspace', key = 'posManager', subkey = 'enabled') is False:
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
            mapdisp = self.NewDisplay(show = False)
            mapdisplay.append(mapdisp)
            maptree = self.gm_cb.GetPage(displayId).maptree
            
            # set windows properties
            mapdisp.SetProperties(render = display['render'],
                                  mode = display['mode'],
                                  showCompExtent = display['showCompExtent'],
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
            if UserSettings.Get(group = 'workspace', key = 'posDisplay', subkey = 'enabled') is False:
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
                
            mapdisp.Show()
            
            displayId += 1
    
        maptree = None 
        selected = [] # list of selected layers
        # 
        # load list of map layers
        #
        for layer in gxwXml.layers:
            display = layer['display']
            maptree = self.gm_cb.GetPage(display).maptree
            
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
                maptree.layer_selected = layer
                
        busy.Destroy()
        
        if maptree:
            # reverse list of map layers
            maptree.Map.ReverseListOfLayers()
            
        for mdisp in mapdisplay:
            mdisp.MapWindow2D.UpdateMap()

        return True

    def OnWorkspaceLoad(self, event = None):
        """!Load given map layers into layer tree"""
        dialog = gdialogs.LoadMapLayersDialog(parent = self, title = _("Load map layers into layer tree"))

        if dialog.ShowModal() == wx.ID_OK:
            # start new map display if no display is available
            if not self.curr_page:
                self.NewDisplay()

            maptree = self.curr_page.maptree
            busy = wx.BusyInfo(message = _("Please wait, loading workspace..."),
                               parent = self)
            wx.Yield()
            
            for layerName in dialog.GetMapLayers():
                if dialog.GetLayerType() == 'raster':
                    cmd = ['d.rast', 'map=%s' % layerName]
                elif dialog.GetLayerType() == 'vector':
                    cmd = ['d.vect', 'map=%s' % layerName]
                newItem = maptree.AddLayer(ltype = dialog.GetLayerType(),
                                           lname = layerName,
                                           lchecked = False,
                                           lopacity = 1.0,
                                           lcmd = cmd,
                                           lgroup = None)

            busy.Destroy()

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
        if not self.curr_page:
            self.NewDisplay()

        busy = wx.BusyInfo(message = _("Please wait, loading workspace..."),
                           parent = self)
        wx.Yield()

        maptree = None
        for layer in workspace.ProcessGrcFile(filename).read(self):
            maptree = self.gm_cb.GetPage(layer['display']).maptree
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
        self.SetTitle(self.baseTitle + " - " + os.path.basename(self.workspaceFile))

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
                self.SetTitle(self.baseTitle + " - " + os.path.basename(self.workspaceFile))
                self.workspaceChanged = False
        else:
            self.OnWorkspaceSaveAs()

    def SaveToWorkspaceFile(self, filename):
        """!Save layer tree layout to workspace file
        
        Return True on success, False on error
        """
        tmpfile = tempfile.TemporaryFile(mode = 'w+b')
        try:
            workspace.WriteWorkspaceFile(lmgr = self, file = tmpfile)
        except StandardError, e:
            gcmd.GError(parent = self,
                        message = _("Writing current settings to workspace file "
                                    "failed."))
            return False
        
        try:
            mfile = open(filename, "w")
            tmpfile.seek(0)
            for line in tmpfile.readlines():
                mfile.write(line)
        except IOError:
            gcmd.GError(parent = self,
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
        self.SetTitle(self.baseTitle)
        self.disp_idx = 0
        self.curr_page = None
        
    def OnDisplayClose(self, event = None):
        """!Close current map display window
        """
        if self.curr_page and self.curr_page.maptree.mapdisplay:
            self.curr_page.maptree.mapdisplay.OnCloseWindow(event)
        
    def OnDisplayCloseAll(self, event = None):
        """!Close all open map display windows
        """
        displays = list()
        for page in range(0, self.gm_cb.GetPageCount()):
            displays.append(self.gm_cb.GetPage(page).maptree.mapdisplay)
        
        for display in displays:
            display.OnCloseWindow(event)
        
    def RulesCmd(self, event):
        """!Launches dialog for commands that need rules input and
        processes rules
        """
        cmd = self.GetMenuCmd(event)
        
        if cmd[0] == 'r.colors':
            ctable = colorrules.ColorTable(self, raster = True)
        else:
            ctable = colorrules.ColorTable(self, raster = False)
        ctable.CentreOnScreen()
        ctable.Show()
        
    def OnInstallExtension(self, event):
        """!Install extension from GRASS Addons SVN repository"""
        win = InstallExtensionWindow(self, size = (650, 550))
        win.CentreOnScreen()
        win.Show()
        
    def OnPreferences(self, event):
        """!General GUI preferences/settings
        """
        if not self.dialogs['preferences']:
            dlg = preferences.PreferencesDialog(parent = self)
            self.dialogs['preferences'] = dlg
            self.dialogs['preferences'].CenterOnScreen()
            
            dlg.Bind(preferences.EVT_SETTINGS_CHANGED, self.OnSettingsChanged)
        
        self.dialogs['preferences'].ShowModal()
        
    def OnHelp(self, event):
        """!Show help
        """
        self.goutput.RunCmd(['g.manual','-i'])
        
    def DispHistogram(self, event):
        """
        Init histogram display canvas and tools
        """
        self.histogram = histogram.HistFrame(self,
                                             id = wx.ID_ANY, pos = wx.DefaultPosition, size = (400,300),
                                             style = wx.DEFAULT_FRAME_STYLE)

        #show new display
        self.histogram.Show()
        self.histogram.Refresh()
        self.histogram.Update()

    def DispProfile(self, event):
        """
        Init profile canvas and tools
        """
        self.profile = profile.ProfileFrame(self,
                                           id = wx.ID_ANY, pos = wx.DefaultPosition, size = (400,300),
                                           style = wx.DEFAULT_FRAME_STYLE)
        self.profile.Show()
        self.profile.Refresh()
        self.profile.Update()
        
    def OnMapCalculator(self, event, cmd = ''):
        """!Init map calculator for interactive creation of mapcalc statements
        """
        if event:
            try:
                cmd = self.GetMenuCmd(event)
            except KeyError:
                cmd = ['r.mapcalc']
        
        win = mapcalculator.MapCalcFrame(parent = self,
                                         cmd = cmd[0])
        win.CentreOnScreen()
        win.Show()
    
    def OnVectorCleaning(self, event, cmd = ''):
        """!Init interactive vector cleaning
        """
        
        if event:
            cmd = self.GetMenuCmd(event)

        win = vclean.VectorCleaningFrame(parent = self, cmd = cmd[0])
        win.CentreOnScreen()
        win.Show()
        
    def OnImportDxfFile(self, event, cmd = None):
        """!Convert multiple DXF layers to GRASS vector map layers"""
        dlg = gdialogs.DxfImportDialog(parent = self)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnImportGdalLayers(self, event, cmd = None):
        """!Convert multiple GDAL layers to GRASS raster map layers"""
        dlg = gdialogs.GdalImportDialog(parent = self)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnLinkGdalLayers(self, event, cmd = None):
        """!Link multiple GDAL layers to GRASS raster map layers"""
        dlg = gdialogs.GdalImportDialog(parent = self, link = True)
        dlg.CentreOnScreen()
        dlg.Show()
        
    def OnImportOgrLayers(self, event, cmd = None):
        """!Convert multiple OGR layers to GRASS vector map layers"""
        dlg = gdialogs.GdalImportDialog(parent = self, ogr = True)
        dlg.CentreOnScreen()
        dlg.Show()
        
    def OnLinkOgrLayers(self, event, cmd = None):
        """!Links multiple OGR layers to GRASS vector map layers"""
        dlg = gdialogs.GdalImportDialog(parent = self, ogr = True, link = True)
        dlg.CentreOnScreen()
        dlg.Show()
        
    def OnImportWMS(self, event):
        """!Import data from OGC WMS server"""
        dlg = ogc_services.WMSDialog(parent = self, service = 'wms')
        dlg.CenterOnScreen()
        
        if dlg.ShowModal() == wx.ID_OK: # -> import layers
            layers = dlg.GetLayers()
            
            if len(layers.keys()) > 0:
                for layer in layers.keys():
                    cmd = ['r.in.wms',
                           'mapserver=%s' % dlg.GetSettings()['server'],
                           'layers=%s' % layer,
                           'output=%s' % layer,
                           'format=png',
                           '--overwrite']
                    styles = ','.join(layers[layer])
                    if styles:
                        cmd.append('styles=%s' % styles)
                    self.goutput.RunCmd(cmd, switchPage = True)

                    self.curr_page.maptree.AddLayer(ltype = 'raster',
                                                    lname = layer,
                                                    lcmd = ['d.rast', 'map=%s' % layer],
                                                    multiple = False)
            else:
                self.goutput.WriteWarning(_("Nothing to import. No WMS layer selected."))
                
                
        dlg.Destroy()
        
    def OnShowAttributeTable(self, event):
        """!Show attribute table of the given vector map layer
        """
        if not self.curr_page:
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
            maptype = tree.GetPyData(layer)[0]['maplayer'].type
        except:
            maptype = None
        
        if not maptype or maptype != 'vector':
            gcmd.GMessage(parent = self,
                          message = _("Selected map layer is not vector."))
            return
        
        if not tree.GetPyData(layer)[0]:
            return
        dcmd = tree.GetPyData(layer)[0]['cmd']
        if not dcmd:
            return
        
        busy = wx.BusyInfo(message = _("Please wait, loading attribute data..."),
                           parent = self)
        wx.Yield()
        
        dbmanager = dbm.AttributeManager(parent = self, id = wx.ID_ANY,
                                         size = wx.Size(500, 300),
                                         item = layer, log = self.goutput)
        
        busy.Destroy()
        
        # register ATM dialog
        self.dialogs['atm'].append(dbmanager)
        
        # show ATM window
        dbmanager.Show()
        
    def OnNewDisplay(self, event = None):
        """!Create new layer tree and map display instance"""
        self.NewDisplay()

    def NewDisplay(self, show = True):
        """!Create new layer tree, which will
        create an associated map display frame

        @param show show map display window if True

        @return reference to mapdisplay intance
        """
        Debug.msg(1, "GMFrame.NewDisplay(): idx=%d" % self.disp_idx)
        
        # make a new page in the bookcontrol for the layer tree (on page 0 of the notebook)
        self.pg_panel = wx.Panel(self.gm_cb, id = wx.ID_ANY, style = wx.EXPAND)
        self.gm_cb.AddPage(self.pg_panel, text = "Display "+ str(self.disp_idx + 1), select = True)
        self.curr_page = self.gm_cb.GetCurrentPage()
        
        # create layer tree (tree control for managing GIS layers)  and put on new notebook page
        self.curr_page.maptree = layertree.LayerTree(self.curr_page, id = wx.ID_ANY, pos = wx.DefaultPosition,
                                                     size = wx.DefaultSize, style = wx.TR_HAS_BUTTONS |
                                                     wx.TR_LINES_AT_ROOT| wx.TR_HIDE_ROOT |
                                                     wx.TR_DEFAULT_STYLE| wx.NO_BORDER | wx.FULL_REPAINT_ON_RESIZE,
                                                     idx = self.disp_idx, lmgr = self, notebook = self.gm_cb,
                                                     auimgr = self._auimgr, showMapDisplay = show)
        
        # layout for controls
        cb_boxsizer = wx.BoxSizer(wx.VERTICAL)
        cb_boxsizer.Add(self.curr_page.maptree, proportion = 1, flag = wx.EXPAND, border = 1)
        self.curr_page.SetSizer(cb_boxsizer)
        cb_boxsizer.Fit(self.curr_page.maptree)
        self.curr_page.Layout()
        self.curr_page.maptree.Layout()
        
        # use default window layout
        if UserSettings.Get(group = 'general', key = 'defWindowPos', subkey = 'enabled'):
            dim = UserSettings.Get(group = 'general', key = 'defWindowPos', subkey = 'dim')
            idx = 4 + self.disp_idx * 4
            try:
                x, y = map(int, dim.split(',')[idx:idx + 2])
                w, h = map(int, dim.split(',')[idx + 2:idx + 4])
                self.curr_page.maptree.mapdisplay.SetPosition((x, y))
                self.curr_page.maptree.mapdisplay.SetSize((w, h))
            except:
                pass
        
        self.disp_idx += 1
        
        return self.curr_page.maptree.mapdisplay

    def OnAddRaster(self, event):
        """!Add raster map layer"""
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay(show = True)
        
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('raster')
        
    def OnAddRaster3D(self, event):
        """!Add 3D raster map layer"""
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay(show = True)
        
        self.AddRaster3D(event)
        
    def OnAddRasterMisc(self, event):
        """!Create misc raster popup-menu"""
        # start new map display if no display is available
        if not self.curr_page:
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
        self.curr_page.maptree.mapdisplay.Show()
        
    def OnAddVector(self, event):
        """!Add vector map to the current layer tree"""
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay(show = True)
        
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('vector')

    def OnAddVectorMisc(self, event):
        """!Create misc vector popup-menu"""
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay(show = True)

        self._popupMenu((('addThematic', self.OnAddVectorTheme),
                         ('addChart',    self.OnAddVectorChart)))
        
        # show map display
        self.curr_page.maptree.mapdisplay.Show()

    def OnAddVectorTheme(self, event):
        """!Add thematic vector map to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('thememap')

    def OnAddVectorChart(self, event):
        """!Add chart vector map to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('themechart')

    def OnAddOverlay(self, event):
        """!Create decoration overlay menu""" 
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay(show = True)

        self._popupMenu((('addGrid',     self.OnAddGrid),
                         ('addLabels',   self.OnAddLabels),
                         ('addGeodesic', self.OnAddGeodesic),
                         ('addRhumb',    self.OnAddRhumb),
                         (None, None),
                         ('addCmd',      self.OnAddCommand)))
        
        # show map display
        self.curr_page.maptree.mapdisplay.Show()
        
    def OnAddRaster3D(self, event):
        """!Add 3D raster map to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('3d-raster')

    def OnAddRasterRGB(self, event):
        """!Add RGB raster map to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('rgb')

    def OnAddRasterHIS(self, event):
        """!Add HIS raster map to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('his')

    def OnAddRasterShaded(self, event):
        """!Add shaded relief raster map to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('shaded')

    def OnAddRasterArrow(self, event):
        """!Add flow arrows raster map to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('rastarrow')

    def OnAddRasterNum(self, event):
        """!Add cell number raster map to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('rastnum')

    def OnAddCommand(self, event):
        """!Add command line map layer to the current layer tree"""
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay(show = True)

        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('command')

        # show map display
        self.curr_page.maptree.mapdisplay.Show()

    def OnAddGroup(self, event):
        """!Add layer group"""
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay(show = True)

        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('group')

        # show map display
        self.curr_page.maptree.mapdisplay.Show()

    def OnAddGrid(self, event):
        """!Add grid map layer to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('grid')

    def OnAddGeodesic(self, event):
        """!Add geodesic line map layer to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('geodesic')

    def OnAddRhumb(self, event):
        """!Add rhumb map layer to the current layer tree"""
        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('rhumb')

    def OnAddLabels(self, event):
        """!Add vector labels map layer to the current layer tree"""
        # start new map display if no display is available
        if not self.curr_page:
            self.NewDisplay(show = True)

        self.notebook.SetSelection(0)
        self.curr_page.maptree.AddLayer('labels')

        # show map display
        self.curr_page.maptree.mapdisplay.Show()

    def OnDeleteLayer(self, event):
        """!Remove selected map layer from the current layer Tree
        """
        if not self.curr_page or not self.curr_page.maptree.layer_selected:
            self.MsgNoLayerSelected()
            return

        if UserSettings.Get(group = 'manager', key = 'askOnRemoveLayer', subkey = 'enabled'):
            layerName = ''
            for item in self.curr_page.maptree.GetSelections():
                name = str(self.curr_page.maptree.GetItemText(item))
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

        for layer in self.curr_page.maptree.GetSelections():
            if self.curr_page.maptree.GetPyData(layer)[0]['type'] == 'group':
                self.curr_page.maptree.DeleteChildren(layer)
            self.curr_page.maptree.Delete(layer)
        
    def OnKeyDown(self, event):
        """!Key pressed"""
        kc = event.GetKeyCode()
        
        if event.ControlDown():
            if kc == wx.WXK_TAB:
                # switch layer list / command output
                if self.notebook.GetSelection() == 0:
                    self.notebook.SetSelection(1)
                else:
                    self.notebook.SetSelection(0)
        
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
        if not self.curr_page:
            self._auimgr.UnInit()
            self.Destroy()
            return
        
        maptree = self.curr_page.maptree
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
                    event.Veto()
                    dlg.Destroy()
                    return
                dlg.Destroy()
        
        # don't ask any more...
        UserSettings.Set(group = 'manager', key = 'askOnQuit', subkey = 'enabled',
                         value = False)
        
        self.OnDisplayCloseAll()
        
        self.gm_cb.DeleteAllPages()
        
        self._auimgr.UnInit()
        self.Destroy()
        
    def MsgNoLayerSelected(self):
        """!Show dialog message 'No layer selected'"""
        wx.MessageBox(parent = self,
                      message = _("No map layer selected. Operation cancelled."),
                      caption = _("Message"),
                      style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
    
class GMApp(wx.App):
    def __init__(self, workspace = None):
        """!Main GUI class.

        @param workspace path to the workspace file
        """
        self.workspaceFile = workspace
        
        # call parent class initializer
        wx.App.__init__(self, False)
        
        self.locale = wx.Locale(language = wx.LANGUAGE_DEFAULT)
        
    def OnInit(self):
        """!Initialize all available image handlers
        
        @return True
        """
        wx.InitAllImageHandlers()

        # create splash screen
        introImagePath = os.path.join(globalvar.ETCIMGDIR, "silesia_splash.png")
        introImage     = wx.Image(introImagePath, wx.BITMAP_TYPE_PNG)
        introBmp       = introImage.ConvertToBitmap()
        if SC:
            splash = SC.AdvancedSplash(bitmap = introBmp, 
                                       timeout = 2000, parent = None, id = wx.ID_ANY)
            splash.SetText(_('Starting GRASS GUI...'))
            splash.SetTextColour(wx.Colour(45, 52, 27))
            splash.SetTextFont(wx.Font(pointSize = 15, family = wx.DEFAULT, style = wx.NORMAL,
                                       weight = wx.BOLD))
            splash.SetTextPosition((150, 430))
        else:
            wx.SplashScreen (bitmap = introBmp, splashStyle = wx.SPLASH_CENTRE_ON_SCREEN | wx.SPLASH_TIMEOUT,
                             milliseconds = 2000, parent = None, id = wx.ID_ANY)
        
        wx.Yield()
        
        w, h = wx.GetDisplaySize()
        if globalvar.MAP_WINDOW_SIZE[0] + globalvar.GM_WINDOW_SIZE[0] > w:
            gmX = w - globalvar.GM_WINDOW_SIZE[0]
            dim = '%d,0,%d,%d,0,0,%d,%d' % \
                (gmX,
                 globalvar.GM_WINDOW_SIZE[0],
                 globalvar.GM_WINDOW_SIZE[1],
                 globalvar.MAP_WINDOW_SIZE[0],
                 globalvar.MAP_WINDOW_SIZE[1])
            UserSettings.Set(group = 'general', key = 'defWindowPos',
                             subkey = 'dim', value = dim)
        
        # create and show main frame
        mainframe = GMFrame(parent = None, id = wx.ID_ANY,
                            workspace = self.workspaceFile)

        mainframe.Show()
        self.SetTopWindow(mainframe)

        return True

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

def printHelp():
    """!Print program help"""
    print >> sys.stderr, "Usage:"
    print >> sys.stderr, " python wxgui.py [options]"
    print >> sys.stderr, "%sOptions:" % os.linesep
    print >> sys.stderr, " -w\t--workspace file\tWorkspace file to load"
    sys.exit(0)

def process_opt(opts, args):
    """!Process command-line arguments"""
    workspaceFile = None
    for o, a in opts:
        if o in ("-h", "--help"):
            printHelp()
            
        if o in ("-w", "--workspace"):
            if a != '':
                workspaceFile = str(a)
            else:
                workspaceFile = args.pop(0)

    return (workspaceFile,)

def main(argv = None):
    #
    # process command-line arguments
    #
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "hw:",
                                       ["help", "workspace"])
        except getopt.error, msg:
            raise Usage(msg)

    except Usage, err:
        print >> sys.stderr, err.msg
        print >> sys.stderr, "for help use --help"
        printHelp()

    workspaceFile = process_opt(opts, args)[0]

    #
    # run application
    #
    app = GMApp(workspaceFile)
    # suppress wxPython logs
    q = wx.LogNull()

    app.MainLoop()

if __name__ == "__main__":
    sys.exit(main())
