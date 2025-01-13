"""
@package mapdisp.frame

@brief Map display with toolbar for various display management
functions, and additional toolbars (vector digitizer, 3d view).

Can be used either from Layer Manager or as d.mon backend.

Classes:
 - mapdisp::MapPanel

(C) 2006-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (SingleMapPanel, handlers support)
@author Anna Kratochvilova <kratochanna gmail.com> (SingleMapPanel)
@author Stepan Turek <stepan.turek seznam.cz> (handlers support)
"""

from __future__ import annotations

import copy
import os
from typing import TYPE_CHECKING

from core import globalvar
import wx
import wx.aui

from mapdisp.toolbars import MapToolbar, NvizIcons
from mapdisp.gprint import PrintOptions
from core.gcmd import GError, GMessage, RunCommand
from core.utils import ListOfCatsToRange, GetLayerNameFromCmd
from gui_core.dialogs import GetImageHandlers, ImageSizeDialog
from core.debug import Debug
from core.settings import UserSettings
from gui_core.mapdisp import SingleMapPanel, FrameMixin
from gui_core.query import QueryDialog, PrepareQueryResults
from mapwin.buffered import BufferedMapWindow
from mapwin.decorations import (
    ArrowController,
    BarscaleController,
    DtextController,
    LegendController,
    LegendVectController,
)
from mapwin.analysis import (
    MeasureAreaController,
    MeasureDistanceController,
    ProfileController,
)
from gui_core.forms import GUI
from core.giface import Notification
from gui_core.vselect import VectorSelectBase, VectorSelectHighlighter
from gui_core.wrap import Menu
from mapdisp import statusbar as sb
from main_window.page import MainPageBase

import grass.script as gs
from grass.pydispatch.signal import Signal

if TYPE_CHECKING:
    import lmgr.frame
    import main_window.frame


class MapPanel(SingleMapPanel, MainPageBase):
    """Main panel for map display window. Drawing takes place in
    child double buffered drawing window.
    """

    def __init__(
        self,
        parent,
        giface,
        title=_("Map Display"),
        toolbars=["map"],
        statusbar=True,
        tree=None,
        lmgr: main_window.frame.GMFrame | lmgr.frame.GMFrame | None = None,
        Map=None,
        auimgr=None,
        dockable=False,
        name="MapWindow",
        **kwargs,
    ):
        """Main map display window with toolbars, statusbar and
        2D map window, 3D map window and digitizer.

        :param toolbars: array of activated toolbars, e.g. ['map', 'digit']
        :param statusbar: True to add statusbar
        :param tree: reference to layer tree
        :param lmgr: Layer Manager
        :param map: instance of render.Map
        :param auimgr: AUI manager
        :param name: panel name
        :param kwargs: wx.Panel attributes
        """
        SingleMapPanel.__init__(
            self,
            parent=parent,
            title=title,
            Map=Map,
            auimgr=auimgr,
            name=name,
            **kwargs,
        )
        MainPageBase.__init__(self, dockable)

        self._giface = giface
        # Layer Manager object
        # need by GLWindow (a lot), VDigitWindow (a little bit)
        self._layerManager = lmgr
        # Layer Manager layer tree object
        # used for VDigit toolbar and window and GLWindow
        self.tree = tree

        # Saved Map Display output img size
        self._saved_output_img_size = None

        # Emitted when starting (switching to) 3D mode.
        # Parameter firstTime specifies if 3D was already activated.
        self.starting3dMode = Signal("MapPanel.starting3dMode")

        # Emitted when ending (switching from) 3D mode.
        self.ending3dMode = Signal("MapPanel.ending3dMode")

        # Emitted when closing display by closing its window.
        self.closingVNETDialog = Signal("MapPanel.closingVNETDialog")

        #
        # Add toolbars
        #
        for toolb in toolbars:
            self.AddToolbar(toolb)

        #
        # Add statusbar
        #
        self.statusbar = None
        self.statusbarManager = None
        if statusbar:
            # items for choice
            statusbarItems = [
                sb.SbCoordinates,
                sb.SbRegionExtent,
                sb.SbCompRegionExtent,
                sb.SbDisplayGeometry,
                sb.SbMapScale,
                sb.SbGoTo,
            ]
            self.statusbarItemsDisabledInNviz = (
                sb.SbDisplayGeometry,
                sb.SbMapScale,
            )
            self.statusbar = self.CreateStatusbar(statusbarItems)

            self.Map.GetRenderMgr().updateProgress.connect(
                self.statusbarManager.SetProgress
            )

        self.Map.GetRenderMgr().renderingFailed.connect(
            lambda cmd, error: self._giface.WriteError(
                _("Failed to run command '%(command)s'. Details:\n%(error)s")
                % {"command": " ".join(cmd), "error": error}
            )
        )

        # init decoration objects
        self.decorations = {}
        self._decorationWindows = {}

        self.mapWindowProperties.autoRenderChanged.connect(
            lambda value: self.OnRender(None) if value else None
        )

        #
        # Init map display (buffered DC & set default cursor)
        #
        self.MapWindow2D = BufferedMapWindow(
            self,
            giface=self._giface,
            Map=self.Map,
            properties=self.mapWindowProperties,
            overlays=self.decorations,
        )
        self.MapWindow2D.mapQueried.connect(self.Query)
        self.MapWindow2D.overlayActivated.connect(self._activateOverlay)
        self.MapWindow2D.overlayRemoved.connect(self.RemoveOverlay)
        self._setUpMapWindow(self.MapWindow2D)

        self.MapWindow2D.mouseHandlerUnregistered.connect(self.ResetPointer)

        self.MapWindow2D.InitZoomHistory()
        self.MapWindow2D.zoomChanged.connect(self.StatusbarUpdate)

        # register context menu actions
        if self.statusbar:
            self._registerContextMenuActions()

        self._giface.updateMap.connect(self.MapWindow2D.UpdateMap)
        # default is 2D display mode
        self.MapWindow = self.MapWindow2D
        self.MapWindow.SetNamedCursor("default")
        # used by vector digitizer
        self.MapWindowVDigit = None
        # used by Nviz (3D display mode)
        self.MapWindow3D = None

        if "map" in self.toolbars:
            self.toolbars["map"].SelectDefault()

        #
        # Bind various events
        #
        self.Bind(wx.EVT_SIZE, self.OnSize)

        #
        # Update fancy gui style
        #
        self._mgr.AddPane(
            self.MapWindow,
            wx.aui.AuiPaneInfo()
            .CentrePane()
            .Dockable(False)
            .BestSize((-1, -1))
            .Name("2d")
            .CloseButton(False)
            .DestroyOnClose(True)
            .Layer(0),
        )

        # statusbar
        if self.statusbar:
            self.AddStatusbarPane()

        self._mgr.Update()

        #
        # Init print module and classes
        #
        self.printopt = PrintOptions(self, self.MapWindow)

        #
        # Re-use dialogs
        #
        self.dialogs = {}
        self.dialogs["attributes"] = None
        self.dialogs["category"] = None
        self.dialogs["vnet"] = None
        self.dialogs["query"] = None
        self.dialogs["vselect"] = None

        # initialize layers to query (d.what.vect/rast)
        self._vectQueryLayers = []
        self._rastQueryLayers = []
        # initialize highlighter for vector features
        self._highlighter_layer = None

        self.measureController = None

        self._resize()

    def _registerContextMenuActions(self):
        """Register show/hide toolbars and statusbar context menu actions"""

        def show_hide_toolbar_label():
            return (
                _("Hide toolbars") if self.AreAllToolbarsShown() else _("Show toolbars")
            )

        def on_show_hide_toolbar(event):
            self.ShowAllToolbars(not self.AreAllToolbarsShown())

        self.MapWindow2D.RegisterContextAction(
            name="showAllToolbars",
            label=show_hide_toolbar_label,
            action=on_show_hide_toolbar,
        )

        def show_hide_statusbar_label():
            return (
                _("Hide statusbar") if self.IsStatusbarShown() else _("Show statusbar")
            )

        def on_show_hide_statusbar(event):
            self.ShowStatusbar(not self.IsStatusbarShown())

        self.MapWindow2D.RegisterContextAction(
            name="showStatusbar",
            label=show_hide_statusbar_label,
            action=on_show_hide_statusbar,
        )

    def GetMapWindow(self):
        return self.MapWindow

    def _addToolbarVDigit(self):
        """Add vector digitizer toolbar"""
        from vdigit.main import VDigit, haveVDigit
        from vdigit.toolbars import VDigitToolbar

        if not haveVDigit:
            from vdigit import errorMsg

            self.toolbars["map"].combo.SetValue(_("2D view"))

            GError(
                _("Unable to start wxGUI vector digitizer.\nDetails: %s") % errorMsg,
                parent=self,
            )
            return

        if not self.MapWindowVDigit:
            from vdigit.mapwindow import VDigitWindow

            self.MapWindowVDigit = VDigitWindow(
                parent=self,
                giface=self._giface,
                properties=self.mapWindowProperties,
                Map=self.Map,
                tree=self.tree,
                lmgr=self._layerManager,
                overlays=self.decorations,
            )
            self._setUpMapWindow(self.MapWindowVDigit)
            self.MapWindowVDigit.digitizingInfo.connect(
                lambda text: self.statusbarManager.statusbarItems[
                    "coordinates"
                ].SetAdditionalInfo(text)
            )
            self.MapWindowVDigit.digitizingInfoUnavailable.connect(
                lambda: self.statusbarManager.statusbarItems[
                    "coordinates"
                ].SetAdditionalInfo(None)
            )
            self.MapWindowVDigit.Show()
            self._mgr.AddPane(
                self.MapWindowVDigit,
                wx.aui.AuiPaneInfo()
                .CentrePane()
                .Dockable(False)
                .BestSize((-1, -1))
                .Name("vdigit")
                .CloseButton(False)
                .DestroyOnClose(True)
                .Layer(0),
            )

        self._switchMapWindow(self.MapWindowVDigit)

        if self._mgr.GetPane("2d").IsShown():
            self._mgr.GetPane("2d").Hide()
        elif self._mgr.GetPane("3d").IsShown():
            self._mgr.GetPane("3d").Hide()
        self._mgr.GetPane("vdigit").Show()
        if "vdigit" not in self.toolbars:
            self.toolbars["vdigit"] = VDigitToolbar(
                parent=self,
                toolSwitcher=self._toolSwitcher,
                MapWindow=self.MapWindow,
                digitClass=VDigit,
                giface=self._giface,
            )
            self.toolbars["vdigit"].quitDigitizer.connect(self.QuitVDigit)

            def openATM(selection):
                self._layerManager.OnShowAttributeTable(None, selection=selection)

            self.toolbars["vdigit"].openATM.connect(
                lambda selection: openATM(selection)
            )
            self.Map.layerAdded.connect(self._updateVDigitLayers)
        self.MapWindowVDigit.SetToolbar(self.toolbars["vdigit"])

        self._mgr.AddPane(
            self.toolbars["vdigit"],
            wx.aui.AuiPaneInfo()
            .Name("vdigittoolbar")
            .Caption(_("Vector Digitizer Toolbar"))
            .ToolbarPane()
            .Top()
            .Row(1)
            .LeftDockable(False)
            .RightDockable(False)
            .BottomDockable(False)
            .TopDockable(True)
            .CloseButton(False)
            .Layer(2)
            .BestSize(self.toolbars["vdigit"].GetBestSize()),
        )
        # change mouse to draw digitized line
        self.MapWindow.mouse["box"] = "point"
        self.MapWindow.zoomtype = 0
        self.MapWindow.pen = wx.Pen(colour="red", width=2, style=wx.SOLID)
        self.MapWindow.polypen = wx.Pen(colour="green", width=2, style=wx.SOLID)

    def _updateVDigitLayers(self, layer):
        """Update vdigit layers"""
        if "vdigit" in self.toolbars:
            self.toolbars["vdigit"].UpdateListOfLayers(updateTool=True)

    def AddNviz(self):
        """Add 3D view mode window"""
        from nviz.main import GLWindow, errorMsg, haveNviz

        # check for GLCanvas and OpenGL
        if not haveNviz:
            self.toolbars["map"].combo.SetValue(_("2D view"))
            GError(
                parent=self,
                message=_(
                    "Unable to switch to 3D display mode.\nThe Nviz python extension "
                    "was not found or loaded properly.\n"
                    "Switching back to 2D display mode.\n\nDetails: %s"
                )
                % errorMsg,
            )
            return

        # here was disabling 3D for other displays, now done on starting3dMode

        self.toolbars["map"].Enable2D(False)
        # add rotate tool to map toolbar
        self.toolbars["map"].InsertTool(
            (("rotate", NvizIcons["rotate"], self.OnRotate, wx.ITEM_CHECK, 7),)
        )  # 7 is position
        self._toolSwitcher.AddToolToGroup(
            group="mouseUse",
            toolbar=self.toolbars["map"],
            tool=self.toolbars["map"].rotate,
        )
        self.toolbars["map"].InsertTool(
            (
                (
                    "flyThrough",
                    NvizIcons["flyThrough"],
                    self.OnFlyThrough,
                    wx.ITEM_CHECK,
                    8,
                ),
            )
        )
        self._toolSwitcher.AddToolToGroup(
            group="mouseUse",
            toolbar=self.toolbars["map"],
            tool=self.toolbars["map"].flyThrough,
        )
        # update status bar

        self.statusbarManager.DisableStatusbarItemsByClass(
            self.statusbarItemsDisabledInNviz
        )
        self.mapWindowProperties.sbItem = 0

        # erase map window
        self.MapWindow.EraseMap()

        self._giface.WriteCmdLog(
            _("Starting 3D view mode..."), notification=Notification.HIGHLIGHT
        )
        self.SetStatusText(_("Please wait, loading data..."), 0)

        # create GL window
        if not self.MapWindow3D:
            self.MapWindow3D = GLWindow(
                self,
                giface=self._giface,
                id=wx.ID_ANY,
                frame=self,
                Map=self.Map,
                tree=self.tree,
                lmgr=self._layerManager,
            )
            self._setUpMapWindow(self.MapWindow3D)
            self.MapWindow3D.mapQueried.connect(self.Query)
            self._switchMapWindow(self.MapWindow3D)
            self.MapWindow.SetNamedCursor("default")

            # here was AddNvizTools in lmgr
            self.starting3dMode.emit(firstTime=True)

            # switch from MapWindow to MapWindowGL
            self._mgr.GetPane("2d").Hide()
            self._mgr.AddPane(
                self.MapWindow3D,
                wx.aui.AuiPaneInfo()
                .CentrePane()
                .Dockable(False)
                .BestSize((-1, -1))
                .Name("3d")
                .CloseButton(False)
                .DestroyOnClose(True)
                .Layer(0),
            )

            self.MapWindow3D.Show()
            self.MapWindow3D.ResetViewHistory()
            self.MapWindow3D.UpdateView(None)
            self.MapWindow3D.overlayActivated.connect(self._activateOverlay)
            self.MapWindow3D.overlayRemoved.connect(self.RemoveOverlay)
        else:
            self._switchMapWindow(self.MapWindow3D)
            os.environ["GRASS_REGION"] = self.Map.SetRegion(windres=True, windres3=True)
            self.MapWindow3D.GetDisplay().Init()
            self.MapWindow3D.LoadDataLayers()
            del os.environ["GRASS_REGION"]

            # switch from MapWindow to MapWindowGL
            self._mgr.GetPane("2d").Hide()
            self._mgr.GetPane("3d").Show()

            # here was AddNvizTools in lmgr and updating of pages
            self.starting3dMode.emit(firstTime=False)

            self.MapWindow3D.ResetViewHistory()

        # connect signals for updating overlays
        for overlay in self.decorations.values():
            overlay.overlayChanged.connect(self.MapWindow3D.UpdateOverlays)
        self.Map.GetRenderMgr().renderDone.connect(self.MapWindow3D._onUpdateOverlays)

        self._giface.updateMap.disconnect(self.MapWindow2D.UpdateMap)
        self._giface.updateMap.connect(self.MapWindow3D.UpdateMap)
        self.MapWindow3D.overlays = self.MapWindow2D.overlays
        # update overlays needs to be called after because getClientSize
        # is called during update and it must give reasonable values
        wx.CallAfter(self.MapWindow3D.UpdateOverlays)

        self.SetStatusText("", 0)
        self._mgr.Update()

    def Disable3dMode(self):
        """Disables 3D mode (NVIZ) in user interface."""
        # TODO: this is broken since item is removed but switch is drived by
        # index
        if "3D" in self.toolbars["map"].combo.GetString(1):
            self.toolbars["map"].combo.Delete(1)

    def RemoveNviz(self):
        """Restore 2D view. Can be called even if 3D is not active."""
        if not self.IsPaneShown("3d"):
            return
        try:
            self.toolbars["map"].RemoveTool(self.toolbars["map"].rotate)
            self.toolbars["map"].RemoveTool(self.toolbars["map"].flyThrough)
        except AttributeError:
            pass

        # update status bar
        self.SetStatusText(_("Please wait, unloading data..."), 0)
        self.statusbarManager.disabledItems = {}
        self.mapWindowProperties.sbItem = UserSettings.Get(
            group="display", key="statusbarMode", subkey="selection"
        )

        # unloading messages from library cause highlight anyway
        self._giface.WriteCmdLog(
            _("Switching back to 2D view mode..."),
            notification=Notification.NO_NOTIFICATION,
        )
        if self.MapWindow3D:
            self.MapWindow3D.OnClose(event=None)
        # switch from MapWindowGL to MapWindow
        self._mgr.GetPane("2d").Show()
        self._mgr.GetPane("3d").Hide()

        self._switchMapWindow(self.MapWindow2D)
        # here was RemoveNvizTools form lmgr
        self.ending3dMode.emit()
        try:
            self.MapWindow2D.overlays = self.MapWindow3D.overlays
        except AttributeError:
            pass
        # TODO: here we end because self.MapWindow3D is None for a while
        self._giface.updateMap.disconnect(self.MapWindow3D.UpdateMap)
        self._giface.updateMap.connect(self.MapWindow2D.UpdateMap)
        # disconnect overlays
        for overlay in self.decorations.values():
            overlay.overlayChanged.disconnect(self.MapWindow3D.UpdateOverlays)
        self.Map.GetRenderMgr().renderDone.disconnect(
            self.MapWindow3D._onUpdateOverlays
        )
        self.MapWindow3D.ClearTextures()

        self.MapWindow.UpdateMap()
        self._mgr.Update()
        self.GetMapToolbar().SelectDefault()

    def AddToolbar(self, name, fixed=False):
        """Add defined toolbar to the window

        Currently recognized toolbars are:
         - 'map'     - basic map toolbar
         - 'vdigit'  - vector digitizer

        :param name: toolbar to add
        :param fixed: fixed toolbar
        """
        # default toolbar
        if name == "map":
            if "map" not in self.toolbars:
                self.toolbars["map"] = MapToolbar(
                    self, toolSwitcher=self._toolSwitcher, giface=self._giface
                )

            self._mgr.AddPane(
                self.toolbars["map"],
                wx.aui.AuiPaneInfo()
                .Name("maptoolbar")
                .Caption(_("Map Toolbar"))
                .ToolbarPane()
                .Top()
                .Name("mapToolbar")
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2)
                .BestSize(self.toolbars["map"].GetBestSize()),
            )

        # nviz
        elif name == "nviz":
            self.toolbars["map"].combo.SetValue(_("3D view"))
            self.AddNviz()

        # vector digitizer
        elif name == "vdigit":
            self.toolbars["map"].combo.SetValue(_("Vector digitizer"))
            self._addToolbarVDigit()

        # raster digitizer
        elif name == "rdigit":
            self.toolbars["map"].combo.SetValue(_("Raster digitizer"))
            self.AddRDigit()

        if fixed:
            self.toolbars["map"].combo.Disable()

        self._mgr.Update()

    def RemoveToolbar(self, name, destroy=False):
        """Removes defined toolbar from the window

        :param name toolbar to remove
        :param destroy True to destroy otherwise toolbar is only hidden
        """
        super().RemoveToolbar(name, destroy)

        if name == "vdigit":
            self._mgr.GetPane("vdigit").Hide()
            self._mgr.GetPane("2d").Show()
            self._switchMapWindow(self.MapWindow2D)

        self.toolbars["map"].Enable2D(True)

        self._mgr.Update()

    def RemoveQueryLayer(self):
        """Removes temporary map layers (queries)"""
        qlayer = self.GetMap().GetListOfLayers(name=globalvar.QUERYLAYER)
        for layer in qlayer:
            self.GetMap().DeleteLayer(layer)

    def OnRender(self, event):
        """Re-render map composition (each map layer)"""
        self.RemoveQueryLayer()

        # deselect features in vdigit
        if self.GetToolbar("vdigit"):
            if self.MapWindow.digit:
                self.MapWindow.digit.GetDisplay().SetSelected([])
            self.MapWindow.UpdateMap(
                render=True,
                renderVector=True,
                reRenderTool=True,
            )
        else:
            self.MapWindow.UpdateMap(render=True, reRenderTool=True)

        # reset dialog with selected features
        if self.dialogs["vselect"]:
            self.dialogs["vselect"].Reset()

        # update statusbar
        self.StatusbarUpdate()

    def OnPointer(self, event):
        """Pointer button clicked"""
        self.MapWindow.SetModePointer()

        if self.GetToolbar("vdigit"):
            self.toolbars["vdigit"].action["id"] = -1
            self.toolbars["vdigit"].action["desc"] = ""

    def OnSelect(self, event):
        """Vector feature selection button clicked"""
        layerList = self._giface.GetLayerList()
        layerSelected = layerList.GetSelectedLayer()
        if not self.dialogs["vselect"]:
            if layerSelected is None:
                GMessage(_("No map layer selected. Operation canceled."))
                return

            self.dialogs["vselect"] = VectorSelectBase(self.parent, self._giface)
            self.dialogs["vselect"].CreateDialog(createButton=True)
            self.dialogs["vselect"].onCloseDialog.connect(
                self._onCloseVectorSelectDialog
            )

    def _onCloseVectorSelectDialog(self):
        self.dialogs["vselect"] = None

    def OnRotate(self, event):
        """Rotate 3D view"""
        self.MapWindow.mouse["use"] = "rotate"

        # change the cursor
        self.MapWindow.SetNamedCursor("hand")

    def OnFlyThrough(self, event):
        """Fly-through mode"""
        self.MapWindow.mouse["use"] = "fly"

        # change the cursor
        self.MapWindow.SetNamedCursor("hand")
        self.MapWindow.SetFocus()

    def SaveToFile(self, event):
        """Save map to image"""
        filetype, ltype = self._prepareSaveToFile()
        if not ltype:
            return

        # get size
        dlg = ImageSizeDialog(self, img_size=self._saved_output_img_size)
        dlg.CentreOnParent()
        if dlg.ShowModal() != wx.ID_OK:
            dlg.Destroy()
            return
        self._saved_output_img_size = dlg.GetValues()
        dlg.Destroy()

        # get filename
        dlg = wx.FileDialog(
            parent=self,
            message=_(
                "Choose a file name to save the image (no need to add extension)"
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

            self.MapWindow.SaveToFile(path, fileType, *self._saved_output_img_size)

        dlg.Destroy()

    def DOutFile(self, command, callback=None):
        """Saves map to image by running d.out.file from gui or d.mon.
        Command is expected to be validated by parser.
        """
        filetype, ltype = self._prepareSaveToFile()
        if not ltype:
            return
        width, height = self.MapWindow.GetClientSize()
        for param in command[1:]:
            try:
                p, val = param.split("=")
            except ValueError:
                # --overwrite
                continue
            if p == "format":  # must be there
                extType = "ppm" if self.IsPaneShown("3d") else val
            if p == "output":  # must be there
                name = val
            elif p == "size":
                width, height = val.split(",")

        base, ext = os.path.splitext(name)
        if not ext:
            name = base + "." + extType
        elif ext[1:] != extType:
            extType = ext[1:]

        # default type is PNG
        bitmapType = "ppm" if self.IsPaneShown("3d") else wx.BITMAP_TYPE_PNG
        for each in ltype:
            if each["ext"] == extType:
                bitmapType = each["type"]
                break
        self.MapWindow.SaveToFile(name, bitmapType, int(width), int(height), callback)

    def DOutFileOptData(self, dcmd, layer, params, propwin):
        """Dummy function which is called when d.out.file is called
        and returns parsed and validated command which is then passed
        to DOutFile method."""
        if not dcmd:
            return

        self.DOutFile(dcmd)

    def DToRast(self, command):
        """Saves currently loaded composition of layers as a raster map."""

        def _DToRastDone():
            # import back as red, green, blue rasters
            returncode, messages = RunCommand(
                "r.in.gdal",
                flags="o",
                input=pngFile,
                output=tmpName,
                quiet=True,
                overwrite=overwrite,
                getErrorMsg=True,
            )
            if returncode != 0:
                self._giface.WriteError(_("Failed to run d.to.rast:\n") + messages)
                return
            # set region for composite
            gs.use_temp_region()
            returncode, messages = RunCommand(
                "g.region", raster=tmpName + ".red", quiet=True, getErrorMsg=True
            )
            if returncode != 0:
                gs.del_temp_region()
                self._giface.WriteError(_("Failed to run d.to.rast:\n") + messages)
                return
            # composite
            returncode, messages = RunCommand(
                "r.composite",
                red=tmpName + ".red",
                green=tmpName + ".green",
                blue=tmpName + ".blue",
                output=outputRaster,
                quiet=True,
                overwrite=overwrite,
                getErrorMsg=True,
            )
            gs.del_temp_region()
            RunCommand(
                "g.remove",
                type="raster",
                flags="f",
                quiet=True,
                name=[tmpName + ".red", tmpName + ".green", tmpName + ".blue"],
            )
            if returncode != 0:
                self._giface.WriteError(_("Failed to run d.to.rast:\n") + messages)
                gs.try_remove(pngFile)
                return

            # alignExtent changes only region variable
            oldRegion = self.GetMap().GetCurrentRegion().copy()
            self.GetMap().AlignExtentFromDisplay()
            region = self.GetMap().GetCurrentRegion().copy()
            self.GetMap().region.update(oldRegion)
            RunCommand(
                "r.region",
                map=outputRaster,
                n=region["n"],
                s=region["s"],
                e=region["e"],
                w=region["w"],
                quiet=True,
            )
            gs.try_remove(pngFile)

        if self.IsPaneShown("3d"):
            self._giface.WriteError(_("d.to.rast can be used only in 2D mode."))
            return
        outputRaster = None
        overwrite = False
        for param in command[1:]:
            try:
                p, val = param.split("=")
                if p == "output":
                    outputRaster = val
            except ValueError:
                if param.startswith("--overwrite"):
                    overwrite = True

        if not outputRaster:
            return
        # output file as PNG
        tmpName = "d_to_rast_tmp"
        pngFile = gs.tempfile(create=False) + ".png"
        dOutFileCmd = ["d.out.file", "output=" + pngFile, "format=png"]
        self.DOutFile(dOutFileCmd, callback=_DToRastDone)

    def DToRastOptData(self, dcmd, layer, params, propwin):
        """Dummy function which is called when d.to.rast is called
        and returns parsed and validated command which is then passed
        to DToRast method."""
        if not dcmd:
            return

        self.DToRast(dcmd)

    def _prepareSaveToFile(self):
        """Get wildcards and format extensions."""
        if self.IsPaneShown("3d"):
            filetype = "TIF file (*.tif)|*.tif|PPM file (*.ppm)|*.ppm"
            ltype = [{"ext": "tif", "type": "tif"}, {"ext": "ppm", "type": "ppm"}]
        else:
            img = self.MapWindow.img
            if not img:
                GMessage(
                    parent=self,
                    message=_("Nothing to render (empty map). Operation canceled."),
                )
                return None, None
            filetype, ltype = GetImageHandlers(img)
        return filetype, ltype

    def PrintMenu(self, event):
        """
        Print options and output menu for map display
        """
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

    def CleanUp(self):
        """Clean up before closing map display.
        End digitizer/nviz."""
        Debug.msg(2, "MapPanel.CleanUp()")
        self.Map.Clean()
        # close edited map and 3D tools properly
        if self.GetToolbar("vdigit"):
            maplayer = self.toolbars["vdigit"].GetLayer()
            if maplayer:
                self.toolbars["vdigit"].OnExit()
        self.RemoveNviz()
        if hasattr(self, "rdigit") and self.rdigit:
            self.rdigit.CleanUp()
        if self.dialogs["vnet"]:
            self.closingVNETDialog.emit()
        self._mgr.UnInit()

    def OnCloseWindow(self, event, askIfSaveWorkspace=True):
        """Window closed.
        Also close associated layer tree page
        """
        Debug.msg(2, "MapPanel.OnCloseWindow()")
        if self.canCloseCallback:
            pgnum_dict = self.canCloseCallback(askIfSaveWorkspace=askIfSaveWorkspace)
            if pgnum_dict is not None:
                self.CleanUp()
                if pgnum_dict["layers"] > -1:
                    if self.IsDockable():
                        self.closingPage.emit(
                            pgnum_dict=pgnum_dict, is_docked=self.IsDocked()
                        )
                        if not self.IsDocked():
                            frame = self.GetParent()
                            frame.Destroy()
                    else:
                        self.closingPage.emit(pgnum_dict=pgnum_dict)
                    # Destroy is called when notebook page is deleted
        else:
            self.CleanUp()
            self.Destroy()

    def Query(self, x, y):
        """Query selected layers.

        :param x,y: coordinates
        """
        if self._vectQueryLayers or self._rastQueryLayers:
            rast = self._rastQueryLayers
            vect = self._vectQueryLayers
        else:
            layers = self._giface.GetLayerList().GetSelectedLayers(checkedOnly=False)
            rast = []
            vect = []
            for layer in layers:
                if layer.type == "command":
                    continue
                name, found = GetLayerNameFromCmd(layer.cmd)
                if not found:
                    continue
                ltype = layer.maplayer.GetType()
                if ltype == "raster":
                    rast.append(name)
                elif ltype in {"rgb", "his"}:
                    for iname in name.split("\n"):
                        rast.append(iname)
                elif ltype in {"vector", "thememap", "themechart"}:
                    vect.append(name)
            if vect:
                # check for vector maps open to be edited
                digitToolbar = self.GetToolbar("vdigit")
                if digitToolbar:
                    lmap = digitToolbar.GetLayer().GetName()
                    for name in vect:
                        if lmap == name:
                            self._giface.WriteWarning(
                                _("Vector map <%s> opened for editing - skipped.")
                                % lmap
                            )
                            vect.remove(name)

        if not (rast + vect):
            GMessage(
                parent=self,
                message=_("No raster or vector map layer selected for querying."),
            )
            return

        # set query snap distance for v.what at map unit equivalent of 10
        # pixels
        qdist = 10.0 * ((self.Map.region["e"] - self.Map.region["w"]) / self.Map.width)

        # TODO: replace returning None by exception or so
        try:
            east, north = self.MapWindow.Pixel2Cell((x, y))
        except TypeError:
            return

        if not self.IsPaneShown("3d"):
            self.QueryMap(east, north, qdist, rast, vect)
        else:
            if rast:
                self.MapWindow.QuerySurface(x, y)
            if vect:
                self.QueryMap(east, north, qdist, rast=[], vect=vect)

    def SetQueryLayersAndActivate(self, ltype, maps):
        """Activate query mode and set layers to query.
        This method is used for querying in d.mon using d.what.rast/vect"""
        self.toolbars["map"].SelectTool(self.toolbars["map"].query)
        if ltype == "vector":
            self._vectQueryLayers = maps
        elif ltype == "raster":
            self._rastQueryLayers = maps

    def QueryMap(self, east, north, qdist, rast, vect):
        """Query raster or vector map layers by r/v.what

        :param east,north: coordinates
        :param qdist: query distance
        :param rast: raster map names
        :param vect: vector map names
        """
        Debug.msg(
            1, "QueryMap(): raster=%s vector=%s" % (",".join(rast), ",".join(vect))
        )
        if self._highlighter_layer is None:
            self._highlighter_layer = VectorSelectHighlighter(
                mapdisp=self._giface.GetMapDisplay(), giface=self._giface
            )

        rastQuery = []
        vectQuery = []
        env = os.environ.copy()
        for raster in rast:
            env["GRASS_REGION"] = gs.region_env(raster=raster)
            rastQuery += gs.raster_what(
                map=raster, coord=(east, north), localized=True, env=env
            )
        if vect:
            encoding = UserSettings.Get(group="atm", key="encoding", subkey="value")
            try:
                vectQuery = gs.vector_what(
                    map=vect,
                    coord=(east, north),
                    distance=qdist,
                    encoding=encoding,
                    multiple=True,
                )
            except gs.ScriptError:
                GError(
                    parent=self,
                    message=_(
                        "Failed to query vector map(s) <{maps}>. "
                        "Check database settings and topology."
                    ).format(maps=",".join(vect)),
                )

        self._highlighter_layer.Clear()
        if vectQuery and "Category" in vectQuery[0]:
            self._queryHighlight(vectQuery)

        result = rastQuery + vectQuery
        result = PrepareQueryResults(coordinates=(east, north), result=result)
        if self.dialogs["query"]:
            self.dialogs["query"].Raise()
            self.dialogs["query"].SetData(result)
        else:
            self.dialogs["query"] = QueryDialog(parent=self, data=result)
            self.dialogs["query"].Bind(wx.EVT_CLOSE, self._oncloseQueryDialog)
            self.dialogs["query"].redirectOutput.connect(self._onRedirectQueryOutput)
            self.dialogs["query"].Show()

    def _oncloseQueryDialog(self, event):
        self.dialogs["query"] = None
        self._vectQueryLayers = []
        self._rastQueryLayers = []
        self._highlighter_layer.Clear()
        self._highlighter_layer = None
        event.Skip()

    def _onRedirectQueryOutput(self, output, style="log"):
        """Writes query output into console"""
        if style == "log":
            self._giface.WriteLog(output, notification=Notification.MAKE_VISIBLE)
        elif style == "cmd":
            self._giface.WriteCmdLog(output)

    def _queryHighlight(self, vectQuery):
        """Highlight category from query."""
        if len(vectQuery) > 0:
            self._highlighter_layer.SetLayer(vectQuery[0]["Layer"])
            self._highlighter_layer.SetMap(
                vectQuery[0]["Map"] + "@" + vectQuery[0]["Mapset"]
            )
            tmp = [i["Category"] for i in vectQuery]

            self._highlighter_layer.SetCats(tmp)
            self._highlighter_layer.DrawSelected()

    def OnQuery(self, event):
        """Query tools menu"""
        self.MapWindow.mouse["use"] = "query"
        self.MapWindow.mouse["box"] = "point"
        self.MapWindow.zoomtype = 0

        # change the cursor
        self.MapWindow.SetNamedCursor("cross")

    def AddTmpVectorMapLayer(
        self, name, cats, useId: bool = False, addLayer: bool = True
    ):
        """Add temporal vector map layer to map composition

        :param name: name of map layer
        :param useId: use feature id instead of category
        """
        # color settings from ATM
        color = UserSettings.Get(group="atm", key="highlight", subkey="color")
        colorStr = str(color[0]) + ":" + str(color[1]) + ":" + str(color[2])

        # icon used in vector display and its size
        icon = ""
        size = 0
        # here we know that there is one selected layer and it is vector
        layerSelected = self._giface.GetLayerList().GetSelectedLayer()
        if not layerSelected:
            return None

        vparam = layerSelected.cmd
        for p in vparam:
            if "=" in p:
                parg, pval = p.split("=", 1)
                if parg == "icon":
                    icon = pval
                elif parg == "size":
                    size = float(pval)

        pattern = [
            "d.vect",
            "map=%s" % name,
            "color=%s" % colorStr,
            "fill_color=%s" % colorStr,
            "width=%d" % UserSettings.Get(group="atm", key="highlight", subkey="width"),
        ]
        if icon != "":
            pattern.append("icon=%s" % icon)
        if size > 0:
            pattern.append("size=%i" % size)

        if useId:
            cmd = pattern
            cmd.append("-i")
            cmd.append("cats=%s" % str(cats))
        else:
            cmd = []
            for layer in cats.keys():
                cmd.append(copy.copy(pattern))
                lcats = cats[layer]
                cmd[-1].append("layer=%d" % layer)
                cmd[-1].append("cats=%s" % ListOfCatsToRange(lcats))

        if not addLayer:
            return cmd

        args = {}
        if useId:
            args["ltype"] = "vector"
        else:
            args["ltype"] = "command"
        return self.Map.AddLayer(
            name=globalvar.QUERYLAYER,
            command=cmd,
            active=True,
            hidden=True,
            opacity=1.0,
            render=True,
            **args,
        )

    def OnMeasureDistance(self, event):
        self._onMeasure(MeasureDistanceController)

    def OnMeasureArea(self, event):
        self._onMeasure(MeasureAreaController)

    def _onMeasure(self, controller):
        """Starts measurement mode.

        :param controller: measurement class (MeasureDistanceController,
                           MeasureAreaController)
        """
        self.measureController = controller(self._giface, mapWindow=self.GetMapWindow())
        # assure that the mode is ended and lines are cleared whenever other
        # tool is selected
        self._toolSwitcher.toggleToolChanged.connect(
            lambda: self.measureController.Stop()
        )
        self.measureController.Start()

    def OnProfile(self, event):
        """Launch profile tool"""
        rasters = []
        layers = self._giface.GetLayerList().GetSelectedLayers()
        for layer in layers:
            if layer.type == "raster":
                rasters.append(layer.maplayer.name)
        self.Profile(rasters=rasters)

    def Profile(self, rasters=None):
        """Launch profile tool"""
        from wxplot.profile import ProfileFrame

        self.profileController = ProfileController(
            self._giface, mapWindow=self.GetMapWindow()
        )
        win = ProfileFrame(
            parent=self,
            giface=self._giface,
            rasterList=rasters,
            units=self.Map.projinfo["units"],
            controller=self.profileController,
        )
        win.Show()
        # Open raster select dialog to make sure that a raster (and
        # the desired raster) is selected to be profiled
        win.OnSelectRaster(None)

    def OnHistogramPyPlot(self, event):
        """Init PyPlot histogram display canvas and tools"""
        raster = [
            layer.maplayer.GetName()
            for layer in self._giface.GetLayerList().GetSelectedLayers()
            if layer.maplayer.GetType() == "raster"
        ]

        from wxplot.histogram import HistogramPlotFrame

        win = HistogramPlotFrame(parent=self, giface=self._giface, rasterList=raster)
        win.CentreOnParent()
        win.Show()

    def OnScatterplot(self, event):
        """Init PyPlot scatterplot display canvas and tools"""
        raster = [
            layer.maplayer.GetName()
            for layer in self._giface.GetLayerList().GetSelectedLayers()
            if layer.maplayer.GetType() == "raster"
        ]

        from wxplot.scatter import ScatterFrame

        win = ScatterFrame(parent=self, giface=self._giface, rasterList=raster)

        win.CentreOnParent()
        win.Show()
        # Open raster select dialog to make sure that at least 2 rasters (and the
        # desired rasters) are selected to be plotted
        win.OnSelectRaster(None)

    def OnHistogram(self, event):
        """Init histogram display canvas and tools"""
        from modules.histogram import HistogramFrame

        win = HistogramFrame(self, giface=self._giface)

        win.CentreOnParent()
        win.Show()
        win.Refresh()
        win.Update()

    def _activateOverlay(self, overlayId):
        """Launch decoration dialog according to overlay id.

        :param overlayId: id of overlay
        """
        if overlayId in self.decorations:
            dlg = self.decorations[overlayId].dialog
            if dlg.IsShown():
                dlg.SetFocus()
                dlg.Raise()
            else:
                dlg.Show()

    def RemoveOverlay(self, overlayId):
        """Hide overlay.

        :param overlayId: id of overlay
        """
        del self._decorationWindows[self.decorations[overlayId].dialog]
        self.decorations[overlayId].Remove()
        del self.decorations[overlayId]

    def AddBarscale(self, cmd=None):
        """Handler for scale bar map decoration menu selection."""
        if self.IsPaneShown("3d"):
            self.MapWindow3D.SetDrawScalebar((70, 70))
            return

        if cmd:
            show = False
        else:
            show = True
            cmd = ["d.barscale"]

        # Decoration overlay control dialog
        GUI(parent=self, giface=self._giface, show=show, modal=False).ParseCommand(
            cmd, completed=(self.GetOptData, None, None)
        )

        self.MapWindow.mouse["use"] = "pointer"

    def AddLegendRast(self, cmd=None):
        """Handler for legend raster map decoration menu selection."""

        if cmd:
            show = False
        else:
            show = True
            cmd = ["d.legend"]
            layers = self._giface.GetLayerList().GetSelectedLayers()
            for layer in layers:
                if layer.type == "raster":
                    cmd.append("raster={rast}".format(rast=layer.maplayer.name))
                    break

        GUI(parent=self, giface=self._giface, show=show, modal=False).ParseCommand(
            cmd, completed=(self.GetOptData, None, None)
        )

        self.MapWindow.mouse["use"] = "pointer"

    def AddLegendVect(self, cmd=None, showDialog=None):
        """Handler for legend vector map decoration menu selection."""

        if cmd:
            show = False
        else:
            show = True
            cmd = ["d.legend.vect"]

        GUI(parent=self, giface=self._giface, show=show, modal=False).ParseCommand(
            cmd, completed=(self.GetOptData, None, None)
        )

        self.MapWindow.mouse["use"] = "pointer"

    def AddArrow(self, cmd=None):
        """Handler for north arrow menu selection."""
        if self.IsPaneShown("3d"):
            # here was opening of appearance page of nviz notebook
            # but now moved to MapWindow3D where are other problematic nviz
            # calls
            self.MapWindow3D.SetDrawArrow((70, 70))
            return

        if cmd:
            show = False
        else:
            show = True
            cmd = ["d.northarrow"]

        # Decoration overlay control dialog
        GUI(parent=self, giface=self._giface, show=show, modal=False).ParseCommand(
            cmd, completed=(self.GetOptData, None, None)
        )

        self.MapWindow.mouse["use"] = "pointer"

    def AddDtext(self, cmd=None):
        """Handler for d.text menu selection."""
        if cmd:
            show = False
        else:
            show = True
            cmd = ["d.text"]

        # Decoration overlay control dialog
        GUI(parent=self, giface=self._giface, show=show, modal=False).ParseCommand(
            cmd, completed=(self.GetOptData, None, None)
        )

        self.MapWindow.mouse["use"] = "pointer"

    def GetOptData(self, dcmd, layer, params, propwin):
        """Called after options are set through module dialog.

        :param dcmd: resulting command
        :param layer: not used
        :param params: module parameters (not used)
        :param propwin: dialog window
        """

        if not dcmd:
            return
        if propwin in self._decorationWindows:
            overlay = self._decorationWindows[propwin]
        else:
            cmd = dcmd[0]
            if cmd == "d.northarrow":
                overlay = ArrowController(self.Map, self._giface)
            elif cmd == "d.barscale":
                overlay = BarscaleController(self.Map, self._giface)
            elif cmd == "d.legend":
                overlay = LegendController(self.Map, self._giface)
            elif cmd == "d.legend.vect":
                overlay = LegendVectController(self.Map, self._giface)
            elif cmd == "d.text":
                overlay = DtextController(self.Map, self._giface)

            self.decorations[overlay.id] = overlay
            overlay.overlayChanged.connect(
                lambda: self.MapWindow2D.UpdateMap(render=False, renderVector=False)
            )
            if self.IsPaneShown("3d"):
                overlay.overlayChanged.connect(self.MapWindow3D.UpdateOverlays)

            overlay.dialog = propwin
            self._decorationWindows[propwin] = overlay

        overlay.cmd = dcmd
        overlay.Show()

    def OnZoomToMap(self, event):
        """Set display extents to match selected raster (including
        NULLs) or vector map.
        """
        Debug.msg(3, "MapPanel.OnZoomToMap()")
        self.MapWindow.ZoomToMap(layers=None)

    def OnZoomToRaster(self, event):
        """Set display extents to match selected raster map (ignore NULLs)"""
        self.MapWindow.ZoomToMap(ignoreNulls=True)

    def OnZoomToSaved(self, event):
        """Set display geometry to match extents in
        saved region file
        """
        self.MapWindow.SetRegion(zoomOnly=True)

    def OnSetDisplayToWind(self, event):
        """Set computational region (WIND file) to match display
        extents
        """
        self.MapWindow.DisplayToWind()

    def OnSetWindToRegion(self, event):
        """Set computational region (WIND file) from named region
        file
        """
        self.MapWindow.SetRegion(zoomOnly=False)

    def OnSetExtentToWind(self, event):
        """Set computational region extent interactively"""
        self.MapWindow.SetModeDrawRegion()

    def OnSaveDisplayRegion(self, event):
        """Save display extents to named region file."""
        self.MapWindow.SaveRegion(display=True)

    def OnSaveWindRegion(self, event):
        """Save computational region to named region file."""
        self.MapWindow.SaveRegion(display=False)

    def OnZoomMenu(self, event):
        """Popup Zoom menu"""
        zoommenu = Menu()

        for label, handler in (
            (_("Zoom to default region"), self.OnZoomToDefault),
            (_("Zoom to saved region"), self.OnZoomToSaved),
            (None, None),
            (
                _("Set computational region extent from display"),
                self.OnSetDisplayToWind,
            ),
            (
                _("Set computational region extent interactively"),
                self.OnSetExtentToWind,
            ),
            (_("Set computational region from named region"), self.OnSetWindToRegion),
            (None, None),
            (_("Save display geometry to named region"), self.OnSaveDisplayRegion),
            (_("Save computational region to named region"), self.OnSaveWindRegion),
        ):
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

    def SetProperties(
        self,
        render=False,
        mode=0,
        showCompExtent=False,
        constrainRes=False,
        projection=False,
        alignExtent=True,
    ):
        """Set properties of map display window"""
        self.mapWindowProperties.autoRender = render
        if self.statusbarManager:
            self.statusbarManager.SetMode(mode)
        self.mapWindowProperties.useDefinedProjection = projection
        self.mapWindowProperties.showRegion = showCompExtent
        self.mapWindowProperties.alignExtent = alignExtent
        self.mapWindowProperties.resolution = constrainRes

    def GetMapToolbar(self):
        """Returns toolbar with zooming tools"""
        return self.toolbars.get("map", None)

    def GetDialog(self, name):
        """Get selected dialog if exist"""
        return self.dialogs.get(name, None)

    def OnVNet(self, event):
        """Dialog for v.net* modules"""
        if self.dialogs["vnet"]:
            self.dialogs["vnet"].Raise()
            return

        from vnet.dialogs import VNETDialog

        self.dialogs["vnet"] = VNETDialog(parent=self, giface=self._giface)
        self.closingVNETDialog.connect(self.dialogs["vnet"].OnCloseDialog)
        self.dialogs["vnet"].CenterOnScreen()
        self.dialogs["vnet"].Show()

    def ResetPointer(self):
        """Sets pointer mode.

        Sets pointer and toggles it (e.g. after unregistration of mouse
        handler).
        """
        self.GetMapToolbar().SelectDefault()

    def _switchMapWindow(self, map_win):
        """Notifies activated and disactivated map_wins."""
        self.MapWindow.DisactivateWin()
        map_win.ActivateWin()

        self.MapWindow = map_win

    def AddRDigit(self):
        """Adds raster digitizer: creates toolbar and digitizer controller,
        binds events and signals."""
        from rdigit.controller import EVT_UPDATE_PROGRESS, RDigitController
        from rdigit.toolbars import RDigitToolbar

        self.rdigit = RDigitController(self._giface, mapWindow=self.GetMapWindow())
        self.toolbars["rdigit"] = RDigitToolbar(
            parent=self,
            giface=self._giface,
            controller=self.rdigit,
            toolSwitcher=self._toolSwitcher,
        )
        # connect signals
        self.rdigit.newRasterCreated.connect(self.toolbars["rdigit"].NewRasterAdded)
        self.rdigit.newRasterCreated.connect(
            lambda name: self._giface.mapCreated.emit(name=name, ltype="raster")
        )
        self.rdigit.newFeatureCreated.connect(self.toolbars["rdigit"].UpdateCellValues)
        self.rdigit.uploadMapCategories.connect(
            self.toolbars["rdigit"].UpdateCellValues
        )
        self.rdigit.showNotification.connect(lambda text: self.SetStatusText(text, 0))
        self.rdigit.quitDigitizer.connect(self.QuitRDigit)
        self.rdigit.Bind(
            EVT_UPDATE_PROGRESS,
            lambda evt: self.statusbarManager.SetProgress(
                evt.range, evt.value, evt.text
            ),
        )
        rasters = self.GetMap().GetListOfLayers(
            ltype="raster", mapset=gs.gisenv()["MAPSET"]
        )
        self.toolbars["rdigit"].UpdateRasterLayers(rasters)
        self.toolbars["rdigit"].SelectDefault()

        self.GetMap().layerAdded.connect(self._updateRDigitLayers)
        self.GetMap().layerRemoved.connect(self._updateRDigitLayers)
        self.GetMap().layerChanged.connect(self._updateRDigitLayers)
        self._mgr.AddPane(
            self.toolbars["rdigit"],
            wx.aui.AuiPaneInfo()
            .Name("rdigit toolbar")
            .Caption(_("Raster Digitizer Toolbar"))
            .ToolbarPane()
            .Top()
            .Row(1)
            .LeftDockable(False)
            .RightDockable(False)
            .BottomDockable(False)
            .TopDockable(True)
            .Floatable()
            .CloseButton(False)
            .Layer(2)
            .DestroyOnClose()
            .BestSize(self.toolbars["rdigit"].GetBestSize()),
        )
        self._mgr.Update()

        self.rdigit.Start()

    def _updateRDigitLayers(self, layer):
        mapset = gs.gisenv()["MAPSET"]
        self.toolbars["rdigit"].UpdateRasterLayers(
            rasters=self.GetMap().GetListOfLayers(ltype="raster", mapset=mapset)
        )

    def QuitRDigit(self):
        """Calls digitizer cleanup, removes digitizer object and disconnects
        signals from Map."""
        self.rdigit.CleanUp()
        # disconnect updating layers
        self.GetMap().layerAdded.disconnect(self._updateRDigitLayers)
        self.GetMap().layerRemoved.disconnect(self._updateRDigitLayers)
        self.GetMap().layerChanged.disconnect(self._updateRDigitLayers)
        self._toolSwitcher.toggleToolChanged.disconnect(
            self.toolbars["rdigit"].CheckSelectedTool,
        )

        self.RemoveToolbar("rdigit", destroy=True)
        self.rdigit = None

    def QuitVDigit(self):
        """Quit VDigit"""
        # disable the toolbar
        self.RemoveToolbar("vdigit", destroy=True)


class MapDisplay(FrameMixin, MapPanel):
    """Map display for wrapping map panel with frame methods"""

    def __init__(self, parent, giface, id, tree, lmgr, idx, Map, title, **kwargs):
        # init map panel
        MapPanel.__init__(
            self,
            parent=parent,
            giface=giface,
            id=id,
            tree=tree,
            lmgr=lmgr,
            Map=Map,
            title=title,
            **kwargs,
        )
        # set system icon
        parent.SetIcon(
            wx.Icon(
                os.path.join(globalvar.ICONDIR, "grass_map.ico"), wx.BITMAP_TYPE_ICO
            )
        )
        # use default frame window layout
        client_disp = wx.ClientDisplayRect()
        if UserSettings.Get(group="general", key="defWindowPos", subkey="enabled"):
            dim = UserSettings.Get(group="general", key="defWindowPos", subkey="dim")
            idx = 4 + idx * 4
            try:
                x, y = map(int, dim.split(",")[idx : idx + 2])
                w, h = map(int, dim.split(",")[idx + 2 : idx + 4])
                if x == 1:
                    # Get client display x offset (OS panel)
                    x = client_disp[0]
                if y == 1:
                    # Get client display y offset (OS panel)
                    y = client_disp[1]
                parent.SetPosition((x, y))
                parent.SetSize((w, h))
            except Exception:
                pass
        else:
            # Set client display x, y offset (OS panel)
            parent.SetPosition((client_disp[0], client_disp[1]))

        # bindings
        parent.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        # extend shortcuts and create frame accelerator table
        self.shortcuts_table.append((self.OnFullScreen, wx.ACCEL_NORMAL, wx.WXK_F11))
        self._initShortcuts()

        # add Map Display panel to Map Display frame
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(self, proportion=1, flag=wx.EXPAND)
        parent.SetSizer(sizer)
        parent.Layout()
