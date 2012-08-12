
import sys
import  wx
import time

import grass.script as grass

from gui_core.mapdisp   import DoubleMapFrame
from core.render        import Map
from mapdisp            import statusbar as sb
from core.debug         import Debug
from mapdisp.statusbar  import EVT_AUTO_RENDER

from swipe.toolbars  import SwipeMapToolbar, SwipeMainToolbar, SwipeMiscToolbar
from swipe.mapwindow import SwipeBufferedWindow
from swipe.dialogs   import SwipeMapDialog


class SwipeMapFrame(DoubleMapFrame):
    def __init__(self, parent  = None, title = _("Map Swipe"), name = "swipe", **kwargs):
        DoubleMapFrame.__init__(self, parent = parent, title = title, name = name,
                                firstMap = Map(), secondMap = Map(), **kwargs)
        Debug.msg (1, "SwipeMapFrame.__init__()")
        #
        # Add toolbars
        #
        toolbars = ['swipeMisc', 'swipeMap', 'swipeMain']
        if sys.platform == 'win32':
            self.AddToolbar(toolbars.pop(1))
            toolbars.reverse()
        else:
            self.AddToolbar(toolbars.pop(0))
        for toolb in toolbars:
            self.AddToolbar(toolb)

        #
        # create widgets
        #
        self.splitter = MapSplitter(parent = self, id = wx.ID_ANY)

        self.sliderH = wx.Slider(self, id = wx.ID_ANY, style = wx.SL_HORIZONTAL)
        self.sliderV = wx.Slider(self, id = wx.ID_ANY, style = wx.SL_VERTICAL)
        
        self.firstMapWindow = SwipeBufferedWindow(parent = self.splitter, Map = self.firstMap, frame = self)
        self.secondMapWindow = SwipeBufferedWindow(parent = self.splitter, Map = self.secondMap, frame = self)
        self.MapWindow = self.firstMapWindow # current by default
        self.firstMap.region = self.secondMap.region
        self.firstMapWindow.zoomhistory = self.secondMapWindow.zoomhistory

        self.splitter.SplitVertically(self.firstMapWindow, self.secondMapWindow, 0)

        self._addPanes()
        self._bindWindowsActivation()
    
        self._mgr.GetPane('sliderV').Hide()
        self._mgr.GetPane('sliderH').Show()
        self.slider = self.sliderH

        self.InitStatusbar()

        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(EVT_AUTO_RENDER, self.OnAutoRenderChanged)
        self.Bind(wx.EVT_IDLE, self.OnIdle)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        self.SetSize((800, 600))
        
        self._mgr.Update()

        self.rasters = {'first': None, 'second': None}

        # default action in map toolbar
        self.OnPan(event = None)

        self.resize = False

        wx.CallAfter(self.CallAfterInit)

    def CallAfterInit(self):
        self.InitSliderBindings()
        if not (self.rasters['first'] and self.rasters['second']):
            self.OnSelectRasters(event = None)
        
    def InitStatusbar(self):
        """!Init statusbar (default items)."""
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
        
        # create statusbar and its manager
        statusbar = self.CreateStatusBar(number = 4, style = 0)
        statusbar.SetStatusWidths([-5, -2, -1, -1])
        self.statusbarManager = sb.SbManager(mapframe = self, statusbar = statusbar)
        
        # fill statusbar manager
        self.statusbarManager.AddStatusbarItemsByClass(self.statusbarItems, mapframe = self, statusbar = statusbar)
        self.statusbarManager.AddStatusbarItem(sb.SbMask(self, statusbar = statusbar, position = 2))
        self.statusbarManager.AddStatusbarItem(sb.SbRender(self, statusbar = statusbar, position = 3))
        
        self.statusbarManager.Update()

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
        """!Slider changes its position, sash must be moved too."""
        Debug.msg (5, "SwipeMapFrame.OnSliderPositionChanging()")

        self.GetFirstWindow().movingSash = True
        self.GetSecondWindow().movingSash = True
        pos = event.GetPosition()
        if pos > 0:
            self.splitter.SetSashPosition(pos)
            self.splitter.OnSashChanging(None)

    def OnSliderPositionChanged(self, event):
        """!Slider position changed, sash must be moved too."""
        Debug.msg (5, "SwipeMapFrame.OnSliderPositionChanged()")

        self.splitter.SetSashPosition(event.GetPosition())
        self.splitter.OnSashChanged(None)

    def OnSashChanging(self, event):
        """!Sash position is changing, slider must be moved too."""
        Debug.msg (5, "SwipeMapFrame.OnSashChanging()")

        self.slider.SetValue(self.splitter.GetSashPosition())
        event.Skip()

    def OnSashChanged(self, event):
        """!Sash position changed, slider must be moved too."""
        Debug.msg (5, "SwipeMapFrame.OnSashChanged()")

        self.OnSashChanging(event)
        event.Skip()

    def OnSize(self, event):
        Debug.msg (4, "SwipeMapFrame.OnSize()")
        self.resize = time.clock()

    def OnIdle(self, event):
        if self.resize and time.clock() - self.resize > 0.2:
            w1 = self.GetFirstWindow()
            w2 = self.GetSecondWindow()

            sizeAll = self.splitter.GetSize()
            w1.SetClientSize(sizeAll)
            w2.SetClientSize(sizeAll)
            
            w1.OnSize(event)
            w2.OnSize(event)
            self.ResetSlider()
            self.resize = False

    def OnAutoRenderChanged(self, event):
        """!Auto rendering state changed."""
        style = self.splitter.GetWindowStyle()
        style ^= wx.SP_LIVE_UPDATE
        self.splitter.SetWindowStyle(style)

    def AddToolbar(self, name):
        """!Add defined toolbar to the window
        
        Currently known toolbars are:
         - 'swipeMap'          - basic map toolbar
         - 'swipeMain'         - swipe functionality
        """
        if name == "swipeMap":
            self.toolbars[name] = SwipeMapToolbar(self)
            self._mgr.AddPane(self.toolbars[name],
                      wx.aui.AuiPaneInfo().
                      Name(name).Caption(_("Map Toolbar")).
                      ToolbarPane().Top().
                      LeftDockable(False).RightDockable(False).
                      BottomDockable(False).TopDockable(True).
                      CloseButton(False).Layer(2).Row(1).
                      BestSize((self.toolbars[name].GetBestSize())))

        if name == "swipeMain":
            self.toolbars[name] = SwipeMainToolbar(self)

            self._mgr.AddPane(self.toolbars[name],
                      wx.aui.AuiPaneInfo().
                      Name(name).Caption(_("Main Toolbar")).
                      ToolbarPane().Top().
                      LeftDockable(False).RightDockable(False).
                      BottomDockable(False).TopDockable(True).
                      CloseButton(False).Layer(2).Row(1).
                      BestSize((self.toolbars[name].GetBestSize())))

        if name == "swipeMisc":
            self.toolbars[name] = SwipeMiscToolbar(self)

            self._mgr.AddPane(self.toolbars[name],
                      wx.aui.AuiPaneInfo().
                      Name(name).Caption(_("Misc Toolbar")).
                      ToolbarPane().Top().
                      LeftDockable(False).RightDockable(False).
                      BottomDockable(False).TopDockable(True).
                      CloseButton(False).Layer(2).Row(1).
                      BestSize((self.toolbars[name].GetBestSize())))

    def _addPanes(self):
        """!Add splitter window and sliders to aui manager"""
        # splitter window
        self._mgr.AddPane(self.splitter, wx.aui.AuiPaneInfo().
                  Name('splitter').CaptionVisible(False).PaneBorder(True).
                  Dockable(False).Floatable(False).CloseButton(False).
                  Center().Layer(1).BestSize((self.splitter.GetBestSize())))

        # sliders
        self._mgr.AddPane(self.sliderH, wx.aui.AuiPaneInfo().
                  Name('sliderH').CaptionVisible(False).PaneBorder(False).
                  CloseButton(False).Gripper(True).GripperTop(False).
                  BottomDockable(True).TopDockable(True).
                  LeftDockable(False).RightDockable(False).
                  Bottom().Layer(1).BestSize((self.sliderH.GetBestSize())))

        self._mgr.AddPane(self.sliderV, wx.aui.AuiPaneInfo().
                  Name('sliderV').CaptionVisible(False).PaneBorder(False).
                  CloseButton(False).Gripper(True).GripperTop(True).
                  BottomDockable(False).TopDockable(False).
                  LeftDockable(True).RightDockable(True).
                  Right().Layer(1).BestSize((self.sliderV.GetBestSize())))

    def UpdateRegion(self):
        """!
        Rerender the second window
        when the region of the first changed.
        """
        Debug.msg(3, "SwipeMapFrame.UpdateRegion()")

        if self.GetWindow() == self.GetSecondWindow():
            self.Render(self.GetFirstWindow())
        else:
            self.Render(self.GetSecondWindow())

    def OnZoomToMap(self, event):
        """!
        Set display extents to match selected raster (including NULLs)
        or vector map.
        """
        self.GetFirstWindow().ZoomToMap(layers = self.Map.GetListOfLayers())
        self.GetSecondWindow().ZoomToMap(layers = self.Map.GetListOfLayers())
        # needed again, don't know why
        self.firstMap.region = self.secondMap.region

    def OnZoomBack(self, event):
        self.GetFirstWindow().ZoomBack()
        self.secondMap.region = self.firstMap.region
        self.Render(self.GetSecondWindow())


    def OnSelectRasters(self, event):
        """!Choose raster maps and rerender."""
        dlg = SwipeMapDialog(self, first = self.rasters['first'], second = self.rasters['second'])
        if dlg.ShowModal() == wx.ID_OK:
            maps = dlg.GetValues()
            self.SetFirstRaster(name = maps[0])
            self.SetSecondRaster(name = maps[1])

        dlg.Destroy()
        self.OnRender(event = None)

    def SetFirstRaster(self, name):
        """!Set raster map to first Map"""
        raster = grass.find_file(name = name, element = 'cell')
        if raster['fullname']:
            self.rasters['first'] = raster['fullname']
            self.SetLayer(name = raster['fullname'], mapInstance = self.GetFirstMap())
            self.OnZoomToMap(event = None)

    def SetSecondRaster(self, name):
        """!Set raster map to second Map"""
        raster = grass.find_file(name = name, element = 'cell')
        if raster['fullname']:
            self.rasters['second'] = raster['fullname']
            self.SetLayer(name = raster['fullname'], mapInstance = self.GetSecondMap())
            self.OnZoomToMap(event = None)

    def SetLayer(self, name, mapInstance):
        """!Sets layer in Map.
        
        @param name layer (raster) name
        """
        Debug.msg (3, "SwipeMapFrame.SetLayer(): name=%s" % name)
        
        # this simple application enables to keep only one raster
        mapInstance.DeleteAllLayers()
        cmdlist = ['d.rast', 'map=%s' % name]
        # add layer to Map instance (core.render)
        newLayer = mapInstance.AddLayer(type = 'raster', command = cmdlist, l_active = True,
                                          name = name, l_hidden = False, l_opacity = 1.0,
                                          l_render = True)

    def OnSwitchWindows(self, event):
        """!Switch windows position."""
        Debug.msg(3, "SwipeMapFrame.OnSwitchWindows()")

        splitter = self.splitter
        w1, w2 = splitter.GetWindow1(), splitter.GetWindow2()
        splitter.ReplaceWindow(w1, w2)
        splitter.ReplaceWindow(w2, w1)
        # self.OnSize(None)
        splitter.OnSashChanged(None)

    def OnSwitchOrientation(self, event):
        """!Switch orientation of the sash."""
        Debug.msg(3, "SwipeMapFrame.OnSwitchOrientation()")

        splitter = self.splitter
        splitter.Unsplit()
        if splitter.GetSplitMode() == wx.SPLIT_HORIZONTAL:
            splitter.SplitVertically(self.firstMapWindow, self.secondMapWindow, 0)
            self.slider = self.sliderH
            self._mgr.GetPane('sliderH').Show()
            self._mgr.GetPane('sliderV').Hide()
        else:
            splitter.SplitHorizontally(self.firstMapWindow, self.secondMapWindow, 0)
            self.slider = self.sliderV
            self._mgr.GetPane('sliderV').Show()
            self._mgr.GetPane('sliderH').Hide()
        self._mgr.Update()
        splitter.OnSashChanged(None)
        self.OnSize(None)

    def GetMapToolbar(self):
        """!Returns toolbar with zooming tools"""
        return self.toolbars['swipeMap']

    def IsStandalone(self):
        if self.parent:
            return False

        return True

    def OnCloseWindow(self, event):
        self.Destroy()


class MapSplitter(wx.SplitterWindow):
    """!Splitter window for displaying two maps"""
    def __init__(self, parent, id):
        wx.SplitterWindow.__init__(self, parent = parent, id = id,
                                   style = wx.SP_LIVE_UPDATE
                                   )
        Debug.msg(2, "MapSplitter.__init__()")
        
        self.sashWidthMin = 1
        self.sashWidthMax = 10
        self.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGED, self.OnSashChanged)
        self.Bind(wx.EVT_SPLITTER_SASH_POS_CHANGING, self.OnSashChanging)
        
        wx.CallAfter(self.Init)

    def Init(self):
        self.OnSashChanged(evt = None)
        self.SetMinimumPaneSize(0)
        self.SetSashSize(self.sashWidthMin)

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

        w1.UpdateMap(render = False, renderVector = False)
        w2.UpdateMap(render = False, renderVector = False)

        pos = self.GetSashPosition()
        self.last = pos

    def OnSashChanging(self, event):
        Debug.msg(5, "MapSplitter.OnSashChanging()")

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
