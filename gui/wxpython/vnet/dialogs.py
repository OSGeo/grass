"""!
@package vnet.dialogs

@brief Dialogs for vector network analysis front-end

Classes:
 - dialogs::VNETDialog
 - dialogs::PtsList
 - dialogs::SettingsDialog
 - dialogs::CreateTtbDialog
 - dialogs::OutputVectorDialog
 - dialogs::VnetStatusbar
 - dialogs::DefIntesectionTurnCostDialog
 - dialogs::DefGlobalTurnsDialog
 - dialogs::TurnAnglesList

(C) 2012-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
@author Lukas Bocan <silent_bob centrum.cz> (turn costs support)
@author Eliska Kyzlikova <eliska.kyzlikova gmail.com> (turn costs support)
"""

import os
import sys
import types

from copy import copy
from grass.script     import core as grass

import wx
import wx.aui
import wx.lib.flatnotebook  as FN
import wx.lib.colourselect as csel
import wx.lib.mixins.listctrl  as  listmix
import wx.lib.scrolledpanel    as scrolled

from core             import globalvar, utils
from core.gcmd        import RunCommand, GMessage
from core.settings    import UserSettings

from dbmgr.base       import DbMgrBase 
from dbmgr.vinfo      import VectorDBInfo

from gui_core.widgets import GNotebook
from gui_core.goutput import GConsoleWindow
from gui_core.gselect import Select, LayerSelect, ColumnSelect

from vnet.widgets     import PointsList
from vnet.toolbars    import MainToolbar, PointListToolbar, AnalysisToolbar
from vnet.vnet_core   import VNETManager
from vnet.vnet_utils  import DegreesToRadians, RadiansToDegrees, GetNearestNodeCat, ParseMapStr

#Main TODOs
# - when layer tree of is changed, tmp result map is removed from render list 
# - optimization of map drawing 
# - tmp maps - add number of process
# - destructor problem - when GRASS GIS is closed with open VNETDialog,
#   it's destructor is not called

class VNETDialog(wx.Dialog):
    def __init__(self, parent, giface,
                 id = wx.ID_ANY, title = _("GRASS GIS Vector Network Analysis Tool"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        """!Dialog for vector network analysis"""

        wx.Dialog.__init__(self, parent, id, style=style, title = title, **kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))

        self.parent  = parent
        self.mapWin = giface.GetMapWindow()
        self.giface = giface

        # contains current analysis result (do not have to be last one, when history is browsed), 
        # it is instance of VectMap class
        self.tmp_result = None 

        self.defIsecTurnsHndlrReg = False
        
        # get attribute table columns only with numbers (for cost columns in vnet analysis)
        self.columnTypes = ['integer', 'double precision'] 
        
        self.vnet_mgr = VNETManager(self, giface)

        self.vnet_mgr.analysisDone.connect(self.AnalysisDone)
        self.vnet_mgr.ttbCreated.connect(self.TtbCreated)
        self.vnet_mgr.snapping.connect(self.Snapping)
        self.vnet_mgr.pointsChanged.connect(self.PointsChanged)

        self.currAnModule, valid = self.vnet_mgr.GetParam("analysis")

        # toobars
        self.toolbars = {}
        self.toolbars['mainToolbar'] = MainToolbar(parent = self, vnet_mgr = self.vnet_mgr)
        self.toolbars['analysisToolbar'] = AnalysisToolbar(parent = self, vnet_mgr = self.vnet_mgr)
        #
        # Fancy gui
        #
        self._mgr = wx.aui.AuiManager(self)

        self.mainPanel = wx.Panel(parent=self)
        self.notebook = GNotebook(parent = self.mainPanel,
                                  style = FN.FNB_FANCY_TABS | FN.FNB_BOTTOM |
                                          FN.FNB_NO_X_BUTTON)

        # statusbar
        self.stPriorities = {'important' : 5, 'iformation' : 1}
        self.stBar = VnetStatusbar(parent = self.mainPanel, style = 0)
        self.stBar.SetFieldsCount(number = 1)
    
        self.def_isec_turns = None

        # Create tabs
        
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

        self.SetSize((370, 550))
        self.SetMinSize((370, 420))

        #fix goutput's pane size (required for Mac OSX)
        if self.gwindow:         
            self.gwindow.SetSashPosition(int(self.GetSize()[1] * .75))

        self.OnAnalysisChanged(None)
        self.notebook.SetSelectionByName("parameters")
        self.toolbars['analysisToolbar'].SetMinSize((-1, self.toolbars['mainToolbar'].GetSize()[1]))

        self.toolbars['mainToolbar'].UpdateUndoRedo(0, 0)


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
                              Name("analysisTools").Caption(_("Analysis toolbar")).
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

        self.list = PtsList(parent = pointsPanel, vnet_mgr = self.vnet_mgr)
        self.toolbars['pointsList'] = PointListToolbar(parent = pointsPanel, 
                                                       dialog = self, 
                                                       vnet_mgr = self.vnet_mgr)

        anSettingsPanel = wx.Panel(parent = pointsPanel)

        anSettingsBox = wx.StaticBox(parent = anSettingsPanel, id = wx.ID_ANY,
                                label =" %s " % _("Analysis settings:"))

        maxDistPanel =  wx.Panel(parent = anSettingsPanel)
        maxDistLabel = wx.StaticText(parent = maxDistPanel, id = wx.ID_ANY, label = _("Maximum distance of point to the network:"))
        self.anSettings["max_dist"] = wx.SpinCtrl(parent = maxDistPanel, id = wx.ID_ANY, min = 0, max = maxValue)
        self.anSettings["max_dist"].Bind(wx.EVT_SPINCTRL, lambda event : self.MaxDist())
        self.anSettings["max_dist"].SetValue(100000) #TODO init val
        self.MaxDist()

        #showCutPanel =  wx.Panel(parent = anSettingsPanel)
        #self.anSettings["show_cut"] = wx.CheckBox(parent = showCutPanel, id=wx.ID_ANY,
        #                                          label = _("Show minimal cut"))
        #self.anSettings["show_cut"].Bind(wx.EVT_CHECKBOX, self.OnShowCut)

        isoLinesPanel =  wx.Panel(parent = anSettingsPanel)
        isoLineslabel = wx.StaticText(parent = isoLinesPanel, id = wx.ID_ANY, label = _("Iso lines:"))
        self.anSettings["iso_lines"] = wx.TextCtrl(parent = isoLinesPanel, id = wx.ID_ANY) 
        self.anSettings["iso_lines"].Bind(wx.EVT_TEXT, lambda event : self.IsoLines())
        self.anSettings["iso_lines"].SetValue("1000,2000,3000")
        self.IsoLines()

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

    def MaxDist(self):
        val = self.anSettings["max_dist"].GetValue()
        self.vnet_mgr.SetParams({"max_dist" : val}, {})

    def IsoLines(self):
        val = self.anSettings["iso_lines"].GetValue()
        self.vnet_mgr.SetParams({"iso_lines" : val}, {})

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

        self.giface.updateMap.emit(render=True, renderVector=True)

    def _createOutputPage(self):
        """!Tab with output console"""
        outputPanel = wx.Panel(parent = self)
        self.notebook.AddPage(page = outputPanel, 
                              text = _("Output"), 
                              name = 'output')

        goutput = self.vnet_mgr.goutput #TODO make interface
        self.gwindow = GConsoleWindow(parent = outputPanel, gconsole = goutput)

        #Layout
        outputSizer = wx.BoxSizer(wx.VERTICAL)
        outputSizer.Add(item = self.gwindow, proportion = 1, flag = wx.EXPAND)
        self.gwindow.SetMinSize((-1,-1))

        outputPanel.SetSizer(outputSizer)

    def _createParametersPage(self):
        """!Tab for selection of data for analysis"""
        dataPanel = scrolled.ScrolledPanel(parent=self)
        self.notebook.AddPage(page = dataPanel,
                              text=_('Parameters'), 
                              name = 'parameters')
        label = {}
        dataSelects = [
                        ['input', "Choose vector map for analysis:", Select],
                        ['alayer', "Arc layer number or name:", LayerSelect],
                        ['nlayer', "Node layer number or name:", LayerSelect],
                        #['tlayer', "Layer with turntable:", LayerSelect],
                        #['tuclayer', "Layer with unique categories for turntable:", LayerSelect],
                        ['afcolumn', "", ColumnSelect],
                        ['abcolumn', "", ColumnSelect],
                        ['ncolumn',  "", ColumnSelect],
                      ]

        #self.useTurns = wx.CheckBox(parent = dataPanel, id=wx.ID_ANY,
        #                            label = _('Use turns'))
        
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
                #if dataSel[0] == "tlayer":
                #    self.createTtbBtn = wx.Button(parent = selPanels[dataSel[0]], 
                #                                 label = _("Create turntable")) 
                #    self.createTtbBtn.Bind(wx.EVT_BUTTON, self.OnCreateTtbBtn)

                self.inputData[dataSel[0]] = dataSel[2](parent = selPanels[dataSel[0]],  
                                                        size = (-1, -1))
            label[dataSel[0]] =  wx.StaticText(parent =  selPanels[dataSel[0]], 
                                               name = dataSel[0])
            label[dataSel[0]].SetLabel(dataSel[1])

        self.inputData['input'].Bind(wx.EVT_TEXT, self.OnVectSel) 
        self.inputData['alayer'].Bind(wx.EVT_TEXT, self.OnALayerSel)
        self.inputData['nlayer'].Bind(wx.EVT_TEXT, self.OnNLayerSel)

        for params in ["afcolumn", "abcolumn", "ncolumn"]:#, "tlayer", "tuclayer"]:
            self.inputData[params].Bind(wx.EVT_TEXT, lambda event : self._setInputData())

        #self.useTurns.Bind(wx.EVT_CHECKBOX,
        #                    lambda event: self.UseTurns())
        #self.UseTurns()

        # Layout
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox(dataPanel, -1, "Vector map and layers for analysis")
        bsizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        mainSizer.Add(item = bsizer, proportion = 0,
                      flag = wx.EXPAND  | wx.TOP | wx.LEFT | wx.RIGHT, border = 5) 

        for sel in ['input', 'alayer', 'nlayer']:#, 'tlayer', 'tuclayer']:
            if sel == 'input':
                btn = self.addToTreeBtn
            #elif sel == "tlayer":
            #    btn = self.createTtbBtn
            else:
                btn = None
            #if sel == 'tlayer':
            #    bsizer.Add(item = self.useTurns, proportion = 0,
            #                flag = wx.TOP | wx.LEFT | wx.RIGHT, border = 5)                       

            selPanels[sel].SetSizer(self._doSelLayout(title = label[sel], 
                                                      sel = self.inputData[sel], 
                                                      btn = btn))
            bsizer.Add(item = selPanels[sel], proportion = 0,
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
        dataPanel.SetupScrolling()

        dataPanel.SetAutoLayout(1)

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
        haveDbMgr = self.vnet_mgr.GetAnalysisProperties(analysis)["resultProps"]["dbMgr"]

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
        vectMapName, mapSet = ParseMapStr(vectorMap)
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

            self.giface.updateMap.emit(render=True, renderVector=True)

    def UseTurns(self):
        if self.useTurns.IsChecked():
            self.inputData["tlayer"].GetParent().Show()
            self.inputData["tuclayer"].GetParent().Show()

            self.vnet_mgr.SetParams(params = {}, flags = {"t" : True})
        else:
            self.inputData["tlayer"].GetParent().Hide()
            self.inputData["tuclayer"].GetParent().Hide()

            self.vnet_mgr.SetParams(params = {}, flags = {"t" : False})

        self.Layout()

    def PointsChanged(self, method, kwargs):

        if method == "EditMode" and not kwargs["activated"]:
            ptListToolbar = self.toolbars['pointsList']
            if ptListToolbar:
                ptListToolbar.ToggleTool(id = ptListToolbar.GetToolId("insertPoint"),
                                         toggle = False)

        if method == "EditMode" and kwargs["activated"]:
            ptListToolbar = self.toolbars['pointsList']
            if ptListToolbar:
                ptListToolbar.ToggleTool(id = ptListToolbar.GetToolId("insertPoint"),
                                         toggle = True)

        if method == "SetPointData" and ("e" in kwargs.keys() or "n" in kwargs.keys()):
          self.notebook.SetSelectionByName("points")


    def OnCreateTtbBtn(self, event):

        params, err_params, flags = self.vnet_mgr.GetParams()
        dlg = CreateTtbDialog(parent = self, init_data = params)

        if dlg.ShowModal() == wx.ID_OK:
            self.stBar.AddStatusItem(text = _('Creating turntable...'),
                                     key = 'ttb',
                                     priority =  self.stPriorities['important'])

            params = dlg.GetData()
            if not self.vnet_mgr.CreateTttb(params):
                self.stBar.RemoveStatusItem('ttb')    
        dlg.Destroy()              

    def TtbCreated(self):

        params, err_params, flags = self.vnet_mgr.GetParams()
        self._updateParamsTab(params, flags)
        
        self.stBar.RemoveStatusItem('ttb')  

    def OnVectSel(self, event):
        """!When vector map is selected it populates other comboboxes in Parameters tab (layer selects, columns selects)"""
        if self.vnet_mgr.IsSnappingActive(): #TODO should be in vnet_mgr
            self.vnet_mgr.Snapping(activate = True)

        vectMapName, mapSet = self._parseMapStr(self.inputData['input'].GetValue())
        vectorMap = vectMapName + '@' + mapSet

        for sel in ['alayer', 'nlayer']:#, 'tlayer', 'tuclayer']:
            self.inputData[sel].Clear()
            self.inputData[sel].InsertLayers(vector = vectorMap)

        items = self.inputData['alayer'].GetItems()
        itemsLen = len(items)
        if itemsLen < 1:
            self.addToTreeBtn.Disable()
            if hasattr(self, 'inpDbMgrData'):
                self._updateInputDbMgrPage(show = False)
            self.inputData['alayer'].SetValue("")
            self.inputData['nlayer'].SetValue("")
            for sel in ['afcolumn', 'abcolumn', 'ncolumn']:
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

        self._setInputData()

    def _updateParamsTab(self, params, flags):
        #TODO flag

                #'tlayer', 'tuclayer', 
        for k in ['input', 'alayer', 'nlayer', 'afcolumn', 'abcolumn', 'ncolumn']:
            self.inputData[k].SetValue(params[k])

    def OnALayerSel(self, event):
        """!When arc layer from vector map is selected, populates corespondent columns selects"""
        self.inputData['afcolumn'].InsertColumns(vector = self.inputData['input'].GetValue(), 
                                                 layer = self.inputData['alayer'].GetValue(), 
                                                 type = self.columnTypes)
        self.inputData['abcolumn'].InsertColumns(vector = self.inputData['input'].GetValue(), 
                                                 layer = self.inputData['alayer'].GetValue(), 
                                                 type = self.columnTypes)

        self._setInputData()

    def OnNLayerSel(self, event):
        """!When node layer from vector map is selected, populates corespondent column select"""
        if self.vnet_mgr.IsSnappingActive():
            self.vnet_mgr.Snapping(activate = True)

        self.inputData['ncolumn'].InsertColumns(vector = self.inputData['input'].GetValue(), 
                                                layer = self.inputData['nlayer'].GetValue(), 
                                                type = self.columnTypes)
 
        self._setInputData()

    def _setInputData(self):
        params = {}
        for k, v in self.inputData.iteritems():
            params[k] = v.GetValue()
        flags = {}
        self.vnet_mgr.SetParams(params, flags)

    def _parseMapStr(self, vectMapStr):
        """!Create full map name (add current mapset if it is not present in name)"""
        mapValSpl = vectMapStr.strip().split("@")
        if len(mapValSpl) > 1:
            mapSet = mapValSpl[1]
        else:
            mapSet = grass.gisenv()['MAPSET']
        mapName = mapValSpl[0] 
        
        return mapName, mapSet      

    def OnCloseDialog(self, event):
        """!Cancel dialog"""
        self.vnet_mgr.CleanUp()
        self.parent.dialogs['vnet'] = None
        self.Destroy()

    def OnDefIsecTurnCosts(self, event):
        """!Registers/unregisters mouse handler into map window"""
        if self.defIsecTurnsHndlrReg == False:
            self.mapWin.RegisterMouseEventHandler(wx.EVT_LEFT_DOWN, 
                                                  self.OnDefIsecTurnCost,
                                                  wx.StockCursor(wx.CURSOR_CROSS))
            self.defIsecTurnsHndlrReg = True
        else:
            self.mapWin.UnregisterMouseEventHandler(wx.EVT_LEFT_DOWN, 
                                                    self.OnDefIsecTurnCost)

            self.defIsecTurnsHndlrReg = False

    def OnDefGlobalTurnCosts(self, event):

        dialog = DefGlobalTurnsDialog(self, data = self.vnet_mgr.GetGlobalTurnsData())
        dialog.Show()

    def OnDefIsecTurnCost(self, event): #TODO move to vnet mgr?
        """!Take coordinates from map window"""
        if event == 'unregistered':
            ptListToolbar = self.toolbars['pointsList']
            if ptListToolbar:
                ptListToolbar.ToggleTool( id = ptListToolbar.GetToolId("isec_turn_edit"),
                                          toggle = False)  
            self.handlerRegistered = False
            return

        e, n = self.mapWin.GetLastEN()

        # compute threshold
        snapTreshPix = int(UserSettings.Get(group ='vnet', 
                                            key = 'other', 
                                            subkey = 'snap_tresh'))
        res = max(self.mapWin.Map.region['nsres'], self.mapWin.Map.region['ewres'])
        snapTreshDist = snapTreshPix * res

        params, err_params, flags = self.vnet_mgr.GetParams()

        if "input" in err_params:
            GMessage(parent = self,
                     message = _("Input vector map does not exists."))

        if ["tlayer", "tuclayer"] in err_params:
            GMessage(parent = self, message = "Please choose existing turntable layer and unique categories layer in Parameters tab.")

        cat = GetNearestNodeCat(e, n, int(params['tuclayer']), snapTreshDist, params["input"])

        if not self.def_isec_turns:
            self.def_isec_turns = DefIntesectionTurnCostDialog(self, self.parent)
            self.def_isec_turns.SetSize((500, 400))

        self.def_isec_turns.SetData(params["input"], params["tlayer"])
        self.def_isec_turns.SetIntersection(cat)
        self.def_isec_turns.Show()

    def OnAnalyze(self, event):
        """!Called when network analysis is started"""

        self.stBar.AddStatusItem(text = _('Analysing...'),
                                 key = 'analyze',
                                 priority =  self.stPriorities['important'])

        ret = self.vnet_mgr.RunAnalysis()

        #TODO
        self.resultDbMgrData['analysis'] = self.currAnModule

        if ret < 0:
            self.stBar.RemoveStatusItem(key = 'analyze')
            if ret == -2:
                self.notebook.SetSelectionByName("parameters")

    def AnalysisDone(self):

        curr_step, steps_num = self.vnet_mgr.GetHistStep()
        self.toolbars['mainToolbar'].UpdateUndoRedo(curr_step, steps_num)

        self.tmp_result = self.vnet_mgr.GetResults() 

        mainToolbar = self.toolbars['mainToolbar']
        id = vars(mainToolbar)['showResult']
        mainToolbar.ToggleTool(id =id,
                               toggle = True)

        self.stBar.RemoveStatusItem(key = 'analyze')

        self._updateResultDbMgrPage()
        self._updateDbMgrData()

        self.giface.updateMap.emit(render=True, renderVector=True)

    def OnShowResult(self, event):
        """!Show/hide analysis result"""
        mainToolbar = self.toolbars['mainToolbar']
        id = vars(mainToolbar)['showResult']
        toggleState = mainToolbar.GetToolState(id)

        if not self.tmp_result:
            mainToolbar.ToggleTool(id =id,
                                   toggle = False)
        elif toggleState:
            self.vnet_mgr.ShowResult(True)
        else:
            self.vnet_mgr.ShowResult(False)

    def OnSaveTmpLayer(self, event):
        dlg = OutputVectorDialog(parent = self)
        dlg.ShowModal()
        self.vnet_mgr.SaveTmpLayer(dlg.vectSel.GetValue())
        dlg.Destroy()

    def OnSettings(self, event):
        """!Displays vnet settings dialog"""
        dlg = SettingsDialog(parent=self, id=wx.ID_ANY, 
                             title=_('Settings'), vnet_mgr = self.vnet_mgr)
        
        dlg.ShowModal()
        dlg.Destroy()

    def OnAnalysisChanged(self, event):
        """!Updates dialog when analysis is changed"""
        # set chosen analysis
        iAn = self.toolbars['analysisToolbar'].anChoice.GetSelection() 
        self.currAnModule = self.vnet_mgr.GetAnalyses()[iAn]
        self.vnet_mgr.SetParams({"analysis" : self.currAnModule}, {})

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

        an_props = self.vnet_mgr.GetAnalysisProperties()

        used_cols = []
        attrCols = an_props["cmdParams"]["cols"]


        for col in attrCols.iterkeys():

            if "inputField" in attrCols[col]:
                colInptF = attrCols[col]["inputField"]
            else:
                colInptF = col

            if col in skip:
                continue

            inputPanel = self.inputData[colInptF].GetParent()
            inputPanel.Show()
            inputPanel.FindWindowByName(colInptF).SetLabel(attrCols[col]["label"])

            if col != colInptF:
               skip.append(colInptF)
            used_cols.append(colInptF)

        for col in ["abcolumn", "afcolumn", "ncolumn"]:
            if col not in used_cols:
                inputPanel = self.inputData[colInptF].GetParent()
                inputPanel.Hide()

        self.Layout()

    def Snapping(self, evt):
        """!Start/stop snapping mode"""

        if evt == "deactivated":
            self.stBar.RemoveStatusItem(key = 'snap')
            ptListToolbar = self.toolbars['pointsList']
            ptListToolbar.ToggleTool(id = ptListToolbar.GetToolId("snapping"),
                                     toggle = False)

        elif evt == "computing_points":
            self.stBar.AddStatusItem(text = _('Computing nodes...'),
                                     key = 'snap',
                                     priority = self.stPriorities['important'])

        elif evt == "computing_points_done":
            self.stBar.RemoveStatusItem(key = 'snap')

    def SnapPointsDone(self):
        """!Update map window, when map with nodes to snap is created"""
        self.stBar.RemoveStatusItem(key = 'snap')

    def OnUndo(self, event):
        """!Step back in history"""

        curr_step, steps_num = self.vnet_mgr.Undo()
        self._updateDialog()
        self.toolbars['mainToolbar'].UpdateUndoRedo(curr_step, steps_num)

    def OnRedo(self, event):
        """!Step forward in history"""

        curr_step, steps_num = self.vnet_mgr.Redo()
        self._updateDialog()
        self.toolbars['mainToolbar'].UpdateUndoRedo(curr_step, steps_num)

    def _updateDialog(self):
        params, err_params, flags = self.vnet_mgr.GetParams()
        self._updateParamsTab(params, flags)

        anChoice = self.toolbars['analysisToolbar'].anChoice
        anChoice.SetSelection(self.vnet_mgr.GetAnalyses().index(params["analysis"]))
        self.currAnModule = params["analysis"] 
        self.resultDbMgrData['analysis'] = params["analysis"]

        # set analysis combobox
        anChoice = self.toolbars['analysisToolbar'].anChoice
        anChoice.SetSelection(self.vnet_mgr.GetAnalyses().index(params["analysis"]))

        self._updateResultDbMgrPage()
        self._updateDbMgrData()

        self.OnAnalysisChanged(None)

class PtsList(PointsList):
    def __init__(self, parent, vnet_mgr, id=wx.ID_ANY):
        """! List with points for analysis"""
        self.updateMap = True
        self.vnet_mgr = vnet_mgr
        self.pts_data = self.vnet_mgr.GetPointsManager()

        pts_data_cols = self.pts_data.GetColumns(only_relevant = False)

        cols = []
        for i_col, name in enumerate(pts_data_cols["name"]): 
            if i_col == 0:
                continue
            cols.append([name, pts_data_cols["label"][i_col], 
                              pts_data_cols["type"][i_col], pts_data_cols["def_vals"][i_col]])

        PointsList.__init__(self, parent = parent, cols = cols, id =  id)      

        self.vnet_mgr.pointsChanged.connect(self.PointsChanged)
        self.vnet_mgr.parametersChanged.connect(self.ParametersChanged)

        analysis, valid = self.vnet_mgr.GetParam("analysis")

        self.AnalysisChanged(analysis)

        for iPt in range(self.pts_data.GetPointsCount()):
            self.AddItem()
            pt_dt = self.pts_data.GetPointData(iPt)
            self.SetData(iPt, pt_dt)
        self.Select(self.pts_data.GetSelected())

    def AnalysisChanged(self, analysis):
        active_cols = self.pts_data.GetColumns()
        if 'type' in active_cols["name"]:
            if not self.IsShown('type'):
                self.ShowColumn('type', 1)

            type_idx = active_cols["name"].index("type")
            type_labels = active_cols["type"][type_idx]

            self.ChangeColEditable('type', type_labels)
            colNum = self._getColumnNum('type')

            for iItem, item in enumerate(self.itemDataMap): 
                self.EditCellKey(iItem, 'type', self.selIdxs[iItem][colNum])

                if not item[1]:               
                    self.CheckItem(iItem, False)

        else:
            if self.IsShown('type'):
                self.HideColumn('type')

    def ParametersChanged(self, method, kwargs):
            if "analysis" in kwargs["changed_params"].keys():
                self.AnalysisChanged(analysis = kwargs["changed_params"]["analysis"])

    def OnItemActivated(self, event):
        changed, key = PointsList.OnItemActivated(self, event)
        
        if not changed:
            return

        dt_dict = {}
        active_cols = self.pts_data.GetColumns()
        for col in active_cols["name"]:
            if col == "use":
                continue
            dt_dict[col] = self.GetCellValue(key, col)

        self.pts_data.SetPointData(key, dt_dict)

    def PointsChanged(self, method, kwargs):
        if method == "AddPoint":
            self.AddItem()

        elif method == "DeletePoint":
            self.DeleteItem()

        elif method == "SetPointData":
            self.SetData(kwargs["pt_id"], kwargs["data"])

        elif method == "SetPoints":
            while self.GetSelected() != wx.NOT_FOUND:
                self.DeleteItem()

            for iPt, pt_dt in enumerate(kwargs["pts_data"]):
                self.AddItem()
                self.SetData(iPt, pt_dt)

        elif method == "SetSelected":
            self.Select(self._findIndex(kwargs["pt_id"]))

    def SetData(self, key, data):

        idx = self._findIndex(key)
        for k, v in data.iteritems():
            if k == "use":
                
                if v and not self.IsChecked(idx):
                    self.CheckItem(idx, True)
                elif not v and self.IsChecked(idx):
                    self.CheckItem(idx, False) 
            else:
                found = 0
                for col in self.colsData:
                    if k == col[0]:
                      found = 1
                      break

                if found:
                    self.EditCellKey(key, k, v)    


    def OnItemSelected(self, event):
        """!Item selected"""
        PointsList.OnItemSelected(self, event)
        self.selectedkey = self.GetItemData(self.selected)

        if self.selectedkey == self.pts_data.GetSelected():
          return

        if self.selectedkey == wx.NOT_FOUND:
            self.pts_data.SetSelected(None)
        else:
            self.pts_data.SetSelected(self.selectedkey)

    def OnCheckItem(self, index, flag):
        "flag is True if the item was checked, False if unchecked"
        key =  self.GetItemData(index)
        if self.pts_data.GetPointData(key)["use"]!= flag:
            self.pts_data.SetPointData(key, {"use" : flag})

class SettingsDialog(wx.Dialog):
    def __init__(self, parent, id, title, vnet_mgr, pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE):
        """!Settings dialog"""
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)

        self.vnet_mgr = vnet_mgr

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

        self.vnet_mgr.SettingsUpdated()

    def OnApply(self, event):
        """!Button 'Apply' pressed"""
        self.UpdateSettings()
        #self.Close()

    def OnClose(self, event):
        """!Button 'Cancel' pressed"""
        self.Close()

class CreateTtbDialog(wx.Dialog):

    def __init__(self, parent, init_data, id=wx.ID_ANY,
                 title = _("New vector map with turntable"), style=wx.DEFAULT_DIALOG_STYLE):
        """!Create turntable dialog."""
        wx.Dialog.__init__(self, parent, id, title = _(title), style = style)

        label = {}
        dataSelects = [
                        ['input', "Choose vector map for analysis:", Select],
                        ['output', "Name of vector map with turntable:", Select],
                        ['alayer', "Arc layer which will be expanded by turntable:", LayerSelect],
                        ['tlayer', "Layer with turntable:", LayerSelect],
                        ['tuclayer', "Layer with unique categories for turntable:", LayerSelect],
                      ]

        self.inputData = {}

        selPanels = {}

        for dataSel in dataSelects:
            selPanels[dataSel[0]] = wx.Panel(parent = self)
            if dataSel[0] in  ['input', 'output']:
                self.inputData[dataSel[0]] = dataSel[2](parent = selPanels[dataSel[0]],
                                                        size = (-1, -1), 
                                                        type = 'vector')
            elif dataSel[0] != 'input':
                self.inputData[dataSel[0]] = dataSel[2](parent = selPanels[dataSel[0]],  
                                                        size = (-1, -1))

            label[dataSel[0]] =  wx.StaticText(parent =  selPanels[dataSel[0]], 
                                               name = dataSel[0])
            label[dataSel[0]].SetLabel(dataSel[1])

        self.inputData['input'].Bind(wx.EVT_TEXT, lambda event : self.InputSel) 

        # buttons
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnOk = wx.Button(self, wx.ID_OK)
        self.btnOk.SetDefault()

        # Layout
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        box = wx.StaticBox(self, -1, "Vector map and layers for analysis")
        bsizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        mainSizer.Add(item = bsizer, proportion = 0,
                      flag = wx.EXPAND  | wx.TOP | wx.LEFT | wx.RIGHT, border = 5) 

        btn = None
        for sel in ['input', 'output', 'alayer', 'tlayer', 'tuclayer']:                      

            selPanels[sel].SetSizer(self._doSelLayout(title = label[sel], 
                                                      sel = self.inputData[sel], 
                                                      btn = btn))
            bsizer.Add(item = selPanels[sel], proportion = 0,
                       flag = wx.EXPAND)
  
        for k, v in init_data.iteritems():
            if self.inputData.has_key(k):
                self.inputData[k].SetValue(v)

        inp_vect_map = self.inputData["input"].GetValue().split("@")[0]
        self.inputData['output'].SetValue(inp_vect_map + "_ttb")

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()

        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.ALIGN_RIGHT | wx.ALL, border = 5)


        self.SetSizer(mainSizer)
        self.Fit()

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

    def InputSel(self):
        """!When vector map is selected it populates other comboboxes in Parameters tab (layer selects, columns selects)"""
        vectMapName, mapSet = self._parseMapStr(self.inputData['input'].GetValue())
        vectorMap = vectMapName + '@' + mapSet

        for sel in ['alayer', 'tlayer', 'tuclayer']:
            self.inputData[sel].Clear()
            self.inputData[sel].InsertLayers(vector = vectorMap)

        items = self.inputData['alayer'].GetItems()
        itemsLen = len(items)
        if itemsLen < 1:
            self.addToTreeBtn.Disable()
            if hasattr(self, 'inpDbMgrData'):
                self._updateInputDbMgrPage(show = False)
            self.inputData['alayer'].SetValue("")
            return
        elif itemsLen == 1:
            self.inputData['alayer'].SetSelection(0)
        elif itemsLen >= 1:
            if unicode("1") in items:
                iItem = items.index(unicode("1")) 
                self.inputData['alayer'].SetSelection(iItem)
        self.addToTreeBtn.Enable()
        if hasattr(self, 'inpDbMgrData'):
            self._updateInputDbMgrPage(show = True)

    def GetData(self):

        params = {}
        for param, sel in self.inputData.iteritems():
            params[param] = sel.GetValue()

        return params

class OutputVectorDialog(wx.Dialog):
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

class DefIntesectionTurnCostDialog(wx.Dialog):

    def __init__(self, parent, mapWin, style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, id = wx.ID_ANY, title = _("Edit intersection turns costs"), **kwargs):
        wx.Dialog.__init__(self, parent, id, style=style, title = title, **kwargs)

        self.dbMgr = DbMgrBase(mapdisplay = mapWin)
        self.browsePage = self.dbMgr.CreateDbMgrPage(parent = self, pageName = 'browse')

    def layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)

        sizer.Add(item = self.browsePage, proportion = 1,
                  flag = wx.EXPAND)
        
        self.SetSizer(sizer)

    def SetData(self, vectMapName, layer):
        if vectMapName != self.dbMgr.GetVectorName():
            self.dbMgr.ChangeVectorMap(vectorName = vectMapName)
        else:
            self.browsePage.DeleteAllPages() 
        
        self.browsePage.AddLayer(int(layer))
        self.layer = int(layer)

        # TODO HACK!!!
        self.browsePage.FindWindowById(self.browsePage.layerPage[int(layer)]['sqlNtb']).GetParent().Hide()

    def SetIntersection(self, isec):

        self.browsePage.LoadData(self.layer, where = "isec = %d" % (isec))

class DefGlobalTurnsDialog(wx.Dialog):
    def __init__(self, parent, data, style= wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, 
                 id= wx.ID_ANY, title= _("Define Global Turn Costs"), **kwargs): # v Gassu dopln preklad

        wx.Dialog.__init__(self, parent, id, title, style = style, **kwargs)

        self.data = data

        self.angle_list = TurnAnglesList(parent= self, data= self.data) 

        self.btnAdd = wx.Button(parent=self, id=wx.ID_ANY, label = "Add" )
        self.btnRemove = wx.Button(parent=self, id=wx.ID_ANY, label = "Remove" )
        self.btnClose = wx.Button(parent = self, id = wx.ID_CLOSE)
        self.useUTurns = wx.CheckBox(parent = self, id = wx.ID_ANY, label = "Use U-turns")

        self.btnAdd.Bind(wx.EVT_BUTTON, self.OnAddButtonClick)
        self.btnRemove.Bind(wx.EVT_BUTTON, self.OnRemoveButtonClick)
        self.Bind(wx.EVT_CLOSE, self.OnCloseDialog)
        self.useUTurns.Bind(wx.EVT_CHECKBOX, self.OnCheckedUTurns)
                
        self.btnClose.SetDefault()
        self.useUTurns.SetValue(True)
        self.OnCheckedUTurns(None)
        self.layout()
        self.SetInitialSize((500, 200))



    def layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        labelSizer = wx.BoxSizer(wx.HORIZONTAL)
        addRemoveSizer = wx.BoxSizer(wx.VERTICAL)
        closeSizer = wx.BoxSizer(wx.HORIZONTAL)  

        addRemoveSizer.Add(item= self.btnAdd, proportion= 0, flag= wx.ALIGN_RIGHT, border = 10)
        addRemoveSizer.Add(item= self.btnRemove, proportion= 0, flag= wx.ALIGN_RIGHT, border = 10)

        labelSizer.Add(item= self.angle_list, proportion= 1, flag= wx.EXPAND, border = 10)
        labelSizer.Add(item=addRemoveSizer, proportion = 0, flag = wx.ALIGN_RIGHT, border = 10)       

        closeSizer.Add(item=self.useUTurns, proportion = 1, flag = wx.ALIGN_LEFT, border = 10 )
        closeSizer.Add(item=self.btnClose, proportion = 0, flag = wx.ALIGN_RIGHT, border = 10)        

        sizer.Add(item=labelSizer, proportion = 1, flag = wx.EXPAND)
        sizer.Add(item=closeSizer, proportion = 0, flag = wx.EXPAND)

        self.SetSizer(sizer)
     

    def OnAddButtonClick(self, event):
        """!Add new direction over selected row"""
        selected_indices = self.angle_list.GetSelectedIndices()
 
        if not selected_indices:
            from_value = self.turn_data.GetValue(self.turn_data.GetLinesCount()-1,2)
            to_value = self.turn_data.GetValue(0,1)
            default_row = ["new", from_value, to_value, 0.0]
            self.angle_list.AppendRow(default_row)

        # If no row is selected, new direction is added to the end of table
        i_addition = 0
        for i in selected_indices:
            i += i_addition
            from_value = self.turn_data.GetValue(i-1,2)
            to_value = self.turn_data.GetValue(i,1)
            default_row = ["new", from_value, to_value, 0.0]
            self.angle_list.InsertRow(i,default_row)
            i_addition += 1


    def OnRemoveButtonClick(self, event):
        """!Delete one or more selected directions"""
        selected_indices = self.angle_list.GetSelectedIndices()
  
        i_reduction = 0
        for i in selected_indices:
            i -= i_reduction
            self.angle_list.DeleteRow(i)
            i_reduction += 1

    def OnCloseDialog(self, event):
        """!Close dialog"""
        self.Close()

    def OnCheckedUTurns(self, event):
        """!Use U-turns in analyse"""
        self.data.SetUTurns(self.useUTurns.GetValue())
       
    def SetData(self, data):
        self.angle_list.SetData(data)
        self.data = data



class TurnAnglesList(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.TextEditMixin):
    """!Virtual editable table with global turns"""
    def __init__(self, parent, data, id= wx.ID_ANY, style= wx.LC_REPORT | wx.LC_VIRTUAL, **kwargs):
        wx.ListCtrl.__init__(self, parent, id,style= style, **kwargs)
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.TextEditMixin.__init__(self)
        
        self.Populate()
        self.data = data
        self.SetItemCount(self.data.GetLinesCount())
        

    def Populate(self):
        """!Columns definition"""
        self.InsertColumn(col= 0, heading= "Direction", format= wx.LIST_FORMAT_LEFT) # v Gassu dopln preklad 
        self.InsertColumn(col= 1, heading= "From Angle", format= wx.LIST_FORMAT_RIGHT)
        self.InsertColumn(col= 2, heading= "To Angle", format= wx.LIST_FORMAT_RIGHT)
        self.InsertColumn(col= 3, heading= "Cost", format= wx.LIST_FORMAT_RIGHT)

    def OnGetItemText(self, item, col):
        val = self.data.GetValue(item, col)
        if col in [1,2]:
            val = RadiansToDegrees(val)            
        return str(val)

    def SetVirtualData(self, row, column, text):
        """!Set data to table"""
        if column in [1,2,3]:
            try:
                text = float(text)
            except:
                return
        if column in [1,2]:
            text = DegreesToRadians(text)
            
            # Tested allowed range of values
            if text > math.pi:
                text = 0.0
            elif text < -math.pi:
                text = 0.0

        self.data.SetValue(text, row, column)

        self.edited_row = row
        self.RefreshItems(0,self.data.GetLinesCount()-1)


    def AppendRow(self, values):
        self.data.AppendRow(values)
        self.SetItemCount(self.data.GetLinesCount())

    def InsertRow(self, line, values):
        self.data.InsertRow(line,values)
        self.SetItemCount(self.data.GetLinesCount())

    def DeleteRow(self, row):
        self.data.PopRow(row)
        self.SetItemCount(self.data.GetLinesCount())

    def SetData(self, data):
        self.data = data
        self.SetItemCount(self.data.GetLinesCount())
        self.RefreshItems(0, self.data.GetLinesCount()-1)


    def GetSelectedIndices(self, state =  wx.LIST_STATE_SELECTED):
        """!Get numbers of selected rows"""
        indices = []
        lastFound = -1
        while True:
            index = self.GetNextItem(lastFound, wx.LIST_NEXT_ALL, state)
            if index == -1:
                break
            else:
                lastFound = index
                indices.append(index)
        return indices