"""
@package mapswipe.frame

@brief Map Swipe Frame

Classes:
 - frame::SwipeMapPanel
 - frame::SwipeMapDisplay
 - frame::MapSplitter

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import wx

import grass.script as gs

from gui_core.mapdisp import DoubleMapPanel, FrameMixin
from gui_core.dialogs import GetImageHandlers
from gui_core.wrap import Slider
from core.render import Map
from mapdisp import statusbar as sb
from core.debug import Debug
from core.gcmd import GError, GMessage
from core.layerlist import LayerListToRendererConverter
from core import globalvar
from gui_core.query import QueryDialog, PrepareQueryResults

from mapswipe.toolbars import SwipeMapToolbar, SwipeMainToolbar, SwipeMiscToolbar
from mapswipe.mapwindow import SwipeBufferedWindow
from mapswipe.dialogs import SwipeMapDialog, PreferencesDialog


class SwipeMapPanel(DoubleMapPanel):
    def __init__(
        self, parent=None, giface=None, title=_("Map Swipe"), name="swipe", **kwargs
    ):
        DoubleMapPanel.__init__(
            self,
            parent=parent,
            title=title,
            name=name,
            firstMap=Map(),
            secondMap=Map(),
            **kwargs,
        )
        Debug.msg(1, "SwipeMapPanel.__init__()")
        #
        # Add toolbars
        #
        for name in ("swipeMain", "swipeMap", "swipeMisc"):
            self.AddToolbar(name)
        self._mgr.Update()

        self._giface = giface
        #
        # create widgets
        #
        self.splitter = MapSplitter(parent=self, id=wx.ID_ANY)

        self.sliderH = Slider(self, id=wx.ID_ANY, style=wx.SL_HORIZONTAL)
        self.sliderV = Slider(self, id=wx.ID_ANY, style=wx.SL_VERTICAL)

        self.mapWindowProperties.autoRenderChanged.connect(self.OnAutoRenderChanged)
        self.firstMapWindow = SwipeBufferedWindow(
            parent=self.splitter,
            giface=self._giface,
            properties=self.mapWindowProperties,
            Map=self.firstMap,
        )
        self.secondMapWindow = SwipeBufferedWindow(
            parent=self.splitter,
            giface=self._giface,
            properties=self.mapWindowProperties,
            Map=self.secondMap,
        )
        # bind query signal
        self.firstMapWindow.mapQueried.connect(self.Query)
        self.secondMapWindow.mapQueried.connect(self.Query)

        # bind tracking cursosr to mirror it
        self.firstMapWindow.Bind(wx.EVT_MOTION, lambda evt: self.TrackCursor(evt))
        self.secondMapWindow.Bind(wx.EVT_MOTION, lambda evt: self.TrackCursor(evt))

        self.MapWindow = self.firstMapWindow  # current by default
        self.firstMapWindow.zoomhistory = self.secondMapWindow.zoomhistory
        self.SetBindRegions(True)

        self._mode = "swipe"

        # statusbar items
        statusbarItems = [
            sb.SbCoordinates,
            sb.SbRegionExtent,
            sb.SbCompRegionExtent,
            sb.SbDisplayGeometry,
            sb.SbMapScale,
            sb.SbGoTo,
        ]
        self.statusbar = self.CreateStatusbar(statusbarItems)

        self._addPanes()
        self._bindWindowsActivation()
        self._setUpMapWindow(self.firstMapWindow)
        self._setUpMapWindow(self.secondMapWindow)

        self._mgr.GetPane("sliderV").Hide()
        self._mgr.GetPane("sliderH").Show()
        self.slider = self.sliderH

        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_IDLE, self.OnIdle)

        self.SetSize((800, 600))

        self._mgr.Update()

        self.rasters = {"first": None, "second": None}

        self._inputDialog = None
        self._preferencesDialog = None
        self._queryDialog = None

        # default action in map toolbar
        self.GetMapToolbar().SelectDefault()

        self.resize = False

        wx.CallAfter(self.CallAfterInit)

    def TrackCursor(self, event):
        """Track cursor in one window and show cross in the other.

        Only for mirror mode.
        """
        if self._mode == "swipe":
            event.Skip()
            return
        coords = event.GetPosition()
        if event.GetId() == self.secondMapWindow.GetId():
            self.firstMapWindow.DrawMouseCursor(coords=coords)
        else:
            self.secondMapWindow.DrawMouseCursor(coords=coords)

        event.Skip()

    def ActivateFirstMap(self, event=None):
        """Switch tracking direction"""
        super().ActivateFirstMap(event)

        self.firstMapWindow.ClearLines()
        self.firstMapWindow.Refresh()

    def ActivateSecondMap(self, event=None):
        """Switch tracking direction"""
        super().ActivateSecondMap(event)

        self.secondMapWindow.ClearLines()
        self.secondMapWindow.Refresh()

    def CallAfterInit(self):
        self.InitSliderBindings()
        self.splitter.SplitVertically(self.firstMapWindow, self.secondMapWindow, 0)
        self.splitter.Init()
        if not (self.rasters["first"] and self.rasters["second"]):
            self.OnSelectLayers(event=None)

    def ResetSlider(self):
        if self.splitter.GetSplitMode() == wx.SPLIT_VERTICAL:
            size = self.splitter.GetSize()[0]
        else:
            size = self.splitter.GetSize()[1]
        self.slider.SetRange(0, size)
        self.slider.SetValue(self.splitter.GetSashPosition())

    def InitSliderBindings(self):
        self.sliderH.Bind(wx.EVT_SPIN, self.OnSliderPositionChanging)
        self.sliderH.Bind(wx.EVT_SCROLL_THUMBRELEASE, self.OnSliderPositionChanged)
        self.sliderV.Bind(wx.EVT_SPIN, self.OnSliderPositionChanging)
        self.sliderV.Bind(wx.EVT_SCROLL_THUMBRELEASE, self.OnSliderPositionChanged)
        self.splitter.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGING, self.OnSashChanging)
        self.splitter.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGED, self.OnSashChanged)

    def OnSliderPositionChanging(self, event):
        """Slider changes its position, sash must be moved too."""
        Debug.msg(5, "SwipeMapPanel.OnSliderPositionChanging()")

        self.GetFirstWindow().movingSash = True
        self.GetSecondWindow().movingSash = True
        pos = event.GetPosition()

        if pos > 0:
            self.splitter.SetSashPosition(pos)
            self.splitter.OnSashChanging(None)

    def OnSliderPositionChanged(self, event):
        """Slider position changed, sash must be moved too."""
        Debug.msg(5, "SwipeMapPanel.OnSliderPositionChanged()")

        self.splitter.SetSashPosition(event.GetPosition())
        self.splitter.OnSashChanged(None)

    def OnSashChanging(self, event):
        """Sash position is changing, slider must be moved too."""
        Debug.msg(5, "SwipeMapPanel.OnSashChanging()")

        self.slider.SetValue(self.splitter.GetSashPosition())
        event.Skip()

    def OnSashChanged(self, event):
        """Sash position changed, slider must be moved too."""
        Debug.msg(5, "SwipeMapPanel.OnSashChanged()")

        self.OnSashChanging(event)
        event.Skip()

    def OnSize(self, event):
        Debug.msg(4, "SwipeMapPanel.OnSize()")
        self.resize = gs.clock()
        super().OnSize(event)

    def OnIdle(self, event):
        if self.resize and gs.clock() - self.resize > 0.2:
            w1 = self.GetFirstWindow()
            w2 = self.GetSecondWindow()

            sizeAll = self.splitter.GetSize()
            w1.SetClientSize(sizeAll)
            w2.SetClientSize(sizeAll)

            w1.OnSize(event)
            w2.OnSize(event)
            self.ResetSlider()
            self.resize = False

    def OnAutoRenderChanged(self, value):
        """Auto rendering state changed."""
        style = self.splitter.GetWindowStyle()
        style ^= wx.SP_LIVE_UPDATE
        self.splitter.SetWindowStyle(style)
        self._simpleLmgrChanged()

    def AddToolbar(self, name):
        """Add defined toolbar to the window

        Currently known toolbars are:
         - 'swipeMain'         - swipe functionality
         - 'swipeMap'          - basic map toolbar
         - 'swipeMisc'         - misc (settings, help)
        """

        if name == "swipeMain":
            if "swipeMain" not in self.toolbars:
                self.toolbars["swipeMain"] = SwipeMainToolbar(self)

            self._mgr.AddPane(
                self.toolbars["swipeMain"],
                wx.aui.AuiPaneInfo()
                .Name("swipeMain")
                .Caption(_("Main Toolbar"))
                .ToolbarPane()
                .Top()
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2)
                .Row(1)
                .Position(0)
                .BestSize(self.toolbars["swipeMain"].GetBestSize()),
            )

        if name == "swipeMap":
            if "swipeMap" not in self.toolbars:
                self.toolbars["swipeMap"] = SwipeMapToolbar(self, self._toolSwitcher)

            self._mgr.AddPane(
                self.toolbars["swipeMap"],
                wx.aui.AuiPaneInfo()
                .Name("swipeMap")
                .Caption(_("Map Toolbar"))
                .ToolbarPane()
                .Top()
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2)
                .Row(1)
                .Position(1)
                .BestSize(self.toolbars["swipeMap"].GetBestSize()),
            )

        if name == "swipeMisc":
            if "swipeMisc" not in self.toolbars:
                self.toolbars["swipeMisc"] = SwipeMiscToolbar(self)

            self._mgr.AddPane(
                self.toolbars["swipeMisc"],
                wx.aui.AuiPaneInfo()
                .Name("swipeMisc")
                .Caption(_("Misc Toolbar"))
                .ToolbarPane()
                .Top()
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2)
                .Row(1)
                .Position(2)
                .BestSize(self.toolbars["swipeMisc"].GetBestSize()),
            )

    def _addPanes(self):
        """Add splitter window, sliders and statusbar to aui manager"""
        # splitter window
        self._mgr.AddPane(
            self.splitter,
            wx.aui.AuiPaneInfo()
            .Name("splitter")
            .CaptionVisible(False)
            .PaneBorder(True)
            .Dockable(False)
            .Floatable(False)
            .CloseButton(False)
            .Center()
            .Layer(1)
            .BestSize(self.splitter.GetBestSize()),
        )

        # sliders
        self._mgr.AddPane(
            self.sliderH,
            wx.aui.AuiPaneInfo()
            .Name("sliderH")
            .CaptionVisible(False)
            .PaneBorder(False)
            .CloseButton(False)
            .Gripper(True)
            .GripperTop(False)
            .BottomDockable(True)
            .TopDockable(True)
            .LeftDockable(False)
            .RightDockable(False)
            .Bottom()
            .Layer(1)
            .BestSize(self.sliderH.GetBestSize()),
        )

        self._mgr.AddPane(
            self.sliderV,
            wx.aui.AuiPaneInfo()
            .Name("sliderV")
            .CaptionVisible(False)
            .PaneBorder(False)
            .CloseButton(False)
            .Gripper(True)
            .GripperTop(True)
            .BottomDockable(False)
            .TopDockable(False)
            .LeftDockable(True)
            .RightDockable(True)
            .Right()
            .Layer(1)
            .BestSize(self.sliderV.GetBestSize()),
        )

        # statusbar
        self.AddStatusbarPane()

    def ZoomToMap(self):
        """
        Set display extents to match selected raster (including NULLs)
        or vector map.
        """
        layers = []
        if self.rasters["first"]:
            layers += self.firstMap.GetListOfLayers()
        if self.rasters["second"]:
            layers += self.secondMap.GetListOfLayers()

        if layers:
            self.GetFirstWindow().ZoomToMap(layers=layers)
            self.GetSecondWindow().ZoomToMap(layers=layers)

    def OnZoomToMap(self, event):
        """Zoom to map"""
        self.ZoomToMap()

    def OnZoomBack(self, event):
        self.GetFirstWindow().ZoomBack()
        self.secondMap.region = self.firstMap.region
        self.Render(self.GetSecondWindow())

    def OnSelectLayers(self, event):
        if self._inputDialog is None:
            dlg = SwipeMapDialog(
                self,
                first=self.rasters["first"],
                second=self.rasters["second"],
                firstLayerList=None,
                secondLayerList=None,
            )
            dlg.applyChanges.connect(self.OnApplyInputChanges)
            # connect to converter object to convert to Map
            # store reference to converter is needed otherwise it would be
            # discarded
            self._firstConverter = self._connectSimpleLmgr(
                dlg.GetFirstSimpleLmgr(), self.GetFirstMap()
            )
            self._secondConverter = self._connectSimpleLmgr(
                dlg.GetSecondSimpleLmgr(), self.GetSecondMap()
            )
            self._inputDialog = dlg
            dlg.CentreOnParent()
            dlg.Show()
        elif self._inputDialog.IsShown():
            self._inputDialog.Raise()
            self._inputDialog.SetFocus()
        else:
            self._inputDialog.Show()

    def _connectSimpleLmgr(self, lmgr, renderer):
        converter = LayerListToRendererConverter(renderer)
        lmgr.opacityChanged.connect(converter.ChangeLayerOpacity)
        lmgr.cmdChanged.connect(converter.ChangeLayerCmd)
        lmgr.layerAdded.connect(converter.AddLayer)
        lmgr.layerRemoved.connect(converter.RemoveLayer)
        lmgr.layerActivated.connect(converter.ChangeLayerActive)
        lmgr.layerMovedUp.connect(converter.MoveLayerUp)
        lmgr.layerMovedDown.connect(converter.MoveLayerDown)
        lmgr.anyChange.connect(self._simpleLmgrChanged)
        return converter

    def _simpleLmgrChanged(self):
        self.OnRender(event=None)

    def OnApplyInputChanges(self):
        first, second = self._inputDialog.GetValues()
        if self._inputDialog.IsSimpleMode():
            self.rasters["first"], self.rasters["second"] = first, second
            res1 = self.SetFirstRaster(name=self.rasters["first"])
            res2 = self.SetSecondRaster(name=self.rasters["second"])
            if not (res1 and res2) and (first or second):
                message = ""
                if first and not res1:
                    message += _("Map <%s> not found. ") % self.rasters["first"]
                if second and not res2:
                    message += _("Map <%s> not found.") % self.rasters["second"]
                if message:
                    GError(parent=self, message=message)
                    return
            self.ZoomToMap()
        else:
            LayerListToRendererConverter(self.GetFirstMap()).ConvertAll(first)
            LayerListToRendererConverter(self.GetSecondMap()).ConvertAll(second)

        self.SetRasterNames()
        self.OnRender(event=None)

    def SetFirstRaster(self, name):
        """Set raster map to first Map"""
        if name:
            raster = gs.find_file(name=name, element="cell")
            if raster.get("fullname"):
                self.rasters["first"] = raster["fullname"]
                self.SetLayer(name=raster["fullname"], mapInstance=self.GetFirstMap())
                return True

        return False

    def SetSecondRaster(self, name):
        """Set raster map to second Map"""
        if name:
            raster = gs.find_file(name=name, element="cell")
            if raster.get("fullname"):
                self.rasters["second"] = raster["fullname"]
                self.SetLayer(name=raster["fullname"], mapInstance=self.GetSecondMap())
                return True

        return False

    def SetLayer(self, name, mapInstance):
        """Sets layer in Map.

        :param name: layer (raster) name
        """
        Debug.msg(3, "SwipeMapPanel.SetLayer(): name=%s" % name)

        # this simple application enables to keep only one raster
        mapInstance.DeleteAllLayers()
        cmdlist = ["d.rast", "map=%s" % name]
        # add layer to Map instance (core.render)
        mapInstance.AddLayer(
            ltype="raster",
            command=cmdlist,
            active=True,
            name=name,
            hidden=False,
            opacity=1.0,
            render=True,
        )

    def OnSwitchWindows(self, event):
        """Switch windows position."""
        Debug.msg(3, "SwipeMapPanel.OnSwitchWindows()")

        splitter = self.splitter
        w1, w2 = splitter.GetWindow1(), splitter.GetWindow2()
        splitter.ReplaceWindow(w1, w2)
        splitter.ReplaceWindow(w2, w1)
        # self.OnSize(None)
        splitter.OnSashChanged(None)

    def _saveToFile(self, fileName, fileType):
        """Creates composite image by rendering both images and
        pasting them into the new one.

        .. todo::
            specify size of the new image (problem is inaccurate scaling)
        .. todo::
            make dividing line width and color optional
        """
        w1 = self.splitter.GetWindow1()
        w2 = self.splitter.GetWindow2()
        lineWidth = 1
        # render to temporary files
        filename1 = gs.tempfile(False) + "1"
        filename2 = gs.tempfile(False) + "2"
        width, height = self.splitter.GetClientSize()

        class _onDone:
            """Callback class that remembers how many times
            it was called. Needs to be called twice because
            we are pasting together 2 rendered images, so
            we need to know when both are finished."""

            def __init__(self2):  # noqa: N805
                self2.called = 0

            def __call__(self2):  # noqa: N805
                self2.called += 1
                if self2.called == 2:
                    self2.process()

            def process(self2):  # noqa: N805
                # create empty white image  - needed for line
                im = wx.Image(width, height)
                im.Replace(0, 0, 0, 255, 255, 255)

                # paste images
                if self._mode == "swipe":
                    if self.splitter.GetSplitMode() == wx.SPLIT_HORIZONTAL:
                        im1 = wx.Image(filename1).GetSubImage((0, 0, width, -y))
                        im.Paste(im1, 0, 0)
                        im.Paste(wx.Image(filename2), -x, -y + lineWidth)
                    else:
                        im1 = wx.Image(filename1).GetSubImage((0, 0, -x, height))
                        im.Paste(im1, 0, 0)
                        im.Paste(wx.Image(filename2), -x + lineWidth, -y)
                else:  # noqa: PLR5501
                    if self.splitter.GetSplitMode() == wx.SPLIT_HORIZONTAL:
                        im1 = wx.Image(filename1)
                        im.Paste(im1, 0, 0)
                        im.Paste(wx.Image(filename2), 0, fh + lineWidth)
                    else:
                        im1 = wx.Image(filename1)
                        im.Paste(im1, 0, 0)
                        im.Paste(wx.Image(filename2), fw + lineWidth, 0)
                im.SaveFile(fileName, fileType)

                # remove temporary files
                gs.try_remove(filename1)
                gs.try_remove(filename2)

        callback = _onDone()
        if self._mode == "swipe":
            x, y = w2.GetImageCoords()
            w1.SaveToFile(filename1, fileType, width, height, callback=callback)
            w2.SaveToFile(filename2, fileType, width, height, callback=callback)
        else:
            fw, fh = w1.GetClientSize()
            w1.SaveToFile(filename1, fileType, fw, fh, callback=callback)
            sw, sh = w2.GetClientSize()
            w2.SaveToFile(filename2, fileType, sw, sh, callback=callback)

    def SaveToFile(self, event):
        """Save map to image"""
        img = self.firstMapWindow.img or self.secondMapWindow.img
        if not img:
            GMessage(
                parent=self,
                message=_("Nothing to render (empty map). Operation canceled."),
            )
            return
        filetype, ltype = GetImageHandlers(img)

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

            self._saveToFile(path, fileType)

        dlg.Destroy()

    def OnSwitchOrientation(self, event):
        """Switch orientation of the sash."""
        Debug.msg(3, "SwipeMapPanel.OnSwitchOrientation()")

        splitter = self.splitter
        splitter.Unsplit()
        if splitter.GetSplitMode() == wx.SPLIT_HORIZONTAL:
            splitter.SplitVertically(self.firstMapWindow, self.secondMapWindow, 0)
            self.slider = self.sliderH
            if self._mode == "swipe":
                self._mgr.GetPane("sliderH").Show()
                self._mgr.GetPane("sliderV").Hide()
        else:
            splitter.SplitHorizontally(self.firstMapWindow, self.secondMapWindow, 0)
            self.slider = self.sliderV
            if self._mode == "swipe":
                self._mgr.GetPane("sliderV").Show()
                self._mgr.GetPane("sliderH").Hide()
        self._mgr.Update()
        splitter.OnSashChanged(None)
        self.OnSize(None)
        self.SetRasterNames()

    def OnAddText(self, event):
        """Double click on text overlay

        So far not implemented.
        """

    def SetViewMode(self, mode):
        """Sets view mode.

        :param mode: view mode ('swipe', 'mirror')
        """
        if self._mode == mode:
            return
        self._mode = mode
        self.toolbars["swipeMain"].SetMode(mode)
        # set window mode
        self.GetFirstWindow().SetMode(mode)
        self.GetSecondWindow().SetMode(mode)
        # hide/show slider
        if self.splitter.GetSplitMode() == wx.SPLIT_HORIZONTAL:
            self._mgr.GetPane("sliderV").Show(mode == "swipe")
            size = self.splitter.GetSize()[1] // 2
        else:
            self._mgr.GetPane("sliderH").Show(mode == "swipe")
            size = self.splitter.GetSize()[0] // 2
        # set sash in the middle
        self.splitter.SetSashPosition(size)
        self.slider.SetValue(size)
        self._mgr.Update()
        # enable / disable sash
        self.splitter.EnableSash(mode == "swipe")
        # hack to make it work
        self.splitter.OnSashChanged(None)
        self.SendSizeEvent()

    def SetRasterNames(self):
        if not self._inputDialog or self._inputDialog.IsSimpleMode():
            if self.rasters["first"]:
                self.GetFirstWindow().SetRasterNameText(self.rasters["first"], 101)
            if self.rasters["second"]:
                self.GetSecondWindow().SetRasterNameText(self.rasters["second"], 102)
        else:
            self.GetFirstWindow().SetRasterNameText("", 101)
            self.GetSecondWindow().SetRasterNameText("", 102)

    def Query(self, x, y):
        """Query active layers from both mapwindows.

        :param x,y: coordinates
        """
        rasters = (
            [
                layer.GetName()
                for layer in self.GetFirstMap().GetListOfLayers(
                    ltype="raster", active=True
                )
            ],
            [
                layer.GetName()
                for layer in self.GetSecondMap().GetListOfLayers(
                    ltype="raster", active=True
                )
            ],
        )
        vectors = (
            [
                layer.GetName()
                for layer in self.GetFirstMap().GetListOfLayers(
                    ltype="vector", active=True
                )
            ],
            [
                layer.GetName()
                for layer in self.GetSecondMap().GetListOfLayers(
                    ltype="vector", active=True
                )
            ],
        )

        if not (rasters[0] + rasters[1] + vectors[0] + vectors[1]):
            GMessage(
                parent=self,
                message=_("No raster or vector map layer selected for querying."),
            )
            return

        # set query snap distance for v.what at map unit equivalent of 10
        # pixels
        qdist = 10.0 * (
            (self.GetFirstMap().region["e"] - self.GetFirstMap().region["w"])
            / self.GetFirstMap().width
        )

        east, north = self.GetFirstWindow().Pixel2Cell((x, y))

        result = []
        env = os.environ.copy()
        if rasters[0]:
            for raster in rasters[0]:
                env["GRASS_REGION"] = gs.region_env(raster=raster)
                result.extend(
                    gs.raster_what(
                        map=raster, coord=(east, north), localized=True, env=env
                    )
                )
        if vectors[0]:
            result.extend(
                gs.vector_what(map=vectors[0], coord=(east, north), distance=qdist)
            )
        if rasters[1]:
            for raster in rasters[1]:
                env["GRASS_REGION"] = gs.region_env(raster=raster)
                result.extend(
                    gs.raster_what(
                        map=raster, coord=(east, north), localized=True, env=env
                    )
                )
        if vectors[1]:
            result.extend(
                gs.vector_what(map=vectors[1], coord=(east, north), distance=qdist)
            )

        result = PrepareQueryResults(coordinates=(east, north), result=result)
        if self._queryDialog:
            self._queryDialog.Raise()
            self._queryDialog.SetData(result)
        else:
            self._queryDialog = QueryDialog(parent=self, data=result)
            self._queryDialog.Bind(wx.EVT_CLOSE, self._oncloseQueryDialog)
            self._queryDialog.redirectOutput.connect(
                lambda output: self._giface.WriteLog(output)
            )
            self._queryDialog.Show()

    def _oncloseQueryDialog(self, event):
        self._queryDialog = None
        event.Skip()

    def GetMapToolbar(self):
        """Returns toolbar with zooming tools"""
        return self.toolbars["swipeMap"]

    def OnHelp(self, event):
        self._giface.Help(entry="wxGUI.mapswipe")

    def OnPreferences(self, event):
        if not self._preferencesDialog:
            dlg = PreferencesDialog(parent=self, giface=self._giface)
            self._preferencesDialog = dlg
            self._preferencesDialog.CenterOnParent()

        self._preferencesDialog.Show()

    def OnCloseWindow(self, event):
        self.GetFirstMap().Clean()
        self.GetSecondMap().Clean()
        self._mgr.UnInit()
        if self._inputDialog:
            self._inputDialog.UnInit()
        self.Destroy()


class SwipeMapDisplay(FrameMixin, SwipeMapPanel):
    """Map display for wrapping map panel with frame methods"""

    def __init__(self, parent, giface, **kwargs):
        # init map panel
        SwipeMapPanel.__init__(
            self,
            parent=parent,
            giface=giface,
            **kwargs,
        )
        # set system icon
        parent.SetIcon(
            wx.Icon(
                os.path.join(globalvar.ICONDIR, "grass_map.ico"), wx.BITMAP_TYPE_ICO
            )
        )

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


class MapSplitter(wx.SplitterWindow):
    """Splitter window for displaying two maps"""

    def __init__(self, parent, id):
        wx.SplitterWindow.__init__(self, parent=parent, id=id, style=wx.SP_LIVE_UPDATE)
        Debug.msg(2, "MapSplitter.__init__()")

        self.sashWidthMin = 1
        self.sashWidthMax = 10
        self.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGED, self.OnSashChanged)
        self.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGING, self.OnSashChanging)
        self._moveSash = True

    def EnableSash(self, enable):
        self._moveSash = enable

    def Init(self):
        self.OnSashChanged(evt=None)
        self.SetMinimumPaneSize(0)

    # def OnMotion(self, event):
    #     w = self.GetSashSize()
    #     w1, w2 = self.GetWindow1(), self.GetWindow2()
    #     if self.SashHitTest(event.GetX(), event.GetY(), tolerance = 20):
    #         if w == self.sashWidthMin:
    #             self.SetSashSize(self.sashWidthMax)
    #             self.SetNeedUpdating(True)
    #             w1.movingSash = True
    #             w2.movingSash = True
    #         else:
    #             w1.movingSash = False
    #             w1.movingSash = False
    #     else:
    #         if w == self.sashWidthMax:
    #             self.SetSashSize(self.sashWidthMin)
    #             self.SetNeedUpdating(True)
    #             w1.movingSash = True
    #             w2.movingSash = True
    #         else:
    #             w1.movingSash = False
    #             w2.movingSash = False

    #     event.Skip()

    def OnSashChanged(self, evt):
        Debug.msg(5, "MapSplitter.OnSashChanged()")
        if not self._moveSash:
            return
        w1, w2 = self.GetWindow1(), self.GetWindow2()
        w1.movingSash = False
        w2.movingSash = False

        wx.CallAfter(self.SashChanged)

    def SashChanged(self):
        Debug.msg(5, "MapSplitter.SashChanged()")

        w1, w2 = self.GetWindow1(), self.GetWindow2()
        w1.SetImageCoords((0, 0))
        if self.GetSplitMode() == wx.SPLIT_VERTICAL:
            w = w1.GetSize()[0]
            w2.SetImageCoords((-w, 0))
        else:
            h = w1.GetSize()[1]
            w2.SetImageCoords((0, -h))

        w1.UpdateMap(render=False, renderVector=False)
        w2.UpdateMap(render=False, renderVector=False)

        pos = self.GetSashPosition()
        self.last = pos

    def OnSashChanging(self, event):
        Debug.msg(5, "MapSplitter.OnSashChanging()")
        if event:
            # Prevent map image flickering if it is used sash not slider
            # for changing position
            wx.CallAfter(self._onSashChanging, event)
        else:
            self._onSashChanging(event)

    def _onSashChanging(self, event):
        if not self._moveSash:
            event.SetSashPosition(-1)
            return
        if not (self.GetWindowStyle() & wx.SP_LIVE_UPDATE):
            if event:
                event.Skip()
            return

        pos = self.GetSashPosition()
        dpos = pos - self.last
        self.last = pos
        if self.GetSplitMode() == wx.SPLIT_VERTICAL:
            dx = -dpos
            dy = 0
        else:
            dx = 0
            dy = -dpos
        self.GetWindow2().TranslateImage(dx, dy)

        self.GetWindow1().movingSash = True
        self.GetWindow2().movingSash = True
