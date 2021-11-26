"""
@package iclass.plots

@brief wxIClass plots (histograms, coincidence plots).

Classes:
 - plots::PlotPanel

(C) 2006-2011,2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

import wx.lib.plot as plot
import wx.lib.scrolledpanel as scrolled
from core.gcmd import GError


class PlotPanel(scrolled.ScrolledPanel):
    """Panel for drawing multiple plots.

    There are three types of plots: histograms, coincidence plots and scatter plots.
    Histograms show frequency of cell category values in training areas
    for each band and for one category. Coincidence plots show min max range
    of classes for each band.
    """

    def __init__(self, parent, giface, stats_data):
        scrolled.ScrolledPanel.__init__(self, parent)

        self.SetupScrolling(scroll_x=False, scroll_y=True)
        self._giface = giface
        self.parent = parent
        self.canvasList = []
        self.bandList = []
        self.stats_data = stats_data
        self.currentCat = None

        self.mainSizer = wx.BoxSizer(wx.VERTICAL)

        self._createControlPanel()
        self._createPlotPanel()
        self._createScatterPlotPanel()

        self.SetSizer(self.mainSizer)
        self.mainSizer.Fit(self)
        self.Layout()

    def CloseWindow(self):
        if self.iscatt_panel:
            self.iscatt_panel.CloseWindow()

    def _createPlotPanel(self):

        self.canvasPanel = wx.Panel(parent=self)
        self.mainSizer.Add(self.canvasPanel, proportion=1, flag=wx.EXPAND, border=0)
        self.canvasSizer = wx.BoxSizer(wx.VERTICAL)
        self.canvasPanel.SetSizer(self.canvasSizer)

    def _createControlPanel(self):
        self.plotSwitch = wx.Choice(
            self,
            id=wx.ID_ANY,
            choices=[_("Histograms"), _("Coincident plots"), _("Scatter plots")],
        )
        self.mainSizer.Add(
            self.plotSwitch, proportion=0, flag=wx.EXPAND | wx.ALL, border=5
        )
        self.plotSwitch.Bind(wx.EVT_CHOICE, self.OnPlotTypeSelected)

    def _createScatterPlotPanel(self):
        """Init interactive scatter plot tool"""
        try:
            from iscatt.frame import IClassIScattPanel

            self.iscatt_panel = IClassIScattPanel(
                parent=self,
                giface=self._giface,
                iclass_mapwin=self.parent.GetFirstWindow(),
            )
            self.mainSizer.Add(
                self.iscatt_panel, proportion=1, flag=wx.EXPAND, border=0
            )
            self.iscatt_panel.Hide()
        except ImportError as e:
            self.scatt_error = _(
                "Scatter plot functionality is disabled.\n\nReason: "
                "Unable to import packages needed for scatter plot.\n%s" % e
            )
            wx.CallAfter(GError, self.scatt_error, showTraceback=False, parent=self)
            self.iscatt_panel = None

    def OnPlotTypeSelected(self, event):
        """Plot type selected"""

        if self.plotSwitch.GetSelection() in [0, 1]:
            self.SetupScrolling(scroll_x=False, scroll_y=True)
            if self.iscatt_panel:
                self.iscatt_panel.Hide()
            self.canvasPanel.Show()
            self.Layout()

        elif self.plotSwitch.GetSelection() == 2:
            self.SetupScrolling(scroll_x=False, scroll_y=False)
            if self.iscatt_panel:
                self.iscatt_panel.Show()
            else:
                GError(self.scatt_error)
            self.canvasPanel.Hide()
            self.Layout()

        if self.currentCat is None:
            return

        if self.plotSwitch.GetSelection() == 0:
            stat = self.stats_data.GetStatistics(self.currentCat)
            if not stat.IsReady():
                self.ClearPlots()
                return
            self.DrawHistograms(stat)
        else:
            self.DrawCoincidencePlots()

        self.Layout()

    def StddevChanged(self):
        """Standard deviation multiplier changed, redraw histograms"""
        if self.plotSwitch.GetSelection() == 0:
            stat = self.stats_data.GetStatistics(self.currentCat)
            self.UpdateRanges(stat)

    def EnableZoom(self, type, enable=True):
        for canvas in self.canvasList:
            canvas.enableZoom = enable

        # canvas.zoom = type

    def EnablePan(self, enable=True):
        for canvas in self.canvasList:
            canvas.SetEnableDrag(enable)

    def DestroyPlots(self):
        """Destroy all plot canvases"""
        for panel in self.canvasList:
            panel.Destroy()

        self.canvasList = []

    def ClearPlots(self):
        """Clears plot canvases"""
        for bandIdx in range(len(self.bandList)):
            self.canvasList[bandIdx].Clear()

    def Reset(self):
        """Reset plots (when new map imported)"""
        self.currentCat = None
        self.ClearPlots()
        # bands are still the same

    def CreatePlotCanvases(self):
        """Create plot canvases according to the number of bands"""
        for band in self.bandList:
            canvas = plot.PlotCanvas(self.canvasPanel)
            canvas.SetMinSize((-1, 140))
            canvas.fontSizeTitle = 10
            canvas.fontSizeAxis = 8
            self.canvasList.append(canvas)

            self.canvasSizer.Add(canvas, proportion=1, flag=wx.EXPAND, border=0)

        self.SetVirtualSize(self.GetBestVirtualSize())
        self.Layout()

    def UpdatePlots(self, group, subgroup, currentCat, stats_data):
        """Update plots after new analysis

        :param group: imagery group
        :param subgroup: imagery group
        :param currentCat: currently selected category (class)
        :param stats_data: StatisticsData instance (defined in statistics.py)
        """
        self.stats_data = stats_data
        self.currentCat = currentCat
        self.bandList = self.parent.GetGroupLayers(group, subgroup)

        graphType = self.plotSwitch.GetSelection()

        stat = self.stats_data.GetStatistics(currentCat)
        if not stat.IsReady() and graphType == 0:
            return

        self.DestroyPlots()
        self.CreatePlotCanvases()
        self.OnPlotTypeSelected(None)

    def UpdateCategory(self, cat):
        self.currentCat = cat

    def DrawCoincidencePlots(self):
        """Draw coincidence plots"""
        for bandIdx in range(len(self.bandList)):
            self.canvasList[bandIdx].ySpec = "none"
            lines = []
            level = 0.5
            lines.append(self.DrawInvisibleLine(level))

            cats = self.stats_data.GetCategories()
            for i, cat in enumerate(cats):
                stat = self.stats_data.GetStatistics(cat)
                if not stat.IsReady():
                    continue
                color = stat.color
                level = i + 1
                line = self.DrawCoincidenceLine(level, color, stat.bands[bandIdx])
                lines.append(line)

            # invisible
            level += 0.5
            lines.append(self.DrawInvisibleLine(level))

            plotGraph = plot.PlotGraphics(lines, title=self.bandList[bandIdx])
            self.canvasList[bandIdx].Draw(plotGraph)

    def DrawCoincidenceLine(self, level, color, bandValues):
        """Draw line between band min and max values

        :param level: y coordinate of line
        :param color: class color
        :param bandValues: BandStatistics instance
        """
        minim = bandValues.min
        maxim = bandValues.max
        points = [(minim, level), (maxim, level)]
        color = wx.Colour(*map(int, color.split(":")))
        return plot.PolyLine(points, colour=color, width=4)

    def DrawInvisibleLine(self, level):
        """Draw white line to achieve better margins"""
        points = [(100, level), (101, level)]
        return plot.PolyLine(points, colour=wx.WHITE, width=1)

    def DrawHistograms(self, statistics):
        """Draw histograms for one class

        :param statistics: statistics for one class
        """
        self.histogramLines = []
        for bandIdx in range(len(self.bandList)):
            self.canvasList[bandIdx].Clear()
            self.canvasList[bandIdx].ySpec = "auto"
            histgramLine = self.CreateHistogramLine(
                bandValues=statistics.bands[bandIdx]
            )

            meanLine = self.CreateMean(bandValues=statistics.bands[bandIdx])

            minLine = self.CreateMin(bandValues=statistics.bands[bandIdx])

            maxLine = self.CreateMax(bandValues=statistics.bands[bandIdx])

            self.histogramLines.append([histgramLine, meanLine, minLine, maxLine])

            maxRangeLine = self.CreateMaxRange(bandValues=statistics.bands[bandIdx])
            minRangeLine = self.CreateMinRange(bandValues=statistics.bands[bandIdx])

            plotGraph = plot.PlotGraphics(
                self.histogramLines[bandIdx] + [minRangeLine, maxRangeLine],
                title=self.bandList[bandIdx],
            )
            self.canvasList[bandIdx].Draw(plotGraph)

    def CreateMinRange(self, bandValues):
        maxVal = max(bandValues.histo)
        rMin = bandValues.rangeMin

        points = [(rMin, 0), (rMin, maxVal)]

        return plot.PolyLine(points, colour=wx.RED, width=1)

    def CreateMaxRange(self, bandValues):
        maxVal = max(bandValues.histo)
        rMax = bandValues.rangeMax
        points = [(rMax, 0), (rMax, maxVal)]

        return plot.PolyLine(points, colour=wx.RED, width=1)

    def CreateMean(self, bandValues):
        maxVal = max(bandValues.histo)
        mean = bandValues.mean
        points = [(mean, 0), (mean, maxVal)]

        return plot.PolyLine(points, colour=wx.BLUE, width=1)

    def CreateMin(self, bandValues):
        maxVal = max(bandValues.histo)
        minim = bandValues.min
        points = [(minim, 0), (minim, maxVal)]

        return plot.PolyLine(points, colour=wx.Colour(200, 200, 200), width=1)

    def CreateMax(self, bandValues):
        maxVal = max(bandValues.histo)
        maxim = bandValues.max
        points = [(maxim, 0), (maxim, maxVal)]

        return plot.PolyLine(points, colour=wx.Colour(200, 200, 200), width=1)

    def CreateHistogramLine(self, bandValues):
        points = []
        for cellCat, count in enumerate(bandValues.histo):
            if cellCat < bandValues.min - 5:
                continue
            if cellCat > bandValues.max + 5:
                break
            points.append((cellCat, count))

        return plot.PolyLine(points, colour=wx.BLACK, width=1)

    def UpdateRanges(self, statistics):
        """Redraw ranges lines in histograms when std dev multiplier changes

        :param statistics: python Statistics instance
        """
        for bandIdx in range(len(self.bandList)):
            self.canvasList[bandIdx].Clear()
            maxRangeLine = self.CreateMaxRange(bandValues=statistics.bands[bandIdx])
            minRangeLine = self.CreateMinRange(bandValues=statistics.bands[bandIdx])

            plotGraph = plot.PlotGraphics(
                self.histogramLines[bandIdx] + [minRangeLine, maxRangeLine],
                title=self.bandList[bandIdx],
            )
            self.canvasList[bandIdx].Draw(plotGraph)
