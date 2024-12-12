"""
@package gmodeler.canvas

@brief wxGUI Graphical Modeler for creating, editing, and managing models

Classes:
 - canvas::ModelCanvas
 - canvas::ModelEvtHandler

(C) 2010-2023 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Python exports Ondrej Pesek <pesej.ondrek gmail.com>
"""

import wx
from wx.lib import ogl

from gui_core.dialogs import TextEntryDialog as CustomTextEntryDialog
from gui_core.wrap import TextEntryDialog as wxTextEntryDialog, NewId, Menu
from gui_core.forms import GUI
from core.gcmd import GException, GError

from gmodeler.model import (
    ModelRelation,
    ModelAction,
    ModelData,
    ModelLoop,
    ModelCondition,
    ModelComment,
)
from gmodeler.dialogs import (
    ModelRelationDialog,
    ModelDataDialog,
    ModelLoopDialog,
    ModelConditionDialog,
)
from gmodeler.giface import GraphicalModelerGrassInterface


class ModelCanvas(ogl.ShapeCanvas):
    """Canvas where model is drawn"""

    def __init__(self, parent, giface):
        self.parent = parent
        self._giface = giface
        ogl.OGLInitialize()
        ogl.ShapeCanvas.__init__(self, parent)

        self.diagram = ogl.Diagram()
        self.SetDiagram(self.diagram)
        self.diagram.SetCanvas(self)

        self.SetScrollbars(20, 20, 2000 // 20, 2000 // 20)

        self.Bind(wx.EVT_KEY_UP, self.OnKeyUp)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)

    def OnKeyUp(self, event):
        """Key pressed"""
        kc = event.GetKeyCode()
        if kc == wx.WXK_DELETE:
            self.RemoveSelected()

    def OnLeftDown(self, evt):
        self.SetFocus()
        evt.Skip()

    def RemoveSelected(self):
        """Remove selected shapes"""
        self.parent.ModelChanged()

        diagram = self.GetDiagram()
        shapes = [shape for shape in diagram.GetShapeList() if shape.Selected()]
        self.RemoveShapes(shapes)

    def RemoveShapes(self, shapes):
        """Removes shapes"""
        self.parent.ModelChanged()
        diagram = self.GetDiagram()
        for shape in shapes:
            remList, upList = self.parent.GetModel().RemoveItem(shape)
            shape.Select(False)
            diagram.RemoveShape(shape)
            shape.__del__()  # noqa: PLC2801, C2801
            for item in remList:
                diagram.RemoveShape(item)
                item.__del__()  # noqa: PLC2801, C2801

            for item in upList:
                item.Update()

        self.Refresh()

    def GetNewShapePos(self, yoffset=50):
        """Determine optimal position for newly added object

        :return: x,y
        """
        ymax = 20
        for item in self.GetDiagram().GetShapeList():
            y = item.GetY() + item.GetBoundingBoxMin()[1]
            ymax = max(y, ymax)

        return (self.GetSize()[0] // 2, ymax + yoffset)

    def GetShapesSelected(self):
        """Get list of selected shapes"""
        selected = []
        diagram = self.GetDiagram()
        for shape in diagram.GetShapeList():
            if shape.Selected():
                selected.append(shape)

        return selected


class ModelEvtHandler(ogl.ShapeEvtHandler):
    """Model event handler class"""

    def __init__(self, log, frame, giface):
        ogl.ShapeEvtHandler.__init__(self)
        self.log = log
        self.frame = frame
        self.x = self.y = None
        self._giface = giface

    def OnLeftClick(self, x, y, keys=0, attachment=0):
        """Left mouse button pressed -> select item & update statusbar"""
        shape = self.GetShape()

        # probably does nothing, removed from wxPython 2.9
        # canvas = shape.GetCanvas()
        # dc = wx.ClientDC(canvas)
        # canvas.PrepareDC(dc)

        if hasattr(self.frame, "defineRelation"):
            drel = self.frame.defineRelation
            if drel["from"] is None:
                drel["from"] = shape
            elif drel["to"] is None:
                drel["to"] = shape
                rel = ModelRelation(
                    parent=self.frame, fromShape=drel["from"], toShape=drel["to"]
                )
                dlg = ModelRelationDialog(parent=self.frame, shape=rel)
                if dlg.IsValid():
                    ret = dlg.ShowModal()
                    if ret == wx.ID_OK:
                        option = dlg.GetOption()
                        rel.SetName(option)
                        drel["from"].AddRelation(rel)
                        drel["to"].AddRelation(rel)
                        drel["from"].Update()
                        params = {
                            "params": [
                                {"name": option, "value": drel["from"].GetValue()}
                            ]
                        }
                        drel["to"].MergeParams(params)
                        self.frame.AddLine(rel)

                    dlg.Destroy()
                del self.frame.defineRelation

        # select object
        self._onSelectShape(shape, append=keys == 1)

        if hasattr(shape, "GetLog"):
            self.log.SetStatusText(shape.GetLog(), 0)
        else:
            self.log.SetStatusText("", 0)

    def OnLeftDoubleClick(self, x, y, keys=0, attachment=0):
        """Left mouse button pressed (double-click) -> show properties"""
        self.OnProperties()

    def OnProperties(self, event=None):
        """Show properties dialog"""
        self.frame.ModelChanged()
        shape = self.GetShape()
        if isinstance(shape, ModelAction):
            gmodule = GUI(
                parent=self.frame,
                show=True,
                giface=GraphicalModelerGrassInterface(
                    model=self.frame.GetModel(),
                    giface=self._giface,
                ),
            )
            gmodule.ParseCommand(
                shape.GetLog(string=False),
                completed=(self.frame.GetOptData, shape, shape.GetParams()),
            )

        elif isinstance(shape, ModelData):
            if shape.GetPrompt() in {
                "raster",
                "vector",
                "raster_3d",
                "stds",
                "strds",
                "stvds",
                "str3ds",
            }:
                dlg = ModelDataDialog(parent=self.frame, shape=shape)
                shape.SetPropDialog(dlg)
                dlg.CentreOnParent()
                dlg.Show()

        elif isinstance(shape, ModelLoop):
            dlg = ModelLoopDialog(parent=self.frame, shape=shape)
            dlg.CentreOnParent()
            if dlg.ShowModal() == wx.ID_OK:
                shape.SetLabel(dlg.GetCondition())
                model = self.frame.GetModel()
                ids = dlg.GetItems()
                alist = []
                for aId in ids["unchecked"]:
                    action = model.GetItem(aId, objType=ModelAction)
                    if action:
                        action.UnSetBlock(shape)
                for aId in ids["checked"]:
                    action = model.GetItem(aId, objType=ModelAction)
                    if action:
                        action.SetBlock(shape)
                        alist.append(aId)
                shape.SetItems(alist)
                self.frame.DefineLoop(shape)
                self.frame.SetStatusText(shape.GetLog(), 0)
            self.frame.GetCanvas().Refresh()

            dlg.Destroy()

        elif isinstance(shape, ModelCondition):
            dlg = ModelConditionDialog(parent=self.frame, shape=shape)
            dlg.CentreOnParent()
            if dlg.ShowModal() == wx.ID_OK:
                shape.SetLabel(dlg.GetCondition())
                model = self.frame.GetModel()
                ids = dlg.GetItems()
                for b in ids.keys():
                    alist = []
                    for aId in ids[b]["unchecked"]:
                        action = model.GetItem(aId, objType=ModelAction)
                        action.UnSetBlock(shape)
                    for aId in ids[b]["checked"]:
                        action = model.GetItem(aId, objType=ModelAction)
                        action.SetBlock(shape)
                        if action:
                            alist.append(aId)
                    shape.SetItems(alist, branch=b)
                self.frame.DefineCondition(shape)
            self.frame.GetCanvas().Refresh()

            dlg.Destroy()

    def OnBeginDragLeft(self, x, y, keys=0, attachment=0):
        """Drag shape (beginning)"""
        self.frame.ModelChanged()
        if self._previousHandler:
            self._previousHandler.OnBeginDragLeft(x, y, keys, attachment)

    def OnEndDragLeft(self, x, y, keys=0, attachment=0):
        """Drag shape (end)"""
        if self._previousHandler:
            self._previousHandler.OnEndDragLeft(x, y, keys, attachment)

        shape = self.GetShape()
        if isinstance(shape, ModelLoop):
            self.frame.DefineLoop(shape)
        elif isinstance(shape, ModelCondition):
            self.frame.DefineCondition(shape)

        for mo in shape.GetBlock():
            if isinstance(mo, ModelLoop):
                self.frame.DefineLoop(mo)
            elif isinstance(mo, ModelCondition):
                self.frame.DefineCondition(mo)

        shape = self.GetShape()
        canvas = shape.GetCanvas()
        canvas.Refresh()

    def OnEndSize(self, x, y):
        """Resize shape"""
        self.frame.ModelChanged()
        if self._previousHandler:
            self._previousHandler.OnEndSize(x, y)

    def OnRightClick(self, x, y, keys=0, attachment=0):
        """Right click -> pop-up menu"""
        if not hasattr(self, "popupID"):
            self.popupID = {}
            for key in (
                "remove",
                "enable",
                "addPoint",
                "delPoint",
                "intermediate",
                "display",
                "props",
                "id",
                "label",
                "comment",
            ):
                self.popupID[key] = NewId()

        # record coordinates
        self.x = x
        self.y = y

        # select object
        shape = self.GetShape()
        self._onSelectShape(shape)

        popupMenu = Menu()
        popupMenu.Append(self.popupID["remove"], _("Remove"))
        self.frame.Bind(wx.EVT_MENU, self.OnRemove, id=self.popupID["remove"])
        if isinstance(shape, (ModelAction, ModelLoop)):
            if shape.IsEnabled():
                popupMenu.Append(self.popupID["enable"], _("Disable"))
                self.frame.Bind(wx.EVT_MENU, self.OnDisable, id=self.popupID["enable"])
            else:
                popupMenu.Append(self.popupID["enable"], _("Enable"))
                self.frame.Bind(wx.EVT_MENU, self.OnEnable, id=self.popupID["enable"])
        if isinstance(shape, (ModelAction, ModelComment)):
            popupMenu.AppendSeparator()
            if isinstance(shape, ModelAction):
                popupMenu.Append(self.popupID["label"], _("Set label"))
                self.frame.Bind(wx.EVT_MENU, self.OnSetLabel, id=self.popupID["label"])

            popupMenu.Append(self.popupID["comment"], _("Set comment"))
            self.frame.Bind(wx.EVT_MENU, self.OnSetComment, id=self.popupID["comment"])

        if isinstance(shape, ModelRelation):
            popupMenu.AppendSeparator()
            popupMenu.Append(self.popupID["addPoint"], _("Add control point"))
            self.frame.Bind(wx.EVT_MENU, self.OnAddPoint, id=self.popupID["addPoint"])
            popupMenu.Append(self.popupID["delPoint"], _("Remove control point"))
            self.frame.Bind(
                wx.EVT_MENU, self.OnRemovePoint, id=self.popupID["delPoint"]
            )
            if len(shape.GetLineControlPoints()) == 2:
                popupMenu.Enable(self.popupID["delPoint"], False)

        if isinstance(shape, ModelData):
            popupMenu.AppendSeparator()
            if (
                "@" not in shape.GetValue()
                and len(self.GetShape().GetRelations("from")) > 0
            ):
                popupMenu.Append(
                    self.popupID["intermediate"], _("Intermediate"), kind=wx.ITEM_CHECK
                )
                if self.GetShape().IsIntermediate():
                    popupMenu.Check(self.popupID["intermediate"], True)

                self.frame.Bind(
                    wx.EVT_MENU, self.OnIntermediate, id=self.popupID["intermediate"]
                )

            if self.frame._giface.GetMapDisplay():
                popupMenu.Append(
                    self.popupID["display"], _("Display"), kind=wx.ITEM_CHECK
                )
                if self.GetShape().HasDisplay():
                    popupMenu.Check(self.popupID["display"], True)

                self.frame.Bind(
                    wx.EVT_MENU, self.OnHasDisplay, id=self.popupID["display"]
                )

                if self.GetShape().IsIntermediate():
                    popupMenu.Enable(self.popupID["display"], False)

        if isinstance(shape, (ModelData, ModelAction, ModelLoop)):
            popupMenu.AppendSeparator()
            popupMenu.Append(self.popupID["props"], _("Properties"))
            self.frame.Bind(wx.EVT_MENU, self.OnProperties, id=self.popupID["props"])

        self.frame.PopupMenu(popupMenu)
        popupMenu.Destroy()

    def OnDisable(self, event):
        """Disable action"""
        self._onEnable(False)

    def OnEnable(self, event):
        """Disable action"""
        self._onEnable(True)

    def _onEnable(self, enable):
        shape = self.GetShape()
        shape.Enable(enable)
        self.frame.ModelChanged()
        self.frame.canvas.Refresh()

    def OnSetLabel(self, event):
        shape = self.GetShape()
        dlg = wxTextEntryDialog(
            parent=self.frame,
            message=_("Label:"),
            caption=_("Set label"),
            value=shape.GetLabel(),
        )
        if dlg.ShowModal() == wx.ID_OK:
            label = dlg.GetValue()
            shape.SetLabel(label)
            self.frame.ModelChanged()
            self.frame.itemPanel.Update()
            self.frame.canvas.Refresh()
        dlg.Destroy()

    def OnSetComment(self, event):
        shape = self.GetShape()
        dlg = CustomTextEntryDialog(
            parent=self.frame,
            message=_("Comment:"),
            caption=_("Set comment"),
            defaultValue=shape.GetComment(),
            textStyle=wx.TE_MULTILINE,
            textSize=(300, 75),
        )
        if dlg.ShowModal() == wx.ID_OK:
            comment = dlg.GetValue()
            shape.SetComment(comment)
            self.frame.ModelChanged()
            self.frame.canvas.Refresh()
        dlg.Destroy()

    def _onSelectShape(self, shape, append=False):
        canvas = shape.GetCanvas()
        dc = wx.ClientDC(canvas)

        if shape.Selected():
            shape.Select(False, dc)
        else:
            shapeList = canvas.GetDiagram().GetShapeList()

            toUnselect = [s for s in shapeList if s.Selected()] if not append else []

            shape.Select(True, dc)

            for s in toUnselect:
                s.Select(False, dc)

        canvas.Refresh(False)

    def OnAddPoint(self, event):
        """Add control point"""
        shape = self.GetShape()
        shape.InsertLineControlPoint(point=wx.RealPoint(self.x, self.y))
        shape.ResetShapes()
        shape.Select(True)
        self.frame.ModelChanged()
        self.frame.canvas.Refresh()

    def OnRemovePoint(self, event):
        """Remove control point"""
        shape = self.GetShape()
        shape.DeleteLineControlPoint()
        shape.Select(False)
        shape.Select(True)
        self.frame.ModelChanged()
        self.frame.canvas.Refresh()

    def OnIntermediate(self, event):
        """Mark data as intermediate"""
        self.frame.ModelChanged()
        shape = self.GetShape()
        shape.SetIntermediate(event.IsChecked())
        self.frame.canvas.Refresh()

    def OnHasDisplay(self, event):
        """Mark data to be displayed"""
        self.frame.ModelChanged()
        shape = self.GetShape()
        shape.SetHasDisplay(event.IsChecked())
        self.frame.canvas.Refresh()

        try:
            if event.IsChecked():
                # add map layer to display
                self.frame._giface.GetLayerList().AddLayer(
                    ltype=shape.GetPrompt(),
                    name=shape.GetValue(),
                    checked=True,
                    cmd=shape.GetDisplayCmd(),
                )
            else:
                # remove map layer(s) from display
                layers = self.frame._giface.GetLayerList().GetLayersByName(
                    shape.GetValue()
                )
                for layer in layers:
                    self.frame._giface.GetLayerList().DeleteLayer(layer)

        except GException as e:
            GError(parent=self, message="{}".format(e))

    def OnRemove(self, event):
        """Remove shape"""
        self.frame.GetCanvas().RemoveShapes([self.GetShape()])
        self.frame.itemPanel.Update()
