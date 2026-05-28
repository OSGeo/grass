"""
@package wxplot.base

@brief Base classes for interactive plotting using PyPlot

Classes:
 - base::PlotIcons
 - base::BasePlotFrame

(C) 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton, Arizona State University
"""

import os

import wx
from random import randint

from wx.lib import plot
from core.globalvar import ICONDIR
from core.settings import UserSettings
from wxplot.dialogs import TextDialog, OptDialog
from core.render import Map
from icons.icon import MetaIcon
from gui_core.toolbars import BaseIcons
from gui_core.wrap import Menu

import grass.script as gs
from grass.exceptions import CalledModuleError

PlotIcons = {
    "draw": MetaIcon(img="show", label=_("Draw/re-draw plot")),
    "transect": MetaIcon(
        img="layer-raster-profile",
        label=_("Draw transect in map display window to profile"),
    ),
    "options": BaseIcons["settings"],
    "statistics": MetaIcon(img="stats", label=_("Plot statistics")),
    "save": MetaIcon(img="save", label=_("Save profile data to CSV file")),
    "quit": BaseIcons["quit"],
}


class BasePlotFrame(wx.Frame):
    """Abstract PyPlot display frame class"""

    def __init__(
        self,
        parent=None,
        giface=None,
        size=wx.Size(700, 400),
        style=wx.DEFAULT_FRAME_STYLE,
        rasterList=[],
        **kwargs,
    ):
        wx.Frame.__init__(self, parent, id=wx.ID_ANY, size=size, style=style, **kwargs)

        self.parent = parent  # MapFrame for a plot type
        self._giface = giface
        self.Map = Map()  # instance of render.Map to be associated with display
        self.rasterList = rasterList  # list of rasters to plot
        self.raster = {}  # dictionary of raster maps and their plotting parameters
        self.plottype = ""

        self.linestyledict = {
            "solid": wx.SOLID,
            "dot": wx.DOT,
            "long-dash": wx.LONG_DASH,
            "short-dash": wx.SHORT_DASH,
            "dot-dash": wx.DOT_DASH,
        }

        self.ptfilldict = {"transparent": wx.TRANSPARENT, "solid": wx.SOLID}

        #
        # Icon
        #
        self.SetIcon(wx.Icon(os.path.join(ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO))

        #
        # Add statusbar
        #
        self.statusbar = self.CreateStatusBar(number=2, style=0)
        self.statusbar.SetStatusWidths([-2, -1])

        #
        # Define canvas and settings
        #
        #
        self.client = plot.PlotCanvas(self)

        # define the function for drawing pointLabels
        self.client.pointLabelFunc = self.DrawPointLabel

        # Create mouse event for showing cursor coords in status bar
        self.client.canvas.Bind(wx.EVT_LEFT_DOWN, self.OnMouseLeftDown)

        # Show closest point when enabled
        self.client.canvas.Bind(wx.EVT_MOTION, self.OnMotion)

        self.plotlist = []  # list of things to plot
        self.plot = None  # plot draw object
        self.ptitle = ""  # title of window
        self.xlabel = ""  # default X-axis label
        self.ylabel = ""  # default Y-axis label

        self.CentreOnScreen()

        self._createColorDict()

    def _createColorDict(self):
        """Create color dictionary to return wx.Colour tuples
        for assigning colors to images in imagery groups"""

        self.colorDict = {}
        for clr in gs.named_colors.keys():
            if clr == "white":
                continue
            r = int(gs.named_colors[clr][0] * 255)
            g = int(gs.named_colors[clr][1] * 255)
            b = int(gs.named_colors[clr][2] * 255)
            self.colorDict[clr] = (r, g, b, 255)

    def InitPlotOpts(self, plottype):
        """Initialize options for entire plot"""
        self.plottype = plottype  # histogram, profile, or scatter

        self.properties = {}  # plot properties
        self.properties["font"] = {}
        self.properties["font"]["prop"] = UserSettings.Get(
            group=self.plottype, key="font"
        )
        self.wx_font = wx.Font(
            self.properties["font"]["prop"]["defaultSize"],
            self.properties["font"]["prop"]["family"],
            self.properties["font"]["prop"]["style"],
            self.properties["font"]["prop"]["weight"],
        )

        self.properties["raster"] = {}
        self.properties["raster"] = UserSettings.Get(group=self.plottype, key="raster")
        colstr = str(self.properties["raster"]["pcolor"])
        self.properties["raster"]["pcolor"] = tuple(
            int(colval) for colval in colstr.strip("()").split(",")
        )

        if self.plottype == "profile":
            self.properties["marker"] = UserSettings.Get(
                group=self.plottype, key="marker"
            )
            # changing color string to tuple for markers/points
            colstr = str(self.properties["marker"]["color"])
            self.properties["marker"]["color"] = tuple(
                int(colval) for colval in colstr.strip("()").split(",")
            )

        self.properties["grid"] = UserSettings.Get(group=self.plottype, key="grid")
        # changing color string to tuple
        colstr = str(self.properties["grid"]["color"])
        self.properties["grid"]["color"] = tuple(
            int(colval) for colval in colstr.strip("()").split(",")
        )

        self.properties["x-axis"] = {}
        self.properties["x-axis"]["prop"] = UserSettings.Get(
            group=self.plottype, key="x-axis"
        )
        self.properties["x-axis"]["axis"] = None

        self.properties["y-axis"] = {}
        self.properties["y-axis"]["prop"] = UserSettings.Get(
            group=self.plottype, key="y-axis"
        )
        self.properties["y-axis"]["axis"] = None

        self.properties["legend"] = UserSettings.Get(group=self.plottype, key="legend")

        self.zoom = False  # zooming disabled
        self.drag = False  # dragging disabled

        # x and y axis set to normal (non-log)
        self.client.logScale = (False, False)
        if self.properties["x-axis"]["prop"]["type"] == "custom":
            self.client.xSpec = "min"
        else:
            self.client.xSpec = self.properties["x-axis"]["prop"]["type"]

        if self.properties["y-axis"]["prop"]["type"] == "custom":
            self.client.ySpec = "min"
        else:
            self.client.ySpec = self.properties["y-axis"]["prop"]["type"]

    def InitRasterOpts(self, rasterList, plottype):
        """Initialize or update raster dictionary for plotting"""

        rdict = {}  # initialize a dictionary
        self.properties["raster"] = UserSettings.Get(group=self.plottype, key="raster")

        for r in rasterList:
            idx = rasterList.index(r)

            try:
                ret = gs.raster_info(r)
            except CalledModuleError:
                continue
                # if r.info cannot parse map, skip it

            self.raster[r] = self.properties["raster"]  # some default settings
            rdict[r] = {}  # initialize sub-dictionaries for each raster in the list

            rdict[r]["units"] = ""
            if ret["units"] not in {"(none)", '"none"', "", None}:
                rdict[r]["units"] = ret["units"]

            rdict[r]["plegend"] = r  # use fully-qualified names
            # list of cell value,frequency pairs for plotting histogram
            rdict[r]["datalist"] = []
            rdict[r]["pline"] = None
            rdict[r]["datatype"] = ret["datatype"]

            #
            # initialize with saved values
            #
            if self.properties["raster"]["pwidth"] is not None:
                rdict[r]["pwidth"] = self.properties["raster"]["pwidth"]
            else:
                rdict[r]["pwidth"] = 1

            if (
                self.properties["raster"]["pstyle"] is not None
                and self.properties["raster"]["pstyle"] != ""
            ):
                rdict[r]["pstyle"] = self.properties["raster"]["pstyle"]
            else:
                rdict[r]["pstyle"] = "solid"

            if idx < len(self.colorList):
                if idx == 0:
                    # use saved color for first plot
                    if self.properties["raster"]["pcolor"] is not None:
                        rdict[r]["pcolor"] = self.properties["raster"]["pcolor"]
                    else:
                        rdict[r]["pcolor"] = self.colorDict[self.colorList[idx]]
                else:
                    rdict[r]["pcolor"] = self.colorDict[self.colorList[idx]]
                continue

            r = randint(0, 255)
            b = randint(0, 255)
            g = randint(0, 255)
            rdict[r]["pcolor"] = (r, g, b, 255)

        return rdict

    def InitRasterPairs(self, rasterList, plottype):
        """Initialize or update raster dictionary with raster pairs for
        bivariate scatterplots
        """

        if len(rasterList) == 0:
            return None

        rdict = {}  # initialize a dictionary
        for rpair in rasterList:
            idx = rasterList.index(rpair)

            try:
                ret0 = gs.raster_info(rpair[0])
                ret1 = gs.raster_info(rpair[1])

            except (IndexError, CalledModuleError):
                continue
                # if r.info cannot parse map, skip it

            self.raster[rpair] = UserSettings.Get(
                group=plottype, key="rasters"
            )  # some default settings
            # initialize sub-dictionaries for each raster in the list
            rdict[rpair] = {}
            rdict[rpair][0] = {}
            rdict[rpair][1] = {}
            rdict[rpair][0]["units"] = ""
            rdict[rpair][1]["units"] = ""

            if ret0["units"] not in {"(none)", '"none"', "", None}:
                rdict[rpair][0]["units"] = ret0["units"]
            if ret1["units"] not in {"(none)", '"none"', "", None}:
                rdict[rpair][1]["units"] = ret1["units"]

            rdict[rpair]["plegend"] = (
                rpair[0].split("@")[0] + " vs " + rpair[1].split("@")[0]
            )
            # list of cell value,frequency pairs for plotting histogram
            rdict[rpair]["datalist"] = []
            rdict[rpair][0]["datatype"] = ret0["datatype"]
            rdict[rpair][1]["datatype"] = ret1["datatype"]

            #
            # initialize with saved values
            #
            if (
                self.properties["raster"]["ptype"] is not None
                and self.properties["raster"]["ptype"] != ""
            ):
                rdict[rpair]["ptype"] = self.properties["raster"]["ptype"]
            else:
                rdict[rpair]["ptype"] = "dot"
            if self.properties["raster"]["psize"] is not None:
                rdict[rpair]["psize"] = self.properties["raster"]["psize"]
            else:
                rdict[rpair]["psize"] = 1
            if (
                self.properties["raster"]["pfill"] is not None
                and self.properties["raster"]["pfill"] != ""
            ):
                rdict[rpair]["pfill"] = self.properties["raster"]["pfill"]
            else:
                rdict[rpair]["pfill"] = "solid"

            if idx <= len(self.colorList):
                rdict[rpair]["pcolor"] = self.colorDict[self.colorList[idx]]
                continue

            r = randint(0, 255)
            b = randint(0, 255)
            g = randint(0, 255)
            rdict[rpair]["pcolor"] = (r, g, b, 255)

        return rdict

    def SetGraphStyle(self):
        """Set plot and text options"""
        self.client.SetFont(self.wx_font)
        self.client.fontSizeTitle = self.properties["font"]["prop"]["titleSize"]
        self.client.fontSizeAxis = self.properties["font"]["prop"]["axisSize"]

        self.client.enableZoom = self.zoom
        self.client.enableDrag = self.drag

        #
        # axis settings
        #
        if self.properties["x-axis"]["prop"]["type"] == "custom":
            self.client.xSpec = "min"
        else:
            self.client.xSpec = self.properties["x-axis"]["prop"]["type"]

        if self.properties["y-axis"]["prop"]["type"] == "custom":
            self.client.ySpec = "min"
        else:
            self.client.ySpec = self.properties["y-axis"]["prop"]["type"]

        if (
            self.properties["x-axis"]["prop"]["type"] == "custom"
            and self.properties["x-axis"]["prop"]["min"]
            < self.properties["x-axis"]["prop"]["max"]
        ):
            self.properties["x-axis"]["axis"] = (
                self.properties["x-axis"]["prop"]["min"],
                self.properties["x-axis"]["prop"]["max"],
            )
        else:
            self.properties["x-axis"]["axis"] = None

        if (
            self.properties["y-axis"]["prop"]["type"] == "custom"
            and self.properties["y-axis"]["prop"]["min"]
            < self.properties["y-axis"]["prop"]["max"]
        ):
            self.properties["y-axis"]["axis"] = (
                self.properties["y-axis"]["prop"]["min"],
                self.properties["y-axis"]["prop"]["max"],
            )
        else:
            self.properties["y-axis"]["axis"] = None

        if self.properties["x-axis"]["prop"]["log"]:
            self.properties["x-axis"]["axis"] = None
            self.client.xSpec = "min"
        if self.properties["y-axis"]["prop"]["log"]:
            self.properties["y-axis"]["axis"] = None
            self.client.ySpec = "min"

        self.client.logScale = (
            self.properties["x-axis"]["prop"]["log"],
            self.properties["y-axis"]["prop"]["log"],
        )

        #
        # grid settings
        #
        self.client.enableGrid = self.properties["grid"]["enabled"]
        gridpen = wx.Pen(
            colour=wx.Colour(
                self.properties["grid"]["color"][0],
                self.properties["grid"]["color"][1],
                self.properties["grid"]["color"][2],
                255,
            )
        )
        self.client.gridPen = gridpen

        #
        # legend settings
        #
        self.client.fontSizeLegend = self.properties["font"]["prop"]["legendSize"]
        self.client.enableLegend = self.properties["legend"]["enabled"]

    def DrawPlot(self, plotlist):
        """Draw line and point plot from list plot elements."""
        xlabel, ylabel = self._getPlotLabels()
        self.plot = plot.PlotGraphics(plotlist, self.ptitle, xlabel, ylabel)

        if self.properties["x-axis"]["prop"]["type"] == "custom":
            self.client.xSpec = "min"
        else:
            self.client.xSpec = self.properties["x-axis"]["prop"]["type"]

        if self.properties["y-axis"]["prop"]["type"] == "custom":
            self.client.ySpec = "min"
        else:
            self.client.ySpec = self.properties["y-axis"]["prop"]["type"]

        self.client.Draw(
            self.plot,
            self.properties["x-axis"]["axis"],
            self.properties["y-axis"]["axis"],
        )

    def DrawPointLabel(self, dc, mDataDict):
        """This is the function that defines how the pointLabels are
        plotted dc - DC that will be passed mDataDict - Dictionary
        of data that you want to use for the pointLabel

        As an example I have decided I want a box at the curve
        point with some text information about the curve plotted
        below.  Any wxDC method can be used.
        """
        dc.SetPen(wx.Pen(wx.BLACK))
        dc.SetBrush(wx.Brush(wx.BLACK, wx.SOLID))

        sx, sy = mDataDict["scaledXY"]  # scaled x,y of closest point
        # 10by10 square centered on point
        dc.DrawRectangle(sx - 5, sy - 5, 10, 10)
        px, py = mDataDict["pointXY"]
        cNum = mDataDict["curveNum"]
        pntIn = mDataDict["pIndex"]
        legend = mDataDict["legend"]
        # make a string to display
        s = "Crv# %i, '%s', Pt. (%.2f,%.2f), PtInd %i" % (cNum, legend, px, py, pntIn)
        dc.DrawText(s, sx, sy + 1)

    def OnZoom(self, event):
        """Enable zooming and disable dragging"""
        self.zoom = True
        self.drag = False
        self.client.enableZoom = self.zoom
        self.client.enableDrag = self.drag

    def OnDrag(self, event):
        """Enable dragging and disable zooming"""
        self.zoom = False
        self.drag = True
        self.client.enableDrag = self.drag
        self.client.enableZoom = self.zoom

    def OnRedraw(self, event):
        """Redraw the plot window. Unzoom to original size"""
        self.UpdateLabels()
        self.client.Reset()
        self.client.Redraw()

    def OnErase(self, event):
        """Erase the plot window"""
        self.client.Clear()

    def SaveToFile(self, event):
        """Save plot to graphics file"""
        self.client.SaveFile()

    def OnMouseLeftDown(self, event):
        self.SetStatusText(
            _("Left Mouse Down at Point:") + " (%.4f, %.4f)" % self.client._getXY(event)
        )
        event.Skip()  # allows plotCanvas OnMouseLeftDown to be called

    def OnMotion(self, event):
        """Indicate when mouse is outside the plot area"""
        if self.client.enablePointLabel:
            # make up dict with info for the pointLabel
            # I've decided to mark the closest point on the closest curve
            dlst = self.client.GetClosestPoint(
                self.client._getXY(event), pointScaled=True
            )
            if dlst != []:  # returns [] if none
                curveNum, legend, pIndex, pointXY, scaledXY, distance = dlst
                # make up dictionary to pass to my user function (see
                # DrawPointLabel)
                mDataDict = {
                    "curveNum": curveNum,
                    "legend": legend,
                    "pIndex": pIndex,
                    "pointXY": pointXY,
                    "scaledXY": scaledXY,
                }
                # pass dict to update the pointLabel
                self.client.UpdatePointLabel(mDataDict)
        event.Skip()  # go to next handler

    def PlotOptionsMenu(self, event):
        """Popup menu for plot and text options"""
        popt = Menu()
        # Add items to the menu
        settext = wx.MenuItem(popt, wx.ID_ANY, _("Text settings"))
        popt.AppendItem(settext)
        self.Bind(wx.EVT_MENU, self.PlotText, settext)

        setgrid = wx.MenuItem(popt, wx.ID_ANY, _("Plot settings"))
        popt.AppendItem(setgrid)
        self.Bind(wx.EVT_MENU, self.PlotOptions, setgrid)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(popt)
        popt.Destroy()

    def NotFunctional(self):
        """Creates a 'not functional' message dialog"""
        dlg = wx.MessageDialog(
            parent=self,
            message=_("This feature is not yet functional"),
            caption=_("Under Construction"),
            style=wx.OK | wx.ICON_INFORMATION,
        )
        dlg.ShowModal()
        dlg.Destroy()

    def _getPlotLabels(self):
        def log(txt):
            return "log( " + txt + " )"

        x = self.xlabel
        if self.properties["x-axis"]["prop"]["log"]:
            x = log(x)

        y = self.ylabel
        if self.properties["y-axis"]["prop"]["log"]:
            y = log(y)

        return x, y

    def OnPlotText(self, dlg):
        """Custom text settings for histogram plot."""
        self.ptitle = dlg.ptitle
        self.xlabel = dlg.xlabel
        self.ylabel = dlg.ylabel

        if self.plot:
            self.plot.title = dlg.ptitle

        self.OnRedraw(event=None)

    def UpdateLabels(self):
        x, y = self._getPlotLabels()

        self.client.SetFont(self.wx_font)
        self.client.fontSizeTitle = self.properties["font"]["prop"]["titleSize"]
        self.client.fontSizeAxis = self.properties["font"]["prop"]["axisSize"]

        if self.plot:
            self.plot.xLabel = x
            self.plot.yLabel = y

    def PlotText(self, event):
        """Set custom text values for profile title and axis labels."""
        dlg = TextDialog(
            parent=self,
            giface=self._giface,
            id=wx.ID_ANY,
            plottype=self.plottype,
            title=_("Text settings"),
        )

        btnval = dlg.ShowModal()
        if btnval in {wx.ID_SAVE, wx.ID_OK, wx.ID_CANCEL}:
            dlg.Destroy()

    def PlotOptions(self, event):
        """Set various profile options, including: line width, color,
        style; marker size, color, fill, and style; grid and legend
        options.  Calls OptDialog class.
        """

        dlg = OptDialog(
            parent=self,
            giface=self._giface,
            id=wx.ID_ANY,
            plottype=self.plottype,
            title=_("Plot settings"),
        )
        btnval = dlg.ShowModal()

        if btnval in {wx.ID_SAVE, wx.ID_OK, wx.ID_CANCEL}:
            dlg.Destroy()
        self.Update()

    def PrintMenu(self, event):
        """Print options and output menu"""
        printmenu = Menu()
        for title, handler in (
            (_("Page setup"), self.OnPageSetup),
            (_("Print preview"), self.OnPrintPreview),
            (_("Print display"), self.OnDoPrint),
        ):
            item = wx.MenuItem(printmenu, wx.ID_ANY, title)
            printmenu.AppendItem(item)
            self.Bind(wx.EVT_MENU, handler, item)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(printmenu)
        printmenu.Destroy()

    def OnPageSetup(self, event):
        self.client.PageSetup()

    def OnPrintPreview(self, event):
        self.client.PrintPreview()

    def OnDoPrint(self, event):
        self.client.Printout()

    def OnQuit(self, event):
        self.Close(True)
