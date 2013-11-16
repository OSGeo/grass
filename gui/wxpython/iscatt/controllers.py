"""!
@package iscatt.controllers

@brief Controller layer wx.iscatt.

Classes:
 - controllers::ScattsManager
 - controllers::PlotsRenderingManager
 - controllers::CategoriesManager
 - controllers::IMapWinDigitConnection
 - controllers::IClassDigitConnection
 - controllers::IMapDispConnection
 - controllers::IClassConnection
 - controllers::gThread

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""
import os
import sys
from copy import deepcopy
import wx

import time
import threading
import Queue
from core.gconsole import EVT_CMD_DONE

from core.gcmd import GException, GError, GMessage, RunCommand, GWarning
from core.settings import UserSettings
from core.gconsole import wxCmdRun, wxCmdDone, wxCmdPrepare
from iscatt.iscatt_core import Core, idBandsToidScatt, GetRasterInfo, GetRegion, \
MAX_SCATT_SIZE, WARN_SCATT_SIZE, MAX_NCELLS, WARN_NCELLS
from iscatt.dialogs import AddScattPlotDialog, ExportCategoryRaster

from iclass.dialogs import IClassGroupDialog

import grass.script as grass
from grass.pydispatch.signal import Signal

class ScattsManager:
    """!Main controller
    """
    def __init__(self, guiparent, giface, iclass_mapwin = None):
        self.giface = giface
        self.mapDisp  = giface.GetMapDisplay()

        if iclass_mapwin:
            self.mapWin = iclass_mapwin
        else:
            self.mapWin = giface.GetMapWindow()

        self.guiparent = guiparent

        self.show_add_scatt_plot = False

        self.core = Core()

        self.cats_mgr = CategoriesManager(self, self.core)
        self.render_mgr = PlotsRenderingManager(scatt_mgr=self, 
                                                cats_mgr=self.cats_mgr, 
                                                core=self.core)

        self.thread = gThread();
        
        self.plots = {}

        self.plot_mode =  None
        self.pol_sel_mode = [False, None]

        self.data_set = False

        self.cursorPlotMove = Signal("ScattsManager.cursorPlotMove")

        self.renderingStarted = self.render_mgr.renderingStarted
        self.renderingFinished = self.render_mgr.renderingFinished

        self.computingStarted = Signal("ScattsManager.computingStarted")

        if iclass_mapwin: 
            self.digit_conn = IClassDigitConnection(self, 
                                                    self.mapWin, 
                                                    self.core.CatRastUpdater())
            self.iclass_conn = IClassConnection(self, 
                                                iclass_mapwin.parent, 
                                                self.cats_mgr)
        else:
            self.digit_conn = IMapWinDigitConnection()
            self.iclass_conn = IMapDispConnection(scatt_mgr=self, 
                                                  cats_mgr=self.cats_mgr, 
                                                  giface=self.giface)

        self._initSettings()

        self.modeSet = Signal("ScattsManager.mondeSet")

    def CleanUp(self):
        self.thread.Terminate() 
        # there should be better way hot to clean up the thread
        # than calling the clean up function outside the thread,
        # which still may running
        self.core.CleanUp()

    def CleanUpDone(self):
        for scatt_id, scatt in self.plots.items():
            if scatt['scatt']:
                scatt['scatt'].CleanUp()

        self.plots.clear()
    
    def _initSettings(self):
        """!Initialization of settings (if not already defined)
        """
        # initializes default settings
        initSettings = [
                        ['selection', 'sel_pol', (255,255,0)],
                        ['selection', 'sel_pol_vertex', (255,0,0)],
                        ['selection', 'sel_area', (0,255,19)],
                        ['selection', "snap_tresh", 10],
                        ['selection', 'sel_area_opacty', 50],
                        ['ellipses', 'show_ellips', True],
                       ]

        for init in initSettings: 
            UserSettings.ReadSettingsFile()
            UserSettings.Append(dict = UserSettings.userSettings, 
                                group ='scatt',
                                key = init[0],
                                subkey =init[1],
                                value = init[2],
                                overwrite = False)

    def SetData(self):
        self.iclass_conn.SetData()
        self.digit_conn.SetData()

    def SetBands(self, bands):
        self.busy = wx.BusyInfo(_("Loading data..."))
        self.data_set = False
        self.thread.Run(callable=self.core.CleanUp, 
                        ondone=lambda event : self.CleanUpDone())

        if self.show_add_scatt_plot:
            show_add=True
        else:
            show_add=False

        self.all_bands_to_bands = dict(zip(bands, [-1] * len(bands)))
        self.all_bands = bands

        self.region = GetRegion()
        ncells = self.region["rows"] * self.region["cols"]

        if ncells > MAX_NCELLS:
            del self.busy
            self.data_set = True
            return

        self.bands = bands[:]
        self.bands_info = {}
        valid_bands = []
    
        for b in self.bands[:]:
            i = GetRasterInfo(b)

            self.bands_info[b] = i
            if i is not None:
                valid_bands.append(b)

        for i, b in enumerate(valid_bands):
            # name : index in core bands - 
            # if not in core bands (not CELL type) -> index = -1 
            self.all_bands_to_bands[b] = i

        self.thread.Run(callable=self.core.SetData, 
                        bands=valid_bands, 
                        ondone=self.SetDataDone, 
                        userdata={"show_add" : show_add})

    def SetDataDone(self, event):
        del self.busy
        self.data_set = True

        todo = event.ret
        self.bad_bands = event.ret
        bands = self.core.GetBands()

        self.bad_rasts = event.ret
        self.cats_mgr.SetData()
        if event.userdata['show_add']:
          self.AddScattPlot()

    def GetBands(self):
        return self.core.GetBands()

    def AddScattPlot(self):
        if not self.data_set and self.iclass_conn:
            self.show_add_scatt_plot = True
            self.iclass_conn.SetData()
            self.show_add_scatt_plot = False
            return
        if not self.data_set:
            GError(_('No data set.'))
            return

        self.computingStarted.emit()

        bands = self.core.GetBands()

        #added_bands_ids = []
        #for scatt_id in self.plots):
        #    added_bands_ids.append[idBandsToidScatt(scatt_id)]

        self.digit_conn.Update()

        ncells = self.region["rows"] * self.region["cols"]
        if ncells > MAX_NCELLS:
            GError(_(parent=self.guiparent,
                     mmessage=_("Interactive Scatter Plot Tool can not be used.\n"
                                "Number of cells (rows*cols) <%d> in current region" 
                                "is higher than maximum limit <%d>.\n\n"
                                "You can reduce number of cells in current region using <g.region> command."
                       % (ncells, MAX_NCELLS))))
            return
        elif ncells > WARN_NCELLS:
            dlg = wx.MessageDialog(
                     parent=self.guiparent, 
                     message=_("Number of cells (rows*cols) <%d> in current region is "
                               "higher than recommended threshold <%d>.\n"
                               "It is strongly advised to reduce number of cells "
                               "in current region bellow recommend threshold.\n "
                               "It can be done by <g.region> command.\n\n" 
                               "Do you want to continue using "
                               "Interactive Scatter Plot Tool with this region?" 
                     % (ncells, WARN_NCELLS)), 
                     style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_WARNING)
            ret = dlg.ShowModal()
            if ret != wx.ID_YES:
                return
                
        dlg = AddScattPlotDialog(parent=self.guiparent, 
                                 bands=self.all_bands,
                                 check_bands_callback=self.CheckBands)

        if dlg.ShowModal() == wx.ID_OK:

            scatt_ids = []
            sel_bands = dlg.GetBands()

            for b_1, b_2 in sel_bands:
                transpose = False
                if b_1 > b_2:
                    transpose = True
                    tmp_band = b_2
                    b_2 = b_1
                    b_1 = tmp_band

                b_1_id = self.all_bands_to_bands[self.all_bands[b_1]]
                b_2_id = self.all_bands_to_bands[self.all_bands[b_2]]

                scatt_id = idBandsToidScatt(b_1_id, b_2_id, len(bands))
                if self.plots.has_key(scatt_id):
                    continue

                self.plots[scatt_id] = {'transpose' : transpose,
                                        'scatt' : None}
                scatt_ids.append(scatt_id)
            
            self._addScattPlot(scatt_ids)
            
        dlg.Destroy()
     
    def CheckBands(self, b_1, b_2):
        bands = self.core.GetBands()
        added_scatts_ids = self.plots.keys()

        b_1_id = self.all_bands_to_bands[self.all_bands[b_1]]
        b_2_id = self.all_bands_to_bands[self.all_bands[b_1]]

        scatt_id = idBandsToidScatt(b_1_id, b_2_id, len(bands))
        
        if scatt_id in added_scatts_ids:
            GWarning(parent=self.guiparent, 
                     message=_("Scatter plot with same band combination (regardless x y order) " 
                               "is already displayed."))
            return False

        b_1_name =  self.all_bands[b_1]
        b_2_name =  self.all_bands[b_2]

        b_1_i = self.bands_info[b_1_name]
        b_2_i = self.bands_info[b_2_name]

        err = ""
        for b in [b_1_name, b_2_name]:
            if self.bands_info[b] is None:
                err += _("Band <%s> is not CELL (integer) type.\n" % self.all_bands[b_1])
        if err:
            GMessage(parent=self.guiparent, 
                     message=_("Scatter plot cannot be added.\n" + err))
            return False

        mrange = b_1_i['range'] * b_2_i['range']
        if mrange > MAX_SCATT_SIZE:
            GWarning(parent=self.guiparent,
                     message=_("Scatter plot cannot be added.\n"
                               "Multiple of bands ranges <%s:%d * %s:%d = %d> " 
                               "is higher than maximum limit <%d>.\n" 
                                  % (b_1_name, b_1_i['range'], b_1_name, b_2_i['range'], 
                                  mrange, MAX_SCATT_SIZE)))
            return False
        elif mrange > WARN_SCATT_SIZE:
            dlg = wx.MessageDialog(parent = self.guiparent,
                     message=_("Multiple of bands ranges <%s:%d * %s:%d = %d> " 
                               "is higher than recommended limit <%d>.\n" 
                               "It is strongly advised to reduce range extend of bands" 
                               "(e. g. using r.rescale) bellow recommended threshold.\n\n"
                               "Do you really want to add this scatter plot?" 
                                % (b_1_name, b_1_i['range'], b_1_name, b_2_i['range'], 
                                   mrange, WARN_SCATT_SIZE)), 
                     style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_WARNING)
            ret = dlg.ShowModal()
            if ret != wx.ID_YES:
                return False  

        return True
        
    def _addScattPlot(self, scatt_ids):
        self.render_mgr.NewRunningProcess()
        self.thread.Run(callable=self.core.AddScattPlots,
                        scatt_ids=scatt_ids, ondone=self.AddScattPlotDone)

    def AddScattPlotDone(self, event):
        if not self.data_set:
            return

        scatt_ids = event.kwds['scatt_ids']
        for s_id in scatt_ids:
            trans = self.plots[s_id]['transpose']

            self.plots[s_id]['scatt'] = self.guiparent.NewScatterPlot(scatt_id=s_id, 
                                                                      transpose=trans)

            self.plots[s_id]['scatt'].plotClosed.connect(self.PlotClosed)
            self.plots[s_id]['scatt'].cursorMove.connect(
                                        lambda x, y, scatt_id: 
                                        self.cursorPlotMove.emit(x=x, y=y, 
                                                                 scatt_id=scatt_id))

            if self.plot_mode:
                self.plots[s_id]['scatt'].SetMode(self.plot_mode)
                self.plots[s_id]['scatt'].ZoomToExtend()

        self.render_mgr.RunningProcessDone()

    def PlotClosed(self, scatt_id):
        del self.plots[scatt_id]

    def SetPlotsMode(self, mode):

        self.plot_mode = mode
        for scatt in self.plots.itervalues():
            if scatt['scatt']:
                scatt['scatt'].SetMode(mode)

        self.modeSet.emit(mode = mode)

    def ActivateSelectionPolygonMode(self, activate):
        self.pol_sel_mode[0] = activate
        for scatt in self.plots.itervalues():
            if not scatt['scatt']:
                continue
            scatt['scatt'].SetSelectionPolygonMode(activate)
            if not activate and self.plot_mode not in ['zoom', 'pan', 'zoom_extend']:
                self.SetPlotsMode(None)

        self.render_mgr.RunningProcessDone()
        return activate

    def ProcessSelectionPolygons(self, process_mode):        
        scatts_polygons = {}        
        for scatt_id, scatt in self.plots.iteritems():
            if not scatt['scatt']:
                continue
            coords = scatt['scatt'].GetCoords()
            if coords is not None:
                scatts_polygons[scatt_id] = coords

        if not scatts_polygons:
            return

        value = 1
        if process_mode == 'remove':
            value = 0

        sel_cat_id = self.cats_mgr.GetSelectedCat()
        if not sel_cat_id:
            dlg = wx.MessageDialog(parent = self.guiparent,
                      message=_("In order to select arrea in scatter plot, "
                                  "you have to select class first.\n\n"
                                  "There is no class yet, "
                                  "do you want to create one?"),
                      caption=_("No class selected"),
                      style = wx.YES_NO)
            if dlg.ShowModal() == wx.ID_YES:
                self.iclass_conn.EmptyCategories()

        sel_cat_id = self.cats_mgr.GetSelectedCat()
        if not sel_cat_id:
            return

        for scatt in self.plots.itervalues():
            if scatt['scatt']:
                scatt['scatt'].SetEmpty()

        self.computingStarted.emit()

        self.render_mgr.NewRunningProcess()
        self.render_mgr.CategoryChanged(cat_ids=[sel_cat_id])
        self.render_mgr.CategoryCondsChanged(cat_ids=[sel_cat_id])

        self.thread.Run(callable = self.core.UpdateCategoryWithPolygons, 
                        cat_id = sel_cat_id,
                        scatts_pols = scatts_polygons,
                        value = value, ondone=self.SetEditCatDataDone)

    def SetEditCatDataDone(self, event):
        if not self.data_set:
            return

        self.render_mgr.RunningProcessDone()
        if event.exception:
            GError(_("Error occured during computation of scatter plot category:\n%s"), 
                      parent = self.guiparent, showTraceback = False)

        cat_id = event.ret
        self.iclass_conn.RenderCatRast(cat_id)
            
    def SettingsUpdated(self, chanaged_setts):
        self.render_mgr.RenderRequest()

        #['ellipses', 'show_ellips']
    def GetCategoriesManager(self):
        return self.cats_mgr

class PlotsRenderingManager:
    """!Manages rendering of scatter plot.

    @todo still space for optimalization
    """
    def __init__(self, scatt_mgr, cats_mgr, core):
        self.scatt_mgr = scatt_mgr
        self.cats_mgr = cats_mgr
        self.core = core
        self.scatts_dt, self.scatt_conds_dt = self.core.GetScattsData()

        self.runningProcesses = 0

        self.data_to_render = {}
        self.render_queue = []

        self.cat_ids = []
        self.cat_cond_ids = []

        self.renderingStarted = Signal("ScattsManager.renderingStarted")
        self.renderingFinished = Signal("ScattsManager.renderingFinished")

    def AddRenderRequest(self, scatts):
        for scatt_id, cat_ids in scatts:
            if not self.data_to_render.has_key[scatt_id]:
                self.data_to_render = cat_ids
            else:
                for c in cat_ids:
                    if c not in self.data_to_render[scatt_id]:
                         self.data_to_render[scatt_id].append(c) 

    def NewRunningProcess(self):
        self.runningProcesses += 1 

    def RunningProcessDone(self):
        self.runningProcesses -= 1
        if self.runningProcesses <= 1:
            self.RenderScattPlts()

    def RenderRequest(self):
        if self.runningProcesses <= 1:
            self.RenderScattPlts()

    def CategoryChanged(self, cat_ids):
        for c in cat_ids:
            if c not in self.cat_ids:
                self.cat_ids.append(c)

    def CategoryCondsChanged(self, cat_ids):
        for c in cat_ids:
            if c not in self.cat_cond_ids:
                self.cat_cond_ids.append(c)

    def RenderScattPlts(self, scatt_ids = None):
        if len(self.render_queue) > 1:
            return 
        
        self.renderingStarted.emit()
        self.render_queue.append(self.scatt_mgr.thread.GetId())

        cats_attrs = deepcopy(self.cats_mgr.GetCategoriesAttrs())
        cats = self.cats_mgr.GetCategories()[:]
        self.scatt_mgr.thread.Run(callable=self._renderscattplts, scatt_ids=scatt_ids,
                                  cats=cats, cats_attrs=cats_attrs,
                                  ondone=self.RenderingDone)

    def _renderscattplts(self, scatt_ids, cats, cats_attrs):
        cats.reverse()
        cats.insert(0, 0)
        for i_scatt_id, scatt in self.scatt_mgr.plots.items():
            if scatt_ids is not None and \
               i_scatt_id not in scatt_ids:
                continue
            if not scatt['scatt']:
                continue

            scatt_dt = self.scatts_dt.GetScatt(i_scatt_id)
            if self._showConfEllipses():
                ellipses_dt = self.scatts_dt.GetEllipses(i_scatt_id, cats_attrs)
            else:
                ellipses_dt = {}

            for c in scatt_dt.iterkeys():
                try:
                    self.cat_ids.remove(c)
                    scatt_dt[c]['render']=True
                except:
                    scatt_dt[c]['render']=False

            if self.scatt_mgr.pol_sel_mode[0]:
                self._getSelectedAreas(cats, i_scatt_id, scatt_dt, cats_attrs)

            scatt['scatt'].Plot(cats_order=cats,
                                scatts=scatt_dt, 
                                ellipses=ellipses_dt, 
                                styles=cats_attrs)

    def RenderingDone(self, event):
        self.render_queue.remove(event.pid)
        if not self.render_queue:    
            self.renderingFinished.emit()

    def _getSelectedAreas(self, cats_order, scatt_id, scatt_dt, cats_attrs):

        cat_id = self.cats_mgr.GetSelectedCat()
        if not cat_id:
            return

        sel_a_cat_id = -1        

        s = self.scatt_conds_dt.GetScatt(scatt_id, [cat_id])
        if not s:
            return

        cats_order.append(sel_a_cat_id)

        col = UserSettings.Get(group='scatt', 
                                 key='selection', 
                                 subkey='sel_area')

        col = ":".join(map(str, col))
        opac = UserSettings.Get(group='scatt', 
                                key='selection', 
                                subkey='sel_area_opacty') / 100.0

        cats_attrs[sel_a_cat_id] = {'color' : col,
                                    'opacity' : opac,
                                    'show' : True}

        scatt_dt[sel_a_cat_id] = s[cat_id]
        
        scatt_dt[sel_a_cat_id]['render'] = False
        if cat_id in self.cat_cond_ids:
            scatt_dt[sel_a_cat_id]['render'] = True
            self.cat_cond_ids.remove(cat_id)

    def _showConfEllipses(self):
        return UserSettings.Get(group='scatt', 
                                key="ellipses", 
                                subkey="show_ellips")

class CategoriesManager:
    """!Manages categories list of scatter plot.
    """
    def __init__(self, scatt_mgr, core):

        self.core = core
        self.scatt_mgr = scatt_mgr

        self.cats = {}
        self.cats_ids = []

        self.sel_cat_id = None

        self.exportRaster = None

        self.initialized = Signal('CategoriesManager.initialized')
        self.setCategoryAttrs = Signal('CategoriesManager.setCategoryAttrs')
        self.deletedCategory = Signal('CategoriesManager.deletedCategory')
        self.addedCategory = Signal('CategoriesManager.addedCategory')  

    def ChangePosition(self, cat_id, new_pos):
        if new_pos >= len(self.cats_ids):
            return False

        try:
            pos = self.cats_ids.index(cat_id)
        except:
            return False

        if pos > new_pos:
            pos -= 1

        self.cats_ids.remove(cat_id)

        self.cats_ids.insert(new_pos, cat_id)

        self.scatt_mgr.render_mgr.RenderRequest()
        return True

    def _addCategory(self, cat_id):
        self.scatt_mgr.thread.Run(callable=self.core.AddCategory, 
                                  cat_id=cat_id)

    def SetData(self):

        if not self.scatt_mgr.data_set:
            return

        for cat_id in self.cats_ids:
            self.scatt_mgr.thread.Run(callable=self.core.AddCategory, 
                                      cat_id=cat_id)

    def AddCategory(self, cat_id = None, name = None, color = None, nstd = None):

        if cat_id is None:
            if self.cats_ids:
                cat_id = max(self.cats_ids) + 1
            else:
                cat_id = 1

        if self.scatt_mgr.data_set:
            self.scatt_mgr.thread.Run(callable = self.core.AddCategory, 
                                      cat_id = cat_id)
            #TODO check number of cats
            #if ret < 0: #TODO
            #    return -1;

        self.cats[cat_id] = {
                                'name' : 'class_%d' % cat_id,
                                'color' : "0:0:0",
                                'opacity' : 1.0,
                                'show' : True,
                                'nstd' : 1.0,
                            }

        self.cats_ids.insert(0, cat_id)

        if name is not None:
            self.cats[cat_id]["name"] = name
   
        if color is not None:
            self.cats[cat_id]["color"] = color

        if nstd is not None:
            self.cats[cat_id]["nstd"] = nstd

        self.addedCategory.emit(cat_id = cat_id,
                                name = self.cats[cat_id]["name"], 
                                color = self.cats[cat_id]["color"] )
        return cat_id

    def SetCategoryAttrs(self, cat_id, attrs_dict):
        render = False
        update_cat_rast = []

        for k, v in attrs_dict.iteritems():
            if not render and k in ['color', 'opacity', 'show', 'nstd']:
                render = True
            if k in ['color', 'name']:
                update_cat_rast.append(k)

            self.cats[cat_id][k] = v

        if render:
            self.scatt_mgr.render_mgr.CategoryChanged(cat_ids=[cat_id])
            self.scatt_mgr.render_mgr.RenderRequest()
        
        if update_cat_rast:
            self.scatt_mgr.iclass_conn.UpdateCategoryRaster(cat_id, update_cat_rast)

        self.setCategoryAttrs.emit(cat_id = cat_id, attrs_dict = attrs_dict)

    def DeleteCategory(self, cat_id):

        if self.scatt_mgr.data_set:
            self.scatt_mgr.thread.Run(callable = self.core.DeleteCategory, 
                                      cat_id = cat_id)
        del self.cats[cat_id]
        self.cats_ids.remove(cat_id)

        self.deletedCategory.emit(cat_id = cat_id)

    #TODO emit event?
    def SetSelectedCat(self, cat_id):
        self.sel_cat_id = cat_id
        if self.scatt_mgr.pol_sel_mode[0]:
            self.scatt_mgr.render_mgr.RenderRequest()

    def GetSelectedCat(self):
        return self.sel_cat_id

    def GetCategoryAttrs(self, cat_id):
        #TODO is mutable
        return self.cats[cat_id]

    def GetCategoriesAttrs(self):
        #TODO is mutable
        return self.cats
     
    def GetCategories(self):
        return self.cats_ids[:]

    def SetCategoryPosition(self):
        if newindex > oldindex:
            newindex -= 1
        
        self.cats_ids.insert(newindex, self.cats_ids.pop(oldindex))

    def ExportCatRast(self, cat_id):

        cat_attrs = self.GetCategoryAttrs(cat_id)

        dlg = ExportCategoryRaster(parent=self.scatt_mgr.guiparent, 
                                   rasterName=self.exportRaster, 
                                   title=_("Export scatter plot raster of class <%s>")
                                            % cat_attrs['name'])
        
        if dlg.ShowModal() == wx.ID_OK:
            self.exportCatRast = dlg.GetRasterName()
            dlg.Destroy()

            self.scatt_mgr.thread.Run(callable=self.core.ExportCatRast,
                                      userdata={'name' : cat_attrs['name']},
                                      cat_id=cat_id,
                                      rast_name=self.exportCatRast, 
                                      ondone=self.OnExportCatRastDone)

    def OnExportCatRastDone(self, event):
        ret, err = event.ret
        if ret == 0:
            cat_attrs = self.GetCategoryAttrs(event.kwds['cat_id'])
            GMessage(_("Scatter plot raster of class <%s> exported to raster map <%s>.") % 
                      (event.userdata['name'], event.kwds['rast_name']))
        else:
            GMessage(_("Export of scatter plot raster of class <%s> to map <%s> failed.\n%s") % 
                      (event.userdata['name'], event.kwds['rast_name'], err))


class IMapWinDigitConnection:
    """!Manage communication of the scatter plot with digitizer in mapwindow (does not work).
    """
    def Update(self):
        pass

    def SetData(self):
        pass

class IClassDigitConnection:
    """!Manages communication of the scatter plot with digitizer in wx.iclass.
    """
    def __init__(self, scatt_mgr, mapWin, scatt_rast_updater):
        self.mapWin = mapWin
        self.vectMap = None
        self.scatt_rast_updater = scatt_rast_updater
        self.scatt_mgr = scatt_mgr
        self.cats_mgr = scatt_mgr.cats_mgr

        self.cats_to_update = []
        self.pids = {'mapwin_conn' : []}

        self.thread = self.scatt_mgr.thread

        #TODO
        self.mapWin.parent.toolbars["vdigit"].editingStarted.connect(self.DigitDataChanged)

    def Update(self):
        self.thread.Run(callable=self.scatt_rast_updater.SyncWithMap)

    def SetData(self):
        self.cats_to_update = []
        self.pids = {'mapwin_conn' : []}

    def _connectSignals(self):
        self.digit.featureAdded.connect(self.AddFeature)
        self.digit.areasDeleted.connect(self.DeleteAreas)
        self.digit.featuresDeleted.connect(self.DeleteAreas)
        self.digit.vertexMoved.connect(self.EditedFeature)
        self.digit.vertexRemoved.connect(self.EditedFeature)
        self.digit.lineEdited.connect(self.EditedFeature)
        self.digit.featuresMoved.connect(self.EditedFeature)

    def AddFeature(self, new_bboxs, new_areas_cats):
        if not self.scatt_mgr.data_set:
            return
        self.scatt_mgr.computingStarted.emit()

        self.pids['mapwin_conn'].append(self.thread.GetId())
        self.thread.Run(callable = self.scatt_rast_updater.EditedFeature, 
                        new_bboxs = new_bboxs, 
                        old_bboxs = [], 
                        old_areas_cats = [],
                        new_areas_cats = new_areas_cats,
                        ondone=self.OnDone)

    def DeleteAreas(self, old_bboxs, old_areas_cats):
        if not self.scatt_mgr.data_set:
            return
        self.scatt_mgr.computingStarted.emit()

        self.pids['mapwin_conn'].append(self.thread.GetId())
        self.thread.Run(callable = self.scatt_rast_updater.EditedFeature, 
                        new_bboxs = [], 
                        old_bboxs = old_bboxs, 
                        old_areas_cats = old_areas_cats,
                        new_areas_cats = [],
                        ondone=self.OnDone)

    def EditedFeature(self, new_bboxs, new_areas_cats, old_bboxs, old_areas_cats):
        if not self.scatt_mgr.data_set:
            return
        self.scatt_mgr.computingStarted.emit()

        self.pids['mapwin_conn'].append(self.thread.GetId())
        self.thread.Run(callable = self.scatt_rast_updater.EditedFeature, 
                        new_bboxs = new_bboxs, 
                        old_bboxs = old_bboxs, 
                        old_areas_cats = old_areas_cats,
                        new_areas_cats = new_areas_cats,
                        ondone=self.OnDone)

    def DigitDataChanged(self, vectMap, digit):

        self.digit = digit
        self.vectMap = vectMap

        self.digit.EmitSignals(emit = True)

        self.scatt_rast_updater.SetVectMap(vectMap)

        self._connectSignals()

    def OnDone(self, event):
        if not self.scatt_mgr.data_set:
            return
        self.pids['mapwin_conn'].remove(event.pid)
        updated_cats = event.ret
        for cat in updated_cats:
            if cat not in  self.cats_to_update:
                self.cats_to_update.append(cat)

        if not self.pids['mapwin_conn']:
            self.thread.Run(callable = self.scatt_mgr.core.ComputeCatsScatts, 
                            cats_ids = self.cats_to_update[:], ondone=self.Render)
            del self.cats_to_update[:]

    def Render(self, event):
        self.scatt_mgr.render_mgr.RenderScattPlts()

class IMapDispConnection:
    """!Manage comunication of the scatter plot with mapdisplay in mapwindow.
    """
    def __init__(self, scatt_mgr, cats_mgr, giface):
        self.scatt_mgr = scatt_mgr
        self.cats_mgr = cats_mgr
        self.set_g = {'group' :  None, 'subg' :  None}
        self.giface = giface
        self.added_cats_rasts = {}

    def SetData(self):

        dlg = IClassGroupDialog(self.scatt_mgr.guiparent, 
                                group=self.set_g['group'],
                                subgroup=self.set_g['subg'])
        
        bands = []
        while True:
            if dlg.ShowModal() == wx.ID_OK:
                
                bands = dlg.GetGroupBandsErr(parent=self.scatt_mgr.guiparent)
                if bands:
                    name, s = dlg.GetData()
                    group = grass.find_file(name = name, element = 'group')
                    self.set_g['group'] = group['name']
                    self.set_g['subg'] = s

                    break
            else: 
                break
        
        dlg.Destroy()
        self.added_cats_rasts = {}

        if bands:
            self.scatt_mgr.SetBands(bands)

    def EmptyCategories(self):
        return None

    def UpdateCategoryRaster(self, cat_id, attrs, render = True):

        cat_rast = self.scatt_mgr.core.GetCatRast(cat_id)
        if not grass.find_file(cat_rast, element = 'cell', mapset = '.')['file']:
            return
        cats_attrs = self.cats_mgr.GetCategoryAttrs(cat_id)

        if "color" in attrs:
            ret, err_msg = RunCommand('r.colors',
                                      map=cat_rast,
                                      rules="-",
                                      stdin="1 %s" % cats_attrs["color"],
                                      getErrorMsg=True)

            if ret != 0:
                GError("r.colors failed\n%s" % err_msg)
            if render:
                self.giface.updateMap.emit()

        if "name" in attrs:
            #TODO hack
            self.giface.GetLayerList()._tree.SetItemText(self.added_cats_rasts[cat_id], 
                                                         cats_attrs['name'])
            cats_attrs["name"]

    def RenderCatRast(self, cat_id):

        if not cat_id in self.added_cats_rasts.iterkeys():
            cat_rast = self.scatt_mgr.core.GetCatRast(cat_id)

            cat_name = self.cats_mgr.GetCategoryAttrs(cat_id)['name']
            self.UpdateCategoryRaster(cat_id, ['color'], render = False)

            cmd = ['d.rast', 'map=%s' % cat_rast]
            #TODO HACK
            layer = self.giface.GetLayerList()._tree.AddLayer(ltype="raster",
                                                         lname=cat_name,
                                                         lcmd=cmd,
                                                         lchecked=True)
            self.added_cats_rasts[cat_id] = layer
        else: #TODO settings
            self.giface.updateMap.emit()

class IClassConnection:
    """!Manage comunication of the scatter plot with mapdisplay in wx.iclass.
    """
    def __init__(self, scatt_mgr, iclass_frame, cats_mgr):
        self.iclass_frame = iclass_frame
        self.stats_data = self.iclass_frame.stats_data
        self.cats_mgr = cats_mgr
        self.scatt_mgr= scatt_mgr
        self.added_cats_rasts = []

        self.stats_data.statisticsAdded.connect(self.AddCategory)
        self.stats_data.statisticsDeleted.connect(self.DeleteCategory)
        self.stats_data.allStatisticsDeleted.connect(self.DeletAllCategories)
        self.stats_data.statisticsSet.connect(self.SetCategory)

        self.iclass_frame.groupSet.connect(self.GroupSet)

        self.cats_mgr.setCategoryAttrs.connect(self.SetStatistics)
        self.cats_mgr.deletedCategory.connect(self.DeleteStatistics)
        self.cats_mgr.addedCategory.connect(self.AddStatistics)

        self.iclass_frame.categoryChanged.connect(self.CategoryChanged)

        self.SyncCats()

    def UpdateCategoryRaster(self, cat_id, attrs, render = True):
        if not self.scatt_mgr.data_set:
            return

        cat_rast = self.scatt_mgr.core.GetCatRast(cat_id)
        if not cat_rast:
            return

        if not grass.find_file(cat_rast, element = 'cell', mapset = '.')['file']:
            return
        cats_attrs = self.cats_mgr.GetCategoryAttrs(cat_id)
        train_mgr, preview_mgr = self.iclass_frame.GetMapManagers()

        if "color" in attrs:
            ret, err_msg = RunCommand('r.colors',
                                      map=cat_rast,
                                      rules="-",
                                      stdin="1 %s" % cats_attrs["color"],
                                      getErrorMsg=True)

            if ret != 0:
                GError("r.colors failed\n%s" % err_msg)
            if render:
                train_mgr.Render()

        if "name" in attrs:
            cat_rast = self.scatt_mgr.core.GetCatRast(cat_id)

            train_mgr.SetAlias(original=cat_rast, alias=cats_attrs['name'])
            cats_attrs["name"]

    def RenderCatRast(self, cat_id):

        train_mgr, preview_mgr = self.iclass_frame.GetMapManagers()
        if not cat_id in self.added_cats_rasts:
            cat_rast = self.scatt_mgr.core.GetCatRast(cat_id)

            cat_name = self.cats_mgr.GetCategoryAttrs(cat_id)['name']
            self.UpdateCategoryRaster(cat_id, ['color'], render = False)
            train_mgr.AddLayer(cat_rast, alias = cat_name)

            self.added_cats_rasts.append(cat_id)
        else: #TODO settings
            train_mgr.Render()

    def SetData(self):
        self.iclass_frame.AddBands()
        self.added_cats_rasts = []

    def EmptyCategories(self):
        self.iclass_frame.OnCategoryManager(None)

    def SyncCats(self, cats_ids = None):
        self.cats_mgr.addedCategory.disconnect(self.AddStatistics)
        cats = self.stats_data.GetCategories()
        for c in cats:
            if cats_ids and c not in cats_ids:
                continue
            stats = self.stats_data.GetStatistics(c)
            self.cats_mgr.AddCategory(c, stats.name, stats.color, stats.nstd)
        self.cats_mgr.addedCategory.connect(self.AddStatistics)

    def CategoryChanged(self, cat):
        self.cats_mgr.SetSelectedCat(cat) 

    def AddCategory(self, cat, name, color):
        self.cats_mgr.addedCategory.disconnect(self.AddStatistics)
        stats = self.stats_data.GetStatistics(cat)
        self.cats_mgr.AddCategory(cat_id = cat, name = name, color = color, nstd = stats.nstd)
        self.cats_mgr.addedCategory.connect(self.AddStatistics)

    def DeleteCategory(self, cat):
        self.cats_mgr.deletedCategory.disconnect(self.DeleteStatistics)
        self.cats_mgr.DeleteCategory(cat)
        self.cats_mgr.deletedCategory.connect(self.DeleteStatistics)

    def DeletAllCategories(self):

        self.cats_mgr.deletedCategory.disconnect(self.DeleteStatistics)
        cats = self.stats_data.GetCategories()
        for c in cats:
            self.cats_mgr.DeleteCategory(c)
        self.cats_mgr.deletedCategory.connect(self.DeleteStatistics)

    def SetCategory(self, cat, stats):

        self.cats_mgr.setCategoryAttrs.disconnect(self.SetStatistics)
        cats_attr = {}

        for attr in ['name', 'color', 'nstd']:
            if stats.has_key(attr):
                cats_attr[attr] = stats[attr]

        if cats_attr:
            self.cats_mgr.SetCategoryAttrs(cat, cats_attr)
        self.cats_mgr.setCategoryAttrs.connect(self.SetStatistics)


    def SetStatistics(self, cat_id, attrs_dict):
        self.stats_data.statisticsSet.disconnect(self.SetCategory)
        self.stats_data.GetStatistics(cat_id).SetStatistics(attrs_dict)
        self.stats_data.statisticsSet.connect(self.SetCategory)

    def AddStatistics(self, cat_id, name, color):
        self.stats_data.statisticsAdded.disconnect(self.AddCategory)
        self.stats_data.AddStatistics(cat_id, name, color)
        self.stats_data.statisticsAdded.connect(self.AddCategory)

    def DeleteStatistics(self, cat_id):
        self.stats_data.statisticsDeleted.disconnect(self.DeleteCategory)
        self.stats_data.DeleteStatistics(cat_id)
        self.stats_data.statisticsDeleted.connect(self.DeleteCategory)

    def GroupSet(self, group, subgroup):
        kwargs = {}
        if subgroup:
            kwargs['subgroup'] = subgroup

        res = RunCommand('i.group',
                         flags = 'g',
                         group = group,
                         read = True, **kwargs).strip()

        if res.split('\n')[0]:
            bands = res.split('\n')
            self.scatt_mgr.SetBands(bands)

class gThread(threading.Thread, wx.EvtHandler):
    """!Thread for scatter plot backend"""
    requestId = 0

    def __init__(self, requestQ=None, resultQ=None, **kwds):
        wx.EvtHandler.__init__(self)
        self.terminate = False

        threading.Thread.__init__(self, **kwds)

        if requestQ is None:
            self.requestQ = Queue.Queue()
        else:
            self.requestQ = requestQ

        if resultQ is None:
            self.resultQ = Queue.Queue()
        else:
            self.resultQ = resultQ

        #self.setDaemon(True)

        self.Bind(EVT_CMD_DONE, self.OnDone)
        self.start()

    def Run(self, *args, **kwds):
        """!Run command in queue

        @param args unnamed command arguments
        @param kwds named command arguments

        @return request id in queue
        """
        gThread.requestId += 1
        self.requestQ.put((gThread.requestId, args, kwds))

        return gThread.requestId

    def GetId(self):
         """!Get id for next command"""
         return gThread.requestId + 1

    def SetId(self, id):
        """!Set starting id"""
        gThread.requestId = id

    def run(self):
        while True:
            requestId, args, kwds = self.requestQ.get()
            for key in ('callable', 'ondone', 'userdata'):
                if key in kwds:
                    vars()[key] = kwds[key]
                    del kwds[key]
                else:
                    vars()[key] = None

            requestTime = time.time()

            ret = None
            exception = None
            time.sleep(.1)

            if self.terminate:
                return

            ret = vars()['callable'](*args, **kwds)

            if self.terminate:
                return
            #except Exception as e:
            #    exception  = e;

            self.resultQ.put((requestId, ret))

            event = wxCmdDone(ondone=vars()['ondone'],
                              kwds=kwds,
                              args=args, #TODO expand args to kwds
                              ret=ret,
                              exception=exception,
                              userdata=vars()['userdata'],
                              pid=requestId)

            # send event
            wx.PostEvent(self, event)

    def OnDone(self, event):
        if event.ondone:
            event.ondone(event)

    def Terminate(self):
        """!Abort command(s)"""
        self.terminate = True
