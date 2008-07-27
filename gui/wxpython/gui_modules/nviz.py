"""
@package nviz.py

@brief 2.5/3D visialization mode for Map Display Window

List of classes:
 - GLWindow
 - NvizToolWindow
 - ViewPositionWindow

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
import wx.lib.colourselect as csel
import wx.lib.scrolledpanel as scrolled
errorMsg = ''
try:
    from wx import glcanvas
    haveGLCanvas = True
except ImportError, e:
    haveGLCanvas = False
    errorMsg = e
try:
    from OpenGL.GL import *
    from OpenGL.GLUT import *
    haveOpenGL = True
except ImportError, e:
    haveOpenGL = False
    errorMsg = e

import globalvar
import gcmd
import gselect
from debug import Debug as Debug
from mapdisp import MapWindow as MapWindow
from preferences import globalSettings as UserSettings
try:
    nvizPath = os.path.join(globalvar.ETCWXDIR, "nviz")
    sys.path.append(nvizPath)
    import grass7_wxnviz as wxnviz
    haveNviz = True
except ImportError, e:
    haveNviz = False
    errorMsg = e

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
        # render mode 
        self.render = { 'quick' : False,
                        # do not render vector lines in quick mode
                        'vlines' : False,
                        'vpoints' : False }

        # list of loaded map layers
        self.layers = {}
        for type in ('raster', 'vlines', 'vpoints'):
            self.layers[type] = {}
            self.layers[type]['name'] = []
            self.layers[type]['id'] = []

        #
        # create nviz instance
        #
        self.nvizClass = wxnviz.Nviz(sys.stderr)

        #
        # set current display
        #
        self.nvizClass.SetDisplay(self)

        #
        # initialize mouse position
        #
        self.lastX = self.x = 30
        self.lastY = self.y = 30

        #
        # default values
        #
        self.view = copy.deepcopy(UserSettings.Get(group='nviz', key='view')) # copy
        self.iview = UserSettings.Get(group='nviz', key='view', internal=True)
        self.update = [] # list of properties to be updated

        self.size = None
        self.Bind(wx.EVT_ERASE_BACKGROUND, self.OnEraseBackground)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.Bind(wx.EVT_LEFT_UP, self.OnLeftUp)
        self.Bind(wx.EVT_MOTION, self.OnMouseAction)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouseAction)

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
        
        if not self.init:
            self.nvizClass.InitView()

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
                win.SetItems(self.layers['raster']['name'])

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

    def UpdateMap(self, render=True):
        """
        Updates the canvas anytime there is a change to the
        underlaying images or to the geometry of the canvas.

        @param render re-render map composition
        """
        start = time.clock()

        self.resize = False

        # if self.size is None:
        #    self.size = self.GetClientSize()
        
        #         w, h = self.size
        #         w = float(max(w, 1.0))
        #         h = float(max(h, 1.0))
        #         d = float(min(w, h))
        #         xScale = d / w
        #         yScale = d / h
        # print w, h, d, xScale, yScale
        # print self.y, self.lastY, self.x, self.lastX
        # print (self.y - self.lastY) * yScale, (self.x - self.lastX) * xScale 
        # print self.x * xScale
        
        #glRotatef((self.y - self.lastY) * yScale, 1.0, 0.0, 0.0);
        #glRotatef((self.x - self.lastX) * xScale, 0.0, 1.0, 0.0);

        if self.render['quick'] is False:
            self.parent.onRenderGauge.Show()
            self.parent.onRenderGauge.SetRange(2)
            self.parent.onRenderGauge.SetValue(0)

        if 'view' in self.update:
            if 'z-exag' in self.update:
                self.nvizClass.SetZExag(self.view['z-exag']['value'])
                self.update.remove('z-exag')
            
            self.nvizClass.SetView(self.view['pos']['x'], self.view['pos']['y'],
                                   self.iview['height']['value'],
                                   self.view['persp']['value'],
                                   self.view['twist']['value'])

            self.update.remove('view')

        if self.render['quick'] is False:
            self.parent.onRenderGauge.SetValue(1)
            wx.Yield()
            self.nvizClass.Draw(False, False, False)
        elif self.render['quick'] is True:
            # quick
            self.nvizClass.Draw(True,
                                self.render['vlines'],
                                self.render['vpoints'])
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
                    type not in ('raster', 'vector'):
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
            except gcmd.NvizError, e:
                print >> sys.stderr, "Nviz: " + e.message

            try:
                if type == 'vector':
                    self.LoadVector(item)
            except gcmd.NvizError, e:
                print >> sys.stderr, "Nviz: " + e.message

        stop = time.time()
        
        Debug.msg(3, "GLWindow.LoadDataLayers(): time=%f" % (stop-start))

        # print stop - start

    def SetLayerData(self, item, id, nvizType):
        """Set map object properties"""
        type = self.tree.GetPyData(item)[0]['maplayer'].type
        data = self.tree.GetPyData(item)[0]['nviz']
            
        # set default properties
        if data is None:
            self.tree.GetPyData(item)[0]['nviz'] = {}
            data = self.tree.GetPyData(item)[0]['nviz']

            if type == 'raster':
                data['surface'] = {}
                for sec in ('attribute', 'draw', 'mask', 'position'):
                    data['surface'][sec] = {}

                if id > 0 and not data[nvizType].has_key('object'):
                    data[nvizType]['object'] = { 'id' : id,
                                                 'init' : False }

                self.SetSurfaceDefaultProp(data['surface'])
            elif type == 'vector':
                data['vector'] = {}
                for sec in ('lines', 'points'):
                    data['vector'][sec] = {}

                if id > 0 and not data['vector'][nvizType].has_key('object'):
                    data['vector'][nvizType]['object'] = { 'id' : id,
                                                           'init' : False }

                self.SetVectorDefaultProp(data['vector'])

        # set updates
        else:
            if id > 0:
                if type == 'raster':
                    if not data[nvizType].has_key('object'):
                        data[nvizType]['object'] = { 'id' : id,
                                                     'init' : False }
                elif type == 'vector':
                    if not data['vector'][nvizType].has_key('object'):
                        data['vector'][nvizType]['object'] = { 'id' : id,
                                                               'init' : False }
            
            for sec in data.keys():
                for sec1 in data[sec].keys():
                    for sec2 in data[sec][sec1].keys():
                        if sec2 != 'object':
                            self.update.append('%s:%s:%s' % (sec, sec1, sec2))
        
        return data

    def LoadRaster(self, item):
        """Load raster map and set surface attributes

        @param layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']

        if layer.type != 'raster':
            return

        id = self.nvizClass.LoadSurface(str(layer.name), None, None)
        if id < 0:
            raise gcmd.NvizError(parent=self.parent,
                                 message=_("Raster map <%s> not loaded" % layer.name))
        
        self.layers['raster']['name'].append(layer.name)
        self.layers['raster']['id'].append(id)

        # set default/workspace layer properties
        data = self.SetLayerData(item, id, 'surface')

        # update properties
        self.UpdateLayerProperties(item)

        # update tools window
        if hasattr(self.parent, "nvizToolWin") and \
                item == self.GetSelectedLayer(type='item'):
            toolWin = self.parent.nvizToolWin
            win = toolWin.FindWindowById( \
                toolWin.win['vector']['lines']['surface'])
            win.SetItems(self.layers['raster']['name'])

            toolWin.UpdatePage('surface')
            toolWin.SetPage('surface')
            
        return id

    def UnloadRaster(self, item):
        """Unload raster map

        @param item layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']
        data = self.tree.GetPyData(item)[0]['nviz']

        id = data['surface']['object']['id']

        if self.nvizClass.UnloadSurface(id) == 0:
            raise gcmd.NvizError(parent=self.parent,
                                 message=_("Unable to unload raster map <%s>" % layer.name))

        data['surface'].pop('object')

        idx = self.layers['raster']['id'].index(id)
        del self.layers['raster']['name'][idx]
        del self.layers['raster']['id'][idx]

        # update tools window
        if hasattr(self.parent, "nvizToolWin"):
            toolWin = self.parent.nvizToolWin
            win = toolWin.FindWindowById( \
                toolWin.win['vector']['lines']['surface'])
            win.SetItems(self.layers['raster']['name'])

            # remove surface page
            if toolWin.notebook.GetSelection() == toolWin.page['surface']['id']:
                toolWin.notebook.RemovePage(toolWin.page['surface']['id'])
                toolWin.page['surface']['id'] = -1
                toolWin.page['settings']['id'] = 1
        
    def GetSurfaceMode(self, mode, style, shade, string=False):
        """Determine surface draw mode"""
        value = 0
        desc = {}

        if string:
            if mode == 'coarse':
                value |= wxnviz.DM_WIRE
            elif mode == 'fine':
                value |= wxnviz.DM_POLY
            else: # both
                value |= wxnviz.DM_WIRE_POLY

            if style == 'wire':
                value |= wxnviz.DM_GRID_WIRE
            else: # surface
                value |= wxnviz.DM_GRID_SURF

            if shade == 'flat':
                value |= wxnviz.DM_FLAT
            else: # surface
                value |= wxnviz.DM_GOURAUD

            return value

        # -> string is False
        if mode == 0: # coarse
            value |= wxnviz.DM_WIRE
            desc['mode'] = 'coarse'
        elif mode == 1: # fine
            value |= wxnviz.DM_POLY
            desc['mode'] = 'fine'
        else: # both
            value |= wxnviz.DM_WIRE_POLY
            desc['mode'] = 'both'

        if style == 0: # wire
            value |= wxnviz.DM_GRID_WIRE
            desc['style'] = 'wire'
        else: # surface
            value |= wxnviz.DM_GRID_SURF
            desc['style'] = 'surface'

        if shade == 0:
            value |= wxnviz.DM_FLAT
            desc['shading'] = 'flat'
        else: # surface
            value |= wxnviz.DM_GOURAUD
            desc['shading'] = 'gouraud'

        return (value, desc)
    
    def SetSurfaceDefaultProp(self, data):
        """Set default surface properties"""
        #
        # attributes
        #
        data['attribute']['shine'] = {}
        data['attribute']['shine']['map'] = \
            UserSettings.Get(group='nviz', key='surface', subkey=['shine', 'map'])

        data['attribute']['shine']['value'] = \
            UserSettings.Get(group='nviz', key='surface', subkey=['shine', 'value'])

        self.update.append('surface:attribute:shine')

        #
        # draw
        #
        for control, value in UserSettings.Get(group='nviz', key='surface', subkey='draw').iteritems():
            if control[:3] == 'res':
                if 'surface:draw:%s' % 'resolution' not in self.update:
                    self.update.append('surface:draw:%s' % 'resolution')
                    data['draw']['resolution'] = { 'all' : False,
                                                   control[4:] : value }
                else:
                    data['draw']['resolution'][control[4:]] = value
                continue
            elif control not in ('style', 'shading'):
                self.update.append('surface:draw:%s' % control)

            if control == 'wire-color':
                value = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
            elif control in ('mode', 'style', 'shading'):
                if not data['draw'].has_key('mode'):
                    data['draw']['mode'] = {}
                continue

            data['draw'][control] = { 'value' : value,
                                      'all' : False }
        
        value, desc = self.GetSurfaceMode(UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'mode']),
                                          UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'style']),
                                          UserSettings.Get(group='nviz', key='surface', subkey=['draw', 'shading']))

        data['draw']['mode'] = { 'value' : value,
                                 'desc' : desc,
                                 'all' : False }

    def LoadVector(self, item, vecType=None):
        """Load vector map overlay (lines / points)

        @param item layer item
        """
        layer = self.tree.GetPyData(item)[0]['maplayer']

        if layer.type != 'vector':
            return

        if vecType is None:
            vecType = []
            for v in ('lines', 'points'):
                if UserSettings.Get(group='nviz', key='vector',
                                    subkey=[v, 'show']):
                    vecType.append(v)
        id = -1
        for type in vecType:
            if type == 'lines':
                id = self.nvizClass.LoadVector(str(layer.name), False)
            else:
                id = self.nvizClass.LoadVector(str(layer.name), True)

            # set default/workspace layer properties
            data = self.SetLayerData(item, id, type)['vector']

            if id < 0:
                print >> sys.stderr, _("Vector map <%s> (%s) not loaded") % \
                    (layer.name, type)
                continue

            self.layers['v' + type]['name'].append(layer.name)
            self.layers['v'  + type]['id'].append(id)

        if id < 0:
            self.SetLayerData(item, id, 'lines')
            self.SetLayerData(item, id, 'points')
        
        # update properties
        self.UpdateLayerProperties(item)
        
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
                raise gcmd.NvizError(parent=self.parent,
                                     message=_("Unable to unload vector map <%s>" % layer.name))

            data[vtype].pop('object')

            idx = self.layers['v' + vtype]['id'].index(id)
            del self.layers['v' + vtype]['name'][idx]
            del self.layers['v' + vtype]['id'][idx]

        # update tools window
        if hasattr(self.parent, "nvizToolWin") and \
                vecType is None:
            toolWin = self.parent.nvizToolWin
            # remove surface page
            if toolWin.notebook.GetSelection() == toolWin.page['surface']['id']:
                toolWin.notebook.RemovePage(toolWin.page['surface']['id'])
                toolWin.page['surface']['id'] = -1
                toolWin.page['settings']['id'] = 1


    def SetVectorDefaultProp(self, data):
        """Set default vector properties"""
        self.SetVectorLinesDefaultProp(data['lines'])
        self.SetVectorPointsDefaultProp(data['points'])

    def SetVectorLinesDefaultProp(self, data):
        """Set default vector properties -- lines"""
        # width
        data['width'] = UserSettings.Get(group='nviz', key='vector',
                                                  subkey=['lines', 'width'])
        
        # color
        value = UserSettings.Get(group='nviz', key='vector',
                                 subkey=['lines', 'color'])
        color = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
        data['color'] = color

        # mode
        if UserSettings.Get(group='nviz', key='vector',
                            subkey=['lines', 'flat']):
            type = 'flat'
            map  = None
        else:
            if len(self.layers['raster']['name']) > 0:
                type = 'surface'
                map  = self.layers['raster']['name'][0]
            else:
                type = 'flat'
                map = None

        data['mode'] = {}
        data['mode']['type'] = type
        if map:
            data['mode']['surface'] = map

        # height
        data['height'] = UserSettings.Get(group='nviz', key='vector',
                                                   subkey=['lines', 'height'])

        if data.has_key('object'):
            self.update.append('vector:lines:color')
            self.update.append('vector:lines:width')
            self.update.append('vector:lines:mode')
            self.update.append('vector:lines:height')

    def SetVectorPointsDefaultProp(self, data):
        """Set default vector properties -- points"""
        # size
        data['size'] = UserSettings.Get(group='nviz', key='vector',
                                        subkey=['points', 'size'])

        # width
        data['width'] = UserSettings.Get(group='nviz', key='vector',
                                         subkey=['points', 'width'])

        # marker
        data['marker'] = UserSettings.Get(group='nviz', key='vector',
                                          subkey=['points', 'marker'])

        # color
        value = UserSettings.Get(group='nviz', key='vector',
                                 subkey=['points', 'color'])
        color = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
        data['color'] = color

        # mode
        data['mode'] = { 'type' : 'surface',
                         'surface' : '' }
        if len(self.layers['raster']['name']) > 0:
            data['mode']['surface'] = self.layers['raster']['name'][0]
        
        if data.has_key('object'):
            self.update.append('vector:points:size')
            self.update.append('vector:points:width')
            self.update.append('vector:points:marker')
            self.update.append('vector:points:color')
            self.update.append('vector:points:surface')
        
    def Reset(self):
        """Reset (unload data)"""
        self.nvizClass.Reset()
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
            self.iview['height']['value'] = self.nvizClass.SetViewDefault()

        self.view['pos']['x'] = UserSettings.Get(group='nviz', key='view',
                                                 subkey=('pos', 'x'))
        self.view['pos']['y'] = UserSettings.Get(group='nviz', key='view',
                                                 subkey=('pos', 'x'))
        self.view['persp']['value'] = UserSettings.Get(group='nviz', key='view',
                                                       subkey=('persp', 'value'))

        self.view['twist']['value'] = UserSettings.Get(group='nviz', key='view',
                                                       subkey=('twist', 'value'))

        self.update.append('view')

    def UpdateLayerProperties(self, layer=None):
        """Update data layer properties

        @param layer layer item or None (for selected item)
        """
        if not layer:
            mapLayer = self.GetSelectedLayer()
            data = self.GetSelectedLayer(type='nviz')
        else:
            mapLayer = self.tree.GetPyData(layer)[0]['maplayer']
            data = self.tree.GetPyData(layer)[0]['nviz']

        if mapLayer.type == 'raster':
            id = data['surface']['object']['id']
            self.UpdateRasterProperties(id, data['surface'])
            # -> initialized
            data['surface']['object']['init'] = True

        elif mapLayer.type == 'vector':
            for type in ('lines', 'points'):
                if data['vector'][type].has_key('object'):
                    id = data['vector'][type]['object']['id']
                    self.UpdateVectorProperties(id, data['vector'], type)
                    # -> initialized
                    data['vector'][type]['object']['init'] = True

    def UpdateRasterProperties(self, id, data):
        """Apply changes for surfaces"""

        # surface attributes
        for attrb in ('topo', 'color', 'mask',
                     'transp', 'shine', 'emit'):
            if 'surface:attribute:%s' % attrb in self.update:
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
                self.update.remove('surface:attribute:%s' % attrb)

        # draw res
        if 'surface:draw:resolution' in self.update:
            coarse = data['draw']['resolution']['coarse']
            fine   = data['draw']['resolution']['fine']

            if data['draw']['resolution']['all']:
                self.nvizClass.SetSurfaceRes(-1, fine, coarse)
            else:
                self.nvizClass.SetSurfaceRes(id, fine, coarse)
            self.update.remove('surface:draw:resolution')

        # draw style
        if 'surface:draw:mode' in self.update:
            if data['draw']['mode']['value'] < 0: # need to calculate
                data['draw']['mode']['value'] = \
                    self.GetSurfaceMode(mode=data['draw']['mode']['desc']['mode'],
                                        style=data['draw']['mode']['desc']['style'],
                                        shade=data['draw']['mode']['desc']['shading'],
                                        string=True)
            style = data['draw']['mode']['value']
            if data['draw']['mode']['all']:
                self.nvizClass.SetSurfaceStyle(-1, style)
            else:
                self.nvizClass.SetSurfaceStyle(id, style)
            self.update.remove('surface:draw:mode')

        # wire color
        if 'surface:draw:wire-color' in self.update:
            color = data['draw']['wire-color']['value']
            if data['draw']['wire-color']['all']:
                self.nvizClass.SetWireColor(-1, str(color))
            else:
                self.nvizClass.SetWireColor(id, str(color))
                self.update.remove('surface:draw:wire-color')
        
        # position
        if 'surface:position' in self.update:
            x = data['position']['x']
            y = data['position']['y']
            z = data['position']['z']
            self.nvizClass.SetSurfacePosition(id, x, y, z)
            self.update.remove('surface:position')

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
        """Apply changes for vector line layer"""
        # mode
        if 'vector:lines:color' in self.update or \
                'vector:lines:width' in self.update or \
                'vector:lines:mode' in self.update:
            width = data['width']
            color = data['color']
            if data['mode']['type'] == 'flat':
                flat = True
                if data.has_key('surface'):
                    data.pop('surface')
            else:
                flat = False
                if not 'vector:lines:surface' in self.update:
                    self.update.append('vector:lines:surface')

            self.nvizClass.SetVectorLineMode(id, color,
                                             width, flat)
            if 'vector:lines:color' in self.update:
                self.update.remove('vector:lines:color')
            if 'vector:lines:width' in self.update:
                self.update.remove('vector:lines:width')
            if 'vector:lines:mode' in self.update:
                self.update.remove('vector:lines:mode')
        # height
        if 'vector:lines:height' in self.update:
            self.nvizClass.SetVectorLineHeight(id,
                                               data['height'])
            self.update.remove('vector:lines:height')

        # surface
        if 'vector:lines:surface' in self.update:
            idx = self.layers['raster']['name'].index(data['mode']['surface'])
            if idx > -1:
                self.nvizClass.SetVectorLineSurface(id,
                                                    self.layers['raster']['id'][idx])
            self.update.remove('vector:lines:surface')

    def UpdateVectorPointsProperties(self, id, data):
        """Apply changes for vector point layer"""
        if 'vector:points:size' in self.update or \
                'vector:points:width' in self.update or \
                'vector:points:marker' in self.update or \
                'vector:points:color' in self.update:
            
            ret = self.nvizClass.SetVectorPointMode(id, data['color'],
                                                    data['width'], float(data['size']),
                                                    data['marker'] + 1)

            error = None
            if ret == -1:
                error = _("Vector point layer not found (id=%d)") % id
            elif ret == -2:
                error = _("Unable to set data layer properties (id=%d)") % id

            if error:
                raise gcmd.NvizError(parent=self.parent,
                                     message=_("Setting data layer properties failed.\n\n%s") % error)

            for prop in ('size', 'width', 'marker', 'color'):
                if 'vector:points:%s' % prop in self.update:
                    self.update.remove('vector:points:%s' % prop)
        
        # height
        if 'vector:points:height' in self.update:
            self.nvizClass.SetVectorPointHeight(id,
                                                data['height'])
            self.update.remove('vector:points:height')

        # surface
        if 'vector:points:surface' in self.update:
            idx = self.layers['raster']['name'].index(data['mode']['surface'])
            if idx > -1:
                self.nvizClass.SetVectorPointSurface(id,
                                                     self.layers['raster']['id'][idx])
            self.update.remove('vector:points:surface')

class NvizToolWindow(wx.Frame):
    """Experimental window for Nviz tools

    @todo integrate with Map display
    """
    def __init__(self, parent=None, id=wx.ID_ANY, title=_("Nviz tools"),
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_FRAME_STYLE, mapWindow=None):
        
        self.parent = parent # MapFrame
        self.lmgr = self.parent.gismanager # GMFrame
        self.mapWindow = mapWindow

        wx.Frame.__init__(self, parent, id, title, pos, size, style)

        #
        # icon
        #
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCDIR, 'grass_nviz.ico'), wx.BITMAP_TYPE_ICO))

        #
        # dialog body
        #
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        self.win = {} # window ids

        #
        # notebook
        #
        self.notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)

        self.page = {}
        # view page
        self.__createViewPage()
        self.page['view'] = { 'id' : 0 }
        # surface page
        size = self.__createSurfacePage()
        size = (size[0] + 25, size[0] + 20)
        # vector page
        self.__createVectorPage()
        # settings page
        self.__createSettingsPage()
        self.page['settings'] = { 'id' : 1 }
        self.UpdatePage('settings')
        self.pageChanging = False

        mainSizer.Add(item=self.notebook, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=5)

        #
        # bindings
        #
        self.Bind(wx.EVT_CLOSE, self.OnClose)

        #
        # layout
        #
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

        self.SetSize(size)

    def __createViewPage(self):
        """Create view settings page"""
        panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
        self.notebook.AddPage(page=panel,
                              text=" %s " % _("View"))
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        self.win['view'] = {}

        # position
        posSizer = wx.GridBagSizer(vgap=3, hgap=3)
        posSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("W")),
                     pos=(1, 0), flag=wx.ALIGN_CENTER)
        posSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("N")),
                     pos=(0, 1), flag=wx.ALIGN_CENTER | wx.ALIGN_BOTTOM)
        viewPos = ViewPositionWindow(panel, id=wx.ID_ANY, size=(175, 175),
                                     mapwindow=self.mapWindow)
        self.win['view']['pos'] = viewPos.GetId()
        posSizer.Add(item=viewPos,
                     pos=(1, 1), flag=wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL)
        posSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("S")),
                     pos=(2, 1), flag=wx.ALIGN_CENTER | wx.ALIGN_TOP)
        posSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("E")),
                     pos=(1, 2), flag=wx.ALIGN_CENTER)
        gridSizer.Add(item=posSizer, pos=(0, 0))
                  
        # perspective
        range = UserSettings.Get(group='nviz', key='view', subkey='persp', internal=True)
        self.CreateControl(panel, dict=self.win['view'], name='persp',
                           range=(range['min'], range['max']),
                           bind=(self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        gridSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Perspective:")),
                      pos=(1, 0), flag=wx.ALIGN_CENTER)
        gridSizer.Add(item=self.FindWindowById(self.win['view']['persp']['slider']), pos=(2, 0))
        gridSizer.Add(item=self.FindWindowById(self.win['view']['persp']['spin']), pos=(3, 0),
                      flag=wx.ALIGN_CENTER)        

        # twist
        range = UserSettings.Get(group='nviz', key='view', subkey='twist', internal=True)
        self.CreateControl(panel, dict=self.win['view'], name='twist',
                           range=(range['min'], range['max']),
                           bind=(self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        gridSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Twist:")),
                      pos=(1, 1), flag=wx.ALIGN_CENTER)
        gridSizer.Add(item=self.FindWindowById(self.win['view']['twist']['slider']), pos=(2, 1))
        gridSizer.Add(item=self.FindWindowById(self.win['view']['twist']['spin']), pos=(3, 1),
                      flag=wx.ALIGN_CENTER)        

        # height + z-exag
        self.CreateControl(panel, dict=self.win['view'], name='height', sliderHor=False,
                           range=(self.mapWindow.view['height']['min'], self.mapWindow.view['height']['max']),
                           bind=(self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        self.CreateControl(panel, dict=self.win['view'], name='z-exag', sliderHor=False,
                           range=(0, 1),
                           bind=(self.OnViewChange, self.OnViewChanged, self.OnViewChangedSpin))
        heightSizer = wx.GridBagSizer(vgap=3, hgap=3)
        heightSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Height:")),
                      pos=(0, 0), flag=wx.ALIGN_LEFT, span=(1, 2))
        heightSizer.Add(item=self.FindWindowById(self.win['view']['height']['slider']),
                        flag=wx.ALIGN_RIGHT, pos=(1, 0))
        heightSizer.Add(item=self.FindWindowById(self.win['view']['height']['spin']),
                        flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos=(1, 1))
        heightSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY, label=_("Z-exag:")),
                      pos=(0, 2), flag=wx.ALIGN_LEFT, span=(1, 2))
        heightSizer.Add(item=self.FindWindowById(self.win['view']['z-exag']['slider']),
                        flag=wx.ALIGN_RIGHT, pos=(1, 2))
        heightSizer.Add(item=self.FindWindowById(self.win['view']['z-exag']['spin']),
                        flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT | wx.TOP |
                        wx.BOTTOM | wx.RIGHT, pos=(1, 3))

        gridSizer.Add(item=heightSizer, pos=(0, 1), flag=wx.ALIGN_RIGHT)

        # view setup + reset
        viewSizer = wx.BoxSizer(wx.HORIZONTAL)

        viewSizer.Add(item=wx.StaticText(panel, id=wx.ID_ANY,
                                         label=_("Look at:")),
                      flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL, border=5)
        
        viewType = wx.Choice (parent=panel, id=wx.ID_ANY, size=(125, -1),
                              choices = [_("top"),
                                         _("north"),
                                         _("south"),
                                         _("east"),
                                         _("west"),
                                         _("north-west"),
                                         _("north-east"),
                                         _("south-east"),
                                         _("south-west")])
        viewType.SetSelection(0)
        viewType.Bind(wx.EVT_CHOICE, self.OnLookAt)
        # self.win['lookAt'] = viewType.GetId()
        viewSizer.Add(item=viewType, flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL,
                      border=5)

        reset = wx.Button(panel, id=wx.ID_ANY, label=_("Reset"))
        reset.SetToolTipString(_("Reset to default view"))
        # self.win['reset'] = reset.GetId()
        reset.Bind(wx.EVT_BUTTON, self.OnResetView)

        viewSizer.Add(item=reset, proportion=1,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT,
                      border=5)

        gridSizer.AddGrowableCol(3)
        gridSizer.Add(item=viewSizer, pos=(4, 0), span=(1, 2),
                      flag=wx.EXPAND)

        # body
        pageSizer.Add(item=gridSizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL,
                      border=5)

        panel.SetSizer(pageSizer)

        return panel.GetBestSize()

    def __createSurfacePage(self):
        """Create view settings page"""
        panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
        self.page['surface'] = {}
        self.page['surface']['id'] = -1
        self.page['surface']['panel'] = panel.GetId()

        # panel = scrolled.ScrolledPanel(parent=self.notebook, id=wx.ID_ANY)
        # panel.SetupScrolling(scroll_x=True, scroll_y=True)

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        self.win['surface'] = {}
        #
        # surface attributes
        #
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Surface attributes")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        # labels
        # col = 0
        #         for type in (_("Attribute"),
        #                      _("Use"),
        #                      _("Map"),
        #                      _("Constant")):
        #             gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
        #                                              label=type),
        #                           pos=(0, col))
        #             col += 1

        # type 
        self.win['surface']['attr'] = {}
        row = 0
        for code, attrb in (('topo', _("Topography")),
                           ('color', _("Color")),
                           ('mask', _("Mask")),
                           ('transp', _("Transparency")),
                           ('shine', _("Shininess")),
                           ('emit', _("Emission"))):
            self.win['surface'][code] = {} 
            gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                             label=attrb + ':'),
                          pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
            use = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                             choices = [_("map")])
            if code not in ('topo', 'color', 'shine'):
                use.Insert(item=_("unset"), pos=0)
                self.win['surface'][code]['required'] = False
            else:
                self.win['surface'][code]['required'] = True
            if code != 'mask':
                use.Append(item=_('constant'))
            self.win['surface'][code]['use'] = use.GetId()
            use.Bind(wx.EVT_CHOICE, self.OnSurfaceUse)
            gridSizer.Add(item=use, flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(row, 1))

            map = gselect.Select(parent=panel, id=wx.ID_ANY,
                                 # size=globalvar.DIALOG_GSELECT_SIZE,
                                 size=(200, -1),
                                 type="raster")
            self.win['surface'][code]['map'] = map.GetId() - 1 # FIXME
            map.Bind(wx.EVT_TEXT, self.OnSurfaceMap)
            # changing map topography not allowed
            if code == 'topo':
                map.Enable(False)
            gridSizer.Add(item=map, flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(row, 2))

            if code == 'color':
                value = csel.ColourSelect(panel, id=wx.ID_ANY,
                                          colour=UserSettings.Get(group='nviz', key='surface',
                                                                  subkey=['color', 'value']))
                value.Bind(csel.EVT_COLOURSELECT, self.OnSurfaceMap)
            elif code == 'mask':
                value = None
            else:
                value = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                                    initial=0)
                if code == 'topo':
                    value.SetRange(minVal=-1e9, maxVal=1e9)
                elif code in ('shine', 'transp', 'emit'):
                    value.SetRange(minVal=0, maxVal=255)
                else:
                    value.SetRange(minVal=0, maxVal=100)
                value.Bind(wx.EVT_TEXT, self.OnSurfaceMap)
            
            if value:
                self.win['surface'][code]['const'] = value.GetId()
                value.Enable(False)
                gridSizer.Add(item=value, flag=wx.ALIGN_CENTER_VERTICAL,
                              pos=(row, 3))
            else:
                self.win['surface'][code]['const'] = None

            self.SetSurfaceUseMap(code) # -> enable map / disable constant
                
            row += 1

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=5)

        #
        # draw
        #
        self.win['surface']['draw'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Draw")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        gridSizer.AddGrowableCol(4)

        # mode
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Mode:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        mode = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                          choices = [_("coarse"),
                                     _("fine"),
                                     _("both")])
        mode.SetSelection(0)
        mode.SetName("selection")
        mode.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        self.win['surface']['draw']['mode'] = mode.GetId()
        gridSizer.Add(item=mode, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 1))

        # resolution (mode)
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Resolution:")),
                      pos=(0, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        resSizer = wx.BoxSizer(wx.HORIZONTAL)
        resSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                        label=_("coarse:")),
                     flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL, border=3)
        resC = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=1,
                           min=1,
                           max=100)
        resC.SetName("value")
        self.win['surface']['draw']['res-coarse'] = resC.GetId()
        resC.Bind(wx.EVT_SPINCTRL, self.OnSurfaceResolution)
        resSizer.Add(item=resC, flag=wx.ALL, border=3)
        
        resSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                        label=_("fine:")),
                     flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL, border=3)
        resF = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=1,
                           min=1,
                           max=100)
        resF.SetName("value")
        self.win['surface']['draw']['res-fine'] = resF.GetId()
        resF.Bind(wx.EVT_SPINCTRL, self.OnSurfaceResolution)
        resSizer.Add(item=resF, flag=wx.ALL, border=3)

        gridSizer.Add(item=resSizer, pos=(0, 3), span=(1, 2))

        # style
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Coarse style:")),
                      pos=(1, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        style = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                          choices = [_("wire"),
                                     _("surface")])
        style.SetName("selection")
        self.win['surface']['draw']['style'] = style.GetId()
        style.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        gridSizer.Add(item=style, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(1, 1))

        # shading
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Shading:")),
                      pos=(1, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        shade = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                           choices = [_("flat"),
                                      _("gouraud")])
        shade.SetName("selection")
        self.win['surface']['draw']['shading'] = shade.GetId()
        shade.Bind(wx.EVT_CHOICE, self.OnSurfaceMode)
        gridSizer.Add(item=shade, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(1, 3))

        # color
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Wire color:")),
                      pos=(2, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        color = csel.ColourSelect(panel, id=wx.ID_ANY)
        color.SetName("colour")
        color.Bind(csel.EVT_COLOURSELECT, self.OnSurfaceWireColor)
        self.win['surface']['draw']['wire-color'] = color.GetId()
        gridSizer.Add(item=color, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(2, 1))

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        all = wx.Button(panel, id=wx.ID_ANY, label=_("All"))
        all.SetToolTipString(_("Use for all loaded surfaces"))
        # self.win['reset'] = reset.GetId()
        all.Bind(wx.EVT_BUTTON, self.OnSurfaceModeAll)
        gridSizer.Add(item=all, flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                      pos=(2, 4))

        #
        # mask
        #
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Mask")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Mask zeros:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        elev = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                           label=_("by elevation"))
        elev.Enable(False) # TODO: not implemented yet
        gridSizer.Add(item=elev, pos=(0, 1))

        color = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                           label=_("by color"))
        color.Enable(False) # TODO: not implemented yet
        gridSizer.Add(item=color, pos=(0, 2))

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        #
        # position
        #
        self.win['surface']['position'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Position")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        # position
        axis = wx.Choice (parent=panel, id=wx.ID_ANY, size=(75, -1),
                          choices = ["X",
                                     "Y",
                                     "Z"])
        axis.SetSelection(0)
        self.win['surface']['position']['axis'] = axis.GetId()
        axis.Bind(wx.EVT_CHOICE, self.OnSurfaceAxis)
        gridSizer.Add(item=axis, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 0))
        value = wx.Slider(parent=panel, id=wx.ID_ANY,
                          value=0,
                          minValue=-1e4,
                          maxValue=1e4,
                          style=wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | \
                              wx.SL_TOP | wx.SL_LABELS,
                          size=(350, -1))
        self.win['surface']['position']['pos'] = value.GetId()
        value.Bind(wx.EVT_SCROLL, self.OnSurfacePosition)
        gridSizer.Add(item=value, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 1))

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)
        
        panel.SetSizer(pageSizer)
        
        return panel.GetBestSize()

    def __createVectorPage(self):
        """Create view settings page"""
        panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
        self.page['vector'] = {}
        self.page['vector']['id'] = -1
        self.page['vector']['panel'] = panel.GetId()

        pageSizer = wx.BoxSizer(wx.VERTICAL)

        self.win['vector'] = {}

        #
        # vector lines
        #
        self.win['vector']['lines'] = {}

        showLines = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("Show vector lines"))
        self.win['vector']['lines']['show'] = showLines.GetId()
        showLines.Bind(wx.EVT_CHECKBOX, self.OnVectorShow)

        pageSizer.Add(item=showLines, proportion=0,
                      flag=wx.ALL | wx.EXPAND, border=5)

        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Vector lines")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        # width
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Width:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        width = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                            initial=1,
                            min=1,
                            max=100)
        self.win['vector']['lines']['width'] = width.GetId()
        width.Bind(wx.EVT_SPINCTRL, self.OnVectorLines)
        gridSizer.Add(item=width, pos=(0, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.AddGrowableCol(2)

        # color
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Color:")),
                      pos=(0, 3), flag=wx.ALIGN_CENTER_VERTICAL)

        color = csel.ColourSelect(panel, id=wx.ID_ANY,
                                  colour=UserSettings.Get(group='nviz', key='vector',
                                                          subkey=['lines', 'color']))
        self.win['vector']['lines']['color'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnVectorLines)

        gridSizer.Add(item=color, pos=(0, 4))

        gridSizer.AddGrowableCol(5)

        # display
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Display:")),
                      pos=(0, 6), flag=wx.ALIGN_CENTER_VERTICAL)

        display = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                             choices = [_("on surface"),
                                        _("flat")])
        self.win['vector']['lines']['flat'] = display.GetId()
        display.Bind(wx.EVT_CHOICE, self.OnVectorDisplay)

        gridSizer.Add(item=display, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 7))

        # hight
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Hight above surface:")),
                      pos=(1, 0), flag=wx.ALIGN_CENTER_VERTICAL,
                      span=(1, 2))
        
        surface = wx.ComboBox(parent=panel, id=wx.ID_ANY, size=(250, -1),
                              style=wx.CB_SIMPLE | wx.CB_READONLY,
                              choices=[])
        surface.Bind(wx.EVT_COMBOBOX, self.OnVectorSurface)
        self.win['vector']['lines']['surface'] = surface.GetId()
        gridSizer.Add(item=surface, 
                      pos=(1, 2), span=(1, 6),
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)


        self.CreateControl(panel, dict=self.win['vector']['lines'], name='height', size=300,
                           range=(0, 1000),
                           bind=(self.OnVectorHeight, self.OnVectorHeightFull, self.OnVectorHeightSpin))
        gridSizer.Add(item=self.FindWindowById(self.win['vector']['lines']['height']['slider']),
                      pos=(2, 2), span=(1, 6))
        gridSizer.Add(item=self.FindWindowById(self.win['vector']['lines']['height']['spin']),
                      pos=(3, 4),
                      flag=wx.ALIGN_CENTER)

        boxSizer.Add(item=gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        #
        # vector points
        #
        self.win['vector']['points'] = {}

        showPoints = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("Show vector points"))
        self.win['vector']['points']['show'] = showPoints.GetId()
        showPoints.Bind(wx.EVT_CHECKBOX, self.OnVectorShow)

        pageSizer.Add(item=showPoints, proportion=0,
                      flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=5)

        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Vector points")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        # icon size
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Icon size:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        isize = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                            initial=1,
                            min=1,
                            max=1e6)
        isize.SetName('value')
        self.win['vector']['points']['size'] = isize.GetId()
        isize.Bind(wx.EVT_SPINCTRL, self.OnVectorPoints)
        gridSizer.Add(item=isize, pos=(0, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # icon width
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("width:")),
                      pos=(0, 2), flag=wx.ALIGN_CENTER_VERTICAL)

        iwidth = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                             initial=1,
                             min=1,
                             max=1e6)
        iwidth.SetName('value')
        self.win['vector']['points']['width'] = iwidth.GetId()
        iwidth.Bind(wx.EVT_SPINCTRL, self.OnVectorPoints)
        gridSizer.Add(item=iwidth, pos=(0, 3),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # icon symbol
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("symbol:")),
                      pos=(0, 4), flag=wx.ALIGN_CENTER_VERTICAL)
        isym = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                          choices=UserSettings.Get(group='nviz', key='vector',
                                                   subkey=['points', 'marker'], internal=True))
        isym.SetName("selection")
        self.win['vector']['points']['marker'] = isym.GetId()
        isym.Bind(wx.EVT_CHOICE, self.OnVectorPoints)
        gridSizer.Add(item=isym, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 5))

        # icon color
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("color:")),
                      pos=(0, 6), flag=wx.ALIGN_CENTER_VERTICAL)
        icolor = csel.ColourSelect(panel, id=wx.ID_ANY)
        icolor.SetName("color")
        self.win['vector']['points']['color'] = icolor.GetId()
        icolor.Bind(csel.EVT_COLOURSELECT, self.OnVectorPoints)
        gridSizer.Add(item=icolor, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 7))

        # high
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Hight above surface:")),
                      pos=(1, 0), flag=wx.ALIGN_CENTER_VERTICAL,
                      span=(1, 2))
        
        surface = wx.ComboBox(parent=panel, id=wx.ID_ANY, size=(250, -1),
                              style=wx.CB_SIMPLE | wx.CB_READONLY,
                              choices=[])
        surface.Bind(wx.EVT_COMBOBOX, self.OnVectorSurface)
        self.win['vector']['points']['surface'] = surface.GetId()
        gridSizer.Add(item=surface, 
                      pos=(1, 2), span=(1, 6),
                      flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT)

        self.CreateControl(panel, dict=self.win['vector']['points'], name='height', size=300,
                           range=(0, 1000),
                           bind=(self.OnVectorHeight, self.OnVectorHeightFull, self.OnVectorHeightSpin))
        gridSizer.Add(item=self.FindWindowById(self.win['vector']['points']['height']['slider']),
                      pos=(2, 2), span=(1, 6))
        gridSizer.Add(item=self.FindWindowById(self.win['vector']['points']['height']['spin']),
                      pos=(3, 4),
                      flag=wx.ALIGN_CENTER)

        
        boxSizer.Add(item=gridSizer, proportion=1,
                     flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)


        panel.SetSizer(pageSizer)

        return panel.GetBestSize()

    def __createSettingsPage(self):
        """Create settings page"""
        panel = wx.Panel(parent=self.notebook, id=wx.ID_ANY)
        self.notebook.AddPage(page=panel,
                              text=" %s " % _("Settings"))
        
        pageSizer = wx.BoxSizer(wx.VERTICAL)

        self.win['settings'] = {}

        #
        # general
        #
        self.win['settings']['general'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("General")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        # background color
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Background color:")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        color = csel.ColourSelect(panel, id=wx.ID_ANY,
                                  colour=UserSettings.Get(group='nviz', key='settings',
                                                          subkey=['general', 'bgcolor']))
        self.win['settings']['general']['bgcolor'] = color.GetId()
        color.Bind(csel.EVT_COLOURSELECT, self.OnBgColor)
        gridSizer.Add(item=color, pos=(0, 1))


        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL,
                      border=5)

        #
        # view
        #
        self.win['settings']['view'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("View")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)


        # perspective
        self.win['settings']['view']['persp'] = {}
        pvals = UserSettings.Get(group='nviz', key='view', subkey='persp')
        ipvals = UserSettings.Get(group='nviz', key='view', subkey='persp', internal=True)
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Perspective (value):")),
                      pos=(0, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        pval = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=pvals['value'],
                           min=ipvals['min'],
                           max=ipvals['max'])
        self.win['settings']['view']['persp']['value'] = pval.GetId()
        gridSizer.Add(item=pval, pos=(0, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("(step):")),
                      pos=(0, 2), flag=wx.ALIGN_CENTER_VERTICAL)

        pstep = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=pvals['step'],
                           min=ipvals['min'],
                           max=ipvals['max']-1)
        self.win['settings']['view']['persp']['step'] = pstep.GetId()
        gridSizer.Add(item=pstep, pos=(0, 3),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # position
        self.win['settings']['view']['pos'] = {}
        posvals = UserSettings.Get(group='nviz', key='view', subkey='pos')
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Position") + " (x):"),
                      pos=(1, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        px = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=posvals['x'] * 100,
                           min=0,
                           max=100)
        self.win['settings']['view']['pos']['x'] = px.GetId()
        gridSizer.Add(item=px, pos=(1, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label="(y):"),
                      pos=(1, 2), flag=wx.ALIGN_CENTER_VERTICAL)

        py = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=posvals['y'] * 100,
                           min=0,
                           max=100)
        self.win['settings']['view']['pos']['y'] = py.GetId()
        gridSizer.Add(item=py, pos=(1, 3),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # height
        self.win['settings']['view']['height'] = {}
        hvals = UserSettings.Get(group='nviz', key='view', subkey='height')
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Height") + " (min):"),
                      pos=(2, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        hmin = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=hvals['min'],
                           min=-1e6,
                           max=1e6)
        self.win['settings']['view']['height']['min'] = hmin.GetId()
        gridSizer.Add(item=hmin, pos=(2, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label="(max):"),
                      pos=(2, 2), flag=wx.ALIGN_CENTER_VERTICAL)

        hmax = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=hvals['max'],
                           min=-1e6,
                           max=1e6)
        self.win['settings']['view']['height']['max'] = hmax.GetId()
        gridSizer.Add(item=hmax, pos=(2, 3),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label="(step):"),
                      pos=(2, 4), flag=wx.ALIGN_CENTER_VERTICAL)

        hstep = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=hvals['step'],
                           min=1,
                           max=hvals['max']-1)
        self.win['settings']['view']['height']['step'] = hstep.GetId()
        gridSizer.Add(item=hstep, pos=(2, 5),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # twist
        self.win['settings']['view']['twist'] = {}
        tvals = UserSettings.Get(group='nviz', key='view', subkey='twist')
        itvals = UserSettings.Get(group='nviz', key='view', subkey='twist', internal=True)
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Twist (value):")),
                      pos=(3, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        tval = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=tvals['value'],
                           min=itvals['min'],
                           max=itvals['max'])
        self.win['settings']['view']['twist']['value'] = tval.GetId()
        gridSizer.Add(item=tval, pos=(3, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("(step):")),
                      pos=(3, 2), flag=wx.ALIGN_CENTER_VERTICAL)

        tstep = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=tvals['step'],
                           min=itvals['min'],
                           max=itvals['max']-1)
        self.win['settings']['view']['twist']['step'] = tstep.GetId()
        gridSizer.Add(item=tstep, pos=(3, 3),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # z-exag
        self.win['settings']['view']['z-exag'] = {}
        zvals = UserSettings.Get(group='nviz', key='view', subkey='z-exag')
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Z-exag (value):")),
                      pos=(4, 0), flag=wx.ALIGN_CENTER_VERTICAL)

        zval = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=zvals['value'],
                           min=-1e6,
                           max=1e6)
        self.win['settings']['view']['z-exag']['value'] = zval.GetId()
        gridSizer.Add(item=zval, pos=(4, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("(step):")),
                      pos=(4, 2), flag=wx.ALIGN_CENTER_VERTICAL)

        zstep = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                           initial=zvals['step'],
                           min=-1e6,
                           max=1e6)
        self.win['settings']['view']['z-exag']['step'] = zstep.GetId()
        gridSizer.Add(item=zstep, pos=(4, 3),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        #
        # surface
        #
        self.win['settings']['surface'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Surface")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)


        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        #
        # vector lines
        #
        self.win['settings']['vector'] = {}
        self.win['settings']['vector']['lines'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Vector lines")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=3)

        # show
        row = 0
        showLines = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("Show lines"))
        self.win['settings']['vector']['lines']['show'] = showLines.GetId()
        showLines.SetValue(UserSettings.Get(group='nviz', key='vector',
                                            subkey=['lines', 'show']))
        gridSizer.Add(item=showLines, pos=(row, 0))


        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        #
        # vector points
        #
        self.win['settings']['vector']['points'] = {}
        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % (_("Vector points")))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=3, hgap=5)

        # show
        row = 0
        showPoints = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                 label=_("Show points"))
        showPoints.SetValue(UserSettings.Get(group='nviz', key='vector',
                                             subkey=['points', 'show']))
        self.win['settings']['vector']['points']['show'] = showPoints.GetId()
        gridSizer.Add(item=showPoints, pos=(row, 0), span=(1, 8))

        # icon size
        row += 1 
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Size:")),
                      pos=(row, 0), flag=wx.ALIGN_CENTER_VERTICAL)
        
        isize = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                            initial=100,
                            min=1,
                            max=1e6)
        self.win['settings']['vector']['points']['size'] = isize.GetId()
        isize.SetValue(UserSettings.Get(group='nviz', key='vector',
                                        subkey=['points', 'size']))
        gridSizer.Add(item=isize, pos=(row, 1),
                      flag=wx.ALIGN_CENTER_VERTICAL)


        # icon width
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Witdh:")),
                      pos=(row, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        
        iwidth = wx.SpinCtrl(parent=panel, id=wx.ID_ANY, size=(65, -1),
                            initial=2,
                            min=1,
                            max=1e6)
        self.win['settings']['vector']['points']['width'] = isize.GetId()
        iwidth.SetValue(UserSettings.Get(group='nviz', key='vector',
                                         subkey=['points', 'width']))
        gridSizer.Add(item=iwidth, pos=(row, 3),
                      flag=wx.ALIGN_CENTER_VERTICAL)

        # icon symbol
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Marker:")),
                      pos=(row, 4), flag=wx.ALIGN_CENTER_VERTICAL)
        isym = wx.Choice (parent=panel, id=wx.ID_ANY, size=(100, -1),
                          choices=UserSettings.Get(group='nviz', key='vector',
                                                   subkey=['points', 'marker'], internal=True))
        isym.SetName("selection")
        self.win['settings']['vector']['points']['marker'] = isym.GetId()
        isym.SetSelection(UserSettings.Get(group='nviz', key='vector',
                                           subkey=['points', 'marker']))
        gridSizer.Add(item=isym, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 5))

        # icon color
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY,
                                         label=_("Color:")),
                      pos=(row, 6), flag=wx.ALIGN_CENTER_VERTICAL)
        icolor = csel.ColourSelect(panel, id=wx.ID_ANY)
        icolor.SetName("color")
        self.win['settings']['vector']['points']['color'] = icolor.GetId()
        icolor.SetColour(UserSettings.Get(group='nviz', key='vector',
                                          subkey=['points', 'color']))
        gridSizer.Add(item=icolor, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 7))

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=3)
        pageSizer.Add(item=boxSizer, proportion=0,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                      border=5)

        #
        # buttons
        #
        btnDefault = wx.Button(panel, wx.ID_CANCEL, label=_("Default"))
        btnSave = wx.Button(panel, wx.ID_SAVE)
        btnApply = wx.Button(panel, wx.ID_APPLY)

        btnDefault.Bind(wx.EVT_BUTTON, self.OnDefault)
        btnDefault.SetToolTipString(_("Restore default settings"))
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnApply.SetToolTipString(_("Apply changes for the current session"))
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        btnSave.SetDefault()

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnDefault)
        btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnSave)
        btnSizer.Realize()

        pageSizer.Add(item=btnSizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_RIGHT | wx.ALIGN_BOTTOM,
                      border=5)

        panel.SetSizer(pageSizer)

        return panel.GetBestSize()

    def CreateControl(self, parent, dict, name, range, bind, sliderHor=True, size=200):
        """Add control (Slider + SpinCtrl)"""
        dict[name] = {}
        if sliderHor:
            style = wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | \
                wx.SL_BOTTOM
            sizeW = (size, -1)
        else:
            style = wx.SL_VERTICAL | wx.SL_AUTOTICKS | \
                wx.SL_BOTTOM | wx.SL_INVERSE
            sizeW = (-1, size)
        try:
            val = self.mapWindow.view[name]['value']
        except KeyError:
            val=-1
        slider = wx.Slider(parent=parent, id=wx.ID_ANY,
                           value=val,
                           minValue=range[0],
                           maxValue=range[1],
                           style=style,
                           size=sizeW)
        slider.SetName('slider')
        slider.Bind(wx.EVT_SCROLL, bind[0])
        slider.Bind(wx.EVT_SCROLL_CHANGED, bind[1])
        dict[name]['slider'] = slider.GetId()

        spin = wx.SpinCtrl(parent=parent, id=wx.ID_ANY, size=(65, -1),
                           initial=val,
                           min=range[0],
                           max=range[1])
        #         spin = wx.SpinButton(parent=parent, id=wx.ID_ANY)
        #         spin.SetValue (self.mapWindow.view[name]['value'])
        #         spin.SetRange(self.mapWindow.view[name]['min'],
        #                      self.mapWindow.view[name]['max'])

        # no 'changed' event ... (FIXME)
        spin.SetName('spin')
        spin.Bind(wx.EVT_SPINCTRL, bind[2])
        dict[name]['spin'] = spin.GetId()

    def UpdateSettings(self):
        """Update dialog settings"""
        for control in ('height',
                        'persp',
                        'twist',
                        'z-exag'):
            for win in self.win['view'][control].itervalues():
                if control == 'height':
                    value = UserSettings.Get(group='nviz', key='view',
                                             subkey=['height', 'value'], internal=True)
                else:
                    value = self.mapWindow.view[control]['value']
                self.FindWindowById(win).SetValue(value)

        self.FindWindowById(self.win['view']['pos']).Draw()
        self.FindWindowById(self.win['view']['pos']).Refresh(False)
        
        self.Refresh(False)

    def __GetWindowName(self, dict, id):
        for name in dict.iterkeys():
            if type(dict[name]) is type({}):
                for win in dict[name].itervalues():
                    if win == id:
                        return name
            else:
                if dict[name] == id:
                    return name

        return None

    def OnViewChange(self, event):
        """Change view, render in quick mode"""
        # find control
        winName = self.__GetWindowName(self.win['view'], event.GetId())
        if not winName:
            return

        if winName == 'height':
            view = self.mapWindow.iview # internal
        else:
            view = self.mapWindow.view

        view[winName]['value'] = event.GetInt()

        for win in self.win['view'][winName].itervalues():
            self.FindWindowById(win).SetValue(view[winName]['value'])

        self.mapWindow.update.append('view')
        if winName == 'z-exag':
            self.mapWindow.update.append('z-exag')
        
        self.mapWindow.render['quick'] = True
        self.mapWindow.Refresh(False)

    def OnViewChanged(self, event):
        """View changed, render in full resolution"""
        self.mapWindow.render['quick'] = False
        self.mapWindow.Refresh(False)

    def OnViewChangedSpin(self, event):
        """View changed, render in full resolution"""
        # TODO: use step value instead

        self.OnViewChange(event)
        self.OnViewChanged(None)

    def OnResetView(self, event):
        """Reset to default view (view page)"""
        self.mapWindow.ResetView()
        self.UpdateSettings()
        self.mapWindow.Refresh(False)

    def OnLookAt(self, event):
        """Look at (view page)"""
        sel = event.GetSelection()
        if sel == 0: # top
            self.mapWindow.view['pos']['x'] = 0.5
            self.mapWindow.view['pos']['y'] = 0.5
        elif sel == 1: # north
            self.mapWindow.view['pos']['x'] = 0.5
            self.mapWindow.view['pos']['y'] = 0.0
        elif sel == 2: # south
            self.mapWindow.view['pos']['x'] = 0.5
            self.mapWindow.view['pos']['y'] = 1.0
        elif sel == 3: # east
            self.mapWindow.view['pos']['x'] = 1.0
            self.mapWindow.view['pos']['y'] = 0.5
        elif sel == 4: # west
            self.mapWindow.view['pos']['x'] = 0.0
            self.mapWindow.view['pos']['y'] = 0.5
        elif sel == 5: # north-west
            self.mapWindow.view['pos']['x'] = 0.0
            self.mapWindow.view['pos']['y'] = 0.0
        elif sel == 6: # north-east
            self.mapWindow.view['pos']['x'] = 1.0
            self.mapWindow.view['pos']['y'] = 0.0
        elif sel == 7: # south-east
            self.mapWindow.view['pos']['x'] = 1.0
            self.mapWindow.view['pos']['y'] = 1.0
        elif sel == 8: # south-west
            self.mapWindow.view['pos']['x'] = 0.0
            self.mapWindow.view['pos']['y'] = 1.0

        self.mapWindow.update.append('view')

        self.UpdateSettings()

        self.mapWindow.Refresh(False)

    def OnDefault(self, event):
        """Restore default settings"""
        settings = copy.deepcopy(UserSettings.GetDefaultSettings()['nviz'])
        UserSettings.Set(group='nviz',
                         value=settings)
        
        for subgroup, key in settings.iteritems(): # view, surface, vector...
            if subgroup != 'view':
                continue
            for subkey, value in key.iteritems():
                for subvalue in value.keys():
                    win = self.FindWindowById(self.win['settings'][subgroup][subkey][subvalue])
                    val = settings[subgroup][subkey][subvalue]
                    if subkey == 'pos':
                        val = int(val * 100)

                    win.SetValue(val)
        
        event.Skip()

    def OnApply(self, event):
        """Apply button pressed"""
        if self.notebook.GetSelection() == self.page['settings']['id']:
            self.ApplySettings()
        
        if event:
            event.Skip()

    def ApplySettings(self):
        """Apply Nviz settings for current session"""
        settings = UserSettings.Get(group='nviz')
        for subgroup, key in settings.iteritems(): # view, surface, vector...
            for subkey, value in key.iteritems():
                for subvalue in value.keys():
                    try: # TODO
                        win = self.FindWindowById(self.win['settings'][subgroup][subkey][subvalue])
                    except:
                        # print 'e', subgroup, subkey, subvalue
                        continue
                    
                    if win.GetName() == "selection":
                        value = win.GetSelection()
                    elif win.GetName() == "color":
                        value = tuple(win.GetColour())
                    else:
                        value = win.GetValue()
                    if subkey == 'pos':
                        value = float(value) / 100
                    
                    settings[subgroup][subkey][subvalue] = value
                    
    def OnSave(self, event):
        """OK button pressed
        
        Apply changes, update map and save settings of selected layer
        """
        #
        # apply changes
        #
        self.OnApply(None)

        if self.notebook.GetSelection() == self.page['settings']['id']:
            fileSettings = {}
            UserSettings.ReadSettingsFile(settings=fileSettings)
            fileSettings['nviz'] = UserSettings.Get(group='nviz')
            file = UserSettings.SaveToFile(fileSettings)
            self.lmgr.goutput.WriteLog(_('Nviz settings saved to file <%s>.') % file)

    def OnBgColor(self, event):
        """Background color changed"""
        color = event.GetValue()
        color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])

        self.mapWindow.nvizClass.SetBgColor(str(color))

        if self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnClose(self, event):
        """Close button pressed
        
        Close dialog
        """
        self.Hide()

    def OnSurfaceUse(self, event):
        """Surface attribute -- use -- map/constant"""
        if not self.mapWindow.init:
            return

        wx.Yield()

        # find attribute row
        attrb = self.__GetWindowName(self.win['surface'], event.GetId())
        if not attrb:
            return

        selection = event.GetSelection()
        if self.win['surface'][attrb]['required']: # no 'unset'
            selection += 1
        if selection == 0: # unset
            useMap = None
            value = ''
        elif selection == 1: # map
            useMap = True
            value = self.FindWindowById(self.win['surface'][attrb]['map']).GetValue()
        elif selection == 2: # constant
            useMap = False
            if attrb == 'color':
                value = self.FindWindowById(self.win['surface'][attrb]['const']).GetColour()
                value = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
            else:
                value = self.FindWindowById(self.win['surface'][attrb]['const']).GetValue()

        self.SetSurfaceUseMap(attrb, useMap)
        
        self.mapWindow.update.append('surface:attribute:%s' % attrb)
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['surface']['attribute'][attrb] = { 'map' : useMap,
                                                'value' : str(value),
                                                }
        self.mapWindow.UpdateLayerProperties()

        if self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)

    def SetSurfaceUseMap(self, attrb, map=None):
        if attrb in ('topo', 'color', 'shine'):
            incSel = -1 # decrement selection (no 'unset')
        else:
            incSel = 0

        if map is True: # map
            if attrb != 'topo': # changing map topography not allowed
                self.FindWindowById(self.win['surface'][attrb]['map'] + 1).Enable(True) # FIXME
            if self.win['surface'][attrb]['const']:
                self.FindWindowById(self.win['surface'][attrb]['const']).Enable(False)
            self.FindWindowById(self.win['surface'][attrb]['use']).SetSelection(1 + incSel)
        elif map is False: # const
            self.FindWindowById(self.win['surface'][attrb]['map'] + 1).Enable(False)
            if self.win['surface'][attrb]['const']:
                self.FindWindowById(self.win['surface'][attrb]['const']).Enable(True)
            self.FindWindowById(self.win['surface'][attrb]['use']).SetSelection(2 + incSel)
        else: # unset
            self.FindWindowById(self.win['surface'][attrb]['map'] + 1).Enable(False)
            if self.win['surface'][attrb]['const']:
                self.FindWindowById(self.win['surface'][attrb]['const']).Enable(False)
            self.FindWindowById(self.win['surface'][attrb]['use']).SetSelection(0)

    def OnSurfaceMap(self, event):
        """Set surface attribute"""
        if not self.mapWindow.init:
            return

        attrb = self.__GetWindowName(self.win['surface'], event.GetId()) 
        if not attrb:
            return

        selection = self.FindWindowById(self.win['surface'][attrb]['use']).GetSelection()
        if self.win['surface'][attrb]['required']:
            selection += 1

        if selection == 0: # unset
            map = None
            value = ''
        elif selection == 1: # map
            value = self.FindWindowById(self.win['surface'][attrb]['map']).GetValue()
            map = True
        else: # constant
            if attrb == 'color':
                value = self.FindWindowById(self.win['surface'][attrb]['const']).GetColour()
                # tuple to string
                value = str(value[0]) + ':' + str(value[1]) + ':' + str(value[2])
            else:
                value = self.FindWindowById(self.win['surface'][attrb]['const']).GetValue()
            map = False
        
        if not self.pageChanging:
            self.mapWindow.update.append('surface:attribute:%s' % attrb)
            data = self.mapWindow.GetSelectedLayer(type='nviz')
            data['surface']['attribute'][attrb] = { 'map' : map,
                                                    'value' : str(value),
                                                    }
            self.mapWindow.UpdateLayerProperties()

            if self.parent.autoRender.IsChecked():
                self.mapWindow.Refresh(False)

    def OnSurfaceResolution(self, event):
        """Draw resolution changed"""
        self.SetSurfaceResolution()

        if apply and self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)

    def SetSurfaceResolution(self, all=False):
        """Set draw resolution"""
        coarse = self.FindWindowById(self.win['surface']['draw']['res-coarse']).GetValue()
        fine = self.FindWindowById(self.win['surface']['draw']['res-fine']).GetValue()
            
        self.mapWindow.update.append('surface:draw:resolution')
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['surface']['draw']['resolution'] = { 'coarse' : coarse,
                                                  'fine' : fine,
                                                  'all' : all } 

        self.mapWindow.UpdateLayerProperties()

    def SetSurfaceMode(self, all=False):
        """Set draw mode

        @param apply allow auto-rendering
        """
        mode = self.FindWindowById(self.win['surface']['draw']['mode']).GetSelection()
        if mode == 0: # coarse
            self.FindWindowById(self.win['surface']['draw']['res-coarse']).Enable(True)
            self.FindWindowById(self.win['surface']['draw']['res-fine']).Enable(False)
        elif mode == 1: # fine
            self.FindWindowById(self.win['surface']['draw']['res-coarse']).Enable(False)
            self.FindWindowById(self.win['surface']['draw']['res-fine']).Enable(True)
        else: # both
            self.FindWindowById(self.win['surface']['draw']['res-coarse']).Enable(True)
            self.FindWindowById(self.win['surface']['draw']['res-fine']).Enable(True)

        style = self.FindWindowById(self.win['surface']['draw']['style']).GetSelection()

        shade = self.FindWindowById(self.win['surface']['draw']['shading']).GetSelection()

        value, desc = self.mapWindow.GetSurfaceMode(mode, style, shade)

        return value, desc

    def OnSurfaceMode(self, event):
        """Set draw mode"""
        value, desc = self.SetSurfaceMode()

        self.mapWindow.update.append('surface:draw:mode')
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['surface']['draw']['mode'] = { 'value' : value,
                                            'all' : False,
                                            'desc' : desc }

        self.mapWindow.UpdateLayerProperties()

        if apply and self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)

    def OnSurfaceModeAll(self, event):
        """Set draw mode (including wire color) for all loaded surfaces"""
        self.SetSurfaceMode(all=True)
        self.SetSurfaceResolution(all=True)
        color = self.FindWindowById(self.win['surface']['draw']['wire-color']).GetColour()
        self.SetSurfaceWireColor(color, all=True)

        if apply and self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)

    def SetSurfaceWireColor(self, color, all=False, apply=True):
        """Set wire color"""
        value = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])

    def OnSurfaceWireColor(self, event):
        """Set wire color"""
        self.SetSurfaceWireColor(event.GetValue())

        self.mapWindow.update.append('surface:draw:wire-color')
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['surface']['draw']['wire-color'] = { 'value' : value,
                                                  'all' : all }
        self.mapWindow.UpdateLayerProperties()

        if self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)

    def OnSurfaceAxis(self, event):
        """Surface position, axis changed"""
        mapLayer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        id = data['object']['id']

        axis = self.FindWindowById(self.win['surface']['position']['axis']).GetSelection()
        win = self.FindWindowById(self.win['surface']['position']['pos'])

        x, y, z = self.mapWindow.nvizClass.GetSurfacePosition(id)

        if axis == 0: # x
            win.SetRange(-1e4, 1e4)
            win.SetValue(x)
        elif axis == 1: # y
            win.SetRange(-1e4, 1e4)
            win.SetValue(y)
        else: # z
            win.SetRange(-1e3, 1e3)
            win.SetValue(z)

    def OnSurfacePosition(self, event):
        """Surface position"""
        axis = self.FindWindowById(self.win['surface']['position']['axis']).GetSelection()
        value = event.GetInt()

        mapLayer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        id = data['object']['id']
        x, y, z = self.mapWindow.nvizClass.GetSurfacePosition(id)

        if axis == 0: # x
            x = value
        elif axis == 1: # y
            y = value
        else: # z
            z = value
        
        self.mapWindow.update.append('surface:position')
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['surface']['position']['x'] = x
        data['surface']['position']['y'] = y
        data['surface']['position']['z'] = z
        
        self.mapWindow.UpdateLayerProperties()

        if self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)

    def UpdateVectorShow(self, vecType, enabled):
        """Enable/disable lines/points widgets

        @param vecType vector type (lines, points)
        """
        if vecType != 'lines' and vecType != 'points':
            return False

        for win in self.win['vector'][vecType].keys():
            if win == 'show':
                continue
            if type(self.win['vector'][vecType][win]) == type({}):
                for swin in self.win['vector'][vecType][win].keys():
                    if enabled:
                        self.FindWindowById(self.win['vector'][vecType][win][swin]).Enable(True)
                    else:
                        self.FindWindowById(self.win['vector'][vecType][win][swin]).Enable(False)
            else:
                if enabled:
                    self.FindWindowById(self.win['vector'][vecType][win]).Enable(True)
                else:
                    self.FindWindowById(self.win['vector'][vecType][win]).Enable(False)

        return True
    
    def OnVectorShow(self, event):
        """Show vector lines/points"""
        winId = event.GetId()

        vecType = None
        if winId == self.win['vector']['lines']['show']:
            vecType = 'lines'
        elif winId == self.win['vector']['points']['show']:
            vecType = 'points'

        if vecType is None:
            return

        checked = event.IsChecked()
        item = self.mapWindow.GetSelectedLayer(type='item')
        data = self.mapWindow.GetSelectedLayer(type='nviz')['vector']
        
        if checked:
            self.mapWindow.LoadVector(item, (vecType,))
        else:
            self.mapWindow.UnloadVector(item, (vecType,))

        self.UpdateVectorShow(vecType, checked)
        
        if checked:
            try:
                id = data[vecType]['object']['id']
            except KeyError:
                return

            self.mapWindow.SetLayerData(item, id, vecType)
        
            # update properties
            self.mapWindow.UpdateLayerProperties(item)

        if self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)

        event.Skip()
    
    def OnVectorDisplay(self, event):
        """Display vector lines on surface/flat"""
        if event.GetSelection() == 0: # surface
            if len(self.mapWindow.layers['raster']['name']) < 1:
                event.Veto()
                return

            self.FindWindowById(self.win['vector']['lines']['surface']).Enable(True)
            # set first found surface
            data = self.mapWindow.GetSelectedLayer(type='nviz')
            data['vector']['lines']['mode']['surface'] = self.mapWindow.layers['raster']['name'][0]
            self.FindWindowById(self.win['vector']['lines']['surface']).SetStringSelection( \
                self.mapWindow.layers['raster']['name'][0])
        else: # flat
            self.FindWindowById(self.win['vector']['lines']['surface']).Enable(False)

        self.OnVectorLines(event)

        event.Skip()

    def OnVectorLines(self, event):
        """Set vector lines mode, apply changes if auto-rendering is enabled"""
        width = self.FindWindowById(self.win['vector']['lines']['width']).GetValue()

        color = self.FindWindowById(self.win['vector']['lines']['color']).GetColour()
        color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])

        mode = {}
        if self.FindWindowById(self.win['vector']['lines']['flat']).GetSelection() == 0:
            mode['type'] = 'surface'
            mode['surface'] = self.FindWindowById(self.win['vector']['lines']['surface']).GetValue()
            self.mapWindow.update.append('vector:lines:surface')
        else:
            mode['type'] = 'flat'

        self.mapWindow.update.append('vector:lines:width')
        self.mapWindow.update.append('vector:lines:color')
        self.mapWindow.update.append('vector:lines:mode')
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        data['vector']['lines']['width'] = width
        data['vector']['lines']['color'] = color
        data['vector']['lines']['mode'] = mode

        self.mapWindow.UpdateLayerProperties()
                
        if self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVectorHeight(self, event):
        value = event.GetInt()
        id = event.GetId()
        if id == self.win['vector']['lines']['height']['spin'] or \
                id == self.win['vector']['lines']['height']['slider']:
            vtype = 'lines'
        else:
            vtype = 'points'
        
        if type(event) == type(wx.ScrollEvent()):
            # slider
            win = self.FindWindowById(self.win['vector'][vtype]['height']['spin'])
        else:
            # spin
            win = self.FindWindowById(self.win['vector'][vtype]['height']['slider'])
        win.SetValue(value)

        self.mapWindow.update.append('vector:%s:height' % vtype)
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        
        data['vector'][vtype]['height'] = value

        self.mapWindow.UpdateLayerProperties()

        self.mapWindow.render['quick'] = True
        self.mapWindow.render['v' + vtype] = True
        self.mapWindow.Refresh(False)
    
    def OnVectorHeightFull(self, event):
        """Vector height changed, render in full resolution"""
        id = event.GetId()
        if id == self.win['vector']['lines']['height']['spin'] or \
                id == self.win['vector']['lines']['height']['slider']:
            vtype = 'lines'
        else:
            vtype = 'points'

        self.mapWindow.render['quick'] = False
        self.mapWindow.render['v' + vtype] = False
        self.mapWindow.Refresh(False)

    def OnVectorHeightSpin(self, event):
        """Vector height changed, render in full resolution"""
        # TODO: use step value instead

        self.OnVectorHeight(event)
        self.OnVectorHeightFull(event)

    def OnVectorSurface(self, event):
        """Reference surface for vector map (lines/points)"""
        id = event.GetId()
        if id == self.win['vector']['lines']['surface']:
            vtype = 'lines'
        else:
            vtype = 'points'

        self.mapWindow.update.append('vector:%s:surface' % vtype)
        
        data['vector'][vtype]['mode']['surface'] = event.GetValue()

        self.mapWindow.UpdateLayerProperties()

        if self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)
        
    def OnVectorPoints(self, event):
        """Set vector points mode, apply changes if auto-rendering is enabled"""
        size  = self.FindWindowById(self.win['vector']['points']['size']).GetValue()
        width = self.FindWindowById(self.win['vector']['points']['width']).GetValue()

        color = self.FindWindowById(self.win['vector']['points']['color']).GetColour()
        color = str(color[0]) + ':' + str(color[1]) + ':' + str(color[2])

        marker = self.FindWindowById(self.win['vector']['points']['marker']).GetSelection()

        self.mapWindow.update.append('vector:points:size')
        self.mapWindow.update.append('vector:points:width')
        self.mapWindow.update.append('vector:points:color')
        self.mapWindow.update.append('vector:points:marker')
        data = self.mapWindow.GetSelectedLayer(type='nviz')['vector']['points']
        data['size'] = size
        data['width'] = width
        data['color'] = color
        data['marker'] = marker

        self.mapWindow.UpdateLayerProperties()
                
        if self.parent.autoRender.IsChecked():
            self.mapWindow.Refresh(False)
        
    def UpdatePage(self, pageId):
        """Update dialog (selected page)"""
        self.pageChanging = True
        layer = self.mapWindow.GetSelectedLayer()
        data = self.mapWindow.GetSelectedLayer(type='nviz')
        
        if pageId == 'view':
            max = self.mapWindow.view['z-exag']['value'] * 10
            for control in ('spin', 'slider'):
                self.FindWindowById(self.win['view']['z-exag'][control]).SetRange(0,
                                                                                  max)
        elif pageId == 'surface':
            if self.notebook.GetSelection() != self.page['surface']['id']:
                if self.page['vector']['id'] > -1:
                    self.notebook.RemovePage(self.page['vector']['id'])
                    self.page['vector']['id'] = -1

                self.page['surface']['id'] = 1
                self.page['settings']['id'] = 2

                panel = wx.FindWindowById(self.page['surface']['panel'])
                self.notebook.InsertPage(n=self.page['surface']['id'],
                                         page=panel,
                                         text=" %s " % _("Layer properties"),
                                         select=True)

            self.UpdateSurfacePage(layer, data['surface'])

        elif pageId == 'vector':
            if self.notebook.GetSelection() != self.page['vector']['id']:
                if self.page['surface']['id'] > -1:
                    self.notebook.RemovePage(self.page['surface']['id'])
                    self.page['surface']['id'] = -1

                self.page['vector']['id'] = 1
                self.page['settings']['id'] = 2

                panel = wx.FindWindowById(self.page['vector']['panel'])
                self.notebook.InsertPage(n=self.page['vector']['id'],
                                         page=panel,
                                         text=" %s " % _("Layer properties"),
                                         select=True)

            self.UpdateVectorPage(layer, data['vector'])
        
        self.pageChanging = False
        
    def UpdateSurfacePage(self, layer, data):
        #
        # attributes
        #
        for attr in ('topo', 'color'): # required
            if layer and layer.type == 'raster':
                self.FindWindowById(self.win['surface'][attr]['map']).SetValue(layer.name)
            else:
                self.FindWindowById(self.win['surface'][attr]['map']).SetValue('')
            self.SetSurfaceUseMap(attr, True) # -> map

        if data['attribute'].has_key('color'):
            value = data['attribute']['color']['value']
            if data['attribute']['color']['map']:
                self.FindWindowById(self.win['surface']['color']['map']).SetValue(value)
            else: # constant
                color = map(int, value.split(':'))
                self.FindWindowById(self.win['surface']['color']['const']).SetColour(color)
            self.SetSurfaceUseMap(attr, data['attribute']['color']['map'])

        self.SetSurfaceUseMap('shine', data['attribute']['shine']['map'])
        value = data['attribute']['shine']['value']
        if data['attribute']['shine']['map']:
            self.FindWindowById(self.win['surface']['shine']['map']).SetValue(value)
        else:
            self.FindWindowById(self.win['surface']['shine']['const']).SetValue(value)

        #
        # draw
        #
        for control, dict in data['draw'].iteritems():
            if control == 'resolution':
                self.FindWindowById(self.win['surface']['draw']['res-coarse']).SetValue(dict['coarse'])
                self.FindWindowById(self.win['surface']['draw']['res-fine']).SetValue(dict['fine'])
                continue

            if control == 'mode':
                if dict['desc']['mode'] == 'coarse':
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(0)
                elif dict['desc']['mode'] == 'fine':
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(1)
                else: # both
                    self.FindWindowById(self.win['surface']['draw']['mode']).SetSelection(2)
                    
                if dict['desc']['style'] == 'wire':
                    self.FindWindowById(self.win['surface']['draw']['style']).SetSelection(0)
                else: # surface
                    self.FindWindowById(self.win['surface']['draw']['style']).SetSelection(1)

                if dict['desc']['shading'] == 'flat':
                    self.FindWindowById(self.win['surface']['draw']['shading']).SetSelection(0)
                else: # gouraud
                    self.FindWindowById(self.win['surface']['draw']['shading']).SetSelection(1)
                
                continue

            value = dict['value']
            win = self.FindWindowById(self.win['surface']['draw'][control])
            
            name = win.GetName()

            if name == "selection":
                win.SetSelection(value)
            elif name == "colour":
                color = map(int, value.split(':'))
                win.SetColour(color)
            else:
                win.SetValue(value)
        # enable/disable res widget + set draw mode
        self.SetSurfaceMode()
        color = self.FindWindowById(self.win['surface']['draw']['wire-color'])
        self.SetSurfaceWireColor(color.GetColour())

    def UpdateVectorPage(self, layer, data):
        #
        # lines
        #
        show = self.FindWindowById(self.win['vector']['lines']['show'])
        if data['lines'].has_key('object'):
            show.SetValue(True)
        else:
            show.SetValue(False)
        self.UpdateVectorShow(show.GetId(),
                              show.IsChecked())

        width = self.FindWindowById(self.win['vector']['lines']['width'])
        width.SetValue(data['lines']['width'])

        color = self.FindWindowById(self.win['vector']['lines']['color'])
        color.SetValue(map(int, data['lines']['color'].split(':')))

        for vtype in ('lines', 'points'):
            if vtype == 'lines':
                display = self.FindWindowById(self.win['vector']['lines']['flat'])
                if data[vtype]['mode']['type'] == 'flat':
                    display.SetSelection(1)
                else:
                    display.SetSelection(0)

            if data[vtype]['mode']['type'] == 'surface' and \
                    len(self.mapWindow.layers['raster']['name']) > 0:
                surface = self.FindWindowById(self.win['vector'][vtype]['surface'])
                surface.SetItems(self.mapWindow.layers['raster']['name'])
                surface.SetStringSelection(data[vtype]['mode']['surface'])
                
        for type in ('slider', 'spin'):
            win = self.FindWindowById(self.win['vector']['lines']['height'][type])
            win.SetValue(data['lines']['height'])

        #
        # points
        #
        show = self.FindWindowById(self.win['vector']['points']['show'])
        if data['points'].has_key('object'):
            show.SetValue(True)
        else:
            show.SetValue(False)
        self.UpdateVectorShow(show.GetId(),
                              show.IsChecked())

        for prop in ('size', 'width', 'marker', 'color'):
            win = self.FindWindowById(self.win['vector']['points'][prop])
            name = win.GetName()
            if name == 'selection':
                win.SetSelection(data['points'][prop])
            elif name == 'color':
                color = map(int, data['points'][prop].split(':'))
                win.SetValue(color)
            else:
                win.SetValue(data['points'][prop])

    def SetPage(self, name):
        """Get named page"""
        self.notebook.SetSelection(self.page[name]['id'])

class ViewPositionWindow(wx.Window):
    """Position control window (for NvizToolWindow)"""
    def __init__(self, parent, id, mapwindow,
                 pos=wx.DefaultPosition,
                 size=wx.DefaultSize):
        self.mapWindow = mapwindow

        wx.Window.__init__(self, parent, id, pos, size)

        self.SetBackgroundColour("WHITE")

        self.pdc = wx.PseudoDC()

        self.pdc.SetBrush(wx.Brush(colour='dark green', style=wx.SOLID))
        self.pdc.SetPen(wx.Pen(colour='dark green', width=2, style=wx.SOLID))

        self.Draw()

        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x: None)
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        # self.Bind(wx.EVT_MOTION,       self.OnMouse)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouse)

    def Draw(self, pos=None):
        w, h = self.GetClientSize()

        if pos is None:
            x = self.mapWindow.view['pos']['x']
            y = self.mapWindow.view['pos']['y']
            x = x * w
            y = y * h
        else:
            x, y = pos

        self.pdc.Clear()
        self.pdc.BeginDrawing()
        self.pdc.DrawLine(w / 2, h / 2, x, y)
        self.pdc.DrawCircle(x, y, 5)
        self.pdc.EndDrawing()

    def OnPaint(self, event):
        dc = wx.BufferedPaintDC(self)
        dc.SetBackground(wx.Brush("White"))
        dc.Clear()

        self.PrepareDC(dc)
        self.pdc.DrawToDC(dc)

    def OnMouse(self, event):
        if event.LeftIsDown():
            x, y = event.GetPosition()
            self.Draw(pos=(x, y))
            self.Refresh(False)
            w, h = self.GetClientSize()
            x = float(x) / w
            y = float(y) / h
            if x >= 0 and x <= 1.0:
                self.mapWindow.view['pos']['x'] = x
            if y >= 0 and y <= 1.0:
                self.mapWindow.view['pos']['y'] = y
            self.mapWindow.update.append('view')

            self.mapWindow.render['quick'] = True
            self.mapWindow.Refresh(eraseBackground=False)

        elif event.LeftUp():
            self.mapWindow.render['quick'] = False
            self.mapWindow.Refresh(eraseBackground=False)
        
        event.Skip()
