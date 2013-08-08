"""!
@package vnet.vnet_core

@brief Vector network analysis logic.

Classes:
 - vnet_core::VNETManager
 - vnet_core::VNETAnalyses
 - vnet_core::VNETHistory
 - vnet_core::SnappingNodes

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
@author Lukas Bocan <silent_bob centrum.cz> (turn costs support)
@author Eliska Kyzlikova <eliska.kyzlikova gmail.com> (turn costs support)
"""

import os
from grass.script     import core as grass

import wx

from core             import utils
from core.gcmd        import RunCommand, GMessage
from core.gconsole    import CmdThread, EVT_CMD_DONE, GConsole
from core.utils import _

from gui_core.gselect import VectorDBInfo

from vnet.vnet_data   import VNETData, VNETTmpVectMaps, VectMap, History
from vnet.vnet_utils  import ParseMapStr, haveCtypes, GetNearestNodeCat

from grass.pydispatch.signal import Signal
 
class VNETManager:
    def __init__(self, guiparent, giface):

        self.data = {}

        self.guiparent = guiparent
        self.giface = giface
        self.mapWin = giface.GetMapWindow()

        self.goutput = GConsole(guiparent = guiparent) 

        self.vnet_data = VNETData(guiparent = guiparent, mapWin = self.mapWin)

        self.results = {"analysis" : None,
                        "vect_map" : None} #TODO more results

        # this class instance manages all temporary vector maps created during life of VNETDialog  
        self.tmp_maps = VNETTmpVectMaps(parent = guiparent, mapWin = self.mapWin)

        # initialization of History class used for saving and reading data from file
        # it is used for browsing analysis results
        self.history = VNETHistory(self.guiparent, self.vnet_data, self.tmp_maps)
        self.analyses = VNETAnalyses(self.vnet_data, self.RunAnDone, self.goutput, self.tmp_maps)

        self.snap_nodes = SnappingNodes(self.giface, self.vnet_data, self.tmp_maps, self.mapWin)

        self.ttbCreated = Signal('VNETManager.ttbCreated')
        self.analysisDone = Signal('VNETManager.analysisDone')
        self.pointsChanged = self.vnet_data.pointsChanged
        self.parametersChanged = self.vnet_data.parametersChanged

        self.snapping = self.snap_nodes.snapping
        self.pointsChanged.connect(self.PointsChanged)


    def  __del__(self):
        self.CleanUp()

    def CleanUp(self):
        """!Removes temp layers, unregisters handlers and graphics"""

        update = self.tmp_maps.DeleteAllTmpMaps()

        self.vnet_data.CleanUp()
        
        if update:
            self.giface.updateMap.emit(render=True, renderVector=True)
        else:
            self.giface.updateMap.emit(render=False, renderVector=False)

    def GetPointsManager(self):
        return self.vnet_data.GetPointsData()

    def GetGlobalTurnsData(self):
        return self.vnet_data.GetGlobalTurnsData()

    def RunAnalysis(self):
        
        analysis, valid = self.vnet_data.GetParam("analysis")

        params, err_params, flags = self.vnet_data.GetParams()
        relevant_params = self.vnet_data.GetRelevantParams(analysis)

        if not relevant_params:
            return -1

        if not self.vnet_data.InputsErrorMsgs(_("Unable to perform analysis."), 
                                              analysis, params, flags, err_params, relevant_params):
            return -2

        if self.results["vect_map"]:
            self.results["vect_map"].DeleteRenderLayer()

        # history - delete data in buffer for hist step  
        self.history.DeleteNewHistStepData()

        # create new map (included to history) for result of analysis
        self.results["vect_map"] = self.history.NewTmpVectMapToHist('vnet_tmp_result')

        if not self.results["vect_map"]:
                return False         

        # for case there is some map with same name 
        # (when analysis does not produce any map, this map would have been shown as result) 
        RunCommand('g.remove', 
                    vect = self.results["vect_map"].GetVectMapName())

        # save data from 
        self.history._saveAnInputToHist(analysis, params, flags)

        ret = self.analyses.RunAnalysis(self.results["vect_map"].GetVectMapName(), params, flags)
        if not ret:
            return -3
        else:
            return 1

    def RunAnDone(self, cmd, returncode, results):

        self.results["analysis"] = cmd[0]
       
        self.results["vect_map"].SaveVectMapState()

        cmd, cmd_colors =  self.vnet_data.GetLayerStyle()
        self.results["vect_map"].AddRenderLayer(cmd, cmd_colors)

        self.history.SaveHistStep()

        self.analysisDone.emit()

    def ShowResult(self, show):
        #TODO can be more results e. g. smallest cut

        if show:
            self._checkResultMapChanged(self.results["vect_map"])
            cmd, cmd_colors = self.vnet_data.GetLayerStyle()
            self.results["vect_map"].AddRenderLayer(cmd, cmd_colors)
        else:
            self.results["vect_map"].DeleteRenderLayer()

        self.giface.updateMap.emit(render=True, renderVector=True)

    def GetAnalysisProperties(self, analysis = None):
        return self.vnet_data.GetAnalysisProperties(analysis = analysis)

    def GetResults(self):
        return self.results["vect_map"]

    def Undo(self):
        self._updateDataForHistStep(self.history.Undo())
        #SetUpdateMap TODO
        return self.history.GetHistStep()
    def Redo(self):
        self._updateDataForHistStep(self.history.Redo())
        #SetUpdateMap
        return self.history.GetHistStep()

    def _updateDataForHistStep(self, data):
        if not data:
            return

        analysis, resultMapName, params, flags = data

        self.results["analysis"] = analysis
        self.vnet_data.SetParams(params, flags)

        self.results["vect_map"].DeleteRenderLayer()
        self.results["vect_map"]  = self.tmp_maps.GetTmpVectMap(resultMapName)
        self._checkResultMapChanged(self.results["vect_map"])

        cmd, cmd_colors = self.vnet_data.GetLayerStyle()
        self.results["vect_map"].AddRenderLayer(cmd, cmd_colors)

        self.giface.updateMap.emit(render=True, renderVector=True)

    def GetHistStep(self):
        return self.history.GetHistStep()

    def SetParams(self, params, flags):
        self.vnet_data.SetParams(params, flags)

    def GetParams(self):
        params, inv_params, flags = self.vnet_data.GetParams()
        return params, inv_params, flags

    def GetParam(self, param):
        return self.vnet_data.GetParam(param)

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

    def IsSnappingActive(self):
        return self.vnet_data.GetSnapping()

    def Snapping(self, activate):
        self.snap_nodes.ComputeNodes(activate)

    def GetAnalyses(self):
        return self.vnet_data.GetAnalyses()

    def SettingsUpdated(self):
        self.vnet_data.GetPointsData().SetPointDrawSettings()
        if not self.results["vect_map"]  or \
           not self.tmp_maps.HasTmpVectMap(self.results["vect_map"].GetVectMapName()):
            self.giface.updateMap.emit(render=False, renderVector=False)
        elif self.results["vect_map"].GetRenderLayer():
            cmd, cmd_colors = self.vnet_data.GetLayerStyle()
            self.results["vect_map"].AddRenderLayer(cmd, cmd_colors)
            
            self.giface.updateMap.emit(render=True, renderVector=True)
            #TODO optimization
        else:
            self.giface.updateMap.emit(render=False, renderVector=False)

    def PointsChanged(self, method, kwargs):
        self.giface.updateMap.emit(render=False, renderVector=False)

    def CreateTttb(self, params):

        outputMap = params["output"]
        mapName, mapSet = ParseMapStr(outputMap)
        if mapSet !=  grass.gisenv()['MAPSET']:
            GMessage(parent = self,
                     message = _("Map can be created only in current mapset"))
            return False
        existsMap = grass.find_file(name = mapName, 
                                    element = 'vector', 
                                    mapset = grass.gisenv()['MAPSET'])
        if existsMap["name"]:
            dlg = wx.MessageDialog(parent = self.guiparent,
                                   message = _("Vector map %s already exists. " +
                                                "Do you want to overwrite it?") % 
                                                (existsMap["fullname"]),
                                   caption = _("Overwrite vector map"),
                                   style = wx.YES_NO | wx.NO_DEFAULT |
                                               wx.ICON_QUESTION | wx.CENTRE) 
            ret = dlg.ShowModal()
            dlg.Destroy()
            if ret == wx.ID_NO:
                return False

            cmdTtb = [ 
                        "v.net.turntable",
                        "input=" + params["input"], 
                        "output=" + params["output"],
                        "alayer=" + params["alayer"],
                        "tlayer=" + params["tlayer"],
                        "tuclayer=" + params["tuclayer"],
                        "--overwrite", 
                       ]

            self.goutput.RunCmd(command = cmdTtb, onDone = self._createTtbDone)

        return True

    def _createTtbDone(self, cmd, returncode):

        if returncode != 0:
            GMessage(parent = self.guiparent,
                     message = _("Creation of turntable failed."))
            return
        else:
            params = {}
            for c in cmd:
                spl_c = c.split("=")
                if len(spl_c) != 2:
                    continue

                if spl_c[0] and spl_c != "input":
                    params[spl_c[0]] = spl_c[1]
                if spl_c[0] == "output":
                    params["input"] = spl_c[1]

            self.vnet_data.SetParams(params, {})

        self.ttbCreated.emit(returncode = returncode)

    def SaveTmpLayer(self, layer_name):
        """!Permanently saves temporary map of analysis result"""
        msg = _("Vector map with analysis result does not exist.")

        if not hasattr(self.results["vect_map"], "GetVectMapName"):
            GMessage(parent = self.guiparent,
                     message = msg)
            return

        mapToAdd = self.results["vect_map"].GetVectMapName()
        mapToAddEx = grass.find_file(name = mapToAdd, 
                                        element = 'vector', 
                                        mapset = grass.gisenv()['MAPSET'])

        if not mapToAddEx["name"]: 
            GMessage(parent = self.guiparent,
                    message = msg)
            return

        addedMap = layer_name
        mapName, mapSet = ParseMapStr(addedMap)
        if mapSet !=  grass.gisenv()['MAPSET']:
            GMessage(parent = self.guiparent,
                     message = _("Map can be saved only to currently set mapset"))
            return
        existsMap = grass.find_file(name = mapName, 
                                    element = 'vector', 
                                    mapset = grass.gisenv()['MAPSET'])
        if existsMap["name"]:
            dlg = wx.MessageDialog(parent = self.guiparent,
                                    message = _("Vector map %s already exists. " +
                                            "Do you want to overwrite it?") % 
                                            (existsMap["fullname"]),
                                   caption = _("Overwrite vector map"),
                                   style = wx.YES_NO | wx.NO_DEFAULT |
                                           wx.ICON_QUESTION | wx.CENTRE)            
            ret = dlg.ShowModal()
            if ret == wx.ID_NO:
                dlg.Destroy()
                return
            dlg.Destroy()

        RunCommand("g.copy",
                    overwrite = True,
                    vect = [self.results["vect_map"].GetVectMapName(), mapName])

        if len(self.giface.GetLayerList().GetLayersByName(mapName)) == 0:
            # TODO: get rid of insert
            cmd, cmd_colors = self.vnet_data.GetLayerStyle()
            cmd.insert(0, 'd.vect')
            cmd.append('map=%s' % mapName)

            self.giface.GetLayerList().AddLayer(ltype="vector",
                                                name=mapName,
                                                cmd=cmd,
                                                checked=True)
            if cmd_colors:
                layerStyleVnetColors = utils.CmdToTuple(cmd_colors)

                RunCommand(layerStyleVnetColors[0],
                        **layerStyleVnetColors[1])
        else:
            self.giface.updateMap.emit(render=True, renderVector=True)

class VNETAnalyses:
    def __init__(self, data, onAnDone, goutput, tmp_maps):
        self.data = data

        self.pts_data = self.data.GetPointsData()
        self.tmp_maps = tmp_maps

        self.onAnDone = onAnDone
        self.goutput = goutput

    def RunAnalysis(self, output, params, flags):
        """!Perform network analysis"""

        analysis, valid = self.data.GetParam("analysis")

        catPts = self._getPtByCat(analysis)

        if analysis == "v.net.path":
            self._vnetPathRunAn(analysis, output, params, flags, catPts)
        elif flags["t"]:
            self._runTurnsAn(analysis, output, params, flags, catPts)
        else:
            self._runAn(analysis, output, params, flags, catPts)

    def _vnetPathRunAn(self, analysis, output, params, flags, catPts):
        """!Called when analysis is run for v.net.path module"""
        if self.pts_data.GetPointsCount() < 1:
            return False
        cats = self.data.GetAnalysisProperties()["cmdParams"]["cats"]

        # Creates part of cmd fro analysis
        cmdParams = [analysis]
        cmdParams.extend(self._setInputParams(analysis, params, flags))
        cmdParams.append("output=" + output)

        cmdPts = []
        for cat in cats:
            if  len(catPts[cat[0]]) < 1:#TODO
                GMessage(message=_("Please choose '%s' and '%s' point.") % (cats[0][1], cats[1][1]))
                return False
            cmdPts.append(catPts[cat[0]][0])

        resId = 1
        inpPoints = str(resId) + " " + str(cmdPts[0][0]) + " " + str(cmdPts[0][1]) + \
                                 " " + str(cmdPts[1][0]) + " " + str(cmdPts[1][1])

        self.coordsTmpFile = grass.tempfile()
        coordsTmpFileOpened = open(self.coordsTmpFile, 'w')
        coordsTmpFileOpened.write(inpPoints)
        coordsTmpFileOpened.close()

        if flags["t"]:
            cmdParams.append("-t")

            self.tmpTurnAn = AddTmpMapAnalysisMsg("vnet_tunr_an_tmp", self.tmp_maps)
            if not self.tmpTurnAn:
                return False

            mapName, mapSet = ParseMapStr(self.tmpTurnAn.GetVectMapName())
            cmdCopy = [ 
                        "g.copy",
                        "vect=%s,%s" % (params["input"], mapName), 
                        "--overwrite",                             
                       ]
            cmdParams.append("input=" + self.tmpTurnAn.GetVectMapName())

            ret, msg, err = RunCommand('g.copy',
                                        getErrorMsg = True,
                                        vect = "%s,%s" % (params['input'], mapName),
                                        read = True,
                                        overwrite = True)

            self._updateTtbByGlobalCosts(self.tmpTurnAn.GetVectMapName(), 
                                         int(params["tlayer"]))
            
            #self._prepareCmd(cmdCopy)
            #self.goutput.RunCmd(command = cmdCopy)
        else:
            cmdParams.append("input=" + params["input"])

        cmdParams.append("file=" + self.coordsTmpFile)

        cmdParams.append("dmax=" + str(params["max_dist"]))

        cmdParams.append("--overwrite")
        self._prepareCmd(cmd = cmdParams)
        
        if flags["t"]:
            self.goutput.RunCmd(command = cmdParams, onDone = self._vnetPathRunTurnsAnDone)
        else:
            self.goutput.RunCmd(command = cmdParams, onDone = self._vnetPathRunAnDone)

    def _vnetPathRunTurnsAnDone(self, cmd, returncode):
        #TODO
        #self.tmp_maps.DeleteTmpMap(self.tmpTurnAn)
        self._vnetPathRunAnDone(cmd, returncode)

    def _vnetPathRunAnDone(self, cmd, returncode):
        """!Called when v.net.path analysis is done"""
        grass.try_remove(self.coordsTmpFile)

        self._onDone(cmd, returncode)

    def _onDone(self, cmd, returncode):
        for c in cmd:
            if "output=" in c:
                output = c.split("=")[1]
                break  

        self.onAnDone(cmd, returncode, output)

    def _runTurnsAn(self, analysis, output, params, flags, catPts):

        # Creates part of cmd fro analysis
        cmdParams = [analysis]
        cmdParams.extend(self._setInputParams(analysis, params, flags))
        cmdParams.append("output=" + output)

        cats = {}
        for cat_name, pts_coor in catPts.iteritems():

            for coor in pts_coor:
                cat_num = str(GetNearestNodeCat(e = coor[0], 
                                                n = coor[1], 
                                                field = int(params["tuclayer"],
                                                tresh = params["max_dist"]), 
                                                vectMap = params["input"]))
                if cat_num < 0:
                    continue
                if cats.has_key(cat_name):
                    cats[cat_name].append(cat_num)
                else:
                    cats[cat_name] = [cat_num]

        for cat_name, cat_nums in cats.iteritems():
            cmdParams.append(cat_name + "=" + ",".join(cat_nums))

        self.tmpTurnAn = AddTmpMapAnalysisMsg("vnet_tunr_an_tmp", self.tmp_maps)
        if not self.tmpTurnAn:
            return False

        # create and run commands which goes to analysis thread

        mapName, mapSet = ParseMapStr(self.tmpTurnAn.GetVectMapName())
        cmdCopy = [ 
                    "g.copy",
                    "vect=%s,%s" % (params['input'], mapName), 
                    "--overwrite",                             
                   ]
        cmdParams.append("input=" + self.tmpTurnAn.GetVectMapName())

        ret, msg, err = RunCommand('g.copy',
                                    getErrorMsg = True,
                                    vect = "%s,%s" % (params['input'], mapName),
                                    read = True,
                                    overwrite = True)

        self._updateTtbByGlobalCosts(self.tmpTurnAn.GetVectMapName(), 
                                     int(params["tlayer"]))


        self._setCmdForSpecificAn(cmdParams)

        cmdParams.append("-t")

        self._prepareCmd(cmdParams)
        self.goutput.RunCmd(command = cmdParams, onDone = self._runTurnsAnDone)

    def _updateTtbByGlobalCosts(self, vectMapName, tlayer):
        #TODO get layer number do not use it directly
        intervals = self.turnsData["global"].GetData()

        cmdUpdGlob = [ 
                      "v.db.update",
                      "map=", self.inputData["input"].GetValue(), 
                      "layer=%d" % tlayer,
                      "column=cost",
                    ]

        dbInfo = VectorDBInfo(vectMapName)
        table = dbInfo.GetTable(tlayer)
        driver,  database = dbInfo.GetDbSettings(tlayer)


        sqlFile = grass.tempfile()
        sqlFile_f = open(sqlFile, 'w')

        for ival in intervals:
            from_angle = ival[0]
            to_angle = ival[1]
            cost = ival[2]

            if to_angle < from_angle:
                to_angle = math.pi * 2  + to_angle
            #if angle < from_angle:
            #    angle = math.pi * 2  + angle

            where = " WHERE (((angle < {0}) AND ({2} + angle >= {0} AND {2} + angle < {1})) OR \
                            ((angle >= {0}) AND (angle >= {0} AND angle < {1}))) AND cost==0.0 ".format(str(from_angle), str(to_angle), str(math.pi * 2))

            stm = ("UPDATE %s SET cost=%f " % (table, cost)) + where + ";\n";
            sqlFile_f.write(stm)

        sqlFile_f.close()

            #TODO imporve parser and run in thread

   
        ret, msg, err = RunCommand('db.execute',
                               getErrorMsg = True,
                               input = sqlFile,
                               read = True,
                               driver = driver,
                               database = database)

        grass.try_remove(sqlFile)

    def _runTurnsAnDone(self, cmd, returncode):
        """!Called when analysis is done"""
        #self.tmp_maps.DeleteTmpMap(self.tmpTurnAn) #TODO remove earlier (OnDone lambda?)
 
        self._onDone(cmd, returncode)


    def _runAn(self, analysis, output, params, flags, catPts):
        """!Called for all v.net.* analysis (except v.net.path)"""

        # Creates part of cmd fro analysis
        cmdParams = [analysis]
        cmdParams.extend(self._setInputParams(analysis, params, flags))
        cmdParams.append("output=" + output)

        cats = self.data.GetAnalysisProperties()["cmdParams"]["cats"]

        if len(cats) > 1:
            for cat in cats:
                if  len(catPts[cat[0]]) < 1:
                    GMessage(parent = self, 
                            message = _("Please choose '%s' and '%s' point.") \
                                        % (cats[0][1], cats[1][1]))
                    return False
        else:
            for cat in cats:
                if  len(catPts[cat[0]]) < 2:
                    GMessage(parent = self, 
                             message = _("Please choose at least two points."))
                    return False      

        # TODO add also to thread for analysis?
        vcatResult = RunCommand("v.category",
                           input = params['input'],
                           option = "report",
                           flags = "g",
                           read = True)     

        vcatResult = vcatResult.splitlines()
        for cat in vcatResult:#TODO
            cat = cat.split()
            if "all" in cat:
                maxCat = int(cat[4])
                break

        layerNum = params["nlayer"]

        pt_ascii, catsNums = self._getAsciiPts (catPts = catPts, 
                                                maxCat = maxCat, 
                                                layerNum = layerNum)

        self.tmpPtsAsciiFile = grass.tempfile()#TODO better tmp files cleanup (make class for managing tmp files)
        tmpPtsAsciiFileOpened = open(self.tmpPtsAsciiFile, 'w')
        tmpPtsAsciiFileOpened.write(pt_ascii)
        tmpPtsAsciiFileOpened.close()

        self.tmpInPts = AddTmpMapAnalysisMsg("vnet_tmp_in_pts", self.tmp_maps)
        if not self.tmpInPts:
            return False

        self.tmpInPtsConnected = AddTmpMapAnalysisMsg("vnet_tmp_in_pts_connected", self.tmp_maps)
        if not self.tmpInPtsConnected:
            return False

        cmdParams.append("input=" + self.tmpInPtsConnected.GetVectMapName())
        cmdParams.append("--overwrite")  
        
        self._setCmdForSpecificAn(cmdParams)
        
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
                    "input=" + params['input'],
                    "output=" + self.tmpInPtsConnected.GetVectMapName(),
                    "alayer=" +  params["alayer"],
                    "nlayer=" +  params["nlayer"], 
                    "operation=connect",
                    "thresh=" + str(params["max_dist"]),             
                    "--overwrite"                         
                  ] #TODO snapping to nodes optimization

        self._prepareCmd(cmdVNet)
        self.goutput.RunCmd(command = cmdVNet)

        self._prepareCmd(cmdParams)
        self.goutput.RunCmd(command = cmdParams, onDone = self._runAnDone)

    def _runAnDone(self, cmd, returncode):
        """!Called when analysis is done"""
        self.tmp_maps.DeleteTmpMap(self.tmpInPts) #TODO remove earlier (OnDone lambda?)
        self.tmp_maps.DeleteTmpMap(self.tmpInPtsConnected)
        grass.try_remove(self.tmpPtsAsciiFile)

        if cmd[0] == "v.net.flow":
            self.tmp_maps.DeleteTmpMap(self.vnetFlowTmpCut)

        self._onDone(cmd, returncode)

    def _setInputParams(self, analysis, params, flags):
        """!Return list of chosen values (vector map, layers). 

        The list items are in form to be used in command for analysis e.g. 'alayer=1'.    
        """

        inParams = []
        for col, v in self.data.GetAnalysisProperties()["cmdParams"]["cols"].iteritems():

            if "inputField" in v:
                colInptF = v["inputField"]
            else:
                colInptF = col

            inParams.append(col + '=' + params[colInptF])

        for layer in ['alayer', 'nlayer', 'tlayer', 'tuclayer']:
            if not flags["t"] and layer in ['tlayer', 'tuclayer']:
                continue
            # TODO
            if flags["t"] and layer == 'nlayer':
                inParams.append(layer + "=" + params['tuclayer'])
                continue

            inParams.append(layer + "=" + params[layer])

        return inParams

    def _getPtByCat(self, analysis):
        """!Return points separated by theirs categories"""
        anProps = self.data.GetAnalysisProperties()
        cats = anProps["cmdParams"]["cats"]

        ptByCats = {}
        for cat in anProps["cmdParams"]["cats"]:
            ptByCats[cat[0]] = []
 
        for i in range(self.pts_data.GetPointsCount()):
            pt_data = self.pts_data.GetPointData(i)
            if pt_data["use"]:
                for i_cat, cat in enumerate(cats):
                    #i_cat + 1 - we ave to increment it because pt_data["type"] includes "" in first place
                    if (i_cat + 1) == pt_data["type"] or len(ptByCats) == 1: 
                        coords = (pt_data['e'], pt_data['n'])
                        ptByCats[cat[0]].append(coords)

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

    def _setCmdForSpecificAn(self, cmdParams):
        # append parameters needed for particular analysis
        if cmdParams[0] == "v.net.distance":
            cmdParams.append("from_layer=1")
            cmdParams.append("to_layer=1")
        elif cmdParams[0] == "v.net.flow":
            #self.vnetFlowTmpCut = self.NewTmpVectMapToHist('vnet_tmp_flow_cut') TODO
            self.vnetFlowTmpCut = AddTmpMapAnalysisMsg('vnet_tmp_flow_cut', self.tmp_maps)
            if not self.vnetFlowTmpCut:
                return          
            cmdParams.append("cut=" +  self.vnetFlowTmpCut.GetVectMapName())         
        elif cmdParams[0] == "v.net.iso":
            costs, valid = self.data.GetParam("iso_lines") #TODO valid
            cmdParams.append("costs=" + costs)

class VNETHistory():
    def __init__(self, guiparent, data, tmp_maps):
        self.history = History()
        self.guiparent = guiparent

        self.tmp_maps = tmp_maps

        # variable, which appends  unique number to every name of map, which is saved into history
        self.histTmpVectMapNum = 0
        
        self.data = data
    
    def Undo(self):
        """!Step back in history"""
        histStepData = self.history.GetPrev()

        if histStepData:
            return self._updateHistStepData(histStepData)

        return None

    def Redo(self):
        """!Step forward in history"""
        histStepData = self.history.GetNext()

        if histStepData:
            return self._updateHistStepData(histStepData)

        return None

    def GetHistStep(self):
        return self.history.GetCurrHistStep(), self.history.GetStepsNum()

    def SaveHistStep(self):
        """!Save new step into history"""
        removedHistData = self.history.SaveHistStep()

        if not removedHistData:
            return

        # delete temporary maps in history steps which were deleted 
        for removedStep in removedHistData.itervalues():
            mapsNames = removedStep["tmp_data"]["maps"]
            for vectMapName in mapsNames:
                tmpMap = self.tmp_maps.GetTmpVectMap(vectMapName)
                self.tmp_maps.DeleteTmpMap(tmpMap)

    def DeleteNewHistStepData(self):
        # history - delete data in buffer for hist step  
        self.history.DeleteNewHistStepData()
        # empty list for maps to be saved to history
        self.tmpVectMapsToHist= []

    def _updateHistStepData(self, histStepData):
        """!Updates dialog according to chosen history step"""
        # set analysis module
        analysis = histStepData["vnet_modules"]["curr_module"]
        self.data.SetParams({"analysis" : analysis}, {})

        pts = []
        # add points to list
        for iPt in range(len(histStepData["points"])):
            ptDataHist = histStepData["points"]["pt" + str(iPt)]
            
            e, n = ptDataHist["coords"]
            pt_data = {"e" : e, "n" : n}

            pt_data['type'] = int(ptDataHist["catIdx"])

            pt_data['topology'] = ptDataHist["topology"]

            pt_data['use'] = ptDataHist["checked"]

            pts.append(pt_data)

        self.data.GetPointsData().SetPoints(pts)

        # update analysis result maps 
        mapsNames = histStepData["tmp_data"]["maps"]
        for m in mapsNames:
            if "vnet_tmp_result" in m:
                resultMapName = m
                break

        # update parameters
        params = {}
        histInputData = histStepData["an_params"]
        for inpName, inp in histInputData.iteritems():
            params[inpName] = str(inp) 
            if inpName == "input":
                inpMap = inp

        prevInpModTime = str(histStepData["other"]["input_modified"])
        currInpModTime = VectMap(None, inpMap).GetLastModified()

        if currInpModTime.strip()!= prevInpModTime.strip():
            dlg = wx.MessageDialog(parent = self.guiparent,
                                   message = _("Input map '%s' for analysis was changed outside " + 
                                               "vector network analysis tool.\n" +
                                               "Topology column may not " +
                                               "correspond to changed situation.") %\
                                                inpMap,
                                   caption = _("Input changed outside"),
                                   style =  wx.ICON_INFORMATION| wx.CENTRE)
            dlg.ShowModal()
            dlg.Destroy()
    
        #TODO 
        flags = {}
        return analysis, resultMapName, params, flags 

    def _saveAnInputToHist(self, analysis, params, flags):
        """!Save all data needed for analysis into history buffer"""
        pts_num = self.data.GetPointsData().GetPointsCount()

        for pt_id in range(pts_num):
            data = self.data.GetPointsData().GetPointData(pt_id)

            ptName = "pt" + str(pt_id)

            coords = [data["e"], data["n"]]
            self.history.Add(key = "points", 
                             subkey = [ptName, "coords"], 
                             value = coords)

            self.history.Add(key = "points", 
                             subkey = [ptName, "catIdx"], 
                             value = data['type'])

            self.history.Add(key = "points", 
                             subkey = [ptName, "topology"], 
                             value = data['topology'])


            self.history.Add(key = "points", 
                             subkey = [ptName, "checked"], 
                             value = data["use"])

            for param, value in params.iteritems():

                if param == "input":
                    inpMap = VectMap(self, value)
                    self.history.Add(key = "other", 
                                     subkey = "input_modified", 
                                     value = inpMap.GetLastModified())  
                    param_val =  value
                else:
                    param_val =  value
                 
                self.history.Add(key = "an_params", 
                                 subkey = param, 
                                 value = param_val)
            
        self.history.Add(key = "vnet_modules", 
                         subkey = "curr_module", 
                         value =  analysis)


    def NewTmpVectMapToHist(self, prefMapName):
        """!Add new vector map, which will be saved into history step"""

        mapName = prefMapName + str(self.histTmpVectMapNum)
        self.histTmpVectMapNum += 1

        tmpMap = AddTmpMapAnalysisMsg(mapName, self.tmp_maps)
        if not tmpMap:
            return tmpMap
           
        self.tmpVectMapsToHist.append(tmpMap.GetVectMapName())
        self.history.Add(key = "tmp_data", 
                         subkey = "maps",
                         value = self.tmpVectMapsToHist)

        return tmpMap

def AddTmpMapAnalysisMsg(mapName, tmp_maps): #TODO 
        """!Wraped AddTmpVectMap"""
        msg = _("Temporary map %s  already exists.\n"  + 
                "Do you want to continue in analysis and overwrite it?") \
                 % (mapName +'@' + grass.gisenv()['MAPSET'])
        tmpMap = tmp_maps.AddTmpVectMap(mapName, msg)
        return tmpMap


class SnappingNodes(wx.EvtHandler):
    def __init__(self, giface, data, tmp_maps, mapWin):

        self.giface = giface        
        self.data = data
        self.tmp_maps = tmp_maps
        self.mapWin = mapWin

        wx.EvtHandler.__init__(self)
        self.snapping= Signal('VNETManager.snapping')

        # Stores all data related to snapping
        self.snapData = {}

    def ComputeNodes(self, activate):
        """!Start/stop snapping mode"""

        if not haveCtypes:
            GMessage(parent = self,
                     message = _("Unable to use ctypes. \n") + \
                               _("Snapping mode can not be activated."))
            return -1

        if not activate:

            if self.tmp_maps.HasTmpVectMap("vnet_snap_points"):
                self.snapPts.DeleteRenderLayer() 
                
                self.giface.updateMap.emit(render=False, renderVector=False)

            if self.snapData.has_key('cmdThread'):
                self.snapData['cmdThread'].abort()

            self.data.SetSnapping(False)
            
            self.snapping.emit(evt = "deactivated")

            return  -1

        self.data.SetSnapping(activate)

        params, inv_params, flags = self.data.GetParams()
        if not self.data.InputsErrorMsgs(msg = _("Snapping mode can not be activated."),
                                     analysis = None,
                                     params = params,
                                     inv_params = inv_params,
                                     flags = flags,
                                     relevant_params = ["input", "nlayer"]):
            return -1

        if not self.tmp_maps.HasTmpVectMap("vnet_snap_points"):
            endStr = _("Do you really want to activate snapping and overwrite it?")
            self.snapPts = self.tmp_maps.AddTmpVectMap("vnet_snap_points", endStr)

            if not self.snapPts:
                return -1 

        elif self.snapPts.VectMapState() == 0:
                dlg = wx.MessageDialog(message = _("Temporary map '%s' was changed outside " +
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
                    self.tmp_maps.DeleteTmpMap(self.snapPts)
                    return -1

        self.data.SetSnapping(True)

        inpFullName = params["input"]
        inpName, mapSet = inpFullName.split("@")
        computeNodes = True

        if not self.snapData.has_key("inputMap"):
            pass
        elif inpFullName != self.snapData["inputMap"].GetVectMapName():
            self.snapData["inputMap"] = VectMap(None, inpFullName)
        elif self.snapData["inputMap"].VectMapState() == 1:
                computeNodes = False
    
        # new map needed
        if computeNodes:
            if not self.snapData.has_key('cmdThread'):
                self.snapData['cmdThread'] = CmdThread(self)
            else:
                self.snapData['cmdThread'].abort()

            cmd = ["v.to.points", "input=" + params['input'], 
                                  "output=" + self.snapPts.GetVectMapName(),
                                  "use=node", "--overwrite"]
            # process GRASS command with argument
            self.snapData["inputMap"] = VectMap(None, inpFullName)
            self.snapData["inputMap"].SaveVectMapState()

            self.Bind(EVT_CMD_DONE, self._onNodesDone)
            self.snapData['cmdThread'].RunCmd(cmd)

            self.snapping.emit(evt = "computing_points")

            return 0
        # map is already created and up to date for input data
        else:
            self.snapPts.AddRenderLayer()
            
            self.giface.updateMap.emit(render=True, renderVector=True)

            self.snapping.emit(evt = "computing_points_done")

            return 1

    def _onNodesDone(self, event):
        """!Update map window, when map with nodes to snap is created"""
        if not event.aborted:
            self.snapPts.SaveVectMapState()
            self.snapPts.AddRenderLayer() 

            self.giface.updateMap.emit(render=True, renderVector=True)

            self.snapping.emit(evt = "computing_points_done")
