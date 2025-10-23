"""
@package gui_core.mapdisp

@brief Base classes for Map display window

Classes:
 - mapdisp::MapPanelBase
 - mapdisp::SingleMapPanel
 - mapdisp::DoubleMapPanel
 - mapdisp::FrameMixin

(C) 2009-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton <michael.barton@asu.edu>
@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import sys

import wx

from core.debug import Debug
from gui_core.toolbars import ToolSwitcher
from gui_core.wrap import NewId
from mapdisp import statusbar as sb
from mapwin.base import MapWindowProperties

from grass.script import core as grass


class MapPanelBase(wx.Panel):
    r"""Base class for map display window

    Derived class must use (create and initialize) \c statusbarManager
    or override
    GetProperty(), SetProperty() and HasProperty() methods.

    Several methods has to be overridden or
    \c NotImplementedError("MethodName") will be raised.

    It is expected that derived class will call _setUpMapWindow().

    Derived class can has one or more map windows (and map renders)
    but implementation of MapPanelBase expects that one window and
    one map will be current.
    Current instances of map window and map renderer should be returned
    by methods GetWindow() and GetMap() respectively.

    AUI manager is stored in \c self._mgr.
    """

    def __init__(
        self,
        parent=None,
        id=wx.ID_ANY,
        title="",
        auimgr=None,
        name="",
        **kwargs,
    ):
        r"""

        .. warning::
            Use \a auimgr parameter only if you know what you are doing.

        :param parent: gui parent
        :param id: wx id
        :param title: window title
        :param toolbars: array of activated toolbars, e.g. ['map', 'digit']
        :param auimgr: AUI manager (if \c None, wx.aui.AuiManager is used)
        :param name: panel name
        :param kwargs: arguments passed to \c wx.Panel
        """

        self.parent = parent

        wx.Panel.__init__(self, parent, id, name=name, **kwargs)

        # toolbars
        self.toolbars = {}
        self.iconsize = (16, 16)

        # properties are shared in other objects, so defining here
        self.mapWindowProperties = MapWindowProperties()
        self.mapWindowProperties.setValuesFromUserSettings()
        # update statusbar when user-defined projection changed
        self.mapWindowProperties.useDefinedProjectionChanged.connect(
            self.StatusbarUpdate
        )

        #
        # Fancy gui
        #
        if auimgr is None:
            from wx.aui import AuiManager

            self._mgr = AuiManager(self)
        else:
            self._mgr = auimgr

        # handles switching between tools in different toolbars
        self._toolSwitcher = ToolSwitcher()
        self._toolSwitcher.toggleToolChanged.connect(self._onToggleTool)

        # set accelerator table
        self.shortcuts_table = [
            (self.OnCloseWindow, wx.ACCEL_CTRL, ord("W")),
            (self.OnRender, wx.ACCEL_CTRL, ord("R")),
            (self.OnRender, wx.ACCEL_NORMAL, wx.WXK_F5),
            (self.OnEnableDisableRender, wx.ACCEL_NORMAL, wx.WXK_F6),
        ]

        self._initShortcuts()

    def _initShortcuts(self):
        """init shortcuts to acceleration table"""
        accelTable = []
        for handler, entry, kdb in self.shortcuts_table:
            wxId = NewId()
            self.Bind(wx.EVT_MENU, handler, id=wxId)
            accelTable.append((entry, kdb, wxId))
        self.SetAcceleratorTable(wx.AcceleratorTable(accelTable))

    def _initMap(self, Map):
        """Initialize map display, set dimensions and map region"""
        if not grass.find_program("g.region", "--help"):
            sys.exit(
                _("GRASS module '%s' not found. Unable to start map display window.")
                % "g.region"
            )

        Debug.msg(2, "MapPanel._initMap():")
        Map.ChangeMapSize(self.GetClientSize())
        Map.region = Map.GetRegion()  # g.region -upgc
        # self.Map.SetRegion() # adjust region to match display window

    def _resize(self):
        Debug.msg(1, "MapPanel_resize():")
        wm, hw = self.MapWindow.GetClientSize()
        wf, hf = self.GetSize()
        dw = wf - wm
        dh = hf - hw
        self.SetSize((wf + dw, hf + dh))

    def _onToggleTool(self, id):
        if self._toolSwitcher.IsToolInGroup(id, "mouseUse"):
            self.GetWindow().UnregisterAllHandlers()

    def OnSize(self, event):
        """Adjust statusbar on changing size"""
        # reposition checkbox in statusbar
        self.StatusbarReposition()

        # update statusbar
        self.StatusbarUpdate()

    def OnCloseWindow(self, event):
        self.Destroy()

    def GetToolSwitcher(self):
        return self._toolSwitcher

    def SetProperty(self, name, value):
        """Sets property"""
        if hasattr(self.mapWindowProperties, name):
            setattr(self.mapWindowProperties, name, value)
        else:
            self.statusbarManager.SetProperty(name, value)

    def GetProperty(self, name):
        """Returns property"""
        if hasattr(self.mapWindowProperties, name):
            return getattr(self.mapWindowProperties, name)
        return self.statusbarManager.GetProperty(name)

    def HasProperty(self, name):
        """Checks whether object has property"""
        return self.statusbarManager.HasProperty(name)

    def GetPPM(self):
        """Get pixel per meter

        .. todo::
            now computed every time, is it necessary?

        .. todo::
            enable user to specify ppm (and store it in UserSettings)
        """
        # TODO: need to be fixed...
        # screen X region problem
        # user should specify ppm
        dc = wx.ScreenDC()
        dpSizePx = wx.DisplaySize()  # display size in pixels
        dpSizeMM = wx.DisplaySizeMM()  # display size in mm (system)
        dpSizeIn = (dpSizeMM[0] / 25.4, dpSizeMM[1] / 25.4)  # inches
        sysPpi = dc.GetPPI()
        comPpi = (dpSizePx[0] / dpSizeIn[0], dpSizePx[1] / dpSizeIn[1])

        ppi = comPpi  # pixel per inch
        ppm = ((ppi[0] / 2.54) * 100, (ppi[1] / 2.54) * 100)  # pixel per meter

        Debug.msg(
            4,
            "MapPanelBase.GetPPM(): size: px=%d,%d mm=%f,%f "
            "in=%f,%f ppi: sys=%d,%d com=%d,%d; ppm=%f,%f"
            % (
                dpSizePx[0],
                dpSizePx[1],
                dpSizeMM[0],
                dpSizeMM[1],
                dpSizeIn[0],
                dpSizeIn[1],
                sysPpi[0],
                sysPpi[1],
                comPpi[0],
                comPpi[1],
                ppm[0],
                ppm[1],
            ),
        )

        return ppm

    def SetMapScale(self, value, map=None):
        """Set current map scale

        :param value: scale value (n if scale is 1:n)
        :param map: Map instance (if none self.Map is used)
        """
        if not map:
            map = self.Map

        region = self.Map.region
        dEW = value * (region["cols"] / self.GetPPM()[0])
        dNS = value * (region["rows"] / self.GetPPM()[1])
        region["n"] = region["center_northing"] + dNS / 2.0
        region["s"] = region["center_northing"] - dNS / 2.0
        region["w"] = region["center_easting"] - dEW / 2.0
        region["e"] = region["center_easting"] + dEW / 2.0

        # add to zoom history
        self.GetWindow().ZoomHistory(region["n"], region["s"], region["e"], region["w"])

    def GetMapScale(self, map=None):
        """Get current map scale

        :param map: Map instance (if none self.Map is used)
        """
        if not map:
            map = self.GetMap()

        region = map.region
        ppm = self.GetPPM()

        heightCm = region["rows"] / ppm[1] * 100
        widthCm = region["cols"] / ppm[0] * 100

        Debug.msg(
            4, "MapPanel.GetMapScale(): width_cm=%f, height_cm=%f" % (widthCm, heightCm)
        )

        xscale = (region["e"] - region["w"]) / (region["cols"] / ppm[0])
        yscale = (region["n"] - region["s"]) / (region["rows"] / ppm[1])
        scale = (xscale + yscale) / 2.0

        Debug.msg(
            3,
            "MapPanel.GetMapScale(): xscale=%f, yscale=%f -> scale=%f"
            % (xscale, yscale, scale),
        )

        return scale

    def GetProgressBar(self):
        """Returns progress bar

        Progress bar can be used by other classes.
        """
        return self.statusbarManager.GetProgressBar()

    def GetMap(self):
        """Returns current map (renderer) instance"""
        msg = self.GetMap.__name__
        raise NotImplementedError(msg)

    def GetWindow(self):
        """Returns current map window"""
        msg = self.GetWindow.__name__
        raise NotImplementedError(msg)

    def GetWindows(self):
        """Returns list of map windows"""
        msg = self.GetWindows.__name__
        raise NotImplementedError(msg)

    def GetMapToolbar(self):
        """Returns toolbar with zooming tools"""
        msg = self.GetMapToolbar.__name__
        raise NotImplementedError(msg)

    def GetToolbar(self, name):
        """Returns toolbar if exists and is active, else None."""
        if name in self.toolbars and self.toolbars[name].IsShown():
            return self.toolbars[name]

        return None

    def StatusbarUpdate(self):
        """Update statusbar content"""
        if self.statusbarManager:
            Debug.msg(5, "MapPanelBase.StatusbarUpdate()")
            self.statusbarManager.Update()

    def CoordinatesChanged(self):
        """Shows current coordinates on statusbar."""
        # assuming that the first mode is coordinates
        # probably shouldn't be here but a better solution isn't available now
        if self.statusbarManager:
            if self.statusbarManager.GetMode() == 0:
                self.statusbarManager.ShowItem("coordinates")

    def CreateStatusbar(self, statusbarItems):
        """Create statusbar (default items)."""
        # create statusbar and its manager
        statusbar = wx.StatusBar(self, id=wx.ID_ANY)
        statusbar.SetMinHeight(24)
        statusbar.SetFieldsCount(3)
        statusbar.SetStatusWidths([-6, -2, -1])
        self.statusbarManager = sb.SbManager(mapframe=self, statusbar=statusbar)

        # fill statusbar manager
        self.statusbarManager.AddStatusbarItemsByClass(
            statusbarItems, mapframe=self, statusbar=statusbar
        )
        self.statusbarManager.AddStatusbarItem(
            sb.SbRender(self, statusbar=statusbar, position=2)
        )
        return statusbar

    def AddStatusbarPane(self):
        """Add statusbar as a pane"""
        self._mgr.AddPane(
            self.statusbar,
            wx.aui.AuiPaneInfo()
            .Bottom()
            .MinSize(30, 30)
            .Fixed()
            .Name("statusbar")
            .CloseButton(False)
            .DestroyOnClose(True)
            .ToolbarPane()
            .Dockable(False)
            .PaneBorder(False)
            .Gripper(False),
        )

    def SetStatusText(self, *args):
        """Override wx.StatusBar method"""
        self.statusbar.SetStatusText(*args)

    def ShowStatusbar(self, show):
        """Show/hide statusbar and associated pane"""
        self._mgr.GetPane("statusbar").Show(show)
        self._mgr.Update()

    def IsStatusbarShown(self):
        """Check if statusbar is shown"""
        return self._mgr.GetPane("statusbar").IsShown()

    def StatusbarReposition(self):
        """Reposition items in statusbar"""
        if self.statusbarManager:
            self.statusbarManager.Reposition()

    def StatusbarEnableLongHelp(self, enable=True):
        """Enable/disable toolbars long help"""
        for toolbar in self.toolbars.values():
            if toolbar:
                toolbar.EnableLongHelp(enable)

    def ShowAllToolbars(self, show=True):
        action = self.RemoveToolbar if not show else self.AddToolbar
        for toolbar in self.GetToolbarNames():
            action(toolbar)

    def AreAllToolbarsShown(self):
        return self.GetMapToolbar().IsShown()

    def GetToolbarNames(self):
        """Return toolbar names"""
        return list(self.toolbars.keys())

    def AddToolbar(self):
        """Add defined toolbar to the window"""
        msg = self.AddToolbar.__name__
        raise NotImplementedError(msg)

    def RemoveToolbar(self, name, destroy=False):
        """Removes defined toolbar from the window

        :param name toolbar to remove
        :param destroy True to destroy otherwise toolbar is only hidden
        """
        self._mgr.DetachPane(self.toolbars[name])
        if destroy:
            self._toolSwitcher.RemoveToolbarFromGroup("mouseUse", self.toolbars[name])
            self.toolbars[name].Destroy()
            self.toolbars.pop(name)
        else:
            self.toolbars[name].Hide()

        self._mgr.Update()

    def IsPaneShown(self, name):
        """Check if pane (toolbar, mapWindow ...) of given name is currently shown"""
        if self._mgr.GetPane(name).IsOk():
            return self._mgr.GetPane(name).IsShown()
        return False

    def OnRender(self, event):
        """Re-render map composition (each map layer)"""
        msg = self.OnRender.__name__
        raise NotImplementedError(msg)

    def OnEnableDisableRender(self, event):
        """Enable/disable auto-rendering map composition (each map layer)"""
        if self.MapWindow.parent.mapWindowProperties.autoRender:
            self.MapWindow.parent.mapWindowProperties.autoRender = False
        else:
            self.MapWindow.parent.mapWindowProperties.autoRender = True

    def OnDraw(self, event):
        """Re-display current map composition"""
        self.MapWindow.UpdateMap(render=False)

    def OnErase(self, event):
        """Erase the canvas"""
        self.MapWindow.EraseMap()

    def OnZoomIn(self, event):
        """Zoom in the map."""
        self.MapWindow.SetModeZoomIn()

    def OnZoomOut(self, event):
        """Zoom out the map."""
        self.MapWindow.SetModeZoomOut()

    def _setUpMapWindow(self, mapWindow):
        """Binds map windows' zoom history signals to map toolbar."""
        # enable or disable zoom history tool
        if self.GetMapToolbar():
            mapWindow.zoomHistoryAvailable.connect(
                lambda: self.GetMapToolbar().Enable("zoomBack", enable=True)
            )
            mapWindow.zoomHistoryUnavailable.connect(
                lambda: self.GetMapToolbar().Enable("zoomBack", enable=False)
            )
        mapWindow.mouseMoving.connect(self.CoordinatesChanged)

    def OnPointer(self, event):
        """Sets mouse mode to pointer."""
        self.MapWindow.SetModePointer()

    def OnPan(self, event):
        """Panning, set mouse to drag"""
        self.MapWindow.SetModePan()

    def OnZoomBack(self, event):
        """Zoom last (previously stored position)"""
        self.MapWindow.ZoomBack()

    def OnZoomToMap(self, event):
        """
        Set display extents to match selected raster (including NULLs)
        or vector map.
        """
        self.MapWindow.ZoomToMap(layers=self.Map.GetListOfLayers())

    def OnZoomToWind(self, event):
        """Set display geometry to match computational region
        settings (set with g.region)
        """
        self.MapWindow.ZoomToWind()

    def OnZoomToDefault(self, event):
        """Set display geometry to match default region settings"""
        self.MapWindow.ZoomToDefault()

    def OnMapDisplayProperties(self, event):
        """Show Map Display Properties dialog"""
        from mapdisp.properties import MapDisplayPropertiesDialog

        dlg = MapDisplayPropertiesDialog(
            parent=self,
            mapframe=self,
            properties=self.mapWindowProperties,
            sbmanager=self.statusbarManager,
        )
        dlg.CenterOnParent()
        dlg.Show()


class SingleMapPanel(MapPanelBase):
    r"""Panel with one map window.

    It is base class for panels which needs only one map.

    Derived class should have \c self.MapWindow or
    it has to override GetWindow() methods.

    @note To access maps use getters only
    (when using class or when writing class itself).
    """

    def __init__(
        self,
        parent=None,
        giface=None,
        id=wx.ID_ANY,
        title="",
        Map=None,
        auimgr=None,
        name="",
        **kwargs,
    ):
        """

        :param parent: gui parent
        :param id: wx id
        :param title: window title
        :param map: instance of render.Map
        :param name: panel name
        :param kwargs: arguments passed to MapPanelBase
        """

        MapPanelBase.__init__(
            self,
            parent=parent,
            id=id,
            title=title,
            auimgr=auimgr,
            name=name,
            **kwargs,
        )

        self.Map = Map  # instance of render.Map

        #
        # initialize region values
        #
        if self.Map:
            self._initMap(Map=self.Map)

    def GetMap(self):
        """Returns map (renderer) instance"""
        return self.Map

    def GetWindow(self):
        """Returns map window"""
        return self.MapWindow

    def GetWindows(self):
        """Returns list of map windows"""
        return [self.MapWindow]

    def OnRender(self, event):
        """Re-render map composition (each map layer)"""
        self.GetWindow().UpdateMap(render=True, renderVector=True)

        # update statusbar
        self.StatusbarUpdate()


class DoubleMapPanel(MapPanelBase):
    """Panel with two map windows.

    It is base class for panels which needs two maps.
    There is no primary and secondary map. Both maps are equal.
    However, one map is current.

    It is expected that derived class will call _bindWindowsActivation()
    when both map windows will be initialized.

    Derived classes should have method GetMapToolbar() returns toolbar
    which has methods SetActiveMap() and Enable().

    @note To access maps use getters only
    (when using class or when writing class itself).

    .. todo:
        Use it in GCP manager (probably changes to both DoubleMapPanel
        and GCP MapPanel will be necessary).
    """

    def __init__(
        self,
        parent=None,
        id=wx.ID_ANY,
        title=None,
        firstMap=None,
        secondMap=None,
        auimgr=None,
        name=None,
        **kwargs,
    ):
        r"""

        \a firstMap is set as active (by assign it to \c self.Map).
        Derived class should assigning to \c self.MapWindow to make one
        map window current by default.

        :param parent: gui parent
        :param id: wx id
        :param title: window title
        :param name: panel name
        :param kwargs: arguments passed to MapPanelBase
        """

        MapPanelBase.__init__(
            self,
            parent=parent,
            id=id,
            title=title,
            auimgr=auimgr,
            name=name,
            **kwargs,
        )

        self.firstMap = firstMap
        self.secondMap = secondMap
        self.Map = firstMap

        #
        # initialize region values
        #
        self._initMap(Map=self.firstMap)
        self._initMap(Map=self.secondMap)
        self._bindRegions = False

    def _bindWindowsActivation(self):
        self.GetFirstWindow().Bind(wx.EVT_ENTER_WINDOW, self.ActivateFirstMap)
        self.GetSecondWindow().Bind(wx.EVT_ENTER_WINDOW, self.ActivateSecondMap)

    def _onToggleTool(self, id):
        if self._toolSwitcher.IsToolInGroup(id, "mouseUse"):
            self.GetFirstWindow().UnregisterAllHandlers()
            self.GetSecondWindow().UnregisterAllHandlers()

    def GetFirstMap(self):
        """Returns first Map instance"""
        return self.firstMap

    def GetSecondMap(self):
        """Returns second Map instance"""
        return self.secondMap

    def GetFirstWindow(self):
        """Get first map window"""
        return self.firstMapWindow

    def GetSecondWindow(self):
        """Get second map window"""
        return self.secondMapWindow

    def GetMap(self):
        r"""Returns current map (renderer) instance

        @note Use this method to access current map renderer.
        (It is not guaranteed that current map will be stored in
        \c self.Map in future versions.)
        """
        return self.Map

    def GetWindow(self):
        """Returns current map window

        :func:`GetMap()`
        """
        return self.MapWindow

    def GetWindows(self):
        """Return list of all windows"""
        return [self.firstMapWindow, self.secondMapWindow]

    def ActivateFirstMap(self, event=None):
        """Make first Map and MapWindow active and (un)bind regions of the two Maps."""
        if self.MapWindow == self.firstMapWindow:
            return

        self.Map = self.firstMap
        self.MapWindow = self.firstMapWindow
        self.GetMapToolbar().SetActiveMap(0)

        # bind/unbind regions
        if self._bindRegions:
            self.firstMapWindow.zoomChanged.connect(self.OnZoomChangedFirstMap)
            self.secondMapWindow.zoomChanged.disconnect(self.OnZoomChangedSecondMap)

    def ActivateSecondMap(self, event=None):
        """Make second Map and MapWindow active and (un)bind regions of the two Maps."""
        if self.MapWindow == self.secondMapWindow:
            return

        self.Map = self.secondMap
        self.MapWindow = self.secondMapWindow
        self.GetMapToolbar().SetActiveMap(1)

        if self._bindRegions:
            self.secondMapWindow.zoomChanged.connect(self.OnZoomChangedSecondMap)
            self.firstMapWindow.zoomChanged.disconnect(self.OnZoomChangedFirstMap)

    def SetBindRegions(self, on):
        """Set or unset binding display regions."""
        self._bindRegions = on
        if on:
            if self.MapWindow == self.firstMapWindow:
                self.firstMapWindow.zoomChanged.connect(self.OnZoomChangedFirstMap)
            else:
                self.secondMapWindow.zoomChanged.connect(self.OnZoomChangedSecondMap)
        else:  # noqa: PLR5501
            if self.MapWindow == self.firstMapWindow:
                self.firstMapWindow.zoomChanged.disconnect(self.OnZoomChangedFirstMap)
            else:
                self.secondMapWindow.zoomChanged.disconnect(self.OnZoomChangedSecondMap)

    def OnZoomChangedFirstMap(self):
        """Display region of the first window (Map) changed.

        Synchronize the region of the second map and re-render it.
        This is the default implementation which can be overridden.
        """
        region = self.GetFirstMap().GetCurrentRegion()
        self.GetSecondMap().region.update(region)
        self.Render(mapToRender=self.GetSecondWindow())

    def OnZoomChangedSecondMap(self):
        """Display region of the second window (Map) changed.

        Synchronize the region of the second map and re-render it.
        This is the default implementation which can be overridden.
        """
        region = self.GetSecondMap().GetCurrentRegion()
        self.GetFirstMap().region.update(region)
        self.Render(mapToRender=self.GetFirstWindow())

    def OnZoomIn(self, event):
        """Zoom in the map."""
        self.GetFirstWindow().SetModeZoomIn()
        self.GetSecondWindow().SetModeZoomIn()

    def OnZoomOut(self, event):
        """Zoom out the map."""
        self.GetFirstWindow().SetModeZoomOut()
        self.GetSecondWindow().SetModeZoomOut()

    def OnPan(self, event):
        """Panning, set mouse to pan"""
        self.GetFirstWindow().SetModePan()
        self.GetSecondWindow().SetModePan()

    def OnPointer(self, event):
        """Set pointer mode (dragging overlays)"""
        self.GetFirstWindow().SetModePointer()
        self.GetSecondWindow().SetModePointer()

    def OnQuery(self, event):
        """Set query mode"""
        self.GetFirstWindow().SetModeQuery()
        self.GetSecondWindow().SetModeQuery()

    def OnRender(self, event):
        """Re-render map composition (each map layer)"""
        kwargs = {}
        # Handle re-render map event (mouse click on toolbar Render map
        # tool, F5/Ctrl + R keyboard shortcut)
        if event and event.GetEventType() == wx.EVT_TOOL.typeId:
            kwargs = {"reRenderTool": True}
        self.Render(mapToRender=self.GetFirstWindow(), **kwargs)
        self.Render(mapToRender=self.GetSecondWindow(), **kwargs)

    def Render(self, mapToRender, reRenderTool=False):
        """Re-render map composition"""
        mapToRender.UpdateMap(
            render=True,
            renderVector=mapToRender == self.GetFirstWindow(),
            reRenderTool=reRenderTool,
        )

        # update statusbar
        self.StatusbarUpdate()

    def OnErase(self, event):
        """Erase the canvas"""
        self.Erase(mapToErase=self.GetFirstWindow())
        self.Erase(mapToErase=self.GetSecondWindow())

    def Erase(self, mapToErase):
        """Erase the canvas"""
        mapToErase.EraseMap()

    def OnDraw(self, event):
        """Re-display current map composition"""
        kwargs = {}
        # Handle display map event (mouse click on toolbar Display map
        # tool)
        if event and event.GetEventType() == wx.EVT_TOOL.typeId:
            kwargs = {"reRenderTool": True}
        self.Draw(mapToDraw=self.GetFirstWindow(), **kwargs)
        self.Draw(mapToDraw=self.GetSecondWindow(), **kwargs)

    def Draw(self, mapToDraw, reRenderTool=False):
        """Re-display current map composition"""
        mapToDraw.UpdateMap(render=False, reRenderTool=reRenderTool)


class FrameMixin:
    """Mixin class for wx.Panel that provides methods standardly
    used on wx.Frame widget"""

    def Show(self):
        self.GetParent().Show()

    def SetTitle(self, name):
        self.GetParent().SetTitle(name)

    def Raise(self):
        self.GetParent().Raise()

    def SetFocus(self):
        self.GetParent().SetFocus()

    def CenterOnScreen(self):
        self.GetParent().CenterOnScreen()

    def CentreOnScreen(self):
        self.GetParent().CentreOnScreen()

    def IsFullScreen(self):
        return self.GetParent().IsFullScreen()

    def IsIconized(self):
        self.GetParent().IsIconized()

    def Maximize(self):
        self.GetParent().Maximize()

    def ShowFullScreen(self, show):
        for toolbar in self.toolbars.keys():
            self._mgr.GetPane(self.toolbars[toolbar]).Show(self.IsFullScreen())
        if self.statusbar:
            self._mgr.GetPane("statusbar").Show(self.IsFullScreen())
        self._mgr.Update()

        self.GetParent().ShowFullScreen(show)

    def OnFullScreen(self, event):
        """!Switches frame to fullscreen mode, hides toolbars and statusbar"""
        self.ShowFullScreen(not self.IsFullScreen())
        event.Skip()

    def BindToFrame(self, *args):
        self.GetParent().Bind(*args)

    def Destroy(self):
        self.GetParent().Destroy()

    def GetPosition(self):
        return self.GetParent().GetPosition()

    def SetPosition(self, pt):
        self.GetParent().SetPosition(pt)

    def GetSize(self):
        return self.GetParent().GetSize()

    def SetSize(self, *args):
        self.GetParent().SetSize(*args)

    def Close(self):
        self.GetParent().Close()
