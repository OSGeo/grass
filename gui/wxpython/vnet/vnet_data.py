"""
@package vnet.vnet_data

@brief Vector network analysis classes for data management.

Classes:
 - vnet_data::VNETData
 - vnet_data::VNETPointsData
 - vnet_data::VNETAnalysisParameters
 - vnet_data::VNETAnalysesProperties
 - vnet_data::VNETTmpVectMaps
 - vnet_data::VectMap
 - vnet_data::History
 - vnet_data::VNETGlobalTurnsData

(C) 2013-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (GSoC 2012, mentor: Martin Landa)
@author Lukas Bocan <silent_bob centrum.cz> (turn costs support)
@author Eliska Kyzlikova <eliska.kyzlikova gmail.com> (turn costs support)
"""
import os
import math
import six
from copy import deepcopy

from grass.script.utils import try_remove
from grass.script import core as grass
from grass.script.task import cmdlist_to_tuple

import wx


from core import utils
from core.gcmd import RunCommand, GMessage
from core.settings import UserSettings

from vnet.vnet_utils import ParseMapStr, SnapToNode

from gui_core.gselect import VectorDBInfo
from grass.pydispatch.signal import Signal

from vnet.vnet_utils import DegreesToRadians


class VNETData:
    def __init__(self, guiparent, mapWin):

        # setting initialization
        self._initSettings()

        self.guiparent = guiparent

        self.an_props = VNETAnalysesProperties()
        self.an_params = VNETAnalysisParameters(self.an_props)

        self.an_points = VNETPointsData(mapWin, self.an_props, self.an_params)

        self.global_turns = VNETGlobalTurnsData()

        self.pointsChanged = self.an_points.pointsChanged
        self.parametersChanged = self.an_params.parametersChanged

    def CleanUp(self):
        self.an_points.CleanUp()

    def GetAnalyses(self):
        return self.an_props.used_an

    def GetPointsData(self):
        return self.an_points

    def GetGlobalTurnsData(self):
        return self.global_turns

    def GetRelevantParams(self, analysis=None):
        if analysis:
            return self.an_props.GetRelevantParams(analysis)
        else:
            analysis, valid = self.an_params.GetParam("analysis")
            return self.an_props.GetRelevantParams(analysis)

    def GetAnalysisProperties(self, analysis=None):
        if analysis:
            return self.an_props[analysis]
        else:
            analysis, valid = self.an_params.GetParam("analysis")
            return self.an_props[analysis]

    def GetParam(self, param):
        return self.an_params.GetParam(param)

    def GetParams(self):
        return self.an_params.GetParams()

    def SetParams(self, params, flags):
        return self.an_params.SetParams(params, flags)

    def SetSnapping(self, activate):
        self.an_points.SetSnapping(activate)

    def GetSnapping(self):
        return self.an_points.GetSnapping()

    def GetLayerStyle(self):
        """Returns cmd for d.vect, with set style for analysis result"""
        analysis, valid = self.an_params.GetParam("analysis")

        resProps = self.an_props[analysis]["resultProps"]

        width = UserSettings.Get(group="vnet", key="res_style", subkey="line_width")
        layerStyleCmd = ["layer=1", "width=" + str(width)]

        if "catColor" in resProps:
            layerStyleCmd.append("flags=c")
        elif "singleColor" in resProps:
            col = UserSettings.Get(group="vnet", key="res_style", subkey="line_color")
            layerStyleCmd.append(
                "color=" + str(col[0]) + ":" + str(col[1]) + ":" + str(col[2])
            )

        layerStyleVnetColors = []
        if "attrColColor" in resProps:
            colorStyle = UserSettings.Get(
                group="vnet", key="res_style", subkey="color_table"
            )
            invert = UserSettings.Get(
                group="vnet", key="res_style", subkey="invert_colors"
            )

            layerStyleVnetColors = [
                "v.colors",
                "color=" + colorStyle,
                "column=" + resProps["attrColColor"],
            ]
            if invert:
                layerStyleVnetColors.append("-n")

        return layerStyleCmd, layerStyleVnetColors

    def InputsErrorMsgs(
        self, msg, analysis, params, flags, inv_params, relevant_params
    ):
        """Checks input data in Parameters tab and shows messages if some value is not valid

        :param str msg: message added to start of message string
        :return: True if checked inputs are OK
        :return: False if some of checked inputs is not ok
        """

        if flags["t"] and "turn_layer" not in relevant_params:
            GMessage(
                parent=self.guiparent,
                message=_("Module <%s> does not support turns costs." % analysis),
            )
            return False

        errMapStr = ""
        if "input" in inv_params:
            if params["input"]:
                errMapStr = _("Vector map '%s' does not exist.") % (params["input"])
            else:
                errMapStr = _("Vector map was not chosen.")

        if errMapStr:
            GMessage(parent=self.guiparent, message=msg + "\n" + errMapStr)
            return False

        errLayerStr = ""
        vals = {
            "arc_layer": _("arc layer"),
            "node_layer": _("node layer"),
            "turn_layer": _("turntable layer"),
            "turn_cat_layer": _("unique categories layer"),
        }
        for layer, layerLabel in six.iteritems(vals):

            if layer in ["turn_layer", "turn_cat_layer"] and not flags["t"]:
                continue
            if layer in inv_params:
                if params[layer]:
                    errLayerStr += _(
                        "Chosen %s '%s' does not exist in vector map '%s'.\n"
                    ) % (layerLabel, params[layer], params["input"])
                else:
                    errLayerStr += _("Choose existing %s.\n") % (layerLabel)
        if errLayerStr:
            GMessage(parent=self.guiparent, message=msg + "\n" + errLayerStr)
            return False

        errColStr = ""
        for col in ["arc_column", "arc_backward_column", "node_column"]:
            if params[col] and col in inv_params and col in relevant_params:
                errColStr += _(
                    "Chosen column '%s' does not exist in attribute table of layer '%s' of vector map '%s'.\n"
                ) % (params[col], params[layer], params["input"])

        if errColStr:
            GMessage(parent=self.guiparent, message=msg + "\n" + errColStr)
            return False

        return True

    def _initSettings(self):
        """Initialization of settings (if not already defined)"""
        # initializes default settings
        initSettings = [
            ["res_style", "line_width", 5],
            ["res_style", "line_color", (192, 0, 0)],
            ["res_style", "color_table", "byr"],
            ["res_style", "invert_colors", False],
            ["point_symbol", "point_size", 10],
            ["point_symbol", "point_width", 2],
            ["point_colors", "unused", (131, 139, 139)],
            ["point_colors", "used1cat", (192, 0, 0)],
            ["point_colors", "used2cat", (0, 0, 255)],
            ["point_colors", "selected", (9, 249, 17)],
            ["other", "snap_tresh", 10],
            ["other", "max_hist_steps", 5],
        ]

        for init in initSettings:
            UserSettings.ReadSettingsFile()
            UserSettings.Append(
                dict=UserSettings.userSettings,
                group="vnet",
                key=init[0],
                subkey=init[1],
                value=init[2],
                overwrite=False,
            )


class VNETPointsData:
    def __init__(self, mapWin, an_data, an_params):

        self.mapWin = mapWin
        self.an_data = an_data
        self.an_params = an_params

        # information, whether mouse event handler is registered in map window
        self.handlerRegistered = False

        self.pointsChanged = Signal("VNETPointsData.pointsChanged")
        self.an_params.parametersChanged.connect(self.ParametersChanged)

        self.snapping = False

        self.data = []
        self.cols = {
            "name": ["use", "type", "topology", "e", "n"],
            "label": [_("use"), _("type"), _("topology"), "e", "n"],
            # TDO
            "type": [None, ["", _("Start point"), _("End Point")], None, float, float],
            "def_vals": [False, 0, "new point", 0, 0],
        }

        # registration graphics for drawing
        self.pointsToDraw = self.mapWin.RegisterGraphicsToDraw(
            graphicsType="point", setStatusFunc=self.SetPointStatus
        )

        self.SetPointDrawSettings()

        self.AddPoint()
        self.AddPoint()

        self.SetPointData(0, {"use": True, "type": 1})
        self.SetPointData(1, {"use": True, "type": 2})

        self.selected = 0

    def __del__(self):
        self.CleanUp()

    def CleanUp(self):
        self.mapWin.UnregisterGraphicsToDraw(self.pointsToDraw)

        if self.handlerRegistered:
            self.mapWin.UnregisterMouseEventHandler(
                wx.EVT_LEFT_DOWN, self.OnMapClickHandler
            )

    def SetSnapping(self, activate):
        self.snapping = activate

    def GetSnapping(self):
        return self.snapping

    def AddPoint(self):

        self.pointsToDraw.AddItem(
            coords=(self.cols["def_vals"][3], self.cols["def_vals"][4])
        )
        self.data.append(self.cols["def_vals"][:])

        self.pointsChanged.emit(method="AddPoint", kwargs={})

    def DeletePoint(self, pt_id):
        item = self.pointsToDraw.GetItem(pt_id)
        if item:
            self.pointsToDraw.DeleteItem(item)
            self.data.pop(pt_id)

        self.pointsChanged.emit(method="DeletePoint", kwargs={"pt_id": pt_id})

    def SetPoints(self, pts_data):

        for item in self.pointsToDraw.GetAllItems():
            self.pointsToDraw.DeleteItem(item)

        self.data = []
        for pt_data in pts_data:
            pt_data_list = self._ptDataToList(pt_data)
            self.data.append(pt_data_list)
            self.pointsToDraw.AddItem(coords=(pt_data_list[3], pt_data_list[4]))

        self.pointsChanged.emit(method="SetPoints", kwargs={"pts_data": pts_data})

    def SetPointData(self, pt_id, data):
        for col, v in six.iteritems(data):
            if col == "use":
                continue

            idx = self.cols["name"].index(col)
            self.data[pt_id][idx] = v

        # if type is changed checked columns must be recalculated by _usePoint
        if "type" in data and "use" not in data:
            data["use"] = self.GetPointData(pt_id)["use"]

        if "use" in data:
            if self._usePoint(pt_id, data["use"]) == -1:
                data["use"] = False
            idx = self.cols["name"].index("use")
            self.data[pt_id][idx] = data["use"]

        self.pointsChanged.emit(
            method="SetPointData", kwargs={"pt_id": pt_id, "data": data}
        )

    def GetPointData(self, pt_id):
        return self._ptListDataToPtData(self.data[pt_id])

    def GetPointsCount(self):
        return len(self.data)

    def SetPointStatus(self, item, itemIndex):
        """Before point is drawn, decides properties of drawing style"""
        analysis, valid = self.an_params.GetParam("analysis")
        cats = self.an_data[analysis]["cmdParams"]["cats"]

        if itemIndex == self.selected:
            wxPen = "selected"
        elif not self.data[itemIndex][0]:
            wxPen = "unused"
            item.hide = False
        elif len(cats) > 1:
            idx = self.data[itemIndex][1]
            if idx == 2:  # End/To/Sink point
                wxPen = "used2cat"
            else:
                wxPen = "used1cat"
        else:
            wxPen = "used1cat"

        item.SetPropertyVal("label", str(itemIndex + 1))
        item.SetPropertyVal("penName", wxPen)

    def SetSelected(self, pt_id):
        self.selected = pt_id
        self.pointsChanged.emit(method="SetSelected", kwargs={"pt_id": pt_id})

    def GetSelected(self):
        return self.selected

    def SetPointDrawSettings(self):
        """Set settings for drawing of points"""
        ptSize = int(
            UserSettings.Get(group="vnet", key="point_symbol", subkey="point_size")
        )
        self.pointsToDraw.SetPropertyVal("size", ptSize)

        colors = UserSettings.Get(group="vnet", key="point_colors")
        ptWidth = int(
            UserSettings.Get(group="vnet", key="point_symbol", subkey="point_width")
        )

        textProp = self.pointsToDraw.GetPropertyVal("text")
        textProp["font"].SetPointSize(ptSize + 2)

        for colKey, col in six.iteritems(colors):
            pen = self.pointsToDraw.GetPen(colKey)
            if pen:
                pen.SetColour(wx.Colour(col[0], col[1], col[2], 255))
                pen.SetWidth(ptWidth)
            else:
                self.pointsToDraw.AddPen(
                    colKey,
                    wx.Pen(
                        colour=wx.Colour(col[0], col[1], col[2], 255), width=ptWidth
                    ),
                )

    def ParametersChanged(self, method, kwargs):
        if "analysis" in list(kwargs["changed_params"].keys()):
            self._updateTypeCol()

            if self.an_params.GetParam("analysis")[0] == "v.net.path":
                self._vnetPathUpdateUsePoints(None)

    def _updateTypeCol(self):
        """Rename category values when module is changed. Expample: Start point -> Sink point"""
        colValues = [""]
        analysis, valid = self.an_params.GetParam("analysis")
        anParamsCats = self.an_data[analysis]["cmdParams"]["cats"]

        for ptCat in anParamsCats:
            colValues.append(ptCat[1])

        type_idx = self.cols["name"].index("type")
        self.cols["type"][type_idx] = colValues

    def _ptDataToList(self, pt_data):

        pt_list_data = [None] * len(self.cols["name"])

        for k, val in six.iteritems(pt_data):
            pt_list_data[self.cols["name"].index(k)] = val

        return pt_list_data

    def _ptListDataToPtData(self, pt_list_data):

        pt_data = {}
        for i, val in enumerate(pt_list_data):
            pt_data[self.cols["name"][i]] = val

        return pt_data

    def _usePoint(self, pt_id, use):
        """Item is checked/unchecked"""
        analysis, valid = self.an_params.GetParam("analysis")
        cats = self.an_data[analysis]["cmdParams"]["cats"]
        # TODO move
        # if self.updateMap:
        #    up_map_evt = gUpdateMap(render = False, renderVector = False)
        #    wx.PostEvent(self.dialog.mapWin, up_map_evt)

        if len(cats) <= 1:
            return 0

        use_idx = self.cols["name"].index("use")
        checkedVal = self.data[pt_id][1]

        # point without given type cannot be selected
        if checkedVal == 0:
            self.data[pt_id][use_idx] = False
            self.pointsChanged.emit(
                method="SetPointData", kwargs={"pt_id": pt_id, "data": {"use": False}}
            )
            return -1

        if analysis == "v.net.path" and use:
            self._vnetPathUpdateUsePoints(pt_id)

    def _vnetPathUpdateUsePoints(self, checked_pt_id):

        alreadyChecked = []

        type_idx = self.cols["name"].index("type")
        use_idx = self.cols["name"].index("use")

        if checked_pt_id is not None:
            checkedKey = checked_pt_id
            alreadyChecked.append(self.data[checked_pt_id][type_idx])
        else:
            checkedKey = -1

        for iKey, dt in enumerate(self.data):
            pt_type = dt[type_idx]

            if (
                (pt_type in alreadyChecked and checkedKey != iKey) or pt_type == 0
            ) and self.data[iKey][use_idx]:
                self.data[iKey][use_idx] = False
                self.pointsChanged.emit(
                    method="SetPointData",
                    kwargs={"pt_id": iKey, "data": {"use": False}},
                )
            elif self.data[iKey][use_idx]:
                alreadyChecked.append(pt_type)

    def EditPointMode(self, activate):
        """Registers/unregisters mouse handler into map window"""

        if activate == self.handlerRegistered:
            return

        if activate:
            self.mapWin.RegisterMouseEventHandler(
                wx.EVT_LEFT_DOWN, self.OnMapClickHandler, "cross"
            )
            self.handlerRegistered = True
        else:
            self.mapWin.UnregisterMouseEventHandler(
                wx.EVT_LEFT_DOWN, self.OnMapClickHandler
            )
            self.handlerRegistered = False

        self.pointsChanged.emit(method="EditMode", kwargs={"activated": activate})

    def IsEditPointModeActive(self):
        return self.handlerRegistered

    def OnMapClickHandler(self, event):
        """Take coordinates from map window"""
        # TODO update snapping after input change
        if event == "unregistered":
            self.handlerRegistered = False
            return

        if not self.data:
            self.AddPoint()

        e, n = self.mapWin.GetLastEN()

        if self.snapping:

            # compute threshold
            snapTreshPix = int(
                UserSettings.Get(group="vnet", key="other", subkey="snap_tresh")
            )
            res = max(self.mapWin.Map.region["nsres"], self.mapWin.Map.region["ewres"])
            snapTreshDist = snapTreshPix * res

            params, err_params, flags = self.an_params.GetParams()
            vectMap = params["input"]

            if "input" in err_params:
                msg = _("new point")

            coords = SnapToNode(e, n, snapTreshDist, vectMap)
            if coords:
                e = coords[0]
                n = coords[1]

                msg = "snapped to node"
            else:
                msg = _("new point")

        else:
            msg = _("new point")

        self.SetPointData(self.selected, {"topology": msg, "e": e, "n": n})

        self.pointsToDraw.GetItem(self.selected).SetCoords([e, n])

        if self.selected == len(self.data) - 1:
            self.SetSelected(0)
        else:
            self.SetSelected(self.GetSelected() + 1)

    def GetColumns(self, only_relevant=True):

        cols_data = deepcopy(self.cols)

        hidden_cols = []
        hidden_cols.append(self.cols["name"].index("e"))
        hidden_cols.append(self.cols["name"].index("n"))

        analysis, valid = self.an_params.GetParam("analysis")
        if only_relevant and len(self.an_data[analysis]["cmdParams"]["cats"]) <= 1:
            hidden_cols.append(self.cols["name"].index("type"))

        i_red = 0
        hidden_cols.sort()
        for idx in hidden_cols:
            for dt in six.itervalues(cols_data):
                dt.pop(idx - i_red)
            i_red += 1

        return cols_data


class VNETAnalysisParameters:
    def __init__(self, an_props):

        self.an_props = an_props

        self.params = {
            "analysis": self.an_props.used_an[0],
            "input": "",
            "arc_layer": "",
            "node_layer": "",
            "arc_column": "",
            "arc_backward_column": "",
            "node_column": "",
            "turn_layer": "",
            "turn_cat_layer": "",
            "iso_lines": "",  # TODO check validity
            "max_dist": 0,
        }  # TODO check validity

        self.flags = {"t": False}

        self.parametersChanged = Signal("VNETAnalysisParameters.parametersChanged")

    def SetParams(self, params, flags):

        changed_params = {}
        for p, v in six.iteritems(params):
            if p == "analysis" and v not in self.an_props.used_an:
                continue

            if p == "input":
                mapName, mapSet = ParseMapStr(v)
                v = mapName + "@" + mapSet

            if p in self.params:
                if isinstance(v, str):
                    v = v.strip()

                self.params[p] = v
                changed_params[p] = v

        changed_flags = {}
        for p, v in six.iteritems(flags):
            if p in self.flags:
                self.flags[p] = v
                changed_flags[p] = v

        self.parametersChanged.emit(
            method="SetParams",
            kwargs={"changed_params": changed_params, "changed_flags": changed_flags},
        )

        return changed_params, changed_flags

    def GetParam(self, param):

        invParams = []
        if param in [
            "input",
            "arc_layer",
            "node_layer",
            "arc_column",
            "arc_backward_column",
            "node_column",
            "turn_layer",
            "turn_cat_layer",
        ]:
            invParams = self._getInvalidParams(self.params)

        if invParams:
            return self.params[param], False

        return self.params[param], True

    def GetParams(self):

        invParams = self._getInvalidParams(self.params)
        return self.params, invParams, self.flags

    def _getInvalidParams(self, params):
        """Check of analysis input data for invalid values (Parameters tab)"""
        # dict of invalid values {key from self.itemData (comboboxes from
        # Parameters tab) : invalid value}
        invParams = []

        # check vector map
        if params["input"]:
            mapName, mapSet = params["input"].split("@")
            if mapSet in grass.list_grouped("vector"):
                vectMaps = grass.list_grouped("vector")[mapSet]

        if not params["input"] or mapName not in vectMaps:
            invParams = list(params.keys())[:]
            return invParams

        # check arc/node layer
        layers = utils.GetVectorNumberOfLayers(params["input"])

        for layer in ["arc_layer", "node_layer", "turn_layer", "turn_cat_layer"]:
            if not layers or params[layer] not in layers:
                invParams.append(layer)

        dbInfo = VectorDBInfo(params["input"])

        try:
            table = dbInfo.GetTable(int(params["arc_layer"]))
            columnchoices = dbInfo.GetTableDesc(table)
        except (KeyError, ValueError):
            table = None

        # check costs columns
        for col in ["arc_column", "arc_backward_column", "node_column"]:

            if col == "node_column":
                try:
                    table = dbInfo.GetTable(int(params["node_layer"]))
                    columnchoices = dbInfo.GetTableDesc(table)
                except (KeyError, ValueError):
                    table = None

            if not table or not params[col] in list(columnchoices.keys()):
                invParams.append(col)
                continue

            if columnchoices[params[col]]["type"] not in [
                "integer",
                "double precision",
            ]:
                invParams.append(col)
                continue

        return invParams


class VNETAnalysesProperties:
    def __init__(self):
        """Initializes parameters for different v.net.* modules"""
        # initialization of v.net.* analysis parameters (data which
        # characterizes particular analysis)

        self.attrCols = {
            "arc_column": {
                "label": _("Arc forward/both direction(s) cost column:"),
                "name": _("arc forward/both"),
            },
            "arc_backward_column": {
                "label": _("Arc backward direction cost column:"),
                "name": _("arc backward"),
            },
            "acolumn": {
                "label": _("Arcs' cost column (for both directions):"),
                "name": _("arc"),
                "inputField": "arc_column",
            },
            "node_column": {"label": _("Node cost column:"), "name": _("node")},
        }

        self.vnetProperties = {
            "v.net.path": {
                "label": _("Shortest path %s") % "(v.net.path)",
                "cmdParams": {
                    "cats": [["st_pt", _("Start point")], ["end_pt", _("End point")]],
                    "cols": ["arc_column", "arc_backward_column", "node_column"],
                },
                "resultProps": {
                    "singleColor": None,
                    "dbMgr": True,  # TODO delete this property, this information can be get from result
                },
                "turns_support": True,
            },
            "v.net.salesman": {
                "label": _("Traveling salesman %s") % "(v.net.salesman)",
                "cmdParams": {
                    "cats": [["center_cats", None]],
                    "cols": ["arc_column", "arc_backward_column"],
                },
                "resultProps": {"singleColor": None, "dbMgr": False},
                "turns_support": True,
            },
            "v.net.flow": {
                "label": _("Maximum flow %s") % "(v.net.flow)",
                "cmdParams": {
                    "cats": [
                        ["source_cats", _("Source point")],
                        ["sink_cats", _("Sink point")],
                    ],
                    "cols": ["arc_column", "arc_backward_column", "node_column"],
                },
                "resultProps": {"attrColColor": "flow", "dbMgr": True},
                "turns_support": False,
            },
            "v.net.alloc": {
                "label": _("Subnets for nearest centers %s") % "(v.net.alloc)",
                "cmdParams": {
                    "cats": [["center_cats", None]],
                    "cols": ["arc_column", "arc_backward_column", "node_column"],
                },
                "resultProps": {"catColor": None, "dbMgr": False},
                "turns_support": True,
            },
            "v.net.steiner": {
                "label": _("Steiner tree for the network and given terminals %s")
                % "(v.net.steiner)",
                "cmdParams": {
                    "cats": [["terminal_cats", None]],
                    "cols": [
                        "acolumn",
                    ],
                },
                "resultProps": {"singleColor": None, "dbMgr": False},
                "turns_support": True,
            },
            "v.net.distance": {
                "label": _("Shortest distance via the network %s") % "(v.net.distance)",
                "cmdParams": {
                    "cats": [["from_cats", "From point"], ["to_cats", "To point"]],
                    "cols": ["arc_column", "arc_backward_column", "node_column"],
                },
                "resultProps": {"catColor": None, "dbMgr": True},
                "turns_support": False,
            },
            "v.net.iso": {
                "label": _("Cost isolines %s") % "(v.net.iso)",
                "cmdParams": {
                    "cats": [["center_cats", None]],
                    "cols": ["arc_column", "arc_backward_column", "node_column"],
                },
                "resultProps": {"catColor": None, "dbMgr": False},
                "turns_support": True,
            },
        }

        self.used_an = [
            "v.net.path",
            "v.net.salesman",
            "v.net.flow",
            "v.net.alloc",
            "v.net.distance",
            "v.net.iso",
            # "v.net.steiner"
        ]

        for an in list(self.vnetProperties.keys()):
            if an not in self.used_an:
                del self.vnetProperties[an]
                continue

            cols = self.vnetProperties[an]["cmdParams"]["cols"]
            self.vnetProperties[an]["cmdParams"]["cols"] = {}
            for c in cols:
                self.vnetProperties[an]["cmdParams"]["cols"][c] = self.attrCols[c]

    def has_key(self, key):
        return key in self.vnetProperties

    def __getitem__(self, key):
        return self.vnetProperties[key]

    def GetRelevantParams(self, analysis):

        if analysis not in self.vnetProperties:
            return None

        relevant_params = ["input", "arc_layer", "node_layer"]

        if self.vnetProperties[analysis]["turns_support"]:
            relevant_params += ["turn_layer", "turn_cat_layer"]

        cols = self.vnetProperties[analysis]["cmdParams"]["cols"]

        for col, v in six.iteritems(cols):
            if "inputField" in col:
                colInptF = v["inputField"]
            else:
                colInptF = col
            relevant_params.append(colInptF)

        return relevant_params


class VNETTmpVectMaps:
    """Class which creates, stores and destroys all tmp maps created during analysis"""

    def __init__(self, parent, mapWin):
        self.tmpMaps = []  # temporary maps
        self.parent = parent
        self.mapWin = mapWin

    def AddTmpVectMap(self, mapName, msg):
        """New temporary map

        :return: instance of VectMap representing temporary map
        """
        currMapSet = grass.gisenv()["MAPSET"]
        tmpMap = grass.find_file(name=mapName, element="vector", mapset=currMapSet)

        fullName = tmpMap["fullname"]
        # map already exists
        if fullName:
            # TODO move dialog out of class, AddTmpVectMap(self, mapName,
            # overvrite = False)
            dlg = wx.MessageDialog(
                parent=self.parent,
                message=msg,
                caption=_("Overwrite map layer"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE,
            )

            ret = dlg.ShowModal()
            dlg.Destroy()

            if ret == wx.ID_NO:
                return None
        else:
            fullName = mapName + "@" + currMapSet

        newVectMap = VectMap(self.mapWin, fullName)
        self.tmpMaps.append(newVectMap)

        return newVectMap

    def HasTmpVectMap(self, vectMapName):
        """
        :param: vectMapName name of vector map

        :return: True if it contains the map
        :return: False if not
        """

        mapValSpl = vectMapName.strip().split("@")
        if len(mapValSpl) > 1:
            mapSet = mapValSpl[1]
        else:
            mapSet = grass.gisenv()["MAPSET"]
        mapName = mapValSpl[0]
        fullName = mapName + "@" + mapSet

        for vectTmpMap in self.tmpMaps:
            if vectTmpMap.GetVectMapName() == fullName:
                return True
        return False

    def GetTmpVectMap(self, vectMapName):
        """Get instance of VectMap with name vectMapName"""
        for vectMap in self.tmpMaps:
            if vectMap.GetVectMapName() == vectMapName.strip():
                return vectMap
        return None

    def RemoveFromTmpMaps(self, vectMap):
        """Temporary map is removed from the class instance however it is not deleted

        :param vectMap: instance of VectMap class to be removed

        :return: True if was removed
        :return: False if does not contain the map
        """
        try:
            self.tmpMaps.remove(vectMap)
            return True
        except ValueError:
            return False

    def DeleteTmpMap(self, vectMap):
        """Temporary map is removed from the class and it is deleted

        :param vectMap: instance of VectMap class to be deleted

        :return: True if was removed
        :return: False if does not contain the map
        """
        if vectMap:
            vectMap.DeleteRenderLayer()
            RunCommand(
                "g.remove", flags="f", type="vector", name=vectMap.GetVectMapName()
            )
            self.RemoveFromTmpMaps(vectMap)
            return True
        return False

    def DeleteAllTmpMaps(self):
        """Delete all temporary maps in the class"""
        update = False
        for tmpMap in self.tmpMaps:
            RunCommand(
                "g.remove", flags="f", type="vector", name=tmpMap.GetVectMapName()
            )
            if tmpMap.DeleteRenderLayer():
                update = True
        return update


class VectMap:
    """Represents map
    It can check if it was modified or render it
    """

    def __init__(self, mapWin, fullName):
        self.fullName = fullName
        self.mapWin = mapWin
        self.renderLayer = None
        self.modifTime = None  # time, for modification check

    def __del__(self):

        self.DeleteRenderLayer()

    def AddRenderLayer(self, cmd=None, colorsCmd=None):
        """Add map from map window layers to render"""

        if not self.mapWin:
            return False

        existsMap = grass.find_file(
            name=self.fullName, element="vector", mapset=grass.gisenv()["MAPSET"]
        )

        if not existsMap["name"]:
            self.DeleteRenderLayer()
            return False

        if not cmd:
            cmd = []
        cmd.insert(0, "d.vect")
        cmd.append("map=%s" % self.fullName)

        if self.renderLayer:
            self.DeleteRenderLayer()

        if colorsCmd:
            colorsCmd.append("map=%s" % self.fullName)
            layerStyleVnetColors = cmdlist_to_tuple(colorsCmd)

            RunCommand(layerStyleVnetColors[0], **layerStyleVnetColors[1])

        self.renderLayer = self.mapWin.Map.AddLayer(
            ltype="vector",
            command=cmd,
            name=self.fullName,
            active=True,
            opacity=1.0,
            render=False,
            pos=-1,
        )
        return True

    def DeleteRenderLayer(self):
        """Remove map from map window layers to render"""
        if not self.mapWin:
            return False

        if self.renderLayer:
            self.mapWin.Map.DeleteLayer(self.renderLayer)
            self.renderLayer = None
            return True
        return False

    def GetRenderLayer(self):
        return self.renderLayer

    def GetVectMapName(self):
        return self.fullName

    def SaveVectMapState(self):
        """Save modification time for vector map"""
        self.modifTime = self.GetLastModified()

    def VectMapState(self):
        """Checks if map was modified

        :return: -1 - if no modification time was saved
        :return:  0 - if map was modified
        :return:  1 - if map was not modified
        """
        if self.modifTime is None:
            return -1
        if self.modifTime != self.GetLastModified():
            return 0
        return 1

    def GetLastModified(self):
        """Get modification time

        :return: MAP DATE time string from vector map head file
        """

        mapValSpl = self.fullName.split("@")
        mapSet = mapValSpl[1]
        mapName = mapValSpl[0]

        headPath = os.path.join(
            grass.gisenv()["GISDBASE"],
            grass.gisenv()["LOCATION_NAME"],
            mapSet,
            "vector",
            mapName,
            "head",
        )
        try:
            head = open(headPath, "r")
            for line in head.readlines():
                i = line.find(
                    "MAP DATE:",
                )
                if i == 0:
                    head.close()
                    return line.split(":", 1)[1].strip()

            head.close()
            return ""
        except IOError:
            return ""


class History:
    """Class which reads and saves history data (based on gui.core.settings Settings class file save/load)

    .. todo::
        Maybe it could be useful for other GRASS wxGUI tools.
    """

    def __init__(self):

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
        self.sep = ";"

    def __del__(self):
        try_remove(self.histFile)

    def GetNext(self):
        """Go one step forward in history"""
        self.currHistStep -= 1
        self.currHistStepData.clear()
        self.currHistStepData = self._getHistStepData(self.currHistStep)

        return self.currHistStepData

    def GetPrev(self):
        """Go one step back in history"""
        self.currHistStep += 1
        self.currHistStepData.clear()
        self.currHistStepData = self._getHistStepData(self.currHistStep)

        return self.currHistStepData

    def GetStepsNum(self):
        """Get number of steps saved in history"""
        return self.histStepsNum

    def GetCurrHistStep(self):
        """Get current history step"""
        return self.currHistStep

    def Add(self, key, subkey, value):
        """Add new data into buffer"""
        if key not in self.newHistStepData:
            self.newHistStepData[key] = {}

        if isinstance(subkey, list):
            if subkey[0] not in self.newHistStepData[key]:
                self.newHistStepData[key][subkey[0]] = {}
            self.newHistStepData[key][subkey[0]][subkey[1]] = value
        else:
            self.newHistStepData[key][subkey] = value

    def SaveHistStep(self):
        """Create new history step with data in buffer"""
        self.maxHistSteps = UserSettings.Get(
            group="vnet", key="other", subkey="max_hist_steps"
        )
        self.currHistStep = 0

        newHistFile = grass.tempfile()
        newHist = open(newHistFile, "w")

        self._saveNewHistStep(newHist)

        oldHist = open(self.histFile)
        removedHistData = self._savePreviousHist(newHist, oldHist)

        oldHist.close()
        newHist.close()
        try_remove(self.histFile)
        self.histFile = newHistFile

        self.newHistStepData.clear()

        return removedHistData

    def _savePreviousHist(self, newHist, oldHist):
        """Save previous history into new file"""
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
                    newHist.write("%s%s%s" % ("\n", line, "\n"))
                    self.histStepsNum = newHistStepsNum
            else:
                if newHistStepsNum >= self.maxHistSteps:
                    self._parseLine(line, removedHistStep)
                else:
                    newHist.write("%s" % line)

        return removedHistData

    def _saveNewHistStep(self, newHist):
        """Save buffer (new step) data into file"""
        newHist.write("%s%s%s" % ("\n", "history step=0", "\n"))
        for key in list(self.newHistStepData.keys()):
            subkeys = list(self.newHistStepData[key].keys())
            newHist.write("%s%s" % (key, self.sep))
            for idx in range(len(subkeys)):
                value = self.newHistStepData[key][subkeys[idx]]
                if isinstance(value, dict):
                    if idx > 0:
                        newHist.write("%s%s%s" % ("\n", key, self.sep))
                    newHist.write("%s%s" % (subkeys[idx], self.sep))
                    kvalues = list(self.newHistStepData[key][subkeys[idx]].keys())
                    srange = range(len(kvalues))
                    for sidx in srange:
                        svalue = self._parseValue(
                            self.newHistStepData[key][subkeys[idx]][kvalues[sidx]]
                        )
                        newHist.write("%s%s%s" % (kvalues[sidx], self.sep, svalue))
                        if sidx < len(kvalues) - 1:
                            newHist.write("%s" % self.sep)
                else:
                    if idx > 0 and isinstance(
                        self.newHistStepData[key][subkeys[idx - 1]], dict
                    ):
                        newHist.write("%s%s%s" % ("\n", key, self.sep))
                    value = self._parseValue(self.newHistStepData[key][subkeys[idx]])
                    newHist.write("%s%s%s" % (subkeys[idx], self.sep, value))
                    if idx < len(subkeys) - 1 and not isinstance(
                        self.newHistStepData[key][subkeys[idx + 1]], dict
                    ):
                        newHist.write("%s" % self.sep)
            newHist.write("\n")
        self.histStepsNum = 0

    def _parseValue(self, value, read=False):
        """Parse value"""
        if read:  # -> read data (cast values)

            if value:
                if (
                    value[0] == "[" and value[-1] == "]"
                ):  # TODO, possible wrong interpretation
                    value = value[1:-1].split(",")
                    value = map(self._castValue, value)
                    return value

            if value == "True":
                value = True
            elif value == "False":
                value = False
            elif value == "None":
                value = None
            elif ":" in value:  # -> color
                try:
                    value = tuple(map(int, value.split(":")))
                except ValueError:  # -> string
                    pass
            else:
                try:
                    value = int(value)
                except ValueError:
                    try:
                        value = float(value)
                    except ValueError:
                        pass
        else:  # -> write data
            if isinstance(value, type(())):  # -> color
                value = str(value[0]) + ":" + str(value[1]) + ":" + str(value[2])

        return value

    def _castValue(self, value):
        """Cast value"""
        try:
            value = int(value)
        except ValueError:
            try:
                value = float(value)
            except ValueError:
                value = value[1:-1]

        return value

    def _getHistStepData(self, histStep):
        """Load data saved in history step"""
        hist = open(self.histFile)
        histStepData = {}

        newHistStep = False
        isSearchedHistStep = False
        for line in hist.readlines():

            if not line.strip() and isSearchedHistStep:
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
        """Parse line in file with history"""
        line = line.rstrip("%s" % os.linesep).split(self.sep)
        key = line[0]
        kv = line[1:]
        idx = 0
        subkeyMaster = None
        if len(kv) % 2 != 0:  # multiple (e.g. nviz)
            subkeyMaster = kv[0]
            del kv[0]
        idx = 0
        while idx < len(kv):
            if subkeyMaster:
                subkey = [subkeyMaster, kv[idx]]
            else:
                subkey = kv[idx]
            value = kv[idx + 1]
            value = self._parseValue(value, read=True)
            if key not in histStepData:
                histStepData[key] = {}

            if isinstance(subkey, list):
                if subkey[0] not in histStepData[key]:
                    histStepData[key][subkey[0]] = {}
                histStepData[key][subkey[0]][subkey[1]] = value
            else:
                histStepData[key][subkey] = value
            idx += 2

    def DeleteNewHistStepData(self):
        """Delete buffer data for new history step"""
        self.newHistStepData.clear()


class VNETGlobalTurnsData:
    """Turn Data"""

    def __init__(self):
        # Definition of four basic directions
        self.turn_data = [
            ["Straight", DegreesToRadians(-30), DegreesToRadians(+30), 0.0],
            ["Right Turn", DegreesToRadians(+30), DegreesToRadians(+150), 0.0],
            ["Reverse", DegreesToRadians(+150), DegreesToRadians(-150), 0.0],
            ["Left Turn", DegreesToRadians(-150), DegreesToRadians(-30), 0.0],
        ]

    def GetData(self):
        data = []
        for ival in self.turn_data:
            data.append(ival[1:])

        return data

    def GetValue(self, line, col):
        return self.turn_data[line][col]

    def GetLinesCount(self):
        return len(self.turn_data)

    def SetValue(self, value, line, col):
        self.DataValidator(line, col, value)
        self.turn_data[line][col] = value

    def SetUTurns(self, value):
        """Checked if checeBox is checed"""
        useUTurns = value

    def AppendRow(self, values):
        self.turn_data.append(values)

    def InsertRow(self, line, values):
        self.turn_data.insert(line, values)

    def PopRow(self, values):
        self.RemoveDataValidator(values)
        self.turn_data.pop(values)

    def DataValidator(self, row, col, value):
        """Angle recalculation due to value changing"""

        if col not in [1, 2]:
            return

        if col == 1:
            new_from_angle = value
            old_from_angle = self.turn_data[row][1]
            new_to_angle = self.turn_data[row][2]
            if self.IsInInterval(old_from_angle, new_to_angle, new_from_angle):

                prev_row = row - 1
                if prev_row == -1:
                    prev_row = len(self.turn_data) - 1
                self.turn_data[prev_row][2] = new_from_angle
                return

        if col == 2:
            new_to_angle = value
            old_to_angle = self.turn_data[row][2]
            new_from_angle = self.turn_data[row][1]
            if self.IsInInterval(new_from_angle, old_to_angle, new_to_angle):

                next_row = row + 1
                if len(self.turn_data) == next_row:
                    next_row = 0
                self.turn_data[next_row][1] = new_to_angle
                return

        inside_new = []
        overlap_new_from = []
        overlap_new_to = []

        for i in range(self.GetLinesCount()):
            if i == row:
                continue
            from_angle = self.turn_data[i][1]
            is_in_from = self.IsInInterval(new_from_angle, new_to_angle, from_angle)

            to_angle = self.turn_data[i][2]
            is_in_to = self.IsInInterval(new_from_angle, new_to_angle, to_angle)

            if is_in_from and is_in_to:
                inside_new.append(i)
            if is_in_from:
                overlap_new_to.append(i)
            if is_in_to:
                overlap_new_from.append(i)

        for i_row in overlap_new_from:
            self.turn_data[i_row][2] = new_from_angle

        for i_row in overlap_new_to:
            self.turn_data[i_row][1] = new_to_angle

        for i_row in inside_new:
            if col == 1:
                angle = new_from_angle
            else:
                angle = new_to_angle

            self.turn_data[i_row][1] = angle
            self.turn_data[i_row][2] = angle

    def RemoveDataValidator(self, row):
        """Angle recalculation due to direction remove"""
        if row == 0:
            prev_row = self.GetLinesCount() - 1
        else:
            prev_row = row - 1

        remove_to_angle = self.turn_data[row][2]
        self.turn_data[prev_row][2] = remove_to_angle

    def IsInInterval(self, from_angle, to_angle, angle):
        """Test if a direction includes or not includes a value"""
        if to_angle < from_angle:
            to_angle = math.pi * 2 + to_angle
        if angle < from_angle:
            angle = math.pi * 2 + angle

        if angle > from_angle and angle < to_angle:
            return True
        return False
