"""!
@package iclass.frame

@brief wxIClass frame with toolbar for digitizing training areas and
for spectral signature analysis.

Classes:
 - frame::IClassMapFrame
 - frame::MapManager

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys
import copy
import tempfile

if __name__ == "__main__":
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))
from core import globalvar
import wx

from ctypes import *

try:
    from grass.lib.imagery import *
    from grass.lib.vector import *
    haveIClass = True
    errMsg = ''
except ImportError, e:
    haveIClass = False
    errMsg = _("Loading imagery lib failed.\n%s") % e

import grass.script as grass

from mapdisp            import statusbar as sb
from mapdisp.mapwindow  import BufferedWindow
from vdigit.toolbars    import VDigitToolbar
from gui_core.mapdisp   import DoubleMapFrame
from core.render        import Map, MapLayer
from core.gcmd          import RunCommand, GMessage
from gui_core.dialogs   import SetOpacityDialog
from dbmgr.vinfo        import VectorDBInfo
import grass.script as grass

from iclass.digit       import IClassVDigitWindow, IClassVDigit
from iclass.toolbars    import IClassMapToolbar, IClassMiscToolbar,\
                               IClassToolbar, IClassMapManagerToolbar
from iclass.statistics  import Statistics, BandStatistics
from iclass.dialogs     import CategoryListCtrl, IClassCategoryManagerDialog,\
                               IClassGroupDialog, IClassSignatureFileDialog,\
                               IClassExportAreasDialog, IClassMapDialog
from iclass.plots       import PlotPanel
        
class IClassMapFrame(DoubleMapFrame):
    """! wxIClass main frame
    
    It has two map windows one for digitizing training areas and one for
    result preview.
    It generates histograms, raster maps and signature files using
    @c I_iclass_* functions from C imagery library.
    
    It is wxGUI counterpart of old i.class module.
    """
    def __init__(self, parent = None, giface = None, title = _("Supervised Classification Tool"),
                 toolbars = ["iClassMisc", "iClassMap", "vdigit", "iClass"],
                 size = (875, 600), name = 'IClassWindow', **kwargs):
        """!
        @param parent (no parent is expected)
        @param title window title
        @param toolbars dictionary of active toolbars (defalult value represents all toolbars)
        @param size default size
        """
        DoubleMapFrame.__init__(self, parent = parent, title = title,
                                name = name,
                                firstMap = Map(), secondMap = Map(),
                                **kwargs)
        self._giface = giface
        self.firstMapWindow = IClassVDigitWindow(parent = self, giface = self._giface,
                                                 map = self.firstMap, frame = self)
        self.secondMapWindow = BufferedWindow(parent = self, giface = self._giface,
                                              Map = self.secondMap, frame = self)
        self.MapWindow = self.firstMapWindow # current by default
        
        self._bindWindowsActivation()
        
        self.SetSize(size)
        #
        # Add toolbars
        #
        
        toolbarsCopy = toolbars[:]
        if sys.platform == 'win32':
            self.AddToolbar(toolbarsCopy.pop(1))
            toolbarsCopy.reverse()
        else:
            self.AddToolbar(toolbarsCopy.pop(0))
        for toolb in toolbarsCopy:
            self.AddToolbar(toolb)
        self.firstMapWindow.SetToolbar(self.toolbars['vdigit'])
        
        self.GetMapToolbar().GetActiveMapTool().Bind(wx.EVT_CHOICE, self.OnUpdateActive)
        
        #
        # Add statusbar
        #
        
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
        
        self.trainingMapManager = MapManager(self, mapWindow = self.GetFirstWindow(),
                                             Map = self.GetFirstMap())
        self.previewMapManager = MapManager(self, mapWindow = self.GetSecondWindow(),
                                            Map = self.GetSecondMap())
                                           
        self.InitStatistics()
        
        self.changes = False
        self.exportVector = None
        
        # dialogs
        self.dialogs = dict()
        self.dialogs['classManager'] = None
        # just to make digitizer happy
        self.dialogs['attributes'] = None
        self.dialogs['category'] = None
        
        
        # PyPlot init
        self.plotPanel = PlotPanel(self, statDict = self.statisticsDict,
                                   statList = self.statisticsList)
                                   
        self._addPanes()
        self._mgr.Update()
        
        self.trainingMapManager.SetToolbar(self.toolbars['iClassTrainingMapManager'])
        self.previewMapManager.SetToolbar(self.toolbars['iClassPreviewMapManager'])
        
        # default action
        self.OnPan(event = None)
        
        wx.CallAfter(self.AddTrainingAreaMap)
        
        #self.dialogs['category'] = None
        
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        
    def OnCloseWindow(self, event):
        self.GetFirstWindow().digit.GetDisplay().CloseMap()
        self.Destroy()
        
    def __del__(self):
        """! Frees C structs and removes vector map and all raster maps."""
        I_free_signatures(self.signatures)
        I_free_group_ref(self.refer)
        for st in self.cStatisticsDict.values():
            I_iclass_free_statistics(st)
            
        self.RemoveTempVector()
        for i in self.statisticsList:
            self.RemoveTempRaster(self.statisticsDict[i].rasterName)
            
    def OnHelp(self, event):
        """!Show help page"""
        self._giface.Help(entry = 'wxGUI.IClass')
        
    def CreateTempVector(self):
        """!Create temporary vector map for training areas"""
        vectorPath = grass.tempfile(create = False)
        vectorName = 'trAreas' + os.path.basename(vectorPath).replace('.','')

        cmd = ('v.edit', {'tool': 'create',
                          'map': vectorName})
        
        ret = RunCommand(prog = cmd[0],
                         parent = self,
                         overwrite = True,
                         **cmd[1])
        if ret != 0:
            return False
        
        return vectorName
        
    def RemoveTempVector(self):
        """!Removes temporary vector map with training areas"""
        ret = RunCommand(prog = 'g.remove',
                         parent = self,
                         vect = self.trainingAreaVector)
        if ret != 0:
            return False
        return True
        
    def RemoveTempRaster(self, raster):
        """!Removes temporary raster maps"""
        self.GetFirstMap().Clean()
        self.GetSecondMap().Clean()
        ret = RunCommand(prog = 'g.remove',
                         parent = self,
                         rast = raster)
        if ret != 0:
            return False
        return True

    def AddToolbar(self, name):
        """!Add defined toolbar to the window
        
        Currently known toolbars are:
         - 'iClassMap'          - basic map toolbar
         - 'iClass'             - iclass tools
         - 'iClassMisc'         - miscellaneous (help)
         - 'vdigit'             - digitizer toolbar (areas)
         
         Toolbars 'iClassPreviewMapManager' are added in _addPanes().
        """
        if name == "iClassMap":
            self.toolbars[name] = IClassMapToolbar(self)
            
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name(name).Caption(_("Map Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2).Row(1).
                              BestSize((self.toolbars[name].GetBestSize())))
                              
        if name == "iClass":
            self.toolbars[name] = IClassToolbar(self)
            
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name(name).Caption(_("IClass Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2).Row(2).
                              BestSize((self.toolbars[name].GetBestSize())))
                              
        if name == "iClassMisc":
            self.toolbars[name] = IClassMiscToolbar(self)
            
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name(name).Caption(_("IClass Misc Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2).Row(1).
                              BestSize((self.toolbars[name].GetBestSize())))

        if name == "vdigit":
            self.toolbars[name] = VDigitToolbar(self, MapWindow = self.GetFirstWindow(),
                                                digitClass = IClassVDigit,
                                                tools = ['addArea', 'moveVertex', 'addVertex',
                                                         'removeVertex', 'editLine',
                                                         'moveLine', 'deleteArea', 'undo', 'redo'])
            
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name(name).Caption(_("Digitization Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(2).Row(2).
                              BestSize((self.toolbars[name].GetBestSize())))
                              
    def _addPanes(self):
        """!Add mapwindows and toolbars to aui manager"""
        if sys.platform == 'win32':
            self._addPaneMapWindow(name = 'training')
            self._addPaneToolbar(name = 'iClassTrainingMapManager')
            self._addPaneMapWindow(name = 'preview')
            self._addPaneToolbar(name = 'iClassPreviewMapManager')
        else:
            self._addPaneToolbar(name = 'iClassPreviewMapManager')
            self._addPaneMapWindow(name = 'preview')
            self._addPaneToolbar(name = 'iClassTrainingMapManager')
            self._addPaneMapWindow(name = 'training')
        
        self._mgr.AddPane(self.plotPanel, wx.aui.AuiPaneInfo().
                  Name("plots").Caption(_("Plots")).
                  Dockable(False).Floatable(False).CloseButton(False).
                  Left().Layer(1).BestSize((400, -1)))
        
    def _addPaneToolbar(self, name):
        if name == 'iClassPreviewMapManager':
            parent = self.previewMapManager
        else:
            parent = self.trainingMapManager
        
        self.toolbars[name] = IClassMapManagerToolbar(self, parent)
        self._mgr.AddPane(self.toolbars[name],
                          wx.aui.AuiPaneInfo().ToolbarPane().Movable().
                          Name(name).
                          CloseButton(False).Center().Layer(0).
                          BestSize((self.toolbars[name].GetBestSize())))
        
    def _addPaneMapWindow(self, name):
        if name == 'preview':
            window = self.GetSecondWindow()
            caption = _("Preview Display")
        else:
            window = self.GetFirstWindow()
            caption = _("Training Areas Display")
        
        self._mgr.AddPane(window, wx.aui.AuiPaneInfo().
                          Name(name).Caption(caption).
                          Dockable(False).Floatable(False).CloseButton(False).
                          Center().Layer(0))
        
    def IsStandalone(self):
        """!Check if Map display is standalone"""
        return True
    
    def OnUpdateActive(self, event):
        """!
        @todo move to DoubleMapFrame?
        """
        if self.GetMapToolbar().GetActiveMap() == 0:
            self.MapWindow = self.firstMapWindow
            self.Map = self.firstMap
        else:
            self.MapWindow = self.secondMapWindow
            self.Map = self.secondMap

        self.UpdateActive(self.MapWindow)
        # for wingrass
        if os.name == 'nt':
            self.MapWindow.SetFocus()

    def UpdateActive(self, win):
        """!
        @todo move to DoubleMapFrame?
        """
        mapTb = self.GetMapToolbar()
        # optionally disable tool zoomback tool
        mapTb.Enable('zoomBack', enable = (len(self.MapWindow.zoomhistory) > 1))

        if mapTb.GetActiveMap() != (win == self.secondMapWindow):
            mapTb.SetActiveMap((win == self.secondMapWindow))
        self.StatusbarUpdate() 
        
    def GetMapToolbar(self):
        """!Returns toolbar with zooming tools"""
        return self.toolbars['iClassMap']

    def OnZoomMenu(self, event):
        """!Popup Zoom menu """
        zoommenu = wx.Menu()
        # Add items to the menu

        zoomsource = wx.MenuItem(zoommenu, wx.ID_ANY, _('Adjust Training Area Display to Preview Display'))
        zoommenu.AppendItem(zoomsource)
        self.Bind(wx.EVT_MENU, self.OnZoomToPreview, zoomsource)

        zoomtarget = wx.MenuItem(zoommenu, wx.ID_ANY, _('Adjust Preview display to Training Area Display'))
        zoommenu.AppendItem(zoomtarget)
        self.Bind(wx.EVT_MENU, self.OnZoomToTraining, zoomtarget)

        # Popup the menu. If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(zoommenu)
        zoommenu.Destroy()
        
    def OnZoomToTraining(self, event):
        """!Set preview display to match extents of training display """

        if not self.MapWindow == self.GetSecondWindow():
            self.MapWindow = self.GetSecondWindow()
            self.Map = self.GetSecondMap()
            self.UpdateActive(self.GetSecondWindow())

        newreg = self.firstMap.GetCurrentRegion()
        self.GetSecondMap().region = copy.copy(newreg)
        
        self.Render(self.GetSecondWindow())
        
    def OnZoomToPreview(self, event):
        """!Set preview display to match extents of training display """

        if not self.MapWindow == self.GetFirstWindow():
            self.MapWindow = self.GetFirstWindow()
            self.Map = self.GetFirstMap()
            self.UpdateActive(self.GetFirstWindow())

        newreg = self.GetSecondMap().GetCurrentRegion()
        self.GetFirstMap().region = copy.copy(newreg)
        
        self.Render(self.GetFirstWindow())
        
    def OnAddBands(self, event):
        """!Add imagery group"""
        dlg = IClassGroupDialog(self, group = self.group)
        if dlg.ShowModal() == wx.ID_OK:
            group = grass.find_file(name = dlg.GetGroup(), element = 'group')
            if group['name']:
                self.group = group['name']
                
        dlg.Destroy()
        
    def OnImportAreas(self, event):
        """!Import training areas"""
        # check if we have any changes
        if self.GetAreasCount() or self.statisticsList:
            qdlg = wx.MessageDialog(parent = self,
                                    message = _("All changes will be lost. "
                                                "Do you want to continue?") ,
                                    style = wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)
            if qdlg.ShowModal() == wx.ID_NO:
                qdlg.Destroy()
                return
            qdlg.Destroy()
            
        dlg = IClassMapDialog(self, title = _("Import vector map"), element = 'vector')
        if dlg.ShowModal() == wx.ID_OK:
            vName = dlg.GetMap()
            warning = self._checkImportedTopo(vName)
            if warning:
                GMessage(parent = self, message = warning)
                
            self.ImportAreas(vName)
            
        dlg.Destroy()
        
    def _checkImportedTopo(self, vector):
        """!Check if imported vector map has areas
        
        @param vector vector map name
        
        @return warning message (empty if topology is ok)
        """
        topo = grass.vector_info_topo(map = vector)
        
        warning = ''
        if topo['areas'] == 0:
            warning = _("No areas in vector map <%s>.\n" % vector)
        if topo['points'] or topo['lines']:
            warning +=_("Vector map <%s> contains points or lines, "
                        "these features are ignored." % vector)
            
        return warning
            
    def ImportAreas(self, vector):
        """!Import training areas.
        
        If table connected, try load certain columns to class manager
        
        @param vector vector map name
        """
        wx.BeginBusyCursor()
        wx.Yield()
        
        mapLayer = self.toolbars['vdigit'].mapLayer
        # set mapLayer temporarily to None
        # to avoid 'save changes' code in vdigit.toolbars
        self.toolbars['vdigit'].mapLayer = None
        
        ret =  self.toolbars['vdigit'].StopEditing()
        if not ret:
            wx.EndBusyCursor()
            return False
            
        ret, msg = RunCommand('g.copy',
                              vect = [vector, self.trainingAreaVector],
                              overwrite = True,
                              getErrorMsg = True)
        if ret != 0:
            wx.EndBusyCursor()
            return False
            
        ret = self.toolbars['vdigit'].StartEditing(mapLayer)
        if not ret:
            wx.EndBusyCursor()
            return False
            
        self.poMapInfo = self.GetFirstWindow().digit.GetDisplay().poMapInfo
        
        # remove temporary rasters
        for i in self.statisticsList:
            self.RemoveTempRaster(self.statisticsDict[i].rasterName)
        
        # clear current statistics
        self.statisticsDict.clear()
        del self.statisticsList[:] # not ...=[] !
        
        # reset plots
        self.plotPanel.Reset()
        
        self.GetFirstWindow().UpdateMap(render = False, renderVector = True)
        
        self.ImportClasses(vector)
        
        # should be saved in attribute table?
        self.toolbars['iClass'].UpdateStddev(1.5)
        
        wx.EndBusyCursor()
        
        return True
        
    def ImportClasses(self, vector):
        """!If imported map has table, try to import certain columns to class manager"""
        # check connection
        dbInfo = VectorDBInfo(vector)
        connected = (len(dbInfo.layers.keys()) > 0)
        
        # remove attribute table of temporary vector, we don't need it
        if connected:
            RunCommand('v.db.droptable',
                       flags = 'f',
                       map = self.trainingAreaVector)
            
        # we use first layer with table, TODO: user should choose
        layer = None
        for key in dbInfo.layers.keys():
            if dbInfo.GetTable(key):
                layer = key
        
        # get columns to check if we can use them
        # TODO: let user choose which columns mean what
        if layer is not None: 
            columns = dbInfo.GetColumns(table = dbInfo.GetTable(layer))
        else:
            columns = []
        
        # get class manager
        if self.dialogs['classManager'] is None:
            self.dialogs['classManager'] = IClassCategoryManagerDialog(self)
                
        listCtrl = self.dialogs['classManager'].GetListCtrl()
        
        # unable to load data (no connection, table, right columns)
        if not connected or layer is None or \
                   'class' not in columns or \
                   'color' not in columns:
            # no table connected
            cats = RunCommand('v.category',
                       input = vector,
                       layer = 1, # set layer?
                       # type = ['centroid', 'area'] ?
                       option = "print",
                       read = True)
            cats = map(int, cats.strip().split())
            cats = sorted(list(set(cats)))

            for cat in cats:
                listCtrl.AddCategory(cat = cat, name = 'class_%d' % cat, color = "0:0:0")
        # connection, table and columns exists
        else:
            columns = ['cat', 'class', 'color']
            ret = RunCommand('v.db.select',
                                 quiet = True,
                                 parent = self,
                                 flags = 'c',
                                 map = vector,
                                 layer = 1,
                                 columns = ','.join(columns),
                                 read = True)
            records = ret.strip().split('\n')
            for record in records:
                record = record.split('|')
                listCtrl.AddCategory(cat = int(record[0]), name = record[1], color = record[2])
            
    def OnExportAreas(self, event):
        """!Export training areas"""
        if self.GetAreasCount() == 0:
            GMessage(parent = self, message = _("No training areas to export."))
            return
            
        dlg = IClassExportAreasDialog(self, vectorName = self.exportVector)
        
        if dlg.ShowModal() == wx.ID_OK:
            vName = dlg.GetVectorName()
            self.exportVector = vName
            withTable = dlg.WithTable()
            
            self.ExportAreas(vectorName = vName, withTable = withTable)
            
        dlg.Destroy()
        
    def ExportAreas(self, vectorName, withTable):
        """!Export training areas to new vector map (with attribute table).
        
        @param vectorName name of exported vector map
        @param withTable true if attribute table is required
        """
        wx.BeginBusyCursor()
        wx.Yield()
        
        # close, build, copy and open again the temporary vector
        displayDriver = self.GetFirstWindow().digit.GetDisplay()
        displayDriver.CloseMap()
        RunCommand('g.copy',
                    vect = ','.join([self.trainingAreaVector, vectorName]),
                    overwrite = True)
        # remove connection if exists:
        dbinfo = grass.vector_db(vectorName)
        if dbinfo:
            for layer in dbinfo.keys():
                RunCommand('v.db.connect', flags = 'd', map = vectorName, layer = layer)

        mapset = grass.gisenv()['MAPSET']
        self.poMapInfo = displayDriver.OpenMap(name = self.trainingAreaVector, mapset = mapset)
            
        if not withTable:
            wx.EndBusyCursor()
            return
            
        # add table
        columns = ["class varchar(30)",
                   "color varchar(11)",
                   "n_cells integer",]
                   
        nbands = len(self.GetGroupLayers(self.group))
        for statistic, format in (("min", "integer"), ("mean", "double precision"), ("max", "integer")):
            for i in range(nbands):
                # 10 characters limit?
                columns.append("band%(band)d_%(stat)s %(format)s" % {'band' : i + 1,
                                                                    'stat' : statistic,
                                                                    'format' : format})
        
        ret, msg = RunCommand('v.db.addtable',
                         map = vectorName,
                         columns = columns,
                         getErrorMsg = True)
        if ret != 0:
            wx.EndBusyCursor()
            GMessage(parent = self, message = _("Failed to add attribute table. "
                                                "Details:\n%s" % msg))
            return
            
        # populate table
        for cat in self.statisticsList:
            stat = self.statisticsDict[cat]
            
            self._runDBUpdate(map = vectorName, column = "class", value = stat.name, cat = cat)
            self._runDBUpdate(map = vectorName, column = "color", value = stat.color, cat = cat)
            
            if not stat.IsReady():
                continue
                
            self._runDBUpdate(map = vectorName, column = "n_cells",value = stat.ncells, cat = cat)
            
            for i in range(nbands):
                self._runDBUpdate(map = vectorName, column = "band%d_min" % (i + 1), value = stat.bands[i].min, cat = cat)
                self._runDBUpdate(map = vectorName, column = "band%d_mean" % (i + 1), value = stat.bands[i].mean, cat = cat)
                self._runDBUpdate(map = vectorName, column = "band%d_max" % (i + 1), value = stat.bands[i].max, cat = cat)
        
        wx.EndBusyCursor()
        
    def _runDBUpdate(self, map, column, value, cat):
        """!Helper function for calling v.db.update.
        
        @param map vector map name
        @param column name of updated column
        @param value new value
        @param cat which category to update
        
        @return returncode (0 is OK)
        """
        ret = RunCommand('v.db.update',
                        map = map,
                        layer = 1,
                        column = column,
                        value = value,
                        where = "cat = %d" % cat)
                            
        return ret
        
    def OnCategoryManager(self, event):
        """!Show category management dialog"""
        if self.dialogs['classManager'] is None:
            dlg = IClassCategoryManagerDialog(self)
            dlg.Show()
            self.dialogs['classManager'] = dlg
        else:
            if not self.dialogs['classManager'].IsShown():
                self.dialogs['classManager'].Show()
        
    def CategoryChanged(self, currentCat):
        """!Updates everything which depends on current category.
        
        Updates number of stddev, histograms, layer in preview display. 
        """
        nstd = self.statisticsDict[currentCat].nstd
        self.toolbars['iClass'].UpdateStddev(nstd)
        
        self.plotPanel.UpdateCategory(currentCat)
        self.plotPanel.OnPlotTypeSelected(None)
                                   
        name = self.statisticsDict[currentCat].rasterName
        name = self.previewMapManager.GetAlias(name)
        if name:
            self.previewMapManager.SelectLayer(name)
        
    def DeleteAreas(self, cats):
        """!Removes all training areas of given categories
        
        @param cats list of categories to be deleted
        """
        self.firstMapWindow.digit.DeleteAreasByCat(cats)
        self.firstMapWindow.UpdateMap(render=False, renderVector=True)
        
    def HighlightCategory(self, cats):
        """!Highlight araes given by category"""
        self.firstMapWindow.digit.GetDisplay().SetSelected(cats, layer = 1)
        self.firstMapWindow.UpdateMap(render=False, renderVector=True)
        
    def ZoomToAreasByCat(self, cat):
        """!Zoom to areas given by category"""
        n, s, w, e = self.GetFirstWindow().digit.GetDisplay().GetRegionSelected()
        self.GetFirstMap().GetRegion(n = n, s = s, w = w, e = e, update = True)
        self.GetFirstMap().AdjustRegion()
        self.GetFirstMap().AlignExtentFromDisplay()
        
        self.GetFirstWindow().UpdateMap(render = True, renderVector = True)
        
    def UpdateRasterName(self, newName, cat):
        """!Update alias of raster map when category name is changed"""
        origName = self.statisticsDict[cat].rasterName
        self.previewMapManager.SetAlias(origName, newName)
        
    def StddevChanged(self, cat, nstd):
        """!Standard deviation multiplier changed, rerender map, histograms"""
        stat = self.statisticsDict[cat]
        stat.nstd = nstd
        
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
        """!Informs if any important changes happened
        since last analysis computation.
        """
        self.changes = changes
        
    def AddRasterMap(self, name, firstMap = True, secondMap = True):
        """!Add raster map to Map"""
        cmdlist = ['d.rast', 'map=%s' % name]
        if firstMap:
            self.GetFirstMap().AddLayer(type='raster', command=cmdlist, l_active=True,
                                        name=name, l_hidden=False, l_opacity=1.0, l_render=False)
            self.Render(self.GetFirstWindow())
        if secondMap:
            self.GetSecondMap().AddLayer(type='raster', command=cmdlist, l_active=True,
                                        name=name, l_hidden=False, l_opacity=1.0, l_render=False)
            self.Render(self.GetSecondWindow())
           
    def AddTrainingAreaMap(self):
        """!Add vector map with training areas to Map (training
        sub-display)"""
        vname = self.CreateTempVector()
        if vname:
            self.trainingAreaVector = vname
        else:
            GMessage(parent = self, message = _("Failed to create temporary vector map."))
            return
            
        mapLayer = self.GetFirstMap().AddLayer(type = 'vector',
                                               command = ['d.vect', 'map=%s' % vname],
                                               name = vname, l_active = False)
        
        self.toolbars['vdigit'].StartEditing(mapLayer)
        self.poMapInfo = self.GetFirstWindow().digit.GetDisplay().poMapInfo
        self.Render(self.GetFirstWindow())
        
    def OnRunAnalysis(self, event):
        """!Run analysis and update plots"""
        if self.RunAnalysis():
            currentCat = self.GetCurrentCategoryIdx()
            self.plotPanel.UpdatePlots(group = self.group, currentCat = currentCat,
                                       statDict = self.statisticsDict,
                                       statList = self.statisticsList)
        
    def RunAnalysis(self):
        """!Run analysis
        
        Calls C functions to compute all statistics and creates raster maps.
        Signatures are created but signature file is not.
        """
        if not self.CheckInput(group = self.group, vector = self.trainingAreaVector):
            return
            
        for statistic in self.cStatisticsDict.values():
            I_iclass_free_statistics(statistic)
        self.cStatisticsDict = {}
        
        # init Ref struct with the files in group */
        I_free_group_ref(self.refer)
        if (not I_iclass_init_group(self.group, self.refer)):
            return False
        
        I_free_signatures(self.signatures)
        I_iclass_init_signatures(self.signatures, self.refer)
        
        cats = self.statisticsList[:]
        for i in cats:
            stats = self.statisticsDict[i]
            
            statistics_obj = IClass_statistics()
            statistics = pointer(statistics_obj)
            
            I_iclass_init_statistics(statistics, 
                                     stats.category,
                                     stats.name,
                                     stats.color,
                                     stats.nstd)
                                     
            ret = I_iclass_analysis(statistics, self.refer, self.poMapInfo, "1",
                                 self.group, stats.rasterName)
            if ret > 0:
                # tests
                self.cStatisticsDict[i] = statistics
                
                stats.SetStatistics(statistics)
                stats.SetReady()
                self.statisticsDict[stats.category] = stats
                
                self.ConvertToNull(name = stats.rasterName)
                self.previewMapManager.AddLayer(name = stats.rasterName,
                                                alias = stats.name, resultsLayer = True)
                # write statistics
                I_iclass_add_signature(self.signatures, statistics)
                
            elif ret == 0:
                GMessage(parent = self, message = _("No area in category %s. Category skipped.") % stats.category)
                I_iclass_free_statistics(statistics)
            else:
                GMessage(parent = self, message = _("Analysis failed."))
                I_iclass_free_statistics(statistics)
        
        self.UpdateChangeState(changes = False)
        return True
        
    def OnSaveSigFile(self, event):
        """!Asks for signature file name and saves it."""
        if not self.group:
            GMessage(parent = self, message = _("No imagery group selected."))
            return
            
        if self.changes:
            qdlg = wx.MessageDialog(parent = self,
                                    message = _("Due to recent changes in classes, "
                                                "signatures can be outdated and should be recalculated. "
                                                "Do you still want to continue?") ,
                                   caption = _("Outdated signatures"),
                                   style = wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)
            if qdlg.ShowModal() == wx.ID_YES:
                qdlg.Destroy()
            else:
                qdlg.Destroy()
                return
                    
        dlg = IClassSignatureFileDialog(self, group = self.group, file = self.sigFile)
        
        if dlg.ShowModal() == wx.ID_OK:
            if os.path.exists(dlg.GetFileName(fullPath = True)):
                qdlg = wx.MessageDialog(parent = self,
                                        message = _("A signature file named %s already exists.\n"
                                                    "Do you want to replace it?") % dlg.GetFileName(),
                                        caption = _("File already exists"),
                                        style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)
                if qdlg.ShowModal() == wx.ID_YES:
                    qdlg.Destroy()
                else:
                    qdlg.Destroy()
                    return
            self.sigFile = dlg.GetFileName()
            self.WriteSignatures(self.signatures, self.group, self.sigFile)
            
        dlg.Destroy()
        
    def InitStatistics(self):
        """!Initialize variables and c structures neccessary for
        computing statistics.
        """
        self.group = None
        self.sigFile = None
        
        self.statisticsDict = {}
        self.statisticsList = []
        
        self.cStatisticsDict = {}
        
        self.signatures_obj = Signature()
        self.signatures = pointer(self.signatures_obj)
        I_init_signatures(self.signatures, 0) # must be freed on exit
        
        refer_obj = Ref()
        self.refer = pointer(refer_obj)
        I_init_group_ref(self.refer) # must be freed on exit
        
    def WriteSignatures(self, signatures, group, filename):
        """!Writes current signatures to signature file
        
        @param signatures signature (c structure)
        @param group imagery group
        @param filename signature file name
        """
        I_iclass_write_signatures(signatures, group, group, filename)
                                        
    def CheckInput(self, group, vector):
        """!Check if input is valid"""
        # check if group is ok
        if not group:
            GMessage(parent = self,
                     message = _("No imagery group selected. "
                                 "Operation canceled."))
            return False
            
        groupLayers = self.GetGroupLayers(group)
            
        nLayers = len(groupLayers)
        if nLayers <= 1:
            GMessage(parent = self,
                     message = _("Group <%(group)s> does not have enough files "
                                 "(it has %(files)d files). Operation canceled.") % \
                         { 'group' : group,
                           'files' : nLayers })
            return False
        
        #check if vector has any areas
        if self.GetAreasCount() == 0:
            GMessage(parent = self,
            message = _("No areas given. "
                        "Operation canceled."))
            return False
            
        # check if vector is inside raster
        regionBox = bound_box()
        Vect_get_map_box(self.poMapInfo, byref(regionBox))
        
        rasterInfo = grass.raster_info(groupLayers[0])
        
        if regionBox.N > rasterInfo['north'] or \
           regionBox.S < rasterInfo['south'] or \
           regionBox.E > rasterInfo['east'] or \
           regionBox.W < rasterInfo['west']:
           GMessage(parent = self,
                    message = _("Vector features are outside raster layers. "
                                "Operation canceled."))
           return False
            
        return True
        
    def GetAreasCount(self):
        """!Returns number of not dead areas"""
        count = 0
        numAreas = Vect_get_num_areas(self.poMapInfo)
        for i in range(numAreas):
            if Vect_area_alive(self.poMapInfo, i + 1):
                count += 1
        return count
        
    def GetGroupLayers(self, group):
        """! Get layers in group
    
        @todo consider moving this function to core module for convenient
        """
        res = RunCommand('i.group',
                         flags = 'g',
                         group = group,
                         read = True).strip()
        if res.split('\n')[0]:
            return res.split('\n')
        return []
    
    def ConvertToNull(self, name):
        """! Sets value which represents null values for given raster map.
        
        @param name raster map name
        """
        RunCommand('r.null',
                   map = name,
                   setnull = 0)
                     
    def GetCurrentCategoryIdx(self):
        """!Returns current category number"""
        return self.toolbars['iClass'].GetSelectedCategoryIdx()
        
    def OnZoomIn(self, event):
        """!Enable zooming for plots"""
        super(IClassMapFrame, self).OnZoomIn(event)
        self.plotPanel.EnableZoom(type = 1)
        
    def OnZoomOut(self, event):
        """!Enable zooming for plots"""
        super(IClassMapFrame, self).OnZoomOut(event)
        self.plotPanel.EnableZoom(type = -1)
        
    def OnPan(self, event):
        """!Enable panning for plots"""
        super(IClassMapFrame, self).OnPan(event)
        self.plotPanel.EnablePan()
        
    def OnPointer(self, event):
        """!Set pointer mode.

        @fixme: needs refactoring
        """
        toolbar = self.GetMapToolbar()
        self.SwitchTool(toolbar, event)

        self.GetFirstWindow().mouse['use'] = 'pointer'

class MapManager:
    """! Class for managing map renderer.
    
    It is connected with iClassMapManagerToolbar.
    """
    def __init__(self, frame, mapWindow, Map):
        """!
        
        It is expected that \a mapWindow is conected with \a Map.
        
        @param frame application main window
        @param mapWindow map window instance
        @param Map map renderer instance
        """
        self.map = Map
        self.frame = frame
        self.mapWindow = mapWindow
        self.toolbar = None
        
        self.layerName = {}
        
        
    def SetToolbar(self, toolbar):
        self.toolbar = toolbar
        
    def AddLayer(self, name, alias = None, resultsLayer = False):
        """!Adds layer to Map and update toolbar 
        
        @param name layer (raster) name
        @param resultsLayer True if layer is temp. raster showing the results of computation
        """
        if (resultsLayer and
            name in [l.GetName() for l in self.map.GetListOfLayers(l_name = name)]):
            self.frame.Render(self.mapWindow)
            return
            
        cmdlist = ['d.rast', 'map=%s' % name]
        self.map.AddLayer(type = 'raster', command = cmdlist, l_active = True,
                          name = name, l_hidden = False, l_opacity = 1.0, l_render = True)
        self.frame.Render(self.mapWindow)
        
        if alias is not None:
            alias = self._addSuffix(alias)
            self.layerName[alias] = name
            name = alias
        else:
            self.layerName[name] = name
            
        self.toolbar.choice.Insert(name, 0)
        self.toolbar.choice.SetSelection(0)
        
    def RemoveTemporaryLayer(self, name):
        """!Removes temporary layer (if exists) from Map and and updates toolbar.
        
        @param name real name of layer
        """
        # check if layer is loaded
        layers = self.map.GetListOfLayers(l_type = 'raster')
        idx = None
        for i, layer in enumerate(layers):
            if name == layer.GetName():
                idx = i
                break
        if idx is None:
            return
        # remove it from Map
        self.map.RemoveLayer(name = name)
        
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
        
    def RemoveLayer(self, name, idx):
        """!Removes layer from Map and update toolbar"""
        name = self.layerName[name]
        self.map.RemoveLayer(name = name)
        del self.layerName[name]
        self.toolbar.choice.Delete(idx)
        if not self.toolbar.choice.IsEmpty():
            self.toolbar.choice.SetSelection(0)
            
        self.frame.Render(self.mapWindow)
            
    def SelectLayer(self, name):
        """!Moves selected layer to top"""
        layers = self.map.GetListOfLayers(l_type = 'raster')
        idx = None
        for i, layer in enumerate(layers):
            if self.layerName[name] == layer.GetName():
                idx = i
                break
                
        if idx is not None: # should not happen
            layers.append(layers.pop(idx))
            
            choice = self.toolbar.choice
            idx = choice.FindString(name)
            choice.Delete(idx)
            choice.Insert(name, 0)
            choice.SetSelection(0)
            
            #layers.reverse()
            self.map.ReorderLayers(layers)
            self.frame.Render(self.mapWindow)
        
    def SetOpacity(self, name):
        """!Sets opacity of layers."""
        name = self.layerName[name]
        layers = self.map.GetListOfLayers(l_name = name)
        if not layers:
            return
            
        # works for first layer only
        oldOpacity = layers[0].GetOpacity()
        dlg = SetOpacityDialog(self.frame, opacity = oldOpacity)
        
        if dlg.ShowModal() == wx.ID_OK:
            self.map.ChangeOpacity(layer = layers[0], l_opacity = dlg.GetOpacity())
            
        dlg.Destroy()
        
        self.frame.Render(self.mapWindow)
        
    def _addSuffix(self, name):
        suffix = _('results')
        return '_'.join((name, suffix))
        
    def GetAlias(self, name):
        """!Returns alias for layer"""
        name =  [k for k, v in self.layerName.iteritems() if v == name]
        if name:
            return name[0]
        return None
        
    def SetAlias(self, original, alias):
        name = self.GetAlias(original)
        if name:
            self.layerName[self._addSuffix(alias)] = original
            del self.layerName[name]
            idx = self.toolbar.choice.FindString(name)
            if idx != wx.NOT_FOUND:
                self.toolbar.choice.SetString(idx, self._addSuffix(alias))

def test():
    import gettext
    import core.render as render
    
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    
    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()
    
    frame = IClassMapFrame()
    frame.Show()
    app.MainLoop()

if __name__ == "__main__":
    test()
    
