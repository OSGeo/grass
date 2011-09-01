"""!
@package mapdisp_vdigit.py

@brief Map display canvas extended for vector digitizer

See also vdigit.py, wxvdriver.py and wxvdigit.py

Classes:
 - VDigitWindow

(C) 2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import wx

import dbm_dialogs

import gcmd
from debug import Debug
from mapdisp_window import BufferedWindow
from preferences import globalSettings as UserSettings

from vdigit import VDigitCategoryDialog
from vdigit import VDigitZBulkDialog
from vdigit import VDigitDuplicatesDialog

class VDigitWindow(BufferedWindow):
    """!A Buffered window extended for vector digitizer.
    """
    def __init__(self, parent, id = wx.ID_ANY,
                 Map = None, tree = None, lmgr = None,
                 style = wx.NO_FULL_REPAINT_ON_RESIZE, **kwargs):
        BufferedWindow.__init__(self, parent, id, Map, tree, lmgr,
                                style, **kwargs)
        
        self.pdcVector = wx.PseudoDC()
        self.toolbar   = self.parent.toolbars['vdigit']
        self.digit     = None # wxvdigit.IVDigit
        
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyDown)
        
    def SetToolbar(self, toolbar):
        """!Set up related toolbar
        """
        self.toolbar = toolbar
        
    def _onMotion(self, coord, precision):
        """!Track mouse motion and update statusbar (see self.Motion)

        @parem coord easting, northing
        @param precision formatting precision
        """
        e, n = coord
        
        if self.toolbar.GetAction() != 'addLine' or \
                self.toolbar.GetAction('type') not in ('line', 'boundary') or \
                len(self.polycoords) == 0:
            return False
        
        # for linear feature show segment and total length
        distance_seg = self.Distance(self.polycoords[-1],
                                     (e, n), screen = False)[0]
        distance_tot = distance_seg
        for idx in range(1, len(self.polycoords)):
            distance_tot += self.Distance(self.polycoords[idx-1],
                                          self.polycoords[idx],
                                          screen = False)[0]
        self.parent.statusbar.SetStatusText("%.*f, %.*f (seg: %.*f; tot: %.*f)" % \
                                                (precision, e, precision, n,
                                                 precision, distance_seg,
                                                 precision, distance_tot), 0)
        
        return True
    
    def OnKeyDown(self, event):
        """!Key pressed"""
        shift = event.ShiftDown()
        kc = event.GetKeyCode()
        
        event = None
        if not shift:
            if kc == ord('P'):
                event = wx.CommandEvent(winid = self.toolbar.addPoint)
                tool = self.toolbar.OnAddPoint
            elif kc == ord('L'):
                event = wx.CommandEvent(winid = self.toolbar.addLine)
                tool = self.toolbar.OnAddLine
        if event:
            self.toolbar.OnTool(event)
            tool(event)
        
    def _updateMap(self):
        if not self.toolbar or \
                not self.toolbar.GetLayer():
            return
        
        # set region
        self.digit.GetDisplay().UpdateRegion()
        # re-calculate threshold for digitization tool
        # self.parent.digit.GetDisplay().GetThreshold()
        # draw map
        # self.pdcVector.Clear()
        self.pdcVector.RemoveAll()
        
        try:
            item = self.tree.FindItemByData('maplayer', self.toolbar.GetLayer())
        except TypeError:
            item = None
        
        if item and self.tree.IsItemChecked(item):
            self.redrawAll = True
            self.digit.GetDisplay().DrawMap()
        
        # translate tmp objects (pointer position)
        if self.toolbar.GetAction() == 'moveLine' and \
                hasattr(self, "moveInfo"):
            if 'beginDiff' in self.moveInfo:
                # move line
                for id in self.moveInfo['id']:
                    self.pdcTmp.TranslateId(id,
                                            self.moveInfo['beginDiff'][0],
                                            self.moveInfo['beginDiff'][1])
                del self.moveInfo['beginDiff']
        
    def OnLeftDownAddLine(self, event):
        """!Left mouse button pressed - add new feature
        """
        try:
            mapLayer = self.toolbar.GetLayer().GetName()
        except:
            return
        
        if self.toolbar.GetAction('type') in ['point', 'centroid']:
            # add new point / centroiud
            east, north = self.Pixel2Cell(self.mouse['begin'])
            nfeat, fids = self.digit.AddFeature(self.toolbar.GetAction('type'), [(east, north)])
            if nfeat < 1:
                return
            
            self.UpdateMap(render = False) # redraw map
            
            # add new record into atribute table
            if UserSettings.Get(group = 'vdigit', key = "addRecord", subkey = 'enabled'):
                # select attributes based on layer and category
                cats = { fids[0] : {
                        UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value') :
                            (UserSettings.Get(group = 'vdigit', key = "category", subkey = 'value'), )
                        }}
                
                posWindow = self.ClientToScreen((self.mouse['end'][0] + self.dialogOffset,
                                                 self.mouse['end'][1] + self.dialogOffset))
                
                addRecordDlg = dbm_dialogs.DisplayAttributesDialog(parent = self, map = mapLayer,
                                                                   cats = cats,
                                                                   pos = posWindow,
                                                                   action = "add")
                
                if self.toolbar.GetAction('type') == 'centroid':
                    for fid in fids:
                        self._geomAttrb(fid, addRecordDlg, 'area')
                        self._geomAttrb(fid, addRecordDlg, 'perimeter')
                
                if addRecordDlg.mapDBInfo and \
                        addRecordDlg.ShowModal() == wx.ID_OK:
                    sqlfile = tempfile.NamedTemporaryFile(mode = "w")
                    for sql in addRecordDlg.GetSQLString():
                        sqlfile.file.write(sql + ";\n")
                    sqlfile.file.flush()
                    
                    gcmd.RunCommand('db.execute',
                                    parent = self,
                                    quiet = True, 
                                    input = sqlfile.name)
                
                if addRecordDlg.mapDBInfo:
                    self._updateATM()
        
        elif self.toolbar.GetAction('type') in ["line", "boundary", "area"]:
            # add new point to the line
            self.polycoords.append(self.Pixel2Cell(event.GetPositionTuple()[:]))
            self.DrawLines(pdc = self.pdcTmp)
        
    def _geomAttrb(self, fid, dialog, attrb):
        """!Define geometry attributes
        """
        mapLayer = self.toolbar.GetLayer()
        item = self.tree.FindItemByData('maplayer', mapLayer)
        vdigit = self.tree.GetPyData(item)[0]['vdigit']
        if not vdigit or \
                'geomAttr' not in vdigit or \
                attrb not in vdigit['geomAttr']:
            return
        
        val = -1
        if attrb == 'length':
            val = self.digit.GetLineLength(fid)
            type = attrb
        elif attrb == 'area':
            val = self.digit.GetAreaSize(fid)
            type = attrb
        elif attrb == 'perimeter':
            val = self.digit.GetAreaPerimeter(fid)
            type = 'length'
        
        if val > 0:
            layer = int(UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value'))
            column = vdigit['geomAttr'][attrb]['column']
            val = UnitsConvertValue(val, type, vdigit['geomAttr'][attrb]['units'])
            dialog.SetColumnValue(layer, column, val)
            dialog.OnReset()
        
    def _geomAttrbUpdate(self, fids):
        """!Update geometry atrributes of currently selected features
        
        @param fid list feature id
        """
        mapLayer = self.parent.toolbars['vdigit'].GetLayer()
        vectorName =  mapLayer.GetName()
        item = self.tree.FindItemByData('maplayer', mapLayer)
        vdigit = self.tree.GetPyData(item)[0]['vdigit']
        
        if vdigit is None or 'geomAttr' not in vdigit:
            return
        
        dbInfo = gselect.VectorDBInfo(vectorName)
        sqlfile = tempfile.NamedTemporaryFile(mode = "w")
        for fid in fids:
            for layer, cats in self.digit.GetLineCats(fid).iteritems():
                table = dbInfo.GetTable(layer)
                for attrb, item in vdigit['geomAttr'].iteritems():
                    val = -1
                    if attrb == 'length':
                        val = self.digit.GetLineLength(fid)
                        type = attrb
                    elif attrb == 'area':
                        val = self.digit.GetAreaSize(fid)
                        type = attrb
                    elif attrb == 'perimeter':
                        val = self.digit.GetAreaPerimeter(fid)
                        type = 'length'
                    
                    if val < 0:
                        continue
                    val = UnitsConvertValue(val, type, item['units'])
                    
                    for cat in cats:
                        sqlfile.write('UPDATE %s SET %s = %f WHERE %s = %d;\n' % \
                                          (table, item['column'], val,
                                           dbInfo.GetKeyColumn(layer), cat))
            
            sqlfile.file.flush()
            gcmd.RunCommand('db.execute',
                            parent = True,
                            quiet = True,
                            input = sqlfile.name)
            
    def _updateATM(self):
        """!Update open Attribute Table Manager
        
        @todo: use AddDataRow() instead
        """
        # update ATM
        digitVector = self.toolbar.GetLayer().GetName()
                            
        for atm in self.lmgr.dialogs['atm']:
            atmVector = atm.GetVectorName()
            if atmVector == digitVector:
                layer = UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value')
                # TODO: use AddDataRow instead
                atm.LoadData(layer)
        
    def OnLeftDownEditLine(self, event):
        """!Left mouse button pressed - edit linear feature - add new
        vertex.
        """
        self.polycoords.append(self.Pixel2Cell(self.mouse['begin']))
        self.moveInfo['id'].append(wx.NewId())
        self.DrawLines(pdc = self.pdcTmp)
        
    def OnLeftDownMoveLine(self, event):
        """!Left mouse button pressed - vector digitizer move
        feature/vertex, edit linear feature
        """
        self.moveInfo = dict()
        # geographic coordinates of initial position (left-down)
        self.moveInfo['begin'] = None
        # list of ids to modify    
        self.moveInfo['id'] = list()
        
        if self.toolbar.GetAction() in ["moveVertex", "editLine"]:
            # set pen
            pcolor = UserSettings.Get(group = 'vdigit', key = "symbol",
                                      subkey = ["highlight", "color"])
            self.pen = self.polypen = wx.Pen(colour = pcolor,
                                             width = 2, style = wx.SHORT_DASH)
            self.pdcTmp.SetPen(self.polypen)
        
    def OnLeftDownDisplayCA(self, event):
        """!Left mouse button pressed - vector digitizer display categories
        or attributes action
        """
        try:
            mapLayer = self.toolbar.GetLayer().GetName()
        except:
            return
        
        coords = self.Pixel2Cell(self.mouse['begin'])
        
        # unselect
        self.digit.GetDisplay().SetSelected([])
        
        # select feature by point
        cats = {}
        self.digit.GetDisplay().SelectLineByPoint(coords) 
        
        if not self.digit.GetDisplay().GetSelected():
            for key in ('attributes', 'category'):
                if self.parent.dialogs[key] and \
                        self.parent.dialogs[key].IsShown():
                    self.parent.dialogs[key].Hide()
            self.UpdateMap(render = False, renderVector = True)
            return
        
        if UserSettings.Get(group = 'vdigit', key = 'checkForDupl',
                            subkey = 'enabled'):
            lines = self.digit.GetDisplay().GetSelected()
        else:
            lines = (self.digit.GetDisplay().GetSelected()[0],) # only first found
                        
        for line in lines:
            cats[line] = self.digit.GetLineCats(line)
        
        posWindow = self.ClientToScreen((self.mouse['end'][0] + self.dialogOffset,
                                         self.mouse['end'][1] + self.dialogOffset))
        
        if self.toolbar.GetAction() == "displayAttrs":
            # select attributes based on coordinates (all layers)
            if self.parent.dialogs['attributes'] is None:
                self.parent.dialogs['attributes'] = \
                    dbm_dialogs.DisplayAttributesDialog(parent = self, map = mapLayer,
                                                        cats = cats,
                                                        action = "update")
            else:
                # upgrade dialog
                self.parent.dialogs['attributes'].UpdateDialog(cats = cats)
           
            if self.parent.dialogs['attributes']:
                if len(cats.keys()) > 0:
                    # highlight feature & re-draw map
                    if not self.parent.dialogs['attributes'].IsShown():
                        self.parent.dialogs['attributes'].Show()
                else:
                    if self.parent.dialogs['attributes'] and \
                            self.parent.dialogs['attributes'].IsShown():
                        self.parent.dialogs['attributes'].Hide()
        
        else: # displayCats
            if self.parent.dialogs['category'] is None:
                # open new dialog
                dlg = VDigitCategoryDialog(parent = self,
                                           map = mapLayer,
                                           cats = cats,
                                           pos = posWindow,
                                           title = _("Update categories"))
                self.parent.dialogs['category'] = dlg
            else:
                # update currently open dialog
                self.parent.dialogs['category'].UpdateDialog(cats = cats)
                
            if self.parent.dialogs['category']:
                if len(cats.keys()) > 0:
                    # highlight feature & re-draw map
                    if not self.parent.dialogs['category'].IsShown():
                        self.parent.dialogs['category'].Show()
                else:
                    if self.parent.dialogs['category'].IsShown():
                        self.parent.dialogs['category'].Hide()
        
        self.UpdateMap(render = False, renderVector = True)
        
    def OnLeftDownCopyCA(self, event):
        """!Left mouse button pressed - vector digitizer copy
        categories or attributes action
        """
        if not hasattr(self, "copyCatsList"):
            self.copyCatsList = []
        else:
            self.copyCatsIds = []
            self.mouse['box'] = 'box'
        
    def OnLeftDownCopyLine(self, event):
        """!Left mouse button pressed - vector digitizer copy lines
        action
        """
        if not hasattr(self, "copyIds"):
            self.copyIds = []
            self.layerTmp = None
        
    def OnLeftDownBulkLine(self, event):
        """!Left mouse button pressed - vector digitizer label 3D
        vector lines
        """
        if len(self.polycoords) > 1: # start new line
            self.polycoords = []
            self.ClearLines(pdc = self.pdcTmp)
        self.polycoords.append(self.Pixel2Cell(event.GetPositionTuple()[:]))
        if len(self.polycoords) == 1:
            begin = self.Pixel2Cell(self.polycoords[-1])
            end   = self.Pixel2Cell(self.mouse['end'])
        else:
            end   = self.Pixel2Cell(self.polycoords[-1])
            begin = self.Pixel2Cell(self.mouse['begin'])
            
            self.DrawLines(self.pdcTmp, polycoords = (begin, end))
        
    def OnLeftDownUndo(self, event):
        """!Left mouse button pressed with control key - vector
        digitizer undo functionality
        """
        if self.mouse["use"] != "pointer" or not self.toolbar:
            return
        
        if (self.toolbar.GetAction() == "addLine" and \
                self.toolbar.GetAction('type') in ["line", "boundary", "area"]) or \
                self.toolbar.GetAction() == "editLine":
            # add line or boundary -> remove last point from the line
            try:
                removed = self.polycoords.pop()
                Debug.msg(4, "BufferedWindow.OnMiddleDown(): polycoords_poped=%s" % \
                              [removed,])
                # self.mouse['begin'] = self.Cell2Pixel(self.polycoords[-1])
            except:
                pass
            
        if self.toolbar.GetAction() == "editLine":
            # remove last vertex & line
            if len(self.moveInfo['id']) > 1:
                self.moveInfo['id'].pop()
                
            self.UpdateMap(render = False, renderVector = False)
            
        elif self.toolbar.GetAction() in ["deleteLine", "moveLine", "splitLine",
                                          "addVertex", "removeVertex", "moveVertex",
                                          "copyCats", "flipLine", "mergeLine",
                                          "snapLine", "connectLine", "copyLine",
                                          "queryLine", "breakLine", "typeConv"]:
            # varios tools -> unselected selected features
            self.digit.GetDisplay().SetSelected([])
            if self.toolbar.GetAction() in ["moveLine", "moveVertex", "editLine"] and \
                    hasattr(self, "moveInfo"):
                del self.moveInfo
                
            elif self.toolbar.GetAction() == "copyCats":
                try:
                    del self.copyCatsList
                    del self.copyCatsIds
                except AttributeError:
                    pass
                
            elif self.toolbar.GetAction() == "copyLine":
                del self.copyIds
                if self.layerTmp:
                    self.Map.DeleteLayer(self.layerTmp)
                    self.UpdateMap(render = True, renderVector = False)
                del self.layerTmp

            self.polycoords = []
            self.UpdateMap(render = False) # render vector
        
        elif self.toolbar.GetAction() == "zbulkLine":
            # reset polyline
            self.polycoords = []
            self.digit.GetDisplay().SetSelected([])
            self.UpdateMap(render = False)
        
        self.redrawAll = True
        self.UpdateMap(render = False, renderVector = False)

    def _onLeftDown(self, event):
        """!Left mouse button donw - vector digitizer various actions
        """
        try:
            mapLayer = self.toolbar.GetLayer().GetName()
        except:
            gcmd.GMessage(parent = self,
                          message = _("No vector map selected for editing."))
            event.Skip()
            return
        
        action = self.toolbar.GetAction()
        if not action:
            return
        
        if action not in ("moveVertex",
                          "addVertex",
                          "removeVertex",
                          "editLine"):
            # set pen
            self.pen = wx.Pen(colour = 'Red', width = 2, style = wx.SHORT_DASH)
            self.polypen = wx.Pen(colour = 'dark green', width = 2, style = wx.SOLID)
            
        if action in ("addVertex",
                      "removeVertex",
                      "splitLines"):
            # unselect
            self.digit.GetDisplay().SetSelected([])

        if action == "addLine":
            self.OnLeftDownAddLine(event)
            
        elif action == "editLine" and \
                hasattr(self, "moveInfo"):
            self.OnLeftDownEditLine(event)

        elif action in ("moveLine", "moveVertex", "editLine") and \
                not hasattr(self, "moveInfo"):
            self.OnLeftDownMoveLine(event)
        
        elif action in ("displayAttrs"
                        "displayCats"):
            self.OnLeftDownDisplayCA(event)
            
        elif action in ("copyCats",
                        "copyAttrs"):
            self.OnLeftDownCopyCA(event)
            
        elif action == "copyLine":
            self.OnLeftDownCopyLine(event)
            
        elif action == "zbulkLine":
            self.OnLeftDownBulkLine(event)
        
    def OnLeftUpVarious(self, event):
        """!Left mouse button released - vector digitizer various
        actions
        """
        pos1 = self.Pixel2Cell(self.mouse['begin'])
        pos2 = self.Pixel2Cell(self.mouse['end'])
        
        nselected = 0
        # -> delete line || move line || move vertex
        if self.toolbar.GetAction() in ("moveVertex",
                                        "editLine"):
            if len(self.digit.GetDisplay().GetSelected()) == 0:
                nselected = self.digit.GetDisplay().SelectLineByPoint(pos1)['point']
                
                if self.toolbar.GetAction() == "editLine":
                    try:
                        selVertex = self.digit.GetDisplay().GetSelectedVertex(pos1)[0]
                    except IndexError:
                        selVertex = None
                        
                    if selVertex:
                        # self.UpdateMap(render=False)
                        ids = self.digit.GetDisplay().GetSelected(grassId = False)
                        # move this line to tmp layer
                        self.polycoords = []
                        for id in ids:
                            if id % 2: # register only vertices
                                e, n = self.Pixel2Cell(self.pdcVector.GetIdBounds(id)[0:2])
                                self.polycoords.append((e, n))
                        self.digit.GetDisplay().DrawSelected(False) 
                                
                        if selVertex < ids[-1] / 2:
                            # choose first or last node of line
                            self.moveInfo['id'].reverse()
                            self.polycoords.reverse()
                    else:
                        # unselect
                        self.digit.GetDisplay().SetSelected([])
                        del self.moveInfo
                
                    self.UpdateMap(render = False)
            
        elif self.toolbar.GetAction() in ("copyCats",
                                          "copyAttrs"):
            if not hasattr(self, "copyCatsIds"):
                # 'from' -> select by point
                nselected = self.digit.GetDisplay().SelectLineByPoint(pos1)['point']
                if nselected:
                    self.copyCatsList = self.digit.GetDisplay().GetSelected()
            else:
                # -> 'to' -> select by bbox
                self.digit.GetDisplay().SetSelected([])
                # return number of selected features (by box/point)
                nselected = self.digit.GetDisplay().SelectLinesByBox((pos1, pos2))
                if nselected == 0:
                    if self.digit.GetDisplay().SelectLineByPoint(pos1) is not None:
                        nselected = 1
                        
                if nselected > 0:
                    self.copyCatsIds = self.digit.GetDisplay().GetSelected()
        
        elif self.toolbar.GetAction() == "queryLine":
            selected = self.digit.SelectLinesByQuery(bbox = (pos1, pos2))
            nselected = len(selected)
            if nselected > 0:
                self.digit.GetDisplay().SetSelected(selected)
        
        else:
            # -> moveLine || deleteLine, etc. (select by point/box)
            if self.toolbar.GetAction() == 'moveLine' and \
                    len(self.digit.GetDisplay().GetSelected()) > 0:
                nselected = 0
            else:
                if self.toolbar.GetAction() == 'moveLine':
                    drawSeg = True
                else:
                    drawSeg = False
                
                nselected = self.digit.GetDisplay().SelectLinesByBox(bbox = (pos1, pos2),
                                                                     drawSeg = drawSeg)
                if nselected == 0:
                    if self.digit.GetDisplay().SelectLineByPoint(pos1) is not None:
                        nselected = 1
        
        if nselected > 0:
            if self.toolbar.GetAction() in ("moveLine", "moveVertex") and \
                    hasattr(self, "moveInfo"):
                # get pseudoDC id of objects which should be redrawn
                if self.toolbar.GetAction() == "moveLine":
                    # -> move line
                    self.moveInfo['id'] = self.digit.GetDisplay().GetSelected(grassId = False)
                else: # moveVertex
                    self.moveInfo['id'] = self.digit.GetDisplay().GetSelectedVertex(pos1)
                    if len(self.moveInfo['id']) == 0: # no vertex found
                        self.digit.GetDisplay().SetSelected([])
            
            #
            # check for duplicates
            #
            if UserSettings.Get(group = 'vdigit', key = 'checkForDupl', subkey = 'enabled'):
                dupl = self.digit.GetDisplay().GetDuplicates()
                self.UpdateMap(render = False)
                    
                if dupl:
                    posWindow = self.ClientToScreen((self.mouse['end'][0] + self.dialogOffset,
                                                     self.mouse['end'][1] + self.dialogOffset))
                    
                    dlg = VDigitDuplicatesDialog(parent = self, data = dupl, pos = posWindow)
                    
                    if dlg.ShowModal() == wx.ID_OK:
                        self.digit.GetDisplay().UnSelect(dlg.GetUnSelected())
                        # update selected
                        self.UpdateMap(render = False)
                
            if self.toolbar.GetAction() != "editLine":
                # -> move line || move vertex
                self.UpdateMap(render = False)
        
        else: # no vector object found
            if not (self.toolbar.GetAction() in ("moveLine",
                                                 "moveVertex") and \
                        hasattr(self, "moveInfo") and \
                        len(self.moveInfo['id']) > 0):
                # avoid left-click when features are already selected
                self.UpdateMap(render = False, renderVector = False)
        
    def OnLeftUpModifyLine(self, event):
        """!Left mouse button released - vector digitizer split line,
        add/remove vertex action
        """
        pos1 = self.Pixel2Cell(self.mouse['begin'])
        
        pointOnLine = self.digit.GetDisplay().SelectLineByPoint(pos1)['point']
        if not pointOnLine:
            return
        
        if self.toolbar.GetAction() in ["splitLine", "addVertex"]:
            self.UpdateMap(render = False) # highlight object
            self.DrawCross(pdc = self.pdcTmp, coords = self.Cell2Pixel((pointOnLine[0], pointOnLine[1])),
                           size = 5)
        else: # removeVertex
            # get only id of vertex
            try:
                id = self.digit.GetDisplay().GetSelectedVertex(pos1)[0]
            except IndexError:
                id = None
            
            if id:
                x, y = self.pdcVector.GetIdBounds(id)[0:2]
                self.pdcVector.RemoveId(id)
                self.UpdateMap(render = False) # highlight object
                self.DrawCross(pdc = self.pdcTmp, coords = (x, y),
                               size = 5)
            else:
                # unselect
                self.digit.GetDisplay().SetSelected([])
                self.UpdateMap(render = False)
        
    def OnLeftUpCopyLine(self, event):
        """!Left mouse button released - vector digitizer copy feature
        action
        """
        pos1 = self.Pixel2Cell(self.mouse['begin'])
        pos2 = self.Pixel2Cell(self.mouse['end'])
        
        if UserSettings.Get(group = 'vdigit', key = 'bgmap',
                            subkey = 'value', internal = True) == '':
            # no background map -> copy from current vector map layer
            nselected = self.digit.GetDisplay().SelectLinesByBox((pos1, pos2))
            
            if nselected > 0:
                # highlight selected features
                self.UpdateMap(render = False)
            else:
                self.UpdateMap(render = False, renderVector = False)
        else:
            # copy features from background map
            self.copyIds += self.digit.SelectLinesFromBackgroundMap(bbox = (pos1, pos2))
            if len(self.copyIds) > 0:
                color = UserSettings.Get(group = 'vdigit', key = 'symbol',
                                         subkey = ['highlight', 'color'])
                colorStr = str(color[0]) + ":" + \
                    str(color[1]) + ":" + \
                    str(color[2])
                dVectTmp = ['d.vect',
                            'map=%s' % UserSettings.Get(group = 'vdigit', key = 'bgmap',
                                                        subkey = 'value', internal = True),
                            'cats=%s' % utils.ListOfCatsToRange(self.copyIds),
                            '-i',
                            'color=%s' % colorStr,
                            'fcolor=%s' % colorStr,
                            'type=point,line,boundary,centroid',
                            'width=2']
                
                if not self.layerTmp:
                    self.layerTmp = self.Map.AddLayer(type = 'vector',
                                                      name = globalvar.QUERYLAYER,
                                                      command = dVectTmp)
                else:
                    self.layerTmp.SetCmd(dVectTmp)
                
                self.UpdateMap(render = True, renderVector = False)
            else:
                self.UpdateMap(render = False, renderVector = False)
            
            self.redrawAll = None
            
    def OnLeftUpBulkLine(self, event):
        """!Left mouse button released - vector digitizer z-bulk line
        action
        """
        # select lines to be labeled
        pos1 = self.polycoords[0]
        pos2 = self.polycoords[1]
        nselected = self.digit.GetDisplay().SelectLinesByBox((pos1, pos2))
        
        if nselected > 0:
            # highlight selected features
            self.UpdateMap(render = False)
            self.DrawLines(pdc = self.pdcTmp) # redraw temp line
        else:
            self.UpdateMap(render = False, renderVector = False)
            
    def OnLeftUpConnectLine(self, event):
        """!Left mouse button released - vector digitizer connect line
        action
        """
        if len(self.digit.GetDisplay().GetSelected()) > 0:
            self.UpdateMap(render = False)
        
    def _onLeftUp(self, event):
        """!Left mouse button released"""
        if hasattr(self, "moveInfo"):
            if len(self.digit.GetDisplay().GetSelected()) == 0:
                self.moveInfo['begin'] = self.Pixel2Cell(self.mouse['begin']) # left down
            
            # eliminate initial mouse moving efect
            self.mouse['begin'] = self.mouse['end'] 
        
        if self.toolbar.GetAction() in ("deleteLine",
                                        "moveLine",
                                        "moveVertex",
                                        "copyCats",
                                        "copyAttrs",
                                        "editLine",
                                        "flipLine",
                                        "mergeLine",
                                        "snapLine",
                                        "queryLine",
                                        "breakLine",
                                        "typeConv",
                                        "connectLine"):
            self.OnLeftUpVarious(event)
        
        elif self.toolbar.GetAction() in ("splitLine",
                                          "addVertex",
                                          "removeVertex"):
            self.OnLeftUpModifyLine(event)
        
        elif self.toolbar.GetAction() == "copyLine":
            self.OnLeftUpCopyLine(event)
            
        elif self.toolbar.GetAction() == "zbulkLine" and \
                len(self.polycoords) == 2:
            self.OnLeftUpBulkLine(event)
        
        elif self.toolbar.GetAction() == "connectLine":
            self.OnLeftUpConnectLine(event)
        
        if len(self.digit.GetDisplay().GetSelected()) > 0:
            self.redrawAll = None
        
    def _onRightDown(self, event):
        # digitization tool (confirm action)
        if self.toolbar.GetAction() in ("moveLine", "moveVertex") and \
                hasattr(self, "moveInfo"):
            pFrom = self.moveInfo['begin']
            pTo = self.Pixel2Cell(event.GetPositionTuple())
            
            move = (pTo[0] - pFrom[0],
                    pTo[1] - pFrom[1])
            
            if self.toolbar.GetAction() == "moveLine":
                # move line
                if self.digit.MoveSelectedLines(move) < 0:
                    return
            elif self.toolbar.GetAction() == "moveVertex":
                # move vertex
                fid = self.digit.MoveSelectedVertex(pFrom, move)
                if fid < 0:
                    return
                
                self._geomAttrbUpdate([fid,])
            
            del self.moveInfo
        
    def _onRightUp(self, event):
        """!Right mouse button released
        """
        # digitization tool (confirm action)
        if self.toolbar.GetAction() == "addLine" and \
                self.toolbar.GetAction('type') in ["line", "boundary", "area"]:
            # -> add new line / boundary
            try:
                mapName = self.toolbar.GetLayer().GetName()
            except:
                mapName = None
                gcmd.GError(parent = self,
                            message = _("No vector map selected for editing."))
                    
            if mapName:
                if self.toolbar.GetAction('type') == 'line':
                    line = True
                else:
                    line = False
                    
                if len(self.polycoords) < 2: # ignore 'one-point' lines
                    return
                    
                nfeat, fids = self.digit.AddFeature(self.toolbar.GetAction('type'), self.polycoords)
                if nfeat < 0:
                    return
                    
                position = self.Cell2Pixel(self.polycoords[-1])
                self.polycoords = []
                self.UpdateMap(render = False)
                self.redrawAll = True
                self.Refresh()
                
            # add new record into atribute table
                if UserSettings.Get(group = 'vdigit', key = "addRecord", subkey = 'enabled') and \
                        (line is True or \
                             (not line and nfeat > 0)):
                    posWindow = self.ClientToScreen((position[0] + self.dialogOffset,
                                                     position[1] + self.dialogOffset))
                        
                    # select attributes based on layer and category
                    cats = { fids[0] : {
                            UserSettings.Get(group = 'vdigit', key = "layer", subkey = 'value') :
                                (UserSettings.Get(group = 'vdigit', key = "category", subkey = 'value'), )
                            }}
                    
                    addRecordDlg = dbm_dialogs.DisplayAttributesDialog(parent = self, map = mapName,
                                                                       cats = cats,
                                                                       pos = posWindow,
                                                                       action = "add")
                    
                    for fid in fids:
                        self._geomAttrb(fid, addRecordDlg, 'length')
                        # auto-placing centroid
                        self._geomAttrb(fid, addRecordDlg, 'area')
                        self._geomAttrb(fid, addRecordDlg, 'perimeter')

                    
                    if addRecordDlg.mapDBInfo and \
                            addRecordDlg.ShowModal() == wx.ID_OK:
                        sqlfile = tempfile.NamedTemporaryFile(mode = "w")
                        for sql in addRecordDlg.GetSQLString():
                            sqlfile.file.write(sql + ";\n")
                        sqlfile.file.flush()
                        gcmd.RunCommand('db.execute',
                                        parent = True,
                                        quiet = True,
                                        input = sqlfile.name)
                        
                    if addRecordDlg.mapDBInfo:
                        self._updateATM()
            
        elif self.toolbar.GetAction() == "deleteLine":
            # -> delete selected vector features
            if self.digit.DeleteSelectedLines() < 0:
                return
            self._updateATM()
        elif self.toolbar.GetAction() == "splitLine":
            # split line
            if self.digit.SplitLine(self.Pixel2Cell(self.mouse['begin'])) < 0:
                return
        elif self.toolbar.GetAction() == "addVertex":
            # add vertex
            fid = self.digit.AddVertex(self.Pixel2Cell(self.mouse['begin']))
            if fid < 0:
                return
        elif self.toolbar.GetAction() == "removeVertex":
            # remove vertex
            fid = self.digit.RemoveVertex(self.Pixel2Cell(self.mouse['begin']))
            if fid < 0:
                return
            self._geomAttrbUpdate([fid,])
        elif self.toolbar.GetAction() in ("copyCats", "copyAttrs"):
            try:
                if self.toolbar.GetAction() == 'copyCats':
                    if self.digit.CopyCats(self.copyCatsList,
                                           self.copyCatsIds, copyAttrb = False) < 0:
                        return
                else:
                    if self.digit.CopyCats(self.copyCatsList,
                                           self.copyCatsIds, copyAttrb = True) < 0:
                        return
                
                del self.copyCatsList
                del self.copyCatsIds
            except AttributeError:
                pass
            
            self._updateATM()
                
        elif self.toolbar.GetAction() == "editLine" and \
                hasattr(self, "moveInfo"):
            line = self.digit.GetDisplay().GetSelected()
            if self.digit.EditLine(line, self.polycoords) < 0:
                return
                
            del self.moveInfo
                
        elif self.toolbar.GetAction() == "flipLine":
            if self.digit.FlipLine() < 0:
                return
        elif self.toolbar.GetAction() == "mergeLine":
            if self.digit.MergeLine() < 0:
                return
        elif self.toolbar.GetAction() == "breakLine":
            if self.digit.BreakLine() < 0:
                return
        elif self.toolbar.GetAction() == "snapLine":
            if self.digit.SnapLine() < 0:
                return
        elif self.toolbar.GetAction() == "connectLine":
            if len(self.digit.GetDisplay().GetSelected()) > 1:
                if self.digit.ConnectLine() < 0:
                    return
        elif self.toolbar.GetAction() == "copyLine":
            if self.digit.CopyLine(self.copyIds) < 0:
                return
            del self.copyIds
            if self.layerTmp:
                self.Map.DeleteLayer(self.layerTmp)
                self.UpdateMap(render = True, renderVector = False)
            del self.layerTmp
        
        elif self.toolbar.GetAction() == "zbulkLine" and len(self.polycoords) == 2:
            pos1 = self.polycoords[0]
            pos2 = self.polycoords[1]
            
            selected = self.digit.GetDisplay().GetSelected()
            dlg = VDigitZBulkDialog(parent = self, title = _("Z bulk-labeling dialog"),
                                    nselected = len(selected))
            if dlg.ShowModal() == wx.ID_OK:
                if self.digit.ZBulkLines(pos1, pos2, dlg.value.GetValue(),
                                         dlg.step.GetValue()) < 0:
                    return
            self.UpdateMap(render = False, renderVector = True)
        elif self.toolbar.GetAction() == "typeConv":
            # -> feature type conversion
            # - point <-> centroid
            # - line <-> boundary
            if self.digit.TypeConvForSelectedLines() < 0:
                return

        if self.toolbar.GetAction() != "addLine":
            # unselect and re-render
            self.digit.GetDisplay().SetSelected([])
            self.polycoords = []
            self.UpdateMap(render = False)
        
    def _onMouseMoving(self, event):
        self.mouse['end'] = event.GetPositionTuple()[:]
        
        Debug.msg (5, "BufferedWindow.OnMouseMoving(): coords=%f,%f" % \
                       (self.mouse['end'][0], self.mouse['end'][1]))
        
        if self.toolbar.GetAction() == "addLine" and \
                self.toolbar.GetAction('type') in ["line", "boundary", "area"]:
            if len(self.polycoords) > 0:
                self.MouseDraw(pdc = self.pdcTmp, begin = self.Cell2Pixel(self.polycoords[-1]))
        
        elif self.toolbar.GetAction() in ["moveLine", "moveVertex", "editLine"] \
                and hasattr(self, "moveInfo"):
            dx = self.mouse['end'][0] - self.mouse['begin'][0]
            dy = self.mouse['end'][1] - self.mouse['begin'][1]
            
            if len(self.moveInfo['id']) > 0:
                # draw lines on new position
                if self.toolbar.GetAction() == "moveLine":
                    # move line
                    for id in self.moveInfo['id']:
                        self.pdcTmp.TranslateId(id, dx, dy)
                elif self.toolbar.GetAction() in ["moveVertex", "editLine"]:
                    # move vertex ->
                    # (vertex, left vertex, left line,
                    # right vertex, right line)
                    
                    # do not draw static lines
                    if self.toolbar.GetAction() == "moveVertex":
                        self.polycoords = []
                        ### self.pdcTmp.TranslateId(self.moveInfo['id'][0], dx, dy)
                        self.pdcTmp.RemoveId(self.moveInfo['id'][0])
                        if self.moveInfo['id'][1] > 0: # previous vertex
                            x, y = self.Pixel2Cell(self.pdcTmp.GetIdBounds(self.moveInfo['id'][1])[0:2])
                            self.pdcTmp.RemoveId(self.moveInfo['id'][1] + 1)
                            self.polycoords.append((x, y))
                        ### x, y = self.Pixel2Cell(self.pdcTmp.GetIdBounds(self.moveInfo['id'][0])[0:2])
                        self.polycoords.append(self.Pixel2Cell(self.mouse['end']))
                        if self.moveInfo['id'][2] > 0: # next vertex
                            x, y = self.Pixel2Cell(self.pdcTmp.GetIdBounds(self.moveInfo['id'][2])[0:2])
                            self.pdcTmp.RemoveId(self.moveInfo['id'][2]-1)
                            self.polycoords.append((x, y))
                        
                        self.ClearLines(pdc = self.pdcTmp)
                        self.DrawLines(pdc = self.pdcTmp)
                        
                    else: # edit line
                        try:
                            if self.moveInfo['id'][-1] > 0: # previous vertex
                                self.MouseDraw(pdc = self.pdcTmp,
                                               begin = self.Cell2Pixel(self.polycoords[-1]))
                        except: # no line
                            self.moveInfo['id'] = []
                            self.polycoords = []
                
            self.Refresh() # TODO: use RefreshRect()
            self.mouse['begin'] = self.mouse['end']
            
        elif self.toolbar.GetAction() == "zbulkLine":
            if len(self.polycoords) == 1:
                # draw mouse moving
                self.MouseDraw(self.pdcTmp)
                
    def _zoom(self, event):
        tmp1 = self.mouse['end']
        tmp2 = self.Cell2Pixel(self.moveInfo['begin'])
        dx = tmp1[0] - tmp2[0]
        dy = tmp1[1] - tmp2[1]
        self.moveInfo['beginDiff'] = (dx, dy)
        for id in self.moveInfo['id']:
            self.pdcTmp.RemoveId(id)
        
