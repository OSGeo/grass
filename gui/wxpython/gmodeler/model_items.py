"""
@package gmodeler.model_items

@brief wxGUI Graphical Modeler individual items classes

Classes:
 - model_items::ModelObject
 - model_items::ModelAction
 - model_items::ModelData
 - model_items::ModelDataSingle
 - model_items::ModelDataSeries
 - model_items::ModelRelation
 - model_items::ModelItem
 - model_items::ModelLoop
 - model_items::ModelCondition
 - model_items::ModelComment

(C) 2010-2025 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Ondrej Pesek <pesej.ondrek gmail.com>
"""

import copy
import re


import wx
from abc import abstractmethod
from wx.lib import ogl

from core.gcmd import (
    GException,
)
from core.settings import UserSettings
from gui_core.forms import GUI
from gui_core.wrap import IsDark


class ModelObject:
    def __init__(self, id=-1, label=""):
        self.id = id  # internal id, should be not changed
        self.label = ""
        self.rels = []  # list of ModelRelations

        self.isEnabled = True
        self.inBlock = []  # list of related loops/conditions

    def __del__(self):
        pass

    def GetLabel(self):
        """Get label"""
        return self.label

    def SetLabel(self, label=""):
        """Set label"""
        self.label = label

    def GetId(self):
        """Get id"""
        return self.id

    def SetId(self, newId):
        """Set id"""
        if self.inBlock:
            for loop in self.inBlock:
                # update block item
                loop.UpdateItem(self.id, newId)

        self.id = newId

    def AddRelation(self, rel):
        """Record new relation"""
        self.rels.append(rel)

    def GetRelations(self, fdir=None):
        """Get list of relations

        :param bool fdir: True for 'from'
        """
        if fdir is None:
            return self.rels

        result = []
        for rel in self.rels:
            if fdir == "from":
                if rel.GetFrom() == self:
                    result.append(rel)
            elif rel.GetTo() == self:
                result.append(rel)

        return result

    def IsEnabled(self):
        """Get True if action is enabled, otherwise False"""
        return self.isEnabled

    def Enable(self, enabled=True):
        """Enable/disable action"""
        self.isEnabled = enabled
        self.Update()

    def Update(self):
        pass

    def SetBlock(self, item):
        """Add object to the block (loop/condition)

        :param item: reference to ModelLoop or ModelCondition which
                     defines loops/condition
        """
        if item not in self.inBlock:
            self.inBlock.append(item)

    def UnSetBlock(self, item):
        """Remove object from the block (loop/condition)

        :param item: reference to ModelLoop or ModelCondition which
                     defines loops/condition
        """
        if item in self.inBlock:
            self.inBlock.remove(item)

    def GetBlock(self):
        """Get list of related ModelObject(s) which defines block
        (loop/condition)

        :return: list of ModelObjects
        """
        return self.inBlock

    def GetBlockId(self):
        """Get list of related ids which defines block

        :return: list of ids
        """
        return [mo.GetId() for mo in self.inBlock]


class ModelAction(ModelObject, ogl.DividedShape):
    """Action class (GRASS module)"""

    def __init__(
        self,
        parent,
        x,
        y,
        id=-1,
        cmd=None,
        task=None,
        width=None,
        height=None,
        label=None,
        comment="",
    ):
        ModelObject.__init__(self, id, label)

        self.parent = parent
        self.task = task
        self.comment = comment

        if not width:
            width = UserSettings.Get(
                group="modeler", key="action", subkey=("size", "width")
            )
        if not height:
            height = UserSettings.Get(
                group="modeler", key="action", subkey=("size", "height")
            )

        if cmd:
            self.task = GUI(show=None).ParseCommand(cmd=cmd)
        else:
            self.task = task or None

        self.propWin = None

        self.data = []  # list of connected data items

        self.isValid = False
        self.isParameterized = False

        if self.parent.GetCanvas():
            ogl.DividedShape.__init__(self, width, height)

            self.regionLabel = ogl.ShapeRegion()
            self.regionLabel.SetFormatMode(
                ogl.FORMAT_CENTRE_HORIZ | ogl.FORMAT_CENTRE_VERT
            )
            self.AddRegion(self.regionLabel)

            self.regionComment = None

            self.SetCanvas(self.parent)
            self.SetX(x)
            self.SetY(y)
            self._setPen()
            self._setBrush()
            self.SetLabel(label)
            if comment:
                self.SetComment(comment)

            self.SetRegionSizes()
            self.ReformatRegions()

        if self.task:
            self.SetValid(self.task.get_options())

    def _setBrush(self, running=False):
        """Set brush"""
        if running:
            color = UserSettings.Get(
                group="modeler", key="action", subkey=("color", "running")
            )
        elif not self.isEnabled:
            color = UserSettings.Get(group="modeler", key="disabled", subkey="color")
        elif self.isValid:
            color = UserSettings.Get(
                group="modeler", key="action", subkey=("color", "valid")
            )
        else:
            color = UserSettings.Get(
                group="modeler", key="action", subkey=("color", "invalid")
            )

        wxColor = wx.Colour(color[0], color[1], color[2])
        self.SetBrush(wx.Brush(wxColor))

    def _setPen(self):
        """Set pen"""
        if self.isParameterized:
            width = int(
                UserSettings.Get(
                    group="modeler", key="action", subkey=("width", "parameterized")
                )
            )
        else:
            width = int(
                UserSettings.Get(
                    group="modeler", key="action", subkey=("width", "default")
                )
            )
        style = wx.SOLID if self.isEnabled else wx.DOT

        pen = wx.Pen(wx.BLACK, width, style)
        self.SetPen(pen)

    def ReformatRegions(self):
        rnum = 0
        canvas = self.parent.GetCanvas()

        dc = wx.ClientDC(canvas)  # used for measuring

        for region in self.GetRegions():
            text = region.GetText()
            self.FormatText(dc, text, rnum)
            rnum += 1

    def OnSizingEndDragLeft(self, pt, x, y, keys, attch):
        ogl.DividedShape.OnSizingEndDragLeft(self, pt, x, y, keys, attch)
        self.SetRegionSizes()
        self.ReformatRegions()
        self.GetCanvas().Refresh()

    def SetLabel(self, label=None):
        """Set label

        :param label: if None use command string instead
        """
        if label:
            self.label = label
        elif self.label:
            label = self.label
        else:
            try:
                label = self.task.get_cmd(ignoreErrors=True)[0]
            except IndexError:
                label = _("unknown")

        idx = self.GetId()
        self.regionLabel.SetText("(%d) %s" % (idx, label))
        self.SetRegionSizes()
        self.ReformatRegions()

    def SetComment(self, comment):
        """Set comment"""
        self.comment = comment

        if self.regionComment is None:
            self.regionComment = ogl.ShapeRegion()
            self.regionComment.SetFormatMode(ogl.FORMAT_CENTRE_HORIZ)
            font = wx.SystemSettings.GetFont(wx.SYS_DEFAULT_GUI_FONT)
            font.SetStyle(wx.ITALIC)
            self.regionComment.SetFont(font)

        # clear doesn't work
        # self.regionComment.ClearText()
        self.regionComment.SetText(comment)

        self.ClearRegions()
        self.AddRegion(self.regionLabel)
        self.regionLabel.SetProportions(0.0, 1.0)

        if self.comment:
            self.AddRegion(self.regionComment)
            self.regionLabel.SetProportions(0.0, 0.4)

        self.SetRegionSizes()
        self.ReformatRegions()

    def GetComment(self):
        """Get comment"""
        return self.comment

    def SetProperties(self, params, propwin):
        """Record properties dialog"""
        self.task.params = params["params"]
        self.task.flags = params["flags"]
        self.propWin = propwin

    def GetPropDialog(self):
        """Get properties dialog"""
        return self.propWin

    def GetLog(self, string=True, substitute=None):
        """Get logging info

        :param string: True to get cmd as a string otherwise a list
        :param substitute: dictionary of parameter to substitute or None
        """
        cmd = self.task.get_cmd(
            ignoreErrors=True, ignoreRequired=True, ignoreDefault=False
        )

        # substitute variables
        if substitute:
            variables = []
            if "variables" in substitute:
                for p in substitute["variables"]["params"]:
                    variables.append(p.get("name", ""))
            else:
                variables = self.parent.GetVariables()

            # order variables by length
            for variable in sorted(variables, key=len, reverse=True):
                pattern = re.compile("%{" + variable + "}")
                value = ""
                if substitute and "variables" in substitute:
                    for p in substitute["variables"]["params"]:
                        if variable == p.get("name", ""):
                            if p.get("type", "string") == "string":
                                value = p.get("value", "")
                            else:
                                value = str(p.get("value", ""))
                            break

                if not value:
                    value = variables[variable].get("value", "")

                if not value:
                    continue

                for idx in range(len(cmd)):
                    if pattern.search(cmd[idx]):
                        cmd[idx] = pattern.sub(value, cmd[idx])
                    idx += 1

        if not string:
            return cmd
        if cmd is None:
            return ""
        return " ".join(cmd)

    def GetLabel(self):
        """Get name"""
        if self.label:
            return self.label

        cmd = self.task.get_cmd(ignoreErrors=True)
        if cmd and len(cmd) > 0:
            return cmd[0]

        return _("unknown")

    def GetParams(self, dcopy=False):
        """Get dictionary of parameters"""
        if dcopy:
            return copy.deepcopy(self.task.get_options())

        return self.task.get_options()

    def GetTask(self):
        """Get grassTask instance"""
        return self.task

    def SetParams(self, params):
        """Set dictionary of parameters"""
        self.task.params = params["params"]
        self.task.flags = params["flags"]

    def MergeParams(self, params):
        """Merge dictionary of parameters"""
        if "flags" in params:
            for f in params["flags"]:
                self.task.set_flag(f["name"], f.get("value", False))
        if "params" in params:
            for p in params["params"]:
                self.task.set_param(p["name"], p.get("value", ""))

    def SetValid(self, options):
        """Set validity for action

        :param options: dictionary with flags and params (gtask)
        """
        self.isValid = True

        for f in options["flags"]:
            if f.get("parameterized", False):
                self.isParameterized = True
                break

        for p in options["params"]:
            if (
                self.isValid
                and p.get("required", False)
                and p.get("value", "") == ""
                and p.get("default", "") == ""
            ):
                self.isValid = False
            if not self.isParameterized and p.get("parameterized", False):
                self.isParameterized = True

        if self.parent.GetCanvas():
            self._setBrush()
            self._setPen()

    def IsValid(self):
        """Check validity (all required parameters set)"""
        return self.isValid

    def IsParameterized(self):
        """Check if action is parameterized"""
        return self.isParameterized

    def GetParameterizedParams(self):
        """Return parameterized flags and options"""
        param = {"flags": [], "params": []}

        options = self.GetParams()

        for f in options["flags"]:
            if f.get("parameterized", False):
                param["flags"].append(f)

        for p in options["params"]:
            if p.get("parameterized", False):
                param["params"].append(p)

        return param

    def FindData(self, name):
        """Find data item by name"""
        for rel in self.GetRelations():
            data = rel.GetData()
            if name == rel.GetLabel() and name in data.GetLabel():
                return data

        return None

    def Update(self, running=False):
        """Update action"""
        if running:
            self._setBrush(running=True)
        else:
            self._setBrush()
        self._setPen()

    def OnDraw(self, dc):
        """Draw action in canvas"""
        self._setBrush()
        self._setPen()
        ogl.RectangleShape.Recentre(self, dc)  # re-center text
        ogl.RectangleShape.OnDraw(self, dc)


class ModelData(ModelObject):
    def __init__(self, parent, x, y, value="", prompt="", width=None, height=None):
        """Data item class

        :param parent: window parent
        :param x, y: position of the shape
        :param fname, tname: list of parameter names from / to
        :param value: value
        :param prompt: type of GIS element
        :param width, height: dimension of the shape
        """
        ModelObject.__init__(self)

        self.parent = parent
        self.value = value
        self.prompt = prompt
        self.intermediate = False
        self.display = False
        self.propWin = None
        if not width:
            width = UserSettings.Get(
                group="modeler", key="data", subkey=("size", "width")
            )
        if not height:
            height = UserSettings.Get(
                group="modeler", key="data", subkey=("size", "height")
            )

        self._defineShape(width, height, x, y)

        self._setPen()
        self._setBrush()
        self.SetLabel()

    @abstractmethod
    def _defineShape(self, width, height, x, y):
        """Define data item

        :param width, height: dimension of the shape
        :param x, y: position of the shape
        """

    def IsIntermediate(self):
        """Checks if data item is intermediate"""
        return self.intermediate

    def SetIntermediate(self, im):
        """Set intermediate flag"""
        self.intermediate = im

    def HasDisplay(self):
        """Checks if data item is marked to be displayed"""
        return self.display

    def SetHasDisplay(self, tbd):
        """Set to-be-displayed flag"""
        self.display = tbd

    def OnDraw(self, dc):
        self._setPen()

        super().OnDraw(dc)

    def GetLog(self, string=True):
        """Get logging info"""
        name = [rel.GetLabel() for rel in self.GetRelations()]
        if name:
            return "/".join(name) + "=" + self.value + " (" + self.prompt + ")"
        return self.value + " (" + self.prompt + ")"

    def GetLabel(self):
        """Get list of names"""
        return [rel.GetLabel() for rel in self.GetRelations()]

    def GetPrompt(self):
        """Get prompt"""
        return self.prompt

    def SetPrompt(self, prompt):
        """Set prompt

        :param prompt:
        """
        self.prompt = prompt

    def GetValue(self):
        """Get value"""
        return self.value

    def SetValue(self, value):
        """Set value

        :param value:
        """
        self.value = value
        self.SetLabel()
        for direction in ("from", "to"):
            for rel in self.GetRelations(direction):
                action = rel.GetTo() if direction == "from" else rel.GetFrom()

                task = GUI(show=None).ParseCommand(cmd=action.GetLog(string=False))
                task.set_param(rel.GetLabel(), self.value)
                action.MergeParams(task.get_options())

    def GetPropDialog(self):
        """Get properties dialog"""
        return self.propWin

    def SetPropDialog(self, win):
        """Get properties dialog"""
        self.propWin = win

    def _getBrush(self):
        """Get brush"""
        if self.prompt in {"raster", "strds"}:
            color = UserSettings.Get(
                group="modeler", key="data", subkey=("color", "raster")
            )
        elif self.prompt in {"raster_3d", "str3ds"}:
            color = UserSettings.Get(
                group="modeler", key="data", subkey=("color", "raster3d")
            )
        elif self.prompt in {"vector", "stvds"}:
            color = UserSettings.Get(
                group="modeler", key="data", subkey=("color", "vector")
            )
        elif self.prompt == "dbtable":
            color = UserSettings.Get(
                group="modeler", key="data", subkey=("color", "dbtable")
            )
        else:
            color = UserSettings.Get(
                group="modeler", key="action", subkey=("color", "invalid")
            )
        wxColor = wx.Colour(color[0], color[1], color[2])

        return wx.Brush(wxColor)

    def _setBrush(self):
        """Set brush"""
        self.SetBrush(self._getBrush())

    def _getPen(self):
        """Get pen"""
        isParameterized = False
        for rel in self.GetRelations("from"):
            if rel.GetTo().IsParameterized():
                isParameterized = True
                break
        if not isParameterized:
            for rel in self.GetRelations("to"):
                if rel.GetFrom().IsParameterized():
                    isParameterized = True
                    break

        if isParameterized:
            width = int(
                UserSettings.Get(
                    group="modeler", key="action", subkey=("width", "parameterized")
                )
            )
        else:
            width = int(
                UserSettings.Get(
                    group="modeler", key="action", subkey=("width", "default")
                )
            )
        style = wx.DOT if self.intermediate else wx.SOLID

        return wx.Pen(wx.BLACK, width, style)

    def _setPen(self):
        """Get pen"""
        self.SetPen(self._getPen())

    def SetLabel(self):
        """Update text"""
        self.ClearText()
        name = [rel.GetLabel() for rel in self.GetRelations()]
        self.AddText("/".join(name))
        if self.value:
            self.AddText(self.value)
        else:
            self.AddText(_("<not defined>"))

    def Update(self):
        """Update action"""
        self._setBrush()
        self._setPen()
        self.SetLabel()

    def GetDisplayCmd(self):
        """Get display command as list"""
        cmd = []
        if self.prompt == "raster":
            cmd.append("d.rast")
        elif self.prompt == "vector":
            cmd.append("d.vect")
        else:
            msg = "Unsupported display prompt: {}".format(self.prompt)
            raise GException(msg)

        cmd.append("map=" + self.value)

        return cmd


class ModelDataSingle(ModelData, ogl.EllipseShape):
    def _defineShape(self, width, height, x, y):
        """Define single data item (raster, raster_3d, vector)

        :param width, height: dimension of the shape
        :param x, y: position of the shape
        """
        ogl.EllipseShape.__init__(self, width, height)  # noqa: PLC2801, C2801
        if self.parent.GetCanvas():
            self.SetCanvas(self.parent.GetCanvas())

        self.SetX(x)
        self.SetY(y)


class ModelDataSeries(ModelData, ogl.CompositeShape):
    def _defineShape(self, width, height, x, y):
        """Define single data item (raster, raster_3d, vector)

        :param width, height: dimension of the shape
        :param x, y: position of the shape
        """
        ogl.CompositeShape.__init__(self)  # noqa: PLC2801, C2801
        if self.parent.GetCanvas():
            self.SetCanvas(self.parent.GetCanvas())

        self.constraining_shape = ogl.EllipseShape(width, height)
        self.constrained_shape = ogl.EllipseShape(width - 20, height)
        self.AddChild(self.constraining_shape)
        self.AddChild(self.constrained_shape)

        constraint = ogl.Constraint(
            ogl.CONSTRAINT_CENTRED_BOTH,
            self.constraining_shape,
            [self.constrained_shape],
        )
        self.AddConstraint(constraint)
        self.Recompute()

        self.constraining_shape.SetDraggable(False)
        self.constrained_shape.SetDraggable(False)
        self.constrained_shape.SetSensitivityFilter(ogl.OP_CLICK_LEFT)

        canvas = self.parent.GetCanvas()
        if canvas:
            dc = wx.ClientDC(canvas)
            canvas.PrepareDC(dc)
            self.Move(dc, x, y)

        self.SetX(x)
        self.SetY(y)

    def _setBrush(self):
        """Set brush"""
        brush = self._getBrush()
        self.constraining_shape.SetBrush(brush)
        self.constrained_shape.SetBrush(brush)

    def _setPen(self):
        """Set brush"""
        brush = self._getPen()
        self.constraining_shape.SetPen(brush)
        self.constrained_shape.SetPen(brush)


class ModelRelation(ogl.LineShape):
    """Data - action relation"""

    def __init__(self, parent, fromShape, toShape, param=""):
        self.fromShape = fromShape
        self.toShape = toShape
        self.param = param
        self.parent = parent

        self._points = None

        if self.parent.GetCanvas():
            ogl.LineShape.__init__(self)

    def __del__(self):
        if self in self.fromShape.rels:
            self.fromShape.rels.remove(self)
        if self in self.toShape.rels:
            self.toShape.rels.remove(self)

    def GetFrom(self):
        """Get id of 'from' shape"""
        return self.fromShape

    def GetTo(self):
        """Get id of 'to' shape"""
        return self.toShape

    def GetData(self):
        """Get related ModelData instance

        :return: ModelData instance
        :return: None if not found
        """
        if isinstance(self.fromShape, ModelData):
            return self.fromShape
        if isinstance(self.toShape, ModelData):
            return self.toShape

        return None

    def GetLabel(self):
        """Get parameter name"""
        return self.param

    def ResetShapes(self):
        """Reset related objects"""
        self.fromShape.ResetControlPoints()
        self.toShape.ResetControlPoints()
        self.ResetControlPoints()

    def SetControlPoints(self, points):
        """Set control points"""
        self._points = points

    def GetControlPoints(self):
        """Get list of control points"""
        return self._points

    def _setPen(self, bg_white=False):
        """Set pen"""
        self.SetPen(
            wx.Pen(wx.WHITE if IsDark() and not bg_white else wx.BLACK, 1, wx.SOLID)
        )

    def OnDraw(self, dc):
        """Draw relation"""
        self._setPen(dc.GetBackground() == wx.WHITE_BRUSH)
        ogl.LineShape.OnDraw(self, dc)

    def SetName(self, param):
        self.param = param


class ModelItem(ModelObject):
    def __init__(
        self, parent, x, y, id=-1, width=None, height=None, label="", items=[]
    ):
        """Abstract class for loops and conditions"""
        ModelObject.__init__(self, id, label)
        self.parent = parent

    def _setPen(self):
        """Set pen"""
        style = wx.SOLID if self.isEnabled else wx.DOT

        pen = wx.Pen(wx.BLACK, 1, style)
        self.SetPen(pen)

    def SetId(self, id):
        """Set loop id"""
        self.id = id

    def SetLabel(self, label=""):
        """Set loop text (condition)"""
        if label:
            self.label = label
        self.ClearText()
        self.AddText("(" + str(self.id) + ") " + self.label)

    def GetLog(self):
        """Get log info"""
        if self.label:
            return _("Condition: ") + self.label
        return _("Condition: not defined")

    def AddRelation(self, rel):
        """Record relation"""
        self.rels.append(rel)

    def Clear(self):
        """Clear object, remove rels"""
        self.rels = []


class ModelLoop(ModelItem, ogl.RectangleShape):
    def __init__(
        self, parent, x, y, id=-1, idx=-1, width=None, height=None, label="", items=[]
    ):
        """Defines a loop"""
        ModelItem.__init__(self, parent, x, y, id, width, height, label, items)
        self.itemIds = []  # unordered

        if not width:
            width = UserSettings.Get(
                group="modeler", key="loop", subkey=("size", "width")
            )
        if not height:
            height = UserSettings.Get(
                group="modeler", key="loop", subkey=("size", "height")
            )

        if self.parent.GetCanvas():
            ogl.RectangleShape.__init__(self, width, height)

            self.SetCanvas(self.parent)
            self.SetX(x)
            self.SetY(y)
            self._setPen()
            self._setBrush()
            self.SetCornerRadius(100)
            self.SetLabel(label)

    def _setBrush(self):
        """Set brush"""
        if not self.isEnabled:
            color = UserSettings.Get(group="modeler", key="disabled", subkey="color")
        else:
            color = UserSettings.Get(
                group="modeler", key="loop", subkey=("color", "valid")
            )

        wxColor = wx.Colour(color[0], color[1], color[2])
        self.SetBrush(wx.Brush(wxColor))

    def Enable(self, enabled=True):
        """Enable/disable action"""
        for idx in self.itemIds:
            item = self.parent.FindAction(idx)
            if item:
                item.Enable(enabled)

        ModelObject.Enable(self, enabled)
        self.Update()

    def Update(self):
        self._setPen()
        self._setBrush()

    def GetItems(self, items):
        """Get sorted items by id"""
        return [item for item in items if item.GetId() in self.itemIds]

    def SetItems(self, items):
        """Set items (id)"""
        self.itemIds = items

    def UpdateItem(self, oldId, newId):
        """Update item in the list"""
        idx = self.itemIds.index(oldId)
        if idx != -1:
            self.itemIds[idx] = newId

    def OnDraw(self, dc):
        """Draw loop in canvas"""
        self._setBrush()
        ogl.RectangleShape.Recentre(self, dc)  # re-center text
        ogl.RectangleShape.OnDraw(self, dc)


class ModelCondition(ModelItem, ogl.PolygonShape):
    def __init__(
        self,
        parent,
        x,
        y,
        id=-1,
        width=None,
        height=None,
        label="",
        items={"if": [], "else": []},
    ):
        """Defines an if-else condition"""
        ModelItem.__init__(self, parent, x, y, id, width, height, label, items)
        self.itemIds = {"if": [], "else": []}

        if not width:
            self.width = UserSettings.Get(
                group="modeler", key="if-else", subkey=("size", "width")
            )
        else:
            self.width = width
        if not height:
            self.height = UserSettings.Get(
                group="modeler", key="if-else", subkey=("size", "height")
            )
        else:
            self.height = height

        if self.parent.GetCanvas():
            ogl.PolygonShape.__init__(self)

            points = [
                (0, -self.height / 2),
                (self.width / 2, 0),
                (0, self.height / 2),
                (-self.width / 2, 0),
            ]
            self.Create(points)

            self.SetCanvas(self.parent)
            self.SetX(x)
            self.SetY(y)
            self.SetPen(wx.BLACK_PEN)
            if label:
                self.AddText("(" + str(self.id) + ") " + label)
            else:
                self.AddText("(" + str(self.id) + ")")

    def GetLabel(self):
        """Get name"""
        return _("if-else")

    def GetWidth(self):
        """Get object width"""
        return self.width

    def GetHeight(self):
        """Get object height"""
        return self.height

    def GetItems(self, items):
        """Get sorted items by id"""
        result = {"if": [], "else": []}
        for item in items:
            if item.GetId() in self.itemIds["if"]:
                result["if"].append(item)
            elif item.GetId() in self.itemIds["else"]:
                result["else"].append(item)

        return result

    def SetItems(self, items, branch="if"):
        """Set items (id)

        :param items: list of items
        :param branch: 'if' / 'else'
        """
        if branch in {"if", "else"}:
            self.itemIds[branch] = items


class ModelComment(ModelObject, ogl.RectangleShape):
    def __init__(self, parent, x, y, id=-1, width=None, height=None, label=""):
        """Defines a model comment"""
        ModelObject.__init__(self, id, label)

        if not width:
            width = UserSettings.Get(
                group="modeler", key="comment", subkey=("size", "width")
            )
        if not height:
            height = UserSettings.Get(
                group="modeler", key="comment", subkey=("size", "height")
            )

        if parent.GetCanvas():
            ogl.RectangleShape.__init__(self, width, height)
            self.SetCanvas(parent)
            self.SetX(x)
            self.SetY(y)
            font = wx.SystemSettings.GetFont(wx.SYS_DEFAULT_GUI_FONT)
            font.SetStyle(wx.ITALIC)
            self.SetFont(font)
            self._setPen()
            self._setBrush()
            self.SetLabel(label)

    def _setBrush(self, running=False):
        """Set brush"""
        color = UserSettings.Get(group="modeler", key="comment", subkey="color")
        wxColor = wx.Colour(color[0], color[1], color[2])
        self.SetBrush(wx.Brush(wxColor))

    def _setPen(self):
        """Set pen"""
        pen = wx.Pen(wx.BLACK, 1, wx.DOT)
        self.SetPen(pen)

    def SetLabel(self, label=None):
        """Set label

        :param label: if None use command string instead
        """
        if label:
            self.label = label
        elif self.label:
            label = self.label
        else:
            label = ""
        idx = self.GetId()

        self.ClearText()
        self.AddText("(%d) %s" % (idx, label))

    def GetComment(self):
        return self.GetLabel()

    def SetComment(self, comment):
        self.SetLabel(comment)
        self.GetCanvas().Refresh()
