"""!
@package iclass::frame

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
import grass.script as grass

from iclass.digit       import IClassVDigitWindow, IClassVDigit
from iclass.toolbars    import IClassMapToolbar, IClassMiscToolbar,\
                               IClassToolbar, IClassMapManagerToolbar
from iclass.statistics  import Statistics, BandStatistics
from iclass.dialogs     import CategoryListCtrl, IClassCategoryManagerDialog,\
                               IClassGroupDialog, IClassMapDialog, IClassSignatureFileDialog
from iclass.plots       import PlotPanel
        
class IClassMapFrame(DoubleMapFrame):
    """! wxIClass main frame
    
    It has two map windows one for digitizing training areas and one for
    result preview.
    It generates histograms, raster maps and signature files using
    @c I_iclass_* functions from C imagery library.
    
    It is wxGUI counterpart of old i.class module.
    """
    def __init__(self, parent = None, title = _("Supervised Classification Tool"),
                 toolbars = ["iClassMisc", "iClassMap", "vdigit", "iClass"],
                 size = (800, 600), name = 'IClassWindow', **kwargs):
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
        
        self.firstMapWindow = IClassVDigitWindow(self, map = self.firstMap)
        self.secondMapWindow = BufferedWindow(self, Map = self.secondMap)
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
                            
        self.statusbarItemsHiddenInNviz = (sb.SbAlignExtent,
                                           sb.SbDisplayGeometry,
                                           sb.SbShowRegion,
                                           sb.SbResolution,
                                           sb.SbMapScale)
        
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
        grass.run_command('g.manual',
                          entry = 'wxGUI.IClass')
        
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
                                                         'moveLine', 'deleteLine'])
            
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
        mapTb.Enable('zoomback', enable = (len(self.MapWindow.zoomhistory) > 1))

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
            if group['fullname']:
                self.group = group['fullname']
                
        dlg.Destroy()
        
    def OnCategoryManager(self, event):
        """!Show category management dialog"""
        dlg = IClassCategoryManagerDialog(self)
        dlg.Show()
        
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
        since last analysis computaiton.
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
        #self.firstMapWindow.digit.CloseMap()
        self.poMapInfo = self.firstMapWindow.digit.GetDisplay().poMapInfo
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
                                     
            if I_iclass_analysis(statistics, self.refer, self.poMapInfo, "1",
                                 self.group, stats.rasterName):
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
            else:
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
                     message = _("Group <%s> does not have enough files "
                                 "(it has %d files). Operation canceled.") % (group, nLayers))
            return False
        
        #check if vector has any areas
        vectorInfo = grass.vector_info(vector)
        numAreas = Vect_get_num_areas(self.poMapInfo)
        
        if numAreas <= 0:
            GMessage(parent = self,
            message = _("No areas given. "
                        "Operation canceled."))
            return False
            
        # check if vector is inside raster
        rasterInfo = grass.raster_info(groupLayers[0])
        
        if vectorInfo['north'] > rasterInfo['north'] or \
           vectorInfo['south'] < rasterInfo['south'] or \
           vectorInfo['east'] > rasterInfo['east'] or \
           vectorInfo['west'] < rasterInfo['west']:
           GMessage(parent = self,
                    message = _("Vector features are outside raster layers. "
                                "Operation canceled."))
           return False
            
        return True
        
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
        """!Enable paanning for plots"""
        super(IClassMapFrame, self).OnPan(event)
        self.plotPanel.EnablePan()
        

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
        
    def RemoveLayer(self, name, idx):
        """!Removes layer from Map and update toolbar"""
        self.map.GetListOfLayers(l_type = 'raster')
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
    
