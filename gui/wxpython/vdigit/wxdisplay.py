"""!
@package vdigit.wxdisplay

@brief wxGUI vector digitizer (display driver)

Code based on wxVdigit C++ component from GRASS 6.4.0
(gui/wxpython/vdigit). Converted to Python in 2010/12-2011/01.

List of classes:
 - wxdisplay::DisplayDriver

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import locale

import wx

from core.debug    import Debug
from core.settings import UserSettings

try:
    from grass.lib.gis    import *
    from grass.lib.vector import *
    from grass.lib.vedit  import *
except ImportError:
    pass

log       = None
progress  = None
last_error = ''

def print_error(msg, type):
    """!Redirect stderr"""
    global log
    if log:
        log.write(msg + os.linesep)
    else:
        print msg
    global last_error
    last_error += ' ' + msg

    return 0

def print_progress(value):
    """!Redirect progress info"""
    global progress
    if progress:
        progress.SetValue(value)
    else:
        pass # discard progress info
    
    return 0

def GetLastError():
    global last_error
    ret = last_error
    if ret[-1] != '.':
        ret += '.'
    
    last_error = '' # reset
    
    return ret

try:
    errtype = CFUNCTYPE(UNCHECKED(c_int), String, c_int)
    errfunc = errtype(print_error)
    pertype = CFUNCTYPE(UNCHECKED(c_int), c_int)
    perfunc = pertype(print_progress)
except NameError:
    pass
    
class DisplayDriver:
    def __init__(self, device, deviceTmp, mapObj, window, glog, gprogress):
        """!Display driver used by vector digitizer
        
        @param device    wx.PseudoDC device where to draw vector objects
        @param deviceTmp wx.PseudoDC device where to draw temporary vector objects
        @param mapOng    Map Object (render.Map)
        @param windiow   parent window for dialogs
        @param glog      logging device (None to discard messages)
        @param gprogress progress bar device (None to discard message)
        """
        global errfunc, perfunc, log, progress
        log = glog
        progress = gprogress
        
        G_gisinit('wxvdigit')
        locale.setlocale(locale.LC_NUMERIC, 'C')
        G_set_error_routine(errfunc) 
        G_set_percent_routine(perfunc)
        # G_set_fatal_error(FATAL_RETURN)
        
        self.mapInfo   = None     # open vector map (Map_Info structure)
        self.poMapInfo = None     # pointer to self.mapInfo
        self.is3D      = False    # is open vector map 3D
        
        self.dc      = device     # PseudoDC devices
        self.dcTmp   = deviceTmp
        self.mapObj  = mapObj
        self.region  = mapObj.GetCurrentRegion()
        self.window  = window
        self.log     = log        # log device

        self.firstNode = True     # track PseudoDC Id of selected features
        self.lastNodeId = -1
        
        # GRASS lib
        self.poPoints = Vect_new_line_struct()
        self.poCats   = Vect_new_cats_struct()
        
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
        self._resetTopology()
        
        self._drawSelected = False
        self._drawSegments = False
        
        self.UpdateSettings()
        
    def __del__(self):
        """!Close currently open vector map"""
        G_unset_error_routine()
        G_unset_percent_routine()
        
        if self.poMapInfo:
            self.CloseMap()
        
        Vect_destroy_line_struct(self.poPoints)
        Vect_destroy_cats_struct(self.poCats)

    def _resetTopology(self):
        """!Reset topology dict
        """
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
            return -1
        
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
        
        Debug.msg(3, "_drawObject(): line=%d type=%d npoints=%d", robj.fid, robj.type, robj.npoints)
        brush = None
        if self._isSelected(robj.fid):
            pdc = self.dcTmp
            if robj.type == TYPE_AREA:
                return 1
            else:
                if self.settings['highlightDupl']['enabled'] and self._isDuplicated(robj.fid):
                    pen = wx.Pen(self.settings['highlightDupl']['color'], self.settings['lineWidth'], wx.SOLID)
                else:            
                    pen = wx.Pen(self.settings['highlight'], self.settings['lineWidth'], wx.SOLID)
                    
            dcId = 1
            self.topology['highlight'] += 1
            if not self._drawSelected:
                return
        else:
            pdc = self.dc
            pen, brush = self._definePen(robj.type)
            dcId = 0
        
        pdc.SetPen(pen)        
        if brush:
            pdc.SetBrush(brush)
        
        if robj.type & (TYPE_POINT | TYPE_CENTROIDIN | TYPE_CENTROIDOUT | TYPE_CENTROIDDUP |
                        TYPE_NODEONE | TYPE_NODETWO | TYPE_VERTEX): # -> point
            if dcId > 0:
                if robj.type == TYPE_VERTEX:
                    dcId = 3 # first vertex
                elif robj.type & (TYPE_NODEONE | TYPE_NODETWO):
                    if self.firstNode:
                        dcId = 1
                        self.firstNode = False
                    else:
                        dcId = self.lastNodeId
            
            for i in range(robj.npoints):
                p = robj.point[i]
                if dcId > 0:
                    pdc.SetId(dcId)
                    dcId += 2
                self._drawCross(pdc, p)
        else:
            if dcId > 0 and self._drawSegments:
                self.fisrtNode = True
                self.lastNodeId = robj.npoints * 2 - 1
                dcId = 2 # first segment
                i = 0
                while i < robj.npoints - 1:
                    point_beg = wx.Point(robj.point[i].x, robj.point[i].y)
                    point_end = wx.Point(robj.point[i+1].x, robj.point[i+1].y)
                    pdc.SetId(dcId) # set unique id & set bbox for each segment
                    pdc.SetPen(pen)
                    pdc.SetIdBounds(dcId - 1, wx.Rect(point_beg.x, point_beg.y, 0, 0))
                    pdc.SetIdBounds(dcId, wx.RectPP(point_beg, point_end))
                    pdc.DrawLine(point_beg.x, point_beg.y,
                                 point_end.x, point_end.y)
                    i    += 1
                    dcId += 2
                pdc.SetIdBounds(dcId - 1, wx.Rect(robj.point[robj.npoints - 1].x,
                                                  robj.point[robj.npoints - 1].y,
                                                  0, 0))
            else:
                points = list()
                for i in range(robj.npoints):
                    p = robj.point[i]
                    points.append(wx.Point(p.x, p.y))
                    
                if robj.type == TYPE_AREA:
                    pdc.DrawPolygon(points)
                else:
                    pdc.DrawLines(points)
        
    def _definePen(self, rtype):
        """!Define pen/brush based on rendered object)
        
        Updates also self.topology dict

        @return pen, brush
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
                brush = wx.Brush(self.settings[key]['color'], wx.SOLID)
            else:
                brush = wx.TRANSPARENT_BRUSH
        else:
            pen = wx.Pen(self.settings[key]['color'], self.settings['lineWidth'], wx.SOLID)
            brush = None
        
        return pen, brush
        
    def _getDrawFlag(self):
        """!Get draw flag from the settings
        
        See vedit.h for list of draw flags.
        
        @return draw flag (int)
        """
        ret = 0
        if self.settings['point']['enabled']:
            ret |= DRAW_POINT
        if self.settings['line']['enabled']:
            ret |= DRAW_LINE
        if self.settings['boundaryNo']['enabled']:
            ret |= DRAW_BOUNDARYNO
        if self.settings['boundaryTwo']['enabled']:
            ret |= DRAW_BOUNDARYTWO
        if self.settings['boundaryOne']['enabled']:
            ret |= DRAW_BOUNDARYONE
        if self.settings['centroidIn']['enabled']:
            ret |= DRAW_CENTROIDIN
        if self.settings['centroidOut']['enabled']:
            ret |= DRAW_CENTROIDOUT
        if self.settings['centroidDup']['enabled']:
            ret |= DRAW_CENTROIDDUP
        if self.settings['nodeOne']['enabled']:
            ret |= DRAW_NODEONE
        if self.settings['nodeTwo']['enabled']:
            ret |= DRAW_NODETWO
        if self.settings['vertex']['enabled']:
            ret |= DRAW_VERTEX
        if self.settings['area']['enabled']:
            ret |= DRAW_AREA
        if self.settings['direction']['enabled']:
            ret |= DRAW_DIRECTION
        
        return ret
        
    def _isSelected(self, line, force = False):
        """!Check if vector object selected?
   
        @param line feature id

        @return True if vector object is selected
        @return False if vector object is not selected
        """
        if line in self.selected['ids']:
            return True
        
        return False

    def _isDuplicated(self, line):
        """!Check for already marked duplicates
        
        @param line feature id

        @return True line already marked as duplicated
        @return False not duplicated
        """
        return line in self.selected['idsDupl']
    
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
        
        if not self.poMapInfo or not self.dc or not self.dcTmp:
            return -1
        
        rlist = Vedit_render_map(self.poMapInfo, byref(self._getRegionBox()), self._getDrawFlag(),
                                 self.region['center_easting'], self.region['center_northing'],
                                 self.mapObj.width, self.mapObj.height,
                                 max(self.region['nsres'], self.region['ewres'])).contents
        
        self._resetTopology()
        
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
        self.selected['field'] = -1
        self.selected['cats'] = list()
        
    def _getSelectType(self):
        """!Get type(s) to be selected

        Used by SelectLinesByBox() and SelectLineByPoint()
        """
        ftype = 0
        for feature in (('point',    GV_POINT),
                        ('line',     GV_LINE),
                        ('centroid', GV_CENTROID),
                        ('boundary', GV_BOUNDARY)):
            if UserSettings.Get(group = 'vdigit', key = 'selectType',
                                subkey = [feature[0], 'enabled']):
                ftype |= feature[1]
        
        return ftype

    def _validLine(self, line):
        """!Check if feature id is valid

        @param line feature id

        @return True valid feature id
        @return False invalid
        """
        if line > 0 and line <= Vect_get_num_lines(self.poMapInfo):
            return True
        
        return False
    
    def SelectLinesByBox(self, bbox, drawSeg = False, poMapInfo = None):
        """!Select vector objects by given bounding box
        
        If line id is already in the list of selected lines, then it will
        be excluded from this list.
        
        @param bbox bounding box definition
        @param drawSeg True to draw segments of line
        @param poMapInfo use external Map_info, None for self.poMapInfo

        @return number of selected features
        @return None on error
        """
        thisMapInfo = poMapInfo is None
        if not poMapInfo:
            poMapInfo = self.poMapInfo
        
        if not poMapInfo:
            return None
        
        if thisMapInfo:
            self._drawSegments = drawSeg
            self._drawSelected = True
        
            # select by ids
            self.selected['cats'] = list()
        
        poList = Vect_new_list()
        x1, y1 = bbox[0]
        x2, y2 = bbox[1]
        poBbox = Vect_new_line_struct()
        Vect_append_point(poBbox, x1, y1, 0.0)
        Vect_append_point(poBbox, x2, y1, 0.0)
        Vect_append_point(poBbox, x2, y2, 0.0)
        Vect_append_point(poBbox, x1, y2, 0.0)
        Vect_append_point(poBbox, x1, y1, 0.0)
        
        Vect_select_lines_by_polygon(poMapInfo, poBbox,
                                     0, None, # isles
                                     self._getSelectType(), poList)
        
        flist = poList.contents
        nlines = flist.n_values
        Debug.msg(1, "DisplayDriver.SelectLinesByBox() num = %d", nlines)
        for i in range(nlines):
            line = flist.value[i]
            if UserSettings.Get(group = 'vdigit', key = 'selectInside',
                                subkey = 'enabled'):
                inside = True
                if not self._validLine(line):
                    return None
                Vect_read_line(poMapInfo, self.poPoints, None, line)
                points = self.poPoints.contents
                for p in range(points.n_points):
                    if not Vect_point_in_poly(points.x[p], points.y[p],
                                              poBbox):
                        inside = False
                        break
                    
                if not inside:
                    continue # skip lines just overlapping bbox
            
            if not self._isSelected(line):
                self.selected['ids'].append(line)
            else:
                self.selected['ids'].remove(line)
        
        Vect_destroy_line_struct(poBbox)
        Vect_destroy_list(poList)
        
        return nlines

    def SelectLineByPoint(self, point, poMapInfo = None):
        """!Select vector feature by given point in given
        threshold
   
        Only one vector object can be selected. Bounding boxes of
        all segments are stores.
        
        @param point points coordinates (x, y)
        @param poMapInfo use external Map_info, None for self.poMapInfo

        @return dict {'line' : feature id, 'point' : point on line}
        """
        thisMapInfo = poMapInfo is None
        if not poMapInfo:
            poMapInfo = self.poMapInfo
        
        if not poMapInfo:
            return { 'line' : -1, 'point': None }
        
        if thisMapInfo:
            self._drawSelected = True
            # select by ids 
            self.selected['cats'] = list()
        
        poFound = Vect_new_list()
        
        lineNearest = Vect_find_line_list(poMapInfo, point[0], point[1], 0,
                                           self._getSelectType(), self.GetThreshold(), self.is3D,
                                           None, poFound)
        Debug.msg(1, "DisplayDriver.SelectLineByPoint() found = %d", lineNearest)
        
        if lineNearest > 0:
            if not self._isSelected(lineNearest):
                self.selected['ids'].append(lineNearest)
            else:
                self.selected['ids'].remove(lineNearest)
        
        px = c_double()
        py = c_double()
        pz = c_double()
        if not self._validLine(lineNearest):
            return { 'line' : -1, 'point': None }
        ftype = Vect_read_line(poMapInfo, self.poPoints, self.poCats, lineNearest)
        Vect_line_distance(self.poPoints, point[0], point[1], 0.0, self.is3D,
                           byref(px), byref(py), byref(pz),
                           None, None, None)

        # check for duplicates
        if self.settings['highlightDupl']['enabled']:
            found = poFound.contents
            for i in range(found.n_values):
                line = found.value[i]
                if line != lineNearest:
                    self.selected['ids'].append(line)

            self.GetDuplicates()

            for i in range(found.n_values):
                line = found.value[i]
                if line != lineNearest and not self._isDuplicated(line):
                    self.selected['ids'].remove(line)
        
        Vect_destroy_list(poFound)
        
        if thisMapInfo:
            # drawing segments can be very expensive
            # only one features selected
            self._drawSegments = True
        
        return { 'line'  : lineNearest,
                 'point' : (px.value, py.value, pz.value) }
    
    def _listToIList(self, plist):
        """!Generate from list struct_ilist
        """
        ilist = Vect_new_list()
        for val in plist:
            Vect_list_append(ilist, val)
        
        return ilist
        
    def GetSelectedIList(self, ilist = None):
        """!Get list of selected objects as struct_ilist

        Returned IList must be freed by Vect_destroy_list().
        
        @return struct_ilist
        """
        if ilist:
            return self._listToIList(ilist)
        
        return self._listToIList(self.selected['ids'])
        
    def GetSelected(self, grassId = True):
        """!Get ids of selected objects
        
        @param grassId True for feature id, False for PseudoDC id
        
        @return list of ids of selected vector objects
        """
        if grassId:
            return self.selected['ids']
        
        dc_ids = list()
        
        if not self._drawSegments:
            dc_ids.append(1)
        elif len(self.selected['ids']) > 0:
            # only first selected feature
            Vect_read_line(self.poMapInfo, self.poPoints, None,
                           self.selected['ids'][0])
            points = self.poPoints.contents
            # node - segment - vertex - segment - node
            for i in range(1, 2 * points.n_points):
                dc_ids.append(i)
        
        return dc_ids
        
    def SetSelected(self, ids, layer = -1):
        """!Set selected vector objects

        @param list of ids (None to unselect features)
        @param layer layer number for features selected based on category number
        """
        if ids:
            self._drawSelected = True
        else:
            self._drawSelected = False

        self.selected['field'] = layer        
        if layer > 0:
            self.selected['cats']  = ids
            self.selected['ids']   = list()
            ### cidx is not up-to-date
            # Vect_cidx_find_all(self.poMapInfo, layer, GV_POINTS | GV_LINES, lid, ilist)
            nlines = Vect_get_num_lines(self.poMapInfo)
            for line in range(1, nlines + 1):
                if not Vect_line_alive(self.poMapInfo, line):
                    continue
                
                ltype = Vect_read_line (self.poMapInfo, None, self.poCats, line)
                if not (ltype & (GV_POINTS | GV_LINES)):
                    continue
                
                found = False
                cats = self.poCats.contents
                for i in range(0, cats.n_cats):
                    for cat in self.selected['cats']:
                        if cats.cat[i] == cat:
                            found = True
                            break
                if found:
                    self.selected['ids'].append(line)
        else:
            self.selected['ids']   = ids
            self.selected['cats']  = []
        
    def GetSelectedVertex(self, pos):
        """!Get PseudoDC vertex id of selected line

        Set bounding box for vertices of line.
        
        @param pos position
        
        @return id of center, left and right vertex
        @return 0 no line found
        @return -1 on error
        """
        returnId = list()
        # only one object can be selected
        if len(self.selected['ids']) != 1 or not self._drawSegments:
            return returnId
        
        startId = 1
        line = self.selected['ids'][0]
        
        if not self._validLine(line):
            return -1
        ftype = Vect_read_line(self.poMapInfo, self.poPoints, self.poCats, line)
        
        minDist = 0.0
        Gid = -1
        # find the closest vertex (x, y)
        DCid = 1
        points = self.poPoints.contents
        for idx in range(points.n_points):
            dist = Vect_points_distance(pos[0], pos[1], 0.0,
                                        points.x[idx], points.y[idx], points.z[idx], 0)
            
            if idx == 0:
                minDist = dist
                Gid     = idx
            else:
                if minDist > dist:
                    minDist = dist
                    Gid = idx
            
            vx, vy = self._cell2Pixel(points.x[idx], points.y[idx], points.z[idx])
            rect = wx.Rect(vx, vy, 0, 0)
            self.dc.SetIdBounds(DCid, rect)
            DCid += 2
        
        if minDist > self.GetThreshold():
            return returnId
        
        # translate id
        DCid = Gid * 2 + 1
        
        # add selected vertex
        returnId.append(DCid)
        # left vertex
        if DCid == startId:
            returnId.append(-1)
        else:
            returnId.append(DCid - 2)
        # right vertex
        if DCid == (points.n_points - 1) * 2 + startId:
            returnId.append(-1)
        else:
            returnId.append(DCid + 2)
        
        return returnId

    def GetRegionSelected(self):
        """!Get minimal region extent of selected features

        @return n,s,w,e
        """
        regionBox = bound_box()
        lineBox = bound_box()
        setRegion = True
        
        nareas = Vect_get_num_areas(self.poMapInfo)
        for line in self.selected['ids']:
            area = Vect_get_centroid_area(self.poMapInfo, line)
            
            if area > 0 and area <= nareas:
                if not Vect_get_area_box(self.poMapInfo, area, byref(lineBox)):
                    continue
            else:
                if not Vect_get_line_box(self.poMapInfo, line, byref(lineBox)):
                    continue
                
            if setRegion:
                Vect_box_copy(byref(regionBox), byref(lineBox))
                setRegion = False
            else:
                Vect_box_extend(byref(regionBox), byref(lineBox))
        
        return regionBox.N, regionBox.S, regionBox.W, regionBox.E

    def DrawSelected(self, flag):
        """!Draw selected features
        
        @param flag True to draw selected features
        """
        self._drawSelected = bool(flag)
        
    def CloseMap(self):
        """!Close vector map
        
        @return 0 on success
        @return non-zero on error
        """
        ret = 0
        if self.poMapInfo:
            # rebuild topology
            Vect_build_partial(self.poMapInfo, GV_BUILD_NONE)
            Vect_build(self.poMapInfo)

            # close map and store topo/cidx
            ret = Vect_close(self.poMapInfo)
            del self.mapInfo
            self.poMapInfo = self.mapInfo = None
        
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
        if not self.mapInfo:
            self.mapInfo = Map_info()
            self.poMapInfo = pointer(self.mapInfo)
        
        # open existing map
        if update:
            ret = Vect_open_update(self.poMapInfo, name, mapset)
            Vect_set_updated(self.poMapInfo, True) # track updated lines
        else:
            ret = Vect_open_old(self.poMapInfo, name, mapset)
        self.is3D = Vect_is_3d(self.poMapInfo)
        
        if ret == -1: # error
            del self.mapInfo
            self.poMapInfo = self.mapInfo = None
        elif ret < 2:
            dlg = wx.MessageDialog(parent = self.window,
                                   message = _("Topology for vector map <%s> is not available. "
                                               "Topology is required by digitizer. Do you want to "
                                               "rebuild topology (takes some time) and open the vector map "
                                               "for editing?") % name,
                                   caption=_("Topology missing"),
                                   style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)
            ret = dlg.ShowModal()
            if ret != wx.ID_YES:
                del self.mapInfo
                self.poMapInfo = self.mapInfo = None
            else:
                Vect_build(self.poMapInfo)
        
        return self.poMapInfo
    
    def GetMapBoundingBox(self):
        """!Get bounding box of (opened) vector map layer

        @return (w,s,b,e,n,t)
        """
        if not self.poMapInfo:
            return None
        
        bbox = bound_box()
        Vect_get_map_box(self.poMapInfo, byref(bbox))

        return bbox.W, bbox.S, bbox.B, \
            bbox.E, bbox.N, bbox.T
    
    def UpdateSettings(self, alpha = 255):
        """!Update display driver settings

        @todo map units
        
        @param alpha color value for aplha channel
        """
        color = dict()
        for key in self.settings.keys():
            if key == 'lineWidth':
                self.settings[key] = int(UserSettings.Get(group = 'vdigit', key = 'lineWidth',
                                                          subkey = 'value'))
                continue
            
            color = wx.Colour(UserSettings.Get(group = 'vdigit', key = 'symbol',
                                              subkey = [key, 'color'])[0],
                             UserSettings.Get(group = 'vdigit', key = 'symbol',
                                              subkey = [key, 'color'])[1],
                             UserSettings.Get(group = 'vdigit', key = 'symbol',
                                              subkey = [key, 'color'])[2],
                             alpha)
            
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
        """!Update geographical region used by display driver
        """
        self.region = self.mapObj.GetCurrentRegion()
        
    def GetThreshold(self, type = 'snapping', value = None, units = None):
        """!Return threshold value in map units
        
        @param type snapping mode (node, vertex)
        @param value threshold to be set up
        @param units units (map, screen)

        @return threshold value
        """
        if value is None:
            value = UserSettings.Get(group = 'vdigit', key = type, subkey = 'value')
        
        if units is None:
            units = UserSettings.Get(group = 'vdigit', key = type, subkey = 'units')
        
        if value < 0:
            value = (self.region['nsres'] + self.region['ewres']) / 2.0
        
        if units == "screen pixels":
            # pixel -> cell
            res = max(self.region['nsres'], self.region['ewres'])
            return value * res
        
        return value
    
    def GetDuplicates(self):
        """!Return ids of (selected) duplicated vector features
        """
        if not self.poMapInfo:
            return
        
        ids = dict()
        APoints = Vect_new_line_struct()
        BPoints = Vect_new_line_struct()
        
        self.selected['idsDupl'] = list()
        
        for i in range(len(self.selected['ids'])):
            line1 = self.selected['ids'][i]
            if self._isDuplicated(line1):
                continue
            
            Vect_read_line(self.poMapInfo, APoints, None, line1)
            
            for line2 in self.selected['ids']:
                if line1 == line2 or self._isDuplicated(line2):
                    continue
                
                Vect_read_line(self.poMapInfo, BPoints, None, line2)

                if Vect_line_check_duplicate(APoints, BPoints, WITHOUT_Z):
                    if i not in ids:
                        ids[i] = list()
                        ids[i].append((line1, self._getCatString(line1)))
                        self.selected['idsDupl'].append(line1)
                    
                    ids[i].append((line2, self._getCatString(line2)))
                    self.selected['idsDupl'].append(line2)
        
        Vect_destroy_line_struct(APoints)
        Vect_destroy_line_struct(BPoints)

        return ids
    
    def _getCatString(self, line):
        Vect_read_line(self.poMapInfo, None, self.poCats, line)
        
        cats = self.poCats.contents
        catsDict = dict()
        for i in range(cats.n_cats):
            layer = cats.field[i]
            if layer not in catsDict:
                catsDict[layer] = list()
            catsDict[layer].append(cats.cat[i])
        
        catsStr = ''
        for l, c in catsDict.iteritems():
            catsStr = '%d: (%s)' % (l, ','.join(map(str, c)))
        
        return catsStr

    def UnSelect(self, lines):
        """!Unselect vector features

        @param lines list of feature id(s)
        """
        checkForDupl = False

        for line in lines:
            if self._isSelected(line):
                self.selected['ids'].remove(line)
            if self.settings['highlightDupl']['enabled'] and self._isDuplicated(line):
                checkForDupl = True

        if checkForDupl:
            self.GetDuplicates()
        
        return len(self.selected['ids'])
    

