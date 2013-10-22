"""!
@package mapdisp.frame

@brief Map display with toolbar for various display management
functions, and additional toolbars (vector digitizer, 3d view).

Can be used either from Layer Manager or as d.mon backend.

Classes:
 - mapdisp::MapFrame

(C) 2006-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (SingleMapFrame, handlers support)
@author Anna Kratochvilova <kratochanna gmail.com> (SingleMapFrame)
@author Stepan Turek <stepan.turek seznam.cz> (handlers support)
"""

import os
import sys
import copy

from core import globalvar
import wx
import wx.aui

if os.path.join(globalvar.ETCWXDIR, "icons") not in sys.path:
    sys.path.append(os.path.join(globalvar.ETCWXDIR, "icons"))
if os.path.join(globalvar.ETCDIR, "python") not in sys.path:
    sys.path.append(os.path.join(globalvar.ETCDIR, "python"))

from core               import globalvar
from core.render        import Map
from vdigit.toolbars    import VDigitToolbar
from mapdisp.toolbars   import MapToolbar, NvizIcons
from mapdisp.gprint     import PrintOptions
from core.gcmd          import GError, GMessage
from dbmgr.dialogs      import DisplayAttributesDialog
from core.utils         import ListOfCatsToRange, GetLayerNameFromCmd, _
from gui_core.dialogs import GetImageHandlers, ImageSizeDialog
from core.debug         import Debug
from core.settings      import UserSettings
from gui_core.mapdisp   import SingleMapFrame
from mapwin.base import MapWindowProperties
from gui_core.query     import QueryDialog, PrepareQueryResults
from mapwin.buffered import BufferedMapWindow
from mapwin.decorations import TextLayerDialog, \
    LegendController, BarscaleController, ArrowController
from modules.histogram  import HistogramFrame
from wxplot.histogram   import HistogramPlotFrame
from wxplot.profile     import ProfileFrame
from wxplot.scatter     import ScatterFrame
from mapwin.analysis import ProfileController, MeasureDistanceController
from gui_core.forms import GUI
from core.giface import Notification

from mapdisp import statusbar as sb

import grass.script as grass

from grass.pydispatch.signal import Signal


class MapFrame(SingleMapFrame):
    """!Main frame for map display window. Drawing takes place in
    child double buffered drawing window.
    """
    def __init__(self, parent, giface, title = _("GRASS GIS - Map display"),
                 toolbars = ["map"], tree = None, notebook = None, lmgr = None,
                 page = None, Map = Map(), auimgr = None, name = 'MapWindow', **kwargs):
        """!Main map display window with toolbars, statusbar and
        2D map window, 3D map window and digitizer.
        
        @param toolbars array of activated toolbars, e.g. ['map', 'digit']
        @param tree reference to layer tree
        @param notebook control book ID in Layer Manager
        @param lmgr Layer Manager
        @param page notebook page with layer tree
        @param Map instance of render.Map
        @param auimgs AUI manager
        @param name frame name
        @param kwargs wx.Frame attributes
        """
        SingleMapFrame.__init__(self, parent = parent, title = title,
                              Map = Map, auimgr = auimgr, name = name, **kwargs)
        
        self._giface = giface
        # Layer Manager object
        # need by GLWindow (a lot), VDigitWindow (a little bit)
        self._layerManager = lmgr
        # Layer Manager layer tree object
        # used for VDigit toolbar and window and GLWindow
        self.tree = tree
        # Notebook page holding the layer tree
        # used only in OnCloseWindow
        self.page = page
        # Layer Manager layer tree notebook
        # used only in OnCloseWindow
        self.layerbook = notebook

        # Emitted when starting (switching to) 3D mode.
        # Parameter firstTime specifies if 3D was already actived.
        self.starting3dMode = Signal("MapFrame.starting3dMode")

        # Emitted when ending (switching from) 3D mode.
        self.ending3dMode = Signal("MapFrame.ending3dMode")

        # properties are shared in other objects, so defining here
        self.mapWindowProperties = MapWindowProperties()
        self.mapWindowProperties.setValuesFromUserSettings()

        #
        # Add toolbars
        #
        for toolb in toolbars:
            self.AddToolbar(toolb)

        #
        # Add statusbar
        #
        
        # items for choice
        self.statusbarItems = [sb.SbCoordinates,
                               sb.SbRegionExtent,
                               sb.SbCompRegionExtent,
                               sb.SbShowRegion,
                               sb.SbAlignExtent,
                               sb.SbResolution,
                               sb.SbDisplayGeometry,
                               sb.SbMapScale,
                               sb.SbGoTo,
                               sb.SbProjection]
                            
        self.statusbarItemsHiddenInNviz = (sb.SbAlignExtent,
                                           sb.SbDisplayGeometry,
                                           sb.SbShowRegion,
                                           sb.SbResolution,
                                           sb.SbMapScale)
        
        # create statusbar and its manager
        statusbar = self.CreateStatusBar(number = 4, style = 0)
        statusbar.SetStatusWidths([-5, -2, -1, -1])
        self.statusbarManager = sb.SbManager(mapframe = self, statusbar = statusbar)
        
        # fill statusbar manager
        self.statusbarManager.AddStatusbarItemsByClass(self.statusbarItems, mapframe = self, statusbar = statusbar)
        self.statusbarManager.AddStatusbarItem(sb.SbMask(self, statusbar = statusbar, position = 2))
        sbRender = sb.SbRender(self, statusbar = statusbar, position = 3)
        self.statusbarManager.AddStatusbarItem(sbRender)
        
        self.statusbarManager.Update()
        
        #
        self.Map.updateProgress.connect(self.statusbarManager.SetProgress)

        # init decoration objects
        self.decorations = {}
        self.legend = LegendController(self.Map, self._giface)
        self.barscale = BarscaleController(self.Map, self._giface)
        self.arrow = ArrowController(self.Map, self._giface)
        self.decorations[self.legend.id] = self.legend
        self.decorations[self.barscale.id] = self.barscale
        self.decorations[self.arrow.id] = self.arrow

        self.mapWindowProperties.autoRenderChanged.connect(
            lambda value:
            self.OnRender(None) if value else None)

        #
        # Init map display (buffered DC & set default cursor)
        #
        self.MapWindow2D = BufferedMapWindow(self, giface = self._giface,
                                             Map=self.Map,
                                             properties=self.mapWindowProperties,
                                             overlays=self.decorations)
        self.MapWindow2D.mapQueried.connect(self.Query)
        self.MapWindow2D.overlayActivated.connect(self._activateOverlay)
        self.MapWindow2D.overlayHidden.connect(self._hideOverlay)
        self.MapWindow2D.overlayHidden.connect(self._hideOverlay)
        for overlay in (self.legend, self.barscale, self.arrow):
            overlay.overlayChanged.connect(lambda: self.MapWindow2D.UpdateMap(render=False, renderVector=False))
        self._setUpMapWindow(self.MapWindow2D)

        self.MapWindow2D.mouseHandlerUnregistered.connect(self.ResetPointer)

        self.MapWindow2D.InitZoomHistory()
        self.MapWindow2D.zoomChanged.connect(self.StatusbarUpdate)

        self._giface.updateMap.connect(self.MapWindow2D.UpdateMap)
        # default is 2D display mode
        self.MapWindow = self.MapWindow2D
        self.MapWindow.SetNamedCursor('default')
        # used by vector digitizer
        self.MapWindowVDigit = None
        # used by Nviz (3D display mode)
        self.MapWindow3D = None 

        #
        # initialize region values
        #
        self._initMap(Map = self.Map) 

        self.toolbars['map'].SelectDefault()
        #
        # Bind various events
        #
        self.Bind(wx.EVT_CLOSE,    self.OnCloseWindow)
        self.Bind(wx.EVT_SIZE,     self.OnSize)
        
        #
        # Update fancy gui style
        #
        self._mgr.AddPane(self.MapWindow, wx.aui.AuiPaneInfo().CentrePane().
                          Dockable(False).BestSize((-1,-1)).Name('2d').
                          CloseButton(False).DestroyOnClose(True).
                          Layer(0))
        self._mgr.Update()

        #
        # Init print module and classes
        #
        self.printopt = PrintOptions(self, self.MapWindow)

        #
        # Re-use dialogs
        #
        self.dialogs = {}
        self.dialogs['attributes'] = None
        self.dialogs['category'] = None
        self.dialogs['vnet'] = None
        self.dialogs['query'] = None

        self.measureDistController = None

    def GetMapWindow(self):
        return self.MapWindow
    
    def _addToolbarVDigit(self):
        """!Add vector digitizer toolbar
        """
        from vdigit.main import haveVDigit, VDigit
        
        if not haveVDigit:
            from vdigit import errorMsg
            
            self.toolbars['map'].combo.SetValue(_("2D view"))
            
            GError(_("Unable to start wxGUI vector digitizer.\n"
                     "Details: %s") % errorMsg, parent = self)
            return

        if not self.MapWindowVDigit:
            from vdigit.mapwindow import VDigitWindow
            self.MapWindowVDigit = VDigitWindow(parent=self, giface=self._giface,
                                                properties=self.mapWindowProperties,
                                                Map=self.Map, tree=self.tree,
                                                lmgr=self._layerManager,
                                                overlays=self.decorations)
            self._setUpMapWindow(self.MapWindowVDigit)
            self.MapWindowVDigit.digitizingInfo.connect(
                lambda text:
                self.statusbarManager.statusbarItems['coordinates'].SetAdditionalInfo(text))
            self.MapWindowVDigit.digitizingInfoUnavailable.connect(
                lambda:
                self.statusbarManager.statusbarItems['coordinates'].SetAdditionalInfo(None))
            self.MapWindowVDigit.Show()
            self._mgr.AddPane(self.MapWindowVDigit, wx.aui.AuiPaneInfo().CentrePane().
                          Dockable(False).BestSize((-1,-1)).Name('vdigit').
                          CloseButton(False).DestroyOnClose(True).
                          Layer(0))
        
        self.MapWindow = self.MapWindowVDigit
        
        if self._mgr.GetPane('2d').IsShown():
            self._mgr.GetPane('2d').Hide()
        elif self._mgr.GetPane('3d').IsShown():
            self._mgr.GetPane('3d').Hide()
        self._mgr.GetPane('vdigit').Show()
        self.toolbars['vdigit'] = VDigitToolbar(parent=self, toolSwitcher=self._toolSwitcher,
                                                MapWindow = self.MapWindow,
                                                digitClass=VDigit, giface=self._giface,
                                                layerTree=self.tree)
        self.MapWindowVDigit.SetToolbar(self.toolbars['vdigit'])
        
        self._mgr.AddPane(self.toolbars['vdigit'],
                          wx.aui.AuiPaneInfo().
                          Name("vdigittoolbar").Caption(_("Vector Digitizer Toolbar")).
                          ToolbarPane().Top().Row(1).
                          LeftDockable(False).RightDockable(False).
                          BottomDockable(False).TopDockable(True).
                          CloseButton(False).Layer(2).
                          BestSize((self.toolbars['vdigit'].GetBestSize())))
        # change mouse to draw digitized line
        self.MapWindow.mouse['box'] = "point"
        self.MapWindow.zoomtype     = 0
        self.MapWindow.pen          = wx.Pen(colour = 'red',   width = 2, style = wx.SOLID)
        self.MapWindow.polypen      = wx.Pen(colour = 'green', width = 2, style = wx.SOLID)

    def AddNviz(self):
        """!Add 3D view mode window
        """
        from nviz.main import haveNviz, GLWindow, errorMsg
        
        # check for GLCanvas and OpenGL
        if not haveNviz:
            self.toolbars['map'].combo.SetValue(_("2D view"))
            GError(parent = self,
                   message = _("Unable to switch to 3D display mode.\nThe Nviz python extension "
                               "was not found or loaded properly.\n"
                               "Switching back to 2D display mode.\n\nDetails: %s" % errorMsg))
            return

        # here was disabling 3D for other displays, now done on starting3dMode

        self.toolbars['map'].Enable2D(False)
        # add rotate tool to map toolbar
        self.toolbars['map'].InsertTool((('rotate', NvizIcons['rotate'],
                                          self.OnRotate, wx.ITEM_CHECK, 7),)) # 7 is position
        self._toolSwitcher.AddToolToGroup(group='mouseUse', toolbar=self.toolbars['map'],
                                          tool=self.toolbars['map'].rotate)
        self.toolbars['map'].InsertTool((('flyThrough', NvizIcons['flyThrough'],
                                          self.OnFlyThrough, wx.ITEM_CHECK, 8),))
        self._toolSwitcher.AddToolToGroup(group='mouseUse', toolbar=self.toolbars['map'],
                                          tool=self.toolbars['map'].flyThrough)
        self.toolbars['map'].ChangeToolsDesc(mode2d = False)
        # update status bar
        
        self.statusbarManager.HideStatusbarChoiceItemsByClass(self.statusbarItemsHiddenInNviz)
        self.statusbarManager.SetMode(0)
        
        # erase map window
        self.MapWindow.EraseMap()
        
        self._giface.WriteCmdLog(_("Starting 3D view mode..."), notification=Notification.HIGHLIGHT)
        self.SetStatusText(_("Please wait, loading data..."), 0)
        
        # create GL window
        if not self.MapWindow3D:
            self.MapWindow3D = GLWindow(self, giface = self._giface, id = wx.ID_ANY, frame = self,
                                        Map = self.Map, tree = self.tree, lmgr = self._layerManager)
            self._setUpMapWindow(self.MapWindow3D)
            self.MapWindow3D.mapQueried.connect(self.Query)
            self.MapWindow = self.MapWindow3D
            self.MapWindow.SetNamedCursor('default')

            # here was AddNvizTools in lmgr
            self.starting3dMode.emit(firstTime=True)

            # switch from MapWindow to MapWindowGL
            self._mgr.GetPane('2d').Hide()
            self._mgr.AddPane(self.MapWindow3D, wx.aui.AuiPaneInfo().CentrePane().
                              Dockable(False).BestSize((-1,-1)).Name('3d').
                              CloseButton(False).DestroyOnClose(True).
                              Layer(0))
            
            self.MapWindow3D.Show()
            self.MapWindow3D.ResetViewHistory()            
            self.MapWindow3D.UpdateView(None)
            self.MapWindow3D.overlayActivated.connect(self._activateOverlay)
            self.MapWindow3D.overlayHidden.connect(self._hideOverlay)
            self.legend.overlayChanged.connect(lambda: self.MapWindow3D.UpdateOverlays())
        else:
            self.MapWindow = self.MapWindow3D
            os.environ['GRASS_REGION'] = self.Map.SetRegion(windres = True, windres3 = True)
            self.MapWindow3D.GetDisplay().Init()
            del os.environ['GRASS_REGION']
            
            # switch from MapWindow to MapWindowGL
            self._mgr.GetPane('2d').Hide()
            self._mgr.GetPane('3d').Show()

            # here was AddNvizTools in lmgr and updating of pages
            self.starting3dMode.emit(firstTime=False)

            self.MapWindow3D.ResetViewHistory()

        self._giface.updateMap.disconnect(self.MapWindow2D.UpdateMap)
        self._giface.updateMap.connect(self.MapWindow3D.UpdateMap)
        self.MapWindow3D.overlays = self.MapWindow2D.overlays
        self.MapWindow3D.textdict = self.MapWindow2D.textdict
        # update overlays needs to be called after because getClientSize
        # is called during update and it must give reasonable values
        wx.CallAfter(self.MapWindow3D.UpdateOverlays)
        
        self.SetStatusText("", 0)
        self._mgr.Update()

    def Disable3dMode(self):
        """Disables 3D mode (NVIZ) in user interface."""
        # TODO: this is broken since item is removed but switch is drived by index
        if '3D' in self.toolbars['map'].combo.GetString(1):
            self.toolbars['map'].combo.Delete(1)

    def RemoveNviz(self):
        """!Restore 2D view"""
        try:
            self.toolbars['map'].RemoveTool(self.toolbars['map'].rotate)
            self.toolbars['map'].RemoveTool(self.toolbars['map'].flyThrough)
        except AttributeError:
            pass
        
        # update status bar
        self.statusbarManager.ShowStatusbarChoiceItemsByClass(self.statusbarItemsHiddenInNviz)
        self.statusbarManager.SetMode(UserSettings.Get(group = 'display',
                                                       key = 'statusbarMode',
                                                       subkey = 'selection'))
        self.SetStatusText(_("Please wait, unloading data..."), 0)
        # unloading messages from library cause highlight anyway
        self._giface.WriteCmdLog(_("Switching back to 2D view mode..."),
                                 notification=Notification.NO_NOTIFICATION)
        if self.MapWindow3D:
            self.MapWindow3D.OnClose(event = None)
        # switch from MapWindowGL to MapWindow
        self._mgr.GetPane('2d').Show()
        self._mgr.GetPane('3d').Hide()

        self.MapWindow = self.MapWindow2D
        # here was RemoveNvizTools form lmgr
        self.ending3dMode.emit()
        try:
            self.MapWindow2D.overlays = self.MapWindow3D.overlays
            self.MapWindow2D.textdict = self.MapWindow3D.textdict
        except AttributeError:
            pass
        # TODO: here we end because self.MapWindow3D is None for a while
        self._giface.updateMap.disconnect(self.MapWindow3D.UpdateMap)
        self._giface.updateMap.connect(self.MapWindow2D.UpdateMap)

        self.MapWindow.UpdateMap()
        self._mgr.Update()
        self.GetMapToolbar().SelectDefault()
        
    def AddToolbar(self, name, fixed = False):
        """!Add defined toolbar to the window

        Currently recognized toolbars are:
         - 'map'     - basic map toolbar
         - 'vdigit'  - vector digitizer

        @param name toolbar to add
        @param fixed fixed toolbar
        """
        # default toolbar
        if name == "map":
            self.toolbars['map'] = MapToolbar(self, toolSwitcher=self._toolSwitcher)
            
            self._mgr.AddPane(self.toolbars['map'],
                              wx.aui.AuiPaneInfo().
                              Name("maptoolbar").Caption(_("Map Toolbar")).
                              ToolbarPane().Top().Name('mapToolbar').
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2).
                              BestSize((self.toolbars['map'].GetBestSize())))
            
        # vector digitizer
        elif name == "vdigit":
            self.toolbars['map'].combo.SetValue(_("Digitize"))
            self._addToolbarVDigit()
        
        if fixed:
            self.toolbars['map'].combo.Disable()
         
        self._mgr.Update()
        
    def RemoveToolbar (self, name):
        """!Removes defined toolbar from the window

        @todo Only hide, activate by calling AddToolbar()
        """
        # cannot hide main toolbar
        if name == "map":
            return
        
        self._mgr.DetachPane(self.toolbars[name])
        self._toolSwitcher.RemoveToolbarFromGroup('mouseUse', self.toolbars[name])
        self.toolbars[name].Destroy()
        self.toolbars.pop(name)
        
        if name == 'vdigit':
            self._mgr.GetPane('vdigit').Hide()
            self._mgr.GetPane('2d').Show()
            self.MapWindow = self.MapWindow2D
            
        self.toolbars['map'].combo.SetValue(_("2D view"))
        self.toolbars['map'].Enable2D(True)
        
        self._mgr.Update()
    
    def IsPaneShown(self, name):
        """!Check if pane (toolbar, mapWindow ...) of given name is currently shown"""
        if self._mgr.GetPane(name).IsOk():
            return self._mgr.GetPane(name).IsShown()
        return False

    def RemoveQueryLayer(self):
        """!Removes temporary map layers (queries)"""
        qlayer = self.GetMap().GetListOfLayers(name = globalvar.QUERYLAYER)
        for layer in qlayer:
            self.GetMap().DeleteLayer(layer)

    def OnRender(self, event):
        """!Re-render map composition (each map layer)
        """
        self.RemoveQueryLayer()
        
        # deselect features in vdigit
        if self.GetToolbar('vdigit'):
            if self.MapWindow.digit:
                self.MapWindow.digit.GetDisplay().SetSelected([])
            self.MapWindow.UpdateMap(render = True, renderVector = True)
        else:
            self.MapWindow.UpdateMap(render = True)
        
        # update statusbar
        self.StatusbarUpdate()

    def OnPointer(self, event):
        """!Pointer button clicked
        """        
        self.MapWindow.SetModePointer()

        if self.GetToolbar('vdigit'):
            self.toolbars['vdigit'].action['id'] = -1
            self.toolbars['vdigit'].action['desc']=''

    def OnRotate(self, event):
        """!Rotate 3D view
        """
        self.MapWindow.mouse['use'] = "rotate"
        
        # change the cursor
        self.MapWindow.SetNamedCursor('hand')

    def OnFlyThrough(self, event):
        """!Fly-through mode
        """
        self.MapWindow.mouse['use'] = "fly"
        
        # change the cursor
        self.MapWindow.SetNamedCursor('hand')
        self.MapWindow.SetFocus()

    def SaveToFile(self, event):
        """!Save map to image
        """
        if self.IsPaneShown('3d'):
            filetype = "TIF file (*.tif)|*.tif|PPM file (*.ppm)|*.ppm"
            ltype = [{ 'ext' : 'tif', 'type' : 'tif' },
                     { 'ext' : 'ppm', 'type' : 'ppm' }]
        else:
            img = self.MapWindow.img
            if not img:
                GMessage(parent = self,
                         message = _("Nothing to render (empty map). Operation canceled."))
                return
            filetype, ltype = GetImageHandlers(img)
        
        # get size
        dlg = ImageSizeDialog(self)
        dlg.CentreOnParent()
        if dlg.ShowModal() != wx.ID_OK:
            dlg.Destroy()
            return
        width, height = dlg.GetValues()
        dlg.Destroy()
        
        # get filename
        dlg = wx.FileDialog(parent = self,
                            message = _("Choose a file name to save the image "
                                        "(no need to add extension)"),
                            wildcard = filetype,
                            style = wx.SAVE | wx.FD_OVERWRITE_PROMPT)
        
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return
            
            base, ext = os.path.splitext(path)
            fileType = ltype[dlg.GetFilterIndex()]['type']
            extType  = ltype[dlg.GetFilterIndex()]['ext']
            if ext != extType:
                path = base + '.' + extType
            
            self.MapWindow.SaveToFile(path, fileType,
                                      width, height)
            
        dlg.Destroy()

    def PrintMenu(self, event):
        """
        Print options and output menu for map display
        """
        printmenu = wx.Menu()
        # Add items to the menu
        setup = wx.MenuItem(printmenu, wx.ID_ANY, _('Page setup'))
        printmenu.AppendItem(setup)
        self.Bind(wx.EVT_MENU, self.printopt.OnPageSetup, setup)

        preview = wx.MenuItem(printmenu, wx.ID_ANY, _('Print preview'))
        printmenu.AppendItem(preview)
        self.Bind(wx.EVT_MENU, self.printopt.OnPrintPreview, preview)

        doprint = wx.MenuItem(printmenu, wx.ID_ANY, _('Print display'))
        printmenu.AppendItem(doprint)
        self.Bind(wx.EVT_MENU, self.printopt.OnDoPrint, doprint)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(printmenu)
        printmenu.Destroy()

    def OnCloseWindow(self, event):
        """!Window closed.
        Also close associated layer tree page
        """
        Debug.msg(2, "MapFrame.OnCloseWindow(): function starts")
        pgnum = None
        self.Map.Clean()
        
        # close edited map and 3D tools properly
        if self.GetToolbar('vdigit'):
            maplayer = self.toolbars['vdigit'].GetLayer()
            if maplayer:
                self.toolbars['vdigit'].OnExit()
        if self.IsPaneShown('3d'):
            self.RemoveNviz()
        
        if not self._layerManager:
            self.Destroy()
        elif self.page:
            pgnum = self.layerbook.GetPageIndex(self.page)
            if pgnum > -1:
                self.layerbook.DeletePage(pgnum)
        Debug.msg(2, "MapFrame.OnCloseWindow(): function ends")

    def Query(self, x, y):
        """!Query selected layers. 

        @param x,y coordinates
        @param layers selected tree item layers
        """
        layers = self._giface.GetLayerList().GetSelectedLayers(checkedOnly=False)
        rast = []
        vect = []
        for layer in layers:
            name, found = GetLayerNameFromCmd(layer.cmd)
            if not found:
                continue
            ltype = layer.maplayer.GetType()
            if ltype == 'raster':
                rast.append(name)
            elif ltype in ('rgb', 'his'):
                for iname in name.split('\n'):
                    rast.append(iname)
            elif ltype in ('vector', 'thememap', 'themechart'):
                vect.append(name)
        if vect:
            # check for vector maps open to be edited
            digitToolbar = self.GetToolbar('vdigit')
            if digitToolbar:
                lmap = digitToolbar.GetLayer().GetName()
                for name in vect:
                    if lmap == name:
                        self._giface.WriteWarning(_("Vector map <%s> "
                                                                  "opened for editing - skipped.") % lmap)
                        vect.remove(name)

        if not (rast + vect):
            GMessage(parent = self,
                     message = _('No raster or vector map layer selected for querying.'))
            return

        # set query snap distance for v.what at map unit equivalent of 10 pixels
        qdist = 10.0 * ((self.Map.region['e'] - self.Map.region['w']) / self.Map.width)

        # TODO: replace returning None by exception or so
        try:
            east, north = self.MapWindow.Pixel2Cell((x, y))
        except TypeError:
            return

        if not self.IsPaneShown('3d'):
            self.QueryMap(east, north, qdist, rast, vect)
        else:
            if rast:
                self.MapWindow.QuerySurface(x, y)
            if vect:
                self.QueryMap(east, north, qdist, rast = [], vect = vect)

    def QueryMap(self, east, north, qdist, rast, vect):
        """!Query raster or vector map layers by r/v.what
        
        @param east,north coordinates
        @param qdist query distance
        @param rast raster map names
        @param vect vector map names
        """
        Debug.msg(1, "QueryMap(): raster=%s vector=%s" % (','.join(rast),
                                                          ','.join(vect)))

        # use display region settings instead of computation region settings
        self.tmpreg = os.getenv("GRASS_REGION")
        os.environ["GRASS_REGION"] = self.Map.SetRegion(windres = False)

        rastQuery = []
        vectQuery = []
        if rast:
            rastQuery = grass.raster_what(map=rast, coord=(east, north))
        if vect:
            vectQuery = grass.vector_what(map=vect, coord=(east, north), distance=qdist)
        self._QueryMapDone()
        if 'Id' in vectQuery:
            self._queryHighlight(vectQuery)

        result = rastQuery + vectQuery
        result = PrepareQueryResults(coordinates = (east, north), result = result)
        if self.dialogs['query']:
            self.dialogs['query'].Raise()
            self.dialogs['query'].SetData(result)
        else:
            self.dialogs['query'] = QueryDialog(parent = self, data = result)
            self.dialogs['query'].Bind(wx.EVT_CLOSE, self._oncloseQueryDialog)
            self.dialogs['query'].redirectOutput.connect(self._onRedirectQueryOutput)
            self.dialogs['query'].Show()

    def _oncloseQueryDialog(self, event):
        self.dialogs['query'] = None
        event.Skip()

    def _onRedirectQueryOutput(self, output, style='log'):
        """!Writes query output into console"""
        if style == 'log':
            self._giface.WriteLog(output, notification=Notification.MAKE_VISIBLE)
        elif style == 'cmd':
            self._giface.WriteCmdLog(output)

    def _queryHighlight(self, vectQuery):
        """!Highlight category from query."""
        cats = name = None
        for res in vectQuery:
            cats = {res['Layer']: [res['Category']]}
            name = res['Map']
        try:
            qlayer = self.Map.GetListOfLayers(name = globalvar.QUERYLAYER)[0]
        except IndexError:
            qlayer = None

        if not (cats and name):
            if qlayer:
                self.Map.DeleteLayer(qlayer)
                self.MapWindow.UpdateMap(render = False, renderVector = False)
            return

        if not self.IsPaneShown('3d') and self.IsAutoRendered():
            # highlight feature & re-draw map
            if qlayer:
                qlayer.SetCmd(self.AddTmpVectorMapLayer(name, cats,
                                                        useId = False,
                                                        addLayer = False))
            else:
                qlayer = self.AddTmpVectorMapLayer(name, cats, useId = False)
            
            # set opacity based on queried layer
            # TODO fix
            # opacity = layer.maplayer.GetOpacity(float = True)
            # qlayer.SetOpacity(opacity)
            
            self.MapWindow.UpdateMap(render = False, renderVector = False)

    def _QueryMapDone(self):
        """!Restore settings after querying (restore GRASS_REGION)
        """
        if hasattr(self, "tmpreg"):
            if self.tmpreg:
                os.environ["GRASS_REGION"] = self.tmpreg
            elif 'GRASS_REGION' in os.environ:
                del os.environ["GRASS_REGION"]
        elif 'GRASS_REGION' in os.environ:
            del os.environ["GRASS_REGION"]
        
        if hasattr(self, "tmpreg"):
            del self.tmpreg
        
    def OnQuery(self, event):
        """!Query tools menu"""
        self.MapWindow.mouse['use'] = "query"
        self.MapWindow.mouse['box'] = "point"
        self.MapWindow.zoomtype = 0
        
        # change the cursor
        self.MapWindow.SetNamedCursor('cross')
        
    def AddTmpVectorMapLayer(self, name, cats, useId = False, addLayer = True):
        """!Add temporal vector map layer to map composition

        @param name name of map layer
        @param useId use feature id instead of category 
        """
        # color settings from ATM
        color = UserSettings.Get(group = 'atm', key = 'highlight', subkey = 'color')
        colorStr = str(color[0]) + ":" + \
            str(color[1]) + ":" + \
            str(color[2])

        # icon used in vector display and its size
        icon = ''
        size = 0
        # here we know that there is one selected layer and it is vector
        vparam = self._giface.GetLayerList().GetSelectedLayers()[0].cmd
        for p in vparam:
            if '=' in p:
                parg,pval = p.split('=', 1)
                if parg == 'icon': icon = pval
                elif parg == 'size': size = float(pval)

        pattern = ["d.vect",
                   "map=%s" % name,
                   "color=%s" % colorStr,
                   "fcolor=%s" % colorStr,
                   "width=%d"  % UserSettings.Get(group = 'atm', key = 'highlight', subkey = 'width')]
        if icon != '':
            pattern.append('icon=%s' % icon)
        if size > 0:
            pattern.append('size=%i' % size)
        
        if useId:
            cmd = pattern
            cmd.append('-i')
            cmd.append('cats=%s' % str(cats))
        else:
            cmd = []
            for layer in cats.keys():
                cmd.append(copy.copy(pattern))
                lcats = cats[layer]
                cmd[-1].append("layer=%d" % layer)
                cmd[-1].append("cats=%s" % ListOfCatsToRange(lcats))

        if addLayer:
            if useId:
                return self.Map.AddLayer(ltype = 'vector', name = globalvar.QUERYLAYER, command = cmd,
                                         active = True, hidden = True, opacity = 1.0)
            else:
                return self.Map.AddLayer(ltype = 'command', name = globalvar.QUERYLAYER, command = cmd,
                                         active = True, hidden = True, opacity = 1.0)
        else:
            return cmd

    def OnMeasure(self, event):
        if not self.measureDistController:
            self.measureDistController = MeasureDistanceController(self._giface,
                                                                   mapWindow=self.GetMapWindow())
        self.measureDistController.Start()

    def OnProfile(self, event):
        """!Launch profile tool
        """
        rasters = []
        layers = self._giface.GetLayerList().GetSelectedLayers()
        for layer in layers:
            if layer.type == 'raster':
                rasters.append(layer.maplayer.name)
        self.Profile(rasters=rasters)

    def Profile(self, rasters=None):
        """!Launch profile tool"""
        self.profileController = ProfileController(self._giface,
                                                   mapWindow=self.GetMapWindow())
        win = ProfileFrame(parent=self, rasterList=rasters,
                           units=self.Map.projinfo['units'],
                           controller=self.profileController)
        win.Show()
        # Open raster select dialog to make sure that a raster (and
        # the desired raster) is selected to be profiled
        win.OnSelectRaster(None)

    def OnHistogramPyPlot(self, event):
        """!Init PyPlot histogram display canvas and tools
        """
        raster = []

        for layer in self._giface.GetLayerList().GetSelectedLayers():
            if layer.maplayer.GetType() == 'raster':
                raster.append(layer.maplayer.GetName())

        win = HistogramPlotFrame(parent = self, rasterList = raster)
        win.CentreOnParent()
        win.Show()
        
    def OnScatterplot(self, event):
        """!Init PyPlot scatterplot display canvas and tools
        """
        raster = []

        for layer in self._giface.GetLayerList().GetSelectedLayers():
            if layer.maplayer.GetType() == 'raster':
                raster.append(layer.maplayer.GetName())

        win = ScatterFrame(parent = self, rasterList = raster)
        
        win.CentreOnParent()
        win.Show()
        # Open raster select dialog to make sure that at least 2 rasters (and the desired rasters)
        # are selected to be plotted
        win.OnSelectRaster(None)

    def OnHistogram(self, event):
        """!Init histogram display canvas and tools
        """
        win = HistogramFrame(self, giface=self._giface)
        
        win.CentreOnParent()
        win.Show()
        win.Refresh()
        win.Update()

    def _activateOverlay(self, overlayId):
        """!Launch decoration dialog according to overlay id.

        @param overlayId id of overlay        
        """
        if overlayId > 100:
            self.OnAddText(None)
        elif overlayId == 0:
            self.AddLegend(showDialog=True)
        elif overlayId == 1:
            self.AddBarscale(showDialog=True)
        elif overlayId == 2:
            self.AddArrow(showDialog=True)

    def _hideOverlay(self, overlayId):
        """!Hide overlay.

        @param overlayId id of overlay        
        """
        self.decorations[overlayId].Hide()

    def AddBarscale(self, cmd=None, showDialog=None):
        """!Handler for scale bar map decoration menu selection."""
        if self.IsPaneShown('3d'):
            self.MapWindow3D.SetDrawScalebar((70, 70))
            return

        if self.barscale.IsShown() and showDialog is None:
            self.barscale.Hide()
            return

        if cmd:
            self.barscale.cmd = cmd

        if not showDialog:
            self.barscale.Show()
            return

        # Decoration overlay control dialog
        if self.barscale.dialog:
            if self.barscale.dialog.IsShown():
                self.barscale.dialog.SetFocus()
                self.barscale.dialog.Raise()
            else:
                self.barscale.dialog.Show()
        else:
            # If location is latlon, only display north arrow (scale won't work)
            #        proj = self.Map.projinfo['proj']
            #        if proj == 'll':
            #            barcmd = 'd.barscale -n'
            #        else:
            #            barcmd = 'd.barscale'

            # decoration overlay control dialog
            GUI(parent=self, giface=self._giface, show=True,
                modal=False).ParseCommand(self.barscale.cmd,
                                          completed=(self.barscale.GetOptData, None, None))

        self.MapWindow.mouse['use'] = 'pointer'

    def AddLegend(self, cmd=None, showDialog=None):
        """!Handler for legend map decoration menu selection."""
        if self.legend.IsShown() and showDialog is None:
            self.legend.Hide()
            return
        if cmd:
            self.legend.cmd = cmd
        else:
            layers = self._giface.GetLayerList().GetSelectedLayers()
            for layer in layers:
                if layer.type == 'raster':
                    self.legend.cmd.append('map=%s' % layer.maplayer.name)
                    break

        if not showDialog and self.legend.CmdIsValid():
            self.legend.Show()
            return

        # Decoration overlay control dialog
        if self.legend.dialog:
            if self.legend.dialog.IsShown():
                self.legend.dialog.SetFocus()
                self.legend.dialog.Raise()
            else:
                self.legend.dialog.Show()
        else:
            GUI(parent=self, giface=self._giface, show=True,
                modal=False).ParseCommand(self.legend.cmd,
                                          completed=(self.legend.GetOptData, None, None))

        self.MapWindow.mouse['use'] = 'pointer'

    def AddArrow(self, cmd=None, showDialog=None):
        """!Handler for north arrow menu selection."""
        if self.IsPaneShown('3d'):
            # here was opening of appearance page of nviz notebook
            # but now moved to MapWindow3D where are other problematic nviz calls
            self.MapWindow3D.SetDrawArrow((70, 70))
            return

        if self.arrow.IsShown() and showDialog is None:
            self.arrow.Hide()
            return
        if cmd:
            self.arrow.cmd = cmd

        if not showDialog:
            self.arrow.Show()
            return

        # Decoration overlay control dialog
        if self.arrow.dialog:
            if self.arrow.dialog.IsShown():
                self.arrow.dialog.SetFocus()
                self.arrow.dialog.Raise()
            else:
                self.arrow.dialog.Show()
        else:
            GUI(parent=self, giface=self._giface, show=True,
                modal=False).ParseCommand(self.arrow.cmd,
                                          completed=(self.arrow.GetOptData, None, None))

        self.MapWindow.mouse['use'] = 'pointer'

    def OnAddText(self, event):
        """!Handler for text decoration menu selection.
        """
        if self.MapWindow.dragid > -1:
            id = self.MapWindow.dragid
            self.MapWindow.dragid = -1
        else:
            # index for overlay layer in render
            if len(self.MapWindow.textdict.keys()) > 0:
                id = max(self.MapWindow.textdict.keys()) + 1
            else:
                id = 101
        
        self.dialogs['text'] = TextLayerDialog(parent = self, ovlId = id, 
                                               title = _('Add text layer'),
                                               size = (400, 200))
        self.dialogs['text'].CenterOnParent()

        # If OK button pressed in decoration control dialog
        if self.dialogs['text'].ShowModal() == wx.ID_OK:
            text = self.dialogs['text'].GetValues()['text']
            active = self.dialogs['text'].GetValues()['active']
        
            # delete object if it has no text or is not active
            if text == '' or active == False:
                try:
                    self.MapWindow2D.pdc.ClearId(id)
                    self.MapWindow2D.pdc.RemoveId(id)
                    del self.MapWindow.textdict[id]
                    if self.IsPaneShown('3d'):
                        self.MapWindow3D.UpdateOverlays()
                        self.MapWindow.UpdateMap()
                    else:
                        self.MapWindow2D.UpdateMap(render = False, renderVector = False)
                except:
                    pass
                return

            
            self.MapWindow.textdict[id] = self.dialogs['text'].GetValues()
            
            if self.IsPaneShown('3d'):
                self.MapWindow3D.UpdateOverlays()
                self.MapWindow3D.UpdateMap()
            else:
                self.MapWindow2D.pdc.ClearId(id)
                self.MapWindow2D.pdc.SetId(id)
                self.MapWindow2D.UpdateMap(render = False, renderVector = False)
            
        self.MapWindow.mouse['use'] = 'pointer'
        
    def GetOptData(self, dcmd, type, params, propwin):
        """!Callback method for decoration overlay command generated by
        dialog created in menuform.py
        """
        # Reset comand and rendering options in render.Map. Always render decoration.
        # Showing/hiding handled by PseudoDC
        self.Map.ChangeOverlay(ovltype = type, type = 'overlay', name = '', command = dcmd,
                               active = True, render = False)
        self.params[type] = params
        self.propwin[type] = propwin

    def OnZoomToMap(self, event):
        """!Set display extents to match selected raster (including
        NULLs) or vector map.
        """
        Debug.msg(3, "MapFrame.OnZoomToMap()")
        layers = None
        if self.IsStandalone():
            layers = self.MapWindow.GetMap().GetListOfLayers(active = False)
        
        self.MapWindow.ZoomToMap(layers = layers)

    def OnZoomToRaster(self, event):
        """!Set display extents to match selected raster map (ignore NULLs)
        """
        self.MapWindow.ZoomToMap(ignoreNulls = True)
        
    def OnZoomToSaved(self, event):
        """!Set display geometry to match extents in
        saved region file
        """
        self.MapWindow.SetRegion(zoomOnly=True)
        
    def OnSetDisplayToWind(self, event):
        """!Set computational region (WIND file) to match display
        extents
        """
        self.MapWindow.DisplayToWind()

    def OnSetWindToRegion(self, event):
        """!Set computational region (WIND file) from named region
        file
        """
        self.MapWindow.SetRegion(zoomOnly=False)
 
    def OnSaveDisplayRegion(self, event):
        """!Save display extents to named region file.
        """
        self.MapWindow.SaveRegion(display = True)

    def OnSaveWindRegion(self, event):
        """!Save computational region to named region file.
        """
        self.MapWindow.SaveRegion(display = False)
        
    def OnZoomMenu(self, event):
        """!Popup Zoom menu
        """
        zoommenu = wx.Menu()
        
        for label, handler in ((_('Zoom to computational region'), self.OnZoomToWind),
                               (_('Zoom to default region'), self.OnZoomToDefault),
                               (_('Zoom to saved region'), self.OnZoomToSaved),
                               (None, None),
                               (_('Set computational region from display extent'), self.OnSetDisplayToWind),
                               (_('Set computational region from named region'), self.OnSetWindToRegion),
                               (None, None),
                               (_('Save display geometry to named region'), self.OnSaveDisplayRegion),
                               (_('Save computational region to named region'), self.OnSaveWindRegion)):
            if label:
                mid = wx.MenuItem(zoommenu, wx.ID_ANY, label)
                zoommenu.AppendItem(mid)
                self.Bind(wx.EVT_MENU, handler, mid)
            else:
                zoommenu.AppendSeparator()
        
        # Popup the menu. If an item is selected then its handler will
        # be called before PopupMenu returns.
        self.PopupMenu(zoommenu)
        zoommenu.Destroy()

    def SetProperties(self, render = False, mode = 0, showCompExtent = False,
                      constrainRes = False, projection = False, alignExtent = True):
        """!Set properies of map display window"""
        self.mapWindowProperties.autoRender = render
        self.statusbarManager.SetMode(mode)
        self.StatusbarUpdate()
        self.mapWindowProperties.showRegion = showCompExtent
        self.mapWindowProperties.alignExtent = alignExtent
        self.mapWindowProperties.resolution = constrainRes
        self.SetProperty('projection', projection)
        
    def IsStandalone(self):
        """!Check if Map display is standalone

        @depreciated
        """
        # TODO: once it is removed from 2 places in vdigit it can be deleted
        # here and also in base class and other classes in the tree (hopefully)
        # and one place here still uses IsStandalone
        Debug.msg(1, "MapFrame.IsStandalone(): Method IsStandalone is"
                  "depreciated, use some general approach instead such as"
                  " Signals or giface")
        if self._layerManager:
            return False
        
        return True
    
    def GetLayerManager(self):
        """!Get reference to Layer Manager

        @return window reference
        @return None (if standalone)

        @depreciated
        """
        Debug.msg(1, "MapFrame.GetLayerManager(): Method GetLayerManager is"
                  "depreciated, use some general approach instead such as"
                  " Signals or giface")
        return self._layerManager
    
    def GetMapToolbar(self):
        """!Returns toolbar with zooming tools"""
        return self.toolbars['map']

    def OnVNet(self, event):
        """!Dialog for v.net* modules 
        """
        if self.dialogs['vnet']:
            self.dialogs['vnet'].Raise()
            return
        
        from vnet.dialogs import VNETDialog
        self.dialogs['vnet'] = VNETDialog(parent=self, giface=self._giface)
        self.dialogs['vnet'].CenterOnScreen()
        self.dialogs['vnet'].Show()

    def ResetPointer(self):
        """Sets pointer mode.

        Sets pointer and toggles it (e.g. after unregistration of mouse
        handler).
        """
        self.GetMapToolbar().SelectDefault()
