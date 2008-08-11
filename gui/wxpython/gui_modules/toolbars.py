"""
MODULE: toolbar

CLASSES:
    * AbstractToolbar
    * MapToolbar
    * GRToolbar
    * GCPToolbar
    * VDigitToolbar
    * ProfileToolbar
    * NvizToolbar

PURPOSE: Toolbars for Map Display window

AUTHORS: The GRASS Development Team
         Michael Barton, Martin Landa, Jachym Cepicky

COPYRIGHT: (C) 2007-2008 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

import wx
import os, sys

import globalvar
import gcmd
import grassenv
import gdialogs
import vdigit
from vdigit import VDigitSettingsDialog as VDigitSettingsDialog
from debug import Debug as Debug
from icon import Icons as Icons
from preferences import globalSettings as UserSettings

gmpath = os.path.join(globalvar.ETCWXDIR, "icons")
sys.path.append(gmpath)

class AbstractToolbar(object):
    """Abstract toolbar class"""
    def __init__():
        pass
        
    def InitToolbar(self, parent, toolbar, toolData):
        """Initialize toolbar, i.e. add tools to the toolbar

        @return list of ids (of added tools)
        """

        for tool in toolData:
            self.CreateTool(parent, toolbar, *tool)

        self._toolbar = toolbar
        self._data = toolData
        
    def ToolbarData(self):
        """Toolbar data"""

        return None

    def CreateTool(self, parent, toolbar, tool, label, bitmap, kind,
                   shortHelp, longHelp, handler):
        """Add tool to the toolbar

        @return id of tool
        """

        bmpDisabled=wx.NullBitmap

        if label:
            toolWin = toolbar.AddLabelTool(tool, label, bitmap,
                                           bmpDisabled, kind,
                                           shortHelp, longHelp)
            parent.Bind(wx.EVT_TOOL, handler, toolWin)
        else: # add separator
            toolbar.AddSeparator()

        return tool

    def GetToolbar(self):
        """Get toolbar widget reference"""
        return self._toolbar

    def EnableLongHelp(self, enable=True):
        """Enable/disable long help

        @param enable True for enable otherwise disable
        """
        for tool in self._data:
            if tool[0] == '': # separator
                continue
            
            if enable:
                self._toolbar.SetToolLongHelp(tool[0], tool[5])
            else:
                self._toolbar.SetToolLongHelp(tool[0], "")
                
class MapToolbar(AbstractToolbar):
    """
    Main Map Display toolbar
    """

    def __init__(self, mapdisplay, map):
        self.mapcontent = map
        self.mapdisplay = mapdisplay

        self.toolbar = wx.ToolBar(parent=self.mapdisplay, id=wx.ID_ANY)
        self.toolbar.SetToolBitmapSize(globalvar.toolbarSize)

        self.InitToolbar(self.mapdisplay, self.toolbar, self.ToolbarData())
        
        # optional tools
        self.combo = wx.ComboBox(parent=self.toolbar, id=wx.ID_ANY, value='Tools',
                                 choices=['Digitize', 'Nviz'], style=wx.CB_READONLY, size=(90, -1))

        self.comboid = self.toolbar.AddControl(self.combo)
        self.mapdisplay.Bind(wx.EVT_COMBOBOX, self.OnSelectTool, self.comboid)

        # realize the toolbar
        self.toolbar.Realize()
        
        #workaround for Mac bug. May be fixed by 2.8.8, but not before then.
        self.combo.Hide()
        self.combo.Show()
            
    def ToolbarData(self):
        """Toolbar data"""

        self.displaymap = wx.NewId()
        self.rendermap = wx.NewId()
        self.erase = wx.NewId()
        self.pointer = wx.NewId()
        self.query = wx.NewId()
        self.pan = wx.NewId()
        self.zoomin = wx.NewId()
        self.zoomout = wx.NewId()
        self.zoomback = wx.NewId()
        self.zoommenu = wx.NewId()
        self.analyze = wx.NewId()
        self.dec = wx.NewId()
        self.savefile = wx.NewId()
        self.printmap = wx.NewId()

        # tool, label, bitmap, kind, shortHelp, longHelp, handler
        return (
            (self.displaymap, "displaymap", Icons["displaymap"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["displaymap"].GetLabel(), Icons["displaymap"].GetDesc(),
             self.mapdisplay.OnDraw),
            (self.rendermap, "rendermap", Icons["rendermap"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["rendermap"].GetLabel(), Icons["rendermap"].GetDesc(),
             self.mapdisplay.OnRender),
            (self.erase, "erase", Icons["erase"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["erase"].GetLabel(), Icons["erase"].GetDesc(),
             self.mapdisplay.OnErase),
            ("", "", "", "", "", "", ""),
            (self.pointer, "pointer", Icons["pointer"].GetBitmap(),
             wx.ITEM_RADIO, Icons["pointer"].GetLabel(), Icons["pointer"].GetDesc(),
             self.mapdisplay.OnPointer),
            (self.query, "queryDisplay", Icons["queryDisplay"].GetBitmap(),
             wx.ITEM_RADIO, Icons["queryDisplay"].GetLabel(), Icons["queryDisplay"].GetDesc(),
             self.mapdisplay.OnQuery),
            (self.pan, "pan", Icons["pan"].GetBitmap(),
             wx.ITEM_RADIO, Icons["pan"].GetLabel(), Icons["pan"].GetDesc(),
             self.mapdisplay.OnPan),
            (self.zoomin, "zoom_in", Icons["zoom_in"].GetBitmap(),
             wx.ITEM_RADIO, Icons["zoom_in"].GetLabel(), Icons["zoom_in"].GetDesc(),
             self.mapdisplay.OnZoomIn),
            (self.zoomout, "zoom_out", Icons["zoom_out"].GetBitmap(),
             wx.ITEM_RADIO, Icons["zoom_out"].GetLabel(), Icons["zoom_out"].GetDesc(),
             self.mapdisplay.OnZoomOut),
            (self.zoomback, "zoom_back", Icons["zoom_back"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["zoom_back"].GetLabel(), Icons["zoom_back"].GetDesc(),
             self.mapdisplay.OnZoomBack),
            (self.zoommenu, "zoommenu", Icons["zoommenu"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["zoommenu"].GetLabel(), Icons["zoommenu"].GetDesc(),
             self.mapdisplay.OnZoomMenu),
            ("", "", "", "", "", "", ""),
            (self.analyze, "analyze", Icons["analyze"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["analyze"].GetLabel(), Icons["analyze"].GetDesc(),
             self.mapdisplay.OnAnalyze),
            ("", "", "", "", "", "", ""),
            (self.dec, "overlay", Icons["overlay"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["overlay"].GetLabel(), Icons["overlay"].GetDesc(),
             self.mapdisplay.OnDecoration),
            ("", "", "", "", "", "", ""),
            (self.savefile, "savefile", Icons["savefile"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["savefile"].GetLabel(), Icons["savefile"].GetDesc(),
             self.mapdisplay.SaveToFile),
            (self.printmap, "printmap", Icons["printmap"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["printmap"].GetLabel(), Icons["printmap"].GetDesc(),
             self.mapdisplay.PrintMenu),
            ("", "", "", "", "", "", "")
            )

    def OnSelectTool(self, event):
        """
        Select / enable tool available in tools list
        """
        tool =  event.GetString()

        if tool == "Digitize" and not self.mapdisplay.toolbars['vdigit']:
            self.mapdisplay.AddToolbar("vdigit")

        elif tool == "Nviz" and not self.mapdisplay.toolbars['nviz']:
            self.mapdisplay.AddToolbar("nviz")

    def Enable2D(self, enabled):
        """Enable/Disable 2D display mode specific tools"""
        for tool in (self.pointer,
                     self.query,
                     self.pan,
                     self.zoomin,
                     self.zoomout,
                     self.zoomback,
                     self.zoommenu,
                     self.analyze,
                     self.dec,
                     self.savefile,
                     self.printmap):
            self.toolbar.EnableTool(tool, enabled)

class GRToolbar(AbstractToolbar):
    """
    Georectification Display toolbar
    """

    def __init__(self, mapdisplay, map):
        self.mapcontent = map
        self.mapdisplay = mapdisplay

        self.toolbar = wx.ToolBar(parent=self.mapdisplay, id=wx.ID_ANY)

        # self.SetToolBar(self.toolbar)
        self.toolbar.SetToolBitmapSize(globalvar.toolbarSize)

        self.InitToolbar(self.mapdisplay, self.toolbar, self.ToolbarData())

        # realize the toolbar
        self.toolbar.Realize()

    def ToolbarData(self):
        """Toolbar data"""

        self.displaymap = wx.NewId()
        self.rendermap = wx.NewId()
        self.erase = wx.NewId()
        self.gcpset = wx.NewId()
        self.pan = wx.NewId()
        self.zoomin = wx.NewId()
        self.zoomout = wx.NewId()
        self.zoomback = wx.NewId()
        self.zoommenu = wx.NewId()

        # tool, label, bitmap, kind, shortHelp, longHelp, handler
        return (
            (self.displaymap, "displaymap", Icons["displaymap"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["displaymap"].GetLabel(), Icons["displaymap"].GetDesc(),
             self.mapdisplay.OnDraw),
            (self.rendermap, "rendermap", Icons["rendermap"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["rendermap"].GetLabel(), Icons["rendermap"].GetDesc(),
             self.mapdisplay.OnRender),
            (self.erase, "erase", Icons["erase"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["erase"].GetLabel(), Icons["erase"].GetDesc(),
             self.mapdisplay.OnErase),
            ("", "", "", "", "", "", ""),
            (self.gcpset, "grGcpSet", Icons["grGcpSet"].GetBitmap(),
             wx.ITEM_RADIO, Icons["grGcpSet"].GetLabel(), Icons["grGcpSet"].GetDesc(),
             self.mapdisplay.OnPointer),
            (self.pan, "pan", Icons["pan"].GetBitmap(),
             wx.ITEM_RADIO, Icons["pan"].GetLabel(), Icons["pan"].GetDesc(),
             self.mapdisplay.OnPan),
            (self.zoomin, "zoom_in", Icons["zoom_in"].GetBitmap(),
             wx.ITEM_RADIO, Icons["zoom_in"].GetLabel(), Icons["zoom_in"].GetDesc(),
             self.mapdisplay.OnZoomIn),
            (self.zoomout, "zoom_out", Icons["zoom_out"].GetBitmap(),
             wx.ITEM_RADIO, Icons["zoom_out"].GetLabel(), Icons["zoom_out"].GetDesc(),
             self.mapdisplay.OnZoomOut),
            (self.zoomback, "zoom_back", Icons["zoom_back"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["zoom_back"].GetLabel(), Icons["zoom_back"].GetDesc(),
             self.mapdisplay.OnZoomBack),
            (self.zoommenu, "zoommenu", Icons["zoommenu"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["zoommenu"].GetLabel(), Icons["zoommenu"].GetDesc(),
             self.mapdisplay.OnZoomMenu),
            ("", "", "", "", "", "", ""),
            )

class GCPToolbar(AbstractToolbar):
    """
    Toolbar for managing ground control points during georectification
    """
    def __init__(self, parent, tbframe):
        self.parent  = parent # GCP
        self.tbframe = tbframe

        self.toolbar = wx.ToolBar(parent=self.tbframe, id=wx.ID_ANY)

        # self.SetToolBar(self.toolbar)
        self.toolbar.SetToolBitmapSize(globalvar.toolbarSize)

        self.InitToolbar(self.tbframe, self.toolbar, self.ToolbarData())

        # realize the toolbar
        self.toolbar.Realize()

    def ToolbarData(self):
        
        self.gcpSave = wx.NewId()
        self.gcpAdd = wx.NewId()
        self.gcpDelete = wx.NewId()
        self.gcpClear = wx.NewId()
        self.gcpReload = wx.NewId()
        self.rms = wx.NewId()
        self.georect = wx.NewId()
        self.settings = wx.NewId()
        self.quit = wx.NewId()

        return (
            (self.gcpSave, 'grGcpSave', Icons["grGcpSave"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["grGcpSave"].GetLabel(), Icons["grGcpSave"].GetDesc(),
             self.parent.SaveGCPs),
            (self.gcpAdd, 'grGrGcpAdd', Icons["grGcpAdd"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["grGcpAdd"].GetLabel(), Icons["grGcpAdd"].GetDesc(),
             self.parent.AddGCP),
            (self.gcpDelete, 'grGrGcpDelete', Icons["grGcpDelete"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["grGcpDelete"].GetLabel(), Icons["grGcpDelete"].GetDesc(), 
             self.parent.DeleteGCP),
            (self.gcpClear, 'grGcpClear', Icons["grGcpClear"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["grGcpClear"].GetLabel(), Icons["grGcpClear"].GetDesc(), 
             self.parent.ClearGCP),
            (self.gcpReload, 'grGcpReload', Icons["grGcpReload"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["grGcpReload"].GetLabel(), Icons["grGcpReload"].GetDesc(), 
             self.parent.ReloadGCPs),

            ("", "", "", "", "", "", ""),
            (self.rms, 'grGcpRms', Icons["grGcpRms"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["grGcpRms"].GetLabel(), Icons["grGcpRms"].GetDesc(),
             self.parent.OnRMS),
            (self.georect, 'grGeorect', Icons["grGeorect"].GetBitmap(), 
             wx.ITEM_NORMAL, Icons["grGeorect"].GetLabel(), Icons["grGeorect"].GetDesc(),
             self.parent.OnGeorect),
            ("", "", "", "", "", "", ""),
            (self.settings, 'grSettings', Icons["grSettings"].GetBitmap(), 
             wx.ITEM_NORMAL, Icons["grSettings"].GetLabel(), Icons["grSettings"].GetDesc(),
             self.parent.OnSettings),
            (self.quit, 'grGcpQuit', Icons["grGcpQuit"].GetBitmap(), 
             wx.ITEM_NORMAL, Icons["grGcpQuit"].GetLabel(), Icons["grGcpQuit"].GetDesc(),
             self.parent.OnQuit)
            )
    
class VDigitToolbar(AbstractToolbar):
    """
    Toolbar for digitization
    """

    def __init__(self, parent, map, layerTree=None, log=None):
        self.mapcontent    = map       # Map class instance
        self.parent        = parent    # MapFrame
        self.layerTree     = layerTree # reference to layer tree associated to map display
        self.log           = log       # log area
        
        # currently selected map layer for editing (reference to MapLayer instance)
        self.mapLayer = None
        # list of vector layers from Layer Manager (only in the current mapset)
        self.layers   = [] 

        self.comboid    = None

        # only one dialog can be open
        self.settingsDialog   = None

        # create toolbars (two rows optionaly)
        self.toolbar = []
        self.numOfRows = 1 # number of rows for toolbar
        for row in range(0, self.numOfRows):
            self.toolbar.append(wx.ToolBar(parent=self.parent, id=wx.ID_ANY))
            self.toolbar[row].SetToolBitmapSize(globalvar.toolbarSize)
            self.toolbar[row].Bind(wx.EVT_TOOL, self.OnTool)
            
            # create toolbar
            if self.numOfRows ==  1:
                rowdata=None
            else:
                rowdata = row
            self.InitToolbar(self.parent, self.toolbar[row], self.ToolbarData(rowdata))

        # default action (digitize new point, line, etc.)
        self.action = { 'desc' : 'addLine',
                        'type' : 'point',
                        'id'   : self.addPoint }
        
        # list of available vector maps
        self.UpdateListOfLayers(updateTool=True)

        # realize toolbar
        for row in range(0, self.numOfRows):
            self.toolbar[row].Realize()

        # disable undo/redo
        self.toolbar[0].EnableTool(self.undo, False)
        
        # toogle to pointer by default
        self.OnTool(None)

        if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vdigit':
            self.toolbar[0].EnableTool(self.copyCats, False) # not implemented (TODO)
            self.toolbar[0].SetToolShortHelp(self.copyCats, _("Not implemented yet"))
            
    def ToolbarData(self, row=None):
        """
        Toolbar data
        """
        data = []
        if row is None or row == 0:
            self.addPoint = wx.NewId()
            self.addLine = wx.NewId()
            self.addBoundary = wx.NewId()
            self.addCentroid = wx.NewId()
            self.moveVertex = wx.NewId()
            self.addVertex = wx.NewId()
            self.removeVertex = wx.NewId()
            self.splitLine = wx.NewId()
            self.editLine = wx.NewId()
            self.moveLine = wx.NewId()
            self.deleteLine = wx.NewId()
            self.additionalTools = wx.NewId()
            self.displayCats = wx.NewId()
            self.displayAttr = wx.NewId()
            self.copyCats = wx.NewId()

            data = [("", "", "", "", "", "", ""),
                    (self.addPoint, "digAddPoint", Icons["digAddPoint"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digAddPoint"].GetLabel(), Icons["digAddPoint"].GetDesc(),
                     self.OnAddPoint),
                    (self.addLine, "digAddLine", Icons["digAddLine"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digAddLine"].GetLabel(), Icons["digAddLine"].GetDesc(),
                     self.OnAddLine),
                    (self.addBoundary, "digAddBoundary", Icons["digAddBoundary"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digAddBoundary"].GetLabel(), Icons["digAddBoundary"].GetDesc(),
                     self.OnAddBoundary),
                    (self.addCentroid, "digAddCentroid", Icons["digAddCentroid"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digAddCentroid"].GetLabel(), Icons["digAddCentroid"].GetDesc(),
                     self.OnAddCentroid),
                    (self.moveVertex, "digMoveVertex", Icons["digMoveVertex"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digMoveVertex"].GetLabel(), Icons["digMoveVertex"].GetDesc(),
                     self.OnMoveVertex),
                    (self.addVertex, "digAddVertex", Icons["digAddVertex"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digAddVertex"].GetLabel(), Icons["digAddVertex"].GetDesc(),
                     self.OnAddVertex),
                    (self.removeVertex, "digRemoveVertex", Icons["digRemoveVertex"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digRemoveVertex"].GetLabel(), Icons["digRemoveVertex"].GetDesc(),
                     self.OnRemoveVertex),
                    (self.splitLine, "digSplitLine", Icons["digSplitLine"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digSplitLine"].GetLabel(), Icons["digSplitLine"].GetDesc(),
                     self.OnSplitLine),
                    (self.editLine, "digEditLine", Icons["digEditLine"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digEditLine"].GetLabel(), Icons["digEditLine"].GetDesc(),
                     self.OnEditLine),
                    (self.moveLine, "digMoveLine", Icons["digMoveLine"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digMoveLine"].GetLabel(), Icons["digMoveLine"].GetDesc(),
                     self.OnMoveLine),
                    (self.deleteLine, "digDeleteLine", Icons["digDeleteLine"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digDeleteLine"].GetLabel(), Icons["digDeleteLine"].GetDesc(),
                     self.OnDeleteLine),
                    (self.displayCats, "digDispCats", Icons["digDispCats"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digDispCats"].GetLabel(), Icons["digDispCats"].GetDesc(),
                     self.OnDisplayCats),
                    (self.copyCats, "digCopyCats", Icons["digCopyCats"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digCopyCats"].GetLabel(), Icons["digCopyCats"].GetDesc(),
                     self.OnCopyCats),
                    (self.displayAttr, "digDispAttr", Icons["digDispAttr"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digDispAttr"].GetLabel(), Icons["digDispAttr"].GetDesc(),
                     self.OnDisplayAttr),
                    (self.additionalTools, "digAdditionalTools", Icons["digAdditionalTools"].GetBitmap(),
                     wx.ITEM_CHECK, Icons["digAdditionalTools"].GetLabel(),
                     Icons["digAdditionalTools"].GetDesc(),
                     self.OnAdditionalToolMenu)]

        if row is None or row == 1:
            self.undo = wx.NewId()
            self.settings = wx.NewId()
            self.exit = wx.NewId()

            data.append(("", "", "", "", "", "", ""))
            data.append((self.undo, "digUndo", Icons["digUndo"].GetBitmap(),
                         wx.ITEM_NORMAL, Icons["digUndo"].GetLabel(), Icons["digUndo"].GetDesc(),
                         self.OnUndo))
            # data.append((self.undo, "digRedo", Icons["digRedo"].GetBitmap(),
            #             wx.ITEM_NORMAL, Icons["digRedo"].GetLabel(), Icons["digRedo"].GetDesc(),
            #             self.OnRedo))
            data.append((self.settings, "digSettings", Icons["digSettings"].GetBitmap(),
                         wx.ITEM_NORMAL, Icons["digSettings"].GetLabel(), Icons["digSettings"].GetDesc(),
                         self.OnSettings))
            data.append((self.exit, "digExit", Icons["quit"].GetBitmap(),
                         wx.ITEM_NORMAL, Icons["digExit"].GetLabel(), Icons["digExit"].GetDesc(),
                         self.OnExit))

        return data

    def OnTool(self, event):
        """Tool selected -> toggle tool to pointer"""
        id = self.parent.toolbars['map'].pointer
        self.parent.toolbars['map'].toolbar.ToggleTool(id, True)
        self.parent.toolbars['map'].mapdisplay.OnPointer(event)

        if event:
            # deselect previously selected tool
            if self.action.has_key('id'):
                self.toolbar[0].ToggleTool(self.action['id'], False)
        
            self.action['id'] = event.GetId()
            event.Skip()
        else:
            # initialize toolbar
            self.toolbar[0].ToggleTool(self.action['id'], True)
        
    def OnAddPoint(self, event):
        """Add point to the vector map Laier"""
        Debug.msg (2, "VDigitToolbar.OnAddPoint()")
        self.action = { 'desc' : "addLine",
                        'type' : "point",
                        'id'   : self.addPoint }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnAddLine(self, event):
        """Add line to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddLine()")
        self.action = { 'desc' : "addLine",
                        'type' : "line",
                        'id'   : self.addLine }
        self.parent.MapWindow.mouse['box'] = 'line'
        self.parent.MapWindow.polycoords = [] # reset temp line

    def OnAddBoundary(self, event):
        """Add boundary to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddBoundary()")
        self.action = { 'desc' : "addLine",
                        'type' : "boundary",
                        'id'   : self.addBoundary }
        self.parent.MapWindow.mouse['box'] = 'line'
        self.parent.MapWindow.polycoords = [] # reset temp line

    def OnAddCentroid(self, event):
        """Add centroid to the vector map layer"""
        Debug.msg (2, "VDigitToolbar.OnAddCentroid()")
        self.action = { 'desc' : "addLine",
                        'type' : "centroid",
                        'id'   : self.addCentroid }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnExit (self, event=None):
        """Quit digitization tool"""
        # stop editing of the currently selected map layer
        if self.mapLayer:
            self.StopEditing()

        # close dialogs if still open
        if self.settingsDialog:
            self.settingsDialog.OnCancel(None)

        if self.parent.dialogs['category']:
            self.parent.dialogs['category'].OnCancel(None)

        if self.parent.dialogs['attributes']:
            self.parent.dialogs['attributes'].OnCancel(None)

        # disable the toolbar
        self.parent.RemoveToolbar ("vdigit")

    def OnMoveVertex(self, event):
        """Move line vertex"""
        Debug.msg(2, "Digittoolbar.OnMoveVertex():")
        self.action = { 'desc' : "moveVertex",
                        'id'   : self.moveVertex }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnAddVertex(self, event):
        """Add line vertex"""
        Debug.msg(2, "Digittoolbar.OnAddVertex():")
        self.action = { 'desc' : "addVertex",
                        'id'   : self.addVertex }
        self.parent.MapWindow.mouse['box'] = 'point'


    def OnRemoveVertex(self, event):
        """Remove line vertex"""
        Debug.msg(2, "Digittoolbar.OnRemoveVertex():")
        self.action = { 'desc' : "removeVertex",
                        'id'   : self.removeVertex }
        self.parent.MapWindow.mouse['box'] = 'point'


    def OnSplitLine(self, event):
        """Split line"""
        Debug.msg(2, "Digittoolbar.OnSplitLine():")
        self.action = { 'desc' : "splitLine",
                        'id'   : self.splitLine }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnEditLine(self, event):
        """Edit line"""
        Debug.msg(2, "Digittoolbar.OnEditLine():")
        self.action = { 'desc' : "editLine",
                        'id'   : self.editLine }
        self.parent.MapWindow.mouse['box'] = 'line'

    def OnMoveLine(self, event):
        """Move line"""
        Debug.msg(2, "Digittoolbar.OnMoveLine():")
        self.action = { 'desc' : "moveLine",
                        'id'   : self.moveLine }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnDeleteLine(self, event):
        """Delete line"""
        Debug.msg(2, "Digittoolbar.OnDeleteLine():")
        self.action = { 'desc' : "deleteLine",
                        'id'   : self.deleteLine }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnDisplayCats(self, event):
        """Display/update categories"""
        Debug.msg(2, "Digittoolbar.OnDisplayCats():")
        self.action = { 'desc' : "displayCats",
                        'id'   : self.displayCats }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnDisplayAttr(self, event):
        """Display/update attributes"""
        Debug.msg(2, "Digittoolbar.OnDisplayAttr():")
        self.action = { 'desc' : "displayAttrs",
                        'id'   : self.displayAttr }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnCopyCats(self, event):
        """Copy categories"""
        Debug.msg(2, "Digittoolbar.OnCopyCats():")
        self.action = { 'desc' : "copyCats",
                        'id'   : self.copyCats }
        self.parent.MapWindow.mouse['box'] = 'point'

    def OnUndo(self, event):
        """Undo previous changes"""
        self.parent.digit.Undo()

        event.Skip()

    def EnableUndo(self, enable=True):
        """Enable 'Undo' in toolbar

        @param enable False for disable
        """
        ### fix undo first...

        #         if enable:
        #             if self.toolbar[0].GetToolEnabled(self.undo) is False:
        #                 self.toolbar[0].EnableTool(self.undo, True)
        #         else:
        #             if self.toolbar[0].GetToolEnabled(self.undo) is True:
        #                 self.toolbar[0].EnableTool(self.undo, False)
        pass

    def OnSettings(self, event):
        """Show settings dialog"""

        if self.parent.digit is None:
            reload(vdigit)
            from vdigit import Digit as Digit
            self.parent.digit = Digit(mapwindow=self.parent.MapWindow)
            
        if not self.settingsDialog:
            self.settingsDialog = VDigitSettingsDialog(parent=self.parent, title=_("Digitization settings"),
                                                       style=wx.DEFAULT_DIALOG_STYLE)
            self.settingsDialog.Show()

    def OnAdditionalToolMenu(self, event):
        """Menu for additional tools"""
        point = wx.GetMousePosition()
        toolMenu = wx.Menu()
        # Add items to the menu
        copy = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                           text=_('Copy features from (background) vector map'),
                           kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(copy)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnCopy, copy)
        if self.action['desc'] == "copyLine":
            copy.Check(True)

        flip = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                           text=_('Flip selected lines/boundaries'),
                           kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(flip)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnFlip, flip)
        if self.action['desc'] == "flipLine":
            flip.Check(True)

        merge = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                            text=_('Merge selected lines/boundaries'),
                            kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(merge)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnMerge, merge)
        if self.action['desc'] == "mergeLine":
            merge.Check(True)

        breakL = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                             text=_('Break selected lines/boundaries at intersection'),
                             kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(breakL)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnBreak, breakL)
        if self.action['desc'] == "breakLine":
            breakL.Check(True)

        snap = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                           text=_('Snap selected lines/boundaries (only to nodes)'),
                           kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(snap)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnSnap, snap)
        if self.action['desc'] == "snapLine":
            snap.Check(True)

        connect = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                              text=_('Connect selected lines/boundaries'),
                              kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(connect)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnConnect, connect)
        if self.action['desc'] == "connectLine":
            connect.Check(True)

        query = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                            text=_('Query features'),
                            kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(query)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnQuery, query)
        if self.action['desc'] == "queryLine":
            query.Check(True)

        zbulk = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                            text=_('Z bulk-labeling of 3D lines'),
                            kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(zbulk)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnZBulk, zbulk)
        if self.action['desc'] == "zbulkLine":
            zbulk.Check(True)

        typeconv = wx.MenuItem(parentMenu=toolMenu, id=wx.ID_ANY,
                               text=_('Feature type conversion'),
                               kind=wx.ITEM_CHECK)
        toolMenu.AppendItem(typeconv)
        self.parent.MapWindow.Bind(wx.EVT_MENU, self.OnTypeConversion, typeconv)
        if self.action['desc'] == "typeConv":
            typeconv.Check(True)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.parent.MapWindow.PopupMenu(toolMenu)
        toolMenu.Destroy()

        id = self.parent.toolbars['vdigit'].additionalTools
        self.parent.toolbars['vdigit'].toolbar[0].ToggleTool(id, True)
        
    def OnCopy(self, event):
        """Copy selected features from (background) vector map"""
        if self.action['desc'] == 'copyLine': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnCopy():")
        self.action = { 'desc' : "copyLine",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnFlip(self, event):
        """Flip selected lines/boundaries"""
        if self.action['desc'] == 'flipLine': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnFlip():")
        self.action = { 'desc' : "flipLine",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnMerge(self, event):
        """Merge selected lines/boundaries"""
        if self.action['desc'] == 'mergeLine': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnMerge():")
        self.action = { 'desc' : "mergeLine",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnBreak(self, event):
        """Break selected lines/boundaries"""
        if self.action['desc'] == 'breakLine': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnBreak():")
        self.action = { 'desc' : "breakLine",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnSnap(self, event):
        """Snap selected features"""
        if self.action['desc'] == 'snapLine': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnSnap():")
        self.action = { 'desc' : "snapLine",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnConnect(self, event):
        """Connect selected lines/boundaries"""
        if self.action['desc'] == 'connectLine': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnConnect():")
        self.action = { 'desc' : "connectLine",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnQuery(self, event):
        """Query selected lines/boundaries"""
        if self.action['desc'] == 'queryLine': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnQuery(): %s" % \
                      UserSettings.Get(group='vdigit', key='query', subkey='selection'))
        self.action = { 'desc' : "queryLine",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnZBulk(self, event):
        """Z bulk-labeling selected lines/boundaries"""
        if self.action['desc'] == 'zbulkLine': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnZBulk():")
        self.action = { 'desc' : "zbulkLine",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'line'

    def OnTypeConversion(self, event):
        """Feature type conversion

        Supported conversions:
         - point <-> centroid
         - line <-> boundary
        """
        if self.action['desc'] == 'typeConv': # select previous action
            self.toolbar[0].ToggleTool(self.addPoint, True)
            self.OnAddPoint(event)
            return
        
        Debug.msg(2, "Digittoolbar.OnTypeConversion():")
        self.action = { 'desc' : "typeConv",
                        'id'   : self.additionalTools }
        self.parent.MapWindow.mouse['box'] = 'box'

    def OnSelectMap (self, event):
        """
        Select vector map layer for editing

        If there is a vector map layer already edited, this action is
        firstly terminated. The map layer is closed. After this the
        selected map layer activated for editing.
        """
        if event.GetSelection() == 0: # create new vector map layer
            if self.mapLayer:
                openVectorMap = self.mapLayer.GetName(fullyQualified=False)['name']
            else:
                openVectorMap = None
            mapName = gdialogs.CreateNewVector(self.parent,
                                               exceptMap=openVectorMap, log=self.log,
                                               cmdDef=(['v.edit', 'tool=create'], "map"))
            if mapName:
                # add layer to map layer tree
                if self.layerTree:
                    self.layerTree.AddLayer(ltype='vector',
                                            lname=mapName,
                                            lchecked=True,
                                            lopacity=1.0,
                                            lcmd=['d.vect', 'map=%s' % mapName])
                    
                    vectLayers = self.UpdateListOfLayers(updateTool=True)
                    selection = vectLayers.index(mapName)
                else:
                    pass # TODO (no Layer Manager)
            else:
                self.combo.SetValue(_('Select vector map'))
                return 
        else:
            selection = event.GetSelection() - 1 # first option is 'New vector map'

        # skip currently selected map
        if self.layers[selection] == self.mapLayer:
            return False

        if self.mapLayer:
            # deactive map layer for editing
            self.StopEditing()

        # select the given map layer for editing
        self.StartEditing(self.layers[selection])

        event.Skip()

        return True
    
    def StartEditing (self, mapLayer):
        """
        Start editing selected vector map layer.

        @param mapLayer reference to MapLayer instance
        """
        # reload vdigit module
        reload(vdigit)
        from vdigit import Digit as Digit
        self.parent.digit = Digit(mapwindow=self.parent.MapWindow)
        
        self.mapLayer = mapLayer
        
        # open vector map
        try:
            self.parent.digit.SetMapName(mapLayer.GetName())
        except gcmd.DigitError, e:
            self.mapLayer = None
            print >> sys.stderr, e # wxMessageBox
            return False
        
        # update toolbar
        self.combo.SetValue(mapLayer.GetName())
        self.parent.toolbars['map'].combo.SetValue ('Digitize')
        
        Debug.msg (4, "VDigitToolbar.StartEditing(): layer=%s" % mapLayer.GetName())
        
        # deactive layer
        self.mapcontent.ChangeLayerActive(mapLayer, False)

        # change cursor
        if self.parent.MapWindow.mouse['use'] == 'pointer':
            self.parent.MapWindow.SetCursor(self.parent.cursors["cross"])
        
        # create pseudoDC for drawing the map
        self.parent.MapWindow.pdcVector = wx.PseudoDC()
        self.parent.digit.driver.SetDevice(self.parent.MapWindow.pdcVector)
        # self.parent.MapWindow.UpdateMap()
        if not self.parent.MapWindow.resize:
            self.parent.MapWindow.UpdateMap(render=True)
        
        return True

    def StopEditing (self):
        """Stop editing of selected vector map layer.

        @return True on success
        @return False on failure
        """
        if not self.mapLayer:
            return False
        
        Debug.msg (4, "VDigitToolbar.StopEditing(): layer=%s" % self.mapLayer.GetName())
        self.combo.SetValue (_('Select vector map'))
        
        # save changes (only for vdigit)
        if UserSettings.Get(group='advanced', key='digitInterface', subkey='type') == 'vdigit':
            if UserSettings.Get(group='vdigit', key='saveOnExit', subkey='enabled') is False:
                if self.parent.digit.GetUndoLevel() > 0:
                    dlg = wx.MessageDialog(parent=self.parent, message=_("Do you want to save changes "
                                                                         "in vector map <%s>?") % self.mapLayer.GetName(),
                                           caption=_("Save changes?"),
                                           style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
                    if dlg.ShowModal() == wx.ID_NO:
                        # revert changes
                        self.parent.digit.Undo(0)
                    dlg.Destroy()
        
        self.parent.digit.SetMapName(None) # -> close map
        
        # re-active layer 
        item = self.parent.tree.FindItemByData('maplayer', self.mapLayer)
        if item and self.parent.tree.IsItemChecked(item):
            self.mapcontent.ChangeLayerActive(self.mapLayer, True)
        
        # change cursor
        self.parent.MapWindow.SetCursor(self.parent.cursors["default"])
        
        # disable pseudodc for vector map layer
        self.parent.MapWindow.pdcVector = None
        self.parent.digit.driver.SetDevice(None)
        
        self.parent.digit.__del__() # FIXME: destructor is not called here (del)
        self.parent.digit = None
        
        self.mapLayer = None
        
        return True
    
    def UpdateListOfLayers (self, updateTool=False):
        """
        Update list of available vector map layers.
        This list consists only editable layers (in the current mapset)

        Optionaly also update toolbar
        """

        Debug.msg (4, "VDigitToolbar.UpdateListOfLayers(): updateTool=%d" % \
                   updateTool)

        layerNameSelected = None
         # name of currently selected layer
        if self.mapLayer:
            layerNameSelected = self.mapLayer.GetName()

        # select vector map layer in the current mapset
        layerNameList = []
        self.layers = self.mapcontent.GetListOfLayers(l_type="vector",
                                                      l_mapset=grassenv.GetGRASSVariable('MAPSET'))
        for layer in self.layers:
            if not layer.name in layerNameList: # do not duplicate layer
                layerNameList.append (layer.GetName())

        if updateTool: # update toolbar
            if not self.mapLayer:
                value = _('Select vector map')
            else:
                value = layerNameSelected

            if not self.comboid:
                self.combo = wx.ComboBox(self.toolbar[self.numOfRows-1], id=wx.ID_ANY, value=value,
                                         choices=[_('New vector map'), ] + layerNameList, size=(105, -1),
                                         style=wx.CB_READONLY)
                self.comboid = self.toolbar[self.numOfRows-1].InsertControl(0, self.combo)
                self.parent.Bind(wx.EVT_COMBOBOX, self.OnSelectMap, self.comboid)
            else:
                self.combo.SetItems([_('New vector map'), ] + layerNameList)
            
            self.toolbar[self.numOfRows-1].Realize()

        return layerNameList

    def GetLayer(self):
        """Get selected layer for editing -- MapLayer instance"""
        return self.mapLayer

    def GetAction(self, type='desc'):
        """Get current action info"""
        return self.action[type]
    
class ProfileToolbar(AbstractToolbar):
    """
    Toolbar for profiling raster map
    """
    def __init__(self, parent, mapdisplay, map):
        self.parent     = parent
        self.mapcontent = map
        self.mapdisplay = mapdisplay

        self.toolbar = wx.ToolBar(parent=self.mapdisplay, id=wx.ID_ANY)

        # self.SetToolBar(self.toolbar)
        self.toolbar.SetToolBitmapSize(globalvar.toolbarSize)

        self.InitToolbar(self.mapdisplay, self.toolbar, self.ToolbarData())

        # realize the toolbar
        self.toolbar.Realize()

    def ToolbarData(self):
        """Toolbar data"""

        self.transect = wx.NewId()
        self.addraster = wx.NewId()
        self.draw = wx.NewId()
        self.options = wx.NewId()
        self.drag = wx.NewId()
        self.zoom = wx.NewId()
        self.unzoom = wx.NewId()
        self.erase = wx.NewId()
        self.save = wx.NewId()
        self.printer = wx.NewId()
        self.quit = wx.NewId()
                
        # tool, label, bitmap, kind, shortHelp, longHelp, handler
        return   (
            (self.transect, 'transect', Icons["transect"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["transect"].GetLabel(), Icons["transect"].GetDesc(),
             self.parent.OnDrawTransect),
            (self.addraster, 'raster', Icons["addrast"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["addrast"].GetLabel(), Icons["addrast"].GetDesc(),
             self.parent.OnSelectRaster),
            (self.draw, 'profiledraw', Icons["profiledraw"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["profiledraw"].GetLabel(), Icons["profiledraw"].GetDesc(),
             self.parent.OnCreateProfile),
            (self.options, 'options', Icons["profileopt"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["profileopt"].GetLabel(), Icons["profileopt"].GetDesc(),
             self.parent.ProfileOptionsMenu),
            (self.drag, 'drag', Icons['pan'].GetBitmap(),
             wx.ITEM_NORMAL, Icons["pan"].GetLabel(), Icons["pan"].GetDesc(),
             self.parent.OnDrag),
            (self.zoom, 'zoom', Icons['zoom_in'].GetBitmap(),
             wx.ITEM_NORMAL, Icons["zoom_in"].GetLabel(), Icons["zoom_in"].GetDesc(),
             self.parent.OnZoom),
            (self.unzoom, 'unzoom', Icons['zoom_back'].GetBitmap(),
             wx.ITEM_NORMAL, Icons["zoom_back"].GetLabel(), Icons["zoom_back"].GetDesc(),
             self.parent.OnRedraw),
            (self.erase, 'erase', Icons["erase"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["erase"].GetLabel(), Icons["erase"].GetDesc(),
             self.parent.OnErase),
            ("", "", "", "", "", "", ""),
            (self.save, 'save', Icons["savefile"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["savefile"].GetLabel(), Icons["savefile"].GetDesc(),
             self.parent.SaveToFile),
            (self.printer, 'print', Icons["printmap"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["printmap"].GetLabel(), Icons["printmap"].GetDesc(),
             self.parent.PrintMenu),
            (self.quit, 'quit', Icons["quit"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["quit"].GetLabel(), Icons["quit"].GetDesc(),
             self.parent.OnQuit),
            )
    
class NvizToolbar(AbstractToolbar):
    """
    Nviz toolbar
    """
    def __init__(self, parent, map):
        self.parent     = parent
        self.mapcontent = map

        self.toolbar = wx.ToolBar(parent=self.parent, id=wx.ID_ANY)

        # self.SetToolBar(self.toolbar)
        self.toolbar.SetToolBitmapSize(globalvar.toolbarSize)

        self.InitToolbar(self.parent, self.toolbar, self.ToolbarData())

        # realize the toolbar
        self.toolbar.Realize()

    def ToolbarData(self):
        """Toolbar data"""

        self.settings = wx.NewId()
        self.quit = wx.NewId()
                
        # tool, label, bitmap, kind, shortHelp, longHelp, handler
        return   (
            (self.settings, "settings", Icons["nvizSettings"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["nvizSettings"].GetLabel(), Icons["nvizSettings"].GetDesc(),
             self.OnSettings),
            (self.quit, 'quit', Icons["quit"].GetBitmap(),
             wx.ITEM_NORMAL, Icons["quit"].GetLabel(), Icons["quit"].GetDesc(),
             self.OnExit),
            )

    def OnSettings(self, event):
        win = self.parent.nvizToolWin
        if not win.IsShown():
            self.parent.nvizToolWin.Show()
        else:
            self.parent.nvizToolWin.Hide()

    def OnExit (self, event=None):
        """Quit nviz tool (swith to 2D mode)"""

        # disable the toolbar
        self.parent.RemoveToolbar ("nviz")

