"""
@package nviz_mapdisp.py

@brief Nviz extension for wxGUI

This module adds to Map Display 2.5/3D visualization mode.

List of classes:
 - GLWindow

(C) 2008 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008)
"""

import os
import sys
import time
import copy

from threading import Thread

import wx
import wx.lib.scrolledpanel as scrolled
from wx.lib.newevent import NewEvent
from wx import glcanvas

import gcmd
import globalvar
from debug import Debug as Debug
from preferences import globalSettings as UserSettings
from mapdisp import MapWindow as MapWindow
from goutput import wxCmdOutput as wxCmdOutput

sys.path.append(os.path.join(globalvar.ETCWXDIR, "nviz"))
import grass6_wxnviz as wxnviz

wxUpdateProperties, EVT_UPDATE_PROP = NewEvent()
wxUpdateView,       EVT_UPDATE_VIEW = NewEvent()

class NvizThread(Thread):
    def __init__(self, log, progressbar, window):
        Thread.__init__(self)
        
        self.log = log
        self.progressbar = progressbar
        self.window = window
        
        self.nvizClass = None
        
        self.setDaemon(True)
        
    def run(self):
        self.nvizClass = wxnviz.Nviz(self.log)
        
class GLWindow(MapWindow, glcanvas.GLCanvas):
    """OpenGL canvas for Map Display Window"""
    def __init__(self, parent, id,
                 pos=wx.DefaultPosition,
                 size=wx.DefaultSize,
                 style=wx.NO_FULL_REPAINT_ON_RESIZE,
                 Map=None, tree=None, gismgr=None):

        self.parent = parent # MapFrame
        self.Map = Map
        self.tree = tree
        self.gismgr = gismgr
        
        glcanvas.GLCanvas.__init__(self, parent, id)
        MapWindow.__init__(self, parent, id, pos, size, style,
                           Map, tree, gismgr)


        self.parent = parent # MapFrame
        
        self.init = False
        self.initView = False
        
        # render mode 
        self.render = { 'quick' : False,
                        # do not render vector lines in quick mode
                        'vlines' : False,
                        'vpoints' : False }

        # list of loaded map layers (layer tree items)
        self.layers = []
        
        #
        # use display region instead of computational
        #
        os.environ['GRASS_REGION'] = self.Map.SetRegion()

        #
        # create nviz instance
        #
        self.nvizThread = NvizThread(self.gismgr.goutput.cmd_stderr,
                                     self.parent.onRenderGauge,
                                     self.gismgr.goutput.cmd_output)
        self.nvizThread.start()
        time.sleep(.1)
        self.nvizClass =  self.nvizThread.nvizClass

        # GRASS_REGION needed only for initialization
        del os.environ['GRASS_REGION']

        #
        # set current display
        #
        self.nvizClass.SetDisplay(self)
        
        #
        # default values
        #
        self.view = copy.deepcopy(UserSettings.Get(group='nviz', key='view')) # copy
        self.iview = UserSettings.Get(group='nviz', key='view', internal=True)

        self.size = None
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.OnEraseBackground)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.Bind(wx.EVT_LEFT_UP, self.OnLeftUp)
        self.Bind(wx.EVT_MOTION, self.OnMouseAction)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouseAction)

        self.Bind(EVT_UPDATE_PROP, self.UpdateMapObjProperties)
        self.Bind(EVT_UPDATE_VIEW, self.UpdateView)
        
    def OnEraseBackground(self, event):
        pass # do nothing, to avoid flashing on MSW

    def OnSize(self, event):
        self.size = self.parent.GetClientSize()
        if self.GetContext():
            Debug.msg(3, "GLCanvas.OnPaint(): w=%d, h=%d" % \
                      (self.size.width, self.size.height))
            self.SetCurrent()
            self.nvizClass.ResizeWindow(self.size.width,
                                        self.size.height)
        
        event.Skip()

    def OnPaint(self, event):
        Debug.msg(3, "GLCanvas.OnPaint()")

        dc = wx.PaintDC(self)
        self.SetCurrent()
        
        if not self.initView:
            self.nvizClass.InitView()
            self.initView = True

        if not self.init:
            self.LoadDataLayers()

            self.ResetView()
            
            if hasattr(self.parent, "nvizToolWin"):
                self.parent.nvizToolWin.UpdatePage('view')
                layer = self.GetSelectedLayer()
                if layer:
                    if layer.type == 'raster':
                        self.parent.nvizToolWin.UpdatePage('surface')
                    elif layer.type == 'vector':
                        self.parent.nvizToolWin.UpdatePage('vector')

                self.parent.nvizToolWin.UpdateSettings()

                # update widgets
                win = self.parent.nvizToolWin.FindWindowById( \
                    self.parent.nvizToolWin.win['vector']['lines']['surface'])
                win.SetItems(self.GetLayerNames('raster'))

            self.init = True
            
        self.UpdateMap()

    def OnMouseAction(self, event):
        # change position
        if event.Dragging() and event.LeftIsDown():
            self.lastX = self.lastY = self.x = self.y
            self.x, self.y = event.GetPosition()
            self.Refresh(False)

        # change perspective with mouse wheel
        wheel = event.GetWheelRotation()

        if wheel != 0:
            current  = event.GetPositionTuple()[:]
            Debug.msg (5, "GLWindow.OnMouseMotion(): wheel=%d" % wheel)
            prev_value = self.view['persp']['value']
            if wheel > 0:
                value = -1 * self.view['persp']['step']
            else:
                value = self.view['persp']['step']
            self.view['persp']['value'] += value
            if self.view['persp']['value'] < 1:
                self.view['persp']['value'] = 1
            elif self.view['persp']['value'] > 100:
                self.view['persp']['value'] = 100

            if prev_value != self.view['persp']['value']:
                if hasattr(self.parent, "nvizToolWin"):
                    self.parent.nvizToolWin.UpdateSettings()

                    self.nvizClass.SetView(self.view['pos']['x'], self.view['pos']['y'],
                                           self.iview['height']['value'],
                                           self.view['persp']['value'],
                                           self.view['twist']['value'])

                # redraw map
                self.OnPaint(None)

                # update statusbar
                ### self.parent.StatusbarUpdate()

    def OnLeftDown(self, event):
        self.CaptureMouse()
        self.x, self.y = self.lastX, self.lastY = event.GetPosition()
        
    def OnLeftUp(self, event):
        self.ReleaseMouse()

    def UpdateView(self, event):
        """Change view settings"""
        self.nvizClass.SetView(self.view['pos']['x'], self.view['pos']['y'],
                               self.iview['height']['value'],
                               self.view['persp']['value'],
                               self.view['twist']['value'])

        if event.zExag:
            self.nvizClass.SetZExag(self.view['z-exag']['value'])
        
        event.Skip()
        
    def UpdateMap(self, render=True):
        """
        Updates the canvas anytime there is a change to the
        underlaying images or to the geometry of the canvas.

        @param render re-render map composition
        """
        start = time.clock()

        self.resize = False
        
        if self.render['quick'] is False:
            self.parent.onRenderGauge.Show()
            self.parent.onRenderGauge.SetRange(2)
            self.parent.onRenderGauge.SetValue(0)
            
        if self.render['quick'] is False:
            self.parent.onRenderGauge.SetValue(1)
            wx.Yield()
            self.nvizClass.Draw(False, -1)
        elif self.render['quick'] is True:
            # quick
            mode = wxnviz.DRAW_QUICK_SURFACE | wxnviz.DRAW_QUICK_VOLUME
            if self.render['vlines']:
                mode |= wxnviz.DRAW_QUICK_VLINES
            if self.render['vpoints']:
                mode |= wxnviz.DRAW_QUICK_VPOINTS
            self.nvizClass.Draw(True, mode)
        else: # None -> reuse last rendered image
            pass # TODO

        self.SwapBuffers()

        stop = time.clock()

        if self.render['quick'] is False:
            self.parent.onRenderGauge.SetValue(2)
            # hide process bar
            self.parent.onRenderGauge.Hide()

        #
        # update statusbar
        #
        # self.parent.StatusbarUpdate()

        Debug.msg(3, "GLWindow.UpdateMap(): quick=%d, -> time=%g" % \
                      (self.render['quick'], (stop-start)))

        # print stop-start

    def EraseMap(self):
        """
        Erase the canvas
        """
        self.nvizClass.EraseMap()
        self.SwapBuffers()

    def IsLoaded(self, item):
        """Check if layer (item) is already loaded

        @param item layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']
        data = self.tree.GetPyData(item)[0]['nviz']

        if not data:
            return 0

        if layer.type == 'raster':
            if not data['surface'].has_key('object'):
                return 0
        elif layer.type == 'vector':
            if not data['vlines'].has_key('object') and \
                    not data['points'].has_key('object'):
                return 0

        return 1

    def LoadDataLayers(self):
        """Load raster/vector from current layer tree

        @todo volumes
        """
        listOfItems = []
        # load raster & vector maps
        item = self.tree.GetFirstChild(self.tree.root)[0]
        while item and item.IsOk():
            type = self.tree.GetPyData(item)[0]['type']
            if not item.IsChecked() or \
                    type not in ('raster', 'vector', '3d-raster'):
                item = self.tree.GetNextSibling(item)
                continue

            listOfItems.append(item)

            item = self.tree.GetNextSibling(item)

        start = time.time()

        while(len(listOfItems) > 0):
            item = listOfItems.pop()
            type = self.tree.GetPyData(item)[0]['type']
            
            try:
                if type == 'raster':
                    self.LoadRaster(item)
                elif type == '3d-raster':
                    self.LoadRaster3d(item)
            except gcmd.NvizError, e:
                print >> sys.stderr, "Nviz:" + e.message

            try:
                if type == 'vector':
                    data = self.tree.GetPyData(item)[0]['nviz']
                    vecType = []
                    if data and data.has_key('vector'):
                        for v in ('lines', 'points'):
                            if data['vector'][v]:
                                vecType.append(v)
                    self.LoadVector(item, vecType)
            except gcmd.NvizError, e:
                print >> sys.stderr, "Nviz:" + e.message
            
        stop = time.time()
        
        Debug.msg(3, "GLWindow.LoadDataLayers(): time=%f" % (stop-start))

        # print stop - start

    def SetMapObjProperties(self, item, id, nvizType):
        """Set map object properties

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

            if type == 'raster':
                data[nvizType] = {}
                for sec in ('attribute', 'draw', 'mask', 'position'):
                    data[nvizType][sec] = {}

                # reset to default properties
                self.SetSurfaceDefaultProp(data[nvizType])
                        
            elif type == 'vector':
                data['vector'] = {}
                for sec in ('lines', 'points'):
                    data['vector'][sec] = {}
                
                # reset to default properties (lines/points)
                self.SetVectorDefaultProp(data['vector'])

            elif type == '3d-raster':
                data[nvizType] = {}
                for sec in ('attribute', 'draw', 'position'):
                    data[nvizType][sec] = {}
                for sec in ('isosurface', 'slice'):
                    data[nvizType][sec] = []

                # reset to default properties 
                self.SetVolumeDefaultProp(data[nvizType])
        
        else:
            # check data
            if type == 'vector':
                if not data['vector']['lines']:
                    self.SetVectorLinesDefaultProp(data['vector']['lines'])
                if not data['vector']['points']:
                    self.SetVectorPointsDefaultProp(data['vector']['points'])
                    
            # set updates
            for sec in data.keys():
                for sec1 in data[sec].keys():
                    for sec2 in data[sec][sec1].keys():
                        if sec2 != 'all':
                            data[sec][sec1][sec2]['update'] = None

            event = wxUpdateProperties(data=data)
            wx.PostEvent(self, event)
                            
        # set id
        if id > 0:
            if type in ('raster', '3d-raster'):
               data[nvizType]['object'] = { 'id' : id,
                                            'init' : False }
            elif type == 'vector':
                data['vector'][nvizType]['object'] = { 'id' : id,
                                                       'init' : False }
        
        return data

    def LoadRaster(self, item):
        """Load 2d raster map and set surface attributes

        @param layer item
        """
        return self._loadRaster(item)

    def LoadRaster3d(self, item):
        """Load 3d raster map and set surface attributes

        @param layer item
        """
        return self._loadRaster(item)

    def _loadRaster(self, item):
        """Load 2d/3d raster map and set its attributes

        @param layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']

        if layer.type not in ('raster', '3d-raster'):
            return

        if layer.type == 'raster':
            id = self.nvizClass.LoadSurface(str(layer.name), None, None)
            nvizType = 'surface'
            errorMsg = _("Loading raster map")
        elif layer.type == '3d-raster':
            id = self.nvizClass.LoadVolume(str(layer.name), None, None)
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
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self, event)
        
        # update tools window
        if hasattr(self.parent, "nvizToolWin") and \
                item == self.GetSelectedLayer(type='item'):
            toolWin = self.parent.nvizToolWin
            if layer.type == 'raster':
                win = toolWin.FindWindowById( \
                    toolWin.win['vector']['lines']['surface'])
                win.SetItems(self.GetLayerNames(layer.type))

            toolWin.UpdatePage(nvizType)
            toolWin.SetPage(nvizType)
            
        return id

    def UnloadRaster(self, item):
        """Unload 2d raster map

        @param layer item
        """
        return self._unloadRaster(item)

    def UnloadRaster3d(self, item):
        """Unload 3d raster map

        @param layer item
        """
        return self._unloadRaster(item)

    def _unloadRaster(self, item):
        """Unload 2d/3d raster map

        @param item layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']

        if layer.type not in ('raster', '3d-raster'):
            return

        data = self.tree.GetPyData(item)[0]['nviz']

        if layer.type == 'raster':
            nvizType = 'surface'
            unloadFn = self.nvizClass.UnloadSurface
            errorMsg = _("Unable to unload raster map")
            successMsg = _("Raster map")
        else:
            nvizType = 'volume'
            unloadFn = self.nvizClass.UnloadVolume
            errorMsg = _("Unable to unload 3d raster map")
            successMsg = _("3d raster map")

        id = data[nvizType]['object']['id']

        if unloadFn(id) == 0:
            print >> sys.stderr, "Nviz:" + "%s <%s>" % (errorMsg, layer.name)
        else:
            print "Nviz:" + "%s <%s> %s" % (successMsg, layer.name, _("unloaded successfully"))

        data[nvizType].pop('object')

        self.layers.remove(item)
        
        # update tools window
        if hasattr(self.parent, "nvizToolWin") and \
                layer.type == 'raster':
            toolWin = self.parent.nvizToolWin
            win = toolWin.FindWindowById( \
                toolWin.win['vector']['lines']['surface'])
            win.SetItems(self.GetLayerNames(layer.type))

            # remove surface page
            if toolWin.notebook.GetSelection() == toolWin.page[nvizType]['id']:
                toolWin.notebook.RemovePage(toolWin.page[nvizType]['id'])
                toolWin.page[nvizType]['id'] = -1
                toolWin.page['settings']['id'] = 1

    def LoadVector(self, item, vecType=None):
        """Load 2D or 3D vector map overlay

        @param item layer item
        @param vecType vector type (lines / points)
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']

        if layer.type != 'vector':
            return

        if vecType is None:
            # load data type by default
            vecType = []
            for v in ('lines', 'points'):
                if UserSettings.Get(group='nviz', key='vector',
                                    subkey=[v, 'show']):
                    vecType.append(v)

        # set default properties
        self.SetMapObjProperties(item, -1, 'lines')
        self.SetMapObjProperties(item, -1, 'points')

        id = -1
        for type in vecType:
            if type == 'lines':
                id = self.nvizClass.LoadVector(str(layer.name), False)
            else:
                id = self.nvizClass.LoadVector(str(layer.name), True)

            if id < 0:
                print >> sys.stderr, "Nviz:" + _("Loading vector map <%(name)s> (%(type)s) failed") % \
                    { 'name' : layer.name, 'type' : type }
                continue

            # update layer properties
            self.SetMapObjProperties(item, id, type)
        
        self.layers.append(item)
        
        # update properties
        data = self.tree.GetPyData(item)[0]['nviz']
        event = wxUpdateProperties(data=data)
        wx.PostEvent(self, event)
        
        # update tools window
        if hasattr(self.parent, "nvizToolWin") and \
                item == self.GetSelectedLayer(type='item'):
            toolWin = self.parent.nvizToolWin

            toolWin.UpdatePage('vector')
            toolWin.SetPage('vector')
        
        return id

    def UnloadVector(self, item, vecType=None):
        """Unload vector map overlay

        @param item layer item
        @param vecType vector type (lines, points)
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']
        data = self.tree.GetPyData(item)[0]['nviz']['vector']

        if vecType is None:
            vecType = []
            for v in ('lines', 'points'):
                if UserSettings.Get(group='nviz', key='vector',
                                    subkey=[v, 'show']):
                    vecType.append(v)

        for vtype in vecType:
            if not data[vtype].has_key('object'):
                continue

            id = data[vtype]['object']['id']

            if vtype == 'lines':
                ret = self.nvizClass.UnloadVector(id, False)
            else:
                ret = self.nvizClass.UnloadVector(id, True)
            if ret == 0:
                print >> sys.stderr, "Nviz:" + _("Unable to unload vector map <%(name)s> (%(type)s)") % \
                    { 'name': layer.name, 'type' : vtype }
            else:
                print "Nviz:" + _("Vector map <%(name)s> (%(type)s) unloaded successfully") % \
                    { 'name' : layer.name, 'type' : vtype }

            
            data[vtype].pop('object')

            self.layers.remove(id)
            
        # update tools window
        if hasattr(self.parent, "nvizToolWin") and \
                vecType is None:
            toolWin = self.parent.nvizToolWin
            # remove surface page
            if toolWin.notebook.GetSelection() == toolWin.page['surface']['id']:
                toolWin.notebook.RemovePage(toolWin.page['surface']['id'])
                toolWin.page['surface']['id'] = -1
                toolWin.page['settings']['id'] = 1
        
    def GetDrawMode(self, mode=None, style=None, shade=None, string=False):
        """Get surface draw mode (value) from description/selection

        @param mode,style,shade modes
        @param string if True input parameters are strings otherwise
        selections
        """
        value = 0
        desc = {}

        if string:
            if mode is not None:
                if mode == 'coarse':
                    value |= wxnviz.DM_WIRE
                elif mode == 'fine':
                    value |= wxnviz.DM_POLY
                else: # both
                    value |= wxnviz.DM_WIRE_POLY

            if style is not None:
                if style == 'wire':
                    value |= wxnviz.DM_GRID_WIRE
                else: # surface
                    value |= wxnviz.DM_GRID_SURF
                    
            if shade is not None:
                if shade == 'flat':
                    value |= wxnviz.DM_FLAT
                else: # surface
                    value |= wxnviz.DM_GOURAUD

            return value

        # -> string is False
        if mode is not None:
            if mode == 0: # coarse
                value |= wxnviz.DM_WIRE
                desc['mode'] = 'coarse'
            elif mode == 1: # fine
                value |= wxnviz.DM_POLY
                desc['mode'] = 'fine'
            else: # both
                value |= wxnviz.DM_WIRE_POLY
                desc['mode'] = 'both'

        if style is not None:
            if style == 0: # wire
                value |= wxnviz.DM_GRID_WIRE
                desc['style'] = 'wire'
            else: # surface
                value |= wxnviz.DM_GRID_SURF
                desc['style'] = 'surface'

        if shade is not None:
            if shade == 0:
                value |= wxnviz.DM_FLAT
                desc['shading'] = 'flat'
            else: # surface
                value |= wxnviz.DM_GOURAUD
                desc['shading'] = 'gouraud'
        
        return (value, desc)
    
    def SetSurfaceDefaultProp(self, data):
        """Set default surface data properties"""
        #
        # attributes
        #
        for attrb in ('shine', ):
            data['attribute'][attrb] = {}
            for key, value in UserSettings.Get(group='nviz', key='volume',
                                               subkey=attrb).iteritems():
                data['attribute'][attrb][key] = value
            data['attribute'][attrb]['update'] = None
        
        #
        # draw
        #
        data['draw']['all'] = False # apply only for current surface
        for control, value in UserSettings.Get(group='nviz', key='surface', subkey='draw').iteritems():
            if control[:3] == 'res':
                if not data['draw'].has_key('resolution'):
                    data['draw']['resolution'] = {}
                if not data['draw']['resolution'].has_key('update'):
                    data['draw']['resolution']['update'] = None
                data['draw']['resolution'][control[4:]] = value
                continue
            
            if control == 'wire-color':
                value = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
            elif control in ('mode', 'style', 'shading'):
                if not data['draw'].has_key('mode'):
                    data['draw']['mode'] = {}
                continue

            data['draw'][control] = { 'value' : value }
            data['draw'][control]['update'] = None
            
        value, desc = self.GetDrawMode(UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'mode']),
                                       UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'style']),
                                       UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'shading']))

        data['draw']['mode'] = { 'value' : value,
                                 'desc' : desc, 
                                 'update': None }

    def SetVolumeDefaultProp(self, data):
        """Set default volume data properties"""
        #
        # draw
        #
        for control, value in UserSettings.Get(group='nviz', key='volume', subkey='draw').iteritems():
            if control == 'mode':
                continue
            if control == 'shading':
                sel = UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'shading'])
                value, desc = self.GetDrawMode(shade=sel, string=False)

                data['draw']['shading'] = { 'value' : value,
                                            'desc' : desc['shading'] }
            elif control == 'mode':
                sel = UserSettings.Get(group='nviz', key='volume', subkey=['draw', 'mode'])
                if sel == 0:
                    desc = 'isosurface'
                else:
                    desc = 'slice'
                data['draw']['mode'] = { 'value' : sel,
                                         'desc' : desc, }
            else:
                data['draw'][control] = { 'value' : value }

            if not data['draw'][control].has_key('update'):
                data['draw'][control]['update'] = None
        
        #
        # isosurface attributes
        #
        for attrb in ('shine', ):
            data['attribute'][attrb] = {}
            for key, value in UserSettings.Get(group='nviz', key='volume',
                                               subkey=attrb).iteritems():
                data['attribute'][attrb][key] = value
        
    def SetVectorDefaultProp(self, data):
        """Set default vector data properties"""
        self.SetVectorLinesDefaultProp(data['lines'])
        self.SetVectorPointsDefaultProp(data['points'])

    def SetVectorLinesDefaultProp(self, data):
        """Set default vector properties -- lines"""
        # width
        data['width'] = {'value' : UserSettings.Get(group='nviz', key='vector',
                                                    subkey=['lines', 'width']) }
        
        # color
        value = UserSettings.Get(group='nviz', key='vector',
                                 subkey=['lines', 'color'])
        color = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
        data['color'] = { 'value' : color }

        # mode
        if UserSettings.Get(group='nviz', key='vector',
                            subkey=['lines', 'flat']):
            type = 'flat'
            map  = None
        else:
            rasters = self.GetLayerNames('raster')
            if len(rasters) > 0:
                type = 'surface'
                map  = rasters[0]
            else:
                type = 'flat'
                map = None

        data['mode'] = {}
        data['mode']['type'] = type
        data['mode']['update'] = None
        if map:
            data['mode']['surface'] = map

        # height
        data['height'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['lines', 'height']) }

        if data.has_key('object'):
            for attrb in ('color', 'width', 'mode', 'height'):
                data[attrb]['update'] = None
        
    def SetVectorPointsDefaultProp(self, data):
        """Set default vector properties -- points"""
        # size
        data['size'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                    subkey=['points', 'size']) }

        # width
        data['width'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                     subkey=['points', 'width']) }

        # marker
        data['marker'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['points', 'marker']) }

        # color
        value = UserSettings.Get(group='nviz', key='vector',
                                 subkey=['points', 'color'])
        color = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
        data['color'] = { 'value' : color }

        # mode
        data['mode'] = { 'type' : 'surface',
                         'surface' : '', }
        rasters = self.GetLayerNames('raster')
        if len(rasters) > 0:
            data['mode']['surface'] = rasters[0]
        
        # height
        data['height'] = { 'value' : UserSettings.Get(group='nviz', key='vector',
                                                      subkey=['points', 'height']) }

        if data.has_key('object'):
            for attrb in ('size', 'width', 'marker',
                          'color', 'surface', 'height'):
                data[attrb]['update'] = None
        
    def Reset(self):
        """Reset (unload data)"""
        for item in self.layers:
            type = self.tree.GetPyData(item)[0]['maplayer'].type
            if type == 'raster':
                self.UnloadRaster(item)
            elif type == '3d-raster':
                self.UnloadRaster3d(item)
            elif type == 'vector':
                self.UnloadVector(item)
            
        self.init = False

    def OnZoomToMap(self, event):
        """
        Set display extents to match selected raster
        or vector map or volume.

        @todo vector, volume
        """
        layer = self.GetSelectedLayer()

        if layer is None:
            return

        Debug.msg (3, "GLWindow.OnZoomToMap(): layer=%s, type=%s" % \
                       (layer.name, layer.type))

        self.nvizClass.SetViewportDefault()

    def ResetView(self):
        """Reset to default view"""
        self.view['z-exag']['value'], \
            self.iview['height']['value'], \
            self.iview['height']['min'], \
            self.iview['height']['max'] = self.nvizClass.SetViewDefault()
        
        self.view['pos']['x'] = UserSettings.Get(group='nviz', key='view',
                                                 subkey=('pos', 'x'))
        self.view['pos']['y'] = UserSettings.Get(group='nviz', key='view',
                                                 subkey=('pos', 'x'))
        self.view['persp']['value'] = UserSettings.Get(group='nviz', key='view',
                                                       subkey=('persp', 'value'))

        self.view['twist']['value'] = UserSettings.Get(group='nviz', key='view',
                                                       subkey=('twist', 'value'))

        event = wxUpdateView(zExag=False)
        wx.PostEvent(self, event)
        
    def UpdateMapObjProperties(self, event):
        """Generic method to update data layer properties"""
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
        """Update surface map object properties"""
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
                if attrb == 'mask':
                    # TODO: invert mask
                    # TODO: broken in NVIZ
                    self.nvizClass.UnsetSurfaceMask(id)
                elif attrb == 'transp':
                    self.nvizClass.UnsetSurfaceTransp(id)
                elif attrb == 'emit':
                    self.nvizClass.UnsetSurfaceEmit(id) 
            else:
                if type(value) == type('') and \
                        len(value) <= 0: # ignore empty values (TODO: warning)
                    continue
                if attrb == 'topo':
                    self.nvizClass.SetSurfaceTopo(id, map, str(value)) 
                elif attrb == 'color':
                    self.nvizClass.SetSurfaceColor(id, map, str(value))
                elif attrb == 'mask':
                    # TODO: invert mask
                    # TODO: broken in NVIZ
                    self.nvizClass.SetSurfaceMask(id, False, str(value))
                elif attrb == 'transp':
                    self.nvizClass.SetSurfaceTransp(id, map, str(value)) 
                elif attrb == 'shine':
                    self.nvizClass.SetSurfaceShine(id, map, str(value)) 
                elif attrb == 'emit':
                    self.nvizClass.SetSurfaceEmit(id, map, str(value)) 
            data['attribute'][attrb].pop('update')

        # draw res
        if data['draw']['resolution'].has_key('update'):
            coarse = data['draw']['resolution']['coarse']
            fine   = data['draw']['resolution']['fine']

            if data['draw']['all']:
                self.nvizClass.SetSurfaceRes(-1, fine, coarse)
            else:
                self.nvizClass.SetSurfaceRes(id, fine, coarse)
            data['draw']['resolution'].pop('update')
        
        # draw style
        if data['draw']['mode'].has_key('update'):
            if data['draw']['mode']['value'] < 0: # need to calculate
                data['draw']['mode']['value'] = \
                    self.GetDrawMode(mode=data['draw']['mode']['desc']['mode'],
                                        style=data['draw']['mode']['desc']['style'],
                                        shade=data['draw']['mode']['desc']['shading'],
                                        string=True)
            style = data['draw']['mode']['value']
            if data['draw']['all']:
                self.nvizClass.SetSurfaceStyle(-1, style)
            else:
                self.nvizClass.SetSurfaceStyle(id, style)
            data['draw']['mode'].pop('update')

        # wire color
        if data['draw']['wire-color'].has_key('update'):
            color = data['draw']['wire-color']['value']
            if data['draw']['all']:
                self.nvizClass.SetWireColor(-1, str(color))
            else:
                self.nvizClass.SetWireColor(id, str(color))
            data['draw']['wire-color'].pop('update')
        
        # position
        if data['position'].has_key('update'):
            x = data['position']['x']
            y = data['position']['y']
            z = data['position']['z']
            self.nvizClass.SetSurfacePosition(id, x, y, z)
            data['position'].pop('update')
        
    def UpdateVolumeProperties(self, id, data, isosurfId=None):
        """Update volume (isosurface/slice) map object properties"""
        #
        # draw
        #
        if data['draw']['resolution'].has_key('update'):
            self.nvizClass.SetIsosurfaceRes(id, data['draw']['resolution']['value'])
            data['draw']['resolution'].pop('update')
        
        if data['draw']['shading'].has_key('update'):
            if data['draw']['shading']['value'] < 0: # need to calculate
                data['draw']['shading']['value'] = \
                    self.GetDrawMode(shade=data['draw']['shading'],
                                     string=False)
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
                    if attrb == 'mask':
                        # TODO: invert mask
                        # TODO: broken in NVIZ
                        self.nvizClass.UnsetIsosurfaceMask(id, isosurfId)
                    elif attrb == 'transp':
                        self.nvizClass.UnsetIsosurfaceTransp(id, isosurfId)
                    elif attrb == 'emit':
                        self.nvizClass.UnsetIsosurfaceEmit(id, isosurfId) 
                else:
                    if type(value) == type('') and \
                            len(value) <= 0: # ignore empty values (TODO: warning)
                        continue
                    elif attrb == 'color':
                        self.nvizClass.SetIsosurfaceColor(id, isosurfId, map, str(value))
                    elif attrb == 'mask':
                        # TODO: invert mask
                        # TODO: broken in NVIZ
                        self.nvizClass.SetIsosurfaceMask(id, isosurfId, False, str(value))
                    elif attrb == 'transp':
                        self.nvizClass.SetIsosurfaceTransp(id, isosurfId, map, str(value)) 
                    elif attrb == 'shine':
                        self.nvizClass.SetIsosurfaceShine(id, isosurfId, map, str(value)) 
                    elif attrb == 'emit':
                        self.nvizClass.SetIsosurfaceEmit(id, isosurfId, map, str(value)) 
                isosurf[attrb].pop('update')
            isosurfId += 1
        
    def UpdateVectorProperties(self, id, data, type):
        """Update vector layer properties

        @param id layer id
        @param data properties
        @param type lines/points
        """
        if type == 'points':
            self.UpdateVectorPointsProperties(id, data[type])
        else:
            self.UpdateVectorLinesProperties(id, data[type])
    
    def UpdateVectorLinesProperties(self, id, data):
        """Update vector line map object properties"""
        # mode
        if data['color'].has_key('update') or \
                data['width'].has_key('update') or \
                data['mode'].has_key('update'):
            width = data['width']['value']
            color = data['color']['value']
            if data['mode']['type'] == 'flat':
                flat = True
                if data.has_key('surface'):
                    data.pop('surface')
            else:
                flat = False
                
            self.nvizClass.SetVectorLineMode(id, color,
                                             width, flat)
            
            if data['color'].has_key('update'):
                data['color'].pop('update')
            if data['width'].has_key('update'):
                data['width'].pop('update')
            if data['mode'].has_key('update'):
                data['mode'].pop('update')
        
        # height
        if data['height'].has_key('update'):
            self.nvizClass.SetVectorLineHeight(id,
                                               data['height']['value'])
            data['height'].pop('update')
        
        # surface
        if data['mode'].has_key('update'):
            sid = self.GetLayerId(type='raster', name=data['mode']['surface'])
            if sid > -1:
                self.nvizClass.SetVectorLineSurface(id, sid)
            
            data['mode'].pop('update')
        
    def UpdateVectorPointsProperties(self, id, data):
        """Update vector point map object properties"""
        if data['size'].has_key('update') or \
                data['width'].has_key('update') or \
                data['marker'].has_key('update') or \
                data['color'].has_key('update'):
            ret = self.nvizClass.SetVectorPointMode(id, data['color']['value'],
                                                    data['width']['value'], float(data['size']['value']),
                                                    data['marker']['value'] + 1)

            error = None
            if ret == -1:
                error = _("Vector point layer not found (id=%d)") % id
            elif ret == -2:
                error = _("Unable to set data layer properties (id=%d)") % id

            if error:
                raise gcmd.NvizError(parent=self.parent,
                                     message=_("Setting data layer properties failed.\n\n%s") % error)

            for prop in ('size', 'width', 'marker', 'color'):
                if data[prop].has_key('update'):
                    data[prop].pop('update')
        
        # height
        if data['height'].has_key('update'):
            self.nvizClass.SetVectorPointHeight(id,
                                                data['height']['value'])
            data['height'].pop('update')
        
        # surface
        if data['mode'].has_key('update'):
            sid = self.GetLayerId(type='raster', name=data['mode']['surface'])
            if sid > -1:
                self.nvizClass.SetVectorPointSurface(id, sid)
            
            data['mode'].pop('update')

    def GetLayerNames(self, type):
        """Return list of map layer names of given type"""
        layerName = []
        
        for item in self.layers:
            layerName.append(self.tree.GetPyData(item)[0]['maplayer'].GetName())
        
        return layerName
    
    def GetLayerId(self, type, name):
        """Get layer object id or -1"""
        for item in self.layers:
            mapName = self.tree.GetPyData(item)[0]['maplayer'].GetName()
            data = self.tree.GetPyData(item)[0]['nviz']
            if type == 'raster':
                return data['surface']['object']['id']
            elif type == 'vpoint':
                return data['vector']['points']['object']['id']
            elif type == 'vline':
                return data['vector']['lines']['object']['id']
            elif type == '3d-raster':
                return data['volume']['object']['id']

        return -1
            
