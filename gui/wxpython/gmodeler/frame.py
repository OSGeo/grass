"""
@package gmodeler.frame

@brief wxGUI Graphical Modeler for creating, editing, and managing models

Classes:
 - frame::ModelFrame
 - frame::ModelCanvas
 - frame::ModelEvtHandler
 - frame::VariablePanel
 - frame::ItemPanel
 - frame::PythonPanel

(C) 2010-2018 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Python parameterization Ondrej Pesek <pesej.ondrek gmail.com>
"""

import os
import sys
import time
import stat
import tempfile
import random
import six

import wx
from wx.lib import ogl
from core import globalvar
if globalvar.wxPythonPhoenix:
    try:
        import agw.flatnotebook as FN
    except ImportError: # if it's not there locally, try the wxPython lib.
        import wx.lib.agw.flatnotebook as FN
else:
    import wx.lib.flatnotebook as FN
from wx.lib.newevent import NewEvent

from gui_core.widgets import GNotebook
from core.gconsole        import GConsole, \
    EVT_CMD_RUN, EVT_CMD_DONE, EVT_CMD_PREPARE
from gui_core.goutput import GConsoleWindow
from core.debug import Debug
from core.gcmd import GMessage, GException, GWarning, GError
from gui_core.dialogs import GetImageHandlers
from gui_core.dialogs import TextEntryDialog as CustomTextEntryDialog
from gui_core.ghelp import ShowAboutDialog
from core.settings import UserSettings
from gui_core.menu import Menu as Menubar
from gmodeler.menudata import ModelerMenuData
from gui_core.forms import GUI
from gmodeler.preferences import PreferencesDialog, PropertiesDialog
from gmodeler.toolbars import ModelerToolbar
from core.giface import Notification
from gui_core.pystc import PyStc, SetDarkMode
from gmodeler.giface import GraphicalModelerGrassInterface
from gmodeler.model import *
from gmodeler.dialogs import *
from gui_core.wrap import (
    Button, EmptyBitmap, ImageFromBitmap, Menu, NewId, StaticBox,
    StaticText, StockCursor, TextCtrl, IsDark
)
from gui_core.wrap import TextEntryDialog as wxTextEntryDialog

wxModelDone, EVT_MODEL_DONE = NewEvent()

from grass.script.utils import try_remove
from grass.script import core as grass


class ModelFrame(wx.Frame):

    def __init__(self, parent, giface, id=wx.ID_ANY,
                 title=_("Graphical Modeler"), **kwargs):
        """Graphical modeler main window

        :param parent: parent window
        :param id: window id
        :param title: window title

        :param kwargs: wx.Frames' arguments
        """
        self.parent = parent
        self._giface = giface
        self.searchDialog = None  # module search dialog
        self.baseTitle = title
        self.modelFile = None    # loaded model
        self.start_time = None
        self.modelChanged = False
        self.randomness = 40  # random layout

        self.cursors = {
            "default": StockCursor(wx.CURSOR_ARROW),
            "cross": StockCursor(wx.CURSOR_CROSS),
        }

        wx.Frame.__init__(self, parent=parent, id=id, title=title, **kwargs)
        self.SetName("Modeler")
        self.SetIcon(
            wx.Icon(
                os.path.join(
                    globalvar.ICONDIR,
                    'grass.ico'),
                wx.BITMAP_TYPE_ICO))

        self.menubar = Menubar(
            parent=self,
            model=ModelerMenuData().GetModel(
                separators=True))
        self.SetMenuBar(self.menubar)

        self.toolbar = ModelerToolbar(parent=self)
        # workaround for http://trac.wxwidgets.org/ticket/13888
        if sys.platform != 'darwin':
            self.SetToolBar(self.toolbar)

        self.statusbar = self.CreateStatusBar(number=1)

        self.notebook = GNotebook(parent=self,
                                  style=globalvar.FNPageDStyle)

        self.canvas = ModelCanvas(self)
        self.canvas.SetBackgroundColour(
            wx.SystemSettings().GetColour(wx.SYS_COLOUR_WINDOW))
        self.canvas.SetCursor(self.cursors["default"])

        self.model = Model(self.canvas)

        self.variablePanel = VariablePanel(parent=self)

        self.itemPanel = ItemPanel(parent=self)

        self.pythonPanel = PythonPanel(parent=self)

        self._gconsole = GConsole(guiparent=self)
        self.goutput = GConsoleWindow(
            parent=self, giface=giface, gconsole=self._gconsole
        )
        self.goutput.showNotification.connect(
            lambda message: self.SetStatusText(message))

        # here events are binded twice
        self._gconsole.Bind(
            EVT_CMD_RUN,
            lambda event: self._switchPageHandler(
                event=event,
                notification=Notification.MAKE_VISIBLE))
        self._gconsole.Bind(
            EVT_CMD_DONE,
            lambda event: self._switchPageHandler(
                event=event,
                notification=Notification.RAISE_WINDOW))
        self.Bind(EVT_CMD_RUN, self.OnCmdRun)
        # rewrite default method to avoid hiding progress bar
        self._gconsole.Bind(EVT_CMD_DONE, self.OnCmdDone)
        self.Bind(EVT_CMD_PREPARE, self.OnCmdPrepare)
        self.Bind(EVT_MODEL_DONE, self.OnModelDone)

        self.notebook.AddPage(page=self.canvas, text=_('Model'), name='model')
        self.notebook.AddPage(
            page=self.itemPanel,
            text=_('Items'),
            name='items')
        self.notebook.AddPage(
            page=self.variablePanel,
            text=_('Variables'),
            name='variables')
        self.notebook.AddPage(
            page=self.pythonPanel,
            text=_('Python editor'),
            name='python')
        self.notebook.AddPage(
            page=self.goutput,
            text=_('Command output'),
            name='output')
        wx.CallAfter(self.notebook.SetSelectionByName, 'model')
        wx.CallAfter(self.ModelChanged, False)

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        self.notebook.Bind(
            FN.EVT_FLATNOTEBOOK_PAGE_CHANGED,
            self.OnPageChanged)

        self._layout()
        self.SetMinSize((640, 300))
        self.SetSize((800, 600))

        # fix goutput's pane size
        if self.goutput:
            self.goutput.SetSashPosition(int(self.GetSize()[1] * .75))

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        sizer.Add(self.notebook, proportion=1,
                  flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        sizer.Fit(self)

        self.Layout()

    def _addEvent(self, item):
        """Add event to item"""
        evthandler = ModelEvtHandler(self.statusbar,
                                     self)
        evthandler.SetShape(item)
        evthandler.SetPreviousHandler(item.GetEventHandler())
        item.SetEventHandler(evthandler)

    def _randomShift(self):
        """Returns random value to shift layout"""
        return random.randint(-self.randomness, self.randomness)

    def GetCanvas(self):
        """Get canvas"""
        return self.canvas

    def GetModel(self):
        """Get model"""
        return self.model

    def ModelChanged(self, changed=True):
        """Update window title"""
        self.modelChanged = changed

        if self.modelFile:
            if self.modelChanged:
                self.SetTitle(
                    self.baseTitle +
                    " - " +
                    os.path.basename(
                        self.modelFile) +
                    '*')
            else:
                self.SetTitle(
                    self.baseTitle +
                    " - " +
                    os.path.basename(
                        self.modelFile))
        else:
            self.SetTitle(self.baseTitle)

    def OnPageChanged(self, event):
        """Page in notebook changed"""
        page = event.GetSelection()
        if page == self.notebook.GetPageIndexByName('python'):
            if self.pythonPanel.IsEmpty():
                self.pythonPanel.RefreshScript()

            if self.pythonPanel.IsModified():
                self.SetStatusText(
                    _('Python script contains local modifications'), 0)
            else:
                self.SetStatusText(_('Python script is up-to-date'), 0)
        elif page == self.notebook.GetPageIndexByName('items'):
            self.itemPanel.Update()

        event.Skip()

    def OnVariables(self, event):
        """Switch to variables page"""
        self.notebook.SetSelectionByName('variables')

    def OnRemoveItem(self, event):
        """Remove shape
        """
        self.GetCanvas().RemoveSelected()

    def OnCanvasRefresh(self, event):
        """Refresh canvas"""
        self.SetStatusText(_("Redrawing model..."), 0)
        self.GetCanvas().Refresh()
        self.SetStatusText("", 0)

    def OnCmdRun(self, event):
        """Run command"""
        try:
            action = self.GetModel().GetItems()[event.pid]
            if hasattr(action, "task"):
                action.Update(running=True)
        except IndexError:
            pass

    def OnCmdPrepare(self, event):
        """Prepare for running command"""
        if not event.userData:
            return

        event.onPrepare(item=event.userData['item'],
                        params=event.userData['params'])

    def OnCmdDone(self, event):
        """Command done (or aborted)"""
        def time_elapsed(etime):
            try:
                ctime = time.time() - etime
                if ctime < 60:
                    stime = _("%d sec") % int(ctime)
                else:
                    mtime = int(ctime / 60)
                    stime = _("%(min)d min %(sec)d sec") % {
                        'min': mtime, 'sec': int(ctime - (mtime * 60))}
            except KeyError:
                # stopped deamon
                stime = _("unknown")

            return stime

        self.goutput.GetProgressBar().SetValue(0)
        self.goutput.WriteCmdLog('({}) {} ({})'.format(
            str(time.ctime()), _("Command finished"), time_elapsed(event.time)),
            notification=event.notification)

        try:
            action = self.GetModel().GetItems()[event.pid]
            if hasattr(action, "task"):
                action.Update(running=True)
            if event.pid == self._gconsole.cmdThread.GetId() - 1 and self.start_time:
                self.goutput.WriteCmdLog('({}) {} ({})'.format(
                    str(time.ctime()), _("Model computation finished"), time_elapsed(self.start_time)),
                                         notification=event.notification)
                event = wxModelDone()
                wx.PostEvent(self, event)

        except IndexError:
            pass

    def OnCloseWindow(self, event):
        """Close window"""
        if self.modelChanged and UserSettings.Get(
                group='manager', key='askOnQuit', subkey='enabled'):
            if self.modelFile:
                message = _("Do you want to save changes in the model?")
            else:
                message = _("Do you want to store current model settings "
                            "to model file?")

            # ask user to save current settings
            dlg = wx.MessageDialog(self,
                                   message=message,
                                   caption=_("Quit Graphical Modeler"),
                                   style=wx.YES_NO | wx.YES_DEFAULT |
                                   wx.CANCEL | wx.ICON_QUESTION | wx.CENTRE)
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

    def OnSize(self, event):
        """Window resized, save to the model"""
        self.ModelChanged()
        event.Skip()

    def OnPreferences(self, event):
        """Open preferences dialog"""
        dlg = PreferencesDialog(parent=self, giface=self._giface)
        dlg.CenterOnParent()

        dlg.Show()
        self.canvas.Refresh()

    def OnHelp(self, event):
        """Show help"""
        self._giface.Help(entry='wxGUI.gmodeler')

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
                action.GetTask().set_flag('overwrite', properties['overwrite'])

        dlg.Destroy()

    def _deleteIntermediateData(self):
        """Delete intermediate data"""
        rast, vect, rast3d, msg = self.model.GetIntermediateData()
        if rast:
            self._gconsole.RunCmd(['g.remove', '-f', 'type=raster',
                                   'name=%s' % ','.join(rast)])
        if rast3d:
            self._gconsole.RunCmd(['g.remove', '-f', 'type=raster_3d',
                                   'name=%s' % ','.join(rast3d)])
        if vect:
            self._gconsole.RunCmd(['g.remove', '-f', 'type=vector',
                                   'name=%s' % ','.join(vect)])

        self.SetStatusText(_("%d intermediate maps deleted from current mapset") %
                           int(len(rast) + len(rast3d) + len(vect)))

    def OnDeleteData(self, event):
        """Delete intermediate data"""
        rast, vect, rast3d, msg = self.model.GetIntermediateData()

        if not rast and not vect and not rast3d:
            GMessage(parent=self,
                     message=_('No intermediate data to delete.'))
            return

        dlg = wx.MessageDialog(
            parent=self,
            message=_(
                "Do you want to permanently delete data?%s" %
                msg),
            caption=_("Delete intermediate data?"),
            style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)

        ret = dlg.ShowModal()
        dlg.Destroy()
        if ret == wx.ID_YES:
            self._deleteIntermediateData()

    def OnModelNew(self, event):
        """Create new model"""
        Debug.msg(4, "ModelFrame.OnModelNew():")

        # ask user to save current model
        if self.modelFile and self.modelChanged:
            self.OnModelSave()
        elif self.modelFile is None and \
                (self.model.GetNumItems() > 0 or len(self.model.GetData()) > 0):
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Current model is not empty. "
                    "Do you want to store current settings "
                    "to model file?"),
                caption=_("Create new model?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.CANCEL | wx.ICON_QUESTION)
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
        self.modelChanged = False
        self.SetTitle(self.baseTitle)

    def GetModelFile(self, ext=True):
        """Get model file

        :param bool ext: False to avoid extension
        """
        if not self.modelFile:
            return ''
        if ext:
            return self.modelFile
        return os.path.splitext(self.modelFile)[0]

    def OnModelOpen(self, event):
        """Load model from file"""
        filename = ''
        dlg = wx.FileDialog(parent=self, message=_("Choose model file"),
                            defaultDir=os.getcwd(),
                            wildcard=_("GRASS Model File (*.gxm)|*.gxm"))
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if not filename:
            return

        Debug.msg(4, "ModelFrame.OnModelOpen(): filename=%s" % filename)

        # close current model
        self.OnModelClose()

        self.LoadModelFile(filename)

        self.modelFile = filename
        self.SetTitle(
            self.baseTitle +
            " - " +
            os.path.basename(
                self.modelFile))
        self.SetStatusText(
            _('%(items)d items (%(actions)d actions) loaded into model') % {
                'items': self.model.GetNumItems(),
                'actions': self.model.GetNumItems(
                    actionOnly=True)},
            0)

    def OnModelSave(self, event=None):
        """Save model to file"""
        if self.modelFile and self.modelChanged:
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Model file <%s> already exists. "
                    "Do you want to overwrite this file?") %
                self.modelFile,
                caption=_("Save model"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
            else:
                Debug.msg(
                    4, "ModelFrame.OnModelSave(): filename=%s" %
                    self.modelFile)
                self.WriteModelFile(self.modelFile)
                self.SetStatusText(_('File <%s> saved') % self.modelFile, 0)
                self.SetTitle(
                    self.baseTitle +
                    " - " +
                    os.path.basename(
                        self.modelFile))
        elif not self.modelFile:
            self.OnModelSaveAs(None)

    def OnModelSaveAs(self, event):
        """Create model to file as"""
        filename = ''
        dlg = wx.FileDialog(parent=self,
                            message=_("Choose file to save current model"),
                            defaultDir=os.getcwd(),
                            wildcard=_("GRASS Model File (*.gxm)|*.gxm"),
                            style=wx.FD_SAVE)

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
                    "Do you want to overwrite this file?") %
                filename,
                caption=_("File already exists"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() != wx.ID_YES:
                dlg.Destroy()
                return

        Debug.msg(4, "GMFrame.OnModelSaveAs(): filename=%s" % filename)

        self.WriteModelFile(filename)
        self.modelFile = filename
        self.SetTitle(
            self.baseTitle +
            " - " +
            os.path.basename(
                self.modelFile))
        self.SetStatusText(_('File <%s> saved') % self.modelFile, 0)

    def OnModelClose(self, event=None):
        """Close model file"""
        Debug.msg(4, "ModelFrame.OnModelClose(): file=%s" % self.modelFile)
        # ask user to save current model
        if self.modelFile and self.modelChanged:
            self.OnModelSave()
        elif self.modelFile is None and \
                (self.model.GetNumItems() > 0 or len(self.model.GetData()) > 0):
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Current model is not empty. "
                    "Do you want to store current settings "
                    "to model file?"),
                caption=_("Create new model?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.CANCEL | wx.ICON_QUESTION)
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

    def OnModelDone(self, event):
        """Computation finished
        """
        self.SetStatusText('', 0)

        # restore original files
        if hasattr(self.model, "fileInput"):
            for finput in self.model.fileInput:
                data = self.model.fileInput[finput]
                if not data:
                    continue

                fd = open(finput, "w")
                try:
                    fd.write(data)
                finally:
                    fd.close()
            del self.model.fileInput

        # delete intermediate data
        self._deleteIntermediateData()

        # display data if required
        for data in self.model.GetData():
            if not data.HasDisplay():
                continue

            # remove existing map layers first
            layers =  self._giface.GetLayerList().GetLayersByName(data.GetValue())
            if layers:
                for layer in layers:
                    self._giface.GetLayerList().DeleteLayer(layer)

            # add new map layer
            self._giface.GetLayerList().AddLayer(
                ltype=data.GetPrompt(), name=data.GetValue(), checked=True,
                cmd=data.GetDisplayCmd())

    def OnValidateModel(self, event, showMsg=True):
        """Validate entire model"""
        if self.model.GetNumItems() < 1:
            GMessage(parent=self,
                     message=_('Model is empty. Nothing to validate.'))
            return

        self.SetStatusText(_('Validating model...'), 0)
        errList = self.model.Validate()
        self.SetStatusText('', 0)

        if errList:
            GWarning(
                parent=self,
                message=_('Model is not valid.\n\n%s') %
                '\n'.join(errList))
        else:
            GMessage(parent=self,
                     message=_('Model is valid.'))

    def OnExportImage(self, event):
        """Export model to image (default image)
        """
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
        size = wx.Size(int(xmaxImg - xminImg) + 50,
                       int(ymaxImg - yminImg) + 50)
        bitmap = EmptyBitmap(width=size.width, height=size.height)

        filetype, ltype = GetImageHandlers(ImageFromBitmap(bitmap))

        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose a file name to save the image (no need to add extension)"),
            defaultDir="",
            defaultFile="",
            wildcard=filetype,
            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return

            base, ext = os.path.splitext(path)
            fileType = ltype[dlg.GetFilterIndex()]['type']
            extType = ltype[dlg.GetFilterIndex()]['ext']
            if ext != extType:
                path = base + '.' + extType

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

    def OnDefineRelation(self, event):
        """Define relation between data and action items"""
        self.canvas.SetCursor(self.cursors["cross"])
        self.defineRelation = {'from': None,
                               'to': None}

    def OnDefineLoop(self, event):
        """Define new loop in the model

        .. todo::
            move to ModelCanvas?
        """
        self.ModelChanged()

        width, height = self.canvas.GetSize()
        loop = ModelLoop(self, x=width / 2, y=height / 2,
                         id=self.model.GetNumItems() + 1)
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
        cond = ModelCondition(self, x=width / 2, y=height / 2,
                              id=self.model.GetNumItems() + 1)
        self.canvas.diagram.AddShape(cond)
        cond.Show(True)

        self._addEvent(cond)
        self.model.AddItem(cond)

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
            comment=comment)
        overwrite = self.model.GetProperties().get('overwrite', None)
        if overwrite is not None:
            action.GetTask().set_flag('overwrite', overwrite)

        self.canvas.diagram.AddShape(action)
        action.Show(True)

        self._addEvent(action)
        self.model.AddItem(action)

        self.itemPanel.Update()
        self.canvas.Refresh()
        time.sleep(.1)

        # show properties dialog
        win = action.GetPropDialog()
        if not win:
            cmdLength = len(action.GetLog(string=False))
            if cmdLength > 1 and action.IsValid():
                self.GetOptData(dcmd=action.GetLog(string=False), layer=action,
                                params=action.GetParams(), propwin=None)
            else:
                gmodule = GUI(
                    parent=self,
                    show=True,
                    giface=GraphicalModelerGrassInterface(
                        self.model))
                gmodule.ParseCommand(
                    action.GetLog(
                        string=False), completed=(
                        self.GetOptData, action, action.GetParams()))
        elif win and not win.IsShown():
            win.Show()

        if win:
            win.Raise()

    def OnAddData(self, event):
        """Add data item to model
        """
        # add action to canvas
        width, height = self.canvas.GetSize()
        data = ModelData(self, x=width / 2 + self._randomShift(),
                         y=height / 2 + self._randomShift())

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
            textSize=(
                300,
                75))

        if dlg.ShowModal() == wx.ID_OK:
            comment = dlg.GetValue()
            if not comment:
                GError(
                    _("Empty comment. Nothing to add to the model."),
                    parent=self)
            else:
                x, y = self.canvas.GetNewShapePos()
                commentObj = ModelComment(
                    self.model, x=x + self._randomShift(),
                    y=y + self._randomShift(),
                    id=self.model.GetNextId(),
                    label=comment)
                self.canvas.diagram.AddShape(commentObj)
                commentObj.Show(True)
                self._addEvent(commentObj)
                self.model.AddItem(commentObj)

                self.canvas.Refresh()
                self.ModelChanged()

        dlg.Destroy()

    def _switchPageHandler(self, event, notification):
        self._switchPage(notification=notification)
        event.Skip()

    def _switchPage(self, notification):
        """Manages @c 'output' notebook page according to event notification."""
        if notification == Notification.HIGHLIGHT:
            self.notebook.HighlightPageByName('output')
        if notification == Notification.MAKE_VISIBLE:
            self.notebook.SetSelectionByName('output')
        if notification == Notification.RAISE_WINDOW:
            self.notebook.SetSelectionByName('output')
            self.SetFocus()
            self.Raise()

    def OnAbout(self, event):
        """Display About window"""
        ShowAboutDialog(prgName=_('wxGUI Graphical Modeler'), startYear='2010')

    def GetOptData(self, dcmd, layer, params, propwin):
        """Process action data"""
        if params:  # add data items
            width, height = self.canvas.GetSize()
            x = width / 2 - 200 + self._randomShift()
            y = height / 2 + self._randomShift()
            for p in params['params']:
                if p.get(
                        'prompt', '') not in (
                        'raster', 'vector', 'raster_3d', 'dbtable'):
                    continue

                # add new data item if defined or required
                if p.get(
                        'value', None) or(
                        p.get('age', 'old') != 'old' and p.get('required', 'no') ==
                        'yes'):
                    data = layer.FindData(p.get('name', ''))
                    if data:
                        data.SetValue(p.get('value', ''))
                        data.Update()
                        continue

                    data = self.model.FindData(p.get('value', ''),
                                               p.get('prompt', ''))
                    if data:
                        if p.get('age', 'old') == 'old':
                            rel = ModelRelation(
                                parent=self, fromShape=data, toShape=layer,
                                param=p.get('name', ''))
                        else:
                            rel = ModelRelation(
                                parent=self, fromShape=layer, toShape=data,
                                param=p.get('name', ''))
                        layer.AddRelation(rel)
                        data.AddRelation(rel)
                        self.AddLine(rel)
                        data.Update()
                        continue

                    data = ModelData(self, value=p.get('value', ''),
                                     prompt=p.get('prompt', ''),
                                     x=x, y=y)
                    self._addEvent(data)
                    self.canvas.diagram.AddShape(data)
                    data.Show(True)

                    if p.get('age', 'old') == 'old':
                        rel = ModelRelation(
                            parent=self, fromShape=data, toShape=layer,
                            param=p.get('name', ''))
                    else:
                        rel = ModelRelation(
                            parent=self, fromShape=layer, toShape=data,
                            param=p.get('name', ''))
                    layer.AddRelation(rel)
                    data.AddRelation(rel)
                    self.AddLine(rel)
                    data.Update()

                # remove dead data items
                if not p.get('value', ''):
                    data = layer.FindData(p.get('name', ''))
                    if data:
                        remList, upList = self.model.RemoveItem(data, layer)
                        for item in remList:
                            self.canvas.diagram.RemoveShape(item)
                            item.__del__()

                        for item in upList:
                            item.Update()

            # valid / parameterized ?
            layer.SetValid(params)

            self.canvas.Refresh()

        if dcmd:
            layer.SetProperties(params, propwin)

        self.SetStatusText(layer.GetLog(), 0)

    def AddLine(self, rel):
        """Add connection between model objects

        :param rel: relation
        """
        fromShape = rel.GetFrom()
        toShape = rel.GetTo()

        rel.SetCanvas(self)
        rel.SetPen(wx.BLACK_PEN)
        rel.SetBrush(wx.BLACK_BRUSH)
        rel.AddArrow(ogl.ARROW_ARROW)
        points = rel.GetControlPoints()
        rel.MakeLineControlPoints(2)
        if points:
            for x, y in points:
                rel.InsertLineControlPoint(point=wx.RealPoint(x, y))

        self._addEvent(rel)
        try:
            fromShape.AddLine(rel, toShape)
        except TypeError:
            pass  # bug when connecting ModelCondition and ModelLoop - to be fixed

        self.canvas.diagram.AddShape(rel)
        rel.Show(True)

    def LoadModelFile(self, filename):
        """Load model definition stored in GRASS Model XML file (gxm)
        """
        try:
            self.model.LoadModel(filename)
        except GException as e:
            GError(
                parent=self, message=_(
                    "Reading model file <%s> failed.\n"
                    "Invalid file, unable to parse XML document.\n\n%s") %
                (filename, e), showTraceback=False)
            return

        self.modelFile = filename
        self.SetTitle(
            self.baseTitle +
            " - " +
            os.path.basename(
                self.modelFile))

        self.SetStatusText(_("Please wait, loading model..."), 0)

        # load actions
        for item in self.model.GetItems(objType=ModelAction):
            self._addEvent(item)
            self.canvas.diagram.AddShape(item)
            item.Show(True)
            # relations/data
            for rel in item.GetRelations():
                if rel.GetFrom() == item:
                    dataItem = rel.GetTo()
                else:
                    dataItem = rel.GetFrom()
                self._addEvent(dataItem)
                self.canvas.diagram.AddShape(dataItem)
                self.AddLine(rel)
                dataItem.Show(True)

        # load loops
        for item in self.model.GetItems(objType=ModelLoop):
            self._addEvent(item)
            self.canvas.diagram.AddShape(item)
            item.Show(True)

            # connect items in the loop
            self.DefineLoop(item)

        # load conditions
        for item in self.model.GetItems(objType=ModelCondition):
            self._addEvent(item)
            self.canvas.diagram.AddShape(item)
            item.Show(True)

            # connect items in the condition
            self.DefineCondition(item)

        # load comments
        for item in self.model.GetItems(objType=ModelComment):
            self._addEvent(item)
            self.canvas.diagram.AddShape(item)
            item.Show(True)

        # load variables
        self.variablePanel.Update()
        self.itemPanel.Update()
        self.SetStatusText('', 0)

        # final updates
        for action in self.model.GetItems(objType=ModelAction):
            action.SetValid(action.GetParams())
            action.Update()

        self.canvas.Refresh(True)

    def WriteModelFile(self, filename):
        """Save model to model file, recover original file on error.

        :return: True on success
        :return: False on failure
        """
        self.ModelChanged(False)
        tmpfile = tempfile.TemporaryFile(mode='w+')
        try:
            WriteModelFile(fd=tmpfile, model=self.model)
        except Exception:
            GError(parent=self,
                   message=_("Writing current settings to model file failed."))
            return False

        try:
            mfile = open(filename, "w")
            tmpfile.seek(0)
            for line in tmpfile.readlines():
                mfile.write(line)
        except IOError:
            wx.MessageBox(
                parent=self,
                message=_("Unable to open file <%s> for writing.") %
                filename,
                caption=_("Error"),
                style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return False

        mfile.close()

        return True

    def DefineLoop(self, loop):
        """Define loop with given list of items"""
        parent = loop
        items = loop.GetItems(self.GetModel().GetItems())
        if not items:
            return

        # remove defined relations first
        for rel in loop.GetRelations():
            self.canvas.GetDiagram().RemoveShape(rel)
        loop.Clear()

        for item in items:
            rel = ModelRelation(parent=self, fromShape=parent, toShape=item)
            dx = item.GetX() - parent.GetX()
            dy = item.GetY() - parent.GetY()
            loop.AddRelation(rel)
            if dx != 0:
                rel.SetControlPoints(
                    ((parent.GetX(),
                      parent.GetY() + dy / 2),
                     (parent.GetX() + dx, parent.GetY() + dy / 2)))
            self.AddLine(rel)
            parent = item

        # close loop
        item = items[-1]
        rel = ModelRelation(parent=self, fromShape=item, toShape=loop)
        loop.AddRelation(rel)
        self.AddLine(rel)
        dx = (item.GetX() - loop.GetX()) + loop.GetWidth() / 2 + 50
        dy = item.GetHeight() / 2 + 50
        rel.MakeLineControlPoints(0)
        rel.InsertLineControlPoint(
            point=wx.RealPoint(
                loop.GetX() -
                loop.GetWidth() /
                2,
                loop.GetY()))
        rel.InsertLineControlPoint(
            point=wx.RealPoint(
                item.GetX(),
                item.GetY() +
                item.GetHeight() /
                2))
        rel.InsertLineControlPoint(point=wx.RealPoint(item.GetX(),
                                                      item.GetY() + dy))
        rel.InsertLineControlPoint(point=wx.RealPoint(item.GetX() - dx,
                                                      item.GetY() + dy))
        rel.InsertLineControlPoint(point=wx.RealPoint(item.GetX() - dx,
                                                      loop.GetY()))

        self.canvas.Refresh()

    def DefineCondition(self, condition):
        """Define if-else statement with given list of items"""
        items = condition.GetItems(self.model.GetItems(objType=ModelAction))
        if not items['if'] and not items['else']:
            return

        parent = condition

        # remove defined relations first
        for rel in condition.GetRelations():
            self.canvas.GetDiagram().RemoveShape(rel)
        condition.Clear()
        dxIf = condition.GetX() + condition.GetWidth() / 2
        dxElse = condition.GetX() - condition.GetWidth() / 2
        dy = condition.GetY()
        for branch in items.keys():
            for item in items[branch]:
                rel = ModelRelation(parent=self, fromShape=parent,
                                    toShape=item)
                condition.AddRelation(rel)
                self.AddLine(rel)
                rel.MakeLineControlPoints(0)
                if branch == 'if':
                    rel.InsertLineControlPoint(
                        point=wx.RealPoint(
                            item.GetX() - item.GetWidth() / 2,
                            item.GetY()))
                    rel.InsertLineControlPoint(point=wx.RealPoint(dxIf, dy))
                else:
                    rel.InsertLineControlPoint(point=wx.RealPoint(dxElse, dy))
                    rel.InsertLineControlPoint(
                        point=wx.RealPoint(
                            item.GetX() - item.GetWidth() / 2,
                            item.GetY()))
                parent = item

        self.canvas.Refresh()


class ModelCanvas(ogl.ShapeCanvas):
    """Canvas where model is drawn"""

    def __init__(self, parent):
        self.parent = parent
        ogl.OGLInitialize()
        ogl.ShapeCanvas.__init__(self, parent)

        self.diagram = ogl.Diagram()
        self.SetDiagram(self.diagram)
        self.diagram.SetCanvas(self)

        self.SetScrollbars(20, 20, 2000 / 20, 2000 / 20)

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
        shapes = [shape for shape in diagram.GetShapeList()
                  if shape.Selected()]
        self.RemoveShapes(shapes)

    def RemoveShapes(self, shapes):
        """Removes shapes"""
        self.parent.ModelChanged()
        diagram = self.GetDiagram()
        for shape in shapes:
            remList, upList = self.parent.GetModel().RemoveItem(shape)
            shape.Select(False)
            diagram.RemoveShape(shape)
            shape.__del__()
            for item in remList:
                diagram.RemoveShape(item)
                item.__del__()

            for item in upList:
                item.Update()

        self.Refresh()

    def GetNewShapePos(self):
        """Determine optimal position for newly added object

        :return: x,y
        """
        xNew, yNew = map(lambda x: x / 2, self.GetSize())
        diagram = self.GetDiagram()

        for shape in diagram.GetShapeList():
            y = shape.GetY()
            yBox = shape.GetBoundingBoxMin()[1] / 2
            if yBox > 0 and y < yNew + yBox and y > yNew - yBox:
                yNew += yBox * 3

        return xNew, yNew

    def GetShapesSelected(self):
        """Get list of selected shapes"""
        selected = list()
        diagram = self.GetDiagram()
        for shape in diagram.GetShapeList():
            if shape.Selected():
                selected.append(shape)

        return selected


class ModelEvtHandler(ogl.ShapeEvtHandler):
    """Model event handler class"""

    def __init__(self, log, frame):
        ogl.ShapeEvtHandler.__init__(self)
        self.log = log
        self.frame = frame
        self.x = self.y = None

    def OnLeftClick(self, x, y, keys=0, attachment=0):
        """Left mouse button pressed -> select item & update statusbar"""
        shape = self.GetShape()
        canvas = shape.GetCanvas()
        dc = wx.ClientDC(canvas)

        # probably does nothing, removed from wxPython 2.9
        # canvas.PrepareDC(dc)

        if hasattr(self.frame, 'defineRelation'):
            drel = self.frame.defineRelation
            if drel['from'] is None:
                drel['from'] = shape
            elif drel['to'] is None:
                drel['to'] = shape
                rel = ModelRelation(parent=self.frame, fromShape=drel['from'],
                                    toShape=drel['to'])
                dlg = ModelRelationDialog(parent=self.frame,
                                          shape=rel)
                if dlg.IsValid():
                    ret = dlg.ShowModal()
                    if ret == wx.ID_OK:
                        option = dlg.GetOption()
                        rel.SetName(option)
                        drel['from'].AddRelation(rel)
                        drel['to'].AddRelation(rel)
                        drel['from'].Update()
                        params = {'params': [
                            {'name': option, 'value': drel['from'].GetValue()}]}
                        drel['to'].MergeParams(params)
                        self.frame.AddLine(rel)

                    dlg.Destroy()
                del self.frame.defineRelation

        # select object
        self._onSelectShape(shape, append=True if keys == 1 else False)

        if hasattr(shape, "GetLog"):
            self.log.SetStatusText(shape.GetLog(), 0)
        else:
            self.log.SetStatusText('', 0)

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
                    self.frame.GetModel()))
            gmodule.ParseCommand(
                shape.GetLog(
                    string=False), completed=(
                    self.frame.GetOptData, shape, shape.GetParams()))

        elif isinstance(shape, ModelData):
            if shape.GetPrompt() in ('raster', 'vector', 'raster_3d'):
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
                alist = list()
                for aId in ids['unchecked']:
                    action = model.GetItem(aId, objType=ModelAction)
                    if action:
                        action.UnSetBlock(shape)
                for aId in ids['checked']:
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
                    alist = list()
                    for aId in ids[b]['unchecked']:
                        action = model.GetItem(aId, objType=ModelAction)
                        action.UnSetBlock(shape)
                    for aId in ids[b]['checked']:
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
            self.popupID = dict()
            for key in ('remove', 'enable', 'addPoint',
                        'delPoint', 'intermediate', 'display', 'props', 'id',
                        'label', 'comment'):
                self.popupID[key] = NewId()

        # record coordinates
        self.x = x
        self.y = y

        # select object
        shape = self.GetShape()
        self._onSelectShape(shape)

        popupMenu = Menu()
        popupMenu.Append(self.popupID['remove'], _('Remove'))
        self.frame.Bind(wx.EVT_MENU, self.OnRemove, id=self.popupID['remove'])
        if isinstance(shape, ModelAction) or isinstance(shape, ModelLoop):
            if shape.IsEnabled():
                popupMenu.Append(self.popupID['enable'], _('Disable'))
                self.frame.Bind(
                    wx.EVT_MENU,
                    self.OnDisable,
                    id=self.popupID['enable'])
            else:
                popupMenu.Append(self.popupID['enable'], _('Enable'))
                self.frame.Bind(
                    wx.EVT_MENU,
                    self.OnEnable,
                    id=self.popupID['enable'])
        if isinstance(shape, ModelAction) or isinstance(shape, ModelComment):
            popupMenu.AppendSeparator()
        if isinstance(shape, ModelAction):
            popupMenu.Append(self.popupID['label'], _('Set label'))
            self.frame.Bind(
                wx.EVT_MENU,
                self.OnSetLabel,
                id=self.popupID['label'])
        if isinstance(shape, ModelAction) or isinstance(shape, ModelComment):
            popupMenu.Append(self.popupID['comment'], _('Set comment'))
            self.frame.Bind(
                wx.EVT_MENU,
                self.OnSetComment,
                id=self.popupID['comment'])

        if isinstance(shape, ModelRelation):
            popupMenu.AppendSeparator()
            popupMenu.Append(
                self.popupID['addPoint'],
                _('Add control point'))
            self.frame.Bind(
                wx.EVT_MENU,
                self.OnAddPoint,
                id=self.popupID['addPoint'])
            popupMenu.Append(
                self.popupID['delPoint'],
                _('Remove control point'))
            self.frame.Bind(
                wx.EVT_MENU,
                self.OnRemovePoint,
                id=self.popupID['delPoint'])
            if len(shape.GetLineControlPoints()) == 2:
                popupMenu.Enable(self.popupID['delPoint'], False)

        if isinstance(shape, ModelData):
            popupMenu.AppendSeparator()
            if '@' not in shape.GetValue() and \
               len(self.GetShape().GetRelations('from')) > 0:
                popupMenu.Append(
                    self.popupID['intermediate'],
                    _('Intermediate'),
                    kind=wx.ITEM_CHECK)
                if self.GetShape().IsIntermediate():
                    popupMenu.Check(self.popupID['intermediate'], True)

                self.frame.Bind(
                    wx.EVT_MENU,
                    self.OnIntermediate,
                    id=self.popupID['intermediate'])

            if self.frame._giface.GetMapDisplay():
                popupMenu.Append(
                    self.popupID['display'],
                    _('Display'),
                    kind=wx.ITEM_CHECK)
                if self.GetShape().HasDisplay():
                    popupMenu.Check(self.popupID['display'], True)

                self.frame.Bind(
                    wx.EVT_MENU,
                    self.OnHasDisplay,
                    id=self.popupID['display'])

                if self.GetShape().IsIntermediate():
                    popupMenu.Enable(self.popupID['display'], False)

        if isinstance(shape, ModelData) or \
                isinstance(shape, ModelAction) or \
                isinstance(shape, ModelLoop):
            popupMenu.AppendSeparator()
            popupMenu.Append(self.popupID['props'], _('Properties'))
            self.frame.Bind(
                wx.EVT_MENU,
                self.OnProperties,
                id=self.popupID['props'])

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
            value=shape.GetLabel())
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
            parent=self.frame, message=_("Comment:"),
            caption=_("Set comment"),
            defaultValue=shape.GetComment(),
            textStyle=wx.TE_MULTILINE, textSize=(300, 75))
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
            redraw = False
            shapeList = canvas.GetDiagram().GetShapeList()
            toUnselect = list()

            if not append:
                for s in shapeList:
                    if s.Selected():
                        toUnselect.append(s)

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
                    ltype=shape.GetPrompt(), name=shape.GetValue(), checked=True,
                    cmd=shape.GetDisplayCmd())
            else:
                # remove map layer(s) from display
                layers = self.frame._giface.GetLayerList().GetLayersByName(shape.GetValue())
                for layer in layers:
                    self.frame._giface.GetLayerList().DeleteLayer(layer)

        except GException as e:
            GError(parent=self,
                   message='{}'.format(e))

    def OnRemove(self, event):
        """Remove shape
        """
        self.frame.GetCanvas().RemoveShapes([self.GetShape()])
        self.frame.itemPanel.Update()


class VariablePanel(wx.Panel):

    def __init__(self, parent, id=wx.ID_ANY,
                 **kwargs):
        """Manage model variables panel
        """
        self.parent = parent

        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)

        self.listBox = StaticBox(
            parent=self, id=wx.ID_ANY, label=" %s " %
            _("List of variables - right-click to delete"))

        self.list = VariableListCtrl(
            parent=self,
            columns=[
                _("Name"),
                _("Data type"),
                _("Default value"),
                _("Description")],
            frame=self.parent)

        # add new category
        self.addBox = StaticBox(parent=self, id=wx.ID_ANY,
                                label=" %s " % _("Add new variable"))
        self.name = TextCtrl(parent=self, id=wx.ID_ANY)
        wx.CallAfter(self.name.SetFocus)
        self.type = wx.Choice(parent=self, id=wx.ID_ANY,
                              choices=[_("integer"),
                                       _("float"),
                                       _("string"),
                                       _("raster"),
                                       _("vector"),
                                       _("region"),
                                       _("mapset"),
                                       _("file"),
                                       _("dir")])
        self.type.SetSelection(2)  # string
        self.value = TextCtrl(parent=self, id=wx.ID_ANY)
        self.desc = TextCtrl(parent=self, id=wx.ID_ANY)

        # buttons
        self.btnAdd = Button(parent=self, id=wx.ID_ADD)
        self.btnAdd.SetToolTip(_("Add new variable to the model"))
        self.btnAdd.Enable(False)

        # bindings
        self.name.Bind(wx.EVT_TEXT, self.OnText)
        self.value.Bind(wx.EVT_TEXT, self.OnText)
        self.desc.Bind(wx.EVT_TEXT, self.OnText)
        self.btnAdd.Bind(wx.EVT_BUTTON, self.OnAdd)

        self._layout()

    def _layout(self):
        """Layout dialog"""
        listSizer = wx.StaticBoxSizer(self.listBox, wx.VERTICAL)
        listSizer.Add(self.list, proportion=1,
                      flag=wx.EXPAND)

        addSizer = wx.StaticBoxSizer(self.addBox, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap=5, vgap=5)
        gridSizer.Add(StaticText(parent=self, id=wx.ID_ANY,
                                 label="%s:" % _("Name")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 0))
        gridSizer.Add(self.name,
                      pos=(0, 1),
                      flag=wx.EXPAND)
        gridSizer.Add(StaticText(parent=self, id=wx.ID_ANY,
                                 label="%s:" % _("Data type")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(0, 2))
        gridSizer.Add(self.type,
                      pos=(0, 3))
        gridSizer.Add(StaticText(parent=self, id=wx.ID_ANY,
                                 label="%s:" % _("Default value")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(1, 0))
        gridSizer.Add(self.value,
                      pos=(1, 1), span=(1, 3),
                      flag=wx.EXPAND)
        gridSizer.Add(StaticText(parent=self, id=wx.ID_ANY,
                                 label="%s:" % _("Description")),
                      flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(2, 0))
        gridSizer.Add(self.desc,
                      pos=(2, 1), span=(1, 3),
                      flag=wx.EXPAND)
        gridSizer.AddGrowableCol(1)
        addSizer.Add(gridSizer,
                     flag=wx.EXPAND)
        addSizer.Add(self.btnAdd, proportion=0,
                     flag=wx.TOP | wx.ALIGN_RIGHT, border=5)

        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(listSizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(addSizer, proportion=0,
                      flag=wx.EXPAND |
                      wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def OnText(self, event):
        """Text entered"""
        if self.name.GetValue():
            self.btnAdd.Enable()
        else:
            self.btnAdd.Enable(False)

    def OnAdd(self, event):
        """Add new variable to the list"""
        msg = self.list.Append(self.name.GetValue(),
                               self.type.GetStringSelection(),
                               self.value.GetValue(),
                               self.desc.GetValue())
        self.name.SetValue('')
        self.name.SetFocus()

        if msg:
            GError(parent=self,
                   message=msg)
        else:
            self.type.SetSelection(2)  # string
            self.value.SetValue('')
            self.desc.SetValue('')
            self.UpdateModelVariables()

    def UpdateModelVariables(self):
        """Update model variables"""
        variables = dict()
        for values in six.itervalues(self.list.GetData()):
            name = values[0]
            variables[name] = {'type': str(values[1])}
            if values[2]:
                variables[name]['value'] = values[2]
            if values[3]:
                variables[name]['description'] = values[3]

        self.parent.GetModel().SetVariables(variables)
        self.parent.ModelChanged()

    def Update(self):
        """Reload list of variables"""
        self.list.OnReload(None)

    def Reset(self):
        """Remove all variables"""
        self.list.DeleteAllItems()
        self.parent.GetModel().SetVariables([])


class ItemPanel(wx.Panel):

    def __init__(self, parent, id=wx.ID_ANY,
                 **kwargs):
        """Manage model items
        """
        self.parent = parent

        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)

        self.listBox = StaticBox(
            parent=self, id=wx.ID_ANY, label=" %s " %
            _("List of items - right-click to delete"))

        self.list = ItemListCtrl(
            parent=self,
            columns=[
                _("Label"),
                _("In loop"),
                _("Parameterized"),
                _("Command")],
            columnsNotEditable=[
                1,
                2,
                3],
            frame=self.parent)

        self.btnMoveUp = Button(parent=self, id=wx.ID_UP)
        self.btnMoveDown = Button(parent=self, id=wx.ID_DOWN)
        self.btnRefresh = Button(parent=self, id=wx.ID_REFRESH)

        self.btnMoveUp.Bind(wx.EVT_BUTTON, self.OnMoveItemsUp)
        self.btnMoveDown.Bind(wx.EVT_BUTTON, self.OnMoveItemsDown)
        self.btnRefresh.Bind(wx.EVT_BUTTON, self.list.OnReload)

        self._layout()

    def _layout(self):
        """Layout dialog"""
        listSizer = wx.StaticBoxSizer(self.listBox, wx.VERTICAL)
        listSizer.Add(self.list, proportion=1,
                      flag=wx.EXPAND)

        manageSizer = wx.BoxSizer(wx.VERTICAL)
        manageSizer.Add(self.btnMoveUp, border=5, flag=wx.ALL)
        manageSizer.Add(self.btnMoveDown, border=5,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM)
        manageSizer.Add(self.btnRefresh, border=5,
                        flag=wx.LEFT | wx.RIGHT)

        mainSizer = wx.BoxSizer(wx.HORIZONTAL)
        mainSizer.Add(listSizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=3)
        mainSizer.Add(manageSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=3)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def Update(self):
        """Reload list of variables"""
        self.list.OnReload(None)

    def _getSelectedItems(self):
        """Get list of selected items, indices start at 0"""
        items = []
        current = -1
        while True:
            next = self.list.GetNextSelected(current)
            if next == -1:
                break
            items.append(next)
            current = next

        if not items:
            GMessage(_("No items to selected."), parent=self)

        return items

    def OnMoveItemsUp(self, event):
        """Item moved up, update action ids"""
        items = self._getSelectedItems()
        if not items:
            return
        self.list.MoveItems(items, up=True)
        self.parent.GetCanvas().Refresh()
        self.parent.ModelChanged()

    def OnMoveItemsDown(self, event):
        """Item moved up, update action ids"""
        items = self._getSelectedItems()
        if not items:
            return
        self.list.MoveItems(items, up=False)
        self.parent.GetCanvas().Refresh()
        self.parent.ModelChanged()


class PythonPanel(wx.Panel):

    def __init__(self, parent, id=wx.ID_ANY,
                 **kwargs):
        """Model as python script
        """
        self.parent = parent

        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)

        self.filename = None  # temp file to run

        self.bodyBox = StaticBox(parent=self, id=wx.ID_ANY,
                                 label=" %s " % _("Python script"))
        self.body = PyStc(parent=self, statusbar=self.parent.GetStatusBar())
        if IsDark():
            SetDarkMode(self.body)

        self.btnRun = Button(parent=self, id=wx.ID_ANY, label=_("&Run"))
        self.btnRun.SetToolTip(_("Run python script"))
        self.Bind(wx.EVT_BUTTON, self.OnRun, self.btnRun)
        self.btnSaveAs = Button(parent=self, id=wx.ID_SAVEAS)
        self.btnSaveAs.SetToolTip(_("Save python script to file"))
        self.Bind(wx.EVT_BUTTON, self.OnSaveAs, self.btnSaveAs)
        self.btnRefresh = Button(parent=self, id=wx.ID_REFRESH)
        self.btnRefresh.SetToolTip(_("Refresh python script based on the model.\n"
                                           "It will discards all local changes."))
        self.Bind(wx.EVT_BUTTON, self.OnRefresh, self.btnRefresh)

        self._layout()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        bodySizer = wx.StaticBoxSizer(self.bodyBox, wx.HORIZONTAL)
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)

        bodySizer.Add(self.body, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=3)

        btnSizer.Add(self.btnRefresh, proportion=0,
                     flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.AddStretchSpacer()
        btnSizer.Add(self.btnSaveAs, proportion=0,
                     flag=wx.RIGHT, border=5)
        btnSizer.Add(self.btnRun, proportion=0,
                     flag=wx.RIGHT, border=5)

        sizer.Add(bodySizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL, border=3)
        sizer.Add(btnSizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL, border=3)

        sizer.Fit(self)
        sizer.SetSizeHints(self)
        self.SetSizer(sizer)

    def OnRun(self, event):
        """Run Python script"""
        self.filename = grass.tempfile()
        try:
            fd = open(self.filename, "w")
            fd.write(self.body.GetText())
        except IOError as e:
            GError(_("Unable to launch Python script. %s") % e,
                   parent=self)
            return
        finally:
            fd.close()
            mode = stat.S_IMODE(os.lstat(self.filename)[stat.ST_MODE])
            os.chmod(self.filename, mode | stat.S_IXUSR)

        for item in self.parent.GetModel().GetItems():
            if len(item.GetParameterizedParams()['params']) + len(
                    item.GetParameterizedParams()['flags']) > 0:
                self.parent._gconsole.RunCmd(
                    [fd.name, '--ui'],
                    skipInterface=False, onDone=self.OnDone)
                break
        else:
            self.parent._gconsole.RunCmd(
                [fd.name],
                skipInterface=True, onDone=self.OnDone)

        event.Skip()

    def OnDone(self, event):
        """Python script finished"""
        try_remove(self.filename)
        self.filename = None

    def SaveAs(self, force=False):
        """Save python script to file

        :return: filename
        """
        filename = ''
        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose file to save"),
            defaultFile=os.path.basename(
                self.parent.GetModelFile(
                    ext=False)),
            defaultDir=os.getcwd(),
            wildcard=_("Python script (*.py)|*.py"),
            style=wx.FD_SAVE)

        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if not filename:
            return ''

        # check for extension
        if filename[-3:] != ".py":
            filename += ".py"

        if os.path.exists(filename):
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "File <%s> already exists. "
                    "Do you want to overwrite this file?") %
                filename,
                caption=_("Save file"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
                return ''

            dlg.Destroy()

        fd = open(filename, "w")
        try:
            if force:
                WritePythonFile(fd, self.parent.GetModel())
            else:
                fd.write(self.body.GetText())
        finally:
            fd.close()

        # executable file
        os.chmod(filename, stat.S_IRWXU | stat.S_IWUSR)

        return filename

    def OnSaveAs(self, event):
        """Save python script to file"""
        self.SaveAs(force=False)
        event.Skip()

    def RefreshScript(self):
        """Refresh Python script

        :return: True on refresh
        :return: False script hasn't been updated
        """
        if self.body.modified:
            dlg = wx.MessageDialog(
                self,
                message=_(
                    "Python script is locally modificated. "
                    "Refresh will discard all changes. "
                    "Do you really want to continue?"),
                caption=_("Update"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)
            ret = dlg.ShowModal()
            dlg.Destroy()
            if ret == wx.ID_NO:
                return False

        fd = tempfile.TemporaryFile(mode='r+')
        WritePythonFile(fd, self.parent.GetModel())
        fd.seek(0)
        self.body.SetText(fd.read())
        fd.close()

        self.body.modified = False

        return True

    def OnRefresh(self, event):
        """Refresh Python script"""
        if self.RefreshScript():
            self.parent.SetStatusText(_('Python script is up-to-date'), 0)
        event.Skip()

    def IsModified(self):
        """Check if python script has been modified"""
        return self.body.modified

    def IsEmpty(self):
        """Check if python script is empty"""
        return len(self.body.GetText()) == 0
