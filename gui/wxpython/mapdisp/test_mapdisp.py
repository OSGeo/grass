#!/usr/bin/env python

############################################################################
#
# MODULE:    Map window and mapdisplay test module
# AUTHOR(S): Vaclav Petras
# PURPOSE:   Test functionality using small GUI applications.
# COPYRIGHT: (C) 2013 by Vaclav Petras, and the GRASS Development Team
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
############################################################################

#%module
#% description: Georectifies a map and allows to manage Ground Control Points.
#% keywords: general
#% keywords: GUI
#% keywords: georectification
#%end
#%option
#% key: test
#% description: Test to run
#% options: mapwindow,mapdisplay,apitest,distance,profile
#% descriptions: mapwindow;Opens map window ;mapdisplay;Opens map display; apitest;Open an application to test API of map window; distance;Starts map window with distance measurement activated; profile;Starts map window with profile tool activated
#% required: yes
#%end
#%option G_OPT_R_INPUT
#% key: raster
#% multiple: yes
#% required: no
#%end
#%option G_OPT_V_INPUT
#% key: vector
#% multiple: yes
#% required: no
#%end

"""
Module to run test map window (BufferedWidnow) and map display (MapFrame).

@author Vaclav Petras  <wenzeslaus gmail.com>
"""

import os
import sys
import wx

import grass.script as grass

# adding a path to wxGUI modules
if __name__ == '__main__':
    WXGUIBASE = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if WXGUIBASE not in sys.path:
        sys.path.append(WXGUIBASE)

from core.utils import _
from core.settings import UserSettings
from core.globalvar import CheckWxVersion
from core.giface import StandaloneGrassInterface
from mapwin.base import MapWindowProperties
from mapwin.buffered import BufferedMapWindow
from core.render import Map
from rlisetup.sampling_frame import RLiSetupMapPanel
from mapdisp.main import LayerList


class MapdispGrassInterface(StandaloneGrassInterface):
    """!@implements GrassInterface"""
    def __init__(self, map_):
        StandaloneGrassInterface.__init__(self)
        self._map = map_
        self.mapWindow = None

    def GetLayerList(self):
        return LayerList(self._map, giface=self)

    def GetMapWindow(self):
        return self.mapWindow


# this is a copy of method from some frame class
def copyOfInitMap(map_, width, height):
    """!Initialize map display, set dimensions and map region
    """
    if not grass.find_program('g.region', '--help'):
        sys.exit(_("GRASS module '%s' not found. Unable to start map "
                   "display window.") % 'g.region')
    map_.ChangeMapSize((width, height))
    map_.region = map_.GetRegion()  # g.region -upgc
    # self.Map.SetRegion() # adjust region to match display window


class TextShower(object):
    def __init__(self, parent, title):
        self._cf = wx.Frame(parent=parent, title=title)
        self._cp = wx.Panel(parent=self._cf, id=wx.ID_ANY)
        self._cs = wx.BoxSizer(wx.VERTICAL)
        self._cl = wx.StaticText(parent=self._cp, id=wx.ID_ANY, label="No text set yet")
        self._cs.Add(item=self._cl, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        self._cp.SetSizer(self._cs)
        self._cp.Layout()
        self._cf.Show()

    def SetLabel(self, text):
        self._cl.SetLabel(text)


class Tester(object):
    def _listenToAllMapWindowSignals(self, window):
        output = sys.stderr
        # will make bad thigs after it is closed but who cares
        coordinatesShower = TextShower(window, "Coordinates")

        window.zoomChanged.connect(lambda: output.write("zoomChanged\n"))
        window.zoomHistoryUnavailable.connect(lambda: output.write("zoomHistoryUnavailable\n"))
        window.zoomHistoryAvailable.connect(lambda: output.write("zoomHistoryAvailable\n"))

        window.mapQueried.connect(lambda: output.write("mapQueried\n"))
        window.mouseEntered.connect(lambda: output.write("mouseEntered\n"))
        window.mouseLeftUpPointer.connect(lambda: output.write("mouseLeftUpPointer\n"))
        window.mouseLeftUp.connect(lambda: output.write("mouseLeftUp\n"))
        window.mouseMoving.connect(lambda x, y: coordinatesShower.SetLabel("%s , %s" % (x, y)))
        window.mouseHandlerRegistered.connect(lambda: output.write("mouseHandlerRegistered\n"))
        window.mouseHandlerUnregistered.connect(lambda: output.write("mouseHandlerUnregistered\n"))

    def testMapWindow(self, giface, map_):
        self.frame = wx.Frame(parent=None, title=_("Map window test frame"))
        panel = wx.Panel(parent=self.frame, id=wx.ID_ANY)
        sizer = wx.BoxSizer(wx.VERTICAL)
        mapWindowProperties = MapWindowProperties()
        mapWindowProperties.setValuesFromUserSettings()
        width, height = self.frame.GetClientSize()
        copyOfInitMap(map_, width, height)
        window = BufferedMapWindow(parent=panel, giface=giface, Map=map_,
                                   properties=mapWindowProperties)
        sizer.Add(item=window, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        panel.SetSizer(sizer)
        panel.Layout()
        self.frame.Show()

    def testMapDisplay(self, giface, map_):
        from mapdisp.frame import MapFrame
        # known issues (should be similar with d.mon):
        # * opening map in digitizer ends with: vdigit/toolbars.py:723: 'selection' referenced before assignment
        # * nviz start fails (closes window? segfaults?) after mapdisp/frame.py:306: 'NoneType' object has no attribute 'GetLayerNotebook'
        frame = MapFrame(parent=None, title=_("Map display test"),
                         giface=giface, Map=map_)
        # this is questionable: how complete the giface when creating objects
        # which are in giface
        giface.mapWindow = frame.GetMapWindow()
        frame.GetMapWindow().ZoomToMap()
        frame.Show()

    def testMapWindowApi(self, giface, map_):
        self.frame = wx.Frame(parent=None, title=_("Map window API test frame"))
        panel = wx.Panel(parent=self.frame, id=wx.ID_ANY)
        sizer = wx.BoxSizer(wx.VERTICAL)

        mapWindowProperties = MapWindowProperties()
        mapWindowProperties.setValuesFromUserSettings()
        mapWindowProperties.showRegion = True

        width, height = self.frame.GetClientSize()
        copyOfInitMap(map_, width, height)
        window = BufferedMapWindow(parent=panel, giface=giface, Map=map_,
                                   properties=mapWindowProperties)

        giface.mapWindow = window

        sizer.Add(item=window, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        panel.SetSizer(sizer)
        panel.Layout()

        window.ZoomToWind()

        from mapdisp.frame import MeasureController
        self.measureController = MeasureController(giface)
        self.measureController.StartMeasurement()
        self._listenToAllMapWindowSignals(window)

        self.frame.Show()

    def testMapWindowDistance(self, giface, map_):
        self.frame = wx.Frame(parent=None,
                              title=_("Map window distance measurement test frame"))
        panel = wx.Panel(parent=self.frame, id=wx.ID_ANY)
        sizer = wx.BoxSizer(wx.VERTICAL)

        mapWindowProperties = MapWindowProperties()
        mapWindowProperties.setValuesFromUserSettings()
        mapWindowProperties.showRegion = True

        width, height = self.frame.GetClientSize()
        copyOfInitMap(map_, width, height)
        window = BufferedMapWindow(parent=panel, giface=giface, Map=map_,
                                   properties=mapWindowProperties)

        giface.mapWindow = window

        sizer.Add(item=window, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        panel.SetSizer(sizer)
        panel.Layout()

        window.ZoomToWind()

        self._listenToAllMapWindowSignals(window)

        self.frame.Show()

        from mapwin.analysis import MeasureDistanceController
        self.controller = MeasureDistanceController(giface, window)
        self.controller.Start()

    def testMapWindowProfile(self, giface, map_):
        self.frame = wx.Frame(parent=None,
                              title=_("Map window profile tool test frame"))
        panel = wx.Panel(parent=self.frame, id=wx.ID_ANY)
        sizer = wx.BoxSizer(wx.VERTICAL)

        mapWindowProperties = MapWindowProperties()
        mapWindowProperties.setValuesFromUserSettings()
        mapWindowProperties.showRegion = True

        width, height = self.frame.GetClientSize()
        copyOfInitMap(map_, width, height)
        window = BufferedMapWindow(parent=panel, giface=giface, Map=map_,
                                   properties=mapWindowProperties)

        giface.mapWindow = window

        sizer.Add(item=window, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        panel.SetSizer(sizer)
        panel.Layout()

        window.ZoomToWind()

        self._listenToAllMapWindowSignals(window)

        self.frame.Show()

        from mapwin.analysis import ProfileController
        self.controller = ProfileController(giface, window)
        self.controller.Start()

        rasters = []
        for layer in giface.GetLayerList().GetSelectedLayers():
            if layer.maplayer.GetType() == 'raster':
                rasters.append(layer.maplayer.GetName())

        from wxplot.profile import ProfileFrame
        profileWindow = ProfileFrame(parent=self.frame,
                                     controller=self.controller,
                                     units=map_.projinfo['units'],
                                     rasterList=rasters)
        profileWindow.CentreOnParent()
        profileWindow.Show()
        # Open raster select dialog to make sure that a raster (and
        # the desired raster) is selected to be profiled
        profileWindow.OnSelectRaster(None)

    def testMapWindowRlisetup(self, map_):
        self.frame = wx.Frame(parent=None,
                              title=_("Map window rlisetup test frame"))
                              
        RLiSetupMapPanel(parent=self.frame, map_=map_)
        self.frame.Show()

        
def main():
    """!Sets the GRASS display driver
    """
    driver = UserSettings.Get(group='display', key='driver', subkey='type')
    if driver == 'png':
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'png'
    else:
        os.environ['GRASS_RENDER_IMMEDIATE'] = 'cairo'

    # TODO: message format should not be GUI
    # TODO: should messages here be translatable?
    # (for test its great, for translator not)

    options, flags = grass.parser()
    test = options['test']

    app = wx.PySimpleApp()
    if not CheckWxVersion([2, 9]):
        wx.InitAllImageHandlers()

    map_ = Map()

    if options['raster']:
        names = options['raster']
        for name in names.split(','):
            cmdlist = ['d.rast', 'map=%s' % name]
            map_.AddLayer(ltype='raster', command=cmdlist, active=True,
                          name=name, hidden=False, opacity=1.0,
                          render=True)
    if options['vector']:
        names = options['vector']
        for name in names.split(','):
            cmdlist = ['d.vect', 'map=%s' % name]
            map_.AddLayer(ltype='vector', command=cmdlist, active=True,
                          name=name, hidden=False, opacity=1.0,
                          render=True)

    giface = MapdispGrassInterface(map_=map_)
    tester = Tester()

    if test == 'mapwindow':
        tester.testMapWindow(giface, map_)
    elif test == 'mapdisplay':
        tester.testMapDisplay(giface, map_)
    elif test == 'apitest':
        tester.testMapWindowApi(giface, map_)
    elif test == 'distance':
        tester.testMapWindowDistance(giface, map_)
    elif test == 'profile':
        tester.testMapWindowProfile(giface, map_)
    elif test == 'rlisetup':
        tester.testMapWindowRlisetup(map_)
    else:
        # TODO: this should not happen but happens
        import grass.script as sgrass
        sgrass.fatal(_("Unknown value %s of test parameter." % test))

    app.MainLoop()


if __name__ == '__main__':
    main()
