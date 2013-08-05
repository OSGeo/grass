# -*- coding: utf-8 -*-
"""!
@package mapdisp.analysis

@brief Map display controllers for analyses (profiling, measuring)

Classes:
 - analysis::AnalysisControllerBase
 - analysis::ProfileController
 - analysis::MeasureDistanceController

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import os
import math
import wx

from core.utils import _
import core.units as units

from grass.pydispatch.signal import Signal


class AnalysisControllerBase:
    """!Base class for analysis which require drawing line in map display."""
    def __init__(self, giface, mapWindow):
        """!

        @param giface grass interface
        @param mapWindow instance of BufferedWindow
        """
        self._giface = giface
        self._mapWindow = mapWindow

        self._registeredGraphics = None

        self._oldMouseUse = None
        self._oldCursor = None

    def IsActive(self):
        """!Returns True if analysis mode is activated."""
        return bool(self._registeredGraphics)

    def _start(self, x, y):
        """!Handles the actual start of drawing line
        and adding each new point.

        @param x,y east north coordinates
        """
        if not self._registeredGraphics.GetAllItems():
            item = self._registeredGraphics.AddItem(coords=[[x, y]])
            item.SetPropertyVal('penName', 'analysisPen')
        else:
            # needed to switch mouse begin and end to draw intermediate line properly
            coords = self._registeredGraphics.GetItem(0).GetCoords()[-1]
            self._mapWindow.mouse['begin'] = self._mapWindow.Cell2Pixel(coords)

    def _addPoint(self, x, y):
        """!New point added.

        @param x,y east north coordinates
        """
        # add new point and calculate distance
        item = self._registeredGraphics.GetItem(0)
        coords = item.GetCoords() + [[x, y]]
        item.SetCoords(coords)
        # draw
        self._mapWindow.ClearLines()
        self._registeredGraphics.Draw(pdc=self._mapWindow.pdcTmp)
        wx.Yield()

        self._doAnalysis(coords)

    def _doAnalysis(self, coords):
        """!Perform the required analysis
        (compute distnace, update profile)

        @param coords EN coordinates
        """
        raise NotImplementedError()

    def _disconnectAll(self):
        """!Disconnect all mouse signals
        to stop drawing."""
        raise NotImplementedError()

    def _connectAll(self):
        """!Connect all mouse signals to draw."""
        raise NotImplementedError()

    def _getPen(self):
        """!Returns wx.Pen instance."""
        raise NotImplementedError()

    def Stop(self, restore=True):
        """!Analysis mode is stopped.

        @param restore if restore previous cursor, mouse['use']
        """
        self._mapWindow.ClearLines(pdc=self._mapWindow.pdcTmp)
        self._mapWindow.mouse['end'] = self._mapWindow.mouse['begin']
        # disconnect mouse events
        self._disconnectAll()
        # unregister
        self._mapWindow.UnregisterGraphicsToDraw(self._registeredGraphics)
        self._registeredGraphics = None
        self._mapWindow.Refresh()

        if restore:
            # restore mouse['use'] and cursor to the state before measuring starts
            self._mapWindow.SetNamedCursor(self._oldCursor)
            self._mapWindow.mouse['use'] = self._oldMouseUse

    def Start(self):
        """!Init analysis: register graphics to map window,
        connect required mouse signals.
        """
        self._oldMouseUse = self._mapWindow.mouse['use']
        self._oldCursor = self._mapWindow.GetNamedCursor()

        self._registeredGraphics = self._mapWindow.RegisterGraphicsToDraw(graphicsType='line')

        self._connectAll()

        # change mouse['box'] and pen to draw line during dragging
        # TODO: better solution for drawing this line
        self._mapWindow.mouse['use'] = None
        self._mapWindow.mouse['box'] = "line"
        self._mapWindow.pen = wx.Pen(colour='red', width=2, style=wx.SHORT_DASH)

        self._registeredGraphics.AddPen('analysisPen', self._getPen())

        # change the cursor
        self._mapWindow.SetNamedCursor('pencil')


class ProfileController(AnalysisControllerBase):
    """!Class controls profiling in map display.
    It should be used inside ProfileFrame
    """
    def __init__(self, giface, mapWindow):
        AnalysisControllerBase.__init__(self, giface=giface, mapWindow=mapWindow)

        self.transectChanged = Signal('ProfileController.transectChanged')

    def _doAnalysis(self, coords):
        """!Informs profile dialog that profile changed.

        @param coords EN coordinates
        """
        self.transectChanged.emit(coords=coords)

    def _disconnectAll(self):
        self._mapWindow.mouseLeftDown.disconnect(self._start)
        self._mapWindow.mouseLeftUp.disconnect(self._addPoint)

    def _connectAll(self):
        self._mapWindow.mouseLeftDown.connect(self._start)
        self._mapWindow.mouseLeftUp.connect(self._addPoint)

    def _getPen(self):
        return wx.Pen(colour=wx.Colour(0, 100, 0), width=2, style=wx.SHORT_DASH)

    def Stop(self, restore=True):
        AnalysisControllerBase.Stop(self, restore=restore)

        self.transectChanged.emit(coords=[])


class MeasureDistanceController(AnalysisControllerBase):
    """!Class controls measuring distance in map display."""
    def __init__(self, giface, mapWindow):
        AnalysisControllerBase.__init__(self, giface=giface, mapWindow=mapWindow)

        self._projInfo = self._mapWindow.Map.projinfo
        self._totaldist = 0.0  # total measured distance
        self._useCtypes = False

    def _doAnalysis(self, coords):
        """!New point added.

        @param x,y east north coordinates
        """
        self.MeasureDist(coords[-2], coords[-1])

    def _disconnectAll(self):
        self._mapWindow.mouseLeftDown.disconnect(self._start)
        self._mapWindow.mouseLeftUp.disconnect(self._addPoint)
        self._mapWindow.mouseDClick.disconnect(self.Stop)

    def _connectAll(self):
        self._mapWindow.mouseLeftDown.connect(self._start)
        self._mapWindow.mouseLeftUp.connect(self._addPoint)
        self._mapWindow.mouseDClick.connect(self.Stop)

    def _getPen(self):
        return wx.Pen(colour='green', width=2, style=wx.SHORT_DASH)

    def Stop(self, restore=True):
        AnalysisControllerBase.Stop(self, restore=restore)

        self._giface.WriteCmdLog(_('Measuring finished'))

    def Start(self):
        """!Init measurement routine that calculates map distance
        along transect drawn on map display
        """
        AnalysisControllerBase.Start(self)
        self._totaldist = 0.0  # total measured distance

        # initiating output (and write a message)
        # e.g., in Layer Manager switch to output console
        # TODO: this should be something like: write important message or write tip
        # TODO: mixed 'switching' and message? no, measuring handles 'swithing' on its own
        self._giface.WriteWarning(_('Click and drag with left mouse button '
                                    'to measure.%s'
                                    'Double click with left button to clear.') % \
                                    (os.linesep))
        if self._projInfo['proj'] != 'xy':
            mapunits = self._projInfo['units']
            self._giface.WriteCmdLog(_('Measuring distance') + ' ('
                                      + mapunits + '):')
        else:
            self._giface.WriteCmdLog(_('Measuring distance:'))

        if self._projInfo['proj'] == 'll':
            try:
                import grass.lib.gis as gislib
                gislib.G_begin_distance_calculations()
                self._useCtypes = True
            except ImportError, e:
                self._giface.WriteWarning(_('Geodesic distance calculation '
                                            'is not available.\n'
                                            'Reason: %s' % e))

    def MeasureDist(self, beginpt, endpt):
        """!Calculate distance and print to output window.

        @param beginpt,endpt EN coordinates
        """
        # move also Distance method?
        dist, (north, east) = self._mapWindow.Distance(beginpt, endpt, screen=False)

        dist = round(dist, 3)
        mapunits = self._projInfo['units']
        if mapunits == 'degrees' and self._useCtypes:
            mapunits = 'meters'
        d, dunits = units.formatDist(dist, mapunits)

        self._totaldist += dist
        td, tdunits = units.formatDist(self._totaldist,
                                       mapunits)

        strdist = str(d)
        strtotdist = str(td)

        if self._projInfo['proj'] == 'xy' or 'degree' not in self._projInfo['unit']:
            angle = int(math.degrees(math.atan2(north, east)) + 0.5)
            # uncomment below (or flip order of atan2(y,x) above) to use
            #   the mathematical theta convention (CCW from +x axis)
            #angle = 90 - angle
            if angle < 0:
                angle = 360 + angle

            mstring = '%s = %s %s\n%s = %s %s\n%s = %d %s\n%s' \
                % (_('segment'), strdist, dunits,
                   _('total distance'), strtotdist, tdunits,
                   _('bearing'), angle, _('degrees (clockwise from grid-north)'),
                   '-' * 60)
        else:
            mstring = '%s = %s %s\n%s = %s %s\n%s' \
                % (_('segment'), strdist, dunits,
                   _('total distance'), strtotdist, tdunits,
                   '-' * 60)

        self._giface.WriteLog(mstring, priority=2)

        return dist
