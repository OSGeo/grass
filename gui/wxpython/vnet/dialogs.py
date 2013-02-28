"""!
@package vnet.dialogs

@brief Dialog for vector network analysis front-end

Classes:
 - dialogs::VNETDialog
 - dialogs::PtsList
 - dialogs::SettingsDialog
 - dialogs::AddLayerDialog
 - dialogs::VnetTmpVectMaps
 - dialogs::VectMap
 - dialogs::History
 - dialogs::VnetStatusbar

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
"""

import os
import sys
import types
try:
    import grass.lib.vector as vectlib
    from ctypes import pointer, byref, c_char_p, c_int, c_double
    haveCtypes = True
except ImportError:
    haveCtypes = False

from copy import copy
from grass.script     import core as grass

import wx
import wx.aui
import wx.lib.flatnotebook  as FN
import wx.lib.colourselect as csel

from core             import globalvar, utils
from core.gcmd        import RunCommand, GMessage
from core.events      import gUpdateMap
from core.settings    import UserSettings

from dbmgr.base       import DbMgrBase 

from gui_core.widgets import GNotebook
from gui_core.goutput import GConsoleWindow
from core.gconsole    import CmdThread, EVT_CMD_DONE, GConsole
from gui_core.gselect import Select, LayerSelect, ColumnSelect

from vnet.widgets     import PointsList
from vnet.toolbars    import MainToolbar, PointListToolbar, AnalysisToolbar

#Main TODOs
# - when layer tree of is changed, tmp result map is removed from render list 
# - optimization of map drawing 
# - tmp maps - add number of process
# - destructor problem - when GRASS GIS is closed with open VNETDialog,
#   it's destructor is not called

class VNETDialog(wx.Dialog):
    def __init__(self, parent,
                 id = wx.ID_ANY, title = _("GRASS GIS Vector Network Analysis Tool"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        """!Dialog for vector network analysis"""

        wx.Dialog.__init__(self, parent, id, style=style, title = title, **kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))

        self.parent  = parent  # mapdisp.frame MapFrame class
        self.mapWin = parent.MapWindow 

        # contains current analysis result (do not have to be last one, when history is browsed), 
        # it is instance of VectMap class
        self.tmp_result = None 

        # initialization of History class used for saving and reading data from file
        # it is used for browsing analysis results
        self.history = History(self)

        # variable, which appends  unique number to every name of map, which is saved into history
        self.histTmpVectMapNum = 0

        # list of maps, which will be saved into history
        self.tmpVectMapsToHist = []

        # this class instance manages all temporary vector maps created during life of VNETDialog  
        self.tmpMaps = VnetTmpVectMaps(parent = self)

        # setting initialization
        self._initSettings()

        # registration graphics for drawing
        self.pointsToDraw = self.mapWin.RegisterGraphicsToDraw(graphicsType = "point", 
                                                               setStatusFunc = self.SetPointStatus)
        self.SetPointDrawSettings()

        # information, whether mouse event handler is registered in map window
        self.handlerRegistered = False

        # get attribute table columns only with numbers (for cost columns in vnet analysis)
        self.columnTypes = ['integer', 'double precision'] 
        
        # initialization of v.net.* analysis parameters (data which characterizes particular analysis)
        self._initVnetParams()

        # toobars
        self.toolbars = {}
        self.toolbars['mainToolbar'] = MainToolbar(parent = self)
        self.toolbars['analysisToolbar'] = AnalysisToolbar(parent = self)
        #
        # Fancy gui
        #
        self._mgr = wx.aui.AuiManager(self)

        # Columns in points list
        # TODO init dynamically, not hard typed v.net.path
        self.cols =   [
                        ['type', _('type'), [_(""), _("Start point"), _("End point")], 0], 
                        ['topology',  _('topology'), None, ""] 
                      ]

        self.mainPanel = wx.Panel(parent=self)
        self.notebook = GNotebook(parent = self.mainPanel,
                                  style = FN.FNB_FANCY_TABS | FN.FNB_BOTTOM |
                                          FN.FNB_NO_X_BUTTON)

        # statusbar
        self.stPriorities = {'important' : 5, 'iformation' : 1}
        self.stBar = VnetStatusbar(parent = self.mainPanel, style = 0)
        self.stBar.SetFieldsCount(number = 1)
    
        
        # Create tabs
        
        # Stores all data related to snapping
        self.snapData = {}
        self.snapData['snap_mode'] = False
        # Stores widgets which sets some of analysis parameters (e. g. v.ne.iso -> iso lines)
        self.anSettings = {} 
        self._createPointsPage()

        # stores widgets which are shown on parameters page 
        # they set data, on which analysis will be done
        self.inputData = {}
        self._createParametersPage()

        # Output console for analysis
        self._createOutputPage()

        # Stores data which are needed for attribute table browser of analysis input map layers
        self.inpDbMgrData = {}
        self._createInputDbMgrPage()

        # Stores data which are need for attribute table browser of analysis result map layers
        self.resultDbMgrData = {}
        self._createResultDbMgrPage()

        self.notebook.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(wx.EVT_CLOSE, self.OnCloseDialog)

        self._addPanes()
        self._doVnetDialogLayout()
        self._mgr.Update()

        # adds 2 points into list
        for i in range(2):
            self.list.AddItem()
            self.list.EditCellIndex(i, 'type', i + 1) 
            self.list.CheckItem(i, True)

        # selects first point
        self.list.selected = 0
        self.list.Select(self.list.selected)

        dlgSize = (420, 560)
        self.SetMinSize(dlgSize)
        self.SetInitialSize(dlgSize)

        #fix goutput's pane size (required for Mac OSX)
        if self.gwindow:         
            self.gwindow.SetSashPosition(int(self.GetSize()[1] * .75))

        self.OnAnalysisChanged(None)
        self.notebook.SetSelectionByName("parameters")
        self.toolbars['analysisToolbar'].SetMinSize((-1, self.toolbars['mainToolbar'].GetSize()[1]))

    def  __del__(self):
        """!Removes temp layers, unregisters handlers and graphics"""
        update = self.tmpMaps.DeleteAllTmpMaps()

        self.mapWin.UnregisterGraphicsToDraw(self.pointsToDraw)

        if self.handlerRegistered:
            self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN, 
                                                  self.OnMapClickHandler)
        if update:
            up_map_evt = gUpdateMap(render = True, renderVector = True)
            wx.PostEvent(self.mapWin, up_map_evt)
        else:
            up_map_evt = gUpdateMap(render = True, renderVector = True)
            wx.PostEvent(self.mapWin, up_map_evt)


    def _addPanes(self):
        """!Adds toolbar pane and pane with tabs"""
        self._mgr.AddPane(self.toolbars['mainToolbar'],
                              wx.aui.AuiPaneInfo().
                              Name("pointlisttools").Caption(_("Point list toolbar")).
                              ToolbarPane().Top().Row(0).
                              Dockable(False).
                              CloseButton(False).Layer(0))
 
        self._mgr.AddPane(self.toolbars['analysisToolbar'],
                              wx.aui.AuiPaneInfo().
                              Name("pointlisttools").Caption(_("Point list toolbar")).
                              ToolbarPane().Top().Row(1).
                              Dockable(False).
                              CloseButton(False).Layer(0))

        self._mgr.AddPane(self.mainPanel,
                              wx.aui.AuiPaneInfo().
                              Name("tabs").CaptionVisible(visible = False).
                              Center().
                              Dockable(False).
                              CloseButton(False).Layer(0))
    def _doVnetDialogLayout(self):

        sizer = wx.BoxSizer(wx.VERTICAL)

        sizer.Add(item = self.notebook, proportion = 1,
                  flag = wx.EXPAND)

        sizer.Add(item = self.stBar, proportion = 0)

        self.mainPanel.SetSizer(sizer)

        sizer.Fit(self)  
        self.Layout()

    def _createPointsPage(self):
        """!Tab with points list and analysis settings"""
        pointsPanel = wx.Panel(parent = self)
        maxValue = 1e8

        listBox = wx.StaticBox(parent = pointsPanel, id = wx.ID_ANY,
                                label =" %s " % _("Points for analysis:"))

        self.notebook.AddPage(page = pointsPanel, 
                              text=_('Points'), 
                              name = 'points')

        self.list = PtsList(parent = pointsPanel, dialog = self, cols = self.cols)
        self.toolbars['pointsList'] = PointListToolbar(parent = pointsPanel, list = self.list)

        anSettingsPanel = wx.Panel(parent = pointsPanel)

        anSettingsBox = wx.StaticBox(parent = anSettingsPanel, id = wx.ID_ANY,
                                label =" %s " % _("Analysis settings:"))

        maxDistPanel =  wx.Panel(parent = anSettingsPanel)
        maxDistLabel = wx.StaticText(parent = maxDistPanel, id = wx.ID_ANY, label = _("Maximum distance of point to the network:"))
        self.anSettings["max_dist"] = wx.SpinCtrl(parent = maxDistPanel, id = wx.ID_ANY, min = 0, max = maxValue)
        self.anSettings["max_dist"].SetValue(100000) #TODO init val

        #showCutPanel =  wx.Panel(parent = anSettingsPanel)
        #self.anSettings["show_cut"] = wx.CheckBox(parent = showCutPanel, id=wx.ID_ANY,
        #                                          label = _("Show minimal cut"))
        #self.anSettings["show_cut"].Bind(wx.EVT_CHECKBOX, self.OnShowCut)

        isoLinesPanel =  wx.Panel(parent = anSettingsPanel)
        isoLineslabel = wx.StaticText(parent = isoLinesPanel, id = wx.ID_ANY, label = _("Iso lines:"))
        self.anSettings["iso_lines"] = wx.TextCtrl(parent = isoLinesPanel, id = wx.ID_ANY) 
        self.anSettings["iso_lines"].SetValue("1000,2000,3000")

        # Layout
        AnalysisSizer = wx.BoxSizer(wx.VERTICAL)

        listSizer = wx.StaticBoxSizer(listBox, wx.VERTICAL)

        listSizer.Add(item = self.toolbars['pointsList'], proportion = 0)
        listSizer.Add(item = self.list, proportion = 1, flag = wx.EXPAND)

        anSettingsSizer = wx.StaticBoxSizer(anSettingsBox, wx.VERTICAL)

        maxDistSizer = wx.BoxSizer(wx.HORIZONTAL)
        maxDistSizer.Add(item = maxDistLabel, flag = wx.ALIGN_CENTER_VERTICAL, proportion = 1)
        maxDistSizer.Add(item = self.anSettings["max_dist"],
                         flag = wx.EXPAND | wx.ALL, border = 5, proportion = 0)
        maxDistPanel.SetSizer(maxDistSizer)
        anSettingsSizer.Add(item = maxDistPanel, proportion = 1, flag = wx.EXPAND)

        #showCutSizer = wx.BoxSizer(wx.HORIZONTAL)
        #showCutPanel.SetSizer(showCutSizer)
        #showCutSizer.Add(item = self.anSettings["show_cut"],
        #                 flag = wx.EXPAND | wx.ALL, border = 5, proportion = 0)
        #anSettingsSizer.Add(item = showCutPanel, proportion = 1, flag = wx.EXPAND)

        isoLinesSizer = wx.BoxSizer(wx.HORIZONTAL)
        isoLinesSizer.Add(item = isoLineslabel, flag = wx.ALIGN_CENTER_VERTICAL, proportion = 1)
        isoLinesSizer.Add(item = self.anSettings["iso_lines"],
                        flag = wx.EXPAND | wx.ALL, border = 5, proportion = 1)
        isoLinesPanel.SetSizer(isoLinesSizer)
        anSettingsSizer.Add(item = isoLinesPanel, proportion = 1, flag = wx.EXPAND)

        AnalysisSizer.Add(item = listSizer, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        AnalysisSizer.Add(item = anSettingsPanel, proportion = 0, flag = wx.EXPAND | wx.RIGHT | wx.LEFT | wx.BOTTOM, border = 5)

        anSettingsPanel.SetSizer(anSettingsSizer)
        pointsPanel.SetSizer(AnalysisSizer)

    def OnShowCut(self, event):
        """!Shows vector map with minimal cut (v.net.flow) - not yet implemented"""
        val = event.IsChecked()
        if val and self.tmp_result:
            self.tmp_result.DeleteRenderLayer()
            cmd = self.GetLayerStyle()
            self.vnetFlowTmpCut.AddRenderLayer(cmd)
        else:
            self.vnetFlowTmpCut.DeleteRenderLayer()
            cmd = self.GetLayerStyle()
            self.tmp_result.AddRenderLayer(cmd)

        up_map_evt = gUpdateMap(render = True, renderVector = True)
        wx.PostEvent(self.mapWin, up_map_evt)

    def _createOutputPage(self):
        """!Tab with output console"""
        outputPanel = wx.Panel(parent = self)
        self.notebook.AddPage(page = outputPanel, 
                              text = _("Output"), 
                              name = 'output')

        self.goutput = GConsole(guiparent = self)
        self.gwindow = GConsoleWindow(parent = outputPanel, gconsole = self.goutput)

        #Layout
        outputSizer = wx.BoxSizer(wx.VERTICAL)
        outputSizer.Add(item = self.gwindow, proportion = 1, flag = wx.EXPAND)
        self.gwindow.SetMinSize((-1,-1))

        outputPanel.SetSizer(outputSizer)

    def _createParametersPage(self):
        """!Tab for selection of data for analysis"""
        dataPanel = wx.Panel(parent=self)
        self.notebook.AddPage(page = dataPanel,
                              text=_('Parameters'), 
                              name = 'parameters')
        label = {}
        dataSelects = [
                        ['input', "Choose vector map for analysis:", Select],
                        ['alayer', "Arc layer number or name:", LayerSelect],
                        ['nlayer', "Node layer number or name:", LayerSelect],
                        ['afcolumn', self.attrCols['afcolumn']['label'], ColumnSelect],
                        ['abcolumn', self.attrCols['abcolumn']['label'], ColumnSelect],
                        ['ncolumn', self.attrCols['ncolumn']['label'], ColumnSelect],
                      ]

        selPanels = {}
        for dataSel in dataSelects:
            selPanels[dataSel[0]] = wx.Panel(parent = dataPanel)
            if dataSel[0] == 'input':
                self.inputData[dataSel[0]] = dataSel[2](parent = selPanels[dataSel[0]],
                                                        size = (-1, -1), 
                                                        type = 'vector')
                icon = wx.Image(os.path.join(globalvar.ETCICONDIR, "grass", "layer-vector-add.png"))
                icon.Rescale(18, 18)
                icon = wx.BitmapFromImage(icon) 
                self.addToTreeBtn = wx.BitmapButton(parent = selPanels[dataSel[0]], 
                                                    bitmap = icon, 
                                                    size = globalvar.DIALOG_COLOR_SIZE) 
                self.addToTreeBtn.SetToolTipString(_("Add vector map into layer tree"))
                self.addToTreeBtn.Disable()
                self.addToTreeBtn.Bind(wx.EVT_BUTTON, self.OnToTreeBtn)
            elif dataSel[0] != 'input':
                self.inputData[dataSel[0]] = dataSel[2](parent = selPanels[dataSel[0]],  
                                                        size = (-1, -1))
            label[dataSel[0]] =  wx.StaticText(parent =  selPanels[dataSel[0]], 
                                               name = dataSel[0])
            label[dataSel[0]].SetLabel(dataSel[1])

        self.inputData['input'].Bind(wx.EVT_TEXT, self.OnVectSel) 
        self.inputData['alayer'].Bind(wx.EVT_TEXT, self.OnALayerSel)
        self.inputData['nlayer'].Bind(wx.EVT_TEXT, self.OnNLayerSel)

        # Layout
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox(dataPanel, -1, "Vector map and layers for analysis")
        bsizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        mainSizer.Add(item = bsizer, proportion = 0,
                      flag = wx.EXPAND  | wx.TOP | wx.LEFT | wx.RIGHT, border = 5) 

        for sel in ['input', 'alayer', 'nlayer']:
            if sel== 'input':
                btn = self.addToTreeBtn
            else:
                btn = None
            selPanels[sel].SetSizer(self._doSelLayout(title = label[sel], 
                                                      sel = self.inputData[sel], 
                                                      btn = btn))
            bsizer.Add(item = selPanels[sel], proportion = 1,
                       flag = wx.EXPAND)

        box = wx.StaticBox(dataPanel, -1, "Costs")    
        bsizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        mainSizer.Add(item = bsizer, proportion = 0,
                                 flag = wx.EXPAND  | wx.TOP | wx.LEFT | wx.RIGHT, border = 5)       

        for sel in ['afcolumn', 'abcolumn', 'ncolumn']:
            selPanels[sel].SetSizer(self._doSelLayout(title = label[sel], sel = self.inputData[sel]))
            bsizer.Add(item = selPanels[sel], proportion = 0,
                       flag = wx.EXPAND)

        dataPanel.SetSizer(mainSizer)

    def _doSelLayout(self, title, sel, btn = None): 
        # helper function for layout of self.inputData widgets initialized in _createParametersPage
        selSizer = wx.BoxSizer(orient = wx.VERTICAL)

        selTitleSizer = wx.BoxSizer(wx.HORIZONTAL)
        selTitleSizer.Add(item = title, proportion = 1,
                          flag = wx.LEFT | wx.TOP | wx.EXPAND, border = 5)

        selSizer.Add(item = selTitleSizer, proportion = 0,
                                 flag = wx.EXPAND)

        if btn:
                selFiledSizer = wx.BoxSizer(orient = wx.HORIZONTAL)
                selFiledSizer.Add(item = sel, proportion = 1,
                             flag = wx.EXPAND | wx.ALL)

                selFiledSizer.Add(item = btn, proportion = 0,
                             flag = wx.EXPAND | wx.ALL)

                selSizer.Add(item = selFiledSizer, proportion = 0,
                             flag = wx.EXPAND | wx.ALL| wx.ALIGN_CENTER_VERTICAL,
                             border = 5)
        else:
                selSizer.Add(item = sel, proportion = 1,
                             flag = wx.EXPAND | wx.ALL| wx.ALIGN_CENTER_VERTICAL,
                             border = 5)
        return selSizer

    def _createInputDbMgrPage(self):
        """!Tab with attribute tables of analysis input layers"""
        self.inpDbMgrData['dbMgr'] = DbMgrBase()
 
        selMapName = None       
        # if selected vector map is in layer tree then set it
        if self.mapWin.tree and self.mapWin.tree.layer_selected:
            selMapData = self.mapWin.tree.GetPyData(self.mapWin.tree.layer_selected)[0]
            if selMapData['type'] == 'vector': # wrap somehow in LayerTree
                selMapName = selMapData['maplayer'].name

        self.inpDbMgrData['browse'] = self.inpDbMgrData['dbMgr'].CreateDbMgrPage(parent = self.notebook,
                                                                                 pageName = 'browse')
        self.inpDbMgrData['browse'].SetTabAreaColour(globalvar.FNPageColor)

        self.inpDbMgrData['input'] = None
        if selMapName:
            self.inputData['input'].SetValue(selMapName)
            self.OnVectSel(None)

    def _updateInputDbMgrPage(self, show):
        """!Show or hide input tables tab"""
        if show and self.notebook.GetPageIndexByName('inputDbMgr') == -1:
            self.notebook.AddPage(page = self.inpDbMgrData['browse'],
                                  text=_('Input tables'), 
                                  name = 'inputDbMgr')
        elif not show:
            self.notebook.RemovePage(page = 'inputDbMgr')

    def _createResultDbMgrPage(self):
        """!Tab with attribute tables of analysis result layers"""
        self.resultDbMgrData['dbMgr'] = DbMgrBase() 
        self.resultDbMgrData['browse'] = self.resultDbMgrData['dbMgr'].CreateDbMgrPage(parent = self.notebook,
                                                                                       pageName = 'browse')
        self.resultDbMgrData['browse'].SetTabAreaColour(globalvar.FNPageColor)

        if  self.tmp_result:
            self.resultDbMgrData['input'] = self.tmp_result.GetVectMapName()
        else:
            self.resultDbMgrData['input'] = None

    def _updateResultDbMgrPage(self):
        """!Show or Hide Result tables tab"""
        # analysis, which created result
        analysis = self.resultDbMgrData['analysis']
        #TODO maybe no need to store this information, just check it has attribute table, if so show it
        haveDbMgr = self.vnetParams[analysis]["resultProps"]["dbMgr"]

        if haveDbMgr and self.notebook.GetPageIndexByName('resultDbMgr') == -1:
            self.notebook.AddPage(page = self.resultDbMgrData['browse'],
                                  text=_('Result tables'), 
                                  name = 'resultDbMgr')
        elif not haveDbMgr:
            self.notebook.RemovePage(page = 'resultDbMgr')

    def OnPageChanged(self, event):
        """!Tab switched"""
        if event.GetEventObject() == self.notebook:
            dbMgrIndxs = []
            dbMgrIndxs.append(self.notebook.GetPageIndexByName('inputDbMgr'))
            dbMgrIndxs.append(self.notebook.GetPageIndexByName('resultDbMgr'))
            if self.notebook.GetSelection() in dbMgrIndxs:
                self.stBar.AddStatusItem(text = _('Loading tables...'), 
                                         key = 'dbMgr',
                                         priority = self.stPriorities['important'])
                self._updateDbMgrData()
                self.stBar.RemoveStatusItem(key = 'dbMgr')
            # update columns (when some is added in input tables browser), TODO needs optimization  
            elif  self.notebook.GetSelection() == self.notebook.GetPageIndexByName('parameters'):
                self.OnALayerSel(None) 
                self.OnNLayerSel(None)

    def _updateDbMgrData(self):
            """!Updates input/result tables page """
            if self.notebook.GetSelection() == self.notebook.GetPageIndexByName('inputDbMgr'):
                self._updateInputDbMgrData()
            elif self.notebook.GetSelection() == self.notebook.GetPageIndexByName('resultDbMgr'):
                self._updateResultDbMgrData()
            else:
                self.stBar.RemoveStatusItem('manager')   

    def _updateInputDbMgrData(self):
        """!Loads data according to selected layers in Parameters tab"""
        inpSel = self.inputData['input'].GetValue().strip()
        # changed vector map
        if self.inpDbMgrData['input'] != inpSel:
            wx.BeginBusyCursor()
            self.inpDbMgrData['dbMgr'].ChangeVectorMap(vectorName = inpSel)
            self.inpDbMgrData['input'] = inpSel
            for layerName in ['alayer', 'nlayer']:
                try:
                    layer = int(self.inputData[layerName].GetValue())
                except ValueError:
                    continue
                self.inpDbMgrData['browse'].AddLayer(layer)
            wx.EndBusyCursor()
        # same vector map
        else:
            needLayers = []
            browseLayers = self.inpDbMgrData['browse'].GetAddedLayers()
            for layerName in ['alayer', 'nlayer']:
                try:                
                    inpLayer = int(self.inputData[layerName].GetValue())
                except ValueError:
                    continue

                if inpLayer in browseLayers:
                    needLayers.append(inpLayer)
                    continue
                else:
                    wx.BeginBusyCursor()
                    self.inpDbMgrData['browse'].AddLayer(inpLayer)
                    wx.EndBusyCursor()
                    needLayers.append(inpLayer)

            for layer in browseLayers:
                if layer not in needLayers:
                    self.inpDbMgrData['browse'].DeletePage(layer)

    def _updateResultDbMgrData(self):
        """!Loads data from analysis result map"""
        if not self.tmp_result:
            return
        vectName = self.tmp_result.GetVectMapName()

        if self.resultDbMgrData['input'] != vectName:
            wx.BeginBusyCursor()
            dbMgr = self.resultDbMgrData['dbMgr']
            dbMgr.ChangeVectorMap(vectorName = vectName)

            for layer in dbMgr.GetVectorLayers():
                self.resultDbMgrData['browse'].AddLayer(layer)

            self.resultDbMgrData['input'] = vectName
            wx.EndBusyCursor()

    def OnToTreeBtn(self, event):
        """!Add vector map into layer tree"""
        vectorMap = self.inputData['input'].GetValue()
        vectMapName, mapSet = self._parseMapStr(vectorMap)
        vectorMap = vectMapName + '@' + mapSet
        existsMap = grass.find_file(name = vectMapName, 
                                    element = 'vector', 
                                    mapset = mapSet)
        if not existsMap["name"]:
            return

        cmd = ['d.vect', 
               'map=' + vectorMap]

        if self.mapWin.tree and \
           self.mapWin.tree.FindItemByData(key = 'name', value = vectorMap) is None: 
            self.mapWin.tree.AddLayer(ltype = "vector", 
                                      lcmd = cmd,
                                      lname =vectorMap,
                                      lchecked = True)
        #d.mon case
        else:
            self.renderLayer = self.mapWin.Map.AddLayer(ltype = "vector", command = cmd,
                                                        name = vectorMap, active = True,
                                                        opacity = 1.0,    render = True,       
                                                        pos = -1)         

            up_map_evt = gUpdateMap(render = True, renderVector = True)
            wx.PostEvent(self.mapWin, up_map_evt)

    def OnVectSel(self, event):
        """!When vector map is selected it populates other comboboxes in Parameters tab (layer selects, columns selects)"""
        if self.snapData['snap_mode']:
            self.OnSnapping(event = None)

        vectMapName, mapSet = self._parseMapStr(self.inputData['input'].GetValue())
        vectorMap = vectMapName + '@' + mapSet
cd .
        self.inputData['alayer'].Clear()
        self.inputData['nlayer'].Clear()

        self.inputData['alayer'].InsertLayers(vector = vectorMap)
        self.inputData['nlayer'].InsertLayers(vector = vectorMap)

        items = self.inputData['alayer'].GetItems()
        itemsLen = len(items)
        if itemsLen < 1:
            self.addToTreeBtn.Disable()
            if hasattr(self, 'inpDbMgrData'):
                self._updateInputDbMgrPage(show = False)
            self.inputData['alayer'].SetValue("")
            self.inputData['nlayer'].SetValue("")
            for sel in ['afcolumn', 'abcolumn', 'ncolumn']:
                self.inputData[sel].Clear()
                self.inputData[sel].SetValue("")
            return
        elif itemsLen == 1:
            self.inputData['alayer'].SetSelection(0)
            self.inputData['nlayer'].SetSelection(0)
        elif itemsLen >= 1:
            if unicode("1") in items:
                iItem = items.index(unicode("1")) 
                self.inputData['alayer'].SetSelection(iItem)
            if unicode("2") in items:
                iItem = items.index(unicode("2")) 
                self.inputData['nlayer'].SetSelection(iItem)

        self.addToTreeBtn.Enable()
        if hasattr(self, 'inpDbMgrData'):
            self._updateInputDbMgrPage(show = True)

        self.OnALayerSel(event) 
        self.OnNLayerSel(event)

    def OnALayerSel(self, event):
        """!When arc layer from vector map is selected, populates corespondent columns selects"""
        self.inputData['afcolumn'].InsertColumns(vector = self.inputData['input'].GetValue(), 
                                                 layer = self.inputData['alayer'].GetValue(), 
                                                 type = self.columnTypes)
        self.inputData['abcolumn'].InsertColumns(vector = self.inputData['input'].GetValue(), 
                                                 layer = self.inputData['alayer'].GetValue(), 
                                                 type = self.columnTypes)


    def OnNLayerSel(self, event):
        """!When node layer from vector map is selected, populates corespondent column select"""
        if self.snapData['snap_mode']:
            self.OnSnapping(event = None)

        self.inputData['ncolumn'].InsertColumns(vector = self.inputData['input'].GetValue(), 
                                                layer = self.inputData['nlayer'].GetValue(), 
                                                type = self.columnTypes)
 
    def _getInvalidInputs(self, inpToTest):
        """!Check of analysis input data for invalid values (Parameters tab)"""
        # dict of invalid values {key from self.itemData (comboboxes from Parameters tab) : invalid value}
        errInput = {}

        mapVal = self.inputData['input'].GetValue()
        mapName, mapSet = self._parseMapStr(mapVal)
        vectMaps = grass.list_grouped('vect')[mapSet]

        # check of vector map
        if not inpToTest or "input" in inpToTest:
            if mapName not in vectMaps:
                errInput['input'] = mapVal

        # check of arc/node layer
        for layerSelName in ['alayer', 'nlayer'] :
            if not inpToTest or layerSelName in inpToTest:

                layerItems = self.inputData[layerSelName].GetItems()
                layerVal = self.inputData[layerSelName].GetValue().strip()
                if layerVal not in layerItems:
                    errInput[layerSelName] = layerVal

        # check of costs columns
        currModCols = self.vnetParams[self.currAnModule]["cmdParams"]["cols"]
        for col, colData in self.attrCols.iteritems():
            if not inpToTest or col in inpToTest:

                if col not in currModCols:
                    continue  

                if "inputField" in self.attrCols[col]: 
                    colInptF = self.attrCols[col]["inputField"]
                else:
                    colInptF = col

                if not self.inputData[colInptF].IsShown():
                    continue
                colVal = self.inputData[colInptF].GetValue().strip()

                if not colVal:
                    continue
                if colVal not in self.inputData[colInptF].GetItems():
                    errInput[col] = colVal

        return errInput

    def _parseMapStr(self, vectMapStr):
        """!Create full map name (add current mapset if it is not present in name)"""
        mapValSpl = vectMapStr.strip().split("@")
        if len(mapValSpl) > 1:
            mapSet = mapValSpl[1]
        else:
            mapSet = grass.gisenv()['MAPSET']
        mapName = mapValSpl[0] 
        
        return mapName, mapSet      

    def InputsErrorMsgs(self, msg, inpToTest = None):
        """!Checks input data in Parameters tab and shows messages if some value is not valid

            @param msg (str) - message added to start of message string 
            @param inpToTest (list) - list of keys from self.itemData map, which says which input data should be checked,
                                      if is empty or None, all input data are checked
            @return True - if checked inputs are OK
            @return False - if some of checked inputs is not ok
        """
        errInput = self._getInvalidInputs(inpToTest)

        errMapStr = ""
        if errInput.has_key('input'):
            self.notebook.SetSelectionByName("parameters")
            if errInput['input']:
                errMapStr = _("Vector map '%s' does not exist.") %  (errInput['input'])
            else:
                errMapStr = _("Vector map was not chosen.")


        if errMapStr:
            GMessage(parent = self,
                     message = msg + "\n" + errMapStr)
            return False

        errLayerStr = ""
        for layer, layerLabel in {'alayer' : _("arc layer"), 
                                  'nlayer' : _("node layer")}.iteritems():

            if  errInput.has_key(layer):
                if errInput[layer]:
                    errLayerStr += _("Chosen %s '%s' does not exist in vector map '%s'.\n") % \
                                   (layerLabel, self.inputData[layer].GetValue(), self.inputData['input'].GetValue())
                else:
                    errLayerStr += _("Choose existing %s.\n") % \
                                   (layerLabel)
        if errLayerStr:
            GMessage(parent = self,
                     message = msg + "\n" + errLayerStr)
            return False

        errColStr = ""
        for col, colData in self.attrCols.iteritems():
            if col in errInput.iterkeys():
                errColStr += _("Chosen column '%s' does not exist in attribute table of layer '%s' of vector map '%s'.\n") % \
                             (errInput[col], self.inputData[layer].GetValue(), self.inputData['input'].GetValue())

        if errColStr:
            self.notebook.SetSelectionByName("parameters")                   
            GMessage(parent = self,
                     message = msg + "\n" + errColStr)
            return False

        return True

    def OnCloseDialog(self, event):
        """!Cancel dialog"""
        self.parent.dialogs['vnet'] = None
        self.Destroy()

    def SetPointStatus(self, item, itemIndex):
        """!Before point is drawn, decides properties of drawing style"""
        key = self.list.GetItemData(itemIndex)
        cats = self.vnetParams[self.currAnModule]["cmdParams"]["cats"]

        if key == self.list.selected:
            wxPen = "selected"
        elif not self.list.IsChecked(key):
                wxPen = "unused"
                item.hide = False
        elif len(cats) > 1:
            idx = self.list.GetCellSelIdx(key, 'type') 
            if idx == 2: #End/To/Sink point
                wxPen = "used2cat"
            else:
                wxPen = "used1cat"              
        else:
            wxPen = "used1cat"       

        item.SetPropertyVal('label', str(itemIndex + 1))
        item.SetPropertyVal('penName', wxPen)       

    def OnMapClickHandler(self, event):
        """!Take coordinates from map window"""
        if event == 'unregistered':
            ptListToolbar = self.toolbars['pointsList']
            if ptListToolbar:
                ptListToolbar.ToggleTool( id = ptListToolbar.GetToolId("insertPoint"),
                                          toggle = False)  
            self.handlerRegistered = False
            return

        self.notebook.SetSelectionByName("points")
        if not self.list.itemDataMap:
            self.list.AddItem(None)

        e, n = self.mapWin.GetLastEN()

        index = self.list.selected
        key = self.list.GetItemData(index)

        if self.snapData['snap_mode']:
            coords = [e, n]
            if self._snapPoint(coords):
                msg = ("snapped to node")
            else:
                msg = _("new point")

            e = coords[0]
            n = coords[1]

        else:
            msg = _("new point")

        self.list.EditCellKey(key = self.list.selected , 
                              colName = 'topology', 
                              cellData = msg)

        self.pointsToDraw.GetItem(key).SetCoords([e, n])

        if self.list.selected == self.list.GetItemCount() - 1:
            self.list.selected = 0
        else:
            self.list.selected += 1
        self.list.Select(self.list.selected)

    def _snapPoint(self, coords):
        """!Find nearest node to click coordinates (within given threshold)"""
        e = coords[0]
        n = coords[1]

        # compute threshold
        snapTreshPix = int(UserSettings.Get(group ='vnet', 
                                            key = 'other', 
                                            subkey = 'snap_tresh'))
        res = max(self.mapWin.Map.region['nsres'], self.mapWin.Map.region['ewres'])
        snapTreshDist = snapTreshPix * res

        vectorMap = self.inputData['input'].GetValue()
        vectMapName, mapSet = self._parseMapStr(vectorMap)
        inpMapExists = grass.find_file(name = vectMapName, 
                                       element = 'vector', 
                                       mapset = mapSet)
        if not inpMapExists['name']:
            return False

        openedMap = pointer(vectlib.Map_info())
        ret = vectlib.Vect_open_old2(openedMap, 
                                     c_char_p(vectMapName),
                                     c_char_p(mapSet),
                                     c_char_p(self.inputData['alayer'].GetValue()))
        if ret == 1:
            vectlib.Vect_close(openedMap)
        if ret != 2: 
            return False

        nodeNum =  vectlib.Vect_find_node(openedMap,     
                                          c_double(e), 
                                          c_double(n), 
                                          c_double(0), 
                                          c_double(snapTreshDist),
                                          vectlib.WITHOUT_Z)

        if nodeNum > 0:
            e = c_double(0)
            n = c_double(0)
            vectlib.Vect_get_node_coor(openedMap, 
                                       nodeNum, 
                                       byref(e), 
                                       byref(n), 
                                       None); # z
            e = e.value
            n = n.value
        else:
            vectlib.Vect_close(openedMap)
            return False

        coords[0] = e
        coords[1] = n
        return True

    def OnAnalyze(self, event):
        """!Called when network analysis is started"""
        # Check of parameters for analysis
        if not self.InputsErrorMsgs(msg = _("Analysis can not be done.")):
            return

        if self.tmp_result:
            self.tmp_result.DeleteRenderLayer()

        # history - delete data in buffer for hist step  
        self.history.DeleteNewHistStepData()
        # empty list for maps to be saved to history
        self.tmpVectMapsToHist= []
        # create new map (included to history) for result of analysis
        self.tmp_result = self.NewTmpVectMapToHist('vnet_tmp_result')

        if not self.tmp_result:
                return          

        self.stBar.AddStatusItem(text = _('Analysing...'),
                                 key = 'analyze',
                                 priority =  self.stPriorities['important'])

        # for case there is some map with same name 
        # (when analysis does not produce any map, this map would have been shown as result) 
        RunCommand('g.remove', 
                    vect = self.tmp_result.GetVectMapName())

        # save data from 
        self._saveAnInputToHist()

        self.resultDbMgrData['analysis'] = self.currAnModule

        # Creates part of cmd fro analysis
        cmdParams = [self.currAnModule]
        cmdParams.extend(self._getInputParams())
        cmdParams.append("output=" + self.tmp_result.GetVectMapName())

        catPts = self._getPtByCat()

        if self.currAnModule == "v.net.path":
            self._vnetPathRunAn(cmdParams, catPts)
        else:
            self._runAn(cmdParams, catPts)

    def _vnetPathRunAn(self, cmdParams, catPts):
        """!Called when analysis is run for v.net.path module"""
        if len(self.pointsToDraw.GetAllItems()) < 1:
            return
        cats = self.vnetParams[self.currAnModule]["cmdParams"]["cats"]

        cmdPts = []
        for cat in cats:
            if  len(catPts[cat[0]]) < 1:
                GMessage(parent = self,
                         message=_("Please choose '%s' and '%s' point.") % (cats[0][1], cats[1][1]))
                return
            cmdPts.append(catPts[cat[0]][0])

        resId = 1
        inpPoints = str(resId) + " " + str(cmdPts[0][0]) + " " + str(cmdPts[0][1]) + \
                                 " " + str(cmdPts[1][0]) + " " + str(cmdPts[1][1])

        self.coordsTmpFile = grass.tempfile()
        coordsTmpFileOpened = open(self.coordsTmpFile, 'w')
        coordsTmpFileOpened.write(inpPoints)
        coordsTmpFileOpened.close()

        cmdParams.append("file=" + self.coordsTmpFile)

        cmdParams.append("dmax=" + str(self.anSettings["max_dist"].GetValue()))
        cmdParams.append("input=" + self.inputData['input'].GetValue())

        cmdParams.append("--overwrite")
        self._prepareCmd(cmd = cmdParams)

        self.goutput.RunCmd(command = cmdParams, onDone = self._vnetPathRunAnDone)

    def _vnetPathRunAnDone(self, cmd, returncode):
        """!Called when v.net.path analysis is done"""
        grass.try_remove(self.coordsTmpFile)

        self._saveHistStep()
        self.tmp_result.SaveVectMapState()

        self._updateResultDbMgrPage()
        self._updateDbMgrData()

        cmd = self.GetLayerStyle()
        self.tmp_result.AddRenderLayer(cmd)

        up_map_evt = gUpdateMap(render = True, renderVector = True)
        wx.PostEvent(self.mapWin, up_map_evt)

        mainToolbar = self.toolbars['mainToolbar']
        id = vars(mainToolbar)['showResult']
        mainToolbar.ToggleTool(id =id,
                               toggle = True)

        self.stBar.RemoveStatusItem(key = 'analyze')

    def _runAn(self, cmdParams, catPts):
        """!Called for all v.net.* analysis (except v.net.path)"""

        cats = self.vnetParams[self.currAnModule]["cmdParams"]["cats"]

        if len(cats) > 1:
            for cat in cats:
                if  len(catPts[cat[0]]) < 1:
                    GMessage(parent = self, 
                            message = _("Please choose '%s' and '%s' point.") \
                                        % (cats[0][1], cats[1][1]))
                    return
        else:
            for cat in cats:
                if  len(catPts[cat[0]]) < 2:
                    GMessage(parent = self, 
                             message = _("Please choose at least two points."))
                    return        

        # TODO add also to thread for analysis?
        vcatResult = RunCommand("v.category",
                           input = self.inputData['input'].GetValue(),
                           option = "report",
                           flags = "g",
                           read = True)     

        vcatResult = vcatResult.splitlines()
        for cat in vcatResult:#TODO
            cat = cat.split()
            if "all" in cat:
                maxCat = int(cat[4])
                break

        layerNum = self.inputData["nlayer"].GetValue().strip()

        pt_ascii, catsNums = self._getAsciiPts (catPts = catPts, 
                                                maxCat = maxCat, 
                                                layerNum = layerNum)

        self.tmpPtsAsciiFile = grass.tempfile()#TODO better tmp files cleanup (make class for managing tmp files)
        tmpPtsAsciiFileOpened = open(self.tmpPtsAsciiFile, 'w')
        tmpPtsAsciiFileOpened.write(pt_ascii)
        tmpPtsAsciiFileOpened.close()

        self.tmpInPts = self._addTmpMapAnalysisMsg("vnet_tmp_in_pts")
        if not self.tmpInPts:
            return

        self.tmpInPtsConnected = self._addTmpMapAnalysisMsg("vnet_tmp_in_pts_connected")
        if not self.tmpInPtsConnected:
            return

        cmdParams.append("input=" + self.tmpInPtsConnected.GetVectMapName())
        cmdParams.append("--overwrite")  

        # append parameters needed for particular analysis
        if self.currAnModule == "v.net.distance":
            cmdParams.append("from_layer=1")
            cmdParams.append("to_layer=1")
        elif self.currAnModule == "v.net.flow":
            self.vnetFlowTmpCut = self.NewTmpVectMapToHist('vnet_tmp_flow_cut')
            if not self.vnetFlowTmpCut:
                return          
            cmdParams.append("cut=" +  self.vnetFlowTmpCut.GetVectMapName())         
        elif self.currAnModule == "v.net.iso":
            costs = self.anSettings["iso_lines"].GetValue()
            cmdParams.append("costs=" + costs)          
        for catName, catNum in catsNums.iteritems():
            if catNum[0] == catNum[1]:
                cmdParams.append(catName + "=" + str(catNum[0]))
            else:
                cmdParams.append(catName + "=" + str(catNum[0]) + "-" + str(catNum[1]))

        # create and run commands which goes to analysis thread
        cmdVEdit = [ 
                    "v.edit",
                    "map=" + self.tmpInPts.GetVectMapName(), 
                    "input=" + self.tmpPtsAsciiFile,
                    "tool=create",
                    "--overwrite", 
                    "-n"                              
                   ]
        self._prepareCmd(cmdVEdit)
        self.goutput.RunCmd(command = cmdVEdit)

        cmdVNet = [
                    "v.net",
                    "points=" + self.tmpInPts.GetVectMapName(), 
                    "input=" + self.inputData['input'].GetValue(),
                    "output=" + self.tmpInPtsConnected.GetVectMapName(),
                    "alayer=" +  self.inputData["alayer"].GetValue().strip(),
                    "nlayer=" +  self.inputData["nlayer"].GetValue().strip(), 
                    "operation=connect",
                    "thresh=" + str(self.anSettings["max_dist"].GetValue()),             
                    "--overwrite"                         
                  ] #TODO snapping to nodes optimization
        self._prepareCmd(cmdVNet)
        self.goutput.RunCmd(command = cmdVNet)

        self._prepareCmd(cmdParams)
        self.goutput.RunCmd(command = cmdParams, onDone = self._runAnDone)

    def _runAnDone(self, cmd, returncode):
        """!Called when analysis is done"""
        self.tmpMaps.DeleteTmpMap(self.tmpInPts) #TODO remove earlier (OnDone lambda?)
        self.tmpMaps.DeleteTmpMap(self.tmpInPtsConnected)
        grass.try_remove(self.tmpPtsAsciiFile)

        self._saveHistStep()
        self.tmp_result.SaveVectMapState()
        if self.currAnModule == "v.net.flow":
            self.vnetFlowTmpCut.SaveVectMapState()
        self._updateResultDbMgrPage()
        self._updateDbMgrData()

        cmd = self.GetLayerStyle()
        self.tmp_result.AddRenderLayer(cmd)

        up_map_evt = gUpdateMap(render = True, renderVector = True)
        wx.PostEvent(self.mapWin, up_map_evt)

        mainToolbar = self.toolbars['mainToolbar']
        id = vars(mainToolbar)['showResult']
        mainToolbar.ToggleTool(id =id,
                               toggle = True)

        self.stBar.RemoveStatusItem(key = 'analyze')

    def _getInputParams(self):
        """!Return list of chosen values (vector map, layers) from Parameters tab. 

        The list items are in form to be used in command for analysis e.g. 'alayer=1'.    
        """

        inParams = []
        for col in self.vnetParams[self.currAnModule]["cmdParams"]["cols"]:

            if "inputField" in self.attrCols[col]:
                colInptF = self.attrCols[col]["inputField"]
            else:
                colInptF = col

            inParams.append(col + '=' + self.inputData[colInptF].GetValue())

        for layer in ['alayer', 'nlayer']:
            inParams.append(layer + "=" + self.inputData[layer].GetValue().strip())

        return inParams

    def _getPtByCat(self):
        """!Return points separated by theirs categories"""
        cats = self.vnetParams[self.currAnModule]["cmdParams"]["cats"]

        ptByCats = {}
        for cat in self.vnetParams[self.currAnModule]["cmdParams"]["cats"]:
            ptByCats[cat[0]] = []
 
        for i in range(len(self.list.itemDataMap)):
            key = self.list.GetItemData(i)
            if self.list.IsChecked(key):
                for cat in cats:
                    if cat[1] == self.list.itemDataMap[key][1] or len(ptByCats) == 1: 
                        ptByCats[cat[0]].append(self.pointsToDraw.GetItem(key).GetCoords())
                        continue

        return ptByCats

    def _getAsciiPts (self, catPts, maxCat, layerNum):
        """!Return points separated by categories in GRASS ASCII vector representation"""
        catsNums = {}
        pt_ascii = ""
        catNum = maxCat

        for catName, pts in catPts.iteritems():

            catsNums[catName] = [catNum + 1]
            for pt in pts:
                catNum += 1
                pt_ascii += "P 1 1\n"
                pt_ascii += str(pt[0]) + " " + str(pt[1]) +  "\n"
                pt_ascii += str(layerNum) + " " + str(catNum) + "\n"

            catsNums[catName].append(catNum)

        return pt_ascii, catsNums

    def _prepareCmd(self, cmd):
        """!Helper function for preparation of cmd list into form for RunCmd method"""
        for c in cmd[:]:
            if c.find("=") == -1:
                continue
            v = c.split("=")
            if len(v) != 2:
                cmd.remove(c)
            elif not v[1].strip():
                cmd.remove(c)

    def GetLayerStyle(self):
        """!Returns cmd for d.vect, with set style for analysis result"""
        resProps = self.vnetParams[self.currAnModule]["resultProps"]

        width = UserSettings.Get(group='vnet', key='res_style', subkey= "line_width")
        layerStyleCmd = ["layer=1",'width=' + str(width)]

        if "catColor" in resProps:
            layerStyleCmd.append('flags=c')
        elif "singleColor" in resProps:
            col = UserSettings.Get(group='vnet', key='res_style', subkey= "line_color")
            layerStyleCmd.append('color=' + str(col[0]) + ':' + str(col[1]) + ':' + str(col[2]))        

        if "attrColColor" in resProps:
            colorStyle = UserSettings.Get(group='vnet', key='res_style', subkey= "color_table")
            invert = UserSettings.Get(group='vnet', key='res_style', subkey= "invert_colors")

            layerStyleVnetColors = [
                                    "v.colors",
                                    "map=" + self.tmp_result.GetVectMapName(),
                                    "color=" + colorStyle,
                                    "column=" + resProps["attrColColor"],
                                   ]
            if invert:
                layerStyleVnetColors.append("-n")

            layerStyleVnetColors  = utils.CmdToTuple(layerStyleVnetColors)

            RunCommand( layerStyleVnetColors[0],
                        **layerStyleVnetColors[1])

        return layerStyleCmd 

    def OnShowResult(self, event):
        """!Show/hide analysis result"""
        mainToolbar = self.toolbars['mainToolbar']
        id = vars(mainToolbar)['showResult']
        toggleState = mainToolbar.GetToolState(id)

        if not self.tmp_result:
            mainToolbar.ToggleTool(id =id,
                                   toggle = False)
        elif toggleState:
            self._checkResultMapChanged(self.tmp_result)
            cmd = self.GetLayerStyle()
            self.tmp_result.AddRenderLayer(cmd)
        else:
            cmd = self.GetLayerStyle()
            self.tmp_result.DeleteRenderLayer()

        up_map_evt = gUpdateMap(render = True, renderVector = True)
        wx.PostEvent(self.mapWin, up_map_evt)

    def OnInsertPoint(self, event):
        """!Registers/unregisters mouse handler into map window"""
        if self.handlerRegistered == False:
            self.mapWin.RegisterMouseEventHandler(wx.EVT_LEFT_DOWN, 
                                                  self.OnMapClickHandler,
                                                  wx.StockCursor(wx.CURSOR_CROSS))
            self.handlerRegistered = True
        else:
            self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN, 
                                                  self.OnMapClickHandler)
            self.handlerRegistered = False

    def OnSaveTmpLayer(self, event):
        """!Permanently saves temporary map of analysis result"""
        dlg = AddLayerDialog(parent = self)

        msg = _("Vector map with analysis result does not exist.")
        if dlg.ShowModal() == wx.ID_OK: 

            if not hasattr(self.tmp_result, "GetVectMapName"):
                GMessage(parent = self,
                         message = msg)
                return

            mapToAdd = self.tmp_result.GetVectMapName()
            mapToAddEx = grass.find_file(name = mapToAdd, 
                                        element = 'vector', 
                                        mapset = grass.gisenv()['MAPSET'])

            if not mapToAddEx["name"]: 
                GMessage(parent = self,
                         message = msg)
                dlg.Destroy()
                return

            addedMap = dlg.vectSel.GetValue()
            mapName, mapSet = self._parseMapStr(addedMap)
            if mapSet !=  grass.gisenv()['MAPSET']:
                GMessage(parent = self,
                         message = _("Map can be saved only to currently set mapset"))
                return
            existsMap = grass.find_file(name = mapName, 
                                        element = 'vector', 
                                        mapset = grass.gisenv()['MAPSET'])
            dlg.Destroy()
            if existsMap["name"]:
                dlg = wx.MessageDialog(parent = self.parent.parent,
                                       message = _("Vector map %s already exists. " +
                                                "Do you want to overwrite it?") % 
                                                (existsMap["fullname"]),
                                       caption = _("Overwrite map layer"),
                                       style = wx.YES_NO | wx.NO_DEFAULT |
                                               wx.ICON_QUESTION | wx.CENTRE)            
                ret = dlg.ShowModal()
                if ret == wx.ID_NO:
                    dlg.Destroy()
                    return
                dlg.Destroy()

            RunCommand("g.copy",
                       overwrite = True,
                       vect = [self.tmp_result.GetVectMapName(), mapName])

            cmd = self.GetLayerStyle()#TODO get rid of insert
            cmd.insert(0, 'd.vect')
            cmd.append('map=%s' % mapName)

            if not self.mapWin.tree:
                return

            if  self.mapWin.tree.FindItemByData(key = 'name', value = mapName) is None: 
                self.mapWin.tree.AddLayer(ltype = "vector", 
                                          lname = mapName,
                                          lcmd = cmd,
                                          lchecked = True)
            else:
                up_map_evt = gUpdateMap(render = True, renderVector = True)
                wx.PostEvent(self.mapWin, up_map_evt)

    def OnSettings(self, event):
        """!Displays vnet settings dialog"""
        dlg = SettingsDialog(parent=self, id=wx.ID_ANY, title=_('Settings'))
        
        if dlg.ShowModal() == wx.ID_OK:
            pass
        
        dlg.Destroy()

    def OnAnalysisChanged(self, event):
        """!Updates dialog when analysis is changed"""
        # set chosen analysis
        iAn = self.toolbars['analysisToolbar'].anChoice.GetSelection() 
        self.currAnModule = self.vnetModulesOrder[iAn]

        # update dialog for particular analysis
        if self.currAnModule == "v.net.iso":
            self.anSettings['iso_lines'].GetParent().Show()
        else:
            self.anSettings['iso_lines'].GetParent().Hide()

        #if self.currAnModule == "v.net.flow": TODO not implemented
        #    self.anSettings['show_cut'].GetParent().Show()
        #else:
        #    self.anSettings['show_cut'].GetParent().Hide()

        # show only corresponding selects for chosen v.net module
        skip = []
        for col in self.attrCols.iterkeys():
            if "inputField" in self.attrCols[col]:
                colInptF = self.attrCols[col]["inputField"]
            else:
                colInptF = col

            if col in skip:
                continue

            inputPanel = self.inputData[colInptF].GetParent()
            if col in self.vnetParams[self.currAnModule]["cmdParams"]["cols"]:
                inputPanel.Show()
                inputPanel.FindWindowByName(colInptF).SetLabel(self.attrCols[col]["label"])
                inputPanel.Layout()
                if col != colInptF:
                    skip.append(colInptF)
            else:
                self.inputData[colInptF].GetParent().Hide()
        self.Layout()

        # if module has only one category -> hide type column in points list otherwise show it
        if len(self.vnetParams[self.currAnModule]["cmdParams"]["cats"]) > 1:
            if not self.list.IsShown('type'):
                self.list.ShowColumn('type', 1)

            currParamsCats = self.vnetParams[self.currAnModule]["cmdParams"]["cats"]
            self.list.AdaptPointsList(currParamsCats)
        else:
            if self.list.IsShown('type'):
                self.list.HideColumn('type')

        # for v.net.path just one 'Start point' and one 'End point' can be checked
        if self.currAnModule == "v.net.path":
            self.list.UpdateCheckedItems(index = None)

    def OnSnapping(self, event):
        """!Start/stop snapping mode"""
        ptListToolbar = self.toolbars['pointsList']

        if not haveCtypes:
            ptListToolbar.ToggleTool(id = ptListToolbar.GetToolId("snapping"),
                                     toggle = False)
            GMessage(parent = self,
                     message = _("Unable to use ctypes. \n") + \
                               _("Snapping mode can not be activated."))
            return

        if not event or not event.IsChecked():
            if not event: 
                ptListToolbar.ToggleTool(id = ptListToolbar.GetToolId("snapping"),
                                         toggle = False)
            if self.tmpMaps.HasTmpVectMap("vnet_snap_points"):
                self.snapPts.DeleteRenderLayer() 
                
                up_map_evt = gUpdateMap(render = False, renderVector = False)
                wx.PostEvent(self.mapWin, up_map_evt)

            if self.snapData.has_key('cmdThread'):
                self.snapData['cmdThread'].abort()

            self.snapData['snap_mode'] = False
            return  

        if not self.InputsErrorMsgs(msg = _("Snapping mode can not be activated."),
                                    inpToTest = ["input", "nlayer"]):

            ptListToolbar.ToggleTool(id = ptListToolbar.GetToolId("snapping"),
                                     toggle = False)
            return

        if not self.tmpMaps.HasTmpVectMap("vnet_snap_points"):
            endStr = _("Do you really want to activate snapping and overwrite it?")
            self.snapPts = self.tmpMaps.AddTmpVectMap("vnet_snap_points", endStr)

            if not self.snapPts:
                ptListToolbar.ToggleTool(id = ptListToolbar.GetToolId("snapping"),
                                         toggle = False)
                return   
        elif self.snapPts.VectMapState() == 0:
                dlg = wx.MessageDialog(parent = self.parent,
                                       message = _("Temporary map '%s' was changed outside " +
                                                    "vector analysis tool.\n" 
                                                    "Do you really want to activate " + 
                                                    "snapping and overwrite it? ") % \
                                                    self.snapPts.GetVectMapName(),
                                        caption = _("Overwrite map"),
                                        style = wx.YES_NO | wx.NO_DEFAULT |
                                                wx.ICON_QUESTION | wx.CENTRE)

                ret = dlg.ShowModal()
                dlg.Destroy()
                
                if ret == wx.ID_NO:
                    self.tmpMaps.DeleteTmpMap(self.snapPts)
                    ptListToolbar.ToggleTool(id = ptListToolbar.GetToolId("snapping"),
                                             toggle = False)
                    return

        self.snapData['snap_mode'] = True

        inpName = self.inputData['input'].GetValue()
        inpName, mapSet = self._parseMapStr(inpName)
        inpFullName = inpName + '@' + mapSet
        computeNodes = True

        if not self.snapData.has_key("inputMap"):
            pass
        elif inpFullName != self.snapData["inputMap"].GetVectMapName():
            self.snapData["inputMap"] = VectMap(self, inpFullName)
        elif self.snapData["inputMapNlayer"] == self.inputData["nlayer"].GetValue():
            if self.snapData["inputMap"].VectMapState() == 1:
                computeNodes = False
    
        # new map need
        if computeNodes:
            self.stBar.AddStatusItem(text = _('Computing nodes...'),
                                     key = 'snap',
                                     priority = self.stPriorities['important'])
            if not self.snapData.has_key('cmdThread'):
                self.snapData['cmdThread'] = CmdThread(self)

            cmd = ["v.to.points", "input=" + self.inputData['input'].GetValue(), 
                                  "output=" + self.snapPts.GetVectMapName(),
                                  "llayer=" + self.inputData["nlayer"].GetValue(),
                                  "-n", "--overwrite"]
            # process GRASS command with argument
            self.snapData["inputMap"] = VectMap(self, inpFullName)
            self.snapData["inputMap"].SaveVectMapState()

            self.Bind(EVT_CMD_DONE, self._onToPointsDone)
            self.snapData['cmdThread'].RunCmd(cmd)

            self.snapData["inputMapNlayer"] = self.inputData["nlayer"].GetValue()
        # map is already created and up to date for input data
        else:
            self.snapPts.AddRenderLayer()

            up_map_evt = gUpdateMap(render = True, renderVector = True)
            wx.PostEvent(self.mapWin, up_map_evt)

    def _onToPointsDone(self, event):
        """!Update map window, when map with nodes to snap is created"""
        self.stBar.RemoveStatusItem(key = 'snap')
        if not event.aborted:
            self.snapPts.SaveVectMapState()
            self.snapPts.AddRenderLayer() 

            up_map_evt = gUpdateMap(render = True, renderVector = True)
            wx.PostEvent(self.mapWin, up_map_evt)

    def OnUndo(self, event):
        """!Step back in history"""
        histStepData = self.history.GetPrev()
        self.toolbars['mainToolbar'].UpdateUndoRedo()

        if histStepData:
            self._updateHistStepData(histStepData)

    def OnRedo(self, event):
        """!Step forward in history"""
        histStepData = self.history.GetNext()
        self.toolbars['mainToolbar'].UpdateUndoRedo()

        if histStepData:
            self._updateHistStepData(histStepData)

    def _saveAnInputToHist(self):
        """!Save all data needed for analysis into history buffer"""
        pts = self.pointsToDraw.GetAllItems()

        for iPt, pt in enumerate(pts):
            ptName = "pt" + str(iPt)

            coords = pt.GetCoords()
            self.history.Add(key = "points", 
                             subkey = [ptName, "coords"], 
                             value = coords)
            # save type column
            # if is shown
            if  len(self.vnetParams[self.currAnModule]["cmdParams"]["cats"]) > 1:
                cat = self.list.GetCellSelIdx(iPt, 'type')
                self.history.Add(key = "points", 
                                 subkey = [ptName, "catIdx"], 
                                 value = cat)
            # if is hidden
            else:
                self.history.Add(key = 'points_hidden_cols', 
                                 subkey = 'type', 
                                 value = self.list.GetHiddenColSelIdxs('type'))

            topology = self.list.GetCellValue(iPt, 'topology')
            self.history.Add(key = "points", 
                             subkey = [ptName, "topology"], 
                             value = topology)


            self.history.Add(key = "points", 
                             subkey = [ptName, "checked"], 
                             value = self.list.IsChecked(iPt))

            for inpName, inp in self.inputData.iteritems():

                if inpName == "input":
                    vectMapName, mapSet = self._parseMapStr(inp.GetValue())
                    inpMapFullName = vectMapName + '@' + mapSet
                    inpMap = VectMap(self, inpMapFullName)
                    self.history.Add(key = "other", 
                                     subkey = "input_modified", 
                                     value = inpMap.GetLastModified())  
                    inpVal =  inpMapFullName
                else:
                    inpVal =  inp.GetValue()
                 
                self.history.Add(key = "input_data", 
                                 subkey = inpName, 
                                 value = inpVal)
            
        self.history.Add(key = "vnet_modules", 
                         subkey = "curr_module", 
                         value = self.currAnModule)

        for settName, sett in self.anSettings.iteritems():
            self.history.Add(key = "an_settings", 
                             subkey = settName, 
                             value = sett.GetValue())


    def _saveHistStep(self):
        """!Save new step into history"""
        removedHistData = self.history.SaveHistStep()
        self.toolbars['mainToolbar'].UpdateUndoRedo()

        if not removedHistData:
            return

        # delete temporary maps in history steps which were deleted 
        for removedStep in removedHistData.itervalues():
            mapsNames = removedStep["tmp_data"]["maps"]
            for vectMapName in mapsNames:
                tmpMap = self.tmpMaps.GetTmpVectMap(vectMapName)
                self.tmpMaps.DeleteTmpMap(tmpMap)

    def _updateHistStepData(self, histStepData):
        """!Updates dialog according to chosen history step"""

        # set analysis module
        self.currAnModule = histStepData["vnet_modules"]["curr_module"]

        # optimization -map is not re-rendered when change in points list is done
        self.list.SetUpdateMap(updateMap = False)

        # delete points list items
        while self.list.GetSelected() != wx.NOT_FOUND:
            self.list.DeleteItem()

        # show/hide 'type' column according to particular analysis
        if  len(self.vnetParams[self.currAnModule]["cmdParams"]["cats"]) > 1:
            hasType = True
            self.list.ShowColumn('type', 1)
        else:
            hasType = False
            self.list.HideColumn('type')

        # add points to list
        for iPt in range(len(histStepData["points"])):
            ptData = histStepData["points"]["pt" + str(iPt)]
            coords = ptData["coords"]
            self.list.AddItem()
            item = self.pointsToDraw.GetItem(iPt)
            item.SetCoords(coords)

            if hasType:
                self.list.EditCellKey(iPt, 'type', int(ptData["catIdx"]))

            self.list.EditCellKey(iPt, 'topology', ptData["topology"])           

            if ptData["checked"]:
                self.list.CheckItem(iPt, True)

        if hasType:
            currParamsCats = self.vnetParams[self.currAnModule]["cmdParams"]["cats"]
            self.list.AdaptPointsList(currParamsCats)
        else:
            self.list.SetHiddenSelIdxs('type', histStepData["points_hidden_cols"]["type"])

        # set analysis combobox
        anChoice = self.toolbars['analysisToolbar'].anChoice
        anChoice.SetSelection(self.vnetModulesOrder.index(self.currAnModule)) 

        # update analysis result maps 
        mapsNames = histStepData["tmp_data"]["maps"]
        for vectMapName in mapsNames:
            if "vnet_tmp_result" in vectMapName:
                self.tmp_result.DeleteRenderLayer()
                self.tmp_result  = self.tmpMaps.GetTmpVectMap(vectMapName)
                self._checkResultMapChanged(self.tmp_result)

                cmd = self.GetLayerStyle()
                self.tmp_result.AddRenderLayer(cmd)

        # update Parameters tab
        histInputData = histStepData["input_data"]
        for inpName, inp in histInputData.iteritems():
            self.inputData[inpName].SetValue(str(inp)) 
            if inpName == "input":
                inpMap = inp

        prevInpModTime = str(histStepData["other"]["input_modified"])
        currInpModTime = VectMap(self, inpMap).GetLastModified()

        if currInpModTime.strip()!= prevInpModTime.strip():
            dlg = wx.MessageDialog(parent = self,
                                   message = _("Input map '%s' for analysis was changed outside " + 
                                               "vector network analysis tool.\n" +
                                               "Topology column may not " +
                                               "correspond to changed situation.") %\
                                                inpMap,
                                   caption = _("Input changed outside"),
                                   style =  wx.ICON_INFORMATION| wx.CENTRE)
            dlg.ShowModal()
            dlg.Destroy()

        # update Points tab (analysis settings)
        histAnSettData = histStepData["an_settings"]
        for settName, sett in histAnSettData.iteritems():
            if settName == 'iso_lines':
                sett = str(sett)
            self.anSettings[settName].SetValue(sett) 

        self.resultDbMgrData['analysis'] = self.currAnModule
        self._updateResultDbMgrPage()
        self._updateDbMgrData()

        self.OnAnalysisChanged(None)
        self.list.SetUpdateMap(updateMap = True)

        up_map_evt = gUpdateMap(render = True, renderVector = True)
        wx.PostEvent(self.mapWin, up_map_evt)
    
    def _checkResultMapChanged(self, resultVectMap):
        """!Check if map was modified outside"""
        if resultVectMap.VectMapState() == 0:
            dlg = wx.MessageDialog(parent = self,
                                   message = _("Temporary map '%s' with result " + 
                                               "was changed outside vector network analysis tool.\n" +
                                               "Showed result may not correspond " +
                                               "original analysis result.") %\
                                               resultVectMap.GetVectMapName(),
                                   caption = _("Result changed outside"),
                                   style =  wx.ICON_INFORMATION| wx.CENTRE)
            dlg.ShowModal()
            dlg.Destroy()

    def NewTmpVectMapToHist(self, prefMapName):
        """!Add new vector map, which will be saved into history step"""

        mapName = prefMapName + str(self.histTmpVectMapNum)
        self.histTmpVectMapNum += 1

        tmpMap = self._addTmpMapAnalysisMsg(mapName)
        if not tmpMap:
            return tmpMap
           
        self.tmpVectMapsToHist.append(tmpMap.GetVectMapName())
        self.history.Add(key = "tmp_data", 
                         subkey = "maps",
                         value = self.tmpVectMapsToHist)

        return tmpMap

    def _addTmpMapAnalysisMsg(self, mapName):
        """!Wraped AddTmpVectMap"""
        msg = _("Temporary map %s  already exists.\n"  + 
                "Do you want to continue in analysis and overwrite it?") \
                 % (mapName +'@' + grass.gisenv()['MAPSET'])
        tmpMap = self.tmpMaps.AddTmpVectMap(mapName, msg)
        return tmpMap


    def _initVnetParams(self):
        """!Initializes parameters for different v.net.* modules """

        self.attrCols = {
                          'afcolumn' : {
                                        "label" : _("Arc forward/both direction(s) cost column:"),
                                        "name" : _("arc forward/both")
                                       },
                          'abcolumn' : {
                                        "label" : _("Arc backward direction cost column:"),
                                        "name" : _("arc backward")
                                       },
                          'acolumn' : {
                                       "label" : _("Arcs' cost column (for both directions):"),
                                       "name" : _("arc"),
                                       "inputField" : 'afcolumn',
                                      },
                          'ncolumn' : {
                                       "label" : _("Node cost column:"),
                                        "name" : _("node")                                      
                                      }
                        }

        self.vnetParams = {
                                   "v.net.path" : {
                                                     "label" : _("Shortest path %s") % "(v.net.path)",  
                                                     "cmdParams" : {
                                                                      "cats" :  [
                                                                                    ["st_pt", _("Start point")], 
                                                                                    ["end_pt", _("End point")] 
                                                                                ],
                                                                      "cols" :  [
                                                                                 'afcolumn',
                                                                                 'abcolumn',
                                                                                 'ncolumn'
                                                                                ],
                                                                   },
                                                     "resultProps" : {
                                                                      "singleColor" : None,
                                                                      "dbMgr" : True  
                                                                     }
                                                  },

                                    "v.net.salesman" : {
                                                        "label" : _("Traveling salesman %s") % "(v.net.salesman)",  
                                                        "cmdParams" : {
                                                                        "cats" : [["ccats", None]],
                                                                        "cols" : [
                                                                                  'afcolumn',
                                                                                  'abcolumn'
                                                                                 ],
                                                                      },
                                                        "resultProps" : {
                                                                         "singleColor" : None,
                                                                         "dbMgr" : False
                                                                        }
                                                       },
                                    "v.net.flow" : {
                                                     "label" : _("Maximum flow %s") % "(v.net.flow)",  
                                                     "cmdParams" : {
                                                                      "cats" : [
                                                                                ["source_cats", _("Source point")], 
                                                                                ["sink_cats", _("Sink point")]
                                                                               ],                                                   
                                                                      "cols" : [
                                                                                'afcolumn',
                                                                                'abcolumn',
                                                                                'ncolumn'
                                                                               ]
                                                                  },
                                                     "resultProps" : {
                                                                      "attrColColor": "flow",
                                                                      "dbMgr" : True
                                                                     }
                                                   },
                                    "v.net.alloc" : {
                                                     "label" : _("Subnets for nearest centers %s") % "(v.net.alloc)",  
                                                     "cmdParams" : {
                                                                      "cats" : [["ccats", None]],                           
                                                                      "cols" : [
                                                                                 'afcolumn',
                                                                                 'abcolumn',
                                                                                 'ncolumn'
                                                                               ]
                                                                  },
                                                     "resultProps" :  {
                                                                       "catColor" : None, 
                                                                       "dbMgr" : False
                                                                      }
                                                   },
                                    "v.net.steiner" : {
                                                     "label" : _("Steiner tree for the network and given terminals %s") % "(v.net.steiner)",  
                                                     "cmdParams" : {
                                                                      "cats" : [["tcats", None]],                           
                                                                      "cols" : [
                                                                                 'acolumn',
                                                                               ]
                                                                  },
                                                     "resultProps" : {
                                                                      "singleColor" : None,
                                                                      "dbMgr" : False 
                                                                     }
                                                   },
                                   "v.net.distance" : {
                                                       "label" : _("Shortest distance via the network %s") % "(v.net.distance)",  
                                                       "cmdParams" : {
                                                                        "cats" : [
                                                                                  ["from_cats", "From point"],
                                                                                  ["to_cats", "To point"]
                                                                                 ],
                                                                        "cols" : [
                                                                                  'afcolumn',
                                                                                  'abcolumn',
                                                                                  'ncolumn'
                                                                                 ],
                                                                  },
                                                      "resultProps" : {
                                                                        "catColor" : None,
                                                                        "dbMgr" : True
                                                                      }
                                                     },
                                    "v.net.iso" :  {
                                                     "label" : _("Cost isolines %s") % "(v.net.iso)",  
                                                     "cmdParams" : {
                                                                      "cats" : [["ccats", None]],                           
                                                                      "cols" : [
                                                                                 'afcolumn',
                                                                                 'abcolumn',
                                                                                 'ncolumn'
                                                                               ]
                                                                  },
                                                     "resultProps" : {
                                                                      "catColor" : None,
                                                                      "dbMgr" : False
                                                                     }
                                                   }
                                }
        # order in combobox for choose of analysis
        self.vnetModulesOrder = ["v.net.path", 
                                 "v.net.salesman",
                                 "v.net.flow",
                                 "v.net.alloc",
                                 "v.net.distance",
                                 "v.net.iso",
                                 #"v.net.steiner"
                                 ] 
        self.currAnModule = self.vnetModulesOrder[0]

    def _initSettings(self):
        """!Initialization of settings (if not already defined)"""
        # initializes default settings
        initSettings = [
                        ['res_style', 'line_width', 5],
                        ['res_style', 'line_color', (192,0,0)],
                        ['res_style', 'color_table', 'byr'],
                        ['res_style', 'invert_colors', False],
                        ['point_symbol', 'point_size', 10],             
                        ['point_symbol', 'point_width', 2],
                        ['point_colors', "unused", (131,139,139)],
                        ['point_colors', "used1cat", (192,0,0)],
                        ['point_colors', "used2cat", (0,0,255)],
                        ['point_colors', "selected", (9,249,17)],
                        ['other', "snap_tresh", 10],
                        ['other', "max_hist_steps", 5]
                       ]

        for init in initSettings: 
            UserSettings.ReadSettingsFile()
            UserSettings.Append(dict = UserSettings.userSettings, 
                                    group ='vnet',
                                    key = init[0],
                                    subkey =init[1],
                                    value = init[2],
                                    overwrite = False)


    def SetPointDrawSettings(self):
        """!Set settings for drawing of points"""
        ptSize = int(UserSettings.Get(group='vnet', key='point_symbol', subkey = 'point_size'))
        self.pointsToDraw.SetPropertyVal("size", ptSize)

        colors = UserSettings.Get(group='vnet', key='point_colors')
        ptWidth = int(UserSettings.Get(group='vnet', key='point_symbol', subkey = 'point_width'))

        textProp = self.pointsToDraw.GetPropertyVal("text")
        textProp["font"].SetPointSize(ptSize + 2)
    
        for colKey, col in colors.iteritems():
            pen = self.pointsToDraw.GetPen(colKey)
            if pen:
                pen.SetColour(wx.Colour(col[0], col[1], col[2], 255))
                pen.SetWidth(ptWidth)
            else:
                self.pointsToDraw.AddPen(colKey, wx.Pen(colour = wx.Colour(col[0], col[1], col[2], 255), width = ptWidth))

class PtsList(PointsList):
    def __init__(self, parent, dialog, cols, id=wx.ID_ANY):
        """! List with points for analysis"""
        self.updateMap = True
        self.dialog = dialog # VNETDialog class

        PointsList.__init__(self, parent = parent, cols = cols, id =  id)      

    def AddItem(self, event = None, updateMap = True):
        """!Append point to list"""
        self.dialog.pointsToDraw.AddItem(coords = [0,0])

        PointsList.AddItem(self, event)

        self.EditCellKey(key = self.selected , 
                         colName = 'topology', 
                         cellData = _("new point"))  
 
    def DeleteItem(self, event = None):
        """!Delete selected point in list"""
        key = self.GetItemData(self.selected)
        if self.selected != wx.NOT_FOUND:
            item = self.dialog.pointsToDraw.GetItem(key)
            self.dialog.pointsToDraw.DeleteItem(item)

        PointsList.DeleteItem(self, event)

    def OnItemSelected(self, event):
        """!Item selected"""
        PointsList.OnItemSelected(self, event)

        if self.updateMap:
            up_map_evt = gUpdateMap(render = False, renderVector = False)
            wx.PostEvent(self.dialog.mapWin, up_map_evt)

    def AdaptPointsList(self, currParamsCats):
        """Rename category values when module is changed. Expample: Start point -> Sink point"""
        colValues = [""]

        for ptCat in currParamsCats:
            colValues.append(ptCat[1])
        colNum = self._getColumnNum('type')
        self.ChangeColEditable('type', colValues)

        for iItem, item in enumerate(self.itemDataMap): 
            self.EditCellKey(iItem, 'type', self.selIdxs[iItem][colNum])

            if not item[1]:               
                self.CheckItem(iItem, False)
   
    def OnCheckItem(self, index, flag):
        """!Item is checked/unchecked"""
        key = self.GetItemData(index)
        checkedVal = self.itemDataMap[key][1]

        currModule = self.dialog.currAnModule #TODO public func
        cats = self.dialog.vnetParams[currModule]["cmdParams"]["cats"]

        if self.updateMap:
            up_map_evt = gUpdateMap(render = False, renderVector = False)
            wx.PostEvent(self.dialog.mapWin, up_map_evt)

        if len(cats) <= 1:
            return 

        if checkedVal == "":
            self.CheckItem(key, False)
            return

        if currModule == "v.net.path" and flag:
            self.UpdateCheckedItems(index)

    def UpdateCheckedItems(self, index):
        """!For v.net.path - max. just one checked 'Start point' and 'End point'"""
        alreadyChecked = []
        colNum = self._getColumnNum('type')
        if colNum == -1:
            return 
        if index:
            checkedKey = self.GetItemData(index)
            checkedVal = self.selIdxs[checkedKey][colNum]
            alreadyChecked.append(checkedVal)
        else:
            checkedKey = -1

        for iKey, idx in enumerate(self.selIdxs):
            index = self._findIndex(iKey)
            if (idx[colNum] in alreadyChecked and checkedKey != iKey) \
               or idx[colNum] == 0:
                self.CheckItem(index, False)
            elif self.IsChecked(index):
                alreadyChecked.append(idx[colNum])

    def SetUpdateMap(self, updateMap):
        """!Update/Not update map window when some change in list is made"""
        self.updateMap = updateMap

    def GetHiddenColSelIdxs(self, colName):
        """!Get indexes of chosen values in hidden 'type' column"""
        if self.hiddenCols.has_key(colName):
            return self.hiddenCols[colName]['selIdxs']
        return None

    def SetHiddenSelIdxs(self, colName, selIdxs):
        """!Set indexes of chosen values in hidden 'type' column and update text in it's cells"""
        if self.hiddenCols.has_key(colName):
            self.hiddenCols[colName]['selIdxs'] = map(int, selIdxs)
            self.hiddenCols[colName]['itemDataMap'] = []
            # update text in hidden column cells
            for idx in self.hiddenCols[colName]['selIdxs']:
                    self.hiddenCols[colName]['itemDataMap'].append(self.hiddenCols[colName]['colsData'][2][idx])
            return True
        return False

class SettingsDialog(wx.Dialog):
    def __init__(self, parent, id, title, pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE):
        """!Settings dialog"""
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)

        maxValue = 1e8
        self.parent = parent
        self.settings = {}

        rules = RunCommand('v.colors', 
                           read = True,
                           flags = 'l')

        settsLabels = {} 

        settsLabels['color_table'] = wx.StaticText(parent = self, id = wx.ID_ANY, 
                                                   label = _('Color table style %s:') % '(v.net.flow)')
        self.settings['color_table'] = wx.ComboBox(parent = self, id = wx.ID_ANY,
                                                   choices = rules.split(),
                                                   style = wx.CB_READONLY, size = (180, -1))

        setStyle = UserSettings.Get(group ='vnet', key = "res_style", subkey = "color_table")
        i = self.settings['color_table'].FindString(setStyle)
        if i != wx.NOT_FOUND: 
            self.settings['color_table'].Select(i)

        self.settings["invert_colors"] = wx.CheckBox(parent = self, id=wx.ID_ANY,
                                                       label = _('Invert colors %s:') % '(v.net.flow)')
        setInvert = UserSettings.Get(group ='vnet', key = "res_style", subkey = "invert_colors")
        self.settings["invert_colors"].SetValue(setInvert)

        self.colorsSetts = {
                            "line_color" : ["res_style", _("Line color:")],
                            "unused" : ["point_colors", _("Color for unused point:")], 
                            "used1cat" : ["point_colors", _("Color for Start/From/Source/Used point:")],
                            "used2cat" : ["point_colors", _("Color for End/To/Sink point:")],
                            "selected" : ["point_colors", _("Color for selected point:")]
                           }

        for settKey, sett in self.colorsSetts.iteritems():
            settsLabels[settKey] = wx.StaticText(parent = self, id = wx.ID_ANY, label = sett[1])
            col = UserSettings.Get(group ='vnet', key = sett[0], subkey = settKey)        
            self.settings[settKey] = csel.ColourSelect(parent = self, id = wx.ID_ANY,
                                            colour = wx.Colour(col[0],
                                                               col[1],
                                                               col[2], 
                                                               255))

        self.sizeSetts = {
                          "line_width" : ["res_style", _("Line width:")],
                          "point_size" : ["point_symbol", _("Point size:")], 
                          "point_width" : ["point_symbol", _("Point width:")],
                          "snap_tresh" : ["other", _("Snapping treshold in pixels:")],
                          "max_hist_steps" : ["other", _("Maximum number of results in history:")]
                         }

        for settKey, sett in self.sizeSetts.iteritems():
            settsLabels[settKey] = wx.StaticText(parent = self, id = wx.ID_ANY, label = sett[1])
            self.settings[settKey] = wx.SpinCtrl(parent = self, id = wx.ID_ANY, min = 1, max = 50)
            size = int(UserSettings.Get(group = 'vnet', key = sett[0], subkey = settKey))
            self.settings[settKey].SetValue(size)


        # buttons
        self.btnSave = wx.Button(self, wx.ID_SAVE)
        self.btnApply = wx.Button(self, wx.ID_APPLY)
        self.btnClose = wx.Button(self, wx.ID_CLOSE)
        self.btnApply.SetDefault()

        # bindings
        self.btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        self.btnApply.SetToolTipString(_("Apply changes for the current session"))
        self.btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        self.btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        self.btnClose.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btnClose.SetToolTipString(_("Close dialog"))

        #Layout

        # Analysis result style layout
        self.SetMinSize(self.GetBestSize())

        sizer = wx.BoxSizer(wx.VERTICAL)

        styleBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                label =" %s " % _("Analysis result style:"))
        styleBoxSizer = wx.StaticBoxSizer(styleBox, wx.VERTICAL)

        gridSizer = wx.GridBagSizer(vgap = 1, hgap = 1)

        row = 0
        gridSizer.Add(item =  settsLabels["line_color"], flag = wx.ALIGN_CENTER_VERTICAL, pos =(row, 0))
        gridSizer.Add(item = self.settings["line_color"],
                      flag = wx.ALIGN_RIGHT | wx.ALL, border = 5,
                      pos =(row, 1))
 
        row += 1
        gridSizer.Add(item =  settsLabels["line_width"], flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        gridSizer.Add(item = self.settings["line_width"],
                      flag = wx.ALIGN_RIGHT | wx.ALL, border = 5,
                      pos = (row, 1))
        row += 1
        gridSizer.Add(item = settsLabels['color_table'], flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        gridSizer.Add(item = self.settings['color_table'],
                      flag = wx.ALIGN_RIGHT | wx.ALL, border = 5,
                      pos = (row, 1))

        row += 1
        gridSizer.Add(item = self.settings["invert_colors"], flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))

        gridSizer.AddGrowableCol(1)
        styleBoxSizer.Add(item = gridSizer, flag = wx.EXPAND)

        # Point style layout
        ptsStyleBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                   label =" %s " % _("Point style:"))
        ptsStyleBoxSizer = wx.StaticBoxSizer(ptsStyleBox, wx.VERTICAL)

        gridSizer = wx.GridBagSizer(vgap = 1, hgap = 1)

        row = 0
        setts = dict(self.colorsSetts.items() + self.sizeSetts.items())

        settsOrder = ["selected", "used1cat", "used2cat", "unused", "point_size", "point_width"]
        for settKey in settsOrder:
            sett = setts[settKey]
            gridSizer.Add(item = settsLabels[settKey], flag = wx.ALIGN_CENTER_VERTICAL, pos =(row, 0))
            gridSizer.Add(item = self.settings[settKey],
                          flag = wx.ALIGN_RIGHT | wx.ALL, border = 5,
                          pos =(row, 1))  
            row += 1

        gridSizer.AddGrowableCol(1)
        ptsStyleBoxSizer.Add(item = gridSizer, flag = wx.EXPAND)

        # Other settings layout
        otherBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                label =" %s " % _("Other settings"))
        otherBoxSizer = wx.StaticBoxSizer(otherBox, wx.VERTICAL)

        gridSizer = wx.GridBagSizer(vgap = 1, hgap = 1)

        row = 0 
        for otherSettName in ["snap_tresh", "max_hist_steps"]:
            gridSizer.Add(item = settsLabels[otherSettName], flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            gridSizer.Add(item = self.settings[otherSettName],
                          flag = wx.ALIGN_RIGHT | wx.ALL, border = 5,
                          pos = (row, 1))
            row += 1

        gridSizer.AddGrowableCol(1)
        otherBoxSizer.Add(item = gridSizer, flag = wx.EXPAND)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnApply, flag = wx.LEFT | wx.RIGHT, border = 5)
        btnSizer.Add(self.btnSave, flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btnClose, flag = wx.LEFT | wx.RIGHT, border = 5)

        sizer.Add(item = styleBoxSizer, flag = wx.EXPAND | wx.ALL, border = 5)
        sizer.Add(item = ptsStyleBoxSizer, flag = wx.EXPAND | wx.ALL, border = 5)
        sizer.Add(item = otherBoxSizer, flag = wx.EXPAND | wx.ALL, border = 5)
        sizer.Add(item = btnSizer, flag = wx.EXPAND | wx.ALL, border = 5, proportion = 0)    

        self.SetSizer(sizer)
        sizer.Fit(self)
     
    def OnSave(self, event):
        """!Button 'Save' pressed"""
        self.UpdateSettings()

        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings['vnet'] = UserSettings.Get(group='vnet')
        UserSettings.SaveToFile(fileSettings)

        self.Close()

    def UpdateSettings(self):
        UserSettings.Set(group ='vnet', key = "res_style", subkey ='line_width',
                         value = self.settings["line_width"].GetValue())

        for settKey, sett in self.colorsSetts.iteritems():
            col = tuple(self.settings[settKey].GetColour())
            UserSettings.Set(group = 'vnet', 
                             key = sett[0], 
                             subkey = settKey,
                             value = col)

        for settKey, sett in self.sizeSetts.iteritems():
            UserSettings.Set(group = 'vnet', key = sett[0], subkey = settKey, 
                             value = self.settings[settKey].GetValue())

        UserSettings.Set(group = 'vnet', key = 'res_style', subkey = 'color_table', 
                        value = self.settings['color_table'].GetStringSelection())

        UserSettings.Set(group = 'vnet', key = 'res_style', subkey = 'invert_colors', 
                        value = self.settings['invert_colors'].IsChecked())

        self.parent.SetPointDrawSettings()
        if not self.parent.tmp_result  or \
           not self.parent.tmpMaps.HasTmpVectMap(self.parent.tmp_result.GetVectMapName()):
            up_map_evt = gUpdateMap(render = False, renderVector = False)
            wx.PostEvent(self.parent.mapWin, up_map_evt)
        elif self.parent.tmp_result.GetRenderLayer():
            cmd = self.parent.GetLayerStyle()
            self.parent.tmp_result.AddRenderLayer(cmd)
            
            up_map_evt = gUpdateMap(render = True, renderVector = True)
            wx.PostEvent(self.parent.mapWin, up_map_evt)#TODO optimization
        else:
            up_map_evt = gUpdateMap(render = False, renderVector = False)
            wx.PostEvent(self.parent.mapWin, up_map_evt)

    def OnApply(self, event):
        """!Button 'Apply' pressed"""
        self.UpdateSettings()
        #self.Close()

    def OnClose(self, event):
        """!Button 'Cancel' pressed"""
        self.Close()

class AddLayerDialog(wx.Dialog):
    def __init__(self, parent,id=wx.ID_ANY,
                 title =_("Save analysis result"), style=wx.DEFAULT_DIALOG_STYLE):
        """!Save analysis result"""
        wx.Dialog.__init__(self, parent, id, title = _(title), style = style)

        self.panel = wx.Panel(parent = self)
       
        # text fields and it's captions
        self.vectSel = Select(parent = self.panel, type = 'vector', 
                              mapsets = [grass.gisenv()['MAPSET']],size = (-1, -1))
        self.vectSellabel = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                          label = _("Name:")) 

        # buttons
        self.btnCancel = wx.Button(self.panel, wx.ID_CANCEL)
        self.btnOk = wx.Button(self.panel, wx.ID_OK)
        self.btnOk.SetDefault()

        self.SetInitialSize((400, -1))
        self._layout()

    def _layout(self):

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.StaticBox (parent = self.panel, id = wx.ID_ANY,
                            label = "Vector map")

        boxSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)

        boxSizer.Add(item = self.vectSellabel, 
                     flag = wx.ALIGN_CENTER_VERTICAL,
                     proportion = 0)

        boxSizer.Add(item = self.vectSel, proportion = 1,
                     flag = wx.EXPAND | wx.ALL, border = 5)

        sizer.Add(item = boxSizer, proportion = 1,
                  flag = wx.EXPAND | wx.ALL, border = 5)

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()

        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)

        self.panel.SetSizer(sizer)
        sizer.Fit(self)

class VnetTmpVectMaps:
    """!Class which creates, stores and destroys all tmp maps created during analysis"""
    def __init__(self, parent):
        self.tmpMaps = [] # temporary maps 
        self.parent = parent
        self.mapWin = self.parent.mapWin

    def AddTmpVectMap(self, mapName, msg):
        """!New temporary map

            @return instance of VectMap representing temporary map 
        """
        currMapSet = grass.gisenv()['MAPSET']
        tmpMap = grass.find_file(name = mapName, 
                                 element = 'vector', 
                                 mapset = currMapSet)

        fullName = tmpMap["fullname"]
        # map already exists
        if fullName:
            #TODO move dialog out of class, AddTmpVectMap(self, mapName, overvrite = False)
            dlg = wx.MessageDialog(parent = self.parent, 
                                   message = msg,
                                   caption = _("Overwrite map layer"),
                                   style = wx.YES_NO | wx.NO_DEFAULT |
                                   wx.ICON_QUESTION | wx.CENTRE)
                
            ret = dlg.ShowModal()
            dlg.Destroy()
                
            if ret == wx.ID_NO:
                return None
        else:
            fullName = mapName + "@" + currMapSet

        newVectMap = VectMap(self, fullName)
        self.tmpMaps.append(newVectMap)

        return newVectMap

    def HasTmpVectMap(self, vectMapName):
        """ 
            @param vectMapName name of vector map

            @return True if it contains the map
            @return False if not 
        """

        mapValSpl = vectMapName.strip().split("@")
        if len(mapValSpl) > 1:
            mapSet = mapValSpl[1]
        else:
            mapSet = grass.gisenv()['MAPSET']
        mapName = mapValSpl[0] 
        fullName = mapName + "@" + mapSet

        for vectTmpMap in self.tmpMaps:
            if vectTmpMap.GetVectMapName() == fullName:
                return True
        return False

    def GetTmpVectMap(self, vectMapName):
        """ Get instance of VectMap with name vectMapName"""
        for vectMap in self.tmpMaps:
            if vectMap.GetVectMapName() == vectMapName.strip():
                return vectMap
        return None

    def RemoveFromTmpMaps(self, vectMap):
        """!Temporary map is removed from the class instance however it is not deleted

            @param vectMap instance of VectMap class to be removed 

            @return True if was removed
            @return False if does not contain the map
        """
        try:
            self.tmpMaps.remove(vectMap)
            return True
        except ValueError:
            return False

    def DeleteTmpMap(self, vectMap):
        """!Temporary map is removed from the class and it is deleted
        
            @param vectMap instance of VectMap class to be deleted 

            @return True if was removed
            @return False if does not contain the map
        """
        if vectMap:
            vectMap.DeleteRenderLayer()
            RunCommand('g.remove', 
                        vect = vectMap.GetVectMapName())
            self.RemoveFromTmpMaps(vectMap)
            return True
        return False

    def DeleteAllTmpMaps(self):
        """Delete all temporary maps in the class"""
        update = False
        for tmpMap in self.tmpMaps:
            RunCommand('g.remove', 
                        vect = tmpMap.GetVectMapName())
            if tmpMap.DeleteRenderLayer():
                update = True
        return update

class VectMap:
    """!Represents map 
        It can check if it was modified or render it
    """
    def __init__(self, parent, fullName):
        self.fullName = fullName
        self.parent = parent
        self.renderLayer = None
        self.modifTime = None # time, for modification check

    def __del__(self):

        self.DeleteRenderLayer()
   
    def AddRenderLayer(self, cmd = None):
        """!Add map from map window layers to render """
        existsMap = grass.find_file(name = self.fullName, 
                                    element = 'vector', 
                                    mapset = grass.gisenv()['MAPSET'])

        if not existsMap["name"]:
            self.DeleteRenderLayer()
            return False

        if not cmd:
            cmd = []    
        cmd.insert(0, 'd.vect')
        cmd.append('map=%s' % self.fullName)

        if self.renderLayer:       
             self.DeleteRenderLayer()

        self.renderLayer = self.parent.mapWin.Map.AddLayer(ltype = "vector",     command = cmd,
                                                           name = self.fullName, active = True,
                                                           opacity = 1.0,        render = True,       
                                                           pos = -1)
        return True

    def DeleteRenderLayer(self):
        """!Remove map from map window layers to render"""
        if self.renderLayer: 
             self.parent.mapWin.Map.DeleteLayer(self.renderLayer)
             self.renderLayer = None
             return True
        return False

    def GetRenderLayer(self):
        return self.renderLayer

    def GetVectMapName(self):
        return self.fullName

    def SaveVectMapState(self):
        """!Save modification time for vector map"""
        self.modifTime = self.GetLastModified()

    def VectMapState(self):
        """!Checks if map was modified

            @return -1 - if no modification time was saved
            @return  0 - if map was modified
            @return  1 - if map was not modified
        """
        if self.modifTime is None:
            return -1       
        if self.modifTime != self.GetLastModified():
            return 0  
        return 1

    def GetLastModified(self):
        """!Get modification time 

            @return MAP DATE time string from vector map head file 
        """

        mapValSpl = self.fullName.split("@")
        mapSet = mapValSpl[1]
        mapName = mapValSpl[0] 

        headPath =  os.path.join(grass.gisenv()['GISDBASE'],
                                 grass.gisenv()['LOCATION_NAME'],
                                 mapSet,
                                 "vector",
                                 mapName,
                                 "head")
        try:
            head = open(headPath, 'r')
            for line in head.readlines():
                i = line.find('MAP DATE:', )
                if i == 0:
                    head.close()
                    return line.split(':', 1)[1].strip()

            head.close()
            return ""
        except IOError:
            return ""

class History:
    """!Class which reads and saves history data (based on gui.core.settings Settings class file save/load)"""   
    def __init__(self, parent):

        # max number of steps in history (zero based)
        self.maxHistSteps = 3 
        # current history step 
        self.currHistStep = 0
        # number of steps saved in history
        self.histStepsNum = 0

        # dict contains data saved in history for current history step 
        self.currHistStepData = {} 

        # buffer for data to be saved into history 
        self.newHistStepData = {} 

        self.histFile = grass.tempfile()

        # key/value separator
        self.sep = ';'

    def __del__(self):
        grass.try_remove(self.histFile)

    def GetNext(self):
        """!Go one step forward in history"""
        self.currHistStep -= 1
        self.currHistStepData.clear()
        self.currHistStepData = self._getHistStepData(self.currHistStep)

        return self.currHistStepData

    def GetPrev(self):
        """!Go one step back in history"""
        self.currHistStep += 1 
        self.currHistStepData.clear()
        self.currHistStepData = self._getHistStepData(self.currHistStep)

        return self.currHistStepData

    def GetStepsNum(self):
        """!Get number of steps saved in history"""
        return self.histStepsNum

    def GetCurrHistStep(self):
        """!Get current history step"""
        return self.currHistStep

    def Add(self, key, subkey, value):
        """!Add new data into buffer"""
        if key not in self.newHistStepData:
            self.newHistStepData[key] = {}

        if type(subkey) == types.ListType:
            if subkey[0] not in self.newHistStepData[key]:
                self.newHistStepData[key][subkey[0]] = {}
            self.newHistStepData[key][subkey[0]][subkey[1]] = value
        else:
            self.newHistStepData[key][subkey] = value

    def SaveHistStep(self):
        """!Create new history step with data in buffer"""
        self.maxHistSteps = UserSettings.Get(group ='vnet',
                                             key = 'other',
                                             subkey = 'max_hist_steps')
        self.currHistStep = 0 

        newHistFile = grass.tempfile()
        newHist = open(newHistFile, "w")

        self._saveNewHistStep(newHist)

        oldHist = open(self.histFile)
        removedHistData = self._savePreviousHist(newHist, oldHist)

        oldHist.close()
        newHist.close()
        grass.try_remove(self.histFile)
        self.histFile = newHistFile

        self.newHistStepData.clear() 

        return removedHistData

    def _savePreviousHist(self, newHist, oldHist):          
        """!Save previous history into new file"""
        newHistStep = False
        removedHistData = {}
        newHistStepsNum = self.histStepsNum

        for line in oldHist.readlines():
            if not line.strip():
                newHistStep = True
                newHistStepsNum += 1
                continue

            if newHistStep:
                newHistStep = False

                line = line.split("=")
                line[1] = str(newHistStepsNum)
                line = "=".join(line)

                if newHistStepsNum >= self.maxHistSteps:
                    removedHistStep = removedHistData[line] = {}
                    continue
                else:
                    newHist.write('%s%s%s' % (os.linesep, line, os.linesep))
                    self.histStepsNum = newHistStepsNum
            else:
                if newHistStepsNum >= self.maxHistSteps:
                    self._parseLine(line, removedHistStep)
                else:
                    newHist.write('%s' % line)                

        return removedHistData
            
    def _saveNewHistStep(self, newHist):
        """!Save buffer (new step) data into file"""
        newHist.write('%s%s%s' % (os.linesep, "history step=0", os.linesep))  
        for key in self.newHistStepData.keys():
            subkeys =  self.newHistStepData[key].keys()
            newHist.write('%s%s' % (key, self.sep))
            for idx in range(len(subkeys)):
                value =  self.newHistStepData[key][subkeys[idx]]
                if type(value) == types.DictType:
                    if idx > 0:
                        newHist.write('%s%s%s' % (os.linesep, key, self.sep))
                    newHist.write('%s%s' % (subkeys[idx], self.sep))
                    kvalues =  self.newHistStepData[key][subkeys[idx]].keys()
                    srange = range(len(kvalues))
                    for sidx in srange:
                        svalue = self._parseValue(self.newHistStepData[key][subkeys[idx]][kvalues[sidx]])
                        newHist.write('%s%s%s' % (kvalues[sidx], self.sep, svalue))
                        if sidx < len(kvalues) - 1:
                            newHist.write('%s' % self.sep)
                else:
                    if idx > 0 and \
                            type( self.newHistStepData[key][subkeys[idx - 1]]) == types.DictType:
                        newHist.write('%s%s%s' % (os.linesep, key, self.sep))
                    value = self._parseValue(self.newHistStepData[key][subkeys[idx]])
                    newHist.write('%s%s%s' % (subkeys[idx], self.sep, value))
                    if idx < len(subkeys) - 1 and \
                            type(self.newHistStepData[key][subkeys[idx + 1]]) != types.DictType:
                        newHist.write('%s' % self.sep)
            newHist.write(os.linesep)
        self.histStepsNum = 0

    def _parseValue(self, value, read = False):
        """!Parse value"""
        if read: # -> read data (cast values)

            if value:
                if value[0] == '[' and value[-1] == ']':# TODO, possible wrong interpretation
                    value = value[1:-1].split(',')
                    value = map(self._castValue, value)
                    return value

            if value == 'True':
                value = True
            elif value == 'False':
                value = False
            elif value == 'None':
                value = None
            elif ':' in value: # -> color
                try:
                    value = tuple(map(int, value.split(':')))
                except ValueError: # -> string
                    pass
            else:
                try:
                    value = int(value)
                except ValueError:
                    try:
                        value = float(value)
                    except ValueError:
                        pass
        else: # -> write data
            if type(value) == type(()): # -> color
                value = str(value[0]) + ':' +\
                    str(value[1]) + ':' + \
                    str(value[2])
                
        return value

    def _castValue(self, value):
        """!Cast value"""
        try:
            value = int(value)
        except ValueError:
            try:
                value = float(value)
            except ValueError:
                value = value[1:-1]

        return value

    def _getHistStepData(self, histStep):          
        """!Load data saved in history step"""        
        hist = open(self.histFile)
        histStepData = {}

        newHistStep = False
        isSearchedHistStep = False
        for line in hist.readlines():

            if  not line.strip() and isSearchedHistStep:
                 break
            elif not line.strip():
                newHistStep = True
                continue
            elif isSearchedHistStep:
                self._parseLine(line, histStepData)

            if newHistStep:
                line = line.split("=")
                if int(line[1]) == histStep:
                    isSearchedHistStep = True
                newHistStep = False

        hist.close()
        return histStepData

    def _parseLine(self, line, histStepData):
        """!Parse line in file with history"""        
        line = line.rstrip('%s' % os.linesep).split(self.sep)
        key = line[0]
        kv = line[1:]
        idx = 0
        subkeyMaster = None
        if len(kv) % 2 != 0: # multiple (e.g. nviz)
            subkeyMaster = kv[0]
            del kv[0]
        idx = 0
        while idx < len(kv):
            if subkeyMaster:
                subkey = [subkeyMaster, kv[idx]]
            else:
                subkey = kv[idx]
            value = kv[idx+1]
            value = self._parseValue(value, read = True)
            if key not in histStepData:
                histStepData[key] = {}

            if type(subkey) == types.ListType:
                if subkey[0] not in histStepData[key]:
                    histStepData[key][subkey[0]] = {}
                histStepData[key][subkey[0]][subkey[1]] = value
            else:
                histStepData[key][subkey] = value
            idx += 2

    def DeleteNewHistStepData(self):
        """!Delete buffer data for new history step"""        
        self.newHistStepData.clear() 

class VnetStatusbar(wx.StatusBar):
    """!Extends wx.StatusBar class with functionality to show multiple messages with the highest priority"""        
    def __init__(self, parent, style, id = wx.ID_ANY, **kwargs):

        wx.StatusBar.__init__(self, parent, id, style, **kwargs)

        self.maxPriority = 0
        self.statusItems = []

    def AddStatusItem(self, text, key, priority):
        """!Add new item to show

            @param text - string to show
            @param key - item identifier, if already contains 
                         item with same identifier, overwrites this item
            @param priority - only items with highest priority are showed 
        """        
        statusTextItem = {
                            'text' : text, 
                            'key' : key,
                            'priority' : priority
                         }

        for item in self.statusItems:
            if item['key'] == statusTextItem['key']:
                self.statusItems.remove(item)
        self.statusItems.append(statusTextItem)
        if self.maxPriority < statusTextItem['priority']:
            self.maxPriority =  statusTextItem['priority']
        self._updateStatus()

    def _updateStatus(self):

        currStatusText = ''
        for item in reversed(self.statusItems):
            if item['priority'] == self.maxPriority:
                if currStatusText:
                    currStatusText += '; '
                currStatusText += item['text']
        wx.StatusBar.SetStatusText(self, currStatusText)

    def RemoveStatusItem(self, key):
        """!Remove item 

            @param key - item identifier
        """
        update = False
        for iItem, item in enumerate(self.statusItems):
            if item['key'] == key:
                if item['priority'] == self.maxPriority:
                    update = True
                self.statusItems.pop(iItem)
        if update:
            for item in self.statusItems:
                self.maxPriority = 0
                if self.maxPriority < item['priority']:
                    self.maxPriority =  item['priority']
            self._updateStatus()
