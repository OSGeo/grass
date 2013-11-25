"""!
@package vdigit.wxdigit

@brief wxGUI vector digitizer (base class)

Code based on wxVdigit C++ component from GRASS 6.4.0
(gui/wxpython/vdigit). Converted to Python in 2010/12-2011/01.

List of classes:
 - wxdigit::VDigitError
 - wxdigit::IVDigit

@todo Read large amounts of data from Vlib into arrays, which could
then be processed using NumPy and rendered using glDrawArrays or
glDrawElements, so no per-line/per-vertex processing in Python. Bulk
data processing with NumPy is much faster than iterating in Python
(and NumPy would be an excellent candidate for acceleration via
e.g. OpenCL or CUDA; I'm surprised it hasn't happened already).

(C) 2007-2011, 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import grass.script.core as grass

from grass.pydispatch.signal import Signal

from core.gcmd import GError
from core.debug import Debug
from core.settings import UserSettings
from core.utils import _
from vdigit.wxdisplay import DisplayDriver, GetLastError

try:
    from grass.lib.gis import *
    from grass.lib.vector import *
    from grass.lib.vedit import *
    from grass.lib.dbmi import *
except ImportError:
    pass

class VDigitError:
    def __init__(self, parent):
        """!Class for managing error messages of vector digitizer

        @param parent parent window for dialogs
        """
        self.parent  = parent
        self.caption = _('Digitization Error')
    
    def NoMap(self, name = None):
        """!No map for editing"""
        if name:
            message = _('Unable to open vector map <%s>.') % name
        else:
            message =  _('No vector map open for editing.')
        GError(message + ' ' + _('Operation canceled.'),
               parent  = self.parent,
               caption = self.caption)

    def WriteLine(self):
        """!Writing line failed
        """
        GError(message = _('Writing new feature failed. '
                           'Operation canceled.\n\n'
                           'Reason: %s') % GetLastError(),
               parent  = self.parent,
               caption = self.caption)

    def ReadLine(self, line):
        """!Reading line failed
        """
        GError(message = _('Reading feature id %d failed. '
                           'Operation canceled.') % line,
               parent  = self.parent,
               caption = self.caption)

    def DbLink(self, dblink):
        """!No dblink available
        """
        GError(message = _('Database link %d not available. '
                           'Operation canceled.') % dblink,
               parent  = self.parent,
               caption = self.caption)

    def Driver(self, driver):
        """!Staring driver failed
        """
        GError(message = _('Unable to start database driver <%s>. '
                           'Operation canceled.') % driver,
               parent  = self.parent,
               caption = self.caption)

    def Database(self, driver, database):
        """!Opening database failed
        """
        GError(message = _('Unable to open database <%(db)s> by driver <%(driver)s>. '
                           'Operation canceled.') % { 'db' : database, 'driver' : driver},
               parent  = self.parent,
               caption = self.caption)

    def DbExecute(self, sql):
        """!Sql query failed
        """
        GError(message = _("Unable to execute SQL query '%s'. "
                           "Operation canceled.") % sql,
               parent  = self.parent,
               caption = self.caption)

    def DeadLine(self, line):
        """!Dead line
        """
        GError(message = _("Feature id %d is marked as dead. "
                           "Operation canceled.") % line,
               parent  = self.parent,
               caption = self.caption)

    def FeatureType(self, ftype):
        """!Unknown feature type
        """
        GError(message = _("Unsupported feature type %d. "
                           "Operation canceled.") % ftype,
               parent  = self.parent,
               caption = self.caption)
        
class IVDigit:
    def __init__(self, mapwindow, driver = DisplayDriver):
        """!Base class for vector digitizer (ctypes interface)
        
        @param mapwindow reference to a map window
        """
        self.poMapInfo   = None      # pointer to Map_info
        self.mapWindow = mapwindow

        # background map
        self.bgMapInfo   = Map_info()
        self.poBgMapInfo = self.popoBgMapInfo = None
        
        # TODO: replace this by using giface
        if not mapwindow.parent.IsStandalone():
            goutput = mapwindow.parent.GetLayerManager().GetLogWindow()
            log = goutput.GetLog(err = True)
            progress = mapwindow.parent._giface.GetProgress()
        else:
            log = sys.stderr
            progress = None
        
        self.toolbar = mapwindow.parent.toolbars['vdigit']
        
        self._error   = VDigitError(parent = self.mapWindow)
        
        self._display = driver(device    = mapwindow.pdcVector,
                               deviceTmp = mapwindow.pdcTmp,
                               mapObj    = mapwindow.Map,
                               window    = mapwindow,
                               glog      = log,
                               gprogress = progress)
        
        # GRASS lib
        self.poPoints = Vect_new_line_struct()
        self.poCats   = Vect_new_cats_struct()
        
        # self.SetCategory()
        
        # layer / max category
        self.cats = dict()
        
        self._settings = dict()
        self.UpdateSettings() # -> self._settings
        
        # undo/redo
        self.changesets = list()
        self.changesetCurrent = -1 # first changeset to apply
        
        if self.poMapInfo:
            self.InitCats()

        self.emit_signals = False

        # signals which describes features changes during digitization, 
        # activate them using EmitSignals method 

        # currently implemented for functionality used by wx.iclass (for scatter plot)
        
        # signals parameter description:
        # old_bboxs - list of bboxes of boundary features, which covers changed areas
        # it is bbox of old state (before edit)
        # old_areas_cats - list of area categories of boundary features of old state (before edit)
        # same position in both lists corresponds to same feature

        # new_bboxs = list of bboxes of created features / after edit
        # new_areas_cats list of areas cats of created features / after edit
        # same position in both lists corresponds to same features

        # for description of items in bbox and area_cats lists see return value of _getaAreaBboxCats

        # TODO currently it is not possible to identify corresponded features
        # in old and new lists (requires changed to vector updated format)
        # TODO return feature type
        
        #TODO handle errors?
        self.featureAdded = Signal('IVDigit.featureAdded')
        self.areasDeleted = Signal('IVDigit.areasDeleted')
        self.vertexMoved = Signal('IVDigit.vertexMoved')
        self.vertexAdded = Signal('IVDigit.vertexAdded')
        self.vertexRemoved = Signal('IVDigit.vertexRemoved')
        self.featuresDeleted = Signal('IVDigit.featuresDeleted')
        self.featuresMoved = Signal('IVDigit.featuresMoved')
        self.lineEdited = Signal('IVDigit.lineEdited')

    def __del__(self):
        Debug.msg(1, "IVDigit.__del__()")
        Vect_destroy_line_struct(self.poPoints)
        self.poPoints = None
        Vect_destroy_cats_struct(self.poCats)
        self.poCats = None
        
        if self.poBgMapInfo:
            Vect_close(self.poBgMapInfo)
            self.poBgMapInfo = self.popoBgMapInfo = None
            del self.bgMapInfo
     
    def EmitSignals(self, emit):
        """!Activate/deactivate signals which describes features changes during digitization.
        """
        self.emit_signals = emit

    def CloseBackgroundMap(self):
        """!Close background vector map"""
        if not self.poBgMapInfo:
            return
        
        Vect_close(self.poBgMapInfo)
        self.poBgMapInfo = self.popoBgMapInfo = None
        
    def OpenBackgroundMap(self, bgmap):
        """!Open background vector map

        @todo support more background maps then only one
        
        @param bgmap name of vector map to be opened

        @return pointer to map_info
        @return None on error
        """
        name   = create_string_buffer(GNAME_MAX)
        mapset = create_string_buffer(GMAPSET_MAX)
        if not G_name_is_fully_qualified(bgmap, name, mapset):
            name   = str(bgmap)
            mapset = str(G_find_vector2(bgmap, ''))
        else:
            name   = str(name.value)
            mapset = str(mapset.value)
        
        if (name == Vect_get_name(self.poMapInfo) and \
                mapset == Vect_get_mapset(self.poMapInfo)):
            self.poBgMapInfo = self.popoBgMapInfo = None
            self._error.NoMap(bgmap)
            return
        
        self.poBgMapInfo = pointer(self.bgMapInfo)
        self.popoBgMapInfo = pointer(self.poBgMapInfo)
        if Vect_open_old(self.poBgMapInfo, name, mapset) == -1:
            self.poBgMapInfo = self.popoBgMapInfo = None
            self._error.NoMap(bgmap)
            return
        
    def _getSnapMode(self):
        """!Get snapping mode

         - snap to vertex
         - snap to nodes
         - no snapping
        
        @return snap mode
        """
        threshold = self._display.GetThreshold()
        if threshold > 0.0:
            if UserSettings.Get(group = 'vdigit', key = 'snapToVertex', subkey = 'enabled'):
                return SNAPVERTEX
            else:
                return SNAP
        else:
            return NO_SNAP
    
    def _getNewFeaturesLayer(self):
        """!Returns layer of new feature (from settings)"""
        if UserSettings.Get(group = 'vdigit', key = "categoryMode", subkey = 'selection') == 2:
            layer = -1 # -> no category
        else:
            layer = UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value')
        
        return layer
        
    def _getNewFeaturesCat(self):
        """!Returns category of new feature (from settings)"""
        if UserSettings.Get(group = 'vdigit', key = "categoryMode", subkey = 'selection') == 2:
            cat   = -1
        else:
            cat   = self.SetCategory()
        
        return cat
        
    def _breakLineAtIntersection(self, line, pointsLine):
        """!Break given line at intersection

        \param line line id
        \param pointsLine line geometry
        
        \return number of modified lines
        """
        if not self._checkMap():
            return -1
        
        if not Vect_line_alive(self.poMapInfo, line):
            return 0
        
        if not pointsLine:
            if Vect_read_line(self.poMapInfo, self.poPoints, None, line) < 0:
                self._error.ReadLine(line)
                return -1
            points = self.poPoints
        else:
            points = pointsLine
        
        listLine  = Vect_new_boxlist(0)
        listRef   = Vect_new_list()
        listBreak = Vect_new_list()
    
        pointsCheck = Vect_new_line_struct()
    
        lineBox = bound_box()
        # find all relevant lines
        Vect_get_line_box(self.poMapInfo, line, byref(lineBox))
        Vect_select_lines_by_box(self.poMapInfo, byref(lineBox),
                                 GV_LINES, listLine)
    
        # check for intersection
        Vect_list_append(listBreak, line)
        Vect_list_append(listRef, line)
        for i in range(listLine.contents.n_values):
            lineBreak = listLine.contents.id[i]
            if lineBreak == line:
                continue
            
            ltype = Vect_read_line(self.poMapInfo, pointsCheck, None, lineBreak)
            if not (ltype & GV_LINES):
                continue
            
            if Vect_line_check_intersection(self.poPoints, pointsCheck,
                                            WITHOUT_Z):
                Vect_list_append(listBreak, lineBreak)
        
        ret = Vect_break_lines_list(self.poMapInfo, listBreak, listRef,
                                    GV_LINES, None)
        
        Vect_destroy_line_struct(pointsCheck)

        Vect_destroy_boxlist(listLine)
        Vect_destroy_list(listBreak)
        Vect_destroy_list(listRef)
        
        return ret
    
    def _addChangeset(self):
        data = list()
        for i in range(Vect_get_num_updated_lines(self.poMapInfo) - 1, -1, -1):
            line = Vect_get_updated_line(self.poMapInfo, i)
            offset = Vect_get_updated_line_offset(self.poMapInfo, i)
            data.append({ 'line'   : line,
                          'offset' : offset })
        
        self.changesetCurrent += 1
        self.changesets.insert(self.changesetCurrent, data)
        
        Vect_reset_updated(self.poMapInfo)
                
    def _applyChangeset(self, changeset, undo):
        """!Apply changeset (undo/redo changeset)
        
        @param changeset changeset id
        @param undo True for undo otherwise redo

        @return 1 changeset applied
        @return 0 changeset not applied
        @return -1 on error
        """
        if changeset < 0 or changeset >= len(self.changesets):
            return -1
        
        ret = 0
        actions = self.changesets[changeset]
        
        for action in actions:
            line = action['line']
            if action['offset'] > 0:
                if Vect_line_alive(self.poMapInfo, line):
                    Debug.msg(3, "IVDigit._applyChangeset(): changeset=%d, action=add, line=%d -> deleted",
                              changeset, line)
                    
                    Vect_delete_line(self.poMapInfo, line)
                    ret = 1
                else:
                    Debug.msg(3, "Digit.ApplyChangeset(): changeset=%d, action=add, line=%d dead",
                              changeset, line)
            else: # delete
                offset = abs(action['offset'])
                
                if not Vect_line_alive(self.poMapInfo, line):
                    Debug.msg(3, "Digit.ApplyChangeset(): changeset=%d, action=delete, line=%d -> added",
                              changeset, line)
                    
                    if Vect_restore_line(self.poMapInfo, line, offset) < 0:
                        return -1
                    ret = 1
                else:
                    Debug.msg(3, "Digit.ApplyChangeset(): changeset=%d, action=delete, line=%d alive",
                              changeset, line)

            action['offset'] *= -1
        Vect_reset_updated(self.poMapInfo)
        
        return ret
    
    def AddFeature(self, ftype, points):
        """!Add new feature
        
        @param ftype feature type (point, line, centroid, boundary)
        @param points tuple of points ((x, y), (x, y), ...)
        
        @return tuple (number of added features, feature ids)
        """
        layer = self._getNewFeaturesLayer()
        cat = self._getNewFeaturesCat()
        
        if ftype == 'point':
            vtype = GV_POINT
        elif ftype == 'line':
            vtype = GV_LINE
        elif ftype == 'centroid':
            vtype = GV_CENTROID
        elif ftype == 'boundary':
            vtype = GV_BOUNDARY
        elif ftype == 'area':
            vtype = GV_AREA
        else:
            GError(parent = self.mapWindow,
                   message = _("Unknown feature type '%s'") % ftype)
            return (-1, None)
        
        if vtype & GV_LINES and len(points) < 2:
            GError(parent = self.mapWindow,
                   message = _("Not enough points for line"))
            return (-1, None)
        
        self.toolbar.EnableUndo()

        ret = self._addFeature(vtype, points, layer, cat,
                               self._getSnapMode(), self._display.GetThreshold())
        if ret[0] > -1 and self.emit_signals:
            self.featureAdded.emit(new_bboxs=[self._createBbox(points)], 
                                   new_areas_cats=[[{layer : [cat]}, None]])

        return ret

    def DeleteSelectedLines(self):
        """!Delete selected features

        @return number of deleted features
        """
        if not self._checkMap():
            return -1
        
        # collect categories for deleting if requested
        deleteRec = UserSettings.Get(group = 'vdigit', key = 'delRecord', subkey = 'enabled')
        catDict = dict()

        old_bboxs = []
        old_areas_cats = []
        if deleteRec:
            for i in self._display.selected['ids']:
                
                if Vect_read_line(self.poMapInfo, None, self.poCats, i) < 0:
                    self._error.ReadLine(i)
                
                if self.emit_signals:
                    ret = self._getLineAreaBboxCats(i)
                    if ret:
                        old_bboxs += ret[0]
                        old_areas_cats += ret[1]
                
                # catDict was not used -> put into comment
                #cats = self.poCats.contents
                #for j in range(cats.n_cats):
                #    if cats.field[j] not in catDict.keys():
                #        catDict[cats.field[j]] = list()
                #    catDict[cats.field[j]].append(cats.cat[j])
        
        poList = self._display.GetSelectedIList()
        nlines = Vedit_delete_lines(self.poMapInfo, poList)
        
        Vect_destroy_list(poList)
        self._display.selected['ids'] = list()
        
        if nlines > 0:
            if deleteRec:
                self._deleteRecords(catDict)
            self._addChangeset()
            self.toolbar.EnableUndo()

            if self.emit_signals:
                self.featuresDeleted.emit(old_bboxs=old_bboxs, 
                                          old_areas_cats=old_areas_cats)

        return nlines
            
    def _deleteRecords(self, cats):
        """!Delete records from attribute table
        
        @param cats directory field/list of cats
        """
        handle   = dbHandle()
        poHandle = pointer(handle)
        stmt     = dbString()
        poStmt   = pointer(stmt)
        
        for dblink in range(Vect_get_num_dblinks(self.poMapInfo)):
            poFi = Vect_get_dblink(self.poMapInfo, dblink)
            if poFi is None:
                self._error.DbLink(dblink)
                return -1
            
            Fi = poFi.contents
            if Fi.number not in cats.keys():
                continue
            
            poDriver = db_start_driver(Fi.driver)
            if poDriver is None:
                self._error.Driver(Fi.driver)
                return -1
            
            db_init_handle(poHandle)
            db_set_handle(poHandle, Fi.database, None)
            if db_open_database(poDriver, poHandle) != DB_OK:
                self._error.Database(Fi.driver, Fi.database)
                return -1
            
            db_init_string(poStmt)
            db_set_string(poStmt, "DELETE FROM %s WHERE" % Fi.table)
            n_cats = 0
            for cat in cats[Fi.number]:
                if n_cats > 0:
                    db_append_string(poStmt, " or")
                    
                db_append_string(poStmt, " %s = %d" % (Fi.key, cat))
                n_cats += 1
            
            if n_cats > 0 and \
                    db_execute_immediate(poDriver, poStmt) != DB_OK:
                self._error.DbExecute(db_get_string(poStmt))
                return -1
            
            db_close_database_shutdown_driver(poDriver)
        
    def DeleteSelectedAreas(self):
        """!Delete selected areas (centroid+boundaries)

        @return number of deleted 
        """
        if len(self._display.selected['ids']) < 1:
            return 0
        
        poList = self._display.GetSelectedIList()
        cList  = poList.contents
        
        nareas = 0
        old_bboxs = []
        old_areas_cats = []

        for i in range(cList.n_values):

            if Vect_get_line_type(self.poMapInfo, cList.value[i]) != GV_CENTROID:
                continue

            if self.emit_signals:
                area = Vect_get_centroid_area(self.poMapInfo, cList.value[i]);
                if area > 0: 
                    bbox, cats = self._getaAreaBboxCats(area)
                    old_bboxs += bbox
                    old_areas_cats += cats

            nareas += Vedit_delete_area_centroid(self.poMapInfo, cList.value[i])
        
        if nareas > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
            if self.emit_signals:
                self.areasDeleted.emit(old_bboxs=old_bboxs, 
                                       old_areas_cats=old_areas_cats)        

        return nareas
   
    def _getLineAreaBboxCats(self, ln_id):
        """!Helper function

        @param id of feature
        @return None if the feature does not exists
        @return list of @see _getaAreaBboxCats
        """
        ltype = Vect_read_line(self.poMapInfo, None, None, ln_id)

        if ltype == GV_CENTROID:
            #TODO centroid opttimization, can be edited also its area -> it will appear two times in new_ lists
            return self._getCentroidAreaBboxCats(ln_id)
        else: 
            return [self._getBbox(ln_id)], [self._getLineAreasCategories(ln_id)]


    def _getCentroidAreaBboxCats(self, centroid):
        """!Helper function

        @param id of an centroid 
        @return None if area does not exists
        @return see return of _getaAreaBboxCats
        """
        if not Vect_line_alive(self.poMapInfo, centroid):
            return None

        area = Vect_get_centroid_area(self.poMapInfo, centroid)  
        if area > 0:
            return self._getaAreaBboxCats(area)
        else:
            return None

    def _getaAreaBboxCats(self, area):
        """!Helper function

        @param area area id
        @return list of categories @see _getLineAreasCategories and 
        list of bboxes @see _getBbox of area boundary features
        """
        po_b_list = Vect_new_list()
        Vect_get_area_boundaries(self.poMapInfo, area, po_b_list);
        b_list = po_b_list.contents

        geoms = []
        areas_cats = []

        if b_list.n_values > 0:
            for i_line in range(b_list.n_values):

                line = b_list.value[i_line];

                geoms.append(self._getBbox(abs(line)))
                areas_cats.append(self._getLineAreasCategories(abs(line)))
        
        Vect_destroy_list(po_b_list);

        return geoms, areas_cats

    def _getLineAreasCategories(self, ln_id):
        """!Helper function

        @param line_id id of boundary feature 
        @return categories of areas on the left, right side of the feature
        @return format: [[{layer : [cat]}, None]] means:
                area to the left (list of layers which has cats list as values), 
                area to the right (no area there in this case (None)) 
        @return [] the feature is not boundary or does not exists
        """
        if not Vect_line_alive (self.poMapInfo, ln_id):
            return []

        ltype = Vect_read_line(self.poMapInfo, None, None, ln_id)
        if ltype != GV_BOUNDARY:
            return []

        cats = [None, None]

        left = c_int()
        right = c_int()

        if Vect_get_line_areas(self.poMapInfo, ln_id, pointer(left), pointer(right)) == 1:
            areas = [left.value, right.value]

            for i, a in enumerate(areas):
                if a > 0: 
                    centroid = Vect_get_area_centroid(self.poMapInfo, a)
                    if centroid <= 0:
                        continue
                    c = self._getCategories(centroid)
                    if c:
                        cats[i] = c

        return cats

    def _getCategories(self, ln_id):
        """!Helper function

        @param line_id id of feature
        @return list of the feature categories [{layer : cats}, next layer...]
        @return None feature does not exist
        """
        if not Vect_line_alive (self.poMapInfo, ln_id):
            return none

        poCats = Vect_new_cats_struct()
        if Vect_read_line(self.poMapInfo, None, poCats, ln_id) < 0:
            Vect_destroy_cats_struct(poCats)
            return None

        cCats = poCats.contents

        cats = {}
        for j in range(cCats.n_cats):
            if cats.has_key(cCats.field[j]):
                cats[cCats.field[j]].append(cCats.cat[j])
            else:
                cats[cCats.field[j]] = [cCats.cat[j]]
    
        Vect_destroy_cats_struct(poCats)
        return cats

    def _getBbox(self, ln_id):
        """!Helper function

        @param line_id id of line feature
        @return bbox bounding box of the feature
        @return None feature does not exist
        """
        if not Vect_line_alive (self.poMapInfo, ln_id):
            return None

        poPoints = Vect_new_line_struct()
        if Vect_read_line(self.poMapInfo, poPoints, None, ln_id) < 0:
            Vect_destroy_line_struct(poPoints)
            return []

        geom = self._convertGeom(poPoints)
        bbox = self._createBbox(geom)
        Vect_destroy_line_struct(poPoints)
        return bbox

    def _createBbox(self, points):
        """!Helper function

        @param points list of points [(x, y), ...] to be bbox created for
        @return bbox bounding box of points {'maxx':, 'maxy':, 'minx':, 'miny'}
        """
        bbox = {}
        for pt in points:
            if not bbox.has_key('maxy'):
                bbox['maxy'] = pt[1]
                bbox['miny'] = pt[1]
                bbox['maxx'] = pt[0]
                bbox['minx'] = pt[0]
                continue
                
            if   bbox['maxy'] < pt[1]:
                bbox['maxy'] = pt[1]
            elif bbox['miny'] > pt[1]:
                bbox['miny'] = pt[1]
                
            if   bbox['maxx'] < pt[0]:
                bbox['maxx'] = pt[0]
            elif bbox['minx'] > pt[0]:
                bbox['minx'] = pt[0]
        return bbox

    def _convertGeom(self, poPoints):
        """!Helper function
            convert geom from ctypes line_pts to python list

        @return coords in python list [(x, y),...] 
        """
        Points = poPoints.contents

        pts_geom = []
        for j in range(Points.n_points):
            pts_geom.append((Points.x[j], Points.y[j]))

        return pts_geom

    def MoveSelectedLines(self, move):
        """!Move selected features

        @param move direction (x, y)
        """
        if not self._checkMap():
            return -1
        
        nsel = len(self._display.selected['ids'])
        if nsel < 1:
            return -1   
        
        thresh = self._display.GetThreshold()
        snap   = self._getSnapMode()
        
        poList = self._display.GetSelectedIList()

        if self.emit_signals:
            old_bboxs = []
            old_areas_cats = []
            for sel_id in self._display.selected['ids']:
                ret = self._getLineAreaBboxCats(sel_id)
                if ret:
                    old_bboxs += ret[0]
                    old_areas_cats += ret[1]
        
            Vect_set_updated(self.poMapInfo, 1)
            n_up_lines_old = Vect_get_num_updated_lines(self.poMapInfo)
        
        nlines = Vedit_move_lines(self.poMapInfo, self.popoBgMapInfo, int(self.poBgMapInfo is not None),
                                  poList,
                                  move[0], move[1], 0,
                                  snap, thresh)

        Vect_destroy_list(poList)

        if nlines > 0 and self.emit_signals:
            new_bboxs = []
            new_areas_cats = []
            n_up_lines = Vect_get_num_updated_lines(self.poMapInfo)
            for i in range(n_up_lines_old, n_up_lines):
                new_id = Vect_get_updated_line(self.poMapInfo, i)
                ret = self._getLineAreaBboxCats(new_id)
                if ret:
                    new_bboxs += ret[0]
                    new_areas_cats += ret[1]

        if nlines > 0 and self._settings['breakLines']:
            for i in range(1, nlines):
                self._breakLineAtIntersection(nlines + i, None, changeset)
        
        if nlines > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
            
            if self.emit_signals:
                self.featuresMoved.emit(new_bboxs=new_bboxs,
                                        old_bboxs=old_bboxs, 
                                        old_areas_cats=old_areas_cats, 
                                        new_areas_cats=new_areas_cats)

        return nlines

    def MoveSelectedVertex(self, point, move):
        """!Move selected vertex of the line

        @param point location point
        @param move  x,y direction
        
        @return id of new feature
        @return 0 vertex not moved (not found, line is not selected)
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        if len(self._display.selected['ids']) != 1:
            return -1

        # move only first found vertex in bbox 
        poList = self._display.GetSelectedIList()

        if self.emit_signals:
            cList = poList.contents
            old_bboxs = [self._getBbox(cList.value[0])]
            old_areas_cats = [self._getLineAreasCategories(cList.value[0])]

            Vect_set_updated(self.poMapInfo, 1)
            n_up_lines_old = Vect_get_num_updated_lines(self.poMapInfo)

        Vect_reset_line(self.poPoints)
        Vect_append_point(self.poPoints, point[0], point[1], 0.0)

        moved = Vedit_move_vertex(self.poMapInfo, self.popoBgMapInfo, int(self.poBgMapInfo is not None),
                                  poList, self.poPoints,
                                  self._display.GetThreshold(type = 'selectThresh'),
                                  self._display.GetThreshold(),
                                  move[0], move[1], 0.0,
                                  1, self._getSnapMode())
        Vect_destroy_list(poList)

        if moved > 0 and self.emit_signals:
            n_up_lines = Vect_get_num_updated_lines(self.poMapInfo)

            new_bboxs = []
            new_areas_cats = []
            for i in range(n_up_lines_old, n_up_lines):
                new_id = Vect_get_updated_line(self.poMapInfo, i)
                new_bboxs.append(self._getBbox(new_id))
                new_areas_cats.append(self._getLineAreasCategories(new_id))

        if moved > 0 and self._settings['breakLines']:
            self._breakLineAtIntersection(Vect_get_num_lines(self.poMapInfo),
                                          None)
        
        if moved > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()

            if self.emit_signals:
                self.vertexMoved.emit(new_bboxs=new_bboxs,  
                                      new_areas_cats=new_areas_cats, 
                                      old_areas_cats=old_areas_cats, 
                                      old_bboxs=old_bboxs)

        return moved

    def AddVertex(self, coords):
        """!Add new vertex to the selected line/boundary on position 'coords'

        @param coords coordinates to add vertex

        @return id of new feature
        @return 0 nothing changed
        @return -1 on failure
        """
        added = self._ModifyLineVertex(coords, add = True)
        
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
        deleted = self._ModifyLineVertex(coords, add = False)
        
        if deleted > 0:
            self.toolbar.EnableUndo()

        return deleted


    def SplitLine(self, point):
        """!Split/break selected line/boundary on given position

        @param point point where to split line
        
        @return 1 line modified
        @return 0 nothing changed
        @return -1 error
        """
        thresh = self._display.GetThreshold('selectThresh')
        if not self._checkMap():
            return -1
        
        poList  = self._display.GetSelectedIList()
        Vect_reset_line(self.poPoints)
        Vect_append_point(self.poPoints, point[0], point[1], 0.0)
        
        ret = Vedit_split_lines(self.poMapInfo, poList,
                                self.poPoints, thresh, None)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self.addChangeset()
            self.toolbar.EnableUndo()
        
        return ret

    def EditLine(self, line, coords):
        """!Edit existing line/boundary

        @param line feature id to be modified
        @param coords list of coordinates of modified line

        @return feature id of new line
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        if len(coords) < 2:
            self.DeleteSelectedLines()
            return 0
        
        if not Vect_line_alive(self.poMapInfo, line):
            self._error.DeadLine(line)
            return -1
        
        # read original feature
        ltype = Vect_read_line(self.poMapInfo, None, self.poCats, line)
        if ltype < 0:
            self._error.ReadLine(line)
            return -1
        
        if self.emit_signals:
            old_bboxs = [self._getBbox(line)]
            old_areas_cats = [self._getLineAreasCategories(line)]

        # build feature geometry
        Vect_reset_line(self.poPoints)
        for p in coords:
            Vect_append_point(self.poPoints, p[0], p[1], 0.0)

        # apply snapping (node or vertex)
        snap = self._getSnapMode()
        if snap != NO_SNAP:
            modeSnap = not (snap == SNAP)
            Vedit_snap_line(self.poMapInfo, self.popoBgMapInfo,
                            int(self.poBgMapInfo is not None),
                           -1, self.poPoints, self._display.GetThreshold(), modeSnap)
        
        newline = Vect_rewrite_line(self.poMapInfo, line, ltype,
                                    self.poPoints, self.poCats)
        if newline > 0 and self.emit_signals:
            new_geom = [self._getBbox(newline)]
            new_areas_cats = [self._getLineAreasCategories(newline)]
        
        if newline > 0 and self._settings['breakLines']:
            self._breakLineAtIntersection(newline, None)
        
        if newline > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
    
            if self.emit_signals:
                self.lineEdited.emit(old_bboxs=old_bboxs, 
                                     old_areas_cats=old_areas_cats, 
                                     new_bboxs=new_bboxs, 
                                     new_areas_cats=new_areas_cats)

        return newline

    def FlipLine(self):
        """!Flip selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_flip_lines(self.poMapInfo, poList)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
        
        return ret

    def MergeLine(self):
        """!Merge selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_merge_lines(self.poMapInfo, poList)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
        
        return ret

    def BreakLine(self):
        """!Break selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        poList = self._display.GetSelectedIList()
        ret = Vect_break_lines_list(self.poMapInfo, poList, None,
                                    GV_LINES, None)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
        
        return ret

    def SnapLine(self):
        """!Snap selected lines/boundaries

        @return on success
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        poList = self._display.GetSelectedIList()
        Vect_snap_lines_list(self.poMapInfo, poList,
                             self._display.GetThreshold(), None)
        Vect_destroy_list(poList)
        
        if nlines < Vect_get_num_lines(self.poMapInfo):
            self._addChangeset()
            self.toolbar.EnableUndo()
        
    def ConnectLine(self):
        """!Connect selected lines/boundaries

        @return 1 lines connected
        @return 0 lines not connected
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_connect_lines(self.poMapInfo, poList,
                                  self._display.GetThreshold())
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
        
        return ret
        
    def CopyLine(self, ids = []):
        """!Copy features from (background) vector map

        @param ids list of line ids to be copied

        @return number of copied features
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        poList = self._display.GetSelectedIList(ids)
        ret = Vedit_copy_lines(self.poMapInfo, self.poBgMapInfo,
                               poList)
        Vect_destroy_list(poList)
        
        if ret > 0 and self.poBgMapInfo and self._settings['breakLines']:
            for i in range(1, ret):
                self._breakLineAtIntersection(nlines + i, None)
        
        if ret > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
        
        return ret

    def CopyCats(self, fromId, toId, copyAttrb = False):
        """!Copy given categories to objects with id listed in ids

        @param cats ids of 'from' feature
        @param ids  ids of 'to' feature(s)

        @return number of modified features
        @return -1 on error
        """
        if len(fromId) < 1 or len(toId) < 1:
            return 0
        
        poCatsFrom = self.poCats
        poCatsTo = Vect_new_cats_struct();
        
        nlines = 0
        
        for fline in fromId:
            if not Vect_line_alive(self.poMapInfo, fline):
                continue
            
            if Vect_read_line(self.poMapInfo, None, poCatsFrom, fline) < 0:
                self._error.ReadLine(fline)
                return -1
            
            for tline in toId:
                if not Vect_line_alive(self.poMapInfo, tline):
                    continue
                
                ltype = Vect_read_line(self.poMapInfo, self.poPoints, poCatsTo, tline)
                if ltype < 0:
                    self._error.ReadLine(fline)
                    return -1
                
                catsFrom = poCatsFrom.contents
                for i in range(catsFrom.n_cats):
                    if not copyAttrb:
                        # duplicate category
                        cat = catsFrom.cat[i]
                    else:
                        # duplicate attributes
                        cat = self.cats[catsFrom.field[i]] + 1
                        self.cats[catsFrom.field[i]] = cat
                        poFi = Vect_get_field(self.poMapInfo, catsFrom.field[i])
                        if not poFi:
                            self._error.DbLink(i)
                            return -1
                        
                        fi = poFi.contents
                        driver = db_start_driver(fi.driver)
                        if not driver:
                            self._error.Driver(fi.driver)
                            return -1
                        
                        handle = dbHandle()
                        db_init_handle(byref(handle))
                        db_set_handle(byref(handle), fi.database, None)
                        if db_open_database(driver, byref(handle)) != DB_OK:
                            db_shutdown_driver(driver)
                            self._error.Database(fi.driver, fi.database)
                            return -1
                        
                        stmt = dbString()
                        db_init_string(byref(stmt))
                        db_set_string(byref(stmt),
                                      "SELECT * FROM %s WHERE %s=%d" % (fi.table, fi.key,
                                                                        catsFrom.cat[i]))
                        
                        cursor = dbCursor()
                        if db_open_select_cursor(driver, byref(stmt), byref(cursor),
                                                 DB_SEQUENTIAL) != DB_OK:
                                db_close_database_shutdown_driver(driver)
                                return -1
                        
                        table = db_get_cursor_table(byref(cursor))
                        ncols = db_get_table_number_of_columns(table)
                        
                        sql = "INSERT INTO %s VALUES (" % fi.table
                        # fetch the data
                        more = c_int()
                        while True:
                            if db_fetch(byref(cursor), DB_NEXT, byref(more)) != DB_OK:
                                db_close_database_shutdown_driver(driver)
                                return -1
                            if not more.value:
                                break
                            
                            value_string = dbString()
                            for col in range(ncols):
                                if col > 0:
                                    sql += ","
                                    
                                column = db_get_table_column(table, col)
                                if db_get_column_name(column) == fi.key:
                                    sql += "%d" % cat
                                    continue
                                
                                value = db_get_column_value(column)
                                db_convert_column_value_to_string(column, byref(value_string))
                                if db_test_value_isnull(value):
                                    sql += "NULL"
                                else:
                                    ctype = db_sqltype_to_Ctype(db_get_column_sqltype(column))
                                    if ctype != DB_C_TYPE_STRING:
                                        sql += db_get_string(byref(value_string))
                                    else:
                                        sql += "'%s'" % db_get_string(byref(value_string))
                        
                        sql += ")"
                        db_set_string(byref(stmt), sql)
                        if db_execute_immediate(driver, byref(stmt)) != DB_OK:
                            db_close_database_shutdown_driver(driver)
                            return -1
                        
                        db_close_database_shutdown_driver(driver)
                        G_free(poFi)
                
                if Vect_cat_set(poCatsTo, catsFrom.field[i], cat) < 1:
                    continue
                
                if Vect_rewrite_line(self.poMapInfo, tline, ltype, self.poPoints, poCatsTo) < 0:
                    self._error.WriteLine()
                    return -1
                
                nlines +=1
        
        Vect_destroy_cats_struct(poCatsTo)
        
        if nlines > 0:
            self.toolbar.EnableUndo()
        
        return nlines

    def _selectLinesByQueryThresh(self):
        """!Generic method used for SelectLinesByQuery() -- to get
        threshold value"""
        thresh = 0.0
        if UserSettings.Get(group = 'vdigit', key = 'query', subkey = 'selection') == 0:
            thresh = UserSettings.Get(group = 'vdigit', key = 'queryLength', subkey = 'thresh')
            if UserSettings.Get(group = 'vdigit', key = "queryLength", subkey = 'than-selection') == 0:
                thresh = -1 * thresh
        else:
            thresh = UserSettings.Get(group = 'vdigit', key = 'queryDangle', subkey = 'thresh')
            if UserSettings.Get(group = 'vdigit', key = "queryDangle", subkey = 'than-selection') == 0:
                thresh = -1 * thresh
        
        return thresh

    def SelectLinesByQuery(self, bbox):
        """!Select features by query
        
        @todo layer / 3D
        
        @param bbox bounding box definition
        """
        if not self._checkMap():
            return -1
        
        thresh = self._selectLinesByQueryThresh()
        
        query = QUERY_UNKNOWN
        if UserSettings.Get(group = 'vdigit', key = 'query', subkey = 'selection') == 0:
            query = QUERY_LENGTH
        else:
            query = QUERY_DANGLE
        
        ftype = GV_POINTS | GV_LINES # TODO: 3D
        layer = 1 # TODO
        
        ids = list()
        poList = Vect_new_list()
        coList = poList.contents
        if UserSettings.Get(group = 'vdigit', key = 'query', subkey = 'box'):
            Vect_reset_line(self.poPoints)
            x1, y1 = bbox[0]
            x2, y2 = bbox[1]
            z1 = z2 = 0.0
            
            Vect_append_point(self.poPoints, x1, y1, z1)
            Vect_append_point(self.poPoints, x2, y1, z2)
            Vect_append_point(self.poPoints, x2, y2, z1)
            Vect_append_point(self.poPoints, x1, y2, z2)
            Vect_append_point(self.poPoints, x1, y1, z1)

            Vect_select_lines_by_polygon(self.poMapInfo, self.poPoints, 0, None,
                                         ftype, poList)
            
            if coList.n_values == 0:
                return ids
        
        Vedit_select_by_query(self.poMapInfo,
                              ftype, layer, thresh, query,
                              poList)
        
        for i in range(coList.n_values):
            ids.append(int(coList.value[i]))
            
        Debug.msg(3, "IVDigit.SelectLinesByQuery(): lines=%d", coList.n_values)    
        Vect_destroy_list(poList)
        
        return ids

    def IsVector3D(self):
        """!Check if open vector map is 3D
        """
        if not self._checkMap():
            return False
        
        return Vect_is_3d(self.poMapInfo)
    
    def GetLineLength(self, line):
        """!Get line length

        @param line feature id

        @return line length
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        if not Vect_line_alive(self.poMapInfo, line):
            return -1
    
        ltype = Vect_read_line(self.poMapInfo, self.poPoints, None, line)
        if ltype < 0:
            self._error.ReadLine(line)
            return ret
        
        length = -1
        if ltype & GV_LINES: # lines & boundaries
            length = Vect_line_length(self.poPoints)
        
        return length

    def GetAreaSize(self, centroid):
        """!Get area size

        @param centroid centroid id

        @return area size
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        ltype = Vect_read_line(self.poMapInfo, None, None, centroid)
        if ltype < 0:
            self._error.ReadLine(line)
            return ret
        
        if ltype != GV_CENTROID:
            return -1
        
        area = Vect_get_centroid_area(self.poMapInfo, centroid)
        size = -1
        if area > 0:
            if not Vect_area_alive(self.poMapInfo, area):
                return size
            
            size = Vect_get_area_area(self.poMapInfo, area)
        
        return size
        
    def GetAreaPerimeter(self, centroid):
        """!Get area perimeter
        
        @param centroid centroid id
        
        @return area size
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        ltype = Vect_read_line(self.poMapInfo, None, None, centroid)
        if ltype < 0:
            self._error.ReadLine(line)
            return ret
        
        if ltype != GV_CENTROID:
            return -1
        
        area = Vect_get_centroid_area(self.poMapInfo, centroid)
        perimeter = -1
        if area > 0:
            if not Vect_area_alive(self.poMapInfo, area):
                return -1
            
            Vect_get_area_points(self.poMapInfo, area, self.poPoints)
            perimeter = Vect_area_perimeter(self.poPoints)
        
        return perimeter
    
    def SetLineCats(self, line, layer, cats, add = True):
        """!Set categories for given line and layer

        @param line feature id
        @param layer layer number (-1 for first selected line)
        @param cats list of categories
        @param add if True to add, otherwise do delete categories

        @return new feature id (feature need to be rewritten)
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        if line < 1 and len(self._display.selected['ids']) < 1:
            return -1
        
        update = False
        if line == -1:
            update = True
            line = self._display.selected['ids'][0]

        if not Vect_line_alive(self.poMapInfo, line):
            return -1
        
        ltype = Vect_read_line(self.poMapInfo, self.poPoints, self.poCats, line)
        if ltype < 0:
            self._error.ReadLine(line)
            return -1
        
        for c in cats:
            if add:
                Vect_cat_set(self.poCats, layer, c)
            else:
                Vect_field_cat_del(self.poCats, layer, c)
        
        newline = Vect_rewrite_line(self.poMapInfo, line, ltype,
                                    self.poPoints, self.poCats)
        
        if newline > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
        
        if update:
            # update line id since the line was rewritten
            self._display.selected['ids'][0] =  newline
        
        return newline

    def TypeConvForSelectedLines(self):
        """!Feature type conversion for selected objects.

        Supported conversions:
         - point <-> centroid
         - line <-> boundary

        @return number of modified features
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_chtype_lines(self.poMapInfo, poList)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
        
        return ret

    def Undo(self, level = -1):
        """!Undo action

        @param level levels to undo (0 to revert all)

        @return id of current changeset
        """
        changesetLast = len(self.changesets) - 1
        
        if changesetLast < 0:
            return changesetLast
        
        if level > 0 and self.changesetCurrent < 0:
            self.changesetCurrent = 0
        elif level < 0 and self.changesetCurrent > changesetLast:
            self.changesetCurrent = changesetLast
        elif level == 0:
            # 0 -> undo all
            level = -1 * changesetLast + 1
        
        Debug.msg(2, "Digit.Undo(): changeset_last=%d, changeset_current=%d, level=%d",
                  changesetLast, self.changesetCurrent, level)
        
        if level < 0: # undo
            if self.changesetCurrent + level < -1:
                return self.changesetCurrent
            for changeset in range(self.changesetCurrent, self.changesetCurrent + level, -1):
                self._applyChangeset(changeset, undo = True)
        elif level > 0: # redo 
            if self.changesetCurrent + level > len(self.changesets):
                return self.changesetCurrent
            for changeset in range(self.changesetCurrent, self.changesetCurrent + level):
                self._applyChangeset(changeset, undo = False)
        
        self.changesetCurrent += level
        
        Debug.msg(2, "Digit.Undo(): changeset_current=%d, changeset_last=%d",
                  self.changesetCurrent, changesetLast)
        
        self.mapWindow.UpdateMap(render = False)
        
        if self.changesetCurrent < 0: # disable undo tool
            self.toolbar.EnableUndo(False)
        else:
            self.toolbar.EnableUndo(True)
        
        if self.changesetCurrent <= changesetLast:
            self.toolbar.EnableRedo(True)
        else:
            self.toolbar.EnableRedo(False)
        
    def ZBulkLines(self, pos1, pos2, start, step):
        """!Z-bulk labeling

        @param pos1 reference line (start point)
        @param pos1 reference line (end point)
        @param start starting value
        @param step step value

        @return number of modified lines
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_bulk_labeling(self.poMapInfo, poList,
                                  pos1[0], pos1[1], pos2[0], pos2[1],
                                  start, step)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addChangeset()
            self.toolbar.EnableUndo()
        
        return ret
    
    def GetDisplay(self):
        """!Get display driver instance"""
        return self._display
    
    def OpenMap(self, name, tmp = False):
        """!Open vector map for editing
        
        @param map name of vector map to be set up
        @param tmp True to open temporary vector map
        """
        Debug.msg (3, "AbstractDigit.SetMapName map=%s" % name)

        if '@' in name:
            name, mapset = name.split('@')
        else:
            mapset = grass.gisenv()['MAPSET']
        
        self.poMapInfo = self._display.OpenMap(str(name), str(mapset), True, tmp)
        
        if self.poMapInfo:
            self.InitCats()
        
        return self.poMapInfo
    
    def CloseMap(self):
        """!Close currently open vector map
        """
        if not self._checkMap():
            return
        
        # print extra line before building message
        sys.stdout.write(os.linesep)
        # build topology, close map
        self._display.CloseMap()

    def InitCats(self):
        """!Initialize categories information
        
        @return 0 on success
        @return -1 on error
        """
        self.cats.clear()
        if not self._checkMap():
            return -1
        
        ndblinks = Vect_get_num_dblinks(self.poMapInfo)
        for i in range(ndblinks):
            fi = Vect_get_dblink(self.poMapInfo, i).contents
            if fi:
                self.cats[fi.number] = None
        
        # find max category
        nfields = Vect_cidx_get_num_fields(self.poMapInfo)
        Debug.msg(2, "wxDigit.InitCats(): nfields=%d", nfields)
        
        for i in range(nfields):
            field = Vect_cidx_get_field_number(self.poMapInfo, i)
            ncats = Vect_cidx_get_num_cats_by_index(self.poMapInfo, i)
            if field <= 0:
                continue
            for j in range(ncats):
                cat = c_int()
                type = c_int()
                id = c_int()
                Vect_cidx_get_cat_by_index(self.poMapInfo, i, j,
                                           byref(cat), byref(type), byref(id))
                if field in self.cats:
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
        
    def _checkMap(self):
        """!Check if map is open
        """
        if not self.poMapInfo:
            self._error.NoMap()
            return False
        
        return True

    def _addFeature(self, ftype, coords, layer, cat, snap, threshold):
        """!Add new feature(s) to the vector map

        @param ftype feature type (GV_POINT, GV_LINE, GV_BOUNDARY, ...)
        @coords tuple of coordinates ((x, y), (x, y), ...)
        @param layer layer number (-1 for no cat)
        @param cat category number
        @param snap snap to node/vertex
        @param threshold threshold for snapping
        
        @return tuple (number of added features, list of fids)
        @return number of features -1 on error
        """
        fids = list()
        if not self._checkMap():
            return (-1, None)
        
        is3D = bool(Vect_is_3d(self.poMapInfo))
        
        Debug.msg(2, "IVDigit._addFeature(): npoints=%d, layer=%d, cat=%d, snap=%d",
                  len(coords), layer, cat, snap)
        
        if not (ftype & (GV_POINTS | GV_LINES | GV_AREA)): # TODO: 3D
            self._error.FeatureType(ftype)
            return (-1, None)
        
        # set category
        Vect_reset_cats(self.poCats)
        if layer > 0 and ftype != GV_AREA:
            Vect_cat_set(self.poCats, layer, cat)
            self.cats[layer] = max(cat, self.cats.get(layer, 1))
        
        # append points
        Vect_reset_line(self.poPoints)
        for c in coords:
            Vect_append_point(self.poPoints, c[0], c[1], 0.0)
        
        if ftype & (GV_BOUNDARY | GV_AREA):
            # close boundary
            points = self.poPoints.contents
            last = points.n_points - 1
            if self._settings['closeBoundary']:
                Vect_append_point(self.poPoints, points.x[0], points.y[0], points.z[0])
            elif Vect_points_distance(points.x[0], points.y[0], points.z[0],
                                      points.x[last], points.y[last], points.z[last],
                                      is3D) <= threshold:
                points.x[last] = points.x[0]
                points.y[last] = points.y[0]
                points.z[last] = points.z[0]
        
        if snap != NO_SNAP:
            # apply snapping (node or vertex)
            modeSnap = not (snap == SNAP)
            Vedit_snap_line(self.poMapInfo, self.popoBgMapInfo, int(self.poBgMapInfo is not None),
                            -1, self.poPoints, threshold, modeSnap)
        
        if ftype == GV_AREA:
            ltype = GV_BOUNDARY
        else:
            ltype = ftype
        newline = Vect_write_line(self.poMapInfo, ltype, self.poPoints, self.poCats)
        if newline < 0:
            self._error.WriteLine()
            return (-1, None)
        else:
            fids.append(newline)
        
        left = right = -1
        if ftype & GV_AREA:
            # add centroids for left/right area
            bpoints = Vect_new_line_struct()
            cleft = c_int()
            cright = c_int()
            
            Vect_get_line_areas(self.poMapInfo, newline,
                                byref(cleft), byref(cright))
            left = cleft.value
            right = cright.value
            
            Debug.msg(3, "IVDigit._addFeature(): area - left=%d right=%d",
                      left, right)
            
            # check if area exists and has no centroid inside
            if layer > 0 and (left > 0 or right > 0):
                Vect_cat_set(self.poCats, layer, cat)
                self.cats[layer] = max(cat, self.cats.get(layer, 0))
            
            x = c_double()
            y = c_double()
            if left > 0 and \
                    Vect_get_area_centroid(self.poMapInfo, left) == 0:
                # if Vect_get_area_points(self.poMapInfo, left, bpoints) > 0 and
                # Vect_find_poly_centroid(bpoints, byref(x), byref(y)) == 0:
                if Vect_get_point_in_area(self.poMapInfo, left, byref(x), byref(y)) == 0:
                    Vect_reset_line(bpoints)
                    Vect_append_point(bpoints, x.value, y.value, 0.0)
                    newline = Vect_write_line(self.poMapInfo, GV_CENTROID,
                                              bpoints, self.poCats)
                    if newline < 0:
                        self._error.WriteLine()
                        return (len(fids), fids)
                    else:
                        fids.append(newline)
                    
            if right > 0 and \
                    Vect_get_area_centroid(self.poMapInfo, right) == 0:
                # if Vect_get_area_points(byref(self.poMapInfo), right, bpoints) > 0 and 
                # Vect_find_poly_centroid(bpoints, byref(x), byref(y)) == 0:
                if Vect_get_point_in_area(self.poMapInfo, right, byref(x), byref(y)) == 0:
                    Vect_reset_line(bpoints)
                    Vect_append_point(bpoints, x.value, y.value, 0.0)
                    newline =  Vect_write_line(self.poMapInfo, GV_CENTROID,
                                               bpoints, self.poCats)
                    if newline < 0:
                        self._error.WriteLine()
                        return (len(fids, fids))
                    else:
                        fids.append(newline)
                    
            Vect_destroy_line_struct(bpoints)
        
        # break at intersection
        if self._settings['breakLines']:
            self._breakLineAtIntersection(newline, self.poPoints)

        self._addChangeset()
        
        return (len(fids), fids)
    
    def _ModifyLineVertex(self, coords, add = True):
        """!Add or remove vertex
        
        Shape of line/boundary is not changed when adding new vertex.
        
        @param coords coordinates of point
        @param add True to add, False to remove
        
        @return 1 on success
        @return 0 nothing changed
        @return -1 error
        """
        if not self._checkMap():
            return -1
        
        selected = self._display.selected
        if len(selected['ids']) != 1:
            return 0
        
        poList  = self._display.GetSelectedIList()

        if self.emit_signals:
            cList = poList.contents
            
            old_bboxs = [self._getBbox(cList.value[0])]
            old_areas_cats = [self._getLineAreasCategories(cList.value[0])]

            Vect_set_updated(self.poMapInfo, 1)
            n_up_lines_old = Vect_get_num_updated_lines(self.poMapInfo)

        Vect_reset_line(self.poPoints)
        Vect_append_point(self.poPoints, coords[0], coords[1], 0.0)
        
        thresh = self._display.GetThreshold(type = 'selectThresh')
        
        if add:
            ret = Vedit_add_vertex(self.poMapInfo, poList,
                                   self.poPoints, thresh)
        else:
            ret = Vedit_remove_vertex(self.poMapInfo, poList,
                                      self.poPoints, thresh)

        Vect_destroy_list(poList)

        if ret > 0 and self.emit_signals:
            new_bboxs = []
            new_areas_cats = []

            n_up_lines = Vect_get_num_updated_lines(self.poMapInfo)
            for i in range(n_up_lines_old, n_up_lines):
                new_id = Vect_get_updated_line(self.poMapInfo, i)
                new_areas_cats.append(self._getLineAreasCategories(new_id))
                new_bboxs.append(self._getBbox(new_id))
        
        if not add and ret > 0 and self._settings['breakLines']:
            self._breakLineAtIntersection(Vect_get_num_lines(self.poMapInfo),
                                          None)

        if ret > 0:
            self._addChangeset()

        if ret > 0 and self.emit_signals:
            if add:
                self.vertexAdded.emit(old_bboxs=old_bboxs, new_bboxs=new_bboxs)
            else:
                self.vertexRemoved.emit(old_bboxs=old_bboxs, 
                                        new_bboxs=new_bboxs,
                                        old_areas_cats=old_areas_cats,
                                        new_areas_cats=new_areas_cats)

        return 1
    
    def GetLineCats(self, line):
        """!Get list of layer/category(ies) for selected feature.

        @param line feature id (-1 for first selected feature)

        @return list of layer/cats
        """
        ret = dict()
        if not self._checkMap():
            return ret
        
        if line == -1 and len(self._display.selected['ids']) < 1:
            return ret
        
        if line == -1:
            line = self._display.selected['ids'][0]
            
        if not Vect_line_alive(self.poMapInfo, line):
            self._error.DeadLine(line)
            return ret
        
        if Vect_read_line(self.poMapInfo, None, self.poCats, line) < 0:
            self._error.ReadLine(line)
            return ret
        
        cats = self.poCats.contents
        for i in range(cats.n_cats):
            field = cats.field[i]
            if field not in ret:
                ret[field] = list()
            ret[field].append(cats.cat[i])
        
        return ret

    def GetLayers(self):
        """!Get list of layers
        
        Requires self.InitCats() to be called.

        @return list of layers
        """
        return self.cats.keys()
    
    def UpdateSettings(self):
        """!Update digit (and display) settings
        """
        self._display.UpdateSettings()
        
        self._settings['breakLines']   = bool(UserSettings.Get(group = 'vdigit', key = "breakLines",
                                                              subkey = 'enabled'))
        self._settings['closeBoundary'] = bool(UserSettings.Get(group = 'vdigit', key = "closeBoundary",
                                                                subkey = 'enabled'))
        
    def SetCategory(self):
        """!Update self.cats based on settings"""
        sel = UserSettings.Get(group = 'vdigit', key = 'categoryMode', subkey = 'selection')
        cat = None
        if sel == 0: # next to usep
            cat = self._setCategoryNextToUse()
        elif sel == 1:
            cat = UserSettings.Get(group = 'vdigit', key = 'category', subkey = 'value')
        
        if cat:
            layer = UserSettings.Get(group = 'vdigit', key = 'layer', subkey = 'value')
            self.cats[layer] = cat
        
        return cat
    
    def _setCategoryNextToUse(self):
        """!Find maximum category number for the given layer and
        update the settings

        @return category to be used
        """
        # get max category number for given layer and update the settings
        layer = UserSettings.Get(group = 'vdigit', key = 'layer', subkey = 'value')
        cat = self.cats.get(layer, 0) + 1
        UserSettings.Set(group = 'vdigit', key = 'category', subkey = 'value',
                         value = cat)
        Debug.msg(1, "IVDigit._setCategoryNextToUse(): cat=%d", cat)
        
        return cat

    def SelectLinesFromBackgroundMap(self, bbox):
        """!Select features from background map

        @param bbox bounding box definition
        
        @return list of selected feature ids
        """
        # try select features by box first
        if self._display.SelectLinesByBox(bbox, poMapInfo = self.poBgMapInfo) < 1:
            self._display.SelectLineByPoint(bbox[0], poMapInfo = self.poBgMapInfo)['line']
            
        return self._display.selected['ids']
        
    def GetUndoLevel(self):
        """!Get undo level (number of active changesets)
        
        Note: Changesets starts wiht 0
        """
        return self.changesetCurrent
    
    def GetFeatureType(self):
        """!Get feature type for OGR layers

        @return feature type as string (point, linestring, polygon)
        @return None for native format
        """
        topoFormat = Vect_get_finfo_topology_info(self.poMapInfo, None, None, None)
        if topoFormat == GV_TOPO_PSEUDO:
            return Vect_get_finfo_geometry_type(self.poMapInfo)
        
        return ''
        
