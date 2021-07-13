"""
@package gcp.mapdisplay

@brief Display to manage ground control points with two toolbars, one
for various display management functions, one for manipulating GCPs.

Classes:
- mapdisplay::MapFrame

(C) 2006-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Markus Metz
"""

import os
import platform

from core import globalvar
import wx
import wx.aui

from mapdisp.toolbars import MapToolbar
from gcp.toolbars import GCPDisplayToolbar, GCPManToolbar
from mapdisp.gprint import PrintOptions
from core.gcmd import GMessage
from gui_core.dialogs import GetImageHandlers, ImageSizeDialog
from gui_core.mapdisp import SingleMapFrame
from gui_core.wrap import Menu
from mapwin.buffered import BufferedMapWindow
from mapwin.base import MapWindowProperties

import mapdisp.statusbar as sb
import gcp.statusbar as sbgcp

# for standalone app
cmdfilename = None


class MapFrame(SingleMapFrame):
    """Main frame for map display window. Drawing takes place in
    child double buffered drawing window.
    """

    def __init__(
        self,
        parent,
        giface,
        title=_("Manage Ground Control Points"),
        toolbars=["gcpdisp"],
        Map=None,
        auimgr=None,
        name="GCPMapWindow",
        **kwargs,
    ):
        """Main map display window with toolbars, statusbar and
        DrawWindow

        :param giface: GRASS interface instance
        :param title: window title
        :param toolbars: array of activated toolbars, e.g. ['map', 'digit']
        :param map: instance of render.Map
        :param auimgs: AUI manager
        :param kwargs: wx.Frame attribures
        """

        SingleMapFrame.__init__(
            self,
            parent=parent,
            giface=giface,
            title=title,
            Map=Map,
            auimgr=auimgr,
            name=name,
            **kwargs,
        )

        self._giface = giface
        # properties are shared in other objects, so defining here
        self.mapWindowProperties = MapWindowProperties()
        self.mapWindowProperties.setValuesFromUserSettings()
        self.mapWindowProperties.alignExtent = True

        #
        # Add toolbars
        #
        for toolb in toolbars:
            self.AddToolbar(toolb)

        self.activemap = self.toolbars["gcpdisp"].togglemap
        self.activemap.SetSelection(0)

        self.SrcMap = self.grwiz.SrcMap  # instance of render.Map
        self.TgtMap = self.grwiz.TgtMap  # instance of render.Map
        self._mgr.SetDockSizeConstraint(0.5, 0.5)

        #
        # Add statusbar
        #

        # items for choice
        self.statusbarItems = [
            sb.SbCoordinates,
            sb.SbRegionExtent,
            sb.SbCompRegionExtent,
            sb.SbShowRegion,
            sb.SbResolution,
            sb.SbDisplayGeometry,
            sb.SbMapScale,
            sb.SbProjection,
            sbgcp.SbGoToGCP,
            sbgcp.SbRMSError,
        ]

        # create statusbar and its manager
        statusbar = self.CreateStatusBar(number=4, style=0)
        statusbar.SetStatusWidths([-5, -2, -1, -1])
        self.statusbarManager = sb.SbManager(mapframe=self, statusbar=statusbar)

        # fill statusbar manager
        self.statusbarManager.AddStatusbarItemsByClass(
            self.statusbarItems, mapframe=self, statusbar=statusbar
        )
        self.statusbarManager.AddStatusbarItem(
            sb.SbMask(self, statusbar=statusbar, position=2)
        )
        self.statusbarManager.AddStatusbarItem(
            sb.SbRender(self, statusbar=statusbar, position=3)
        )

        self.statusbarManager.SetMode(8)  # goto GCP

        #
        # Init map display (buffered DC & set default cursor)
        #
        self.grwiz.SwitchEnv("source")
        self.SrcMapWindow = BufferedMapWindow(
            parent=self,
            giface=self._giface,
            id=wx.ID_ANY,
            properties=self.mapWindowProperties,
            Map=self.SrcMap,
        )

        self.grwiz.SwitchEnv("target")
        self.TgtMapWindow = BufferedMapWindow(
            parent=self,
            giface=self._giface,
            id=wx.ID_ANY,
            properties=self.mapWindowProperties,
            Map=self.TgtMap,
        )
        self.MapWindow = self.SrcMapWindow
        self.Map = self.SrcMap
        self._setUpMapWindow(self.SrcMapWindow)
        self._setUpMapWindow(self.TgtMapWindow)
        self.SrcMapWindow.SetNamedCursor("cross")
        self.TgtMapWindow.SetNamedCursor("cross")
        # used to switch current map (combo box in toolbar)
        self.SrcMapWindow.mouseEntered.connect(
            lambda: self._setActiveMapWindow(self.SrcMapWindow)
        )
        self.TgtMapWindow.mouseEntered.connect(
            lambda: self._setActiveMapWindow(self.TgtMapWindow)
        )

        #
        # initialize region values
        #
        self._initMap(Map=self.SrcMap)
        self._initMap(Map=self.TgtMap)

        self.GetMapToolbar().SelectDefault()

        #
        # Bind various events
        #
        self.activemap.Bind(wx.EVT_CHOICE, self.OnUpdateActive)
        self.Bind(wx.EVT_SIZE, self.OnSize)

        #
        # Update fancy gui style
        #
        # AuiManager wants a CentrePane, workaround to get two equally sized
        # windows
        self.list = self.CreateGCPList()

        # self.SrcMapWindow.SetSize((300, 300))
        # self.TgtMapWindow.SetSize((300, 300))
        self.list.SetSize((100, 150))
        self._mgr.AddPane(
            self.list,
            wx.aui.AuiPaneInfo()
            .Name("gcplist")
            .Caption(_("GCP List"))
            .LeftDockable(False)
            .RightDockable(False)
            .PinButton()
            .FloatingSize((600, 200))
            .CloseButton(False)
            .DestroyOnClose(True)
            .Top()
            .Layer(1)
            .MinSize((200, 100)),
        )
        self._mgr.AddPane(
            self.SrcMapWindow,
            wx.aui.AuiPaneInfo()
            .Name("source")
            .Caption(_("Source Display"))
            .Dockable(False)
            .CloseButton(False)
            .DestroyOnClose(True)
            .Floatable(False)
            .Centre(),
        )
        self._mgr.AddPane(
            self.TgtMapWindow,
            wx.aui.AuiPaneInfo()
            .Name("target")
            .Caption(_("Target Display"))
            .Dockable(False)
            .CloseButton(False)
            .DestroyOnClose(True)
            .Floatable(False)
            .Right()
            .Layer(0),
        )

        srcwidth, srcheight = self.SrcMapWindow.GetSize()
        tgtwidth, tgtheight = self.TgtMapWindow.GetSize()
        srcwidth = (srcwidth + tgtwidth) / 2
        self._mgr.GetPane("target").Hide()
        self._mgr.Update()
        self._mgr.GetPane("source").BestSize((srcwidth, srcheight))
        self._mgr.GetPane("target").BestSize((srcwidth, srcheight))
        if self.show_target:
            self._mgr.GetPane("target").Show()
        else:
            self.activemap.Enable(False)
        # needed by Mac OS, does not harm on Linux, breaks display on Windows
        if platform.system() != "Windows":
            self._mgr.Update()

        #
        # Init print module and classes
        #
        self.printopt = PrintOptions(self, self.MapWindow)

        #
        # Initialization of digitization tool
        #
        self.digit = None

        # set active map
        self.MapWindow = self.SrcMapWindow
        self.Map = self.SrcMap

        # do not init zoom history here, that happens when zooming to map(s)

        #
        # Re-use dialogs
        #
        self.dialogs = {}
        self.dialogs["attributes"] = None
        self.dialogs["category"] = None
        self.dialogs["barscale"] = None
        self.dialogs["legend"] = None

        self.decorationDialog = None  # decoration/overlays

        # doing nice things in statusbar when other things are ready
        self.statusbarManager.Update()

    def _setUpMapWindow(self, mapWindow):
        # TODO: almost the same implementation as for MapFrameBase (only names differ)
        # enable or disable zoom history tool
        mapWindow.zoomHistoryAvailable.connect(
            lambda: self.GetMapToolbar().Enable("zoomback", enable=True)
        )
        mapWindow.zoomHistoryUnavailable.connect(
            lambda: self.GetMapToolbar().Enable("zoomback", enable=False)
        )
        mapWindow.mouseMoving.connect(self.CoordinatesChanged)

    def AddToolbar(self, name):
        """Add defined toolbar to the window

        Currently known toolbars are:
         - 'map'     - basic map toolbar
         - 'vdigit'  - vector digitizer
         - 'gcpdisp' - GCP Manager, Display
         - 'gcpman'  - GCP Manager, points management
         - 'nviz'    - 3D view mode
        """
        # default toolbar
        if name == "map":
            self.toolbars["map"] = MapToolbar(self, self._toolSwitcher)

            self._mgr.AddPane(
                self.toolbars["map"],
                wx.aui.AuiPaneInfo()
                .Name("maptoolbar")
                .Caption(_("Map Toolbar"))
                .ToolbarPane()
                .Top()
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2)
                .BestSize((self.toolbars["map"].GetSize())),
            )

        # GCP display
        elif name == "gcpdisp":
            self.toolbars["gcpdisp"] = GCPDisplayToolbar(self, self._toolSwitcher)

            self._mgr.AddPane(
                self.toolbars["gcpdisp"],
                wx.aui.AuiPaneInfo()
                .Name("gcpdisplaytoolbar")
                .Caption(_("GCP Display toolbar"))
                .ToolbarPane()
                .Top()
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2),
            )

            if not self.show_target:
                self.toolbars["gcpdisp"].Enable("zoommenu", enable=False)

            self.toolbars["gcpman"] = GCPManToolbar(self)

            self._mgr.AddPane(
                self.toolbars["gcpman"],
                wx.aui.AuiPaneInfo()
                .Name("gcpmanagertoolbar")
                .Caption(_("GCP Manager toolbar"))
                .ToolbarPane()
                .Top()
                .Row(1)
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2),
            )

        self._mgr.Update()

    def OnUpdateProgress(self, event):
        """
        Update progress bar info
        """
        self.GetProgressBar().UpdateProgress(event.layer, event.map)

        event.Skip()

    def OnFocus(self, event):
        """
        Change choicebook page to match display.
        Or set display for georectifying
        """
        # was in if layer manager but considering the state it was executed
        # always, moreover, there is no layer manager dependent code

        # in GCP Management, set focus to current MapWindow for mouse actions
        self.OnPointer(event)
        self.MapWindow.SetFocus()

        event.Skip()

    def OnDraw(self, event):
        """Re-display current map composition"""
        self.MapWindow.UpdateMap(render=False)

    def OnRender(self, event):
        """Re-render map composition (each map layer)"""
        # FIXME: remove qlayer code or use RemoveQueryLayer() now in mapdisp.frame
        # delete tmp map layers (queries)
        qlayer = self.Map.GetListOfLayers(name=globalvar.QUERYLAYER)
        for layer in qlayer:
            self.Map.DeleteLayer(layer)

        self.SrcMapWindow.UpdateMap(render=True)
        if self.show_target:
            self.TgtMapWindow.UpdateMap(render=True)

        # update statusbar
        self.StatusbarUpdate()

    def OnPointer(self, event):
        """Pointer button clicked"""
        self.SrcMapWindow.SetModePointer()
        self.TgtMapWindow.SetModePointer()
        # change the default cursor
        self.SrcMapWindow.SetNamedCursor("cross")
        self.TgtMapWindow.SetNamedCursor("cross")

    def OnZoomIn(self, event):
        """Zoom in the map."""
        self.SrcMapWindow.SetModeZoomIn()
        self.TgtMapWindow.SetModeZoomIn()

    def OnZoomOut(self, event):
        """Zoom out the map."""
        self.SrcMapWindow.SetModeZoomOut()
        self.TgtMapWindow.SetModeZoomOut()

    def OnPan(self, event):
        """Panning, set mouse to drag"""
        self.SrcMapWindow.SetModePan()
        self.TgtMapWindow.SetModePan()

    def OnErase(self, event):
        """
        Erase the canvas
        """
        self.MapWindow.EraseMap()

        if self.MapWindow == self.SrcMapWindow:
            win = self.TgtMapWindow
        elif self.MapWindow == self.TgtMapWindow:
            win = self.SrcMapWindow

        win.EraseMap()

    def SaveToFile(self, event):
        """Save map to image"""
        img = self.MapWindow.img
        if not img:
            GMessage(
                parent=self,
                message=_("Nothing to render (empty map). Operation canceled."),
            )
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
        dlg = wx.FileDialog(
            parent=self,
            message=_(
                "Choose a file name to save the image " "(no need to add extension)"
            ),
            wildcard=filetype,
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT,
        )

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return

            base, ext = os.path.splitext(path)
            fileType = ltype[dlg.GetFilterIndex()]["type"]
            extType = ltype[dlg.GetFilterIndex()]["ext"]
            if ext != extType:
                path = base + "." + extType

            self.MapWindow.SaveToFile(path, fileType, width, height)

        dlg.Destroy()

    def PrintMenu(self, event):
        """
        Print options and output menu for map display
        """
        point = wx.GetMousePosition()
        printmenu = Menu()
        # Add items to the menu
        setup = wx.MenuItem(printmenu, wx.ID_ANY, _("Page setup"))
        printmenu.AppendItem(setup)
        self.Bind(wx.EVT_MENU, self.printopt.OnPageSetup, setup)

        preview = wx.MenuItem(printmenu, wx.ID_ANY, _("Print preview"))
        printmenu.AppendItem(preview)
        self.Bind(wx.EVT_MENU, self.printopt.OnPrintPreview, preview)

        doprint = wx.MenuItem(printmenu, wx.ID_ANY, _("Print display"))
        printmenu.AppendItem(doprint)
        self.Bind(wx.EVT_MENU, self.printopt.OnDoPrint, doprint)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(printmenu)
        printmenu.Destroy()

    def OnZoomToRaster(self, event):
        """
        Set display extents to match selected raster map (ignore NULLs)
        """
        self.MapWindow.ZoomToMap(ignoreNulls=True)

    def OnZoomToSaved(self, event):
        """Set display geometry to match extents in
        saved region file
        """
        self.MapWindow.SetRegion(zoomOnly=True)

    def OnDisplayToWind(self, event):
        """Set computational region (WIND file) to match display
        extents
        """
        self.MapWindow.DisplayToWind()

    def SaveDisplayRegion(self, event):
        """Save display extents to named region file."""
        self.MapWindow.SaveDisplayRegion()

    def OnZoomMenu(self, event):
        """Popup Zoom menu"""
        point = wx.GetMousePosition()
        zoommenu = Menu()
        # Add items to the menu

        zoomwind = wx.MenuItem(
            zoommenu, wx.ID_ANY, _("Zoom to computational region (set with g.region)")
        )
        zoommenu.AppendItem(zoomwind)
        self.Bind(wx.EVT_MENU, self.OnZoomToWind, zoomwind)

        zoomdefault = wx.MenuItem(zoommenu, wx.ID_ANY, _("Zoom to default region"))
        zoommenu.AppendItem(zoomdefault)
        self.Bind(wx.EVT_MENU, self.OnZoomToDefault, zoomdefault)

        zoomsaved = wx.MenuItem(zoommenu, wx.ID_ANY, _("Zoom to saved region"))
        zoommenu.AppendItem(zoomsaved)
        self.Bind(wx.EVT_MENU, self.OnZoomToSaved, zoomsaved)

        savewind = wx.MenuItem(
            zoommenu, wx.ID_ANY, _("Set computational region from display")
        )
        zoommenu.AppendItem(savewind)
        self.Bind(wx.EVT_MENU, self.OnDisplayToWind, savewind)

        savezoom = wx.MenuItem(
            zoommenu, wx.ID_ANY, _("Save display geometry to named region")
        )
        zoommenu.AppendItem(savezoom)
        self.Bind(wx.EVT_MENU, self.SaveDisplayRegion, savezoom)

        # Popup the menu. If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(zoommenu)
        zoommenu.Destroy()

    def IsStandalone(self):
        """Check if Map display is standalone"""
        # we do not know and we do not care, so always False
        return True

    def GetLayerManager(self):
        """Get reference to Layer Manager

        :return: always None
        """
        return None

    def GetSrcWindow(self):
        return self.SrcMapWindow

    def GetTgtWindow(self):
        return self.TgtMapWindow

    def GetShowTarget(self):
        return self.show_target

    def GetMapToolbar(self):
        """Returns toolbar with zooming tools"""
        return self.toolbars["gcpdisp"]

    def _setActiveMapWindow(self, mapWindow):
        if not self.MapWindow == mapWindow:
            self.MapWindow = mapWindow
            self.Map = mapWindow.Map
            self.UpdateActive(mapWindow)
            # needed for wingrass
            self.SetFocus()
