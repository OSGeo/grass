"""
@package iclass.frame

@brief wxIClass frame with toolbar for digitizing training areas and
for spectral signature analysis.

Classes:
 - frame::IClassMapFrame
 - frame::MapManager

(C) 2006-2013 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import six
import copy
import tempfile

import wx

from ctypes import *

try:
    from grass.lib.imagery import *
    from grass.lib.vector import *

    haveIClass = True
    errMsg = ""
except ImportError as e:
    haveIClass = False
    errMsg = _("Loading imagery lib failed.\n%s") % e

import grass.script as grass

from mapdisp import statusbar as sb
from mapwin.buffered import BufferedMapWindow
from vdigit.toolbars import VDigitToolbar
from gui_core.mapdisp import DoubleMapFrame
from core.render import Map
from core.gcmd import RunCommand, GMessage, GError
from gui_core.dialogs import SetOpacityDialog
from gui_core.wrap import Menu
from mapwin.base import MapWindowProperties
from dbmgr.vinfo import VectorDBInfo

from iclass.digit import IClassVDigitWindow, IClassVDigit
from iclass.toolbars import (
    IClassMapToolbar,
    IClassMiscToolbar,
    IClassToolbar,
    IClassMapManagerToolbar,
)
from iclass.statistics import StatisticsData
from iclass.dialogs import (
    IClassCategoryManagerDialog,
    IClassGroupDialog,
    IClassSignatureFileDialog,
    IClassExportAreasDialog,
    IClassMapDialog,
)
from iclass.plots import PlotPanel

from grass.pydispatch.signal import Signal


class IClassMapFrame(DoubleMapFrame):
    """wxIClass main frame

    It has two map windows one for digitizing training areas and one for
    result preview.
    It generates histograms, raster maps and signature files using
    @c I_iclass_* functions from C imagery library.

    It is wxGUI counterpart of old i.class module.
    """

    def __init__(
        self,
        parent=None,
        giface=None,
        title=_("Supervised Classification Tool"),
        toolbars=["iClassMisc", "iClassMap", "vdigit", "iClass"],
        size=(875, 600),
        name="IClassWindow",
        **kwargs,
    ):
        """
        :param parent: (no parent is expected)
        :param title: window title
        :param toolbars: dictionary of active toolbars (defalult value represents all toolbars)
        :param size: default size
        """
        DoubleMapFrame.__init__(
            self,
            parent=parent,
            title=title,
            name=name,
            firstMap=Map(),
            secondMap=Map(),
            **kwargs,
        )
        self._giface = giface
        self.tree = None
        self.mapWindowProperties = MapWindowProperties()
        self.mapWindowProperties.setValuesFromUserSettings()
        # show computation region by defaut
        self.mapWindowProperties.showRegion = True

        self.firstMapWindow = IClassVDigitWindow(
            parent=self,
            giface=self._giface,
            properties=self.mapWindowProperties,
            map=self.firstMap,
        )
        self.secondMapWindow = BufferedMapWindow(
            parent=self,
            giface=self._giface,
            properties=self.mapWindowProperties,
            Map=self.secondMap,
        )
        self.MapWindow = self.firstMapWindow  # current by default

        self._bindWindowsActivation()
        self._setUpMapWindow(self.firstMapWindow)
        self._setUpMapWindow(self.secondMapWindow)
        self.firstMapWindow.InitZoomHistory()
        self.secondMapWindow.InitZoomHistory()
        # TODO: for vdigit: it does nothing here because areas do not produce
        # this info
        self.firstMapWindow.digitizingInfo.connect(
            lambda text: self.statusbarManager.statusbarItems[
                "coordinates"
            ].SetAdditionalInfo(text)
        )
        self.firstMapWindow.digitizingInfoUnavailable.connect(
            lambda: self.statusbarManager.statusbarItems[
                "coordinates"
            ].SetAdditionalInfo(None)
        )
        self.SetSize(size)

        #
        # Signals
        #

        self.groupSet = Signal("IClassMapFrame.groupSet")
        self.categoryChanged = Signal("IClassMapFrame.categoryChanged")

        self.InitStatistics()

        #
        # Add toolbars
        #
        for toolb in toolbars:
            self.AddToolbar(toolb)
        self.firstMapWindow.SetToolbar(self.toolbars["vdigit"])

        self.GetMapToolbar().GetActiveMapTool().Bind(wx.EVT_CHOICE, self.OnUpdateActive)

        #
        # Add statusbar
        #

        # items for choice
        self.statusbarItems = [
            sb.SbCoordinates,
            sb.SbRegionExtent,
            sb.SbCompRegionExtent,
            sb.SbShowRegion,
            sb.SbAlignExtent,
            sb.SbResolution,
            sb.SbDisplayGeometry,
            sb.SbMapScale,
            sb.SbGoTo,
            sb.SbProjection,
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

        self.statusbarManager.Update()

        self.trainingMapManager = MapManager(
            self, mapWindow=self.GetFirstWindow(), Map=self.GetFirstMap()
        )
        self.previewMapManager = MapManager(
            self, mapWindow=self.GetSecondWindow(), Map=self.GetSecondMap()
        )

        self.changes = False
        self.exportVector = None

        # dialogs
        self.dialogs = dict()
        self.dialogs["classManager"] = None
        self.dialogs["scatt_plot"] = None
        # just to make digitizer happy
        self.dialogs["attributes"] = None
        self.dialogs["category"] = None

        # PyPlot init
        self.plotPanel = PlotPanel(
            self, giface=self._giface, stats_data=self.stats_data
        )

        self._addPanes()
        self._mgr.Update()

        self.trainingMapManager.SetToolbar(self.toolbars["iClassTrainingMapManager"])
        self.previewMapManager.SetToolbar(self.toolbars["iClassPreviewMapManager"])

        # default action
        self.GetMapToolbar().SelectDefault()

        wx.CallAfter(self.AddTrainingAreaMap)

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        self.Bind(wx.EVT_SIZE, self.OnSize)

        self.SendSizeEvent()

    def OnCloseWindow(self, event):
        self.GetFirstWindow().GetDigit().CloseMap()
        self.plotPanel.CloseWindow()
        self._cleanup()
        self._mgr.UnInit()
        self.Destroy()

    def _cleanup(self):
        """Frees C structs and removes vector map and all raster maps."""
        I_free_signatures(self.signatures)
        I_free_group_ref(self.refer)
        for st in self.cStatisticsDict.values():
            I_iclass_free_statistics(st)

        self.RemoveTempVector()
        for i in self.stats_data.GetCategories():
            self.RemoveTempRaster(self.stats_data.GetStatistics(i).rasterName)

    def OnHelp(self, event):
        """Show help page"""
        self._giface.Help(entry="wxGUI.iclass")

    def _getTempVectorName(self):
        """Return new name for temporary vector map (training areas)"""
        vectorPath = grass.tempfile(create=False)

        return "trAreas" + os.path.basename(vectorPath).replace(".", "")

    def SetGroup(self, group, subgroup):
        """Set group and subgroup manually"""
        self.g = {"group": group, "subgroup": subgroup}

    def CreateTempVector(self):
        """Create temporary vector map for training areas"""
        vectorName = self._getTempVectorName()

        env = os.environ.copy()
        env["GRASS_VECTOR_TEMPORARY"] = "1"  # create temporary map
        cmd = ("v.edit", {"tool": "create", "map": vectorName})

        ret = RunCommand(prog=cmd[0], parent=self, env=env, **cmd[1])

        if ret != 0:
            return False

        return vectorName

    def RemoveTempVector(self):
        """Removes temporary vector map with training areas"""
        ret = RunCommand(
            prog="g.remove",
            parent=self,
            flags="f",
            type="vector",
            name=self.trainingAreaVector,
        )
        if ret != 0:
            return False
        return True

    def RemoveTempRaster(self, raster):
        """Removes temporary raster maps"""
        self.GetFirstMap().Clean()
        self.GetSecondMap().Clean()
        ret = RunCommand(
            prog="g.remove", parent=self, flags="f", type="raster", name=raster
        )
        if ret != 0:
            return False
        return True

    def AddToolbar(self, name):
        """Add defined toolbar to the window

        Currently known toolbars are:
         - 'iClassMap'          - basic map toolbar
         - 'iClass'             - iclass tools
         - 'iClassMisc'         - miscellaneous (help)
         - 'vdigit'             - digitizer toolbar (areas)

         Toolbars 'iClassPreviewMapManager' are added in _addPanes().
        """
        if name == "iClassMap":
            self.toolbars[name] = IClassMapToolbar(self, self._toolSwitcher)

            self._mgr.AddPane(
                self.toolbars[name],
                wx.aui.AuiPaneInfo()
                .Name(name)
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
                .Position(0)
                .BestSize((self.toolbars[name].GetBestSize())),
            )

        if name == "iClass":
            self.toolbars[name] = IClassToolbar(self, stats_data=self.stats_data)

            self._mgr.AddPane(
                self.toolbars[name],
                wx.aui.AuiPaneInfo()
                .Name(name)
                .Caption(_("IClass Toolbar"))
                .ToolbarPane()
                .Top()
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2)
                .Row(2)
                .Position(0)
                .BestSize((self.toolbars[name].GetBestSize())),
            )

        if name == "iClassMisc":
            self.toolbars[name] = IClassMiscToolbar(self)

            self._mgr.AddPane(
                self.toolbars[name],
                wx.aui.AuiPaneInfo()
                .Name(name)
                .Caption(_("IClass Misc Toolbar"))
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
                .BestSize((self.toolbars[name].GetBestSize())),
            )

        if name == "vdigit":
            self.toolbars[name] = VDigitToolbar(
                parent=self,
                toolSwitcher=self._toolSwitcher,
                MapWindow=self.GetFirstWindow(),
                digitClass=IClassVDigit,
                giface=self._giface,
                tools=[
                    "addArea",
                    "moveVertex",
                    "addVertex",
                    "removeVertex",
                    "editLine",
                    "moveLine",
                    "deleteArea",
                    "undo",
                    "redo",
                    "settings",
                ],
            )
            self._mgr.AddPane(
                self.toolbars[name],
                wx.aui.AuiPaneInfo()
                .Name(name)
                .Caption(_("Digitization Toolbar"))
                .ToolbarPane()
                .Top()
                .LeftDockable(False)
                .RightDockable(False)
                .BottomDockable(False)
                .TopDockable(True)
                .CloseButton(False)
                .Layer(2)
                .Row(2)
                .Position(1)
                .BestSize((self.toolbars[name].GetBestSize())),
            )

    def _addPanes(self):
        """Add mapwindows and toolbars to aui manager"""
        self._addPaneMapWindow(name="training", position=0)
        self._addPaneToolbar(name="iClassTrainingMapManager", position=1)
        self._addPaneMapWindow(name="preview", position=2)
        self._addPaneToolbar(name="iClassPreviewMapManager", position=3)

        # otherwise best size was ignored
        self._mgr.SetDockSizeConstraint(0.5, 0.5)

        self._mgr.AddPane(
            self.plotPanel,
            wx.aui.AuiPaneInfo()
            .Name("plots")
            .Caption(_("Plots"))
            .Dockable(False)
            .Floatable(False)
            .CloseButton(False)
            .Left()
            .Layer(1)
            .BestSize((335, -1)),
        )

    def _addPaneToolbar(self, name, position):
        if name == "iClassPreviewMapManager":
            parent = self.previewMapManager
        else:
            parent = self.trainingMapManager

        self.toolbars[name] = IClassMapManagerToolbar(self, parent)
        self._mgr.AddPane(
            self.toolbars[name],
            wx.aui.AuiPaneInfo()
            .ToolbarPane()
            .Movable()
            .Name(name)
            .CloseButton(False)
            .Center()
            .Layer(0)
            .Position(position)
            .BestSize((self.toolbars[name].GetBestSize())),
        )

    def _addPaneMapWindow(self, name, position):
        if name == "preview":
            window = self.GetSecondWindow()
            caption = _("Preview Display")
        else:
            window = self.GetFirstWindow()
            caption = _("Training Areas Display")

        self._mgr.AddPane(
            window,
            wx.aui.AuiPaneInfo()
            .Name(name)
            .Caption(caption)
            .Dockable(False)
            .Floatable(False)
            .CloseButton(False)
            .Center()
            .Layer(0)
            .Position(position),
        )

    def IsStandalone(self):
        """Check if Map display is standalone"""
        return True

    def OnUpdateActive(self, event):
        """
        .. todo::
            move to DoubleMapFrame?
        """
        if self.GetMapToolbar().GetActiveMap() == 0:
            self.MapWindow = self.firstMapWindow
            self.Map = self.firstMap
        else:
            self.MapWindow = self.secondMapWindow
            self.Map = self.secondMap

        self.UpdateActive(self.MapWindow)
        # for wingrass
        if os.name == "nt":
            self.MapWindow.SetFocus()

    def UpdateActive(self, win):
        """
        .. todo::
            move to DoubleMapFrame?
        """
        mapTb = self.GetMapToolbar()
        # optionally disable tool zoomback tool
        mapTb.Enable("zoomBack", enable=(len(self.MapWindow.zoomhistory) > 1))

        if mapTb.GetActiveMap() != (win == self.secondMapWindow):
            mapTb.SetActiveMap((win == self.secondMapWindow))
        self.StatusbarUpdate()

    def ActivateFirstMap(self, event=None):
        DoubleMapFrame.ActivateFirstMap(self, event)
        self.GetMapToolbar().Enable(
            "zoomBack", enable=(len(self.MapWindow.zoomhistory) > 1)
        )

    def ActivateSecondMap(self, event=None):
        DoubleMapFrame.ActivateSecondMap(self, event)
        self.GetMapToolbar().Enable(
            "zoomBack", enable=(len(self.MapWindow.zoomhistory) > 1)
        )

    def GetMapToolbar(self):
        """Returns toolbar with zooming tools"""
        return self.toolbars["iClassMap"] if "iClassMap" in self.toolbars else None

    def GetClassColor(self, cat):
        """Get class color as string

        :param cat: class category

        :return: 'R:G:B'
        """
        if cat in self.stats_data.GetCategories():
            return self.stats_data.GetStatistics(cat).color
        return "0:0:0"

    def OnZoomMenu(self, event):
        """Popup Zoom menu"""
        zoommenu = Menu()
        # Add items to the menu

        i = 0
        for label, handler in (
            (
                _("Adjust Training Area Display to Preview Display"),
                self.OnZoomToPreview,
            ),
            (
                _("Adjust Preview display to Training Area Display"),
                self.OnZoomToTraining,
            ),
            (_("Display synchronization ON"), lambda event: self.SetBindRegions(True)),
            (
                _("Display synchronization OFF"),
                lambda event: self.SetBindRegions(False),
            ),
        ):
            if label is None:
                zoommenu.AppendSeparator()
                continue

            item = wx.MenuItem(zoommenu, wx.ID_ANY, label)
            zoommenu.AppendItem(item)
            self.Bind(wx.EVT_MENU, handler, item)
            if i == 3:
                item.Enable(not self._bindRegions)
            elif i == 4:
                item.Enable(self._bindRegions)
            i += 1

        # Popup the menu. If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(zoommenu)
        zoommenu.Destroy()

    def OnZoomToTraining(self, event):
        """Set preview display to match extents of training display"""

        if not self.MapWindow == self.GetSecondWindow():
            self.MapWindow = self.GetSecondWindow()
            self.Map = self.GetSecondMap()
            self.UpdateActive(self.GetSecondWindow())

        newreg = self.firstMap.GetCurrentRegion()
        self.GetSecondMap().region = copy.copy(newreg)

        self.Render(self.GetSecondWindow())

    def OnZoomToPreview(self, event):
        """Set preview display to match extents of training display"""

        if not self.MapWindow == self.GetFirstWindow():
            self.MapWindow = self.GetFirstWindow()
            self.Map = self.GetFirstMap()
            self.UpdateActive(self.GetFirstWindow())

        newreg = self.GetSecondMap().GetCurrentRegion()
        self.GetFirstMap().region = copy.copy(newreg)

        self.Render(self.GetFirstWindow())

    def AddBands(self):
        """Add imagery group"""
        dlg = IClassGroupDialog(self, group=self.g["group"])

        while True:
            if dlg.ShowModal() == wx.ID_OK:

                if dlg.GetGroupBandsErr(parent=self):
                    g, s = dlg.GetData()
                    group = grass.find_file(name=g, element="group")
                    self.g["group"] = group["name"]
                    self.g["subgroup"] = s
                    self.groupSet.emit(
                        group=self.g["group"], subgroup=self.g["subgroup"]
                    )
                    break
            else:
                break

        dlg.Destroy()

    def OnImportAreas(self, event):
        """Import training areas"""
        # check if we have any changes
        if self.GetAreasCount() or self.stats_data.GetCategories():
            qdlg = wx.MessageDialog(
                parent=self,
                message=_("All changes will be lost. " "Do you want to continue?"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE,
            )
            if qdlg.ShowModal() == wx.ID_NO:
                qdlg.Destroy()
                return
            qdlg.Destroy()

        dlg = IClassMapDialog(self, title=_("Import vector map"), element="vector")
        if dlg.ShowModal() == wx.ID_OK:
            vName = dlg.GetMap()

            self.ImportAreas(vName)

        dlg.Destroy()

    def _checkImportedTopo(self, vector):
        """Check if imported vector map has areas

        :param str vector: vector map name

        :return: warning message (empty if topology is ok)
        """
        topo = grass.vector_info_topo(map=vector)

        warning = ""
        if topo["areas"] == 0:
            warning = _("No areas in vector map <%s>.\n" % vector)
        if topo["points"] or topo["lines"]:
            warning += _(
                "Vector map <%s> contains points or lines, "
                "these features are ignored." % vector
            )

        return warning

    def ImportAreas(self, vector):
        """Import training areas.

        If table connected, try load certain columns to class manager

        :param str vector: vector map name
        """
        warning = self._checkImportedTopo(vector)
        if warning:
            GMessage(parent=self, message=warning)
            return

        wx.BeginBusyCursor()
        wx.GetApp().Yield()

        # close, build, copy and open again the temporary vector
        digitClass = self.GetFirstWindow().GetDigit()

        # open vector map to be imported
        if digitClass.OpenMap(vector, update=False) is None:
            GError(parent=self, message=_("Unable to open vector map <%s>") % vector)
            return

        # copy features to the temporary map
        vname = self._getTempVectorName()
        # avoid deleting temporary map
        os.environ["GRASS_VECTOR_TEMPORARY"] = "1"
        if digitClass.CopyMap(vname, tmp=True, update=True) == -1:
            GError(
                parent=self,
                message=_("Unable to copy vector features from <%s>") % vector,
            )
            return
        del os.environ["GRASS_VECTOR_TEMPORARY"]

        # close map
        digitClass.CloseMap()

        # open temporary map (copy of imported map)
        self.poMapInfo = digitClass.OpenMap(vname, tmp=True)
        if self.poMapInfo is None:
            GError(parent=self, message=_("Unable to open temporary vector map"))
            return

        # remove temporary rasters
        for cat in self.stats_data.GetCategories():
            self.RemoveTempRaster(self.stats_data.GetStatistics(cat).rasterName)

        # clear current statistics
        self.stats_data.DeleteAllStatistics()

        # reset plots
        self.plotPanel.Reset()

        self.GetFirstWindow().UpdateMap(render=False, renderVector=True)

        self.ImportClasses(vector)

        # should be saved in attribute table?
        self.toolbars["iClass"].UpdateStddev(1.5)

        wx.EndBusyCursor()

        return True

    def ImportClasses(self, vector):
        """If imported map has table, try to import certain columns to class manager"""
        # check connection
        dbInfo = VectorDBInfo(vector)
        connected = len(dbInfo.layers.keys()) > 0

        # remove attribute table of temporary vector, we don't need it
        if connected:
            RunCommand("v.db.droptable", flags="f", map=self.trainingAreaVector)

        # we use first layer with table, TODO: user should choose
        layer = None
        for key in dbInfo.layers.keys():
            if dbInfo.GetTable(key):
                layer = key

        # get columns to check if we can use them
        # TODO: let user choose which columns mean what
        if layer is not None:
            columns = dbInfo.GetColumns(table=dbInfo.GetTable(layer))
        else:
            columns = []

        # get class manager
        if self.dialogs["classManager"] is None:
            self.dialogs["classManager"] = IClassCategoryManagerDialog(self)

        listCtrl = self.dialogs["classManager"].GetListCtrl()

        # unable to load data (no connection, table, right columns)
        if (
            not connected
            or layer is None
            or "class" not in columns
            or "color" not in columns
        ):
            # no table connected
            cats = RunCommand(
                "v.category",
                input=vector,
                layer=1,  # set layer?
                # type = ['centroid', 'area'] ?
                option="print",
                read=True,
            )
            cats = map(int, cats.strip().split())
            cats = sorted(list(set(cats)))

            for cat in cats:
                listCtrl.AddCategory(cat=cat, name="class_%d" % cat, color="0:0:0")
        # connection, table and columns exists
        else:
            columns = ["cat", "class", "color"]
            ret = RunCommand(
                "v.db.select",
                quiet=True,
                parent=self,
                flags="c",
                map=vector,
                layer=1,
                columns=",".join(columns),
                read=True,
            )
            records = ret.strip().splitlines()
            for record in records:
                record = record.split("|")
                listCtrl.AddCategory(
                    cat=int(record[0]), name=record[1], color=record[2]
                )

    def OnExportAreas(self, event):
        """Export training areas"""
        if self.GetAreasCount() == 0:
            GMessage(parent=self, message=_("No training areas to export."))
            return

        dlg = IClassExportAreasDialog(self, vectorName=self.exportVector)

        if dlg.ShowModal() == wx.ID_OK:
            vName = dlg.GetVectorName()
            self.exportVector = vName
            withTable = dlg.WithTable()
            dlg.Destroy()

            if self.ExportAreas(vectorName=vName, withTable=withTable):
                GMessage(
                    _("%d training areas (%d classes) exported to vector map <%s>.")
                    % (
                        self.GetAreasCount(),
                        len(self.stats_data.GetCategories()),
                        self.exportVector,
                    ),
                    parent=self,
                )

    def ExportAreas(self, vectorName, withTable):
        """Export training areas to new vector map (with attribute table).

        :param str vectorName: name of exported vector map
        :param bool withTable: true if attribute table is required
        """
        wx.BeginBusyCursor()
        wx.GetApp().Yield()

        # close, build, copy and open again the temporary vector
        digitClass = self.GetFirstWindow().GetDigit()
        if "@" in vectorName:
            vectorName = vectorName.split("@")[0]
        if digitClass.CopyMap(vectorName) < 0:
            return False

        if not withTable:
            wx.EndBusyCursor()
            return False

        # add new table
        columns = [
            "class varchar(30)",
            "color varchar(11)",
            "n_cells integer",
        ]

        nbands = len(self.GetGroupLayers(self.g["group"], self.g["subgroup"]))
        for statistic, format in (
            ("min", "integer"),
            ("mean", "double precision"),
            ("max", "integer"),
        ):
            for i in range(nbands):
                # 10 characters limit?
                columns.append(
                    "band%(band)d_%(stat)s %(format)s"
                    % {"band": i + 1, "stat": statistic, "format": format}
                )

        if 0 != RunCommand(
            "v.db.addtable", map=vectorName, columns=columns, parent=self
        ):
            wx.EndBusyCursor()
            return False

        try:
            dbInfo = grass.vector_db(vectorName)[1]
        except KeyError:
            wx.EndBusyCursor()
            return False

        dbFile = tempfile.NamedTemporaryFile(mode="w", delete=False)
        if dbInfo["driver"] != "dbf":
            dbFile.write("BEGIN\n")
        # populate table
        for cat in self.stats_data.GetCategories():
            stat = self.stats_data.GetStatistics(cat)

            self._runDBUpdate(
                dbFile, table=dbInfo["table"], column="class", value=stat.name, cat=cat
            )
            self._runDBUpdate(
                dbFile, table=dbInfo["table"], column="color", value=stat.color, cat=cat
            )

            if not stat.IsReady():
                continue

            self._runDBUpdate(
                dbFile,
                table=dbInfo["table"],
                column="n_cells",
                value=stat.ncells,
                cat=cat,
            )

            for i in range(nbands):
                self._runDBUpdate(
                    dbFile,
                    table=dbInfo["table"],
                    column="band%d_min" % (i + 1),
                    value=stat.bands[i].min,
                    cat=cat,
                )
                self._runDBUpdate(
                    dbFile,
                    table=dbInfo["table"],
                    column="band%d_mean" % (i + 1),
                    value=stat.bands[i].mean,
                    cat=cat,
                )
                self._runDBUpdate(
                    dbFile,
                    table=dbInfo["table"],
                    column="band%d_max" % (i + 1),
                    value=stat.bands[i].max,
                    cat=cat,
                )

        if dbInfo["driver"] != "dbf":
            dbFile.write("COMMIT\n")
        dbFile.file.close()

        ret = RunCommand(
            "db.execute",
            input=dbFile.name,
            driver=dbInfo["driver"],
            database=dbInfo["database"],
        )
        wx.EndBusyCursor()
        os.remove(dbFile.name)
        if ret != 0:
            return False
        return True

    def _runDBUpdate(self, tmpFile, table, column, value, cat):
        """Helper function for UPDATE statement

        :param tmpFile: file where to write UPDATE statements
        :param table: table name
        :param column: name of updated column
        :param value: new value
        :param cat: which category to update
        """
        if isinstance(value, (int, float)):
            tmpFile.write(
                "UPDATE %s SET %s = %d WHERE cat = %d\n" % (table, column, value, cat)
            )
        else:
            tmpFile.write(
                "UPDATE %s SET %s = '%s' WHERE cat = %d\n" % (table, column, value, cat)
            )

    def OnCategoryManager(self, event):
        """Show category management dialog"""
        if self.dialogs["classManager"] is None:
            dlg = IClassCategoryManagerDialog(self)
            dlg.CenterOnParent()
            dlg.Show()
            self.dialogs["classManager"] = dlg
        else:
            if not self.dialogs["classManager"].IsShown():
                self.dialogs["classManager"].Show()

    def CategoryChanged(self, currentCat):
        """Updates everything which depends on current category.

        Updates number of stddev, histograms, layer in preview display.
        """
        if currentCat:
            stat = self.stats_data.GetStatistics(currentCat)
            nstd = stat.nstd
            self.toolbars["iClass"].UpdateStddev(nstd)

            self.plotPanel.UpdateCategory(currentCat)
            self.plotPanel.OnPlotTypeSelected(None)

            name = stat.rasterName
            name = self.previewMapManager.GetAlias(name)
            if name:
                self.previewMapManager.SelectLayer(name)

        self.categoryChanged.emit(cat=currentCat)

    def DeleteAreas(self, cats):
        """Removes all training areas of given categories

        :param cats: list of categories to be deleted
        """
        self.firstMapWindow.GetDigit().DeleteAreasByCat(cats)
        self.firstMapWindow.UpdateMap(render=False, renderVector=True)

    def HighlightCategory(self, cats):
        """Highlight araes given by category"""
        self.firstMapWindow.GetDigit().GetDisplay().SetSelected(cats, layer=1)
        self.firstMapWindow.UpdateMap(render=False, renderVector=True)

    def ZoomToAreasByCat(self, cat):
        """Zoom to areas given by category"""
        n, s, w, e = self.GetFirstWindow().GetDigit().GetDisplay().GetRegionSelected()
        self.GetFirstMap().GetRegion(n=n, s=s, w=w, e=e, update=True)
        self.GetFirstMap().AdjustRegion()
        self.GetFirstMap().AlignExtentFromDisplay()

        self.GetFirstWindow().UpdateMap(render=True, renderVector=True)

    def UpdateRasterName(self, newName, cat):
        """Update alias of raster map when category name is changed"""
        origName = self.stats_data.GetStatistics(cat).rasterName
        self.previewMapManager.SetAlias(origName, self._addSuffix(newName))

    def StddevChanged(self, cat, nstd):
        """Standard deviation multiplier changed, rerender map, histograms"""
        stat = self.stats_data.GetStatistics(cat)
        stat.SetStatistics({"nstd": nstd})

        if not stat.IsReady():
            return

        raster = stat.rasterName

        cstat = self.cStatisticsDict[cat]
        I_iclass_statistics_set_nstd(cstat, nstd)

        I_iclass_create_raster(cstat, self.refer, raster)
        self.Render(self.GetSecondWindow())

        stat.SetBandStatistics(cstat)
        self.plotPanel.StddevChanged()

    def UpdateChangeState(self, changes):
        """Informs if any important changes happened
        since last analysis computation.
        """
        self.changes = changes

    def AddRasterMap(self, name, firstMap=True, secondMap=True):
        """Add raster map to Map"""
        cmdlist = ["d.rast", "map=%s" % name]
        if firstMap:
            self.GetFirstMap().AddLayer(
                ltype="raster",
                command=cmdlist,
                active=True,
                name=name,
                hidden=False,
                opacity=1.0,
                render=False,
            )
            self.Render(self.GetFirstWindow())
        if secondMap:
            self.GetSecondMap().AddLayer(
                ltype="raster",
                command=cmdlist,
                active=True,
                name=name,
                hidden=False,
                opacity=1.0,
                render=False,
            )
            self.Render(self.GetSecondWindow())

    def AddTrainingAreaMap(self):
        """Add vector map with training areas to Map (training
        sub-display)"""
        vname = self.CreateTempVector()
        if vname:
            self.trainingAreaVector = vname
        else:
            GMessage(parent=self, message=_("Failed to create temporary vector map."))
            return

        # use 'hidden' for temporary maps (TODO: do it better)
        mapLayer = self.GetFirstMap().AddLayer(
            ltype="vector",
            command=["d.vect", "map=%s" % vname],
            name=vname,
            active=False,
            hidden=True,
        )

        self.toolbars["vdigit"].StartEditing(mapLayer)
        self.poMapInfo = self.GetFirstWindow().GetDigit().GetMapInfo()
        self.Render(self.GetFirstWindow())

    def OnRunAnalysis(self, event):
        """Run analysis and update plots"""
        if self.RunAnalysis():
            currentCat = self.GetCurrentCategoryIdx()
            self.plotPanel.UpdatePlots(
                group=self.g["group"],
                subgroup=self.g["subgroup"],
                currentCat=currentCat,
                stats_data=self.stats_data,
            )

    def RunAnalysis(self):
        """Run analysis

        Calls C functions to compute all statistics and creates raster maps.
        Signatures are created but signature file is not.
        """
        if not self.CheckInput(group=self.g["group"], vector=self.trainingAreaVector):
            return

        for statistic in self.cStatisticsDict.values():
            I_iclass_free_statistics(statistic)
        self.cStatisticsDict = {}

        # init Ref struct with the files in group */
        I_free_group_ref(self.refer)

        if not I_iclass_init_group(self.g["group"], self.g["subgroup"], self.refer):
            return False

        I_free_signatures(self.signatures)
        I_iclass_init_signatures(self.signatures, self.refer)

        # why create copy
        # cats = self.statisticsList[:]

        cats = self.stats_data.GetCategories()
        for i in cats:
            stats = self.stats_data.GetStatistics(i)

            statistics_obj = IClass_statistics()
            statistics = pointer(statistics_obj)

            I_iclass_init_statistics(
                statistics, stats.category, stats.name, stats.color, stats.nstd
            )

            ret = I_iclass_analysis(
                statistics,
                self.refer,
                self.poMapInfo,
                "1",
                self.g["group"],
                stats.rasterName,
            )
            if ret > 0:
                # tests
                self.cStatisticsDict[i] = statistics

                stats.SetFromcStatistics(statistics)
                stats.SetReady()

                # stat is already part of stats_data?
                # self.statisticsDict[stats.category] = stats

                self.ConvertToNull(name=stats.rasterName)
                self.previewMapManager.AddLayer(
                    name=stats.rasterName,
                    alias=self._addSuffix(stats.name),
                    resultsLayer=True,
                )
                # write statistics
                I_iclass_add_signature(self.signatures, statistics)

            elif ret == 0:
                GMessage(
                    parent=self,
                    message=_("No area in category %s. Category skipped.")
                    % stats.category,
                )
                I_iclass_free_statistics(statistics)
            else:
                GMessage(parent=self, message=_("Analysis failed."))
                I_iclass_free_statistics(statistics)

        self.UpdateChangeState(changes=False)
        return True

    def _addSuffix(self, name):
        suffix = _("results")
        return "_".join((name, suffix))

    def OnSaveSigFile(self, event):
        """Asks for signature file name and saves it."""
        if not self.g["group"]:
            GMessage(parent=self, message=_("No imagery group selected."))
            return

        if self.changes:
            qdlg = wx.MessageDialog(
                parent=self,
                message=_(
                    "Due to recent changes in classes, "
                    "signatures can be outdated and should be recalculated. "
                    "Do you still want to continue?"
                ),
                caption=_("Outdated signatures"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE,
            )
            if qdlg.ShowModal() == wx.ID_YES:
                qdlg.Destroy()
            else:
                qdlg.Destroy()
                return

        dlg = IClassSignatureFileDialog(
            self, group=self.g["group"], subgroup=self.g["subgroup"], file=self.sigFile
        )

        if dlg.ShowModal() == wx.ID_OK:
            if os.path.exists(dlg.GetFileName(fullPath=True)):
                qdlg = wx.MessageDialog(
                    parent=self,
                    message=_(
                        "A signature file named %s already exists.\n"
                        "Do you want to replace it?"
                    )
                    % dlg.GetFileName(),
                    caption=_("File already exists"),
                    style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION | wx.CENTRE,
                )
                if qdlg.ShowModal() == wx.ID_YES:
                    qdlg.Destroy()
                else:
                    qdlg.Destroy()
                    return
            self.sigFile = dlg.GetFileName()
            self.WriteSignatures(
                self.signatures, self.g["group"], self.g["subgroup"], self.sigFile
            )

        dlg.Destroy()

    def InitStatistics(self):
        """Initialize variables and c structures necessary for
        computing statistics.
        """
        self.g = {"group": None, "subgroup": None}
        self.sigFile = None

        self.stats_data = StatisticsData()

        self.cStatisticsDict = {}

        self.signatures_obj = Signature()
        self.signatures = pointer(self.signatures_obj)
        I_init_signatures(self.signatures, 0)  # must be freed on exit

        refer_obj = Ref()
        self.refer = pointer(refer_obj)
        I_init_group_ref(self.refer)  # must be freed on exit

    def WriteSignatures(self, signatures, group, subgroup, filename):
        """Writes current signatures to signature file

        :param signatures: signature (c structure)
        :param group: imagery group
        :param filename: signature file name
        """
        I_iclass_write_signatures(signatures, group, subgroup, filename)

    def CheckInput(self, group, vector):
        """Check if input is valid"""
        # check if group is ok
        # TODO check subgroup
        if not group:
            GMessage(
                parent=self,
                message=_("No imagery group selected. " "Operation canceled."),
            )
            return False

        groupLayers = self.GetGroupLayers(self.g["group"], self.g["subgroup"])

        nLayers = len(groupLayers)
        if nLayers <= 1:
            GMessage(
                parent=self,
                message=_(
                    "Group <%(group)s> does not have enough files "
                    "(it has %(files)d files). Operation canceled."
                )
                % {"group": group, "files": nLayers},
            )
            return False

        # check if vector has any areas
        if self.GetAreasCount() == 0:
            GMessage(parent=self, message=_("No areas given. " "Operation canceled."))
            return False

        # check if vector is inside raster
        regionBox = bound_box()
        Vect_get_map_box(self.poMapInfo, byref(regionBox))

        rasterInfo = grass.raster_info(groupLayers[0])

        if (
            regionBox.N > rasterInfo["north"]
            or regionBox.S < rasterInfo["south"]
            or regionBox.E > rasterInfo["east"]
            or regionBox.W < rasterInfo["west"]
        ):
            GMessage(
                parent=self,
                message=_(
                    "Vector features are outside raster layers. " "Operation canceled."
                ),
            )
            return False

        return True

    def GetAreasCount(self):
        """Returns number of not dead areas"""
        count = 0
        numAreas = Vect_get_num_areas(self.poMapInfo)
        for i in range(numAreas):
            if Vect_area_alive(self.poMapInfo, i + 1):
                count += 1
        return count

    def GetGroupLayers(self, group, subgroup=None):
        """Get layers in subgroup (expecting same name for group and subgroup)

        .. todo::
            consider moving this function to core module for convenient
        """
        kwargs = {}
        if subgroup:
            kwargs["subgroup"] = subgroup

        res = RunCommand("i.group", flags="g", group=group, read=True, **kwargs).strip()
        if res.splitlines()[0]:
            return sorted(res.splitlines())

        return []

    def ConvertToNull(self, name):
        """Sets value which represents null values for given raster map.

        :param name: raster map name
        """
        RunCommand("r.null", map=name, setnull=0)

    def GetCurrentCategoryIdx(self):
        """Returns current category number"""
        return self.toolbars["iClass"].GetSelectedCategoryIdx()

    def OnZoomIn(self, event):
        """Enable zooming for plots"""
        super(IClassMapFrame, self).OnZoomIn(event)
        self.plotPanel.EnableZoom(type=1)

    def OnZoomOut(self, event):
        """Enable zooming for plots"""
        super(IClassMapFrame, self).OnZoomOut(event)
        self.plotPanel.EnableZoom(type=-1)

    def OnPan(self, event):
        """Enable panning for plots"""
        super(IClassMapFrame, self).OnPan(event)
        self.plotPanel.EnablePan()

    def OnPointer(self, event):
        """Set pointer mode.

        .. todo::
            pointers need refactoring
        """
        self.GetFirstWindow().SetModePointer()
        self.GetSecondWindow().SetModePointer()

    def GetMapManagers(self):
        """Get map managers of wxIClass

        :return: trainingMapManager, previewMapManager
        """
        return self.trainingMapManager, self.previewMapManager


class MapManager:
    """Class for managing map renderer.

    It is connected with iClassMapManagerToolbar.
    """

    def __init__(self, frame, mapWindow, Map):
        """

        It is expected that \a mapWindow is connected with \a Map.

        :param frame: application main window
        :param mapWindow: map window instance
        :param map: map renderer instance
        """
        self.map = Map
        self.frame = frame
        self.mapWindow = mapWindow
        self.toolbar = None

        self.layerName = {}

    def SetToolbar(self, toolbar):
        self.toolbar = toolbar

    def AddLayer(self, name, alias=None, resultsLayer=False):
        """Adds layer to Map and update toolbar

        :param str name: layer (raster) name
        :param str resultsLayer: True if layer is temp. raster showing the results of computation
        """
        if resultsLayer and name in [
            layer.GetName() for layer in self.map.GetListOfLayers(name=name)
        ]:
            self.frame.Render(self.mapWindow)
            return

        cmdlist = ["d.rast", "map=%s" % name]
        self.map.AddLayer(
            ltype="raster",
            command=cmdlist,
            active=True,
            name=name,
            hidden=False,
            opacity=1.0,
            render=True,
        )
        self.frame.Render(self.mapWindow)

        if alias is not None:
            self.layerName[alias] = name
            name = alias
        else:
            self.layerName[name] = name

        self.toolbar.choice.Insert(name, 0)
        self.toolbar.choice.SetSelection(0)

    def AddLayerRGB(self, cmd):
        """Adds RGB layer and update toolbar.

        :param cmd: d.rgb command as a list
        """
        name = []
        for param in cmd:
            if "=" in param:
                name.append(param.split("=")[1])
        name = ",".join(name)
        self.map.AddLayer(
            ltype="rgb",
            command=cmd,
            active=True,
            name=name,
            hidden=False,
            opacity=1.0,
            render=True,
        )
        self.frame.Render(self.mapWindow)
        self.layerName[name] = name
        self.toolbar.choice.Insert(name, 0)
        self.toolbar.choice.SetSelection(0)

    def RemoveTemporaryLayer(self, name):
        """Removes temporary layer (if exists) from Map and and updates toolbar.

        :param name: real name of layer
        """
        # check if layer is loaded
        layers = self.map.GetListOfLayers(ltype="raster")
        idx = None
        for i, layer in enumerate(layers):
            if name == layer.GetName():
                idx = i
                break
        if idx is None:
            return
        # remove it from Map
        self.map.RemoveLayer(name=name)

        # update inner list of layers
        alias = self.GetAlias(name)
        if alias not in self.layerName:
            return

        del self.layerName[alias]
        # update choice
        idx = self.toolbar.choice.FindString(alias)
        if idx != wx.NOT_FOUND:
            self.toolbar.choice.Delete(idx)
            if not self.toolbar.choice.IsEmpty():
                self.toolbar.choice.SetSelection(0)

        self.frame.Render(self.mapWindow)

    def Render(self):
        """
        .. todo::
            giface shoud be used instead of this method"""
        self.frame.Render(self.mapWindow)

    def RemoveLayer(self, name, idx):
        """Removes layer from Map and update toolbar"""
        name = self.layerName[name]
        self.map.RemoveLayer(name=name)
        del self.layerName[name]
        self.toolbar.choice.Delete(idx)
        if not self.toolbar.choice.IsEmpty():
            self.toolbar.choice.SetSelection(0)

        self.frame.Render(self.mapWindow)

    def SelectLayer(self, name):
        """Moves selected layer to top"""
        layers = self.map.GetListOfLayers(ltype="rgb") + self.map.GetListOfLayers(
            ltype="raster"
        )
        idx = None
        for i, layer in enumerate(layers):
            if self.layerName[name] == layer.GetName():
                idx = i
                break

        if idx is not None:  # should not happen
            layers.append(layers.pop(idx))

            choice = self.toolbar.choice
            idx = choice.FindString(name)
            choice.Delete(idx)
            choice.Insert(name, 0)
            choice.SetSelection(0)

            # layers.reverse()
            self.map.SetLayers(layers)
            self.frame.Render(self.mapWindow)

    def SetOpacity(self, name):
        """Sets opacity of layers."""
        name = self.layerName[name]
        layers = self.map.GetListOfLayers(name=name)
        if not layers:
            return

        # works for first layer only
        oldOpacity = layers[0].GetOpacity()
        dlg = SetOpacityDialog(self.frame, opacity=oldOpacity)
        dlg.applyOpacity.connect(
            lambda value: self._changeOpacity(layer=layers[0], opacity=value)
        )

        if dlg.ShowModal() == wx.ID_OK:
            self._changeOpacity(layer=layers[0], opacity=dlg.GetOpacity())

        dlg.Destroy()

    def _changeOpacity(self, layer, opacity):
        self.map.ChangeOpacity(layer=layer, opacity=opacity)
        self.frame.Render(self.mapWindow)

    def GetAlias(self, name):
        """Returns alias for layer"""
        name = [k for k, v in six.iteritems(self.layerName) if v == name]
        if name:
            return name[0]
        return None

    def SetAlias(self, original, alias):
        name = self.GetAlias(original)
        if name:
            self.layerName[alias] = original
            del self.layerName[name]
            idx = self.toolbar.choice.FindString(name)
            if idx != wx.NOT_FOUND:
                self.toolbar.choice.SetString(idx, alias)


def test():
    app = wx.App()

    frame = IClassMapFrame()
    frame.Show()
    app.MainLoop()


if __name__ == "__main__":
    test()
