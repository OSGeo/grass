"""
@package wxplot.profile

@brief Profiling using PyPlot

Classes:
 - profile::ProfileFrame
 - profile::ProfileToolbar

(C) 2011-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton, Arizona State University
"""

import os
import sys
import six
import math
import numpy

import wx

import wx.lib.plot as plot
import grass.script as grass
from wxplot.base import BasePlotFrame, PlotIcons
from gui_core.toolbars import BaseToolbar, BaseIcons
from gui_core.wrap import StockCursor
from wxplot.dialogs import ProfileRasterDialog, PlotStatsFrame
from core.gcmd import RunCommand, GWarning, GError, GMessage


class ProfileFrame(BasePlotFrame):
    """Mainframe for displaying profile of one or more raster maps. Uses wx.lib.plot."""

    def __init__(
        self,
        parent,
        giface,
        controller,
        units,
        size=wx.Size(700, 400),
        rasterList=None,
        **kwargs,
    ):
        BasePlotFrame.__init__(self, parent=parent, giface=giface, size=size, **kwargs)

        self.controller = controller
        self.controller.transectChanged.connect(self.SetTransect)
        self.transect = []
        self.toolbar = ProfileToolbar(parent=self)
        # workaround for http://trac.wxwidgets.org/ticket/13888
        if sys.platform != "darwin":
            self.SetToolBar(self.toolbar)
        self.SetTitle(_("GRASS Profile Analysis Tool"))
        # in case of degrees, we want to use meters
        self._units = units if "degree" not in units else "meters"

        #
        # Init variables
        #
        if rasterList is None:
            self.rasterList = []
        else:
            self.rasterList = rasterList
        self.plottype = "profile"
        self.coordstr = ""  # string of coordinates for r.profile
        self.seglist = []  # segment endpoint list
        self.ppoints = ""  # segment endpoints data
        self.transect_length = 0.0  # total transect length
        self.ptitle = _("Profile of")  # title of window
        self.colorList = [
            "blue",
            "red",
            "green",
            "yellow",
            "magenta",
            "cyan",
            "aqua",
            "black",
            "grey",
            "orange",
            "brown",
            "purple",
            "violet",
            "indigo",
        ]

        self._initOpts()

        if (
            len(self.rasterList) > 0
        ):  # set raster name(s) from layer manager if a map is selected
            self.raster = self.InitRasterOpts(self.rasterList, self.plottype)
        else:
            self.raster = {}

        # determine units (axis labels)
        # maybe, we should not accept these invalid units
        # but ok, trying to handle it here
        if self._units is not None and self._units != "":
            self.xlabel = _("Distance (%s)") % self._units
        else:
            self.xlabel = _("Distance along transect")
        self.ylabel = _("Cell values")

        # Bind events
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        self.SetGraphStyle()

    def _initOpts(self):
        """Initialize plot options"""
        self.InitPlotOpts("profile")

    def SetTransect(self, coords):
        self.transect = coords
        if coords:
            self.OnCreateProfile(None)
        else:
            self.OnErase(None)

    def OnDrawTransect(self, event):
        """Draws transect to profile in map display"""
        if self.controller.IsActive():
            self.controller.Stop()
        self.controller.Start()

        self.parent.SetFocus()
        self.parent.Raise()

    def OnSelectRaster(self, event):
        """Select raster map(s) to profile"""
        dlg = ProfileRasterDialog(parent=self)
        dlg.CenterOnParent()
        if dlg.ShowModal() == wx.ID_OK:
            self.rasterList = dlg.rasterList
            self.raster = self.InitRasterOpts(self.rasterList, self.plottype)

            # plot profile
            if len(self.transect) > 0 and len(self.rasterList) > 0:
                self.OnCreateProfile(event=None)

        dlg.Destroy()

    def SetupProfile(self):
        """Create coordinate string for profiling. Create segment
        list for transect segment markers.
        """
        # create list of coordinate points for r.profile
        dist = 0
        cumdist = 0
        self.coordstr = ""
        lasteast = lastnorth = None

        region = grass.region()
        insideRegion = True
        if len(self.transect) > 0:
            for point in self.transect:
                if not (
                    region["w"] <= point[0] <= region["e"]
                    and region["s"] <= point[1] <= region["n"]
                ):
                    insideRegion = False
                # build string of coordinate points for r.profile
                if self.coordstr == "":
                    self.coordstr = "%f,%f" % (point[0], point[1])
                else:
                    self.coordstr = "%s,%f,%f" % (self.coordstr, point[0], point[1])

        if not insideRegion:
            GWarning(
                message=_("Not all points of profile lie inside computational region."),
                parent=self,
            )

        if len(self.rasterList) == 0:
            return

        # title of window
        self.ptitle = _("Profile of")

        # create list of coordinates for transect segment markers
        if len(self.transect) > 0:
            self.seglist = []
            for point in self.transect:
                # get value of raster cell at coordinate point
                ret = RunCommand(
                    "r.what",
                    parent=self,
                    read=True,
                    map=self.rasterList[0],
                    coordinates="%f,%f" % (point[0], point[1]),
                )

                val = ret.splitlines()[0].split("|")[3]
                if val is None or val == "*":
                    continue
                val = float(val)

                # calculate distance between coordinate points
                if lasteast and lastnorth:
                    dist = math.sqrt(
                        math.pow((lasteast - point[0]), 2)
                        + math.pow((lastnorth - point[1]), 2)
                    )
                cumdist += dist

                # store total transect length
                self.transect_length = cumdist

                # build a list of distance,value pairs for each segment of
                # transect
                self.seglist.append((cumdist, val))
                lasteast = point[0]
                lastnorth = point[1]

            # delete extra first segment point
            try:
                self.seglist.pop(0)
            except:
                pass

        #
        # create datalist of dist/value pairs and y labels for each raster map
        #
        self.ylabel = ""
        i = 0

        for r in six.iterkeys(self.raster):
            self.raster[r]["datalist"] = []
            datalist = self.CreateDatalist(r, self.coordstr)
            if len(datalist) > 0:
                self.raster[r]["datalist"] = datalist

                # update ylabel to match units if they exist
                if self.raster[r]["units"] != "":
                    self.ylabel += "%s (%d)," % (self.raster[r]["units"], i)
                i += 1

                # update title
                self.ptitle += " %s ," % r.split("@")[0]

        self.ptitle = self.ptitle.rstrip(",")

        if self.ylabel == "":
            self.ylabel = _("Raster values")
        else:
            self.ylabel = self.ylabel.rstrip(",")

    def CreateDatalist(self, raster, coords):
        """Build a list of distance, value pairs for points along transect

        Uses r.profile to obtain the data.
        """
        datalist = []

        # keep total number of transect points to 500 or less to avoid
        # freezing with large, high resolution maps
        region = grass.region()
        curr_res = min(float(region["nsres"]), float(region["ewres"]))
        transect_rec = 0
        if self.transect_length / curr_res > 500:
            transect_res = self.transect_length / 500
        else:
            transect_res = curr_res

        ret = RunCommand(
            "r.profile",
            parent=self,
            input=raster,
            coordinates=coords,
            resolution=transect_res,
            null="nan",
            quiet=True,
            read=True,
        )

        if not ret:
            return []

        for line in ret.splitlines():
            dist, elev = line.strip().split(" ")
            if (
                dist is None
                or dist == ""
                or dist == "nan"
                or elev is None
                or elev == ""
                or elev == "nan"
            ):
                continue
            dist = float(dist)
            elev = float(elev)
            datalist.append((dist, elev))

        return datalist

    def OnCreateProfile(self, event):
        """Main routine for creating a profile. Uses r.profile to
        create a list of distance,cell value pairs. This is passed to
        plot to create a line graph of the profile. If the profile
        transect is in multiple segments, these are drawn as
        points. Profile transect is drawn, using methods in mapdisp.py
        """

        if len(self.transect) == 0 or len(self.rasterList) == 0:
            dlg = wx.MessageDialog(
                parent=self,
                message=_(
                    "You must draw a transect to profile in the map display window."
                ),
                caption=_("Nothing to profile"),
                style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE,
            )
            dlg.ShowModal()
            dlg.Destroy()
            return

        self.SetCursor(StockCursor(wx.CURSOR_ARROW))

        self.SetupProfile()
        p = self.CreatePlotList()
        self.DrawPlot(p)

    def CreatePlotList(self):
        """Create a plot data list from transect datalist and
        transect segment endpoint coordinates.
        """
        # graph the distance, value pairs for the transect
        self.plotlist = []

        # Add segment marker points to plot data list
        if len(self.seglist) > 0:
            self.ppoints = plot.PolyMarker(
                self.seglist,
                legend=" " + self.properties["marker"]["legend"],
                colour=wx.Colour(
                    self.properties["marker"]["color"][0],
                    self.properties["marker"]["color"][1],
                    self.properties["marker"]["color"][2],
                    255,
                ),
                size=self.properties["marker"]["size"],
                fillstyle=self.ptfilldict[self.properties["marker"]["fill"]],
                marker=self.properties["marker"]["type"],
            )
            self.plotlist.append(self.ppoints)

        # Add profile distance/elevation pairs to plot data list for each
        # raster profiled
        for r in self.rasterList:
            col = wx.Colour(
                self.raster[r]["pcolor"][0],
                self.raster[r]["pcolor"][1],
                self.raster[r]["pcolor"][2],
                255,
            )
            self.raster[r]["pline"] = plot.PolyLine(
                self.raster[r]["datalist"],
                colour=col,
                width=self.raster[r]["pwidth"],
                style=self.linestyledict[self.raster[r]["pstyle"]],
                legend=self.raster[r]["plegend"],
            )

            self.plotlist.append(self.raster[r]["pline"])

        if len(self.plotlist) > 0:
            return self.plotlist
        else:
            return None

    def Update(self):
        """Update profile after changing options"""
        self.SetGraphStyle()
        p = self.CreatePlotList()
        self.DrawPlot(p)

    def SaveProfileToFile(self, event):
        """Save r.profile data to a csv file"""
        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose prefix for file(s) where to save profile values..."),
            defaultDir=os.getcwd(),
            wildcard=_("Comma separated value (*.csv)|*.csv"),
            style=wx.FD_SAVE,
        )
        pfile = []
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            for r in self.rasterList:
                pfile.append(path + "_" + str(r.replace("@", "_")) + ".csv")
                if os.path.exists(pfile[-1]):
                    dlgOv = wx.MessageDialog(
                        self,
                        message=_(
                            "File <%s> already exists. "
                            "Do you want to overwrite this file?"
                        )
                        % pfile[-1],
                        caption=_("Overwrite file?"),
                        style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
                    )
                    if dlgOv.ShowModal() != wx.ID_YES:
                        pfile.pop()
                        dlgOv.Destroy()
                        continue

                try:
                    fd = open(pfile[-1], "w")
                except IOError as e:
                    GError(
                        parent=self,
                        message=_(
                            "Unable to open file <%s> for writing.\n" "Reason: %s"
                        )
                        % (pfile[-1], e),
                    )
                    dlg.Destroy()
                    return

                for datapair in self.raster[r]["datalist"]:
                    fd.write("%.6f,%.6f\n" % (float(datapair[0]), float(datapair[1])))

                fd.close()

        dlg.Destroy()
        if pfile:
            message = _("%d files created:\n%s") % (len(pfile), "\n".join(pfile))
        else:
            message = _("No files generated.")

        GMessage(parent=self, message=message)

    def OnStats(self, event):
        """Displays regression information in messagebox"""
        message = []
        title = _("Statistics for Profile(s)")

        for r in six.iterkeys(self.raster):
            try:
                rast = r.split("@")[0]
                statstr = "Profile of %s\n\n" % rast

                iterable = (i[1] for i in self.raster[r]["datalist"])
                a = numpy.fromiter(iterable, numpy.float)

                statstr += "n: %f\n" % a.size
                statstr += "minimum: %f\n" % numpy.amin(a)
                statstr += "maximum: %f\n" % numpy.amax(a)
                statstr += "range: %f\n" % numpy.ptp(a)
                statstr += "mean: %f\n" % numpy.mean(a)
                statstr += "standard deviation: %f\n" % numpy.std(a)
                statstr += "variance: %f\n" % numpy.var(a)
                cv = numpy.std(a) / numpy.mean(a)
                statstr += "coefficient of variation: %f\n" % cv
                statstr += "sum: %f\n" % numpy.sum(a)
                statstr += "median: %f\n" % numpy.median(a)
                statstr += "distance along transect: %f\n\n" % self.transect_length
                message.append(statstr)
            except:
                pass

        stats = PlotStatsFrame(self, id=wx.ID_ANY, message=message, title=title)

        if stats.Show() == wx.ID_CLOSE:
            stats.Destroy()

    def OnCloseWindow(self, event):
        if self.controller.IsActive():
            self.controller.Stop()
        self.Destroy()


class ProfileToolbar(BaseToolbar):
    """Toolbar for profiling raster map"""

    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)

        # workaround for http://trac.wxwidgets.org/ticket/13888
        if sys.platform == "darwin":
            parent.SetToolBar(self)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        return self._getToolbarData(
            (
                ("addraster", BaseIcons["addRast"], self.parent.OnSelectRaster),
                ("transect", PlotIcons["transect"], self.parent.OnDrawTransect),
                (None,),
                ("draw", PlotIcons["draw"], self.parent.OnCreateProfile),
                ("erase", BaseIcons["erase"], self.parent.OnErase),
                ("drag", BaseIcons["pan"], self.parent.OnDrag),
                ("zoom", BaseIcons["zoomIn"], self.parent.OnZoom),
                ("unzoom", BaseIcons["zoomBack"], self.parent.OnRedraw),
                (None,),
                ("statistics", PlotIcons["statistics"], self.parent.OnStats),
                ("datasave", PlotIcons["save"], self.parent.SaveProfileToFile),
                ("image", BaseIcons["saveFile"], self.parent.SaveToFile),
                ("print", BaseIcons["print"], self.parent.PrintMenu),
                (None,),
                ("settings", PlotIcons["options"], self.parent.PlotOptionsMenu),
                ("quit", PlotIcons["quit"], self.parent.OnQuit),
            )
        )
