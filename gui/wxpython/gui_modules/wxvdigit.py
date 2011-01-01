"""!
@package wxvdigit.py

@brief wxGUI vector digitizer (base class)

Code based on wxVdigit C++ component from GRASS 6.4.0. Converted to
Python in 2010/12-2011/01.

List of classes:
 - IVDigit

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

from debug       import Debug
from preferences import globalSettings as UserSettings

from wxvdriver import DisplayDriver

from grass.lib.grass  import *
from grass.lib.vector import *

class IVDigit:
    def __init__(self, mapwindow):
        self.map       = None
        self.mapWindow = mapwindow
        
        if not mapwindow.parent.IsStandalone():
            self.log = mapwindow.parent.GetLayerManager().goutput.cmd_stderr
        else:
            self.log = sys.stderr
        
        self.toolbar = mapwindow.parent.toolbars['vdigit']
        
        self._display = DisplayDriver(device    = mapwindow.pdcVector,
                                      deviceTmp = mapwindow.pdcTmp,
                                      mapObj    = mapwindow.Map,
                                      log       = self.log)
        
        # self.SetCategory()
        
        # layer / max category
        self.cats = dict()
        # settings
        self._settings = {
            'breakLines'  : None,
            'addCentroid' : None,
            'catBoundary' : None
            }
        # undo/redo
        self.changesets = dict()
        self.changesetCurrent = None # first changeset to apply
        self.changesetEnd     = None # last changeset to be applied
        
        if self._display.mapInfo:
            self.InitCats()
        
        # initial value for undo/redo
        self.changesetEnd = self.changesetCurrent = -1
        
    def __del__(self):
        pass # free changesets ?

    def _setCategory(self):
        pass
    
    def _openBackgroundVectorMap(self):
        pass

    def _breakLineAtIntersection(self):
        pass
    
    def _addActionsBefore(self):
        """!Register action before operation
  
        @return changeset id
        """
        pass
    
    def _addActionsAfter(self):
        pass

    def _addActionToChangeset(self):
        pass

    def _applyChangeset(self):
        pass

    def _freeChangeset(self):
        pass

    def _removeActionFromChangeset(self):
        pass

    def AddPoint (self, map, point, x, y, z=None):
        """!Add new point/centroid

        @param map   map name (unused, for compatability with VEdit)
        @param point feature type (if true point otherwise centroid)
        @param x,y,z coordinates
        """
        if UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection') == 2:
            layer = -1 # -> no category
            cat   = -1
        else:
            layer = UserSettings.Get(group='vdigit', key="layer", subkey='value')
            cat   = self.SetCategory()
            
        if point:
            type = wxvdigit.GV_POINT 
        else:
            type = wxvdigit.GV_CENTROID 

        snap, thresh = self.__getSnapThreshold()

        bgmap = str(UserSettings.Get(group='vdigit', key="bgmap",
                                     subkey='value', internal=True))
        if z:
            ret = self.digit.AddLine(type, [x, y, z], layer, cat,
                                     bgmap, snap, thresh)
        else:
            ret = self.digit.AddLine(type, [x, y], layer, cat,
                                     bgmap, snap, thresh)
        self.toolbar.EnableUndo()

        return ret
        
    def AddLine (self, map, line, coords):
        """!Add line/boundary

        @param map    map name (unused, for compatability with VEdit)
        @param line   feature type (if True line, otherwise boundary)
        @param coords list of coordinates
        """
        if len(coords) < 2:
            return
        
        if UserSettings.Get(group='vdigit', key="categoryMode", subkey='selection') == 2:
            layer = -1 # -> no category
            cat   = -1
        else:
            layer = UserSettings.Get(group='vdigit', key="layer", subkey='value')
            cat   = self.SetCategory()
        
        if line:
            type = wxvdigit.GV_LINE
        else:
            type = wxvdigit.GV_BOUNDARY
        
        listCoords = []
        for c in coords:
            for x in c:
                listCoords.append(x)
        
        snap, thresh = self.__getSnapThreshold()
        
        bgmap = str(UserSettings.Get(group='vdigit', key="bgmap",
                                     subkey='value', internal=True))
        
        ret = self.digit.AddLine(type, listCoords, layer, cat,
                                 bgmap, snap, thresh)

        self.toolbar.EnableUndo()
        
        return ret
    
    def DeleteSelectedLines(self):
        """!Delete selected features

        @return number of deleted lines
        """
        nlines = self.digit.DeleteLines(UserSettings.Get(group='vdigit', key='delRecord', subkey='enabled'))
        
        if nlines > 0:
            self.toolbar.EnableUndo()
            
        return nlines

    def MoveSelectedLines(self, move):
        """!Move selected features

        @param move direction (x, y)
        """
        snap, thresh = self.__getSnapThreshold()
        
        bgmap = str(UserSettings.Get(group='vdigit', key="bgmap",
                                     subkey='value', internal=True))
        
        try:
            nlines = self.digit.MoveLines(move[0], move[1], 0.0, # TODO 3D
                                          bgmap, snap, thresh)
        except SystemExit:
            pass
        
        if nlines > 0:
            self.toolbar.EnableUndo()
        
        return nlines

    def MoveSelectedVertex(self, coords, move):
        """!Move selected vertex of the line

        @param coords click coordinates
        @param move   X,Y direction

        @return id of new feature
        @return 0 vertex not moved (not found, line is not selected)
        """
        snap, thresh = self.__getSnapThreshold()

        bgmap = str(UserSettings.Get(group='vdigit', key="bgmap",
                                     subkey='value', internal=True))
        
        moved = self.digit.MoveVertex(coords[0], coords[1], 0.0, # TODO 3D
                                      move[0], move[1], 0.0,
                                      bgmap, snap,
                                      self.driver.GetThreshold(type='selectThresh'), thresh)

        if moved:
            self.toolbar.EnableUndo()

        return moved

    def AddVertex(self, coords):
        """!Add new vertex to the selected line/boundary on position 'coords'

        @param coords coordinates to add vertex

        @return id of new feature
        @return 0 nothing changed
        @return -1 on failure
        """
        added = self.digit.ModifyLineVertex(1, coords[0], coords[1], 0.0, # TODO 3D
                                            self.driver.GetThreshold(type='selectThresh'))

        if added > 0:
            self.toolbar.EnableUndo()

        return added

    def RemoveVertex(self, coords):
        """!Remove vertex from the selected line/boundary on position 'coords'

        @param coords coordinates to remove vertex

        @return id of new feature
        @return 0 nothing changed
        @return -1 on failure
        """
        deleted = self.digit.ModifyLineVertex(0, coords[0], coords[1], 0.0, # TODO 3D
                                              self.driver.GetThreshold(type='selectThresh'))

        if deleted > 0:
            self.toolbar.EnableUndo()

        return deleted


    def SplitLine(self, coords):
        """!Split selected line/boundary on position 'coords'

        @param coords coordinates to split line

        @return 1 line modified
        @return 0 nothing changed
        @return -1 error
        """
        ret = self.digit.SplitLine(coords[0], coords[1], 0.0, # TODO 3D
                                   self.driver.GetThreshold('selectThresh'))

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def EditLine(self, line, coords):
        """!Edit existing line/boundary

        @param line id of line to be modified
        @param coords list of coordinates of modified line

        @return feature id of new line
        @return -1 on error
        """
        try:
            lineid = line[0]
        except:
            lineid = -1

        if len(coords) < 2:
            self.DeleteSelectedLines()
            return 0
            
        listCoords = []
        for c in coords:
            for x in c:
                listCoords.append(x)

        snap, thresh = self.__getSnapThreshold()
        
        bgmap = str(UserSettings.Get(group='vdigit', key="bgmap",
                                     subkey='value', internal=True))
        
        try:
            ret = self.digit.RewriteLine(lineid, listCoords,
                                         bgmap, snap, thresh)
        except SystemExit:
            pass

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def FlipLine(self):
        """!Flip selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        ret = self.digit.FlipLines()

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def MergeLine(self):
        """!Merge selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        ret = self.digit.MergeLines()

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def BreakLine(self):
        """!Break selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        ret = self.digit.BreakLines()

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def SnapLine(self):
        """!Snap selected lines/boundaries

        @return on success
        @return -1 on error
        """
        snap, thresh = self.__getSnapThreshold()
        ret = self.digit.SnapLines(thresh)
        
        if ret == 0:
            self.toolbar.EnableUndo()

        return ret

    def ConnectLine(self):
        """!Connect selected lines/boundaries

        @return 1 lines connected
        @return 0 lines not connected
        @return -1 on error
        """
        snap, thresh = self.__getSnapThreshold()
        ret = self.digit.ConnectLines(thresh)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret
        
    def CopyLine(self, ids=[]):
        """!Copy features from (background) vector map

        @param ids list of line ids to be copied

        @return number of copied features
        @return -1 on error
        """
        bgmap = str(UserSettings.Get(group='vdigit', key='bgmap',
                                     subkey='value', internal=True))
        
        if len(bgmap) > 0:
            ret = self.digit.CopyLines(ids, bgmap)
        else:
            ret = self.digit.CopyLines(ids, None)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def CopyCats(self, fromId, toId, copyAttrb=False):
        """!Copy given categories to objects with id listed in ids

        @param cats ids of 'from' feature
        @param ids  ids of 'to' feature(s)

        @return number of modified features
        @return -1 on error
        """
        if len(fromId) == 0 or len(toId) == 0:
            return 0
        
        ret = self.digit.CopyCats(fromId, toId, copyAttrb)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def SelectLinesByQuery(self, pos1, pos2):
        """!Select features by query

        @param pos1, pos2 bounding box definition
        """
        thresh = self.SelectLinesByQueryThresh()
        
        w, n = pos1
        e, s = pos2

        query = wxvdigit.QUERY_UNKNOWN
        if UserSettings.Get(group='vdigit', key='query', subkey='selection') == 0:
            query = wxvdigit.QUERY_LENGTH
        else:
            query = wxvdigit.QUERY_DANGLE

        type = wxvdigit.GV_POINTS | wxvdigit.GV_LINES # TODO: 3D
        
        ids = self.digit.SelectLinesByQuery(w, n, 0.0, e, s, 1000.0,
                                            UserSettings.Get(group='vdigit', key='query', subkey='box'),
                                            query, type, thresh)

        Debug.msg(4, "VDigit.SelectLinesByQuery(): %s" % \
                      ",".join(["%d" % v for v in ids]))
        
        return ids

    def GetLineCats(self, line=-1):
        """!Get layer/category pairs from given (selected) line
        
        @param line feature id (-1 for first selected line)
        """
        return dict(self.digit.GetLineCats(line))

    def GetLineLength(self, line):
        """!Get line length

        @param line feature id

        @return line length
        @return -1 on error
        """
        return self.digit.GetLineLength(line)

    def GetAreaSize(self, centroid):
        """!Get area size

        @param centroid centroid id

        @return area size
        @return -1 on error
        """
        return self.digit.GetAreaSize(centroid)
        
    def GetAreaPerimeter(self, centroid):
        """!Get area perimeter

        @param centroid centroid id

        @return area size
        @return -1 on error
        """
        return self.digit.GetAreaPerimeter(centroid)

    def SetLineCats(self, line, layer, cats, add=True):
        """!Set categories for given line and layer

        @param line feature id
        @param layer layer number (-1 for first selected line)
        @param cats list of categories
        @param add if True to add, otherwise do delete categories

        @return new feature id (feature need to be rewritten)
        @return -1 on error
        """
        ret = self.digit.SetLineCats(line, layer, cats, add)

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def GetLayers(self):
        """!Get list of layers"""
        return self.digit.GetLayers()

    def TypeConvForSelectedLines(self):
        """!Feature type conversion for selected objects.

        Supported conversions:
         - point <-> centroid
         - line <-> boundary

        @return number of modified features
        @return -1 on error
        """
        ret = self.digit.TypeConvLines()

        if ret > 0:
            self.toolbar.EnableUndo()

        return ret

    def Undo(self, level=-1):
        """!Undo action

        @param level levels to undo (0 to revert all)

        @return id of current changeset
        """
        try:
            ret = self.digit.Undo(level)
        except SystemExit:
            ret = -2

        if ret == -2:
            raise gcmd.GException(_("Undo failed, data corrupted."))

        self.mapWindow.UpdateMap(render=False)
        
        if ret < 0: # disable undo tool
            self.toolbar.EnableUndo(False)

    def ZBulkLines(self, pos1, pos2, start, step):
        """!Z-bulk labeling

        @param pos1 reference line (start point)
        @param pos1 reference line (end point)
        @param start starting value
        @param step step value

        @return number of modified lines
        @return -1 on error
        """
        ret = self.digit.ZBulkLabeling(pos1[0], pos1[1], pos2[0], pos2[1],
                                       start, step)
        
        if ret > 0:
            self.toolbar.EnableUndo()
        
        return ret
    
    def __getSnapThreshold(self):
        """!Get snap mode and threshold value

        @return (snap, thresh)
        """
        thresh = self.driver.GetThreshold()

        if thresh > 0.0:
            if UserSettings.Get(group='vdigit', key='snapToVertex', subkey='enabled') is True:
                snap = wxvdigit.SNAPVERTEX
            else:
                snap = wxvdigit.SNAP
        else:
            snap = wxvdigit.NO_SNAP

        return (snap, thresh)

    def GetDisplay(self):
        """!Get display driver instance"""
        return self._display
    
    def OpenMap(self, name):
        """!Open vector map for editing
        
        @param map name of vector map to be set up
        """
        Debug.msg (3, "AbstractDigit.SetMapName map=%s" % name)
        self.map = name
        
        name, mapset = name.split('@')
        try:
            ret = self._display.OpenMap(str(name), str(mapset), True)
        except SystemExit:
                ret = -1
        
        # except StandardError, e:
        #     raise gcmd.GException(_("Unable to initialize display driver of vector "
        #                             "digitizer. See 'Command output' for details.\n\n"
        #                             "Details: ") + repr(e))
        
        # if map and ret == -1:
        #     raise gcmd.GException(_('Unable to open vector map <%s> for editing.\n\n'
        #                             'Data are probably corrupted, '
        #                             'try to run v.build to rebuild '
        #                             'the topology (Vector->Develop vector map->'
        #                             'Create/rebuild topology).') % map)
        # if not map and ret != 0:
        #     raise gcmd.GException(_('Unable to open vector map <%s> for editing.\n\n'
        #                             'Data are probably corrupted, '
        #                             'try to run v.build to rebuild '
        #                             'the topology (Vector->Develop vector map->'
        #                             'Create/rebuild topology).') % map)
        
        self.InitCats()

    def CloseMap(self):
        """!Close currently open vector map
        """
        if not self.map:
            return
        
        self._display.CloseMap()

    def InitCats(self):
        """!Initialize categories information
        
        @return 0 on success
        @return -1 on error
        """
        self.cats.clear()
        mapInfo = self._display.mapInfo
        if not mapInfo:
            return -1
        
        ndblinks = Vect_get_num_dblinks(byref(mapInfo))
        for i in range(ndblinks):
            fi = Vect_get_dblink(byref(mapInfo), i).contents
            if fi:
                self.cats[fi.number] = None
        
        # find max category
        nfields = Vect_cidx_get_num_fields(byref(mapInfo))
        Debug.msg(2, "wxDigit.InitCats(): nfields=%d", nfields)
        
        for i in range(nfields):
            field = Vect_cidx_get_field_number(byref(mapInfo), i)
            ncats = Vect_cidx_get_num_cats_by_index(byref(mapInfo), i)
            if field <= 0:
                continue
            for j in range(ncats):
                cat = c_int()
                type = c_int()
                id = c_int()
                Vect_cidx_get_cat_by_index(byref(mapInfo), i, j,
                                           byref(cat), byref(type), byref(id))
                if self.cats.has_key(field):
                    if cat > self.cats[field]:
                        self.cats[field] = cat.value
                else:
                    self.cats[field] = cat.value
            Debug.msg(3, "wxDigit.InitCats(): layer=%d, cat=%d", field, self.cats[field])
            
        # set default values
        for field, cat in self.cats.iteritems():
            if cat == None:
                self.cats[field] = 0 # first category 1
	    Debug.msg(3, "wxDigit.InitCats(): layer=%d, cat=%d", field, self.cats[field])
        
    def AddLine(self):
        pass

    def RewriteLine(self):
        pass
    
    def SplitLine(self):
        pass

    def DeleteLines(self):
        pass

    def MoveLines(self):
        pass

    def FlipLines(self):
        pass

    def MergeLines(self):
        pass

    def BreakLines(self):
        pass

    def SnapLines(self):
        pass

    def ConnectLines(self):
        pass

    def TypeConvLines(self):
        pass

    def ZBulkLabeling(self):
        pass

    def CopyLines(self):
        pass

    def MoveVertex(self):
        pass

    def ModifyLineVertex(self):
        pass

    def SelectLinesByQuery(self):
        pass

    def GetLineLength(self):
        pass

    def GetAreaSize(self):
        pass

    def GetAreaPerimeter(self):
        pass

    def CopyCats(self):
        pass

    def GetCategory(self, layer):
        """!Get max category number for layer
        
        @param layer layer number
        
        @return category number (0 if no category found)
        @return -1 on error
        """
        if cats.find(layer) != cats.end():
            Debug.msg(3, "vdigit.GetCategory(): layer=%d, cat=%d", layer, cats[layer])
            return cats[layer]
        
        return 0
    
    def GetLineCats(self):
        pass

    def SetLineCats(self):
        pass
    
    def GetLayers(self):
        pass
    
    def Undo(self):
        pass

    def GetUndoLevel(self):
        pass

    def UpdateSettings(self, breakLines, addCentroid, catBoundary):
        """!Update digit settings
        
        @param breakLines break lines on intersection
        @param addCentroid add centroid to left/right area
        @param catBoundary attach category to boundary
        """
        self._settings['breakLines']  = breakLines
        self._settings['addCentroid'] = addCentroid
        self._settings['catBoundary'] = None # !catBoundary # do not attach

    def SetCategory(self):
        """!Return category number to use (according Settings)"""
        if not UserSettings.Get(group = 'vdigit', key = 'categoryMode', subkey = 'selection'):
            self.SetCategoryNextToUse()
        
        return UserSettings.Get(group = 'vdigit', key = 'category', subkey = 'value')

    def SetCategoryNextToUse(self):
        """!Find maximum category number in the map layer
        and update Digit.settings['category']

        @return 'True' on success, 'False' on failure
        """
        # vector map layer without categories, reset to '1'
        UserSettings.Set(group = 'vdigit', key = 'category', subkey = 'value', value = 1)
        
        if self.map:
            cat = self.GetCategory(UserSettings.Get(group = 'vdigit', key = 'layer', subkey = 'value'))
            cat += 1
            UserSettings.Set(group = 'vdigit', key = 'category', subkey = 'value',
                             value = cat)
