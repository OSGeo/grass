"""!
@package nviz_mapdisp.py

@brief wxGUI 3D view mode (map canvas)

This module implements 3D visualization mode for map display.

List of classes:
 - NvizThread
 - GLWindow

(C) 2008-2010 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
"""

import os
import sys
import time
import copy
import math

from threading import Thread

import wx
import wx.lib.scrolledpanel as scrolled
from wx.lib.newevent import NewEvent
from wx import glcanvas

import gcmd
import globalvar
from debug import Debug as Debug
from mapdisp_window import MapWindow
from goutput import wxCmdOutput
from preferences import globalSettings as UserSettings
from workspace import Nviz as NvizDefault

import wxnviz

wxUpdateProperties, EVT_UPDATE_PROP  = NewEvent()
wxUpdateView,       EVT_UPDATE_VIEW  = NewEvent()
wxUpdateLight,      EVT_UPDATE_LIGHT = NewEvent()

class NvizThread(Thread):
    def __init__(self, log, progressbar, window):
        Thread.__init__(self)
        
        self.log = log
        self.progressbar = progressbar
        self.window = window
        
        self._display = None
        
        self.setDaemon(True)
        
    def run(self):
        self._display = wxnviz.Nviz(self.log)
        
    def GetDisplay(self):
        """!Get display instance"""
        return self._display
    
class GLWindow(MapWindow, glcanvas.GLCanvas):
    """!OpenGL canvas for Map Display Window"""
    def __init__(self, parent, id = wx.ID_ANY,
                 Map = None, tree = None, lmgr = None):
        self.parent = parent # MapFrame
        
        glcanvas.GLCanvas.__init__(self, parent, id)
        MapWindow.__init__(self, parent, id, 
                           Map, tree, lmgr)
        self.Hide()
        
        self.init = False
        self.initView = False
        
        # render mode 
        self.render = { 'quick' : False,
                        # do not render vector lines in quick mode
                        'vlines' : False,
                        'vpoints' : False }
        
        # list of loaded map layers (layer tree items)
        self.layers  = list()
        # list of query points
        self.qpoints = list()
        
        #
        # use display region instead of computational
        #
        os.environ['GRASS_REGION'] = self.Map.SetRegion()
        
        #
        # create nviz instance
        #
        if self.lmgr:
            logerr = self.lmgr.goutput.cmd_stderr
            logmsg = self.lmgr.goutput.cmd_output
        else:
            logerr = logmsg = None
        self.nvizThread = NvizThread(logerr,
                                     self.parent.statusbarWin['progress'],
                                     logmsg)
        self.nvizThread.start()
        time.sleep(.1)
        self._display = self.nvizThread.GetDisplay()
        
        # GRASS_REGION needed only for initialization
        del os.environ['GRASS_REGION']
        
        self.img = wx.Image(self.Map.mapfile, wx.BITMAP_TYPE_ANY)
        
        #
        # default values
        #
        self.view = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'view')) # copy
        self.iview = UserSettings.Get(group = 'nviz', key = 'view', internal = True)
        
        self.nvizDefault = NvizDefault()
        self.light = copy.deepcopy(UserSettings.Get(group = 'nviz', key = 'light')) # copy
        
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.OnEraseBackground)
        self.Bind(wx.EVT_SIZE,             self.OnSize)
        self.Bind(wx.EVT_PAINT,            self.OnPaint)
        self.Bind(wx.EVT_LEFT_UP,          self.OnLeftUp)
        self.Bind(wx.EVT_MOUSE_EVENTS,     self.OnMouseAction)
        self.Bind(wx.EVT_MOTION,           self.OnMotion)
        
        self.Bind(EVT_UPDATE_PROP,  self.UpdateMapObjProperties)
        self.Bind(EVT_UPDATE_VIEW,  self.UpdateView)
        self.Bind(EVT_UPDATE_LIGHT, self.UpdateLight)
        
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        
    def OnClose(self, event):
        # cleanup when window actually closes (on quit) and not just is hidden
        self.Reset()
        
    def OnEraseBackground(self, event):
        pass # do nothing, to avoid flashing on MSW
    
    def OnSize(self, event):
        size = self.GetClientSize()
        if self.GetContext():
            Debug.msg(3, "GLCanvas.OnSize(): w = %d, h = %d" % \
                      (size.width, size.height))
            self.SetCurrent()
            self._display.ResizeWindow(size.width,
                                       size.height)
        
        event.Skip()
        
    def OnPaint(self, event):
        Debug.msg(1, "GLCanvas.OnPaint()")
        
        dc = wx.PaintDC(self)
        self.SetCurrent()
        
        if not self.initView:
            self._display.InitView()
            self.initView = True
        
        self.LoadDataLayers()
        self.UnloadDataLayers()
        
        if not self.init:
            self.ResetView()
            
            if hasattr(self.lmgr, "nviz"):
                self.lmgr.nviz.UpdatePage('view')
                self.lmgr.nviz.UpdatePage('light')
                layer = self.GetSelectedLayer()
                if layer:
                    if layer.type ==  'raster':
                        self.lmgr.nviz.UpdatePage('surface')
                        self.lmgr.nviz.UpdatePage('fringe')
                    elif layer.type ==  'vector':
                        self.lmgr.nviz.UpdatePage('vector')
                
                ### self.lmgr.nviz.UpdateSettings()
                
                # update widgets
                win = self.lmgr.nviz.FindWindowById( \
                    self.lmgr.nviz.win['vector']['lines']['surface'])
                win.SetItems(self.GetLayerNames('raster'))
            
            self.init = True
        
        self.UpdateMap()
                
    def OnMouseAction(self, event):
        # change perspective with mouse wheel
        wheel = event.GetWheelRotation()
        
        if wheel !=  0:
            current  = event.GetPositionTuple()[:]
            Debug.msg (5, "GLWindow.OnMouseMotion(): wheel = %d" % wheel)
            prev_value = self.view['persp']['value']
            if wheel > 0:
                value = -1 * self.view['persp']['step']
            else:
                value = self.view['persp']['step']
            self.view['persp']['value'] +=  value
            if self.view['persp']['value'] < 1:
                self.view['persp']['value'] = 1
            elif self.view['persp']['value'] > 100:
                self.view['persp']['value'] = 100
            
            if prev_value !=  self.view['persp']['value']:
                if hasattr(self.lmgr, "nviz"):
                    self.lmgr.nviz.UpdateSettings()
                    
                    self._display.SetView(self.view['position']['x'], self.view['position']['y'],
                                          self.iview['height']['value'],
                                          self.view['persp']['value'],
                                          self.view['twist']['value'])
                
                # redraw map
                self.OnPaint(None)
                
                # update statusbar
                ### self.parent.StatusbarUpdate()
        
        event.Skip()

    def Pixel2Cell(self, (x, y)):
        """!Convert image coordinates to real word coordinates

        @param x, y image coordinates
        
        @return easting, northing
        @return None on error
        """
        size = self.GetClientSize()
        # UL -> LL
        sid, x, y, z = self._display.GetPointOnSurface(x, y)
        
        if not sid:
            return None
        
        return (x, y)
    
    def OnLeftUp(self, event):
        self.ReleaseMouse()
        if self.mouse["use"] == "query":
            result = self._display.QueryMap(event.GetX(), event.GetY())
            log = self.lmgr.goutput
            if result:
                self.qpoints.append((result['x'], result['y'], result['z']))
                log.WriteLog("%-30s: %.3f" % (_("Easting"),   result['x']))
                log.WriteLog("%-30s: %.3f" % (_("Northing"),  result['y']))
                log.WriteLog("%-30s: %.3f" % (_("Elevation"), result['z']))
                log.WriteLog("%-30s: %s" % (_("Surface map elevation"), result['elevation']))
                log.WriteLog("%-30s: %s" % (_("Surface map color"), result['color']))
                if len(self.qpoints) > 1:
                    prev = self.qpoints[-2]
                    curr = self.qpoints[-1]
                    dxy = math.sqrt(pow(prev[0]-curr[0], 2) +
                                    pow(prev[1]-curr[1], 2))
                    dxyz = math.sqrt(pow(prev[0]-curr[0], 2) +
                                     pow(prev[1]-curr[1], 2) +
                                     pow(prev[2]-curr[2], 2))
                    log.WriteLog("%-30s: %.3f" % (_("XY distance from previous"), dxy))
                    log.WriteLog("%-30s: %.3f" % (_("XYZ distance from previous"), dxyz))
                    log.WriteLog("%-30s: %.3f" % (_("Distance along surface"),
                                                self._display.GetDistanceAlongSurface(result['id'],
                                                                                      (curr[0], curr[1]),
                                                                                      (prev[0], prev[1]),
                                                                                      useExag = False)))
                    log.WriteLog("%-30s: %.3f" % (_("Distance along exag. surface"),
                                                self._display.GetDistanceAlongSurface(result['id'],
                                                                                      (curr[0], curr[1]),
                                                                                      (prev[0], prev[1]),
                                                                                      useExag = True)))
                log.WriteLog('-' * 80)
            else:
                log.WriteLog(_("No point on surface"))
                log.WriteLog('-' * 80)
        
    def UpdateView(self, event):
        """!Change view settings"""
        data = self.view
        self._display.SetView(data['position']['x'], data['position']['y'],
                              self.iview['height']['value'],
                              data['persp']['value'],
                              data['twist']['value'])
        
        if event and event.zExag and data['z-exag'].has_key('value'):
            self._display.SetZExag(data['z-exag']['value'])
        
        if event:
            event.Skip()

    def UpdateLight(self, event):
        """!Change light settings"""
        data = self.light
        self._display.SetLight(x = data['position']['x'], y = data['position']['y'],
                               z = data['position']['z'], color = data['color'],
                               bright = data['bright'] / 100.,
                               ambient = data['ambient'] / 100.)
        self._display.DrawLightingModel()
        
    def UpdateMap(self, render = True):
        """!Updates the canvas anytime there is a change to the
        underlaying images or to the geometry of the canvas.
        
        @param render re-render map composition
        """
        start = time.clock()
        
        self.resize = False
        
        if self.render['quick'] is False:
            self.parent.statusbarWin['progress'].Show()
            self.parent.statusbarWin['progress'].SetRange(2)
            self.parent.statusbarWin['progress'].SetValue(0)
        
        if self.render['quick'] is False:
            self.parent.statusbarWin['progress'].SetValue(1)
            self._display.Draw(False, -1)
        elif self.render['quick'] is True:
            # quick
            mode = wxnviz.DRAW_QUICK_SURFACE | wxnviz.DRAW_QUICK_VOLUME
            if self.render['vlines']:
                mode |=  wxnviz.DRAW_QUICK_VLINES
            if self.render['vpoints']:
                mode |=  wxnviz.DRAW_QUICK_VPOINTS
            self._display.Draw(True, mode)
        else: # None -> reuse last rendered image
            pass # TODO
        
        self.SwapBuffers()
        
        stop = time.clock()
        
        if self.render['quick'] is False:
            self.parent.statusbarWin['progress'].SetValue(2)
            # hide process bar
            self.parent.statusbarWin['progress'].Hide()
        
        Debug.msg(3, "GLWindow.UpdateMap(): quick = %d, -> time = %g" % \
                      (self.render['quick'], (stop-start)))
        
    def EraseMap(self):
        """!Erase the canvas
        """
        self._display.EraseMap()
        self.SwapBuffers()
        
    def IsLoaded(self, item):
        """!Check if layer (item) is already loaded
        
        @param item layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']
        data = self.tree.GetPyData(item)[0]['nviz']
        
        if not data:
            return 0
        
        if layer.type ==  'raster':
            if not data['surface'].has_key('object'):
                return 0
        elif layer.type ==  'vector':
            if not data['vlines'].has_key('object') and \
                    not data['points'].has_key('object'):
                return 0
        
        return 1

    def _GetDataLayers(self, item, litems):
        """!Return get list of enabled map layers"""
        # load raster & vector maps
        while item and item.IsOk():
            type = self.tree.GetPyData(item)[0]['type']
            if type ==  'group':
                subItem = self.tree.GetFirstChild(item)[0]
                self._GetDataLayers(subItem, litems)
                item = self.tree.GetNextSibling(item)
                
            if not item.IsChecked() or \
                    type not in ('raster', 'vector', '3d-raster'):
                item = self.tree.GetNextSibling(item)
                continue
            
            litems.append(item)
            
            item = self.tree.GetNextSibling(item)
        
    def LoadDataLayers(self):
        """!Load raster/vector from current layer tree
        
        @todo volumes
        """
        if not self.tree:
            return
        
        listOfItems = []
        item = self.tree.GetFirstChild(self.tree.root)[0]
        self._GetDataLayers(item, listOfItems)
        
        start = time.time()
        
        while(len(listOfItems) > 0):
            item = listOfItems.pop()
            type = self.tree.GetPyData(item)[0]['type']
            if item in self.layers:
                continue
            try:
                if type ==  'raster':
                    self.LoadRaster(item)
                elif type ==  '3d-raster':
                    self.LoadRaster3d(item)
                elif type ==  'vector':
                    # data = self.tree.GetPyData(item)[0]['nviz']
                    # vecType = []
                    # if data and data.has_key('vector'):
                    #     for v in ('lines', 'points'):
                    #         if data['vector'][v]:
                    #             vecType.append(v)
                    layer = self.tree.GetPyData(item)[0]['maplayer']
                    npoints, nlines, nfeatures = self.lmgr.nviz.VectorInfo(layer)
                    if npoints > 0:
                        self.LoadVector(item, points = True)
                    if nlines > 0:
                        self.LoadVector(item, points = False)
            except gcmd.GException, e:
                GError(parent = self,
                       message = e)
            self.init = False
        
        stop = time.time()
        
        Debug.msg(3, "GLWindow.LoadDataLayers(): time = %f" % (stop-start))
                
    def UnloadDataLayers(self):
        """!Unload any layers that have been deleted from layer tree"""
        if not self.tree:
            return
        
        listOfItems = []
        item = self.tree.GetFirstChild(self.tree.root)[0]
        self._GetDataLayers(item, listOfItems)
        
        start = time.time()
        
        for layer in self.layers:
            if layer not in listOfItems:
                ltype = self.tree.GetPyData(layer)[0]['type']
                try:
                    if ltype ==  'raster':
                        self.UnloadRaster(layer)
                    elif ltype ==  '3d-raster':
                        self.UnloadRaster3d(layer) 
                    elif ltype ==  'vector':
                        data = self.tree.GetPyData(layer)[0]['nviz']
                        vecType = []
                        if data and data.has_key('vector'):
                            for v in ('lines', 'points'):
                                if data['vector'][v]:
                                    vecType.append(v)
                        self.UnloadVector(layer, vecType)
                    
                    self.UpdateView(None)
                except gcmd.GException, e:
                    gcmd.GError(parent = self,
                                message = e)
                
                self.lmgr.nviz.UpdateSettings()        
        
        stop = time.time()
        
        Debug.msg(3, "GLWindow.UnloadDataLayers(): time = %f" % (stop-start))        

    def SetMapObjProperties(self, item, id, nvizType):
        """!Set map object properties
        
        Properties must be afterwards updated by
        UpdateMapObjProperties().
        
        @param item layer item
        @param id nviz layer id (or -1)
        @param nvizType nviz data type (surface, points, vector)
        """
        type = self.tree.GetPyData(item)[0]['maplayer'].type
        # reference to original layer properties (can be None)
        data = self.tree.GetPyData(item)[0]['nviz']
        if data is None:
            # init data structure
            self.tree.GetPyData(item)[0]['nviz'] = {}
            data = self.tree.GetPyData(item)[0]['nviz']
            
            if type ==  'raster':
                # reset to default properties
                data[nvizType] = self.nvizDefault.SetSurfaceDefaultProp()
                        
            elif type ==  'vector':
                # reset to default properties (lines/points)
                data['vector'] = self.nvizDefault.SetVectorDefaultProp()
            
            elif type ==  '3d-raster':
                # reset to default properties 
                data[nvizType] = self.nvizDefault.SetVolumeDefaultProp()
        
        else:
            # complete data (use default values)
            if type ==  'raster':
                data['surface'] = self.nvizDefault.SetSurfaceDefaultProp()
            if type ==  'vector':
                if not data['vector']['lines']:
                    self.nvizDefault.SetVectorLinesDefaultProp(data['vector']['lines'])
                if not data['vector']['points']:
                    self.nvizDefault.SetVectorPointsDefaultProp(data['vector']['points'])
                    
            # set updates
            for sec in data.keys():
                for sec1 in data[sec].keys():
                    for sec2 in data[sec][sec1].keys():
                        if sec2 !=  'all':
                            data[sec][sec1][sec2]['update'] = None
            
            event = wxUpdateProperties(data = data)
            wx.PostEvent(self, event)
        
        # set id
        if id > 0:
            if type in ('raster', '3d-raster'):
               data[nvizType]['object'] = { 'id' : id,
                                            'init' : False }
            elif type ==  'vector':
                data['vector'][nvizType]['object'] = { 'id' : id,
                                                       'init' : False }
        
        return data

    def LoadRaster(self, item):
        """!Load 2d raster map and set surface attributes
        
        @param layer item
        """
        return self._loadRaster(item)
    
    def LoadRaster3d(self, item):
        """!Load 3d raster map and set surface attributes
        
        @param layer item
        """
        return self._loadRaster(item)
    
    def _loadRaster(self, item):
        """!Load 2d/3d raster map and set its attributes
        
        @param layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']
        
        if layer.type not in ('raster', '3d-raster'):
            return
        
        if layer.type ==  'raster':
            id = self._display.LoadSurface(str(layer.name), None, None)
            nvizType = 'surface'
            errorMsg = _("Loading raster map")
        elif layer.type ==  '3d-raster':
            id = self._display.LoadVolume(str(layer.name), None, None)
            nvizType = 'volume'
            errorMsg = _("Loading 3d raster map")
        else:
            id = -1
        
        if id < 0:
            if layer.type in ('raster', '3d-raster'):
                print >> sys.stderr, "Nviz:" + "%s <%s> %s" % (errorMsg, layer.name, _("failed"))
            else:
                print >> sys.stderr, "Nviz:" + _("Unsupported layer type '%s'") % layer.type
        
        self.layers.append(item)
        
        # set default/workspace layer properties
        data = self.SetMapObjProperties(item, id, nvizType)
        
        # update properties
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self, event)
        
        # update tools window
        if hasattr(self.lmgr, "nviz") and \
                item ==  self.GetSelectedLayer(type = 'item'):
            toolWin = self.lmgr.nviz
            if layer.type ==  'raster':
                win = toolWin.FindWindowById( \
                    toolWin.win['vector']['lines']['surface'])
                win.SetItems(self.GetLayerNames(layer.type))
            
            #toolWin.UpdatePage(nvizType)
            #toolWin.SetPage(nvizType)
        
        return id
    
    def UnloadRaster(self, item):
        """!Unload 2d raster map
        
        @param layer item
        """
        return self._unloadRaster(item)
    
    def UnloadRaster3d(self, item):
        """!Unload 3d raster map
        
        @param layer item
        """
        return self._unloadRaster(item)
    
    def _unloadRaster(self, item):
        """!Unload 2d/3d raster map
        
        @param item layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']
        
        if layer.type not in ('raster', '3d-raster'):
            return
        
        data = self.tree.GetPyData(item)[0]['nviz']
        
        if layer.type ==  'raster':
            nvizType = 'surface'
            unloadFn = self._display.UnloadSurface
            errorMsg = _("Unable to unload raster map")
            successMsg = _("Raster map")
        else:
            nvizType = 'volume'
            unloadFn = self._display.UnloadVolume
            errorMsg = _("Unable to unload 3d raster map")
            successMsg = _("3d raster map")
        
        id = data[nvizType]['object']['id']
        
        if unloadFn(id) ==  0:
            print >> sys.stderr, "Nviz:" + "%s <%s>" % (errorMsg, layer.name)
        else:
            print "Nviz:" + "%s <%s> %s" % (successMsg, layer.name, _("unloaded successfully"))
        
        data[nvizType].pop('object')
        
        self.layers.remove(item)
        
        # update tools window
        if hasattr(self.lmgr, "nviz") and \
                layer.type ==  'raster':
            toolWin = self.lmgr.nviz
            win = toolWin.FindWindowById( \
                toolWin.win['vector']['lines']['surface'])
            win.SetItems(self.GetLayerNames(layer.type))
            
    def LoadVector(self, item, points = None):
        """!Load 2D or 3D vector map overlay
        
        @param item layer item
        @param points True to load points, False to load lines, None
        to load both
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']
        if layer.type !=  'vector':
            return
        
        # if vecType is None:
        #     # load data type by default
        #     vecType = []
        #     for v in ('lines', 'points'):
        #         if UserSettings.Get(group = 'nviz', key = 'vector',
        #                             subkey = [v, 'show']):
        #             vecType.append(v)
        
        # set default properties
        if points is None:
            self.SetMapObjProperties(item, -1, 'lines')
            self.SetMapObjProperties(item, -1, 'points')
            vecTypes = ('points', 'lines')
        elif points:
            self.SetMapObjProperties(item, -1, 'points')
            vecTypes = ('points', )
        else:
            self.SetMapObjProperties(item, -1, 'lines')
            vecTypes = ('lines', )
        
        id = -1
        for vecType in vecTypes:
            if vecType == 'lines':
                id = self._display.LoadVector(str(layer.GetName()), False)
            else:
                id = self._display.LoadVector(str(layer.GetName()), True)
            if id < 0:
                print >> sys.stderr, "Nviz:" + _("Loading vector map <%(name)s> (%(type)s) failed") % \
                    { 'name' : layer.name, 'type' : vecType }
            # update layer properties
            self.SetMapObjProperties(item, id, vecType)
        
        self.layers.append(item)
        
        # update properties
        data = self.tree.GetPyData(item)[0]['nviz']
        event = wxUpdateProperties(data = data)
        wx.PostEvent(self, event)
        
        # update tools window
        if hasattr(self.lmgr, "nviz") and \
                item ==  self.GetSelectedLayer(type = 'item'):
            toolWin = self.lmgr.nviz
            
            toolWin.UpdatePage('vector')
            ### toolWin.SetPage('vector')
        
        return id

    def UnloadVector(self, item, points = None):
        """!Unload vector map overlay
        
        @param item layer item
        @param points,lines True to unload given feature type
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']
        data = self.tree.GetPyData(item)[0]['nviz']['vector']
        
        # if vecType is None:
        #     vecType = []
        #     for v in ('lines', 'points'):
        #         if UserSettings.Get(group = 'nviz', key = 'vector',
        #                             subkey = [v, 'show']):
        #             vecType.append(v)
        
        if points is None:
            vecTypes = ('points', 'lines')
        elif points:
            vecTypes = ('points', )
        else:
            vecTypes = ('lines', )
        
        for vecType in vecTypes:
            if not data[vecType].has_key('object'):
                continue
            
            id = data[vtype]['object']['id']
            
            if vecType ==  'lines':
                ret = self._display.UnloadVector(id, False)
            else:
                ret = self._display.UnloadVector(id, True)
            if ret ==  0:
                print >> sys.stderr, "Nviz:" + _("Unable to unload vector map <%(name)s> (%(type)s)") % \
                    { 'name': layer.name, 'type' : vtype }
            else:
                print "Nviz:" + _("Vector map <%(name)s> (%(type)s) unloaded successfully") % \
                    { 'name' : layer.name, 'type' : vtype }
            
            data[vecType].pop('object')
            
            ### self.layers.remove(id)
        
    def Reset(self):
        """!Reset (unload data)"""
        for item in self.layers:
            type = self.tree.GetPyData(item)[0]['maplayer'].type
            if type ==  'raster':
                self.UnloadRaster(item)
            elif type ==  '3d-raster':
                self.UnloadRaster3d(item)
            elif type ==  'vector':
                self.UnloadVector(item)
        
        self.init = False

    def OnZoomToMap(self, event):
        """!Set display extents to match selected raster or vector
        map or volume.
        
        @todo vector, volume
        """
        layer = self.GetSelectedLayer()
        
        if layer is None:
            return
        
        Debug.msg (3, "GLWindow.OnZoomToMap(): layer = %s, type = %s" % \
                       (layer.name, layer.type))
        
        self._display.SetViewportDefault()

    def ResetView(self):
        """!Reset to default view"""
        self.view['z-exag']['value'], \
            self.iview['height']['value'], \
            self.iview['height']['min'], \
            self.iview['height']['max'] = self._display.SetViewDefault()
        
        self.view['z-exag']['min'] = 0
        self.view['z-exag']['max'] = self.view['z-exag']['value'] * 10
        
        self.view['position']['x'] = UserSettings.Get(group = 'nviz', key = 'view',
                                                 subkey = ('position', 'x'))
        self.view['position']['y'] = UserSettings.Get(group = 'nviz', key = 'view',
                                                 subkey = ('position', 'y'))
        self.view['persp']['value'] = UserSettings.Get(group = 'nviz', key = 'view',
                                                       subkey = ('persp', 'value'))
        
        self.view['twist']['value'] = UserSettings.Get(group = 'nviz', key = 'view',
                                                       subkey = ('twist', 'value'))
        
        event = wxUpdateView(zExag = False)
        wx.PostEvent(self, event)
        
    def UpdateMapObjProperties(self, event):
        """!Generic method to update data layer properties"""
        data = event.data
        
        if data.has_key('surface'):
            id = data['surface']['object']['id']
            self.UpdateSurfaceProperties(id, data['surface'])
            # -> initialized
            data['surface']['object']['init'] = True
            
        elif data.has_key('volume'):
            id = data['volume']['object']['id']
            self.UpdateVolumeProperties(id, data['volume'])
            # -> initialized
            data['volume']['object']['init'] = True
            
        elif data.has_key('vector'):
            for type in ('lines', 'points'):
                if data['vector'][type].has_key('object'):
                    id = data['vector'][type]['object']['id']
                    self.UpdateVectorProperties(id, data['vector'], type)
                    # -> initialized
                    data['vector'][type]['object']['init'] = True
        
    def UpdateSurfaceProperties(self, id, data):
        """!Update surface map object properties"""
        # surface attributes
        for attrb in ('topo', 'color', 'mask',
                     'transp', 'shine', 'emit'):
            if not data['attribute'].has_key(attrb) or \
                    not data['attribute'][attrb].has_key('update'):
                continue
            
            map = data['attribute'][attrb]['map']
            value = data['attribute'][attrb]['value']
            
            if map is None: # unset
                # only optional attributes
                if attrb ==  'mask':
                    # TODO: invert mask
                    # TODO: broken in NVIZ
                    self._display.UnsetSurfaceMask(id)
                elif attrb ==  'transp':
                    self._display.UnsetSurfaceTransp(id)
                elif attrb ==  'emit':
                    self._display.UnsetSurfaceEmit(id) 
            else:
                if type(value) ==  type('') and \
                        len(value) <=  0: # ignore empty values (TODO: warning)
                    continue
                if attrb ==  'topo':
                    self._display.SetSurfaceTopo(id, map, str(value)) 
                elif attrb ==  'color':
                    self._display.SetSurfaceColor(id, map, str(value))
                elif attrb ==  'mask':
                    # TODO: invert mask
                    # TODO: broken in NVIZ
                    self._display.SetSurfaceMask(id, False, str(value))
                elif attrb ==  'transp':
                    self._display.SetSurfaceTransp(id, map, str(value)) 
                elif attrb ==  'shine':
                    self._display.SetSurfaceShine(id, map, str(value)) 
                elif attrb ==  'emit':
                    self._display.SetSurfaceEmit(id, map, str(value)) 
            data['attribute'][attrb].pop('update')
        
        # draw res
        if data['draw']['resolution'].has_key('update'):
            coarse = data['draw']['resolution']['coarse']
            fine   = data['draw']['resolution']['fine']
            
            if data['draw']['all']:
                self._display.SetSurfaceRes(-1, fine, coarse)
            else:
                self._display.SetSurfaceRes(id, fine, coarse)
            data['draw']['resolution'].pop('update')
        
        # draw style
        if data['draw']['mode'].has_key('update'):
            if data['draw']['mode']['value'] < 0: # need to calculate
                data['draw']['mode']['value'] = \
                    self.nvizDefault.GetDrawMode(mode = data['draw']['mode']['desc']['mode'],
                                                 style = data['draw']['mode']['desc']['style'],
                                                 shade = data['draw']['mode']['desc']['shading'],
                                                 string = True)
            style = data['draw']['mode']['value']
            if data['draw']['all']:
                self._display.SetSurfaceStyle(-1, style)
            else:
                self._display.SetSurfaceStyle(id, style)
            data['draw']['mode'].pop('update')
        
        # wire color
        if data['draw']['wire-color'].has_key('update'):
            color = data['draw']['wire-color']['value']
            if data['draw']['all']:
                self._display.SetWireColor(-1, str(color))
            else:
                self._display.SetWireColor(id, str(color))
            data['draw']['wire-color'].pop('update')
        
        # position
        if data['position'].has_key('update'):
            x = data['position']['x']
            y = data['position']['y']
            z = data['position']['z']
            self._display.SetSurfacePosition(id, x, y, z)
            data['position'].pop('update')
        
    def UpdateVolumeProperties(self, id, data, isosurfId = None):
        """!Update volume (isosurface/slice) map object properties"""
        if data['draw']['resolution'].has_key('update'):
            self._display.SetIsosurfaceRes(id, data['draw']['resolution']['value'])
            data['draw']['resolution'].pop('update')
        
        if data['draw']['shading'].has_key('update'):
            if data['draw']['shading']['value'] < 0: # need to calculate
                data['draw']['shading']['value'] = \
                    self.nvizDefault.GetDrawMode(shade = data['draw']['shading'],
                                                 string = False)
            data['draw']['shading'].pop('update')
        
        #
        # isosurface attributes
        #
        isosurfId = 0
        for isosurf in data['isosurface']:
            for attrb in ('color', 'mask',
                          'transp', 'shine', 'emit'):
                if not isosurf.has_key(attrb) or \
                    not isosurf[attrb].has_key('update'):
                    continue
                map = isosurf[attrb]['map']
                value = isosurf[attrb]['value']
                
                if map is None: # unset
                    # only optional attributes
                    if attrb ==  'mask':
                        # TODO: invert mask
                        # TODO: broken in NVIZ
                        self._display.UnsetIsosurfaceMask(id, isosurfId)
                    elif attrb ==  'transp':
                        self._display.UnsetIsosurfaceTransp(id, isosurfId)
                    elif attrb ==  'emit':
                        self._display.UnsetIsosurfaceEmit(id, isosurfId) 
                else:
                    if type(value) ==  type('') and \
                            len(value) <=  0: # ignore empty values (TODO: warning)
                        continue
                    elif attrb ==  'color':
                        self._display.SetIsosurfaceColor(id, isosurfId, map, str(value))
                    elif attrb ==  'mask':
                        # TODO: invert mask
                        # TODO: broken in NVIZ
                        self._display.SetIsosurfaceMask(id, isosurfId, False, str(value))
                    elif attrb ==  'transp':
                        self._display.SetIsosurfaceTransp(id, isosurfId, map, str(value)) 
                    elif attrb ==  'shine':
                        self._display.SetIsosurfaceShine(id, isosurfId, map, str(value)) 
                    elif attrb ==  'emit':
                        self._display.SetIsosurfaceEmit(id, isosurfId, map, str(value)) 
                isosurf[attrb].pop('update')
            isosurfId +=  1
        
    def UpdateVectorProperties(self, id, data, type):
        """!Update vector layer properties
        
        @param id layer id
        @param data properties
        @param type lines/points
        """
        if type ==  'points':
            self.UpdateVectorPointsProperties(id, data[type])
        else:
            self.UpdateVectorLinesProperties(id, data[type])
        
    def UpdateVectorLinesProperties(self, id, data):
        """!Update vector line map object properties"""
        # mode
        if data['color'].has_key('update') or \
                data['width'].has_key('update') or \
                data['mode'].has_key('update'):
            width = data['width']['value']
            color = data['color']['value']
            if data['mode']['type'] ==  'flat':
                flat = True
                if data.has_key('surface'):
                    data.pop('surface')
            else:
                flat = False
            
            self._display.SetVectorLineMode(id, color,
                                            width, flat)
            
            if data['color'].has_key('update'):
                data['color'].pop('update')
            if data['width'].has_key('update'):
                data['width'].pop('update')
            if data['mode'].has_key('update'):
                data['mode'].pop('update')
        
        # height
        if data['height'].has_key('update'):
            self._display.SetVectorLineHeight(id,
                                              data['height']['value'])
            data['height'].pop('update')
        
        # surface
        if data['mode'].has_key('update'):
            sid = self.GetLayerId(type = 'raster', name = data['mode']['surface'])
            if sid > -1:
                self._display.SetVectorLineSurface(id, sid)
            
            data['mode'].pop('update')
        
    def UpdateVectorPointsProperties(self, id, data):
        """!Update vector point map object properties"""
        if data['size'].has_key('update') or \
                data['width'].has_key('update') or \
                data['marker'].has_key('update') or \
                data['color'].has_key('update'):
            ret = self._display.SetVectorPointMode(id, data['color']['value'],
                                                   data['width']['value'], float(data['size']['value']),
                                                   data['marker']['value'] + 1)
            
            error = None
            if ret ==  -1:
                error = _("Vector point layer not found (id = %d)") % id
            elif ret ==  -2:
                error = _("Unable to set data layer properties (id = %d)") % id

            if error:
                raise gcmd.GException(_("Setting data layer properties failed.\n\n%s") % error)
            
            for prop in ('size', 'width', 'marker', 'color'):
                if data[prop].has_key('update'):
                    data[prop].pop('update')
        
        # height
        if data['height'].has_key('update'):
            self._display.SetVectorPointHeight(id,
                                               data['height']['value'])
            data['height'].pop('update')
        
        # surface
        if data['mode'].has_key('update'):
            sid = self.GetLayerId(type = 'raster', name = data['mode']['surface'])
            if sid > -1:
                self._display.SetVectorPointSurface(id, sid)
            
            data['mode'].pop('update')
            
    def GetLayerNames(self, type):
        """!Return list of map layer names of given type"""
        layerName = []
        
        for item in self.layers:
            mapLayer = self.tree.GetPyData(item)[0]['maplayer']
            if type !=  mapLayer.GetType():
                continue
            
            layerName.append(mapLayer.GetName())
        
        return layerName
    
    def GetLayerId(self, type, name):
        """!Get layer object id or -1"""
        if len(name) < 1:
            return -1
        
        for item in self.layers:
            mapLayer = self.tree.GetPyData(item)[0]['maplayer']
            if type !=  mapLayer.GetType() or \
                    name !=  mapLayer.GetName():
                continue
            
            data = self.tree.GetPyData(item)[0]['nviz']
            
            if type ==  'raster':
                return data['surface']['object']['id']
            elif type ==  'vpoint':
                return data['vector']['points']['object']['id']
            elif type ==  'vline':
                return data['vector']['lines']['object']['id']
            elif type ==  '3d-raster':
                return data['volume']['object']['id']
        
        return -1
    
    def SaveToFile(self, FileName, FileType, width, height):
        """!This draws the DC to a buffer that can be saved to a file.
        
        @todo fix BufferedPaintDC
        
        @param FileName file name
        @param FileType type of bitmap
        @param width image width
        @param height image height
        """
        self._display.SaveToFile(FileName, width, height)
                
        # pbuffer = wx.EmptyBitmap(max(1, self.Map.width), max(1, self.Map.height))
        # dc = wx.BufferedPaintDC(self, pbuffer)
        # dc.Clear()
        # self.SetCurrent()
        # self._display.Draw(False, -1)
        # pbuffer.SaveFile(FileName, FileType)
        # self.SwapBuffers()
        
    def GetDisplay(self):
        """!Get display instance"""
        return self._display
        
    def ZoomToMap(self):
        """!Reset view
        """
        self.lmgr.nviz.OnResetView(None)

