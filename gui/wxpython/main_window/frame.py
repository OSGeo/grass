"""
@package main_window::frame

@brief Single Window layout - main menu, main toolbars, dockable
panes for display management and access to command console.

Classes:
 - frame::GMFrame

(C) 2006-2021 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Jachym Cepicky (Mendel University of Agriculture)
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
"""

import sys
import os
import stat
import platform
import re

from core import globalvar

try:
    from agw import aui
except ImportError:
    import wx.lib.agw.aui as aui

import wx

try:
    import wx.lib.agw.flatnotebook as FN
except ImportError:
    import wx.lib.flatnotebook as FN

if os.path.join(globalvar.ETCDIR, "python") not in sys.path:
    sys.path.append(os.path.join(globalvar.ETCDIR, "python"))

from grass.script import core as grass
from grass.script.utils import decode

from core.gcmd import RunCommand, GError, GMessage
from core.settings import UserSettings, GetDisplayVectSettings
from core.utils import SetAddOnPath, GetLayerNameFromCmd, command2ltype, get_shell_pid
from gui_core.preferences import MapsetAccess, PreferencesDialog
from lmgr.layertree import LayerTree, LMIcons
from lmgr.menudata import LayerManagerMenuData, LayerManagerModuleTree
from gui_core.widgets import GNotebook
from core.gconsole import GConsole, EVT_IGNORED_CMD_RUN
from core.giface import Notification
from gui_core.goutput import GConsoleWindow, GC_PROMPT
from gui_core.dialogs import (
    LocationDialog,
    MapsetDialog,
    CreateNewVector,
    GroupDialog,
    MapLayersDialog,
    QuitDialog,
)
from gui_core.menu import SearchModuleWindow
from gui_core.menu import Menu as GMenu
from core.debug import Debug
from lmgr.toolbars import LMWorkspaceToolbar, LMToolsToolbar
from lmgr.toolbars import LMMiscToolbar, LMNvizToolbar, DisplayPanelToolbar
from lmgr.workspace import WorkspaceManager
from lmgr.pyshell import PyShellWindow
from lmgr.giface import LayerManagerGrassInterface
from datacatalog.catalog import DataCatalog
from gui_core.forms import GUI
from gui_core.wrap import Menu, TextEntryDialog
from startup.guiutils import (
    can_switch_mapset_interactive,
    switch_mapset_interactively,
    create_mapset_interactively,
    create_location_interactively,
)
from grass.grassdb.checks import is_first_time_user


class GMFrame(wx.Frame):
    """Single Window Layout which will be parallelly developed next to the
    current Multi Window layout solution."""

    def __init__(
        self,
        parent,
        id=wx.ID_ANY,
        title=None,
        workspace=None,
        size=globalvar.GM_WINDOW_SIZE,
        style=wx.DEFAULT_FRAME_STYLE,
        **kwargs,
    ):
        self.parent = parent
        if title:
            self.baseTitle = title
        else:
            self.baseTitle = _("GRASS GIS")

        self.iconsize = (16, 16)

        self.displayIndex = 0  # index value for map displays and layer trees
        self.currentPage = None  # currently selected page for layer tree notebook
        self.currentPageNum = (
            None  # currently selected page number for layer tree notebook
        )
        self.cwdPath = None  # current working directory

        wx.Frame.__init__(self, parent=parent, id=id, size=size, style=style, **kwargs)

        self._giface = LayerManagerGrassInterface(self)

        # workspace manager
        self.workspace_manager = WorkspaceManager(lmgr=self, giface=self._giface)
        self._setTitle()
        self.SetName("LayerManager")

        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )

        menu_errors = []

        def add_menu_error(message):
            menu_errors.append(message)

        def show_menu_errors(messages):
            if messages:
                self._gconsole.WriteError(
                    _("There were some issues when loading menu" " or Modules tab:")
                )
                for message in messages:
                    self._gconsole.WriteError(message)

        # the main menu bar
        self._menuTreeBuilder = LayerManagerMenuData(message_handler=add_menu_error)
        # the search tree and command console
        self._moduleTreeBuilder = LayerManagerModuleTree(message_handler=add_menu_error)
        self._auimgr = aui.AuiManager(self)

        # list of open dialogs
        self.dialogs = dict()
        self.dialogs["preferences"] = None
        self.dialogs["nvizPreferences"] = None
        self.dialogs["atm"] = list()

        # set pane sizes according to the full screen size of the primary monitor
        size = wx.Display().GetGeometry().GetSize()
        self.PANE_BEST_SIZE = tuple(t / 5 for t in size)
        self.PANE_MIN_SIZE = tuple(t / 10 for t in size)

        # create widgets and build panes
        self.CreateMenuBar()
        self.CreateStatusBar(number=1)
        self.BuildPanes()
        self.BindEvents()

        self._giface.mapCreated.connect(self.OnMapCreated)
        self._giface.updateMap.connect(self._updateCurrentMap)
        self._giface.currentMapsetChanged.connect(self.OnMapsetChanged)

        # use default window layout ?
        if UserSettings.Get(group="general", key="defWindowPos", subkey="enabled"):
            dim = UserSettings.Get(group="general", key="defWindowPos", subkey="dim")
            try:
                x, y = map(int, dim.split(",")[0:2])
                w, h = map(int, dim.split(",")[2:4])
                self.SetPosition((x, y))
                self.SetSize((w, h))
            except Exception:
                pass
        else:
            # does center (of screen) make sense for lmgr?
            self.Centre()

        self.Layout()
        self.Show()
        self.Maximize(True)

        # load workspace file if requested
        if workspace:
            if self.workspace_manager.Load(workspace):
                self._setTitle()
        else:
            # start default initial display
            self.NewDisplay(show=False)

        # show map display window
        # -> OnSize() -> UpdateMap()
        for mapdisp in self.GetMapDisplay(onlyCurrent=False):
            mapdisp.Show()

        # redirect stderr to log area
        self._gconsole.Redirect()

        # fix goutput's pane size (required for Mac OSX)`
        self.goutput.SetSashPosition(int(self.GetSize()[1] * 0.8))

        show_menu_errors(menu_errors)

        # start with layer manager on top
        if self.currentPage:
            self.GetMapDisplay().Raise()
        wx.CallAfter(self.Raise)

        self._show_demo_map()

    def _setTitle(self):
        """Set frame title"""
        gisenv = grass.gisenv()
        location = gisenv["LOCATION_NAME"]
        mapset = gisenv["MAPSET"]
        if self.workspace_manager.workspaceFile:
            filename = os.path.splitext(
                os.path.basename(self.workspace_manager.workspaceFile)
            )[0]
            self.SetTitle(
                "{workspace} - {location}/{mapset} - {program}".format(
                    location=location,
                    mapset=mapset,
                    workspace=filename,
                    program=self.baseTitle,
                )
            )
        else:
            self.SetTitle(
                "{location}/{mapset} - {program}".format(
                    location=location, mapset=mapset, program=self.baseTitle
                )
            )

    def CreateMenuBar(self):
        """Creates menu bar"""
        self.menubar = GMenu(
            parent=self, model=self._menuTreeBuilder.GetModel(separators=True)
        )
        self.SetMenuBar(self.menubar)
        self.menucmd = self.menubar.GetCmd()

    def _createTabMenu(self):
        """Creates context menu for display tabs.

        Used to rename display.
        """
        menu = Menu()
        item = wx.MenuItem(menu, id=wx.ID_ANY, text=_("Rename current Map Display"))
        menu.AppendItem(item)
        self.Bind(wx.EVT_MENU, self.OnRenameDisplay, item)

        return menu

    def _createDisplayPanel(self, parent):
        """Creates display panel"""
        # create superior display panel
        displayPanel = wx.Panel(parent, id=wx.ID_ANY)
        # create display toolbar
        dmgrToolbar = DisplayPanelToolbar(guiparent=displayPanel, parent=self)
        # create display notebook
        notebookLayers = GNotebook(parent=displayPanel, style=globalvar.FNPageStyle)
        notebookLayers.SetTabAreaColour(globalvar.FNPageColor)
        menu = self._createTabMenu()
        notebookLayers.SetRightClickMenu(menu)

        # layout
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(dmgrToolbar, proportion=0, flag=wx.EXPAND)
        sizer.Add(notebookLayers, proportion=1, flag=wx.EXPAND)

        displayPanel.SetAutoLayout(True)
        displayPanel.SetSizer(sizer)
        displayPanel.Fit()
        displayPanel.Layout()

        return displayPanel, notebookLayers

    def _setCopyingOfSelectedText(self):
        copy = UserSettings.Get(
            group="manager", key="copySelectedTextToClipboard", subkey="enabled"
        )
        self.goutput.SetCopyingOfSelectedText(copy)

    def IsPaneShown(self, name):
        """Check if pane (toolbar, ...) of given name is currently shown"""
        if self._auimgr.GetPane(name).IsOk():
            return self._auimgr.GetPane(name).IsShown()
        return False

    def _createDataCatalog(self, parent):
        """Initialize Data Catalog widget"""
        self.datacatalog = DataCatalog(parent=parent, giface=self._giface)
        self.datacatalog.showNotification.connect(
            lambda message: self.SetStatusText(message)
        )

    def _createDisplay(self, parent):
        """Initialize Display widget"""
        self.displayPanel, self.notebookLayers = self._createDisplayPanel(parent)
        # bindings
        self.notebookLayers.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnCBPageChanged)
        self.notebookLayers.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CLOSING, self.OnCBPageClosing)

    def _createSearchModule(self, parent):
        """Initialize Search module widget"""
        if not UserSettings.Get(group="manager", key="hideTabs", subkey="search"):
            self.search = SearchModuleWindow(
                parent=parent,
                handlerObj=self,
                giface=self._giface,
                model=self._moduleTreeBuilder.GetModel(),
            )
            self.search.showNotification.connect(
                lambda message: self.SetStatusText(message)
            )
        else:
            self.search = None

    def _createConsole(self, parent):
        """Initialize Console widget"""
        # create 'command output' text area
        self._gconsole = GConsole(
            guiparent=self,
            giface=self._giface,
            ignoredCmdPattern=r"^d\..*|^r[3]?\.mapcalc$|^i.group$|^r.import$|"
            "^r.external$|^r.external.out$|"
            "^v.import$|^v.external$|^v.external.out$|"
            "^cd$|^cd .*",
        )
        # create 'console' widget
        self.goutput = GConsoleWindow(
            parent=parent,
            giface=self._giface,
            gconsole=self._gconsole,
            menuModel=self._moduleTreeBuilder.GetModel(),
            gcstyle=GC_PROMPT,
        )
        self.goutput.showNotification.connect(
            lambda message: self.SetStatusText(message)
        )

        self._gconsole.mapCreated.connect(self.OnMapCreated)
        self._gconsole.Bind(
            EVT_IGNORED_CMD_RUN, lambda event: self.RunSpecialCmd(event.cmd)
        )

        self._setCopyingOfSelectedText()

    def _createPythonShell(self, parent):
        """Initialize Python shell widget"""
        if not UserSettings.Get(group="manager", key="hideTabs", subkey="pyshell"):
            self.pyshell = PyShellWindow(
                parent=parent,
                giface=self._giface,
                simpleEditorHandler=self.OnSimpleEditor,
            )
        else:
            self.pyshell = None

    def _createMapDisplay(self, parent):
        """Set up Map Display"""
        # blank panel for testing
        self.mapdisplay = wx.Panel(parent=parent)

    def BuildPanes(self):
        """Build panes - toolbars as well as panels"""

        # initialize all main widgets
        self._createDataCatalog(parent=self)
        self._createDisplay(parent=self)
        self._createSearchModule(parent=self)
        self._createConsole(parent=self)
        self._createPythonShell(parent=self)
        self._createMapDisplay(parent=self)
        self.toolbars = {
            "workspace": LMWorkspaceToolbar(parent=self),
            "tools": LMToolsToolbar(parent=self),
            "misc": LMMiscToolbar(parent=self),
            "nviz": LMNvizToolbar(parent=self),
        }
        self._toolbarsData = {
            "workspace": (
                "toolbarWorkspace",  # name
                _("Workspace Toolbar"),  # caption
                1,
                0,
            ),  # row, position
            "tools": ("toolbarTools", _("Tools Toolbar"), 1, 1),
            "misc": ("toolbarMisc", _("Misc Toolbar"), 1, 2),
            "nviz": ("toolbarNviz", _("3D view Toolbar"), 1, 3),
        }

        # add a bunch of panes
        toolbarsList = ("workspace", "tools", "misc", "nviz")
        for toolbar in toolbarsList:
            name, caption, row, position = self._toolbarsData[toolbar]
            self._auimgr.AddPane(
                self.toolbars[toolbar],
                aui.AuiPaneInfo()
                .Name(name)
                .Caption(caption)
                .ToolbarPane()
                .Top()
                .Row(row)
                .Position(position)
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2)
                .BestSize((self.toolbars[toolbar].GetBestSize())),
            )

        self._auimgr.AddPane(
            self.mapdisplay,
            aui.AuiPaneInfo().Name("map display").CenterPane().PaneBorder(True),
        )

        self._auimgr.AddPane(
            self.datacatalog,
            aui.AuiPaneInfo()
            .Name("datacatalog")
            .Caption("Data Catalog")
            .Left()
            .Layer(1)
            .Position(1)
            .BestSize(self.PANE_BEST_SIZE)
            .MinSize(self.PANE_MIN_SIZE)
            .CloseButton(False)
            .MinimizeButton(True)
            .MaximizeButton(True),
        )

        self._auimgr.AddPane(
            self.displayPanel,
            aui.AuiPaneInfo()
            .Name("display")
            .Caption("Display")
            .Left()
            .Layer(1)
            .Position(2)
            .BestSize(self.PANE_BEST_SIZE)
            .MinSize(self.PANE_MIN_SIZE)
            .CloseButton(False)
            .MinimizeButton(True)
            .MaximizeButton(True),
        )

        self._auimgr.AddPane(
            self.search,
            aui.AuiPaneInfo()
            .Name("modules")
            .Caption("Modules")
            .Right()
            .Layer(2)
            .Position(1)
            .BestSize(self.PANE_BEST_SIZE)
            .MinSize(self.PANE_MIN_SIZE)
            .CloseButton(False)
            .MinimizeButton(True)
            .MaximizeButton(True),
        )

        self._auimgr.AddPane(
            self.goutput,
            aui.AuiPaneInfo()
            .Name("console")
            .Caption("Console")
            .Right()
            .Layer(2)
            .Position(2)
            .BestSize(self.PANE_BEST_SIZE)
            .MinSize(self.PANE_MIN_SIZE)
            .CloseButton(False)
            .MinimizeButton(True)
            .MaximizeButton(True),
        )

        self._auimgr.AddPane(
            self.pyshell,
            aui.AuiPaneInfo()
            .Name("python")
            .Caption("Python")
            .Right()
            .Layer(2)
            .Position(3)
            .BestSize(self.PANE_BEST_SIZE)
            .MinSize(self.PANE_MIN_SIZE)
            .CloseButton(False)
            .MinimizeButton(True)
            .MaximizeButton(True),
        )

        self._auimgr.GetPane("toolbarNviz").Hide()
        wx.CallAfter(self.datacatalog.LoadItems)

        self._auimgr.Update()

    def BindEvents(self):
        # bindings
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindowOrExit)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)

    def _show_demo_map(self):
        """If in demolocation, add demo map to map display

        This provides content for first-time user experience.
        """

        def show_demo():
            layer_name = "country_boundaries@PERMANENT"
            exists = grass.find_file(name=layer_name, element="vector")["name"]
            if not exists:
                # Do not fail nor report errors to the first-time user when not found.
                Debug.msg(
                    5, "GMFrame._show_demo_map(): {} does not exist".format(layer_name)
                )
                return
            self.GetLayerTree().AddLayer(
                ltype="vector",
                lname=layer_name,
                lchecked=True,
                lcmd=["d.vect", "map={}".format(layer_name)],
            )

        if is_first_time_user():
            # Show only after everything is initialized for proper map alignment.
            wx.CallLater(1000, show_demo)

    def AddNvizTools(self, firstTime):
        """Add nviz notebook page

        :param firstTime: if a mapdisplay is starting 3D mode for the
                          first time
        """
        Debug.msg(5, "GMFrame.AddNvizTools()")
        from nviz.main import haveNviz

        if not haveNviz:
            return

        from nviz.main import NvizToolWindow

        # show toolbar
        self._auimgr.GetPane("toolbarNviz").Show()
        # reorder other toolbars
        for pos, toolbar in enumerate(
            ("toolbarWorkspace", "toolbarTools", "toolbarMisc", "toolbarNviz")
        ):
            self._auimgr.GetPane(toolbar).Row(1).Position(pos)
        self._auimgr.Update()

        # create nviz tools tab
        self.nviz = NvizToolWindow(
            parent=self.notebook, tree=self.GetLayerTree(), display=self.GetMapDisplay()
        )
        idx = self.notebook.GetPageIndexByName("layers")
        self.notebook.InsertNBPage(
            index=idx + 1, page=self.nviz, text=_("3D view"), name="nviz"
        )
        self.notebook.SetSelectionByName("nviz")

        # this is a bit strange here since a new window is created everytime
        if not firstTime:
            for page in ("view", "light", "fringe", "constant", "cplane", "animation"):
                self.nviz.UpdatePage(page)

    def RemoveNvizTools(self):
        """Remove nviz notebook page"""
        # if more mapwindow3D were possible, check here if nb page should be
        # removed
        self.notebook.SetSelectionByName("layers")
        self.notebook.DeleteNBPage("nviz")

        # hide toolbar
        self._auimgr.GetPane("toolbarNviz").Hide()
        for pos, toolbar in enumerate(
            ("toolbarWorkspace", "toolbarTools", "toolbarMisc")
        ):
            self._auimgr.GetPane(toolbar).Row(1).Position(pos)
        self._auimgr.Update()

    def OnLocationWizard(self, event):
        """Launch location wizard"""
        gisenv = grass.gisenv()
        grassdb, location, mapset = create_location_interactively(
            self, gisenv["GISDBASE"]
        )
        if location:
            self._giface.grassdbChanged.emit(
                grassdb=grassdb, location=location, action="new", element="location"
            )
            if grassdb == gisenv["GISDBASE"]:
                switch_grassdb = None
            else:
                switch_grassdb = grassdb
            if can_switch_mapset_interactive(self, grassdb, location, mapset):
                switch_mapset_interactively(
                    self,
                    self._giface,
                    switch_grassdb,
                    location,
                    mapset,
                    show_confirmation=True,
                )

    def OnSettingsChanged(self):
        """Here can be functions which have to be called
        after receiving settingsChanged signal.
        Now only set copying of selected text to clipboard (in goutput).
        """
        # self._createMenuBar() # bug when menu is re-created on the fly
        self._setCopyingOfSelectedText()

    def OnGCPManager(self, event=None, cmd=None):
        """Launch georectifier module. See OnIClass documentation"""
        from gcp.manager import GCPWizard

        GCPWizard(self, self._giface)

    def OnGModeler(self, event=None, cmd=None):
        """Launch Graphical Modeler. See OnIClass documentation"""
        from gmodeler.frame import ModelFrame

        win = ModelFrame(parent=self, giface=self._giface)
        win.CentreOnScreen()
        win.Show()

    def OnPsMap(self, event=None, cmd=None):
        """Launch Cartographic Composer. See OnIClass documentation"""
        from psmap.frame import PsMapFrame

        win = PsMapFrame(parent=self)
        win.CentreOnScreen()
        win.Show()

    def OnMapSwipe(self, event=None, cmd=None):
        """Launch Map Swipe. See OnIClass documentation"""
        from mapswipe.frame import SwipeMapFrame

        win = SwipeMapFrame(parent=self, giface=self._giface)

        rasters = []
        tree = self.GetLayerTree()
        if tree:
            for layer in tree.GetSelections():
                if tree.GetLayerInfo(layer, key="maplayer").GetType() != "raster":
                    continue
                rasters.append(tree.GetLayerInfo(layer, key="maplayer").GetName())

        if len(rasters) >= 1:
            win.SetFirstRaster(rasters[0])
        if len(rasters) >= 2:
            win.SetSecondRaster(rasters[1])
            win.SetRasterNames()

        win.CentreOnScreen()
        win.Show()

    def OnRLiSetup(self, event=None, cmd=None):
        """Launch r.li setup. See OnIClass documentation"""
        from rlisetup.frame import RLiSetupFrame

        win = RLiSetupFrame(parent=self)
        win.CentreOnScreen()
        win.Show()

    def OnDataCatalog(self, event=None, cmd=None):
        """Launch Data Catalog"""
        from datacatalog.frame import DataCatalogFrame

        win = DataCatalogFrame(parent=self, giface=self._giface)
        win.CentreOnScreen()
        win.Show()

    def OnDone(self, event):
        """Command execution finished"""
        if hasattr(self, "model"):
            self.model.DeleteIntermediateData(log=self._gconsole)
            del self.model
        self.SetStatusText("")

    def OnRunModel(self, event):
        """Run model"""
        filename = ""
        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose model to run"),
            defaultDir=os.getcwd(),
            wildcard=_("GRASS Model File (*.gxm)|*.gxm"),
        )
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if not filename:
            dlg.Destroy()
            return

        from gmodeler.model import Model

        self.model = Model()
        self.model.LoadModel(filename)
        self.model.Run(log=self.GetLogWindow(), onDone=self.OnDone, parent=self)
        dlg.Destroy()

    def OnMapsets(self, event):
        """Launch mapset access dialog"""
        dlg = MapsetAccess(parent=self, id=wx.ID_ANY)
        dlg.CenterOnScreen()

        if dlg.ShowModal() == wx.ID_OK:
            ms = dlg.GetMapsets()
            RunCommand(
                "g.mapsets", parent=self, mapset="%s" % ",".join(ms), operation="set"
            )

    def OnCBPageChanged(self, event):
        """Page in notebook (display) changed"""
        self.currentPage = self.notebookLayers.GetCurrentPage()
        self.currentPageNum = self.notebookLayers.GetSelection()
        try:
            self.GetMapDisplay().SetFocus()
            self.GetMapDisplay().Raise()
        except Exception:
            pass

        event.Skip()

    def OnCBPageClosing(self, event):
        """Page of notebook is being closed
        from Layer Manager (x button next to arrows)
        Also close associated map display.
        """
        # save changes in the workspace
        name = self.notebookLayers.GetPageText(event.GetSelection())
        caption = _("Close Map Display {}").format(name)
        if not self.workspace_manager.CanClosePage(caption):
            event.Veto()
            return

        maptree = self.notebookLayers.GetPage(event.GetSelection()).maptree
        maptree.GetMapDisplay().CleanUp()
        maptree.Close(True)

        self.currentPage = None

        event.Skip()

    def _closePageNoEvent(self, page_index):
        """Close page and destroy map display without
        generating notebook page closing event"""
        self.notebookLayers.Unbind(FN.EVT_FLATNOTEBOOK_PAGE_CLOSING)
        self.notebookLayers.DeletePage(page_index)
        self.notebookLayers.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CLOSING, self.OnCBPageClosing)

    def RunSpecialCmd(self, command):
        """Run command from command line, check for GUI wrappers"""
        if re.compile(r"^d\..*").search(command[0]):
            self.RunDisplayCmd(command)
        elif re.compile(r"r[3]?\.mapcalc").search(command[0]):
            self.OnMapCalculator(event=None, cmd=command)
        elif command[0] == "i.group":
            self.OnEditImageryGroups(event=None, cmd=command)
        elif command[0] == "r.import":
            self.OnImportGdalLayers(event=None, cmd=command)
        elif command[0] == "r.external":
            self.OnLinkGdalLayers(event=None, cmd=command)
        elif command[0] == "r.external.out":
            self.OnRasterOutputFormat(event=None)
        elif command[0] == "v.import":
            self.OnImportOgrLayers(event=None, cmd=command)
        elif command[0] == "v.external":
            self.OnLinkOgrLayers(event=None, cmd=command)
        elif command[0] == "v.external.out":
            self.OnVectorOutputFormat(event=None)
        elif command[0] == "cd":
            self.OnChangeCWD(event=None, cmd=command)
        else:
            raise ValueError(
                "Layer Manager special command (%s)"
                " not supported." % " ".join(command)
            )

    def RunDisplayCmd(self, command):
        """Handles display commands.

        :param command: command in a list
        """
        if not self.currentPage:
            self.NewDisplay(show=True)
        # here should be the d.* commands which are not layers
        if command[0] == "d.erase":
            # rest of d.erase is ignored
            self.GetLayerTree().DeleteAllLayers()
            return
        try:
            # display GRASS commands
            layertype = command2ltype[command[0]]
        except KeyError:
            GMessage(
                parent=self,
                message=_(
                    "Command '%s' not yet implemented in the WxGUI. "
                    "Try adding it as a command layer instead."
                )
                % command[0],
            )
            return

        if layertype == "barscale":
            if len(command) > 1:
                self.GetMapDisplay().AddBarscale(cmd=command)
            else:
                self.GetMapDisplay().AddBarscale()
        elif layertype == "rastleg":
            if len(command) > 1:
                self.GetMapDisplay().AddLegendRast(cmd=command)
            else:
                self.GetMapDisplay().AddLegendRast()
        elif layertype == "vectleg":
            if len(command) > 1:
                self.GetMapDisplay().AddLegendVect(cmd=command, showDialog=False)
            else:
                self.GetMapDisplay().AddLegendVect(showDialog=True)
        elif layertype == "northarrow":
            if len(command) > 1:
                self.GetMapDisplay().AddArrow(cmd=command)
            else:
                self.GetMapDisplay().AddArrow()
        elif layertype == "text":
            if len(command) > 1:
                self.GetMapDisplay().AddDtext(cmd=command)
            else:
                self.GetMapDisplay().AddDtext()
        elif layertype == "redraw":
            self.GetMapDisplay().OnRender(None)
        elif layertype == "export":
            GUI(parent=self, show=False).ParseCommand(
                command, completed=(self.GetMapDisplay().DOutFileOptData, "", "")
            )
        elif layertype == "torast":
            if len(command) <= 1:
                task = GUI(parent=self, show=True).ParseCommand(
                    command, completed=(self.GetMapDisplay().DToRastOptData, "", "")
                )
            else:
                task = GUI(parent=self, show=None).ParseCommand(
                    command, completed=(self.GetMapDisplay().DToRastOptData, "", "")
                )
                self.GetMapDisplay().DToRast(command=task.get_cmd())
        else:
            # add layer into layer tree
            lname, found = GetLayerNameFromCmd(
                command, fullyQualified=True, layerType=layertype
            )
            self.GetLayerTree().AddLayer(
                ltype=layertype,
                lchecked=True if lname else None,
                lname=lname,
                lcmd=command,
            )

    def GetLayerNotebook(self):
        """Get Layers Notebook"""
        return self.notebookLayers

    def GetLayerTree(self):
        """Get current layer tree

        :return: LayerTree instance
        :return: None no layer tree selected
        """
        if self.currentPage:
            return self.currentPage.maptree
        return None

    def GetMapDisplay(self, onlyCurrent=True):
        """Get current map display

        :param bool onlyCurrent: True to return only active mapdisplay
                                 False for list of all mapdisplays

        :return: MapFrame instance (or list)
        :return: None no mapdisplay selected
        """
        if onlyCurrent:
            if self.currentPage:
                return self.GetLayerTree().GetMapDisplay()
            else:
                return None
        else:  # -> return list of all mapdisplays
            mlist = list()
            for idx in range(0, self.notebookLayers.GetPageCount()):
                mlist.append(self.notebookLayers.GetPage(idx).maptree.GetMapDisplay())

            return mlist

    def GetAllMapDisplays(self):
        """Get all (open) map displays"""
        return self.GetMapDisplay(onlyCurrent=False)

    def GetLogWindow(self):
        """Gets console for command output and messages"""
        return self._gconsole

    def GetToolbar(self, name):
        """Returns toolbar if exists else None"""
        if name in self.toolbars:
            return self.toolbars[name]

        return None

    def GetMenuCmd(self, event):
        """Get GRASS command from menu item

        :return: command as a list"""
        layer = None
        if event:
            cmd = self.menucmd[event.GetId()]
        else:
            cmd = ""

        try:
            cmdlist = cmd.split(" ")
        except Exception:  # already list?
            cmdlist = cmd

        # check list of dummy commands for GUI modules that do not have GRASS
        # bin modules or scripts.
        if cmd in ["vcolors", "r.mapcalc", "r3.mapcalc"]:
            return cmdlist

        try:
            layer = self.GetLayerTree().layer_selected
            name = self.GetLayerTree().GetLayerInfo(layer, key="maplayer").name
            type = self.GetLayerTree().GetLayerInfo(layer, key="type")
        except Exception:
            layer = None

        if layer and len(cmdlist) == 1:  # only if no parameters given
            if (type == "raster" and cmdlist[0][0] == "r" and cmdlist[0][1] != "3") or (
                type == "vector" and cmdlist[0][0] == "v"
            ):
                input = GUI().GetCommandInputMapParamKey(cmdlist[0])
                if input:
                    cmdlist.append("%s=%s" % (input, name))

        return cmdlist

    def RunMenuCmd(self, event=None, cmd=[]):
        """Run command selected from menu"""
        if event:
            cmd = self.GetMenuCmd(event)
        self._gconsole.RunCmd(cmd)

    def OnMenuCmd(self, event=None, cmd=[]):
        """Parse command selected from menu"""
        if event:
            cmd = self.GetMenuCmd(event)
        GUI(parent=self, giface=self._giface).ParseCommand(cmd)

    def OnVNet(self, event):
        """Vector network analysis tool"""
        if self.GetMapDisplay():
            self.GetMapDisplay().OnVNet(event)
        else:
            self.NewDisplay(show=True).OnVNet(event)

    def OnVDigit(self, event):
        """Start vector digitizer"""
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
            mapLayer = tree.GetLayerInfo(layer, key="maplayer")
        except Exception:
            mapLayer = None

        if not mapLayer or mapLayer.GetType() != "vector":
            GMessage(parent=self, message=_("Selected map layer is not vector."))
            return

        if mapLayer.GetMapset() != grass.gisenv()["MAPSET"]:
            GMessage(
                parent=self,
                message=_(
                    "Editing is allowed only for vector maps from the "
                    "current mapset."
                ),
            )
            return

        if not tree.GetLayerInfo(layer):
            return
        dcmd = tree.GetLayerInfo(layer, key="cmd")
        if not dcmd:
            return

        digitToolbar = self.GetMapDisplay().GetToolbar("vdigit")
        if digitToolbar:
            stopOnly = False
            if mapLayer is digitToolbar.GetLayer():
                stopOnly = True
            tree.OnStopEditing(None)  # TODO: change to signal
            if stopOnly:
                return

        tree.OnStartEditing(None)  # TODO: change to signal

    def OnRunScript(self, event):
        """Run user-defined script"""
        # open dialog and choose script file
        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose script file to run"),
            defaultDir=os.getcwd(),
            wildcard=_("Python script (*.py)|*.py|Bash script (*.sh)|*.sh"),
        )

        filename = None
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if not filename:
            return False

        if not os.path.exists(filename):
            GError(
                parent=self,
                message=_("Script file '%s' doesn't exist. " "Operation canceled.")
                % filename,
            )
            return

        # check permission
        if not os.access(filename, os.X_OK):
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Script <%s> is not executable. "
                    "Do you want to set the permissions "
                    "that allows you to run this script "
                    "(note that you must be the owner of the file)?"
                    % os.path.basename(filename)
                ),
                caption=_("Set permission?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
            )
            if dlg.ShowModal() != wx.ID_YES:
                return
            dlg.Destroy()
            try:
                mode = stat.S_IMODE(os.lstat(filename)[stat.ST_MODE])
                os.chmod(filename, mode | stat.S_IXUSR)
            except OSError:
                GError(_("Unable to set permission. Operation canceled."), parent=self)
                return

        # check GRASS_ADDON_PATH
        addonPath = os.getenv("GRASS_ADDON_PATH", [])
        if addonPath:
            addonPath = addonPath.split(os.pathsep)
        dirName = os.path.dirname(filename)
        if dirName not in addonPath:
            addonPath.append(dirName)
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Directory '%s' is not defined in GRASS_ADDON_PATH. "
                    "Do you want add this directory to GRASS_ADDON_PATH?"
                )
                % dirName,
                caption=_("Update Addons path?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
            )
            if dlg.ShowModal() == wx.ID_YES:
                SetAddOnPath(os.pathsep.join(addonPath), key="PATH")
            dlg.Destroy()

        self._gconsole.WriteCmdLog(_("Launching script '%s'...") % filename)
        self._gconsole.RunCmd([filename])

    def OnChangeLocation(self, event):
        """Change current location"""
        dlg = LocationDialog(parent=self)
        gisenv = grass.gisenv()

        if dlg.ShowModal() == wx.ID_OK:
            location, mapset = dlg.GetValues()
            dlg.Destroy()

            if not location or not mapset:
                GError(
                    parent=self,
                    message=_("No location/mapset provided. Operation canceled."),
                )
                return  # this should not happen
            if can_switch_mapset_interactive(
                self, gisenv["GISDBASE"], location, mapset
            ):
                switch_mapset_interactively(self, self._giface, None, location, mapset)

    def OnCreateMapset(self, event):
        """Create new mapset"""
        gisenv = grass.gisenv()
        mapset = create_mapset_interactively(
            self, gisenv["GISDBASE"], gisenv["LOCATION_NAME"]
        )
        if mapset:
            self._giface.grassdbChanged.emit(
                grassdb=gisenv["GISDBASE"],
                location=gisenv["LOCATION_NAME"],
                mapset=mapset,
                action="new",
                element="mapset",
            )
            if can_switch_mapset_interactive(
                self, gisenv["GISDBASE"], gisenv["LOCATION_NAME"], mapset
            ):
                switch_mapset_interactively(
                    self, self._giface, None, None, mapset, show_confirmation=True
                )

    def OnChangeMapset(self, event):
        """Change current mapset"""
        dlg = MapsetDialog(parent=self)
        gisenv = grass.gisenv()

        if dlg.ShowModal() == wx.ID_OK:
            mapset = dlg.GetMapset()
            dlg.Destroy()

            if not mapset:
                GError(
                    parent=self, message=_("No mapset provided. Operation canceled.")
                )
                return
            if can_switch_mapset_interactive(
                self, gisenv["GISDBASE"], gisenv["LOCATION_NAME"], mapset
            ):
                switch_mapset_interactively(self, self._giface, None, None, mapset)

    def OnMapsetChanged(self, dbase, location, mapset):
        """Current mapset changed.
        If location is None, mapset changed within location.
        """
        if not location:
            self._setTitle()
        else:
            # close current workspace and create new one
            self.OnWorkspaceClose()
            self.OnWorkspaceNew()

    def OnChangeCWD(self, event=None, cmd=None):
        """Change current working directory

        :param event: to be able to serve as a handler of wx event
        :param cmd: command as a list (must start with 'cd')
        """
        # local functions
        def write_beginning(parameter=None, command=None):
            if parameter:
                self._giface.WriteCmdLog('cd "' + parameter + '"')
            else:
                # naive concat but will be enough most of the time
                self._giface.WriteCmdLog(" ".join(command))

        def write_changed():
            self._giface.WriteLog(
                _('Working directory changed to:\n"%s"') % os.getcwd()
            )

        def write_end():
            self._giface.WriteCmdLog(" ")

        def write_help():
            self._giface.WriteLog(
                _("Changes current working directory" " for this GUI.")
            )
            self._giface.WriteLog(_("Usage: cd [directory]"))
            self._giface.WriteLog(_("Without parameters it opens a dialog."))
            # TODO: the following is longer then 80 chars
            # but this should be solved by the function not caller
            # also because of translations
            self._giface.WriteLog(
                _(
                    "If ~ (tilde) is present as the first"
                    " directory on the path, it is replaced"
                    " by user's home directory."
                )
            )

        # check correctness of cmd
        if cmd and cmd[0] != "cd":
            # this is programmer's error
            # can be relaxed in future
            # but keep it strict unless needed otherwise
            raise ValueError(
                "OnChangeCWD cmd parameter must be list of"
                " length 1 or 2 and 'cd' as a first item"
            )
        if cmd and len(cmd) > 2:
            # this might be a user error
            write_beginning(command=cmd)
            self._giface.WriteError(_("More than one parameter provided."))
            write_help()
            write_end()
            return
        # use chdir or dialog
        if cmd and len(cmd) == 2:
            write_beginning(parameter=cmd[1])
            if cmd[1] in ["-h", "--h", "--help", "help"]:
                write_help()
                write_end()
                return
            try:
                path = os.path.expanduser(cmd[1])
                os.chdir(path)
                write_changed()
            except OSError as error:
                self._giface.WriteError(str(error))
            write_end()
        else:
            dlg = wx.DirDialog(
                parent=self,
                message=_("Choose a working directory"),
                defaultPath=os.getcwd(),
            )

            if dlg.ShowModal() == wx.ID_OK:
                self.cwdPath = dlg.GetPath()  # is saved in the workspace
                write_beginning(parameter=self.cwdPath)
                os.chdir(self.cwdPath)
                write_changed()
                write_end()

    def GetCwdPath(self):
        """Get current working directory or None"""
        return self.cwdPath

    def OnNewVector(self, event):
        """Create new vector map layer"""
        dlg = CreateNewVector(
            self, giface=self._giface, cmd=(("v.edit", {"tool": "create"}, "map"))
        )

        if not dlg:
            return

        name = dlg.GetName(full=True)
        if name and dlg.IsChecked("add"):
            # add layer to map layer tree
            self.GetLayerTree().AddLayer(
                ltype="vector",
                lname=name,
                lchecked=True,
                lcmd=["d.vect", "map=%s" % name],
            )
        dlg.Destroy()

    def OnSystemInfo(self, event):
        """Print system information"""
        vInfo = grass.version()
        if not vInfo:
            sys.stderr.write(_("Unable to get GRASS version\n"))

        # check also OSGeo4W on MS Windows
        if sys.platform == "win32" and not os.path.exists(
            os.path.join(os.getenv("GISBASE"), "WinGRASS-README.url")
        ):
            osgeo4w = " (OSGeo4W)"
        else:
            osgeo4w = ""

        self._gconsole.WriteCmdLog(_("System Info"))
        # platform decoding was added because of the Fedora 19 release
        # which has the name "Schrödinger’s cat" (umlaut and special ' character)
        # which appears in the platform.platform() string
        platform_ = decode(platform.platform())
        self._gconsole.WriteLog(
            "%s: %s\n" "%s: %s\n" "%s: %s\n" "%s: %s\n"
            # "%s: %s (%s)\n"
            "GDAL: %s\n"
            "PROJ: %s\n"
            "GEOS: %s\n"
            "SQLite: %s\n"
            "Python: %s\n"
            "wxPython: %s\n"
            "%s: %s%s\n"
            % (
                _("GRASS version"),
                vInfo.get("version", _("unknown version")),
                _("Code revision"),
                vInfo.get("revision", "?"),
                _("Build date"),
                vInfo.get("build_date", "?"),
                _("Build platform"),
                vInfo.get("build_platform", "?"),
                # _("GIS Library Revision"),
                # vInfo.get('libgis_revision'],
                # vInfo.get('libgis_date'].split('
                # ', 1)[0],
                vInfo.get("gdal", "?"),
                vInfo.get("proj", "?"),
                vInfo.get("geos", "?"),
                vInfo.get("sqlite", "?"),
                platform.python_version(),
                wx.__version__,
                _("Platform"),
                platform_,
                osgeo4w,
            ),
            notification=Notification.MAKE_VISIBLE,
        )
        self._gconsole.WriteCmdLog(" ")

    def OnAboutGRASS(self, event):
        """Display 'About GRASS' dialog"""
        from gui_core.ghelp import AboutWindow

        win = AboutWindow(self)
        win.CentreOnScreen()
        win.Show(True)

    def _popupMenu(self, data):
        """Create popup menu"""
        menu = Menu()

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

    def OnWorkspaceNew(self, event=None):
        """Create new workspace file"""
        self.workspace_manager.New()

    def OnWorkspaceOpen(self, event=None):
        """Open file with workspace definition"""
        self.workspace_manager.Open()

    def OnWorkspaceSave(self, event=None):
        """Save file with workspace definition"""
        self.workspace_manager.Save()

    def OnWorkspaceSaveAs(self, event=None):
        """Save workspace definition to selected file"""
        self.workspace_manager.SaveAs()

    def OnWorkspaceClose(self, event=None):
        """Close file with workspace definition"""
        self.workspace_manager.Close()

    def OnDisplayClose(self, event=None):
        """Close current map display window"""
        if self.currentPage and self.GetMapDisplay():
            self.GetMapDisplay().OnCloseWindow(event)

    def OnDisplayCloseAll(self, event):
        """Close all open map display windows (from menu)"""
        if not self.workspace_manager.CanClosePage(caption=_("Close all Map Displays")):
            return
        self.DisplayCloseAll()

    def DisplayCloseAll(self):
        """Close all open map display windows"""
        for display in self.GetMapDisplay(onlyCurrent=False):
            display.OnCloseWindow(event=None, askIfSaveWorkspace=False)

    def OnRenderAllMapDisplays(self, event=None):
        for display in self.GetAllMapDisplays():
            display.OnRender(None)

    def OnRenameDisplay(self, event):
        """Change Map Display name"""
        name = self.notebookLayers.GetPageText(self.currentPageNum)
        dlg = TextEntryDialog(
            self,
            message=_("Enter new name:"),
            caption=_("Rename Map Display"),
            value=name,
        )
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            self.notebookLayers.SetPageText(page=self.currentPageNum, text=name)
            mapdisplay = self.GetMapDisplay()
            mapdisplay.SetTitle(name)
        dlg.Destroy()

    def OnRasterRules(self, event):
        """Launches dialog for raster color rules"""
        from modules.colorrules import RasterColorTable

        ctable = RasterColorTable(self, layerTree=self.GetLayerTree())
        ctable.Show()
        ctable.CentreOnScreen()

    def OnVectorRules(self, event):
        """Launches dialog for vector color rules"""
        from modules.colorrules import VectorColorTable

        ctable = VectorColorTable(
            self, layerTree=self.GetLayerTree(), attributeType="color"
        )
        ctable.Show()
        ctable.CentreOnScreen()

    def OnEditImageryGroups(self, event, cmd=None):
        """Show dialog for creating and editing groups."""
        dlg = GroupDialog(self)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnInstallExtension(self, event):
        """Install extension from GRASS Addons repository"""
        from modules.extensions import InstallExtensionWindow

        win = InstallExtensionWindow(self, giface=self._giface, size=(650, 550))
        win.CentreOnScreen()
        win.Show()

    def OnManageExtension(self, event):
        """Manage or uninstall extensions"""
        from modules.extensions import ManageExtensionWindow

        win = ManageExtensionWindow(self, size=(650, 300))
        win.CentreOnScreen()
        win.Show()

    def OnPreferences(self, event):
        """General GUI preferences/settings"""
        if not self.dialogs["preferences"]:
            dlg = PreferencesDialog(parent=self, giface=self._giface)
            self.dialogs["preferences"] = dlg
            self.dialogs["preferences"].CenterOnParent()

            dlg.settingsChanged.connect(self.OnSettingsChanged)
            self.Bind(
                wx.EVT_CLOSE, lambda evt: self.dialogs.update(preferences=None), dlg
            )

        self.dialogs["preferences"].Show()

    def OnNvizPreferences(self, event):
        """Show nviz preferences"""
        if not self.dialogs["nvizPreferences"]:
            from nviz.preferences import NvizPreferencesDialog

            dlg = NvizPreferencesDialog(parent=self, giface=self._giface)
            self.dialogs["nvizPreferences"] = dlg
            self.dialogs["nvizPreferences"].CenterOnScreen()
        self.dialogs["nvizPreferences"].Show()

    def OnHelp(self, event):
        """Show help"""
        self._gconsole.RunCmd(["g.manual", "-i"])

    def OnIClass(self, event=None, cmd=None):
        """Start wxIClass tool

        The parameters of all handlers which are associated with module
        and contained in menu/toolboxes must be event and cmd.
        When called from menu event is always None and cmd is the
        associated command (list containing a module name and parameters).

        .. todo::
            This documentation is actually documentation of some
            component related to gui_core/menu.py file.
        """
        from iclass.frame import IClassMapFrame, haveIClass, errMsg

        if not haveIClass:
            GError(
                _('Unable to launch "Supervised Classification Tool".\n\n' "Reason: %s")
                % errMsg
            )
            return

        win = IClassMapFrame(parent=self, giface=self._giface)
        win.CentreOnScreen()

        win.Show()

    def OnAnimationTool(self, event=None, cmd=None):
        """Launch Animation tool. See OnIClass documentation."""
        from animation.frame import AnimationFrame

        frame = AnimationFrame(parent=self, giface=self._giface)
        frame.CentreOnScreen()
        frame.Show()

        tree = self.GetLayerTree()
        if tree:
            rasters = []
            for layer in tree.GetSelectedLayers(checkedOnly=False):
                if tree.GetLayerInfo(layer, key="type") == "raster":
                    rasters.append(tree.GetLayerInfo(layer, key="maplayer").GetName())
            if len(rasters) >= 2:
                from core.layerlist import LayerList
                from animation.data import AnimLayer

                layerList = LayerList()
                layer = AnimLayer()
                layer.mapType = "raster"
                layer.name = ",".join(rasters)
                layer.cmd = ["d.rast", "map="]
                layerList.AddLayer(layer)
                frame.SetAnimations([layerList, None, None, None])

    def OnTimelineTool(self, event=None, cmd=None):
        """Launch Timeline Tool"""
        try:
            from timeline.frame import TimelineFrame
        except ImportError:
            GError(parent=self, message=_("Unable to start Timeline Tool."))
            return
        frame = TimelineFrame(None)
        frame.Show()

    def OnTplotTool(self, event=None, cmd=None):
        """Launch Temporal Plot Tool"""
        try:
            from tplot.frame import TplotFrame
        except ImportError:
            GError(parent=self, message=_("Unable to start Temporal Plot Tool."))
            return
        frame = TplotFrame(parent=self, giface=self._giface)
        frame.Show()

    def OnHistogram(self, event):
        """Init histogram display canvas and tools"""
        from modules.histogram import HistogramFrame

        win = HistogramFrame(self, giface=self._giface)

        win.CentreOnScreen()
        win.Show()
        win.Refresh()
        win.Update()

    def OnMapCalculator(self, event, cmd=""):
        """Init map calculator for interactive creation of mapcalc statements"""
        from modules.mcalc_builder import MapCalcFrame

        if event:
            try:
                cmd = self.GetMenuCmd(event)
            except KeyError:
                cmd = ["r.mapcalc"]
        win = MapCalcFrame(parent=self, giface=self._giface, cmd=cmd[0])
        win.CentreOnScreen()
        win.Show()

    def OnVectorCleaning(self, event, cmd=""):
        """Init interactive vector cleaning"""
        from modules.vclean import VectorCleaningFrame

        win = VectorCleaningFrame(parent=self)
        win.CentreOnScreen()
        win.Show()

    def OnRasterOutputFormat(self, event):
        """Set raster output format handler"""
        self.OnMenuCmd(cmd=["r.external.out"])

    def OnVectorOutputFormat(self, event):
        """Set vector output format handler"""
        from modules.import_export import GdalOutputDialog

        dlg = GdalOutputDialog(parent=self, ogr=True)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnImportDxfFile(self, event, cmd=None):
        """Convert multiple DXF layers to GRASS vector map layers"""
        from modules.import_export import DxfImportDialog

        dlg = DxfImportDialog(parent=self, giface=self._giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnImportGdalLayers(self, event, cmd=None):
        """Convert multiple GDAL layers to GRASS raster map layers"""
        from modules.import_export import GdalImportDialog

        dlg = GdalImportDialog(parent=self, giface=self._giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnLinkGdalLayers(self, event, cmd=None):
        """Link multiple GDAL layers to GRASS raster map layers"""
        from modules.import_export import GdalImportDialog

        dlg = GdalImportDialog(parent=self, giface=self._giface, link=True)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnImportOgrLayers(self, event, cmd=None):
        """Convert multiple OGR layers to GRASS vector map layers"""
        from modules.import_export import OgrImportDialog

        dlg = OgrImportDialog(parent=self, giface=self._giface)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnLinkOgrLayers(self, event, cmd=None):
        """Links multiple OGR layers to GRASS vector map layers"""
        from modules.import_export import OgrImportDialog

        dlg = OgrImportDialog(parent=self, giface=self._giface, link=True)
        dlg.CentreOnScreen()
        dlg.Show()

    def OnAddWS(self, event, cmd=None):
        """Add web services layer"""
        from web_services.dialogs import AddWSDialog

        dlg = AddWSDialog(parent=self, giface=self._giface)
        dlg.CentreOnScreen()
        x, y = dlg.GetPosition()
        dlg.SetPosition((x, y - 200))
        dlg.Show()

    def OnSimpleEditor(self, event):
        # import on demand
        from gui_core.pyedit import PyEditFrame

        # we don't keep track of them and we don't care about open files
        # there when closing the main GUI
        simpleEditor = PyEditFrame(parent=self, giface=self._giface)
        simpleEditor.SetSize(self.GetSize())
        simpleEditor.CenterOnScreen()
        simpleEditor.Show()

    def OnShowAttributeTable(self, event, selection=None):
        """Show attribute table of the given vector map layer"""
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
            maptype = tree.GetLayerInfo(layer, key="maplayer").type
        except Exception:
            maptype = None

        if not maptype or maptype != "vector":
            GMessage(parent=self, message=_("Selected map layer is not vector."))
            return

        if not tree.GetLayerInfo(layer):
            return
        dcmd = tree.GetLayerInfo(layer, key="cmd")
        if not dcmd:
            return

        from dbmgr.manager import AttributeManager

        dbmanager = AttributeManager(
            parent=self,
            id=wx.ID_ANY,
            size=wx.Size(500, 300),
            item=layer,
            log=self._gconsole,
            selection=selection,
        )
        # register ATM dialog
        self.dialogs["atm"].append(dbmanager)
        # show ATM window
        dbmanager.Show()

    def OnNewDisplay(self, event=None):
        """Create new layer tree and map display instance"""
        self.NewDisplay()

    def NewDisplay(self, name=None, show=True):
        """Create new layer tree, which will
        create an associated map display frame

        :param name: name of new map display
        :param show: show map display window if True

        :return: reference to mapdisplay intance
        """
        Debug.msg(1, "GMFrame.NewDisplay(): idx=%d" % self.displayIndex)

        # make a new page in the bookcontrol for the layer tree (on page 0 of
        # the notebook)
        self.pg_panel = wx.Panel(self.notebookLayers, id=wx.ID_ANY, style=wx.EXPAND)
        if name:
            dispName = name
        else:
            dispName = _("Map Display {number}").format(number=self.displayIndex + 1)
        self.notebookLayers.AddPage(page=self.pg_panel, text=dispName, select=True)
        self.currentPage = self.notebookLayers.GetCurrentPage()

        # create layer tree (tree control for managing GIS layers)  and put on
        # new notebook page
        self.currentPage.maptree = LayerTree(
            self.currentPage,
            giface=self._giface,
            id=wx.ID_ANY,
            pos=wx.DefaultPosition,
            size=wx.DefaultSize,
            style=wx.TR_HAS_BUTTONS
            | wx.TR_LINES_AT_ROOT
            | wx.TR_HIDE_ROOT
            | wx.TR_DEFAULT_STYLE
            | wx.NO_BORDER
            | wx.FULL_REPAINT_ON_RESIZE,
            idx=self.displayIndex,
            lmgr=self,
            notebook=self.notebookLayers,
            showMapDisplay=show,
            title=dispName,
        )

        # layout for controls
        cb_boxsizer = wx.BoxSizer(wx.VERTICAL)
        cb_boxsizer.Add(self.GetLayerTree(), proportion=1, flag=wx.EXPAND, border=1)
        self.currentPage.SetSizer(cb_boxsizer)
        cb_boxsizer.Fit(self.GetLayerTree())
        self.currentPage.Layout()
        self.GetLayerTree().Layout()

        mapdisplay = self.currentPage.maptree.mapdisplay
        mapdisplay.Bind(
            wx.EVT_ACTIVATE,
            lambda event, page=self.currentPage: self._onMapDisplayFocus(page),
        )
        mapdisplay.starting3dMode.connect(
            lambda firstTime, mapDisplayPage=self.currentPage: self._onStarting3dMode(
                mapDisplayPage
            )
        )
        mapdisplay.starting3dMode.connect(self.AddNvizTools)
        mapdisplay.ending3dMode.connect(self.RemoveNvizTools)
        mapdisplay.closingDisplay.connect(self._closePageNoEvent)

        # use default window layout
        if UserSettings.Get(group="general", key="defWindowPos", subkey="enabled"):
            dim = UserSettings.Get(group="general", key="defWindowPos", subkey="dim")
            idx = 4 + self.displayIndex * 4
            try:
                x, y = map(int, dim.split(",")[idx : idx + 2])
                w, h = map(int, dim.split(",")[idx + 2 : idx + 4])
                self.GetMapDisplay().SetPosition((x, y))
                self.GetMapDisplay().SetSize((w, h))
            except Exception:
                pass

        # set default properties
        mapdisplay.SetProperties(
            render=UserSettings.Get(
                group="display", key="autoRendering", subkey="enabled"
            ),
            mode=UserSettings.Get(
                group="display", key="statusbarMode", subkey="selection"
            ),
            alignExtent=UserSettings.Get(
                group="display", key="alignExtent", subkey="enabled"
            ),
            constrainRes=UserSettings.Get(
                group="display", key="compResolution", subkey="enabled"
            ),
            showCompExtent=UserSettings.Get(
                group="display", key="showCompExtent", subkey="enabled"
            ),
        )

        self.displayIndex += 1

        return self.GetMapDisplay()

    def _onMapDisplayFocus(self, notebookLayerPage):
        """Changes bookcontrol page to page associated with display."""
        # moved from mapdisp/frame.py
        # TODO: why it is called 3 times when getting focus?
        # and one times when loosing focus?
        if self.workspace_manager.loadingWorkspace:
            return
        pgnum = self.notebookLayers.GetPageIndex(notebookLayerPage)
        if pgnum > -1:
            self.notebookLayers.SetSelection(pgnum)
            self.currentPage = self.notebookLayers.GetCurrentPage()

    def _onStarting3dMode(self, mapDisplayPage):
        """Disables 3D mode for all map displays except for @p mapDisplay"""
        # TODO: it should be disabled also for newly created map windows
        # moreover mapdisp.Disable3dMode() does not work properly
        for page in range(0, self.GetLayerNotebook().GetPageCount()):
            mapdisp = self.GetLayerNotebook().GetPage(page).maptree.GetMapDisplay()
            if self.GetLayerNotebook().GetPage(page) != mapDisplayPage:
                mapdisp.Disable3dMode()

    def OnAddMaps(self, event=None):
        """Add selected map layers into layer tree"""
        dialog = MapLayersDialog(
            parent=self, title=_("Add selected map layers into layer tree")
        )
        dialog.applyAddingMapLayers.connect(self.AddMaps)
        val = dialog.ShowModal()

        if val == wx.ID_OK:
            self.AddMaps(dialog.GetMapLayers(), dialog.GetLayerType(cmd=True))
        dialog.Destroy()

    def AddMaps(self, mapLayers, ltype, check=False):
        """Add map layers to layer tree.

        :param list mapLayers: list of map names
        :param str ltype: layer type ('raster', 'raster_3d', 'vector')
        :param bool check: True if new layers should be checked in
                           layer tree False otherwise
        """
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay()

        maptree = self.GetLayerTree()

        for layerName in mapLayers:
            if ltype == "raster":
                cmd = ["d.rast", "map=%s" % layerName]
            elif ltype == "raster_3d":
                cmd = ["d.rast3d", "map=%s" % layerName]
            elif ltype == "vector":
                cmd = ["d.vect", "map=%s" % layerName] + GetDisplayVectSettings()
            else:
                GError(
                    parent=self, message=_("Unsupported map layer type <%s>.") % ltype
                )
                return

            maptree.AddLayer(
                ltype=ltype,
                lname=layerName,
                lchecked=check,
                lopacity=1.0,
                lcmd=cmd,
                lgroup=None,
            )

    def _updateCurrentMap(self, **kwargs):
        """Updates map of the current map window."""
        if "delay" in kwargs:
            self.GetMapDisplay().GetWindow().UpdateMap(delay=kwargs["delay"])
        else:
            self.GetMapDisplay().GetWindow().UpdateMap()

    def OnMapCreated(self, name, ltype, add=None):
        """Decides whether the map should be added to layer tree."""
        if add is None:
            # add new map into layer if globally enabled
            if UserSettings.Get(group="cmd", key="addNewLayer", subkey="enabled"):
                self.AddOrUpdateMap(name, ltype)
        elif add:
            # add new map into layer tree
            self.AddOrUpdateMap(name, ltype)
        else:
            # update the map
            display = self.GetMapDisplay()
            display.GetWindow().UpdateMap(render=True)

    def AddOrUpdateMap(self, mapName, ltype):
        """Add map layer or update"""
        # start new map display if no display is available
        if ltype not in ["raster", "raster_3d", "vector"]:
            GError(parent=self, message=_("Unsupported map layer type <%s>.") % ltype)
            return

        if not self.currentPage:
            self.AddMaps([mapName], ltype, check=True)
        else:
            display = self.GetMapDisplay()
            mapLayers = map(
                lambda x: x.GetName(), display.GetMap().GetListOfLayers(ltype=ltype)
            )
            if mapName in mapLayers:
                display.GetWindow().UpdateMap(render=True)
            else:
                self.AddMaps([mapName], ltype, check=True)

    def OnAddRaster(self, event):
        """Add raster map layer"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)

        self.GetLayerTree().AddLayer("raster")

    def OnAddRasterMisc(self, event):
        """Create misc raster popup-menu"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)

        self._popupMenu(
            (
                ("layerRaster_3d", self.OnAddRaster3D),
                (None, None),
                ("layerRgb", self.OnAddRasterRGB),
                ("layerHis", self.OnAddRasterHIS),
                (None, None),
                ("layerShaded", self.OnAddRasterShaded),
                (None, None),
                ("layerRastarrow", self.OnAddRasterArrow),
                ("layerRastnum", self.OnAddRasterNum),
            )
        )

        # show map display
        self.GetMapDisplay().Show()

    def OnAddVector(self, event):
        """Add vector map to the current layer tree"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)

        self.GetLayerTree().AddLayer("vector")

    def OnAddVectorMisc(self, event):
        """Create misc vector popup-menu"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)

        self._popupMenu(
            (
                ("layerThememap", self.OnAddVectorTheme),
                ("layerThemechart", self.OnAddVectorChart),
            )
        )

        # show map display
        self.GetMapDisplay().Show()

    def OnAddVectorTheme(self, event):
        """Add thematic vector map to the current layer tree"""
        self.GetLayerTree().AddLayer("thememap")

    def OnAddVectorChart(self, event):
        """Add chart vector map to the current layer tree"""
        self.GetLayerTree().AddLayer("themechart")

    def OnAddOverlay(self, event):
        """Create decoration overlay menu"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)

        self._popupMenu(
            (
                ("layerGrid", self.OnAddGrid),
                ("layerLabels", self.OnAddLabels),
                ("layerGeodesic", self.OnAddGeodesic),
                ("layerRhumb", self.OnAddRhumb),
                (None, None),
                ("layerCmd", self.OnAddCommand),
            )
        )

        # show map display
        self.GetMapDisplay().Show()

    def OnAddRaster3D(self, event):
        """Add 3D raster map to the current layer tree"""
        self.GetLayerTree().AddLayer("raster_3d")

    def OnAddRasterRGB(self, event):
        """Add RGB raster map to the current layer tree"""
        self.GetLayerTree().AddLayer("rgb")

    def OnAddRasterHIS(self, event):
        """Add HIS raster map to the current layer tree"""
        self.GetLayerTree().AddLayer("his")

    def OnAddRasterShaded(self, event):
        """Add shaded relief raster map to the current layer tree"""
        self.GetLayerTree().AddLayer("shaded")

    def OnAddRasterArrow(self, event):
        """Add flow arrows raster map to the current layer tree"""
        # here it seems that it should be retrieved from the mapwindow
        mapdisplay = self.GetMapDisplay()
        resolution = mapdisplay.mapWindowProperties.resolution
        if not resolution:
            dlg = self.MsgDisplayResolution()
            if dlg.ShowModal() == wx.ID_YES:
                mapdisplay.mapWindowProperties.resolution = True
            dlg.Destroy()

        self.GetLayerTree().AddLayer("rastarrow")

    def OnAddRasterNum(self, event):
        """Add cell number raster map to the current layer tree"""
        mapdisplay = self.GetMapDisplay()
        resolution = mapdisplay.mapWindowProperties.resolution
        if not resolution:
            limitText = _(
                "Note that cell values can only be displayed for "
                "regions of less than 10,000 cells."
            )
            dlg = self.MsgDisplayResolution(limitText)
            if dlg.ShowModal() == wx.ID_YES:
                mapdisplay.mapWindowProperties.resolution = True
            dlg.Destroy()

        # region = tree.GetMap().GetCurrentRegion()
        # if region['cells'] > 10000:
        #   GMessage(message = "Cell values can only be displayed
        #   for regions of < 10,000 cells.", parent = self)
        self.GetLayerTree().AddLayer("rastnum")

    def OnAddCommand(self, event):
        """Add command line map layer to the current layer tree"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)

        self.GetLayerTree().AddLayer("command")

        # show map display
        self.GetMapDisplay().Show()

    def OnAddGroup(self, event):
        """Add layer group"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)

        self.GetLayerTree().AddLayer("group")

        # show map display
        self.GetMapDisplay().Show()

    def OnAddGrid(self, event):
        """Add grid map layer to the current layer tree"""
        self.GetLayerTree().AddLayer("grid")

    def OnAddGeodesic(self, event):
        """Add geodesic line map layer to the current layer tree"""
        self.GetLayerTree().AddLayer("geodesic")

    def OnAddRhumb(self, event):
        """Add rhumb map layer to the current layer tree"""
        self.GetLayerTree().AddLayer("rhumb")

    def OnAddLabels(self, event):
        """Add vector labels map layer to the current layer tree"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)

        self.GetLayerTree().AddLayer("labels")

        # show map display
        self.GetMapDisplay().Show()

    def OnShowRegionExtent(self, event):
        """Add vector labels map layer to the current layer tree"""
        # start new map display if no display is available
        if not self.currentPage:
            self.NewDisplay(show=True)
        # get current map display
        mapdisp = self.GetMapDisplay()
        # change the property
        mapdisp.mapWindowProperties.showRegion = True
        # show map display (user said show so make sure it is visible)
        mapdisp.Show()
        # redraw map if auto-rendering is enabled
        # seems little too low level for this place
        # no redraw when Render is unchecked
        if mapdisp.IsAutoRendered():
            mapdisp.GetMapWindow().UpdateMap(render=False)

    def OnDeleteLayer(self, event):
        """Remove selected map layer from the current layer Tree"""
        if not self.currentPage or not self.GetLayerTree().layer_selected:
            self.MsgNoLayerSelected()
            return

        if UserSettings.Get(group="manager", key="askOnRemoveLayer", subkey="enabled"):
            layerName = ""
            for item in self.GetLayerTree().GetSelections():
                name = self.GetLayerTree().GetItemText(item)
                idx = name.find("(" + _("opacity:"))
                if idx > -1:
                    layerName += "<" + name[:idx].strip(" ") + ">,\n"
                else:
                    layerName += "<" + name + ">,\n"
            layerName = layerName.rstrip(",\n")

            if len(layerName) > 2:  # <>
                message = (
                    _("Do you want to remove map layer(s)\n%s\n" "from layer tree?")
                    % layerName
                )
            else:
                message = _(
                    "Do you want to remove selected map layer(s) " "from layer tree?"
                )

            dlg = wx.MessageDialog(
                parent=self,
                message=message,
                caption=_("Remove map layer"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
            )

            if dlg.ShowModal() != wx.ID_YES:
                dlg.Destroy()
                return

            dlg.Destroy()

        for layer in self.GetLayerTree().GetSelections():
            if self.GetLayerTree().GetLayerInfo(layer, key="type") == "group":
                self.GetLayerTree().DeleteChildren(layer)
            # nested children group layer in the parent group layer (both selected)
            try:
                self.GetLayerTree().Delete(layer)
            except ValueError:
                pass

    def OnKeyDown(self, event):
        """Key pressed"""
        kc = event.GetKeyCode()

        try:
            kc = chr(kc)
        except ValueError:
            event.Skip()
            return

        if event.CtrlDown():
            if kc == "R":
                self.OnAddRaster(None)
            elif kc == "V":
                self.OnAddVector(None)

        event.Skip()

    def OnCloseWindow(self, event):
        """Cleanup when wxGUI is quitted"""
        self._closeWindow(event)

    def OnCloseWindowOrExit(self, event):
        """Cleanup when wxGUI is quitted

        Ask user also to quit GRASS including terminal
        """
        dlg = QuitDialog(self)
        ret = dlg.ShowModal()
        dlg.Destroy()
        if ret != wx.ID_CANCEL:
            self._closeWindow(event)
            if ret == wx.ID_YES:
                self._quitGRASS()

    def _closeWindow(self, event):
        """Close wxGUI"""
        # save command protocol if actived
        if self.goutput.btnCmdProtocol.GetValue():
            self.goutput.CmdProtocolSave()

        if not self.currentPage:
            self._auimgr.UnInit()
            self.Destroy()
            return
        if not self.workspace_manager.CanClosePage(caption=_("Quit GRASS GUI")):
            # when called from menu, it gets CommandEvent and not
            # CloseEvent
            if hasattr(event, "Veto"):
                event.Veto()
            return

        self.DisplayCloseAll()

        self._auimgr.UnInit()
        self.Destroy()

    def _quitGRASS(self):
        """Quit GRASS terminal"""
        shellPid = get_shell_pid()
        if shellPid is None:
            return
        Debug.msg(1, "Exiting shell with pid={0}".format(shellPid))
        import signal

        os.kill(shellPid, signal.SIGTERM)

    def MsgNoLayerSelected(self):
        """Show dialog message 'No layer selected'"""
        wx.MessageBox(
            parent=self,
            message=_("No map layer selected. Operation canceled."),
            caption=_("Message"),
            style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE,
        )

    def MsgDisplayResolution(self, limitText=None):
        """Returns dialog for d.rast.num, d.rast.arrow
            when display resolution is not constrained

        :param limitText: adds a note about cell limit
        """
        message = _(
            "Display resolution is currently not constrained to "
            "computational settings. "
            "It's suggested to constrain map to region geometry. "
            "Do you want to constrain "
            "the resolution?"
        )
        if limitText:
            message += "\n\n%s" % _(limitText)
        dlg = wx.MessageDialog(
            parent=self,
            message=message,
            caption=_("Constrain map to region geometry?"),
            style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION | wx.CENTRE,
        )
        return dlg
