# -*- coding: utf-8 -*-
"""
@package rlisetup.sampling_frame

@brief r.li.setup - draw sample frame

Classes:
 - sampling_frame::RLiSetupMapPanel
 - sampling_frame::RLiSetupToolbar
 - sampling_frame::GraphicsSetItem

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import os

import wx
import wx.aui


# start new import
import tempfile
from core.gcmd import RunCommand
import grass.script.core as grass
from core import gcmd

try:
    from grass.lib.gis import *
    from grass.lib.vector import *
    from grass.lib.raster import *
except ImportError:
    pass

from core.giface import StandaloneGrassInterface
from mapwin.base import MapWindowProperties
from mapwin.buffered import BufferedMapWindow
from core.render import Map
from gui_core.toolbars import BaseToolbar, BaseIcons, ToolSwitcher
from icons.icon import MetaIcon
from core.gcmd import GMessage
from grass.pydispatch.signal import Signal
from grass.pydispatch.errors import DispatcherKeyError

from .functions import SamplingType, checkMapExists


class Circle:

    def __init__(self, pt, r):
        self.point = pt
        self.radius = r


class MaskedArea(object):

    def __init__(self, region, raster, radius):
        self.region = region
        self.raster = raster
        self.radius = radius


class RLiSetupMapPanel(wx.Panel):
    """Panel with mapwindow used in r.li.setup"""

    def __init__(self, parent, samplingType, icon=None, map_=None):
        wx.Panel.__init__(self, parent=parent)

        self.mapWindowProperties = MapWindowProperties()
        self.mapWindowProperties.setValuesFromUserSettings()
        giface = StandaloneGrassInterface()
        self.samplingtype = samplingType
        self.parent = parent

        if map_:
            self.map_ = map_
        else:
            self.map_ = Map()
        self.map_.region = self.map_.GetRegion()

        self._mgr = wx.aui.AuiManager(self)
        self.mapWindow = BufferedMapWindow(parent=self, giface=giface,
                                           Map=self.map_,
                                           properties=self.mapWindowProperties)
        self._mgr.AddPane(self.mapWindow, wx.aui.AuiPaneInfo().CentrePane().
                          Dockable(True).BestSize((-1, -1)).Name('mapwindow').
                          CloseButton(False).DestroyOnClose(True).
                          Layer(0))
        self._toolSwitcher = ToolSwitcher()
        self._toolSwitcher.toggleToolChanged.connect(self._onToolChanged)
        self.toolbar = RLiSetupToolbar(self, self._toolSwitcher)

        self.catId = 1

        self._mgr.AddPane(self.toolbar,
                          wx.aui.AuiPaneInfo().
                          Name("maptoolbar").Caption(_("Map Toolbar")).
                          ToolbarPane().Left().Name('mapToolbar').
                          CloseButton(False).Layer(1).Gripper(False).
                          BestSize((self.toolbar.GetBestSize())))
        self._mgr.Update()

        if self.samplingtype == SamplingType.REGIONS:
            self.afterRegionDrawn = Signal('RLiSetupMapPanel.afterRegionDrawn')
            self._registeredGraphics = self.mapWindow.RegisterGraphicsToDraw(
                graphicsType='line')
        elif self.samplingtype in [SamplingType.MUNITSR, SamplingType.MMVWINR]:
            self.sampleFrameChanged = Signal(
                'RLiSetupMapPanel.sampleFrameChanged')
            self._registeredGraphics = self.mapWindow.RegisterGraphicsToDraw(
                graphicsType='rectangle')
        elif self.samplingtype in [SamplingType.MUNITSC, SamplingType.MMVWINC]:
            self.afterCircleDrawn = Signal('RLiSetupMapPanel.afterCircleDrawn')
            self._registeredGraphics = self.mapWindow.RegisterGraphicsToDraw(
                graphicsType='line')
        else:
            self.sampleFrameChanged = Signal(
                'RLiSetupMapPanel.sampleFrameChanged')
            self._registeredGraphics = self.mapWindow.RegisterGraphicsToDraw(
                graphicsType='rectangle')

        self._registeredGraphics.AddPen('rlisetup', wx.Pen(wx.GREEN, width=2,
                                                           style=wx.SOLID))
        self._registeredGraphics.AddItem(coords=[[0, 0], [0, 0]],
                                         penName='rlisetup', hide=True)

        if self.samplingtype != SamplingType.VECT:
            self.toolbar.SelectDefault()

    def GetMap(self):
        return self.map_

    def OnPan(self, event):
        """Panning, set mouse to drag."""
        self.mapWindow.SetModePan()

    def OnZoomIn(self, event):
        """Zoom in the map."""
        self.mapWindow.SetModeZoomIn()

    def OnZoomOut(self, event):
        """Zoom out the map."""
        self.mapWindow.SetModeZoomOut()

    def OnZoomToMap(self, event):
        layers = self.map_.GetListOfLayers()
        self.mapWindow.ZoomToMap(layers=layers, ignoreNulls=False, render=True)

    def OnDrawRadius(self, event):
        """Start draw mode"""
        self.mapWindow.mouse['use'] = "None"
        self.mapWindow.mouse['box'] = "line"
        self.mapWindow.pen = wx.Pen(colour=wx.RED, width=1,
                                    style=wx.SHORT_DASH)
        self.mapWindow.SetNamedCursor('cross')
        self.mapWindow.mouseLeftUp.connect(self._radiusDrawn)

    def OnDigitizeRegion(self, event):
        """Start draw mode"""
        self.mapWindow.mouse['use'] = "None"
        self.mapWindow.mouse['box'] = "line"
        self.mapWindow.pen = wx.Pen(colour=wx.RED, width=1,
                                    style=wx.SHORT_DASH)
        self.mapWindow.SetNamedCursor('cross')
        self.mapWindow.mouseLeftUp.connect(self._lineSegmentDrawn)
        self.mapWindow.mouseDClick.connect(self._mouseDbClick)

        self._registeredGraphics.GetItem(0).SetCoords([])

    def OnDraw(self, event):
        """Start draw mode"""
        self.mapWindow.mouse['use'] = "None"
        self.mapWindow.mouse['box'] = "box"
        self.mapWindow.pen = wx.Pen(colour=wx.RED, width=2,
                                    style=wx.SHORT_DASH)
        self.mapWindow.SetNamedCursor('cross')
        self.mapWindow.mouseLeftUp.connect(self._rectangleDrawn)

    def _lineSegmentDrawn(self, x, y):
        item = self._registeredGraphics.GetItem(0)
        coords = item.GetCoords()
        if len(coords) == 0:
            coords.extend([self.mapWindow.Pixel2Cell(
                self.mapWindow.mouse['begin'])])
        coords.extend([[x, y]])

        item.SetCoords(coords)
        item.SetPropertyVal('hide', False)
        self.mapWindow.ClearLines()
        self._registeredGraphics.Draw()

    def _mouseDbClick(self, x, y):
        item = self._registeredGraphics.GetItem(0)
        coords = item.GetCoords()
        coords.extend([[x, y]])
        item.SetCoords(coords)
        item.SetPropertyVal('hide', False)
        self.mapWindow.ClearLines()
        self._registeredGraphics.Draw()
        self.createRegion()

    def createRegion(self):
        dlg = wx.TextEntryDialog(None, 'Name of sample region',
                                 'Create region', 'region' + str(self.catId))
        ret = dlg.ShowModal()
        while True:
            if ret == wx.ID_OK:
                raster = dlg.GetValue()
                if checkMapExists(raster):
                    GMessage(
                        parent=self, message=_(
                            "The raster file %s already"
                            " exists, please change name") %
                        raster)
                    ret = dlg.ShowModal()
                else:
                    dlg.Destroy()
                    marea = self.writeArea(
                        self._registeredGraphics.GetItem(0).GetCoords(), raster)
                    self.nextRegion(next=True, area=marea)
                    break
            else:
                self.nextRegion(next=False)
                break

    def nextRegion(self, next=True, area=None):
        self.mapWindow.ClearLines()
        item = self._registeredGraphics.GetItem(0)
        item.SetCoords([])
        item.SetPropertyVal('hide', True)

        layers = self.map_.GetListOfLayers()
        self.mapWindow.ZoomToMap(layers=layers, ignoreNulls=False, render=True)
        if next is True:
            self.afterRegionDrawn.emit(marea=area)
        else:
            gcmd.GMessage(parent=self.parent, message=_(
                "Raster map not created. Please redraw region."))

    def writeArea(self, coords, rasterName):
        polyfile = tempfile.NamedTemporaryFile(delete=False)
        polyfile.write("AREA\n")
        for coor in coords:
            east, north = coor
            point = " %s %s\n" % (east, north)
            polyfile.write(point)

        catbuf = "=%d a\n" % self.catId
        polyfile.write(catbuf)
        self.catId = self.catId + 1

        polyfile.close()
        region_settings = grass.parse_command('g.region', flags='p',
                                              delimiter=':')
        pname = polyfile.name.split('/')[-1]
        tmpraster = "rast_" + pname
        tmpvector = "vect_" + pname
        wx.BeginBusyCursor()
        wx.GetApp().Yield()
        RunCommand('r.in.poly', input=polyfile.name, output=tmpraster,
                   rows=region_settings['rows'], overwrite=True)

        RunCommand('r.to.vect', input=tmpraster, output=tmpvector,
                   type='area', overwrite=True)

        RunCommand('v.to.rast', input=tmpvector, output=rasterName,
                   value=1, use='val')
        wx.EndBusyCursor()
        grass.use_temp_region()
        grass.run_command('g.region', vector=tmpvector)
        region = grass.region()

        marea = MaskedArea(region, rasterName)

        RunCommand('g.remove', flags='f', type='raster', name=tmpraster)
        RunCommand('g.remove', flags='f', type='vector', name=tmpvector)

        os.unlink(polyfile.name)
        return marea

    def _onToolChanged(self):
        """Helper function to disconnect drawing"""
        try:
            self.mapWindow.mouseLeftUp.disconnect(self._rectangleDrawn)
            self.mapWindow.mouseLeftUp.disconnect(self._radiusDrawn)
            self.mapWindow.mouseMoving.disconnect(self._mouseMoving)
            self.mapWindow.mouseLeftDown.disconnect(self._mouseLeftDown)
            self.mapWindow.mouseDClick.disconnect(self._mouseDbClick)
        except DispatcherKeyError:
            pass

    def _radiusDrawn(self, x, y):
        """When drawing finished, get region values"""
        mouse = self.mapWindow.mouse
        item = self._registeredGraphics.GetItem(0)
        p1 = mouse['begin']
        p2 = mouse['end']
        dist, (north, east) = self.mapWindow.Distance(p1, p2, False)
        circle = Circle(p1, dist)
        self.mapWindow.ClearLines()
        self.mapWindow.pdcTmp.SetBrush(wx.Brush(wx.CYAN, wx.TRANSPARENT))
        pen = wx.Pen(colour=wx.RED, width=2)
        self.mapWindow.pdcTmp.SetPen(pen)
        self.mapWindow.pdcTmp.DrawCircle(circle.point[0], circle.point[1],
                                         circle.radius)
        self._registeredGraphics.Draw()
        self.createCricle(circle)

    def createCricle(self, c):
        dlg = wx.TextEntryDialog(None,
                                 'Name of sample circle region',
                                 'Create circle region',
                                 'circle' + str(self.catId))
        ret = dlg.ShowModal()
        while True:
            if ret == wx.ID_OK:
                raster = dlg.GetValue()
                if checkMapExists(raster):
                    GMessage(
                        parent=self, message=_(
                            "The raster file %s already"
                            " exists, please change name") %
                        raster)
                    ret = dlg.ShowModal()
                else:
                    dlg.Destroy()
                    circle = self.writeCircle(c, raster)
                    self.nextCircle(next=True, circle=circle)
                    break
            else:
                self.nextCircle(next=False)
                break

    def nextCircle(self, next=True, circle=None):
        self.mapWindow.ClearLines()
        item = self._registeredGraphics.GetItem(0)
        item.SetPropertyVal('hide', True)
        layers = self.map_.GetListOfLayers()
        self.mapWindow.ZoomToMap(layers=layers, ignoreNulls=False, render=True)
        if next is True:
            self.afterCircleDrawn.emit(region=circle)
        else:
            gcmd.GMessage(parent=self.parent, message=_(
                "Raster map not created. redraw region again."))

    def writeCircle(self, circle, rasterName):
        coords = self.mapWindow.Pixel2Cell(circle.point)
        RunCommand('r.circle', output=rasterName, max=circle.radius,
                   coordinate=coords, flags="b")
        grass.use_temp_region()
        grass.run_command('g.region', zoom=rasterName)
        region = grass.region()
        marea = MaskedArea(region, rasterName, circle.radius)
        return marea

    def _rectangleDrawn(self):
        """When drawing finished, get region values"""
        mouse = self.mapWindow.mouse
        item = self._registeredGraphics.GetItem(0)
        p1 = self.mapWindow.Pixel2Cell(mouse['begin'])
        p2 = self.mapWindow.Pixel2Cell(mouse['end'])
        item.SetCoords([p1, p2])
        region = {'n': max(p1[1], p2[1]),
                  's': min(p1[1], p2[1]),
                  'w': min(p1[0], p2[0]),
                  'e': max(p1[0], p2[0])}
        item.SetPropertyVal('hide', False)
        self.mapWindow.ClearLines()
        self._registeredGraphics.Draw()
        if self.samplingtype in [SamplingType.MUNITSR, SamplingType.MMVWINR]:
            dlg = wx.MessageDialog(self, "Is this area ok?",
                                   "select sampling unit",
                                   wx.YES_NO | wx.ICON_QUESTION)
            ret = dlg.ShowModal()
            if ret == wx.ID_YES:
                grass.use_temp_region()
                grass.run_command('g.region', n=region['n'], s=region['s'],
                                  e=region['e'], w=region['w'])
                tregion = grass.region()
                self.sampleFrameChanged.emit(region=tregion)
                self.mapWindow.ClearLines()
                item = self._registeredGraphics.GetItem(0)
                item.SetPropertyVal('hide', True)
                layers = self.map_.GetListOfLayers()
                self.mapWindow.ZoomToMap(layers=layers, ignoreNulls=False,
                                         render=True)
            else:
                self.nextRegion(next=False)
            dlg.Destroy()

        elif self.samplingtype != SamplingType.WHOLE:
            """When drawing finished, get region values"""
            self.sampleFrameChanged.emit(region=region)

icons = {
    'draw': MetaIcon(
        img='edit',
        label=_('Draw sampling frame'),
        desc=_('Draw sampling frame by clicking and dragging')),
    'digitizeunit': MetaIcon(
        img='edit',
        label=_('Draw sampling rectangle'),
        desc=_('Draw sampling rectangle by clicking and dragging')),
    'digitizeunitc': MetaIcon(
        img='line-create',
        label=_('Draw sampling circle'),
        desc=_('Draw sampling circle radius by clicking and dragging')),
    'digitizeregion': MetaIcon(
        img='polygon-create',
        label=_('Draw sampling region'),
        desc=_('Draw sampling region by polygon. Right Double click to end drawing'))}


class RLiSetupToolbar(BaseToolbar):
    """IClass toolbar
    """

    def __init__(self, parent, toolSwitcher):
        """RLiSetup toolbar constructor
        """

        BaseToolbar.__init__(self, parent, toolSwitcher,
                             style=wx.NO_BORDER | wx.TB_VERTICAL)

        self.InitToolbar(self._toolbarData())

        if self.parent.samplingtype == SamplingType.REGIONS:
            self._default = self.digitizeregion
        elif self.parent.samplingtype in [SamplingType.MUNITSR,
                                          SamplingType.MMVWINR]:
            self._default = self.digitizeunit
        elif self.parent.samplingtype in [SamplingType.MUNITSC,
                                          SamplingType.MMVWINC]:
            self._default = self.digitizeunitc
        elif self.parent.samplingtype == SamplingType.VECT:
            self._default = None
        else:
            self._default = self.draw

        for tool in (self._default, self.pan, self.zoomIn, self.zoomOut):
            if tool:
                self.toolSwitcher.AddToolToGroup(group='mouseUse',
                                                 toolbar=self, tool=tool)

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        if self.parent.samplingtype == SamplingType.REGIONS:
            drawTool = ('digitizeregion', icons['digitizeregion'],
                        self.parent.OnDigitizeRegion, wx.ITEM_CHECK)
        elif self.parent.samplingtype in [SamplingType.MUNITSR,
                                          SamplingType.MMVWINR]:
            drawTool = ('digitizeunit', icons['digitizeunit'],
                        self.parent.OnDraw, wx.ITEM_CHECK)
        elif self.parent.samplingtype in [SamplingType.MUNITSC,
                                          SamplingType.MMVWINC]:
            drawTool = ('digitizeunitc', icons['digitizeunitc'],
                        self.parent.OnDrawRadius, wx.ITEM_CHECK)
        else:
            drawTool = ('draw', icons['draw'], self.parent.OnDraw,
                        wx.ITEM_CHECK)
        if self.parent.samplingtype == SamplingType.VECT:
            return self._getToolbarData((
                ('pan', BaseIcons['pan'], self.parent.OnPan,
                 wx.ITEM_CHECK),
                ('zoomIn', BaseIcons['zoomIn'], self.parent.OnZoomIn,
                 wx.ITEM_CHECK),
                ('zoomOut', BaseIcons['zoomOut'], self.parent.OnZoomOut,
                 wx.ITEM_CHECK),
                ('zoomExtent', BaseIcons['zoomExtent'],
                 self.parent.OnZoomToMap),))
        else:
            return self._getToolbarData(
                (drawTool, (None,),
                 ('pan', BaseIcons['pan'],
                  self.parent.OnPan, wx.ITEM_CHECK),
                 ('zoomIn', BaseIcons['zoomIn'],
                  self.parent.OnZoomIn, wx.ITEM_CHECK),
                 ('zoomOut', BaseIcons['zoomOut'],
                  self.parent.OnZoomOut, wx.ITEM_CHECK),
                 ('zoomExtent', BaseIcons['zoomExtent'],
                  self.parent.OnZoomToMap),))
