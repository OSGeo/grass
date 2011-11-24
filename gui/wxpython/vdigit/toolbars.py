"""!
@package vdigit.toolbars

@brief wxGUI vector digitizer toolbars

List of classes:
 - VDigitToolbar

(C) 2007-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

from grass.script import core as grass

from gui_core.toolbars  import BaseToolbar
from gui_core.dialogs   import CreateNewVector
from vdigit.preferences import VDigitSettingsDialog
from vdigit.main        import VDigit
from core.debug         import Debug
from core.settings      import UserSettings
from core.gcmd          import GError

class VDigitToolbar(BaseToolbar):
    """!Toolbar for digitization
    """
    def __init__(self, parent, MapWindow, tools = [], layerTree = None, log = None):
        self.MapWindow     = MapWindow
        self.Map           = MapWindow.GetMap() # Map class instance
        self.layerTree     = layerTree  # reference to layer tree associated to map display
        self.log           = log        # log area
        self.tools         = tools
        BaseToolbar.__init__(self, parent)
        self.digit         = None
        
        # currently selected map layer for editing (reference to MapLayer instance)
        self.mapLayer = None
        # list of vector layers from Layer Manager (only in the current mapset)
        self.layers   = [] 
        
        self.comboid  = self.combo = None
        self.undo     = -1
        
        # only one dialog can be open
        self.settingsDialog   = None
        
        # create toolbars (two rows optionally)
        self.InitToolbar(self._toolbarData())
        self.Bind(wx.EVT_TOOL, self.OnTool)
        
        # default action (digitize new point, line, etc.)
        self.action = { 'desc' : '',
                        'type' : '',
                        'id'   : -1 }
        
        # list of available vector maps
        self.UpdateListOfLayers(updateTool = True)
        
        # realize toolbar
        self.Realize()
        # workaround for Mac bug. May be fixed by 2.8.8, but not before then.
        if self.combo:
            self.combo.Hide()
            self.combo.Show()
        
        # disable undo/redo
        if self.undo:
            self.EnableTool(self.undo, False)
        
        # toogle to pointer by default
        self.OnTool(None)
        
        self.FixSize(width = 105)
        
    def _toolbarData(self):
        """!Toolbar data
        """
        data = []
        icons = Icons['vdigit']
        if not self.tools or 'selector' in self.tools:
            data.append((None, ))
        if not self.tools or 'addPoint' in self.tools:
            data.append(("addPoint", icons["addPoint"],
                         self.OnAddPoint,
                         wx.ITEM_CHECK))
        if not self.tools or 'addLine' in self.tools:
            data.append(("addLine", icons["addLine"],
                        self.OnAddLine,
                        wx.ITEM_CHECK))
        if not self.tools or 'addBoundary' in self.tools:
            data.append(("addBoundary", icons["addBoundary"],
                         self.OnAddBoundary,
                         wx.ITEM_CHECK))
        if not self.tools or 'addCentroid' in self.tools:
            data.append(("addCentroid", icons["addCentroid"],
                         self.OnAddCentroid,
                         wx.ITEM_CHECK))
        if not self.tools or 'addArea' in self.tools:
            data.append(("addArea", icons["addArea"],
                         self.OnAddArea,
                         wx.ITEM_CHECK))
        if not self.tools or 'moveVertex' in self.tools:            
            data.append(("moveVertex", icons["moveVertex"],
                         self.OnMoveVertex,
                         wx.ITEM_CHECK))
        if not self.tools or 'addVertex' in self.tools:
            data.append(("addVertex", icons["addVertex"],
                         self.OnAddVertex,
                         wx.ITEM_CHECK))
        if not self.tools or 'removeVertex' in self.tools:
            data.append(("removeVertex", icons["removeVertex"],
                         self.OnRemoveVertex,
                         wx.ITEM_CHECK))
        if not self.tools or 'editLine' in self.tools:            
            data.append(("editLine", icons["editLine"],
                         self.OnEditLine,
                         wx.ITEM_CHECK))
        if not self.tools or 'moveLine' in self.tools:
            data.append(("moveLine", icons["moveLine"],
                         self.OnMoveLine,
                         wx.ITEM_CHECK))
        if not self.tools or 'deleteLine' in self.tools:
            data.append(("deleteLine", icons["deleteLine"],
                         self.OnDeleteLine,
                         wx.ITEM_CHECK))
        if not self.tools or 'displayCats' in self.tools:
            data.append(("displayCats", icons["displayCats"],
                         self.OnDisplayCats,
                         wx.ITEM_CHECK))
        if not self.tools or 'displayAttr' in self.tools:
            data.append(("displayAttr", icons["displayAttr"],
                         self.OnDisplayAttr,
                         wx.ITEM_CHECK))
        if not self.tools or 'additionalSelf.Tools' in self.tools:
            data.append(("additionalTools", icons["additionalTools"],
                         self.OnAdditionalToolMenu,
                         wx.ITEM_CHECK))
        if not self.tools or 'undo' in self.tools or \
                'settings' in self.tools or \
                'quit' in self.tools:
            data.append((None, ))
        if not self.tools or 'undo' in self.tools:
            data.append(("undo", icons["undo"],
                         self.OnUndo))
        if not self.tools or 'settings' in self.tools:
            data.append(("settings", icons["settings"],
                         self.OnSettings))
        if not self.tools or 'quit' in self.tools:
            data.append(("quit", icons["quit"],
                         self.OnExit))
        
        return self._getToolbarData(data)
    
    def OnTool(self, event):
        """!Tool selected -> disable selected tool in map toolbar"""
        if self.parent.GetName() == 'IClassWindow':
            toolbarName = 'iClassMap'
        else:
            toolbarName = 'map'
        aId = self.parent.toolbars[toolbarName].GetAction(type = 'id')
        self.parent.toolbars[toolbarName].ToggleTool(aId, False)
                
        # set cursor
        cursor = self.parent.cursors["cross"]
        self.MapWindow.SetCursor(cursor)
        
        # pointer
        self.parent.OnPointer(None)
                
        if event:
            # deselect previously selected tool
            aId = self.action.get('id', -1)
            if aId != event.GetId() and \
                    self.action['id'] != -1:
                self.ToggleTool(self.action['id'], False)
            else:
                self.ToggleTool(self.action['id'], True)
            
            self.action['id'] = event.GetId()
            
            event.Skip()
        
        if self.action['id'] != -1:
            self.ToggleTool(self.action['id'], True)
        
        # clear tmp canvas
        if self.action['id'] != aId:
            self.MapWindow.ClearLines(pdc = self.MapWindow.pdcTmp)
            if self.digit and \
                    len(self.MapWindow.digit.GetDisplay().GetSelected()) > 0:
                # cancel action
                self.MapWindow.OnMiddleDown(None)
        
        # set focus
        self.MapWindow.SetFocus()
        
    def OnAddPoint(self, event):
        """!Add point to the vector map Laier"""
        Debug.msg (2, "VDigitToolbar.OnAddPoint()")
        self.action = { 'desc' : "addLine",
                        'type' : "point",
                        'id'   : self.addPoint }
        self.MapWindow.mouse['box'] = 'point'
        
    def OnAddLine(self, event):
        """!Add line to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddLine()")
        self.action = { 'desc' : "addLine",
                        'type' : "line",
                        'id'   : self.addLine }
        self.MapWindow.mouse['box'] = 'line'
        ### self.MapWindow.polycoords = [] # reset temp line
                
    def OnAddBoundary(self, event):
        """!Add boundary to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddBoundary()")
        if self.action['desc'] != 'addLine' or \
                self.action['type'] != 'boundary':
            self.MapWindow.polycoords = [] # reset temp line
        self.action = { 'desc' : "addLine",
                        'type' : "boundary",
                        'id'   : self.addBoundary }
        self.MapWindow.mouse['box'] = 'line'
        
    def OnAddCentroid(self, event):
        """!Add centroid to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddCentroid()")
        self.action = { 'desc' : "addLine",
                        'type' : "centroid",
                        'id'   : self.addCentroid }
        self.MapWindow.mouse['box'] = 'point'

    def OnAddArea(self, event):
        """!Add area to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddCentroid()")
        self.action = { 'desc' : "addLine",
                        'type' : "area",
                        'id'   : self.addArea }
        self.MapWindow.mouse['box'] = 'line'

    def OnExit (self, event=None):
        """!Quit digitization tool"""
        # stop editing of the currently selected map layer
        if self.mapLayer:
            self.StopEditing()
        
        # close dialogs if still open
        if self.settingsDialog:
            self.settingsDialog.OnCancel(None)
            
        # set default mouse settings
        self.MapWindow.mouse['use'] = "pointer"
        self.MapWindow.mouse['box'] = "point"
        self.MapWindow.polycoords = []
        
        # disable the toolbar
        self.parent.RemoveToolbar("vdigit")
        
    def OnMoveVertex(self, event):
        """!Move line vertex"""
        Debug.msg(2, "Digittoolbar.OnMoveVertex():")
        self.action = { 'desc' : "moveVertex",
                        'id'   : self.moveVertex }
        self.MapWindow.mouse['box'] = 'point'

    def OnAddVertex(self, event):
        """!Add line vertex"""
        Debug.msg(2, "Digittoolbar.OnAddVertex():")
        self.action = { 'desc' : "addVertex",
                        'id'   : self.addVertex }
        self.MapWindow.mouse['box'] = 'point'
        
    def OnRemoveVertex(self, event):
        """!Remove line vertex"""
        Debug.msg(2, "Digittoolbar.OnRemoveVertex():")
        self.action = { 'desc' : "removeVertex",
                        'id'   : self.removeVertex }
        self.MapWindow.mouse['box'] = 'point'

    def OnEditLine(self, event):
        """!Edit line"""
        Debug.msg(2, "Digittoolbar.OnEditLine():")
        self.action = { 'desc' : "editLine",
                        'id'   : self.editLine }
        self.MapWindow.mouse['box'] = 'line'

    def OnMoveLine(self, event):
        """!Move line"""
        Debug.msg(2, "Digittoolbar.OnMoveLine():")
        self.action = { 'desc' : "moveLine",
                        'id'   : self.moveLine }
        self.MapWindow.mouse['box'] = 'box'

    def OnDeleteLine(self, event):
        """!Delete line"""
        Debug.msg(2, "Digittoolbar.OnDeleteLine():")
        self.action = { 'desc' : "deleteLine",
                        'id'   : self.deleteLine }
        self.MapWindow.mouse['box'] = 'box'

    def OnDisplayCats(self, event):
        """!Display/update categories"""
        Debug.msg(2, "Digittoolbar.OnDisplayCats():")
        self.action = { 'desc' : "displayCats",
                        'id'   : self.displayCats }
        self.MapWindow.mouse['box'] = 'point'

    def OnDisplayAttr(self, event):
        """!Display/update attributes"""
        Debug.msg(2, "Digittoolbar.OnDisplayAttr():")
        self.action = { 'desc' : "displayAttrs",
                        'id'   : self.displayAttr }
        self.MapWindow.mouse['box'] = 'point'
        
    def OnUndo(self, event):
        """!Undo previous changes"""
        self.digit.Undo()
        
        event.Skip()

    def EnableUndo(self, enable = True):
        """!Enable 'Undo' in toolbar
        
        @param enable False for disable
        """
        if enable:
            if self.GetToolEnabled(self.undo) is False:
                self.EnableTool(self.undo, True)
        else:
            if self.GetToolEnabled(self.undo) is True:
                self.EnableTool(self.undo, False)
        
    def OnSettings(self, event):
        """!Show settings dialog"""
        if self.digit is None:
            try:
                self.digit = self.MapWindow.digit = VDigit(mapwindow = self.MapWindow)
            except SystemExit:
                self.digit = self.MapWindow.digit = None
        
        if not self.settingsDialog:
            self.settingsDialog = VDigitSettingsDialog(parent = self.parent, title = _("Digitization settings"),
                                                       style = wx.DEFAULT_DIALOG_STYLE)
            self.settingsDialog.Show()

    def OnAdditionalToolMenu(self, event):
        """!Menu for additional tools"""
        point = wx.GetMousePosition()
        toolMenu = wx.Menu()
        
        for label, itype, handler, desc in (
            (_('Break selected lines/boundaries at intersection'),
             wx.ITEM_CHECK, self.OnBreak, "breakLine"),
            (_('Connect selected lines/boundaries'),
             wx.ITEM_CHECK, self.OnConnect, "connectLine"),
            (_('Copy categories'),
             wx.ITEM_CHECK, self.OnCopyCats, "copyCats"),
            (_('Copy features from (background) vector map'),
             wx.ITEM_CHECK, self.OnCopy, "copyLine"),
            (_('Copy attributes'),
             wx.ITEM_CHECK, self.OnCopyAttrb, "copyAttrs"),
            (_('Feature type conversion'),
             wx.ITEM_CHECK, self.OnTypeConversion, "typeConv"),
            (_('Flip selected lines/boundaries'),
             wx.ITEM_CHECK, self.OnFlip, "flipLine"),
            (_('Merge selected lines/boundaries'),
             wx.ITEM_CHECK, self.OnMerge, "mergeLine"),
            (_('Snap selected lines/boundaries (only to nodes)'),
             wx.ITEM_CHECK, self.OnSnap, "snapLine"),
            (_('Split line/boundary'),
             wx.ITEM_CHECK, self.OnSplitLine, "splitLine"),
            (_('Query features'),
             wx.ITEM_CHECK, self.OnQuery, "queryLine"),
            (_('Z bulk-labeling of 3D lines'),
             wx.ITEM_CHECK, self.OnZBulk, "zbulkLine")):
            # Add items to the menu
            item = wx.MenuItem(parentMenu = toolMenu, id = wx.ID_ANY,
                               text = label,
                               kind = itype)
            toolMenu.AppendItem(item)
            self.MapWindow.Bind(wx.EVT_MENU, handler, item)
            if self.action['desc'] == desc:
                item.Check(True)
        
        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.MapWindow.PopupMenu(toolMenu)
        toolMenu.Destroy()
        
        if self.action['desc'] == 'addPoint':
            self.ToggleTool(self.additionalTools, False)
        
    def OnCopy(self, event):
        """!Copy selected features from (background) vector map"""
        if self.action['desc'] == 'copyLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnCopy():")
        self.action = { 'desc' : "copyLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'box'

    def OnSplitLine(self, event):
        """!Split line"""
        if self.action['desc'] == 'splitLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnSplitLine():")
        self.action = { 'desc' : "splitLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'point'


    def OnCopyCats(self, event):
        """!Copy categories"""
        if self.action['desc'] == 'copyCats': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.copyCats, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnCopyCats():")
        self.action = { 'desc' : "copyCats",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'point'

    def OnCopyAttrb(self, event):
        """!Copy attributes"""
        if self.action['desc'] == 'copyAttrs': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.copyCats, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnCopyAttrb():")
        self.action = { 'desc' : "copyAttrs",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'point'
        

    def OnFlip(self, event):
        """!Flip selected lines/boundaries"""
        if self.action['desc'] == 'flipLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnFlip():")
        self.action = { 'desc' : "flipLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'box'

    def OnMerge(self, event):
        """!Merge selected lines/boundaries"""
        if self.action['desc'] == 'mergeLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnMerge():")
        self.action = { 'desc' : "mergeLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'box'

    def OnBreak(self, event):
        """!Break selected lines/boundaries"""
        if self.action['desc'] == 'breakLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnBreak():")
        self.action = { 'desc' : "breakLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'box'

    def OnSnap(self, event):
        """!Snap selected features"""
        if self.action['desc'] == 'snapLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnSnap():")
        self.action = { 'desc' : "snapLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'box'

    def OnConnect(self, event):
        """!Connect selected lines/boundaries"""
        if self.action['desc'] == 'connectLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnConnect():")
        self.action = { 'desc' : "connectLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'box'

    def OnQuery(self, event):
        """!Query selected lines/boundaries"""
        if self.action['desc'] == 'queryLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnQuery(): %s" % \
                      UserSettings.Get(group = 'vdigit', key = 'query', subkey = 'selection'))
        self.action = { 'desc' : "queryLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'box'

    def OnZBulk(self, event):
        """!Z bulk-labeling selected lines/boundaries"""
        if not self.digit.IsVector3D():
            GError(parent = self.parent,
                   message = _("Vector map is not 3D. Operation canceled."))
            return
        
        if self.action['desc'] == 'zbulkLine': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnZBulk():")
        self.action = { 'desc' : "zbulkLine",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'line'

    def OnTypeConversion(self, event):
        """!Feature type conversion

        Supported conversions:
         - point <-> centroid
         - line <-> boundary
        """
        if self.action['desc'] == 'typeConv': # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnTypeConversion():")
        self.action = { 'desc' : "typeConv",
                        'id'   : self.additionalTools }
        self.MapWindow.mouse['box'] = 'box'

    def OnSelectMap (self, event):
        """!Select vector map layer for editing

        If there is a vector map layer already edited, this action is
        firstly terminated. The map layer is closed. After this the
        selected map layer activated for editing.
        """
        if event.GetSelection() == 0: # create new vector map layer
            if self.mapLayer:
                openVectorMap = self.mapLayer.GetName(fullyQualified = False)['name']
            else:
                openVectorMap = None
            dlg = CreateNewVector(self.parent,
                                  exceptMap = openVectorMap, log = self.log,
                                  cmd = (('v.edit',
                                          { 'flags' : 'b', 'tool' : 'create' },
                                          'map')),
                                  disableAdd = True)
            
            if dlg and dlg.GetName():
                # add layer to map layer tree
                if self.layerTree:
                    mapName = dlg.GetName() + '@' + grass.gisenv()['MAPSET']
                    self.layerTree.AddLayer(ltype = 'vector',
                                            lname = mapName,
                                            lcmd = ['d.vect', 'map=%s' % mapName])
                    
                    vectLayers = self.UpdateListOfLayers(updateTool = True)
                    selection = vectLayers.index(mapName)
                
                # create table ?
                if dlg.IsChecked('table'):
                    lmgr = self.parent.GetLayerManager()
                    if lmgr:
                        lmgr.OnShowAttributeTable(None, selection = 'table')
                dlg.Destroy()
            else:
                self.combo.SetValue(_('Select vector map'))
                if dlg:
                    dlg.Destroy()
                return
        else:
            selection = event.GetSelection() - 1 # first option is 'New vector map'
        
        # skip currently selected map
        if self.layers[selection] == self.mapLayer:
            return
        
        if self.mapLayer:
            # deactive map layer for editing
            self.StopEditing()
        
        # select the given map layer for editing
        self.StartEditing(self.layers[selection])
        
        event.Skip()
        
    def StartEditing (self, mapLayer):
        """!Start editing selected vector map layer.

        @param mapLayer MapLayer to be edited
        """
        # deactive layer
        self.Map.ChangeLayerActive(mapLayer, False)
        
        # clean map canvas
        self.MapWindow.EraseMap()
        
        # unset background map if needed
        if mapLayer:
            if UserSettings.Get(group = 'vdigit', key = 'bgmap',
                                subkey = 'value', internal = True) == mapLayer.GetName():
                UserSettings.Set(group = 'vdigit', key = 'bgmap',
                                 subkey = 'value', value = '', internal = True)
            
            self.parent.SetStatusText(_("Please wait, "
                                        "opening vector map <%s> for editing...") % mapLayer.GetName(),
                                        0)
        
        self.MapWindow.pdcVector = wx.PseudoDC()
        self.digit = self.MapWindow.digit = VDigit(mapwindow = self.MapWindow)
        
        self.mapLayer = mapLayer
        # open vector map
        if self.digit.OpenMap(mapLayer.GetName()) is None:
            self.mapLayer = None
            self.StopEditing()
            return False
        
        # check feature type (only for OGR layers)
        fType = self.digit.GetFeatureType()
        self.EnableAll()
        self.EnableUndo(False)
        
        if fType == 'Point':
            for tool in (self.addLine, self.addBoundary, self.addCentroid,
                         self.addArea, self.moveVertex, self.addVertex,
                         self.removeVertex, self.editLine):
                self.EnableTool(tool, False)
        elif fType == 'Line String':
            for tool in (self.addPoint, self.addBoundary, self.addCentroid,
                         self.addArea):
                self.EnableTool(tool, False)
        elif fType == 'Polygon':
            for tool in (self.addPoint, self.addLine, self.addBoundary, self.addCentroid):
                self.EnableTool(tool, False)
        elif fType:
            GError(parent = self,
                   message = _("Unsupported feature type '%s'. Unable to edit "
                               "OGR layer <%s>.") % (fType, mapLayer.GetName()))
            self.digit.CloseMap()
            self.mapLayer = None
            self.StopEditing()
            return False
        
        # update toolbar
        if self.combo:
            self.combo.SetValue(mapLayer.GetName())
        if 'map' in self.parent.toolbars:
            self.parent.toolbars['map'].combo.SetValue (_('Digitize'))
        
        if self.parent.GetName() != "IClassWindow":
            lmgr = self.parent.GetLayerManager()
            if lmgr:
                lmgr.toolbars['tools'].Enable('vdigit', enable = False)
        
        Debug.msg (4, "VDigitToolbar.StartEditing(): layer=%s" % mapLayer.GetName())
        
        # change cursor
        if self.MapWindow.mouse['use'] == 'pointer':
            self.MapWindow.SetCursor(self.parent.cursors["cross"])
        
        if not self.MapWindow.resize:
            self.MapWindow.UpdateMap(render = True)
        
        # respect opacity
        opacity = mapLayer.GetOpacity(float = True)
        
        if opacity < 1.0:
            alpha = int(opacity * 255)
            self.digit.GetDisplay().UpdateSettings(alpha = alpha)
        
        return True

    def StopEditing(self):
        """!Stop editing of selected vector map layer.

        @return True on success
        @return False on failure
        """
        if self.combo:
            self.combo.SetValue (_('Select vector map'))
        
        # save changes
        if self.mapLayer:
            Debug.msg (4, "VDigitToolbar.StopEditing(): layer=%s" % self.mapLayer.GetName())
            if UserSettings.Get(group = 'vdigit', key = 'saveOnExit', subkey = 'enabled') is False:
                if self.digit.GetUndoLevel() > -1:
                    dlg = wx.MessageDialog(parent = self.parent,
                                           message = _("Do you want to save changes "
                                                     "in vector map <%s>?") % self.mapLayer.GetName(),
                                           caption = _("Save changes?"),
                                           style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
                    if dlg.ShowModal() == wx.ID_NO:
                        # revert changes
                        self.digit.Undo(0)
                    dlg.Destroy()
            
            self.parent.SetStatusText(_("Please wait, "
                                        "closing and rebuilding topology of "
                                        "vector map <%s>...") % self.mapLayer.GetName(),
                                      0)
            self.digit.CloseMap()
            
            lmgr = self.parent.GetLayerManager()
            if lmgr:
                lmgr.toolbars['tools'].Enable('vdigit', enable = True)
                lmgr.GetLogWindow().GetProgressBar().SetValue(0)
                lmgr.GetLogWindow().WriteCmdLog(_("Editing of vector map <%s> successfully finished") % \
                                                    self.mapLayer.GetName(),
                                                switchPage = False)
            # re-active layer 
            item = self.parent.tree.FindItemByData('maplayer', self.mapLayer)
            if item and self.parent.tree.IsItemChecked(item):
                self.Map.ChangeLayerActive(self.mapLayer, True)
        
        # change cursor
        self.MapWindow.SetCursor(self.parent.cursors["default"])
        self.MapWindow.pdcVector = None
        
        # close dialogs
        for dialog in ('attributes', 'category'):
            if self.parent.dialogs[dialog]:
                self.parent.dialogs[dialog].Close()
                self.parent.dialogs[dialog] = None
        
        del self.digit
        del self.MapWindow.digit
        
        self.mapLayer = None
        
        self.MapWindow.redrawAll = True
        
        return True
    
    def UpdateListOfLayers (self, updateTool = False):
        """!Update list of available vector map layers.
        This list consists only editable layers (in the current mapset)

        @param updateTool True to update also toolbar
        """
        Debug.msg (4, "VDigitToolbar.UpdateListOfLayers(): updateTool=%d" % \
                   updateTool)
        
        layerNameSelected = None
         # name of currently selected layer
        if self.mapLayer:
            layerNameSelected = self.mapLayer.GetName()
        
        # select vector map layer in the current mapset
        layerNameList = []
        self.layers = self.Map.GetListOfLayers(l_type = "vector",
                                                      l_mapset = grass.gisenv()['MAPSET'])
        
        for layer in self.layers:
            if not layer.name in layerNameList: # do not duplicate layer
                layerNameList.append (layer.GetName())
        
        if updateTool: # update toolbar
            if not self.mapLayer:
                value = _('Select vector map')
            else:
                value = layerNameSelected
            
            if not self.comboid:
                if not self.tools or 'selector' in self.tools:
                    self.combo = wx.ComboBox(self, id = wx.ID_ANY, value = value,
                                             choices = [_('New vector map'), ] + layerNameList, size = (80, -1),
                                             style = wx.CB_READONLY)
                    self.comboid = self.InsertControl(0, self.combo)
                    self.parent.Bind(wx.EVT_COMBOBOX, self.OnSelectMap, self.comboid)
            else:
                self.combo.SetItems([_('New vector map'), ] + layerNameList)
            
            self.Realize()
        
        return layerNameList

    def GetLayer(self):
        """!Get selected layer for editing -- MapLayer instance"""
        return self.mapLayer

