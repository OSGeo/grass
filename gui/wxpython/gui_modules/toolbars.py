"""!
@package toolbar

@brief wxGUI toolbar widgets

Classes:
 - AbstractToolbar
 - MapToolbar
 - GCPMapToolbar
 - GCPDisplayToolbar
 - VDigitToolbar
 - ProfileToolbar
 - LMNvizToolbar
 - ModelToolbar
 - HistogramToolbar
 - LMWorkspaceToolbar
 - LMDataToolbar
 - LMToolsToolbar
 - LMMiscToolbar
 - LMVectorToolbar
 - PsMapToolbar

(C) 2007-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys
import platform

from grass.script import core as grass

import wx

import globalvar
import gcmd
import gdialogs
from vdigit import VDigitSettingsDialog, haveVDigit, VDigit
from debug import Debug
from preferences import globalSettings as UserSettings
from nviz import haveNviz
from nviz_preferences import NvizPreferencesDialog

sys.path.append(os.path.join(globalvar.ETCWXDIR, "icons"))
from icon import Icons

class AbstractToolbar(wx.ToolBar):
    """!Abstract toolbar class"""
    def __init__(self, parent):
        self.parent = parent
        wx.ToolBar.__init__(self, parent = self.parent, id = wx.ID_ANY)
        
        self.action = dict()
        
        self.Bind(wx.EVT_TOOL, self.OnTool)
        
        self.SetToolBitmapSize(globalvar.toolbarSize)
        
    def InitToolbar(self, toolData):
        """!Initialize toolbar, add tools to the toolbar
        """
        for tool in toolData:
            self.CreateTool(*tool)
        
        self._data = toolData
        
    def _toolbarData(self):
        """!Toolbar data (virtual)"""
        return None
    
    def CreateTool(self, label, bitmap, kind,
                   shortHelp, longHelp, handler):
        """!Add tool to the toolbar
        
        @return id of tool
        """
        bmpDisabled = wx.NullBitmap
        tool = -1
        if label:
            tool = vars(self)[label] = wx.NewId()
            Debug.msg(3, "CreateTool(): tool=%d, label=%s bitmap=%s" % \
                          (tool, label, bitmap))
            toolWin = self.AddLabelTool(tool, label, bitmap,
                                        bmpDisabled, kind,
                                        shortHelp, longHelp)
            self.Bind(wx.EVT_TOOL, handler, toolWin)
        else: # separator
            self.AddSeparator()
        
        return tool
    
    def EnableLongHelp(self, enable = True):
        """!Enable/disable long help
        
        @param enable True for enable otherwise disable
        """
        for tool in self._data:
            if tool[0] == '': # separator
                continue
            
            if enable:
                self.SetToolLongHelp(vars(self)[tool[0]], tool[4])
            else:
                self.SetToolLongHelp(vars(self)[tool[0]], "")
        
    def OnTool(self, event):
        """!Tool selected
        """
        if self.parent.GetName() == "GCPFrame":
            return
        
        if hasattr(self.parent, 'toolbars'):
            if self.parent.toolbars['vdigit']:
                # update vdigit toolbar (unselect currently selected tool)
                id = self.parent.toolbars['vdigit'].GetAction(type = 'id')
                self.parent.toolbars['vdigit'].ToggleTool(id, False)
        
        if event:
            # deselect previously selected tool
            id = self.action.get('id', -1)
            if id != event.GetId():
                self.ToggleTool(self.action['id'], False)
            else:
                self.ToggleTool(self.action['id'], True)
            
            self.action['id'] = event.GetId()
            
            event.Skip()
        else:
            # initialize toolbar
            self.ToggleTool(self.action['id'], True)
        
    def GetAction(self, type = 'desc'):
        """!Get current action info"""
        return self.action.get(type, '')
    
    def SelectDefault(self, event):
        """!Select default tool"""
        self.ToggleTool(self.defaultAction['id'], True)
        self.defaultAction['bind'](event)
        self.action = { 'id' : self.defaultAction['id'],
                        'desc' : self.defaultAction.get('desc', '') }
        
    def FixSize(self, width):
        """!Fix toolbar width on Windows
            
        @todo Determine why combobox causes problems here
        """
        if platform.system() == 'Windows':
            size = self.GetBestSize()
            self.SetSize((size[0] + width, size[1]))

    def Enable(self, tool, enable = True):
        """!Enable defined tool
        
        @param tool name
        @param enable True to enable otherwise disable tool
        """
        try:
            id = getattr(self, tool)
        except AttributeError:
            return
        
        self.EnableTool(id, enable)
        
    def _getToolbarData(self, data):
        """!Define tool
        """
        retData = list()
        for args in data:
            retData.append(self._defineTool(*args))
        
        return retData

    def _defineTool(self, name = None, icon = None, handler = None, item = wx.ITEM_NORMAL):
        """!Define tool
        """
        if name:
            return (name, icon.GetBitmap(),
                    item, icon.GetLabel(), icon.GetDesc(),
                    handler)
        return ("", "", "", "", "", "") # separator
    
class MapToolbar(AbstractToolbar):
    """!Map Display toolbar
    """
    def __init__(self, parent, mapcontent):
        """!Map Display constructor

        @param parent reference to MapFrame
        @param mapcontent reference to render.Map (registred by MapFrame)
        """
        self.mapcontent = mapcontent # render.Map
        AbstractToolbar.__init__(self, parent = parent) # MapFrame
        
        self.InitToolbar(self._toolbarData())
        
        # optional tools
        choices = [ _('2D view'), ]
        self.toolId = { '2d' : 0 }
        if self.parent.GetLayerManager():
            log = self.parent.GetLayerManager().GetLogWindow()
        
        if haveNviz:
            choices.append(_('3D view'))
            self.toolId['3d'] = 1
        else:
            from nviz import errorMsg
            log.WriteCmdLog(_('3D view mode not available'))
            log.WriteWarning(_('Reason: %s') % str(errorMsg))
            log.WriteLog(_('Note that the wxGUI\'s 3D view mode is currently disabled '
                           'on MS Windows (hopefully this will be fixed soon). '
                           'Please keep an eye out for updated versions of GRASS. '
                           'In the meantime you can use "NVIZ" from the File menu.'), wrap = 60)
            
            self.toolId['3d'] = -1

        if haveVDigit:
            choices.append(_('Digitize'))
            if self.toolId['3d'] > -1:
                self.toolId['vdigit'] = 2
            else:
                self.toolId['vdigit'] = 1
        else:
            from vdigit import errorMsg
            log.WriteCmdLog(_('Vector digitizer not available'))
            log.WriteWarning(_('Reason: %s') % errorMsg)
            log.WriteLog(_('Note that the wxGUI\'s vector digitizer is currently disabled '
                           '(hopefully this will be fixed soon). '
                           'Please keep an eye out for updated versions of GRASS. '
                           'In the meantime you can use "v.digit" from the Develop Vector menu.'), wrap = 60)
            
            self.toolId['vdigit'] = -1
        
        self.combo = wx.ComboBox(parent = self, id = wx.ID_ANY,
                                 choices = choices,
                                 style = wx.CB_READONLY, size = (110, -1))
        self.combo.SetSelection(0)
        
        self.comboid = self.AddControl(self.combo)
        self.parent.Bind(wx.EVT_COMBOBOX, self.OnSelectTool, self.comboid)
        
        # realize the toolbar
        self.Realize()
        
        # workaround for Mac bug. May be fixed by 2.8.8, but not before then.
        self.combo.Hide()
        self.combo.Show()
        
        self.action = { 'id' : self.pointer }
        self.defaultAction = { 'id' : self.pointer,
                               'bind' : self.parent.OnPointer }
        
        self.OnTool(None)
        
        self.EnableTool(self.zoomback, False)
        
        self.FixSize(width = 90)
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['displayWindow']
        return self._getToolbarData((('displaymap', icons['display'],
                                      self.parent.OnDraw),
                                     ('rendermap', icons['render'],
                                      self.parent.OnRender),
                                     ('erase', icons['erase'],
                                      self.parent.OnErase),
                                     (None, ),
                                     ('pointer', icons['pointer'],
                                      self.parent.OnPointer,
                                      wx.ITEM_CHECK),
                                     ('query', icons['query'],
                                      self.parent.OnQuery,
                                      wx.ITEM_CHECK),
                                     ('pan', icons['pan'],
                                      self.parent.OnPan,
                                      wx.ITEM_CHECK),
                                     ('zoomin', icons['zoomIn'],
                                      self.parent.OnZoomIn,
                                      wx.ITEM_CHECK),
                                     ('zoomout', icons['zoomOut'],
                                      self.parent.OnZoomOut,
                                      wx.ITEM_CHECK),
                                     ('zoomextent', icons['zoomExtent'],
                                      self.parent.OnZoomToMap),
                                     ('zoomback', icons['zoomBack'],
                                      self.parent.OnZoomBack),
                                     ('zoommenu', icons['zoomMenu'],
                                      self.parent.OnZoomMenu),
                                     (None, ),
                                     ('analyze', icons['analyze'],
                                      self.parent.OnAnalyze),
                                     (None, ),
                                     ('dec', icons['overlay'],
                                      self.parent.OnDecoration),
                                     (None, ),
                                     ('savefile', icons['saveFile'],
                                      self.parent.SaveToFile),
                                     ('printmap', icons['print'],
                                      self.parent.PrintMenu),
                                     (None, ))
                                    )
    
    def OnSelectTool(self, event):
        """!Select / enable tool available in tools list
        """
        tool =  event.GetSelection()
        
        if tool == self.toolId['2d']:
            self.ExitToolbars()
            self.Enable2D(True)
        
        elif tool == self.toolId['3d'] and \
                not (self.parent.MapWindow3D and self.parent.IsPaneShown('3d')):
            self.ExitToolbars()
            self.parent.AddNviz()
            
        elif tool == self.toolId['vdigit'] and \
                not self.parent.toolbars['vdigit']:
            self.ExitToolbars()
            self.parent.AddToolbar("vdigit")
            self.parent.MapWindow.SetFocus()
        
    def ExitToolbars(self):
        if self.parent.toolbars['vdigit']:
            self.parent.toolbars['vdigit'].OnExit()
        if self.parent.GetLayerManager().IsPaneShown('toolbarNviz'):
            self.parent.RemoveNviz()
        
    def Enable2D(self, enabled):
        """!Enable/Disable 2D display mode specific tools"""
        for tool in (self.zoomin,
                     self.zoomout,
                     self.zoomback,
                     self.zoommenu,
                     self.analyze,
                     self.printmap):
            self.EnableTool(tool, enabled)
        
class GCPManToolbar(AbstractToolbar):
    """!Toolbar for managing ground control points

    @param parent reference to GCP widget
    """
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        icons = Icons['georectify']
        return self._getToolbarData((('gcpSave', icons["gcpSave"],
                                      self.parent.SaveGCPs),
                                     ('gcpReload', icons["gcpReload"],
                                      self.parent.ReloadGCPs),
                                     (None, ),
                                     ('gcpAdd', icons["gcpAdd"],
                                      self.parent.AddGCP),
                                     ('gcpDelete', icons["gcpDelete"],
                                      self.parent.DeleteGCP),
                                     ('gcpClear', icons["gcpClear"],
                                      self.parent.ClearGCP),
                                     (None, ),
                                     ('rms', icons["gcpRms"],
                                      self.parent.OnRMS),
                                     ('georect', icons["georectify"],
                                      self.parent.OnGeorect))
                                    )
    
class GCPDisplayToolbar(AbstractToolbar):
    """!GCP Display toolbar
    """
    def __init__(self, parent):
        """!GCP Display toolbar constructor
        """
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # add tool to toggle active map window
        self.togglemapid = wx.NewId()
        self.togglemap = wx.Choice(parent = self, id = self.togglemapid,
						    choices = [_('source'), _('target')],
						    style = wx.CB_READONLY)

        self.InsertControl(10, self.togglemap)

        self.SetToolShortHelp(self.togglemapid, '%s %s %s' % (_('Set map canvas for '),
                                                              Icons['displayWindow']["zoomBack"].GetLabel(),
                                                              _(' / Zoom to map')))

        # realize the toolbar
        self.Realize()
        
        self.action = { 'id' : self.gcpset }
        self.defaultAction = { 'id' : self.gcpset,
                               'bind' : self.parent.OnPointer }
        
        self.OnTool(None)
        
        self.EnableTool(self.zoomback, False)
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['displayWindow']
        return self._getToolbarData((("displaymap", icons["display"],
                                      self.parent.OnDraw),
                                     ("rendermap", icons["render"],
                                      self.parent.OnRender),
                                     ("erase", icons["erase"],
                                      self.parent.OnErase),
                                     (None, ),
                                     ("gcpset", Icons["georectify"]["gcpSet"],
                                      self.parent.OnPointer),
                                     ("pan", icons["pan"],
                                      self.parent.OnPan),
                                     ("zoomin", icons["zoomIn"],
                                      self.parent.OnZoomIn),
                                     ("zoomout", icons["zoomOut"],
                                      self.parent.OnZoomOut),
                                     ("zoommenu", icons["zoomMenu"],
                                      self.parent.OnZoomMenuGCP),
                                     (None, ),
                                     ("zoomback", icons["zoomBack"],
                                      self.parent.OnZoomBack),
                                     ("zoomtomap", icons["zoomExtent"],
                                      self.parent.OnZoomToMap),
                                     (None, ),
                                     ('settings', Icons["georectify"]["settings"],
                                      self.parent.OnSettings),
                                     ('help', Icons["misc"]["help"],
                                      self.parent.OnHelp),
                                     (None, ),
                                     ('quit', Icons["georectify"]["quit"],
                                      self.parent.OnQuit))
                                    )
    
class VDigitToolbar(AbstractToolbar):
    """!Toolbar for digitization
    """
    def __init__(self, parent, mapcontent, layerTree = None, log = None):
        self.mapcontent    = mapcontent # Map class instance
        self.layerTree     = layerTree  # reference to layer tree associated to map display
        self.log           = log        # log area
        AbstractToolbar.__init__(self, parent)
        self.digit         = None
        
        # currently selected map layer for editing (reference to MapLayer instance)
        self.mapLayer = None
        # list of vector layers from Layer Manager (only in the current mapset)
        self.layers   = [] 
        
        self.comboid    = None
        
        # only one dialog can be open
        self.settingsDialog   = None
        
        # create toolbars (two rows optionally)
        self.InitToolbar(self._toolbarData())
        self.Bind(wx.EVT_TOOL, self.OnTool)
        
        # default action (digitize new point, line, etc.)
        self.action = { 'desc' : 'addLine',
                        'type' : 'point',
                        'id'   : self.addPoint }
        
        # list of available vector maps
        self.UpdateListOfLayers(updateTool = True)
        
        # realize toolbar
        self.Realize()
        # workaround for Mac bug. May be fixed by 2.8.8, but not before then.
        self.combo.Hide()
        self.combo.Show()
        
        # disable undo/redo
        self.EnableTool(self.undo, False)
        
        # toogle to pointer by default
        self.OnTool(None)
        
        self.FixSize(width = 105)
        
    def _toolbarData(self):
        """!Toolbar data
        """
        data = []
        icons = Icons['vdigit']
        return self._getToolbarData(((None, ),
                                     ("addPoint", icons["addPoint"],
                                      self.OnAddPoint),
                                     ("addLine", icons["addLine"],
                                      self.OnAddLine,
                                      wx.ITEM_CHECK),
                                     ("addBoundary", icons["addBoundary"],
                                      self.OnAddBoundary,
                                      wx.ITEM_CHECK),
                                     ("addCentroid", icons["addCentroid"],
                                      self.OnAddCentroid,
                                      wx.ITEM_CHECK),
                                     ("addArea", icons["addArea"],
                                      self.OnAddArea,
                                      wx.ITEM_CHECK),
                                     ("moveVertex", icons["moveVertex"],
                                      self.OnMoveVertex,
                                      wx.ITEM_CHECK),
                                     ("addVertex", icons["addVertex"],
                                      self.OnAddVertex,
                                      wx.ITEM_CHECK),
                                     ("removeVertex", icons["removeVertex"],
                                      self.OnRemoveVertex,
                                      wx.ITEM_CHECK),
                                     ("editLine", icons["editLine"],
                                      self.OnEditLine,
                                      wx.ITEM_CHECK),
                                     ("moveLine", icons["moveLine"],
                                      self.OnMoveLine,
                                      wx.ITEM_CHECK),
                                     ("deleteLine", icons["deleteLine"],
                                      self.OnDeleteLine,
                                      wx.ITEM_CHECK),
                                     ("displayCats", icons["displayCats"],
                                      self.OnDisplayCats,
                                      wx.ITEM_CHECK),
                                     ("displayAttr", icons["displayAttr"],
                                      self.OnDisplayAttr,
                                      wx.ITEM_CHECK),
                                     ("additionalTools", icons["additionalTools"],
                                      self.OnAdditionalToolMenu,
                                      wx.ITEM_CHECK),                                      
                                     (None, ),
                                     ("undo", icons["undo"],
                                      self.OnUndo),
                                     ("settings", icons["settings"],
                                      self.OnSettings),
                                     ("quit", icons["quit"],
                                      self.OnExit))
                                    )
    
    def OnTool(self, event):
        """!Tool selected -> disable selected tool in map toolbar"""
        id = self.parent.toolbars['map'].GetAction(type = 'id')
        self.parent.toolbars['map'].ToggleTool(id, False)
        
        # set cursor
        cursor = self.parent.cursors["cross"]
        self.parent.MapWindow.SetCursor(cursor)
        
        # pointer
        self.parent.OnPointer(None)
        
        if event:
            # deselect previously selected tool
            id = self.action.get('id', -1)
            if id != event.GetId():
                self.ToggleTool(self.action['id'], False)
            else:
                self.ToggleTool(self.action['id'], True)
            
            self.action['id'] = event.GetId()
            
            event.Skip()
        
        self.ToggleTool(self.action['id'], True)
        
        # clear tmp canvas
        if self.action['id'] != id:
            self.parent.MapWindow.ClearLines(pdc = self.parent.MapWindow.pdcTmp)
            if self.digit and \
                    len(self.parent.MapWindow.digit.GetDisplay().GetSelected()) > 0:
                # cancel action
                self.parent.MapWindow.OnMiddleDown(None)
        
        # set focus
        self.parent.MapWindow.SetFocus()
        
    def OnAddPoint(self, event):
        """!Add point to the vector map Laier"""
        Debug.msg (2, "VDigitToolbar.OnAddPoint()")
        self.action = { 'desc' : "addLine",
                        'type' : "point",
                        'id'   : self.addPoint }
        self.parent.MapWindow.mouse['box'] = 'point'
        
    def OnAddLine(self, event):
        """!Add line to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddLine()")
        self.action = { 'desc' : "addLine",
                        'type' : "line",
                        'id'   : self.addLine }
        self.parent.MapWindow.mouse['box'] = 'line'
        ### self.parent.MapWindow.polycoords = [] # reset temp line
                
    def OnAddBoundary(self, event):
        """!Add boundary to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddBoundary()")
        if self.action['desc'] != 'addLine' or \
                self.action['type'] != 'boundary':
            self.parent.MapWindow.polycoords = [] # reset temp line
        self.action = { 'desc' : "addLine",
                        'type' : "boundary",
                        'id'   : self.addBoundary }
        self.parent.MapWindow.mouse['box'] = 'line'
        
    def OnAddCentroid(self, event):
        """!Add centroid to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddCentroid()")
        self.action = { 'desc' : "addLine",
                        'type' : "centroid",
                        'id'   : self.addCentroid }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnAddArea(self, event):
        """!Add area to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddCentroid()")
        self.action = { 'desc' : "addLine",
                        'type' : "area",
                        'id'   : self.addArea }
        self.parent.MapWindow.mouse['box'] = 'line'

    def OnExit (self, event=None):
        """!Quit digitization tool"""
        # stop editing of the currently selected map layer
        if self.mapLayer:
            self.StopEditing()
        
        # close dialogs if still open
        if self.settingsDialog:
            self.settingsDialog.OnCancel(None)
            
        # set default mouse settings
        self.parent.MapWindow.mouse['use'] = "pointer"
        self.parent.MapWindow.mouse['box'] = "point"
        self.parent.MapWindow.polycoords = []
        
        # disable the toolbar
        self.parent.RemoveToolbar("vdigit")
        
    def OnMoveVertex(self, event):
        """!Move line vertex"""
        Debug.msg(2, "Digittoolbar.OnMoveVertex():")
        self.action = { 'desc' : "moveVertex",
                        'id'   : self.moveVertex }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnAddVertex(self, event):
        """!Add line vertex"""
        Debug.msg(2, "Digittoolbar.OnAddVertex():")
        self.action = { 'desc' : "addVertex",
                        'id'   : self.addVertex }
        self.parent.MapWindow.mouse['box'] = 'point'
        
    def OnRemoveVertex(self, event):
        """!Remove line vertex"""
        Debug.msg(2, "Digittoolbar.OnRemoveVertex():")
        self.action = { 'desc' : "removeVertex",
                        'id'   : self.removeVertex }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnEditLine(self, event):
        """!Edit line"""
        Debug.msg(2, "Digittoolbar.OnEditLine():")
        self.action = { 'desc' : "editLine",
                        'id'   : self.editLine }
        self.parent.MapWindow.mouse['box'] = 'line'

    def OnMoveLine(self, event):
        """!Move line"""
        Debug.msg(2, "Digittoolbar.OnMoveLine():")
        self.action = { 'desc' : "moveLine",
                        'id'   : self.moveLine }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnDeleteLine(self, event):
        """!Delete line"""
        Debug.msg(2, "Digittoolbar.OnDeleteLine():")
        self.action = { 'desc' : "deleteLine",
                        'id'   : self.deleteLine }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnDisplayCats(self, event):
        """!Display/update categories"""
        Debug.msg(2, "Digittoolbar.OnDisplayCats():")
        self.action = { 'desc' : "displayCats",
                        'id'   : self.displayCats }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnDisplayAttr(self, event):
        """!Display/update attributes"""
        Debug.msg(2, "Digittoolbar.OnDisplayAttr():")
        self.action = { 'desc' : "displayAttrs",
                        'id'   : self.displayAttr }
        self.parent.MapWindow.mouse['box'] = 'point'
        
    def OnUndo(self, event):
        """!Undo previous changes"""
        self.digit.Undo()
        
        event.Skip()

    def EnableUndo(self, enable=True):
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
                self.digit = self.parent.MapWindow.digit = VDigit(mapwindow = self.parent.MapWindow)
            except SystemExit:
                self.digit = self.parent.MapWindow.digit = None
        
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
            (_('Duplicate attributes'),
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
            self.parent.MapWindow.Bind(wx.EVT_MENU, handler, item)
            if self.action['desc'] == desc:
                item.Check(True)
        
        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.parent.MapWindow.PopupMenu(toolMenu)
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
        self.parent.MapWindow.mouse['box'] = 'box'

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
        self.parent.MapWindow.mouse['box'] = 'point'


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
        self.parent.MapWindow.mouse['box'] = 'point'

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
        self.parent.MapWindow.mouse['box'] = 'point'
        

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
        self.parent.MapWindow.mouse['box'] = 'box'

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
        self.parent.MapWindow.mouse['box'] = 'box'

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
        self.parent.MapWindow.mouse['box'] = 'box'

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
        self.parent.MapWindow.mouse['box'] = 'box'

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
        self.parent.MapWindow.mouse['box'] = 'box'

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
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnZBulk(self, event):
        """!Z bulk-labeling selected lines/boundaries"""
        if not self.digit.IsVector3D():
            gcmd.GError(parent = self.parent,
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
        self.parent.MapWindow.mouse['box'] = 'line'

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
        self.parent.MapWindow.mouse['box'] = 'box'

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
            dlg = gdialogs.CreateNewVector(self.parent,
                                           exceptMap = openVectorMap, log = self.log,
                                           cmd = (('v.edit',
                                                   { 'tool' : 'create' },
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
        self.mapcontent.ChangeLayerActive(mapLayer, False)
        
        # clean map canvas
        self.parent.MapWindow.EraseMap()
        
        # unset background map if needed
        if mapLayer:
            if UserSettings.Get(group = 'vdigit', key = 'bgmap',
                                subkey = 'value', internal = True) == mapLayer.GetName():
                UserSettings.Set(group = 'vdigit', key = 'bgmap',
                                 subkey = 'value', value = '', internal = True)
            
            self.parent.statusbar.SetStatusText(_("Please wait, "
                                                  "opening vector map <%s> for editing...") % mapLayer.GetName(),
                                                0)
        
        self.parent.MapWindow.pdcVector = wx.PseudoDC()
        self.digit = self.parent.MapWindow.digit = VDigit(mapwindow = self.parent.MapWindow)
        
        self.mapLayer = mapLayer
        
        # open vector map
        if self.digit.OpenMap(mapLayer.GetName()) is None:
            self.mapLayer = None
            self.StopEditing()
            return False
        
        # update toolbar
        self.combo.SetValue(mapLayer.GetName())
        self.parent.toolbars['map'].combo.SetValue (_('Digitize'))
        lmgr = self.parent.GetLayerManager()
        if lmgr:
            lmgr.toolbars['tools'].Enable('vdigit', enable = False)
        
        Debug.msg (4, "VDigitToolbar.StartEditing(): layer=%s" % mapLayer.GetName())
        
        # change cursor
        if self.parent.MapWindow.mouse['use'] == 'pointer':
            self.parent.MapWindow.SetCursor(self.parent.cursors["cross"])
        
        if not self.parent.MapWindow.resize:
            self.parent.MapWindow.UpdateMap(render = True)
        
        opacity = mapLayer.GetOpacity(float = True)
        if opacity < 1.0:
            alpha = int(opacity * 255)
            self.digit.UpdateSettings(alpha)
        
        return True

    def StopEditing(self):
        """!Stop editing of selected vector map layer.

        @return True on success
        @return False on failure
        """
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
            
            self.parent.statusbar.SetStatusText(_("Please wait, "
                                                  "closing and rebuilding topology of "
                                                  "vector map <%s>...") % self.mapLayer.GetName(),
                                                0)
            lmgr = self.parent.GetLayerManager()
            if lmgr:
                lmgr.toolbars['tools'].Enable('vdigit', enable = True)
                lmgr.notebook.SetSelectionByName('output')
            self.digit.CloseMap()
            if lmgr:
                lmgr.GetLogWindow().GetProgressBar().SetValue(0)
                lmgr.GetLogWindow().WriteCmdLog(_("Editing of vector map <%s> successfully finished") % \
                                                    self.mapLayer.GetName())
            # re-active layer 
            item = self.parent.tree.FindItemByData('maplayer', self.mapLayer)
            if item and self.parent.tree.IsItemChecked(item):
                self.mapcontent.ChangeLayerActive(self.mapLayer, True)
        
        # change cursor
        self.parent.MapWindow.SetCursor(self.parent.cursors["default"])
        self.parent.MapWindow.pdcVector = None
        
        # close dialogs
        for dialog in ('attributes', 'category'):
            if self.parent.dialogs[dialog]:
                self.parent.dialogs[dialog].Close()
                self.parent.dialogs[dialog] = None
        
        del self.digit
        del self.parent.MapWindow.digit
        
        self.mapLayer = None
        
        self.parent.MapWindow.redrawAll = True
        
        return True
    
    def UpdateListOfLayers (self, updateTool = False):
        """!
        Update list of available vector map layers.
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
        self.layers = self.mapcontent.GetListOfLayers(l_type = "vector",
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
    
class ProfileToolbar(AbstractToolbar):
    """!Toolbar for profiling raster map
    """ 
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['profile']
        return self._getToolbarData((('addraster', Icons['layerManager']["addRast"],
                                      self.parent.OnSelectRaster),
                                     ('transect', icons["transect"],
                                      self.parent.OnDrawTransect),
                                     (None, ),
                                     ('draw', icons["draw"],
                                      self.parent.OnCreateProfile),
                                     ('erase', Icons['displayWindow']["erase"],
                                      self.parent.OnErase),
                                     ('drag', Icons['displayWindow']['pan'],
                                      self.parent.OnDrag),
                                     ('zoom', Icons['displayWindow']['zoomIn'],
                                      self.parent.OnZoom),
                                     ('unzoom', Icons['displayWindow']['zoomBack'],
                                      self.parent.OnRedraw),
                                     (None, ),
                                     ('datasave', icons["save"],
                                      self.parent.SaveProfileToFile),
                                     ('image', Icons['displayWindow']["saveFile"],
                                      self.parent.SaveToFile),
                                     ('print', Icons['displayWindow']["print"],
                                      self.parent.PrintMenu),
                                     (None, ),
                                     ('settings', icons["options"],
                                      self.parent.ProfileOptionsMenu),
                                     ('quit', icons["quit"],
                                      self.parent.OnQuit),
                                     ))
    
class LMNvizToolbar(AbstractToolbar):
    """!Nviz toolbar
    """
    def __init__(self, parent):
        self.lmgr = parent
        
        AbstractToolbar.__init__(self, parent)
        
        # only one dialog can be open
        self.settingsDialog   = None
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['nviz']
        return self._getToolbarData((("nviz_cmd", icons['nviz_cmd'],
                                      self.OnNvizCmd),
                                     (None, ),
                                     ("settings", icons["settings"],
                                      self.OnSettings),
                                     ("help", icons["help"],
                                      self.OnHelp))
                                    )
        
    def OnNvizCmd(self, event):
        """!Show nviz_cmd command"""
        self.lmgr.GetLayerTree().GetMapDisplay().GetWindow().OnNvizCmd()
        
    def OnHelp(self, event):
        """!Show 3D view mode help"""
        if not self.lmgr:
            gcmd.RunCommand('g.manual',
                            entry = 'wxGUI.Nviz')
        else:
            log = self.lmgr.GetLogWindow()
            log.RunCmd(['g.manual',
                        'entry=wxGUI.Nviz'])
        
    def OnSettings(self, event):
        """!Show nviz notebook page"""
        if not self.settingsDialog:
            self.settingsDialog = NvizPreferencesDialog(parent = self.parent)
        self.settingsDialog.Show()
        
class ModelToolbar(AbstractToolbar):
    """!Graphical modeler toolbar (see gmodeler.py)
    """
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['modeler']
        return self._getToolbarData((('new', icons['new'],
                                      self.parent.OnModelNew),
                                     ('open', icons['open'],
                                      self.parent.OnModelOpen),
                                     ('save', icons['save'],
                                      self.parent.OnModelSave),
                                     ('image', icons['toImage'],
                                      self.parent.OnExportImage),
                                     ('python', icons['toPython'],
                                      self.parent.OnExportPython),
                                     (None, ),
                                     ('action', icons['actionAdd'],
                                      self.parent.OnAddAction),
                                     ('data', icons['dataAdd'],
                                      self.parent.OnAddData),
                                     ('relation', icons['relation'],
                                      self.parent.OnDefineRelation),
                                     (None, ),
                                     ('redraw', icons['redraw'],
                                      self.parent.OnCanvasRefresh),
                                     ('validate', icons['validate'],
                                      self.parent.OnValidateModel),
                                     ('run', icons['run'],
                                      self.parent.OnRunModel),
                                     (None, ),
                                     ("variables", icons['variables'],
                                      self.parent.OnVariables),
                                     ("settings", icons['settings'],
                                      self.parent.OnPreferences),
                                     ("help", Icons['misc']['help'],
                                      self.parent.OnHelp),
                                     (None, ),
                                     ('quit', icons['quit'],
                                      self.parent.OnCloseWindow))
                                    )
    
class HistogramToolbar(AbstractToolbar):
    """!Histogram toolbar (see histogram.py)
    """
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['displayWindow']
        return self._getToolbarData((('histogram', icons["histogram"],
                                      self.parent.OnOptions),
                                     ('rendermao', icons["display"],
                                      self.parent.OnRender),
                                     ('erase', icons["erase"],
                                      self.parent.OnErase),
                                     ('font', Icons['misc']["font"],
                                      self.parent.SetHistFont),
                                     (None, ),
                                     ('save', icons["saveFile"],
                                      self.parent.SaveToFile),
                                     ('hprint', icons["print"],
                                      self.parent.PrintMenu),
                                     (None, ),
                                     ('quit', Icons['misc']["quit"],
                                      self.parent.OnQuit))
                                    )

class LMWorkspaceToolbar(AbstractToolbar):
    """!Layer Manager `workspace` toolbar
    """
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('newdisplay', icons["newdisplay"],
                                      self.parent.OnNewDisplay),
                                     ('newdisplay', icons["newdisplayWMS"],
                                      self.parent.OnNewDisplayWMS),
                                     (None, ),
                                     ('workspaceNew', icons["workspaceNew"],
                                      self.parent.OnWorkspaceNew),
                                     ('workspaceOpen', icons["workspaceOpen"],
                                      self.parent.OnWorkspaceOpen),
                                     ('workspaceSave', icons["workspaceSave"],
                                      self.parent.OnWorkspaceSave),
                                     ))

class LMDataToolbar(AbstractToolbar):
    """!Layer Manager `data` toolbar
    """
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('addMulti', icons["addMulti"],
                                      self.parent.OnAddMaps),
                                     ('addrast', icons["addRast"],
                                      self.parent.OnAddRaster),
                                     ('rastmisc', icons["rastMisc"],
                                      self.parent.OnAddRasterMisc),
                                     ('addvect', icons["addVect"],
                                      self.parent.OnAddVector),
                                     ('vectmisc', icons["vectMisc"],
                                      self.parent.OnAddVectorMisc),
                                     ('addgrp',  icons["addGroup"],
                                      self.parent.OnAddGroup),
                                     ('addovl',  icons["addOverlay"],
                                      self.parent.OnAddOverlay),
                                     ('delcmd',  icons["delCmd"],
                                      self.parent.OnDeleteLayer),
                                     ))

class LMToolsToolbar(AbstractToolbar):
    """!Layer Manager `tools` toolbar
    """
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('importMap', icons["import"],
                                      self.parent.OnImportMenu),
                                     (None, ),
                                     ('mapCalc', icons["mapcalc"],
                                      self.parent.OnMapCalculator),
                                     ('georect', Icons["georectify"]["georectify"],
                                      self.parent.OnGCPManager),
                                     ('modeler', icons["modeler"],
                                      self.parent.OnGModeler),
                                     ('mapOutput', icons['mapOutput'],
                                      self.parent.OnPsMap)
                                     ))

class LMMiscToolbar(AbstractToolbar):
    """!Layer Manager `misc` toolbar
    """
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('settings', icons["settings"],
                                      self.parent.OnPreferences),
                                     ('help', Icons["misc"]["help"],
                                      self.parent.OnHelp),
                                     ))

class LMVectorToolbar(AbstractToolbar):
    """!Layer Manager `vector` toolbar
    """
    def __init__(self, parent):
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('vdigit', icons["vdigit"],
                                      self.parent.OnVDigit),
                                     ('attribute', icons["attrTable"],
                                      self.parent.OnShowAttributeTable),
                                     ))
    
class PsMapToolbar(AbstractToolbar):
    def __init__(self, parent):
        """!Toolbar Hardcopy Map Output Utility (psmap.py)
        
        @param parent parent window
        """
        AbstractToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        self.Realize()
        
        self.action = { 'id' : self.pointer }
        self.defaultAction = { 'id' : self.pointer,
                               'bind' : self.parent.OnPointer }
        self.OnTool(None)
        
        from psmap import haveImage
        if not haveImage:
            self.EnableTool(self.preview, False)
        
    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['psMap']
        return self._getToolbarData((('loadFile', icons['scriptLoad'],
                                      self.parent.OnLoadFile),                                    
                                     ('instructionFile', icons['scriptSave'],
                                      self.parent.OnInstructionFile),
                                     (None, ),
                                     ('pagesetup', icons['pageSetup'],
                                      self.parent.OnPageSetup),
                                     (None, ),
                                     ("pointer", Icons["displayWindow"]["pointer"],
                                      self.parent.OnPointer, wx.ITEM_CHECK),
                                     ('pan', Icons["displayWindow"]['pan'],
                                      self.parent.OnPan, wx.ITEM_CHECK),
                                     ("zoomin", Icons["displayWindow"]["zoomIn"],
                                      self.parent.OnZoomIn, wx.ITEM_CHECK),
                                     ("zoomout", Icons["displayWindow"]["zoomOut"],
                                      self.parent.OnZoomOut, wx.ITEM_CHECK),
                                     ('zoomAll', icons['fullExtent'],
                                      self.parent.OnZoomAll),
                                     (None, ),
                                     ('addMap', icons['addMap'],
                                      self.parent.OnAddMap, wx.ITEM_CHECK),
                                     ('addRaster', icons['addRast'],
                                      self.parent.OnAddRaster),
                                     ('addVector', icons['addVect'],
                                      self.parent.OnAddVect),
                                     ("dec", Icons["displayWindow"]["overlay"],
                                      self.parent.OnDecoration),
                                     ("delete", icons["deleteObj"],
                                      self.parent.OnDelete),
                                     (None, ),
                                     ("preview", icons["preview"],
                                      self.parent.OnPreview),
                                     ('generatePS', icons['psExport'],
                                      self.parent.OnPSFile),
                                     ('generatePDF', icons['pdfExport'],
                                      self.parent.OnPDFFile),
                                     (None, ),
                                     ("help", Icons['misc']['help'],
                                      self.parent.OnHelp),
                                     ('quit', icons['quit'],
                                      self.parent.OnCloseWindow))
                                    )
