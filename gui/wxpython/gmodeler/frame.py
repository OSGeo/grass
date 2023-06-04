"""
@package gmodeler.frame

@brief wxGUI Graphical Modeler for creating, editing, and managing models

Classes:
 - frame::ModelerFrame

(C) 2010-2023 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Python exports Ondrej Pesek <pesej.ondrek gmail.com>
"""

import os
import sys

import wx

from core import globalvar
from gui_core.menu import Menu as Menubar

from gmodeler.menudata import ModelerMenuData
from gmodeler.toolbars import ModelerToolbar
from gmodeler.panel import ModelerPanel

class ModelerFrame(wx.Frame):
    def __init__(
        self, parent, giface, id=wx.ID_ANY, title=_("Graphical Modeler"), **kwargs
    ):
        """Graphical modeler main window
        :param parent: parent window
        :param giface: GRASS interface
        :param id: window id
        :param title: window title

        :param kwargs: wx.Frames' arguments
        """
        wx.Frame.__init__(self, parent=parent, id=id, title=title, **kwargs)

        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )

        self.menubar = Menubar(
            parent=self, model=ModelerMenuData().GetModel(separators=True)
        )
        self.SetMenuBar(self.menubar)

        self.toolbar = ModelerToolbar(parent=self)
        # workaround for http://trac.wxwidgets.org/ticket/13888
        if sys.platform != "darwin":
            self.SetToolBar(self.toolbar)

        self.statusbar = self.CreateStatusBar(number=1)
        
        self.Panel = ModelerPanel(parent=self, giface=giface, statusbar=self.statusbar)
        self.SetMinSize((640, 300))
        self.SetSize((800, 600))

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

        # TODO
        self.modelChanged = None
        
    def OnModelNew(self, event):
        """Create new model"""
        Debug.msg(4, "ModelFrame.OnModelNew():")

        # ask user to save current model
        if self.modelFile and self.modelChanged:
            self.OnModelSave()
        elif self.modelFile is None and (
            self.model.GetNumItems() > 0 or len(self.model.GetData()) > 0
        ):
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Current model is not empty. "
                    "Do you want to store current settings "
                    "to model file?"
                ),
                caption=_("Create new model?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.CANCEL | wx.ICON_QUESTION,
            )
            ret = dlg.ShowModal()
            if ret == wx.ID_YES:
                self.OnModelSaveAs()
            elif ret == wx.ID_CANCEL:
                dlg.Destroy()
                return

            dlg.Destroy()

        # delete all items
        self.canvas.GetDiagram().DeleteAllShapes()
        self.model.Reset()
        self.canvas.Refresh()
        self.itemPanel.Update()
        self.variablePanel.Reset()

        # no model file loaded
        self.modelFile = None
        # TODO
        # self.modelChanged = False
        self.SetTitle(self.baseTitle)

    def OnModelOpen(self, event):
        """Load model from file"""
        filename = ""
        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose model file"),
            defaultDir=os.getcwd(),
            wildcard=_("GRASS Model File (*.gxm)|*.gxm"),
        )
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if not filename:
            return

        Debug.msg(4, "ModelFrame.OnModelOpen(): filename=%s" % filename)

        # close current model
        self.OnModelClose()

        self.LoadModelFile(filename)

        self.modelFile = filename
        self.SetTitle(self.baseTitle + " - " + os.path.basename(self.modelFile))
        self.SetStatusText(
            _("%(items)d items (%(actions)d actions) loaded into model")
            % {
                "items": self.model.GetNumItems(),
                "actions": self.model.GetNumItems(actionOnly=True),
            },
            0,
        )

    def OnModelSave(self, event=None):
        """Save model to file"""
        if self.modelFile and self.modelChanged:
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Model file <%s> already exists. "
                    "Do you want to overwrite this file?"
                )
                % self.modelFile,
                caption=_("Save model"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
            )
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
            else:
                Debug.msg(4, "ModelFrame.OnModelSave(): filename=%s" % self.modelFile)
                self.WriteModelFile(self.modelFile)
                self.SetStatusText(_("File <%s> saved") % self.modelFile, 0)
                self.SetTitle(self.baseTitle + " - " + os.path.basename(self.modelFile))
        elif not self.modelFile:
            self.OnModelSaveAs(None)

    def OnModelSaveAs(self, event):
        """Create model to file as"""
        filename = ""
        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose file to save current model"),
            defaultDir=os.getcwd(),
            wildcard=_("GRASS Model File (*.gxm)|*.gxm"),
            style=wx.FD_SAVE,
        )

        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if not filename:
            return

        # check for extension
        if filename[-4:] != ".gxm":
            filename += ".gxm"

        if os.path.exists(filename):
            dlg = wx.MessageDialog(
                parent=self,
                message=_(
                    "Model file <%s> already exists. "
                    "Do you want to overwrite this file?"
                )
                % filename,
                caption=_("File already exists"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
            )
            if dlg.ShowModal() != wx.ID_YES:
                dlg.Destroy()
                return

        Debug.msg(4, "GMFrame.OnModelSaveAs(): filename=%s" % filename)

        self.WriteModelFile(filename)
        self.modelFile = filename
        self.SetTitle(self.baseTitle + " - " + os.path.basename(self.modelFile))
        self.SetStatusText(_("File <%s> saved") % self.modelFile, 0)

    def OnModelClose(self, event=None):
        """Close model file"""
        Debug.msg(4, "ModelFrame.OnModelClose(): file=%s" % self.modelFile)
        # ask user to save current model
        if self.modelFile and self.modelChanged:
            self.OnModelSave()
        elif self.modelFile is None and (
            self.model.GetNumItems() > 0 or len(self.model.GetData()) > 0
        ):
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Current model is not empty. "
                    "Do you want to store current settings "
                    "to model file?"
                ),
                caption=_("Create new model?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.CANCEL | wx.ICON_QUESTION,
            )
            ret = dlg.ShowModal()
            if ret == wx.ID_YES:
                self.OnModelSaveAs()
            elif ret == wx.ID_CANCEL:
                dlg.Destroy()
                return

            dlg.Destroy()

        self.modelFile = None
        self.SetTitle(self.baseTitle)

        self.canvas.GetDiagram().DeleteAllShapes()
        self.model.Reset()

        self.canvas.Refresh()

    def OnRunModel(self, event):
        """Run entire model"""
        self.start_time = time.time()
        self.model.Run(self._gconsole, self.OnModelDone, parent=self)

    def OnExportImage(self, event):
        """Export model to image (default image)"""
        xminImg = 0
        xmaxImg = 0
        yminImg = 0
        ymaxImg = 0
        # get current size of canvas
        for shape in self.canvas.GetDiagram().GetShapeList():
            w, h = shape.GetBoundingBoxMax()
            x = shape.GetX()
            y = shape.GetY()
            xmin = x - w / 2
            xmax = x + w / 2
            ymin = y - h / 2
            ymax = y + h / 2
            if xmin < xminImg:
                xminImg = xmin
            if xmax > xmaxImg:
                xmaxImg = xmax
            if ymin < yminImg:
                yminImg = ymin
            if ymax > ymaxImg:
                ymaxImg = ymax
        size = wx.Size(int(xmaxImg - xminImg) + 50, int(ymaxImg - yminImg) + 50)
        bitmap = EmptyBitmap(width=size.width, height=size.height)

        filetype, ltype = GetImageHandlers(ImageFromBitmap(bitmap))

        dlg = wx.FileDialog(
            parent=self,
            message=_(
                "Choose a file name to save the image (no need to add extension)"
            ),
            defaultDir="",
            defaultFile="",
            wildcard=filetype,
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT,
        )

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return

            base, ext = os.path.splitext(path)
            fileType = ltype[dlg.GetFilterIndex()]["type"]
            extType = ltype[dlg.GetFilterIndex()]["ext"]
            if ext != extType:
                path = base + "." + extType

            dc = wx.MemoryDC(bitmap)
            dc.SetBackground(wx.WHITE_BRUSH)
            dc.SetBackgroundMode(wx.SOLID)

            self.canvas.GetDiagram().Clear(dc)
            self.canvas.GetDiagram().Redraw(dc)

            bitmap.SaveFile(path, fileType)
            self.SetStatusText(_("Model exported to <%s>") % path)

        dlg.Destroy()

    def OnExportPython(self, event=None, text=None):
        """Export model to Python script"""
        filename = self.pythonPanel.SaveAs(force=True)
        self.SetStatusText(_("Model exported to <%s>") % filename)

    def OnCloseWindow(self, event):
        """Close window"""
        if self.modelChanged and UserSettings.Get(
            group="manager", key="askOnQuit", subkey="enabled"
        ):
            if self.modelFile:
                message = _("Do you want to save changes in the model?")
            else:
                message = _(
                    "Do you want to store current model settings " "to model file?"
                )

            # ask user to save current settings
            dlg = wx.MessageDialog(
                self,
                message=message,
                caption=_("Quit Graphical Modeler"),
                style=wx.YES_NO
                | wx.YES_DEFAULT
                | wx.CANCEL
                | wx.ICON_QUESTION
                | wx.CENTRE,
            )
            ret = dlg.ShowModal()
            if ret == wx.ID_YES:
                if not self.modelFile:
                    self.OnWorkspaceSaveAs()
                else:
                    self.WriteModelFile(self.modelFile)
            elif ret == wx.ID_CANCEL:
                dlg.Destroy()
                return
            dlg.Destroy()

        self.Destroy()

    def OnPreferences(self, event):
        """Open preferences dialog"""
        dlg = PreferencesDialog(parent=self, giface=self._giface)
        dlg.CenterOnParent()

        dlg.Show()
        self.canvas.Refresh()

    def OnAddAction(self, event):
        """Add action to model"""
        if self.searchDialog is None:
            self.searchDialog = ModelSearchDialog(parent=self, giface=self._giface)
            self.searchDialog.CentreOnParent()
        else:
            self.searchDialog.Reset()

        if self.searchDialog.ShowModal() == wx.ID_CANCEL:
            self.searchDialog.Hide()
            return

        cmd = self.searchDialog.GetCmd()
        self.searchDialog.Hide()

        self.ModelChanged()

        # add action to canvas
        x, y = self.canvas.GetNewShapePos()
        label, comment = self.searchDialog.GetLabel()
        action = ModelAction(
            self.model,
            cmd=cmd,
            x=x + self._randomShift(),
            y=y + self._randomShift(),
            id=self.model.GetNextId(),
            label=label,
            comment=comment,
        )
        overwrite = self.model.GetProperties().get("overwrite", None)
        if overwrite is not None:
            action.GetTask().set_flag("overwrite", overwrite)

        self.canvas.diagram.AddShape(action)
        action.Show(True)

        self._addEvent(action)
        self.model.AddItem(action)

        self.itemPanel.Update()
        self.canvas.Refresh()
        time.sleep(0.1)

        # show properties dialog
        win = action.GetPropDialog()
        if not win:
            cmdLength = len(action.GetLog(string=False))
            if cmdLength > 1 and action.IsValid():
                self.GetOptData(
                    dcmd=action.GetLog(string=False),
                    layer=action,
                    params=action.GetParams(),
                    propwin=None,
                )
            else:
                gmodule = GUI(
                    parent=self,
                    show=True,
                    giface=GraphicalModelerGrassInterface(self.model),
                )
                gmodule.ParseCommand(
                    action.GetLog(string=False),
                    completed=(self.GetOptData, action, action.GetParams()),
                )
        elif win and not win.IsShown():
            win.Show()

        if win:
            win.Raise()

    def OnAddData(self, event):
        """Add data item to model"""
        # add action to canvas
        width, height = self.canvas.GetSize()
        data = ModelData(
            self, x=width / 2 + self._randomShift(), y=height / 2 + self._randomShift()
        )

        dlg = ModelDataDialog(parent=self, shape=data)
        data.SetPropDialog(dlg)
        dlg.CentreOnParent()
        ret = dlg.ShowModal()
        dlg.Destroy()
        if ret != wx.ID_OK:
            return

        data.Update()
        self.canvas.diagram.AddShape(data)
        data.Show(True)

        self.ModelChanged()

        self._addEvent(data)
        self.model.AddItem(data)

        self.canvas.Refresh()

    def OnAddComment(self, event):
        """Add comment to the model"""
        dlg = CustomTextEntryDialog(
            parent=self,
            message=_("Comment:"),
            caption=_("Add comment"),
            textStyle=wx.TE_MULTILINE,
            textSize=(300, 75),
        )

        if dlg.ShowModal() == wx.ID_OK:
            comment = dlg.GetValue()
            if not comment:
                GError(_("Empty comment. Nothing to add to the model."), parent=self)
            else:
                x, y = self.canvas.GetNewShapePos()
                commentObj = ModelComment(
                    self.model,
                    x=x + self._randomShift(),
                    y=y + self._randomShift(),
                    id=self.model.GetNextId(),
                    label=comment,
                )
                self.canvas.diagram.AddShape(commentObj)
                commentObj.Show(True)
                self._addEvent(commentObj)
                self.model.AddItem(commentObj)

                self.canvas.Refresh()
                self.ModelChanged()

        dlg.Destroy()

    def OnDefineRelation(self, event):
        """Define relation between data and action items"""
        self.canvas.SetCursor(self.cursors["cross"])
        self.defineRelation = {"from": None, "to": None}

    def OnDefineLoop(self, event):
        """Define new loop in the model

        .. todo::
            move to ModelCanvas?
        """
        self.ModelChanged()

        width, height = self.canvas.GetSize()
        loop = ModelLoop(
            self, x=width / 2, y=height / 2, id=self.model.GetNumItems() + 1
        )
        self.canvas.diagram.AddShape(loop)
        loop.Show(True)

        self._addEvent(loop)
        self.model.AddItem(loop)

        self.canvas.Refresh()

    def OnDefineCondition(self, event):
        """Define new condition in the model

        .. todo::
            move to ModelCanvas?
        """
        self.ModelChanged()

        width, height = self.canvas.GetSize()
        cond = ModelCondition(
            self, x=width / 2, y=height / 2, id=self.model.GetNumItems() + 1
        )
        self.canvas.diagram.AddShape(cond)
        cond.Show(True)

        self._addEvent(cond)
        self.model.AddItem(cond)

        self.canvas.Refresh()

    def OnRemoveItem(self, event):
        """Remove shape"""
        self.GetCanvas().RemoveSelected()

    def OnModelProperties(self, event):
        """Model properties dialog"""
        dlg = PropertiesDialog(parent=self)
        dlg.CentreOnParent()
        properties = self.model.GetProperties()
        dlg.Init(properties)
        if dlg.ShowModal() == wx.ID_OK:
            self.ModelChanged()
            for key, value in six.iteritems(dlg.GetValues()):
                properties[key] = value
            for action in self.model.GetItems(objType=ModelAction):
                action.GetTask().set_flag("overwrite", properties["overwrite"])

        dlg.Destroy()

    def OnDeleteData(self, event):
        """Delete intermediate data"""
        rast, vect, rast3d, msg = self.model.GetIntermediateData()

        if not rast and not vect and not rast3d:
            GMessage(parent=self, message=_("No intermediate data to delete."))
            return

        dlg = wx.MessageDialog(
            parent=self,
            message=_("Do you want to permanently delete data?%s" % msg),
            caption=_("Delete intermediate data?"),
            style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
        )

        ret = dlg.ShowModal()
        dlg.Destroy()
        if ret == wx.ID_YES:
            self._deleteIntermediateData()

    def OnValidateModel(self, event, showMsg=True):
        """Validate entire model"""
        if self.model.GetNumItems() < 1:
            GMessage(parent=self, message=_("Model is empty. Nothing to validate."))
            return

        self.SetStatusText(_("Validating model..."), 0)
        errList = self.model.Validate()
        self.SetStatusText("", 0)

        if errList:
            GWarning(
                parent=self, message=_("Model is not valid.\n\n%s") % "\n".join(errList)
            )
        else:
            GMessage(parent=self, message=_("Model is valid."))

    def OnHelp(self, event):
        """Show help"""
        self._giface.Help(entry="wxGUI.gmodeler")

    def OnAbout(self, event):
        """Display About window"""
        ShowAboutDialog(prgName=_("wxGUI Graphical Modeler"), startYear="2010")

    def OnCanvasRefresh(self, event):
        """Refresh canvas"""
        self.SetStatusText(_("Redrawing model..."), 0)
        self.GetCanvas().Refresh()
        self.SetStatusText("", 0)

    def OnVariables(self, event):
        """Switch to variables page"""
        self.notebook.SetSelectionByName("variables")

        
        
