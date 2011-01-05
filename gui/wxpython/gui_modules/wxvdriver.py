"""!
@package wxvdriver.py

@brief wxGUI vector digitizer (display driver)

Code based on wxVdigit C++ component from GRASS 6.4.0
(gui/wxpython/vdigit). Converted to Python in 2010/12-2011/01.

List of classes:
 - DisplayDriver

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import math

import wx

from debug import Debug as Debug
from preferences import globalSettings as UserSettings

from grass.lib.grass  import *
from grass.lib.vector import *
from grass.lib.vedit  import *

class DisplayDriver:
    def __init__(self, device, deviceTmp, mapObj, log = None):
        """Display driver used by vector digitizer
        
        @param device    wx.PseudoDC device where to draw vector objects
        @param deviceTmp wx.PseudoDC device where to draw temporary vector objects
        """
        G_gisinit('')             # initialize GRASS libs
        
        self.mapInfoObj = None    # open vector map (Map_Info structure)
        self.mapInfo    = None    # pointer to self.mapInfoObj
        
        self.dc      = device     # PseudoDC devices
        self.dcTmp   = deviceTmp
        self.mapObj  = mapObj
        self.region  = mapObj.GetCurrentRegion()
        self.log     = log        # log device
        
        # objects used by GRASS vector library
        self.points       = line_pnts()
        self.pointsScreen = list()
        self.cats         = line_cats()
        
        # selected objects
        self.selected = {
            'field'   : -1,      # field number
            'cats'    : list(),  # list of cats
            'ids'     : list(),  # list of ids
            'idsDupl' : list(),  # list of duplicated features
            }
        
        # digitizer settings
        self.settings = {
            'highlight'     : None,
            'highlightDupl' : { 'enabled' : False,
                                'color'   : None },
            'point'         : { 'enabled' : False,
                                'color'   : None },
            'line'          : { 'enabled' : False,
                                'color'   : None },
            'boundaryNo'    : { 'enabled' : False,
                                'color'   : None },
            'boundaryOne'   : { 'enabled' : False,
                                'color'   : None },
            'boundaryTwo'   : { 'enabled' : False,
                                'color'   : None },
            'centroidIn'    : { 'enabled' : False,
                                'color'   : None },
            'centroidOut'   : { 'enabled' : False,
                                'color'   : None },
            'centroidDup'   : { 'enabled' : False,
                                'color'   : None },
            'nodeOne'       : { 'enabled' : False,
                                'color'   : None },
            'nodeTwo'       : { 'enabled' : False,
                                'color'   : None },
            'vertex'        : { 'enabled' : False,
                                'color'   : None },
            'area'          : { 'enabled' : False,
                                'color'   : None },
            'direction'     : { 'enabled' : False,
                                'color'   : None },
            'lineWidth'     : -1,    # screen units 
            }
        
        # topology
        self.topology = {
            'highlight'   : 0,
            'point'       : 0,
            'line'        : 0,
            'boundaryNo'  : 0,
            'boundaryOne' : 0,
            'boundaryTwo' : 0,
            'centroidIn'  : 0,
            'centroidOut' : 0,
            'centroidDup' : 0,
            'nodeOne'     : 0,
            'nodeTwo'     : 0,
            'vertex'      : 0,
            }
        
        self.drawSelected = None
        self.drawSegments = False

        self.UpdateSettings()

        Vect_set_fatal_error(GV_FATAL_PRINT)
        
    # def __del__(self):
    #     """!Close currently open vector map"""
    #     if self.mapInfo:
    #         self.CloseMap()
    
    def _cell2Pixel(self, east, north, elev):
        """!Conversion from geographic coordinates (east, north)
        to screen (x, y)
  
        @todo 3D stuff...

        @param east, north, elev geographical coordinates

        @return x, y screen coordinates (integer)
        """
        map_res = max(self.region['ewres'], self.region['nsres'])
        w = self.region['center_easting']  - (self.mapObj.width  / 2) * map_res
        n = self.region['center_northing'] + (self.mapObj.height / 2) * map_res
        
        return int((east - w) / map_res), int((n - north) / map_res)
    
    def _drawCross(self, pdc, point, size = 5):
        """!Draw cross symbol of given size to device content
   
        Used for points, nodes, vertices

        @param[in,out] PseudoDC where to draw
        @param point coordinates of center
        @param size size of the cross symbol
   
        @return 0 on success
        @return -1 on failure
        """
        if not pdc or not point:
            return -1;
        
        pdc.DrawLine(point.x - size, point.y, point.x + size, point.y)
        pdc.DrawLine(point.x, point.y - size, point.x, point.y + size)
        
        return 0
    
    def _drawObject(self, robj):
        """!Draw given object to the device
        
        The object is defined as robject() from vedit.h.
        
        @param robj object to be rendered
        
        @return  1 on success
        @return -1 on failure (vector feature marked as dead, etc.)
        """
        if not self.dc or not self.dcTmp:
            return -1
        
        dcId = 0
        pdc = self.dc
        # draw object to the device
        pdc.SetId(dcId) # 0 | 1 (selected)
        
        Debug.msg(3, "_drawObject(): type=%d npoints=%d", robj.type, robj.npoints)
        points = list() 
        self._setPen(robj.type, pdc)
        
        for i in range(robj.npoints):
            p = robj.point[i]

            if robj.type & (TYPE_POINT | TYPE_CENTROIDIN | TYPE_CENTROIDOUT | TYPE_CENTROIDDUP |
                            TYPE_NODEONE | TYPE_NODETWO | TYPE_VERTEX): # -> point
                self._drawCross(pdc, p)
            else: # -> line
                # if dcId > 0 and self.drawSegments:
                #     dcId = 2 # first segment
                #     i = 0
                #     while (i < len(self.pointsScreen) - 2):
                #         point_beg = wx.Point(self.pointsScreen[i])
                #         point_end = wx.Point(self.pointsScreen[i + 1])
                        
                #         pdc.SetId(dcId) # set unique id & set bbox for each segment
                #         pdc.SetPen(pen)
                #         rect = wx.Rect(point_beg, point_end)
                #         pdc.SetIdBounds(dcId, rect)
                #         pdc.DrawLine(point_beg.x, point_beg.y,
                #                      point_end.x, point_end.y)
                #         i    += 2
                #         dcId += 2
                # else:
                points.append(wx.Point(p.x, p.y))
        
        if points:
            if robj.type == TYPE_AREA:
                pdc.DrawPolygon(points)
            else:
                pdc.DrawLines(points)
        
    def _setPen(self, rtype, pdc):
        """!Set pen/brush based on rendered object)
        
        Updates also self.topology dict
        """
        if rtype == TYPE_POINT:
            key = 'point'
        elif rtype == TYPE_LINE:
            key = 'line'
        elif rtype == TYPE_BOUNDARYNO:
            key = 'boundaryNo'
        elif rtype == TYPE_BOUNDARYTWO:
            key = 'boundaryTwo'
        elif rtype == TYPE_BOUNDARYONE:
            key = 'boundaryOne'
        elif rtype == TYPE_CENTROIDIN:
            key = 'centroidIn'
        elif rtype == TYPE_CENTROIDOUT:
            key = 'centroidOut'
        elif rtype == TYPE_CENTROIDDUP:
            key = 'centroidDup'
        elif rtype == TYPE_NODEONE:
            key = 'nodeOne'
        elif rtype == TYPE_NODETWO:
            key = 'nodeTwo'
        elif rtype == TYPE_VERTEX:
            key = 'vertex'
        elif rtype == TYPE_AREA:
            key = 'area' 
        elif rtype == TYPE_ISLE:
            key = 'isle'
        elif rtype == TYPE_DIRECTION:
            key = 'direction'
        
        if key not in ('direction', 'area', 'isle'):
            self.topology[key] += 1
        
        if key in ('area', 'isle'):
            pen = wx.TRANSPARENT_PEN
            if key == 'area':
                pdc.SetBrush(wx.Brush(self.settings[key]['color'], wx.SOLID))
            else:
                pdc.SetBrush(wx.TRANSPARENT_BRUSH)
        else:
            pdc.SetPen(wx.Pen(self.settings[key]['color'], self.settings['lineWidth'], wx.SOLID))
        
    def _getDrawFlag(self):
        """!Get draw flag from the settings

        See vedit.h for list of draw flags.
        
        @return draw flag (int)
        """
        ret = 0
        if self.settings['point']['enabled']:
            ret |= TYPE_POINT
        if self.settings['line']['enabled']:
            ret |= TYPE_LINE
        if self.settings['boundaryNo']['enabled']:
            ret |= TYPE_BOUNDARYNO
        if self.settings['boundaryTwo']['enabled']:
            ret |= TYPE_BOUNDARYTWO
        if self.settings['boundaryOne']['enabled']:
            ret |= TYPE_BOUNDARYONE
        if self.settings['centroidIn']['enabled']:
            ret |= TYPE_CENTROIDIN
        if self.settings['centroidOut']['enabled']:
            ret |= TYPE_CENTROIDOUT
        if self.settings['centroidDup']['enabled']:
            ret |= TYPE_CENTROIDDUP
        if self.settings['nodeOne']['enabled']:
            ret |= TYPE_NODEONE
        if self.settings['nodeTwo']['enabled']:
            ret |= TYPE_NODETWO
        if self.settings['vertex']['enabled']:
            ret |= TYPE_VERTEX
        if self.settings['area']['enabled']:
            ret |= TYPE_AREA
        if self.settings['direction']['enabled']:
            ret |= TYPE_DIRECTION
        
        return ret
        
    def _printIds(self):
        pass

    def _isSelected(self, featId, foo = False):
        return False

    def _isDuplicated(self, featId):
        return False

    def _resetTopology(self):
        pass

    def _getRegionBox(self):
        """!Get bound_box() from current region

        @return bound_box
        """
        box = bound_box()
        
        box.N = self.region['n']
        box.S = self.region['s']
        box.E = self.region['e']
        box.W = self.region['w']
        box.T = PORT_DOUBLE_MAX
        box.B = -PORT_DOUBLE_MAX
        
        return box

    def DrawMap(self, force = False):
        """!Draw content of the vector map to the device
        
        @param force force drawing
        @return number of drawn features
        @return -1 on error
        """
        Debug.msg(1, "DisplayDriver.DrawMap(): force=%d", force)
        
        if not self.mapInfo or not self.dc or not self.dcTmp:
            return -1
        
        rlist = Vedit_render_map(self.mapInfo, byref(self._getRegionBox()), self._getDrawFlag(),
                                 self.region['center_easting'], self.region['center_northing'],
                                 self.mapObj.width, self.mapObj.height,
                                 max(self.region['nsres'], self.region['ewres'])).contents
        # ResetTopology()
        
        self.dc.BeginDrawing()
        self.dcTmp.BeginDrawing()
        
        # draw objects
        for i in range(rlist.nitems):
            robj = rlist.item[i].contents
            self._drawObject(robj)
        
        self.dc.EndDrawing()
        self.dcTmp.EndDrawing()
        
        # reset list of selected features by cat 
        # list of ids - see IsSelected()
        ### selected.field = -1;
        ### Vect_reset_list(selected.cats);
        
    def SelectLinesByBox(self):
        pass

    def SelectLineByPoint(self):
        pass

    def GetSelected(self, grassId = False):
        """!Get ids of selected objects
        
        @param grassId if true return GRASS line ids, false to return PseudoDC ids
   
        @return list of ids of selected vector objects
        """
        if grassId:
            # return ListToVector(selected.ids);
            pass

        dc_ids = list()

        if not self.drawSegments:
            dc_ids.append(1)
        else:
            # only first selected feature
            # Vect_read_line(self.mapInfo, byref(self.points), None,
            # self.selected.ids->value[0]);
            npoints = self.points.n_points
            # node - segment - vertex - segment - node
            for i in range(1, 2 * self.points.npoints):
                dc_ids.append(i)
        
        return dc_ids


    def GetSelectedCoord(self):
        pass

    def GetDuplicates(self):
        pass

    def GetRegionSelected(self):
        pass

    def SetSelected(self, ids, layer = -1):
        """!Set selected vector objects
   
        @param ids list of feature ids to be set
        @param layer field number (-1 for ids instead of cats)
        """
        if ids:
            self.drawSelected = True
        else:
            self.drawSelected = False
            return

        if layer > 0:
            selected.field = layer
            # VectorToList(selected.cats, id);
        else:
            field = -1
            # VectorToList(selected.ids, id);
        
        return 1

    def UnSelect(self):
        pass

    def GetSelectedVertex(self):
        pass

    def DrawSelected(self):
        pass
    
    def CloseMap(self):
        """!Close vector map
        
        @return 0 on success
        @return non-zero on error
        """
        ret = 0
        if self.mapInfo:
            if self.mapInfoObj.mode == GV_MODE_RW:
                # rebuild topology
                Vect_build_partial(self.mapInfo, GV_BUILD_NONE)
                Vect_build(self.mapInfo)

            # close map and store topo/cidx
            ret = Vect_close(self.mapInfo)
            del self.mapInfoObj
            self.mapInfo = self.mapInfoObj = None
        
        return ret
    
    def OpenMap(self, name, mapset, update = True):
        """!Open vector map by the driver
        
        @param name name of vector map to be open
        @param mapset name of mapset where the vector map lives
   
        @return map_info
        @return None on error
        """
        Debug.msg("DisplayDriver.OpenMap(): name=%s mapset=%s updated=%d",
                  name, mapset, update)
        if not self.mapInfoObj:
            self.mapInfoObj = Map_info()
            self.mapInfo = pointer(self.mapInfoObj)
        
        # define open level (level 2: topology)
        Vect_set_open_level(2)
        
        # avoid GUI crash when G_fatal_error() is called (opening the vector map)
        Vect_set_fatal_error(GV_FATAL_PRINT)
        
        # open existing map
        if update:
            ret = Vect_open_update(self.mapInfo, name, mapset)
        else:
            ret = Vect_open_old(self.mapInfo, name, mapset)

        if ret == -1: # error
            del self.mapInfoObj
            self.mapInfo = self.mapInfoObj = None
        
        return self.mapInfo
    
    def ReloadMap(self):
        pass

    def SetDevice(self):
        pass

    def GetMapBoundingBox(self):
        """!Get bounding box of (opened) vector map layer

        @return (w,s,b,e,n,t)
        """
        if not self.mapInfo:
            return None
        
        bbox = bound_box()
        Vect_get_map_box(self.mapInfo, byref(bbox))

        return bbox.W, bbox.S, bbox.B, \
            bbox.E, bbox.N, bbox.T
    
    def Is3D(self):
        pass

    def SetRegion(self):
        pass
    
    def UpdateSettings(self, alpha = 255):
        """!Update display driver settings

        @todo map units
        
        @alpha color value for aplha channel
        """
        color = dict()
        for key in self.settings.keys():
            if key == 'lineWidth':
                self.settings[key] = int(UserSettings.Get(group = 'vdigit', key = 'lineWidth',
                                                          subkey = 'value'))
                continue
            
            color = wx.Color(UserSettings.Get(group = 'vdigit', key = 'symbol',
                                              subkey = [key, 'color'])[0],
                             UserSettings.Get(group = 'vdigit', key = 'symbol',
                                              subkey = [key, 'color'])[1],
                             UserSettings.Get(group = 'vdigit', key = 'symbol',
                                              subkey = [key, 'color'])[2])
            
            if key == 'highlight':
                self.settings[key] = color
                continue
            
            if key == 'highlightDupl':
                self.settings[key]['enabled'] = bool(UserSettings.Get(group = 'vdigit', key = 'checkForDupl',
                                                                      subkey = 'enabled'))
            else:
                self.settings[key]['enabled'] = bool(UserSettings.Get(group = 'vdigit', key = 'symbol',
                                                                      subkey = [key, 'enabled']))
            
            self.settings[key]['color'] = color
        
    def UpdateRegion(self):
        """!Update geographical region used by display driver.
        """
        self.region = self.mapObj.GetCurrentRegion()
        
    def GetThreshold(self, type = 'snapping', value = None, units = None):
        """!Return threshold value in map units
        
        @param type snapping mode (node, vertex)
        @param value threshold to be set up
        @param units units (map, screen)

        @return (snap mode id, threshold value)
        """
        if value is None:
            value = UserSettings.Get(group = 'vdigit', key = type, subkey =' value')
        
        if units is None:
            units = UserSettings.Get(group = 'vdigit', key = type, subkey = 'units')
        
        if value < 0:
            value = (self.region['nsres'] + self.region['ewres']) / 2.0
        
        if units == "screen pixels":
            # pixel -> cell
            res = max(self.region['nsres'], self.region['ewres'])
            threshold = value * res
        else:
            threshold = value
        
        if threshold > 0.0:
            if UserSettings.Get(group = 'vdigit', key = 'snapToVertex', subkey = 'enabled'):
                snap = SNAPVERTEX
            else:
                snap = SNAP
        else:
            snap = NO_SNAP

        return (snap, threshold)
