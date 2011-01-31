"""!
@package wxvdigit.py

@brief wxGUI vector digitizer (base class)

Code based on wxVdigit C++ component from GRASS 6.4.0
(gui/wxpython/vdigit). Converted to Python in 2010/12-2011/01.

List of classes:
 - VDigitError
 - IVDigit

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

from gcmd        import GError
from debug       import Debug
from preferences import globalSettings as UserSettings

from wxvdriver   import DisplayDriver

from grass.lib.grass  import *
from grass.lib.vector import *
from grass.lib.vedit  import *
from grass.lib.dbmi   import *

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
        GError(message + ' ' + _('Operation cancelled.'),
               parent  = self.parent,
               caption = self.caption)

    def WriteLine(self):
        """!Writing line failed
        """
        GError(message = _('Writing new feature failed. '
                           'Operation cancelled.'),
               parent  = self.parent,
               caption = self.caption)

    def ReadLine(self, line):
        """!Reading line failed
        """
        GError(message = _('Reading feature id %d failed. '
                           'Operation cancelled.') % line,
               parent  = self.parent,
               caption = self.caption)

    def DbLink(self, dblink):
        """!No dblink available
        """
        GError(message = _('Database link %d not available. '
                           'Operation cancelled.') % dblink,
               parent  = self.parent,
               caption = self.caption)

    def Driver(self, driver):
        """!Staring driver failed
        """
        GError(message = _('Unable to start database driver <%s>. '
                           'Operation cancelled.') % driver,
               parent  = self.parent,
               caption = self.caption)

    def Database(self, driver, database):
        """!Opening database failed
        """
        GError(message = _('Unable to open database <%s> by driver <%s>. '
                           'Operation cancelled.') % (database, driver),
               parent  = self.parent,
               caption = self.caption)

    def DbExecute(self, sql):
        """!Sql query failed
        """
        GError(message = _("Unable to execute SQL query '%s'. "
                           "Operation cancelled.") % sql,
               parent  = self.parent,
               caption = self.caption)

    def DeadLine(self, line):
        """!Dead line
        """
        GError(message = _("Feature id %d is marked as dead. "
                           "Operation cancelled.") % line,
               parent  = self.parent,
               caption = self.caption)

    def FeatureType(self, ftype):
        """!Unknown feature type
        """
        GError(message = _("Unsupported feature type %d. "
                           "Operation cancelled.") % ftype,
               parent  = self.parent,
               caption = self.caption)
        
class IVDigit:
    def __init__(self, mapwindow):
        """!Base class for vector digitizer (ctypes interface)
        
        @parem mapwindow reference for map window (BufferedWindow)
        """
        self.poMapInfo   = None      # pointer to Map_info
        self.mapWindow = mapwindow

        # background map
        self.bgMapInfo   = Map_info()
        self.poBgMapInfo = self.popoBgMapInfo = None
        
        if not mapwindow.parent.IsStandalone():
            goutput = mapwindow.parent.GetLayerManager().GetLogWindow()
            log = goutput.GetLog(err = True)
            progress = goutput.GetProgressBar()
        else:
            log = sys.stderr
            progress = None
        
        self.toolbar = mapwindow.parent.toolbars['vdigit']
        
        self._error   = VDigitError(parent = self.mapWindow)
        
        self._display = DisplayDriver(device    = mapwindow.pdcVector,
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
        self.changesets = dict()
        self.changesetCurrent = -1 # first changeset to apply
        self.changesetEnd     = -1 # last changeset to be applied
        
        if self.poMapInfo:
            self.InitCats()
        
    def __del__(self):
        Vect_destroy_line_struct(self.poPoints)
        self.poPoints = None
        Vect_destroy_cats_struct(self.poCats)
        self.poCats = None
        
        if self.poBgMapInfo:
            Vect_close(self.poBgMapInfo)
            self.poBgMapInfo = self.popoBgMapInfo = None
            del self.bgMapInfo
        
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
    
    def _breakLineAtIntersection(self, line, pointsLine, changeset):
        """!Break given line at intersection

        \param line line id
        \param pointsLine line geometry
        \param changeset id
  
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
        
        listLine  = Vect_new_list()
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
            lineBreak = listLine.contents.value[i]
            if lineBreak == line:
		continue
            
            ltype = Vect_read_line(self.poMapInfo, pointsCheck, None, lineBreak)
            if not (ltype & GV_LINES):
                continue
            
            if Vect_line_check_intersection(self.poPoints, pointsCheck,
                                            WITHOUT_Z):
                Vect_list_append(listBreak, lineBreak)
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        for i in range(listBreak.contents.n_values):
            self._addActionToChangeset(changeset, listBreak.contents.value[i], add = False)
        
        ret = Vect_break_lines_list(self.poMapInfo, listBreak, listRef,
                                    GV_LINES, None)
        
        for i in range(listBreak.contents.n_values):
            if Vect_line_alive(self.poMapInfo, listBreak.contents.value[i]):
                self._removeActionFromChangeset(changeset, listBreak.contents.value[i],
                                                add = False)
        
        for line in range(nlines + 1, Vect_get_num_lines(self.poMapInfo) + 1):
            self._addActionToChangeset(changeset, line, add = True)
        
        Vect_destroy_line_struct(pointsCheck)

        Vect_destroy_list(listLine)
        Vect_destroy_list(listBreak)
        Vect_destroy_list(listRef)
        
        return ret
    
    def _addActionsBefore(self):
        """!Register action before operation
  
        @return changeset id
        """
        changeset = len(self.changesets)
        for line in self._display.selected['ids']:
            if Vect_line_alive(self.poMapInfo, line):
                self._addActionToChangeset(changeset, line, add = False)
        
        return changeset

    def _applyChangeset(self, changeset, undo):
        """!Apply changeset (undo/redo changeset)
        
        @param changeset changeset id
        @param undo True for undo otherwise redo

        @return 1 changeset applied
        @return 0 changeset not applied
        @return -1 on error
        """
        if changeset < 0 or changeset > len(self.changesets.keys()):
            return -1
        
        if self.changesetEnd < 0:
            self.changesetEnd = changeset
            
        ret = 0
        actions = self.changesets[changeset]
        for action in actions: 
            add = action['add']
            line = action['line']
            if (undo and add) or \
                    (not undo and not add):
                if Vect_line_alive(self.poMapInfo, line):
                    Debug.msg(3, "IVDigit._applyChangeset(): changeset=%d, action=add, line=%d -> deleted",
                              changeset, line)
                    Vect_delete_line(self.poMapInfo, line)
                    ret = 1
                else:
                    Debug.msg(3, "Digit.ApplyChangeset(): changeset=%d, action=add, line=%d dead",
                              changeset, line)
            else: # delete
                offset = action['offset']
                if not Vect_line_alive(self.poMapInfo, line):
                    Debug.msg(3, "Digit.ApplyChangeset(): changeset=%d, action=delete, line=%d -> added",
                              changeset, line)
                    if Vect_restore_line(self.poMapInfo, line, offset) < 0:
                        return -1
                    ret = 1
                else:
                    Debug.msg(3, "Digit.ApplyChangeset(): changeset=%d, action=delete, line=%d alive",
                              changeset, line)
        
        return ret
    
    def _addActionsAfter(self, changeset, nlines):
        """!Register action after operation

        @param changeset changeset id
        @param nline number of lines
        """
        for line in self._display.selected['ids']:
            if Vect_line_alive(self.poMapInfo, line):
                self._removeActionFromChangeset(changeset, line, add = False)
        
        for line in range(nlines + 1, Vect_get_num_lines(self.poMapInfo)):
            if Vect_line_alive(self.poMapInfo, line):
                self._addActionToChangeset(changeset, line, add = True)
        
    def _addActionToChangeset(self, changeset, line, add):
        """!Add action to changeset
        
        @param changeset id of changeset
        @param line feature id
        @param add True to add, otherwise delete
        """
        if not self._checkMap():
            return 
        
        if not Vect_line_alive(self.poMapInfo, line):
            return
        
        offset = Vect_get_line_offset(self.poMapInfo, line)
        
        if not self.changesets.has_key(changeset):
            self.changesets[changeset] = list()
            self.changesetCurrent = changeset
        
        self.changesets[changeset].append({ 'add'    : add,
                                            'line'   : line,
                                            'offset' : offset })
        
        Debug.msg(3, "IVDigit._addActionToChangeset(): changeset=%d, type=%d, line=%d, offset=%d",
                  changeset, type, line, offset)
        
    def _removeActionFromChangeset(self, changeset, line, add):
        """!Remove action from changeset
        
        @param changeset changeset id
        @param line line id
        @param add True for add, False for delete
        
        @return number of actions in changeset
        @return -1 on error
        """
        if changeset not in self.changesets.keys():
            return -1
        
        alist = self.changesets[changeset] 
        for action in alist:
            if action['add'] == add and action['line'] == line:
                alist.remove(action)
        
        return len(alist)

    def AddFeature(self, ftype, points):
        """!Add new feature
        
        @param ftype feature type (point, line, centroid, boundary)
        @param points tuple of points ((x, y), (x, y), ...)
        
        @return new feature id
        """
        if UserSettings.Get(group = 'vdigit', key = "categoryMode", subkey = 'selection') == 2:
            layer = -1 # -> no category
            cat   = -1
        else:
            layer = UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value')
            cat   = self.SetCategory()
        
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
            return
        
        if vtype & GV_LINES and len(points) < 2:
            GError(parent = self.mapWindow,
                   message = _("Not enough points for line"))
            return
        
        self.toolbar.EnableUndo()
        
        return self._addFeature(vtype, points, layer, cat,
                                self._getSnapMode(), self._display.GetThreshold())
    
    def DeleteSelectedLines(self):
        """!Delete selected features

        @return number of deleted features
        """
        deleteRec = UserSettings.Get(group = 'vdigit', key = 'delRecord', subkey = 'enabled')
        if not self._checkMap():
            return -1
        
        n_dblinks = Vect_get_num_dblinks(self.poMapInfo)
        Cats_del = None
        
        # collect categories for delete if requested
        if deleteRec:
            poCats    = Vect_new_cats_struct()
            poCatsDel = Vect_new_cats_struct()
            for i in self._display.selected['ids']:
                if Vect_read_line(self.poMapInfo, None, poCats, i) < 0:
                    Vect_destroy_cats_struct(poCatsDel)
                    self._error.ReadLine(i)
                    return -1
                
                cats = poCats.contents
                for j in range(cats.n_cats):
                    Vect_cat_set(poCatsDel, cats.field[j], cats.cat[j])
            
            Vect_destroy_cats_struct(poCats)
        
        # register changeset
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        nlines = Vedit_delete_lines(self.poMapInfo, poList)
        Vect_destroy_list(poList)
        self._display.selected['ids'] = list()
        
        if nlines > 0 and deleteRec:
            handle  = dbHandle()
            poHandle = pointer(handle)
            stmt    = dbString()
            poStmt   = pointer(stmt)
            
            for dblink in range(n_dblinks):
                poFi = Vect_get_dblink(self.poMapInfo, dblink)
                if poFi is None:
                    self._error.DbLink(dblink)
                    return -1
                
                Fi = poFi.contents
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
                n_cats = 0;
                catsDel = poCatsDel.contents
                for c in range(catsDel.n_cats):
                    if catsDel.field[c] == Fi.number:
                        if n_cats > 0:
                            db_append_string(poStmt, " or")
                    
		    db_append_string(poStmt, " %s = %d" % (Fi.key, catsDel.cat[c]))
		    n_cats += 1
                
                Vect_cat_del(poCatsDel, Fi.number)
                
                if n_cats and \
                        db_execute_immediate(poDriver, poStmt) != DB_OK:
                    self._error.DbExecute(db_get_string(poStmt))
                    return -1
                
                db_close_database(poDriver)
                db_shutdown_driver(poDriver)
        
        if poCatsDel:
            Vect_destroy_cats_struct(poCatsDel)
        
        if nlines > 0:
            self.toolbar.EnableUndo()
        
        return nlines

    def MoveSelectedLines(self, move):
        """!Move selected features

        @param move direction (x, y)
        """
        if not self._checkMap():
            return -1
        
        thresh = self._display.GetThreshold()
        snap   = self._getSnapMode()
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        # register changeset
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        nlines = Vedit_move_lines(self.poMapInfo, self.popoBgMapInfo, int(self.poBgMapInfo is not None),
                                  poList,
                                  move[0], move[1], 0,
                                  snap, thresh)
        Vect_destroy_list(poList)
        
        if nlines > 0:
            self._addActionsAfter(changeset, nlines)
        else:
            del self.changesets[changeset]
        
        if nlines > 0 and self._settings['breakLines']:
            for i in range(1, nlines):
                self._breakLineAtIntersection(nlines + i, None, changeset)
        
        if nlines > 0:
            self.toolbar.EnableUndo()
        
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
        
        Vect_reset_line(self.poPoints)
        Vect_append_point(self.poPoints, point[0], point[1], 0.0)
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        changeset = self._addActionsBefore()
        
        # move only first found vertex in bbox 
        poList = self._display.GetSelectedIList()
        moved = Vedit_move_vertex(self.poMapInfo, self.popoBgMapInfo, int(self.poBgMapInfo is not None),
                                  poList, self.poPoints,
                                  self._display.GetThreshold(type = 'selectThresh'),
                                  self._display.GetThreshold(),
                                  move[0], move[1], 0.0,
                                  1, self._getSnapMode())
        Vect_destroy_list(poList)
        
        if moved > 0:
            self._addActionsAfter(changeset, nlines)
        else:
            del self.changesets[changeset]
        
        if moved > 0 and self._settings['breakLines']:
            self._breakLineAtIntersection(Vect_get_num_lines(self.poMapInfo),
                                          None, changeset)
        
        if moved > 0:
            self.toolbar.EnableUndo()
        
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
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        changeset = self._addActionsBefore()
        
        ret = Vedit_split_lines(self.poMapInfo, poList,
                                self.poPoints, thresh, poList)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addActionsAfter(changeset, nlines)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]
        
        return ret

    def EditLine(self, line, coords):
        """!Edit existing line/boundary

        @param line id of line to be modified
        @param coords list of coordinates of modified line

        @return feature id of new line
        @return -1 on error
        """
        if self._checkMap():
            return -1
        
        try:
            lineid = line[0]
        except:
            lineid = -1
        
        if len(coords) < 2:
            self.DeleteSelectedLines()
            return 0
        
        ret = self.digit.RewriteLine(lineid, listCoords,
                                     bgmap, self._getSnapMode(),
                                     self._display.GetThreshold())
        
        if ret > 0:
            self.toolbar.EnableUndo()
        
        return ret

    def FlipLine(self):
        """!Flip selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        # register changeset
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_flip_lines(self.poMapInfo, poList)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addActionsAfter(changeset, nlines)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]
        
        return ret

    def MergeLine(self):
        """!Merge selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_merge_lines(self.poMapInfo, poList)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addActionsAfter(changeset, nlines)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]
                
        return ret

    def BreakLine(self):
        """!Break selected lines/boundaries

        @return number of modified lines
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        ret = Vect_break_lines_list(self.poMapInfo, poList, None,
                                    GV_LINES, None)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addActionsAfter(changeset, nlines)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]
                
        return ret

    def SnapLine(self):
        """!Snap selected lines/boundaries

        @return on success
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        Vect_snap_lines_list(self.poMapInfo, poList,
                             self._display.GetThreshold(), None)
        Vect_destroy_list(poList)
        
        if nlines < Vect_get_num_lines(self.poMapInfo):
            self._addActionsAfter(changeset, nlines)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]
        
    def ConnectLine(self):
        """!Connect selected lines/boundaries

        @return 1 lines connected
        @return 0 lines not connected
        @return -1 on error
        """
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        # register changeset
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_connect_lines(self.poMapInfo, poList,
                                  self._display.GetThreshold())
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addActionsAfter(changeset, nlines)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]
        
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
        
        if ret > 0:
            changeset = len(self.changesets)
            for line in (range(nlines + 1, Vect_get_num_lines(self.poMapInfo))):
                self._addActionToChangeset(changeset, line, add = True)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]

        if ret > 0 and self.poBgMapInfo and self._settings['breakLines']:
            for i in range(1, ret):
                self._breakLineAtIntersection(nlines + i, None, changeset)
        
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
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        # register changeset
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_chtype_lines(self.poMapInfo, poList)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addActionsAfter(changeset, nlines)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]
        
        return ret

    def Undo(self, level = -1):
        """!Undo action

        @param level levels to undo (0 to revert all)

        @return id of current changeset
        """
        changesetLast = len(self.changesets.keys()) - 1

        if changesetLast < 0:
            return changesetLast
        
        if self.changesetCurrent == -2: # value uninitialized 
            self.changesetCurrent = changesetLast
            
        if level > 0 and self.changesetCurrent < 0:
            self.changesetCurrent = 0
        
        if level == 0:
            # 0 -> undo all
            level = -1 * changesetLast + 1

        Debug.msg(2, "Digit.Undo(): changeset_last=%d, changeset_current=%d, level=%d",
                  changesetLast, self.changesetCurrent, level)
    
        if level < 0: # undo
            if self.changesetCurrent + level < -1:
                return changesetCurrent;
            for changeset in range(self.changesetCurrent, self.changesetCurrent + level, -1):
                self._applyChangeset(changeset, undo = True)
        elif level > 0: # redo 
            if self.changesetCurrent + level > len(self.changesets.keys()):
                return self.changesetCurrent
            for changeset in range(self.changesetCurrent, self.changesetCurrent + level):
                self._applyChangeset(changeset, undo = False)
        
        self.changesetCurrent += level

        Debug.msg(2, "Digit.Undo(): changeset_current=%d, changeset_last=%d, changeset_end=%d",
                  self.changesetCurrent, changesetLast, self.changesetEnd)
        
        if self.changesetCurrent == self.changesetEnd:
            self.changesetEnd = changesetLast
            return -1
        
        self.mapWindow.UpdateMap(render = False)
        
        if self.changesetCurrent < 0: # disable undo tool
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
        if not self._checkMap():
            return -1
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        
        # register changeset
        changeset = self._addActionsBefore()
        
        poList = self._display.GetSelectedIList()
        ret = Vedit_bulk_labeling(self.poMapInfo, poList,
                                  pos1[0], pos1[1], pos2[0], pos2[1],
                                  start, step)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addActionsAfter(changeset, nlines)
            self.toolbar.EnableUndo()
        else:
            del self.changesets[changeset]
        
        return ret
    
    def GetDisplay(self):
        """!Get display driver instance"""
        return self._display
    
    def OpenMap(self, name):
        """!Open vector map for editing
        
        @param map name of vector map to be set up
        """
        Debug.msg (3, "AbstractDigit.SetMapName map=%s" % name)
        
        name, mapset = name.split('@')
        
        self.poMapInfo = self._display.OpenMap(str(name), str(mapset), True)
        
        if self.poMapInfo:
            self.InitCats()
        
        return self.poMapInfo
    
    def CloseMap(self):
        """!Close currently open vector map
        """
        if not self._checkMap():
            return
        
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
        
    def _checkMap(self):
        """!Check if map is open
        """
        if not self.poMapInfo:
            self._error.NoMap()
            return False
        
        return True

    def _addFeature(self, ftype, coords, layer, cat, snap, threshold):
        """!Add new feature to the vector map

        @param ftype feature type (GV_POINT, GV_LINE, GV_BOUNDARY, ...)
        @coords tuple of coordinates ((x, y), (x, y), ...)
        @param layer layer number (-1 for no cat)
        @param cat category number
        @param snap snap to node/vertex
        @param threshold threshold for snapping
        
        @return -1 on error
        @return feature id of new feature
        """
        if not self._checkMap():
            return -1
        
        is3D = bool(Vect_is_3d(self.poMapInfo))
        
        Debug.msg(2, "IVDigit._addFeature(): npoints=%d, layer=%d, cat=%d, snap=%d",
                  len(coords), layer, cat, snap)
        
        if not (ftype & (GV_POINTS | GV_LINES | GV_AREA)): # TODO: 3D
            self._error.FeatureType(ftype)
            return -1
        
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
            if Vect_points_distance(points.x[0], points.x[0], points.z[0],
                                    points.x[last], points.x[last], points.z[last],
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
            return -1
        
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
            
            # check if area exists and has no centroid inside
            if layer > 0 and (left > 0 or right > 0):
                Vect_cat_set(self.poCats, layer, cat)
                self.cats[layer] = max(cat, self.cats.get(layer, 0))
            
            x = c_double()
            y = c_double()
            if left > 0 and \
                    Vect_get_area_centroid(self.poMapInfo, left) == 0:
                if Vect_get_area_points(self.poMapInfo, left, bpoints) > 0 and \
                        Vect_find_poly_centroid(bpoints, byref(x), byref(y)) == 0:
                    Vect_reset_line(bpoints)
                    Vect_append_point(bpoints, x.value, y.value, 0.0)
                    if Vect_write_line(self.poMapInfo, GV_CENTROID,
                                       bpoints, self.poCats) < 0:
                        self._error.WriteLine()
                        return -1
            
            if right > 0 and \
                    Vect_get_area_centroid(self.poMapInfo, right) == 0:
                if Vect_get_area_points(byref(self.poMapInfo), right, bpoints) > 0 and \
                        Vect_find_poly_centroid(bpoints, byref(x), byref(y)) == 0:
                    Vect_reset_line(bpoints)
                    Vect_append_point(bpoints, x.value, y.value, 0.0)
                    if Vect_write_line(byref(self.poMapInfo), GV_CENTROID,
                                       bpoints, self.poCats) < 0:
                        self._error.WriteLine()
                        return -1
            Vect_destroy_line_struct(bpoints)
        
        # register changeset
        changeset = len(self.changesets)
        self._addActionToChangeset(changeset, newline, add = True)
        
        # break at intersection
        if self._settings['breakLines']:
            self._breakLineAtIntersection(newline, self.poPoints, changeset)
        
        return newline
    
    def _ModifyLineVertex(self, coords, add = True):
        """!Add or remove vertex
        
        Shape of line/boundary is not changed when adding new vertex.
        
        @param coords coordinates of point
        @param add True to add, False to remove
        
        @return id id of the new feature
        @return 0 nothing changed
        @return -1 error
        """
        if not self._checkMap():
            return -1
        
        selected = self._display.selected
        if len(selected['ids']) != 1:
            return 0
        
        poList  = self._display.GetSelectedIList()
        Vect_reset_line(self.poPoints)
        Vect_append_point(self.poPoints, coords[0], coords[1], 0.0)
        
        nlines = Vect_get_num_lines(self.poMapInfo)
        thresh = self._display.GetThreshold(type = 'selectThresh')
        
        changeset = self._addActionsBefore()
        
        if add:
            ret = Vedit_add_vertex(self.poMapInfo, poList,
                                   self.poPoints, thresh)
        else:
            ret = Vedit_remove_vertex(self.poMapInfo, poList,
                                      self.poPoints, thresh)
        Vect_destroy_list(poList)
        
        if ret > 0:
            self._addActionsAfter(changeset, nlines)
        else:
            del self.changesets[changeset]
        
        if not add and ret > 0 and self._settings['breakLines']:
            self._breakLineAtIntersection(Vect_get_num_lines(self.poMapInfo),
                                          None, changeset)
                
        return nlines + 1 # feature is write at the end of the file
    
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
        
        self._settings['breakLines']  = bool(UserSettings.Get(group = 'vdigit', key = "breakLines",
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
        ret = list()
        
        # try select features by Box
        ids = self._display.SelectLinesByBox(bbox, poMapInfo = self.poBgMapInfo)
        if not ids:
            ids = [self._display.SelectLineByPoint(bbox[0], poMapInfo = self.poBgMapInfo)['line'], ]
        
        return ids

    def GetUndoLevel(self):
        """!Get undo level (number of active changesets)
        
        Note: Changesets starts wiht 0
        """
        return self.changesetCurrent
    
