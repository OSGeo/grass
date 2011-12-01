"""!
@package gmodeler.frame

@brief wxGUI Graphical Modeler for creating, editing, and managing models

Classes:
 - frame::ModelToolbar
 - frame::ModelFrame
 - frame::ModelCanvas
 - frame::ModelEvtHandler
 - frame::ModelListCtrl
 - frame::VariablePanel
 - frame::ValiableListCtrl
 - frame::ItemPanel
 - frame::ItemListCtrl
 - frame::ItemCheckListCtrl

(C) 2010-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import time
import stat
import textwrap
import tempfile
import copy
import re

import wx
from wx.lib import ogl
import wx.lib.flatnotebook    as FN
import wx.lib.mixins.listctrl as listmix

if __name__ == "__main__":
    sys.path.append(os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython'))
from core                 import globalvar
from gui_core.widgets     import GNotebook
from gui_core.goutput     import GMConsole
from core.debug           import Debug
from core.gcmd            import GMessage, GException, GWarning, GError, RunCommand
from gui_core.dialogs     import GetImageHandlers
from gui_core.preferences import PreferencesBaseDialog
from core.settings        import UserSettings
from core.menudata        import MenuData
from gui_core.toolbars    import BaseToolbar
from gui_core.menu        import Menu
from gmodeler.menudata    import ModelerData
from icons.icon           import Icons
from gui_core.forms       import GUI

from gmodeler.model       import *
from gmodeler.dialogs     import *

from grass.script import core as grass

class ModelToolbar(BaseToolbar):
    """!Graphical modeler toolbaro (see gmodeler.py)
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
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
                                     ('loop', icons['loop'],
                                      self.parent.OnDefineLoop),
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

class ModelFrame(wx.Frame):
    def __init__(self, parent, id = wx.ID_ANY,
                 title = _("GRASS GIS Graphical Modeler"), **kwargs):
        """!Graphical modeler main window
        
        @param parent parent window
        @param id window id
        @param title window title

        @param kwargs wx.Frames' arguments
        """
        self.parent = parent
        self.searchDialog = None # module search dialog
        self.baseTitle = title
        self.modelFile = None    # loaded model
        self.modelChanged = False
        
        self.cursors = {
            "default" : wx.StockCursor(wx.CURSOR_ARROW),
            "cross"   : wx.StockCursor(wx.CURSOR_CROSS),
            }
        
        wx.Frame.__init__(self, parent = parent, id = id, title = title, **kwargs)
        self.SetName("Modeler")
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self.menubar = Menu(parent = self, data = ModelerData())
        
        self.SetMenuBar(self.menubar)
        
        self.toolbar = ModelToolbar(parent = self)
        self.SetToolBar(self.toolbar)
        
        self.statusbar = self.CreateStatusBar(number = 1)
        
        self.notebook = GNotebook(parent = self,
                                  style = FN.FNB_FANCY_TABS | FN.FNB_BOTTOM |
                                  FN.FNB_NO_NAV_BUTTONS | FN.FNB_NO_X_BUTTON)
        
        self.canvas = ModelCanvas(self)
        self.canvas.SetBackgroundColour(wx.WHITE)
        self.canvas.SetCursor(self.cursors["default"])
        
        self.model = Model(self.canvas)
        
        self.variablePanel = VariablePanel(parent = self)
        
        self.itemPanel = ItemPanel(parent = self)
        
        self.goutput = GMConsole(parent = self, notebook = self.notebook)
        
        self.notebook.AddPage(page = self.canvas, text=_('Model'), name = 'model')
        self.notebook.AddPage(page = self.itemPanel, text=_('Items'), name = 'items')
        self.notebook.AddPage(page = self.variablePanel, text=_('Variables'), name = 'variables')
        self.notebook.AddPage(page = self.goutput, text=_('Command output'), name = 'output')
        wx.CallAfter(self.notebook.SetSelectionByName, 'model')
        wx.CallAfter(self.ModelChanged, False)

        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        self.Bind(wx.EVT_SIZE, self.OnSize)
        
        self._layout()
        self.SetMinSize((475, 300))
        self.SetSize((640, 480))
        
        # fix goutput's pane size
        if self.goutput:
            self.goutput.SetSashPosition(int(self.GetSize()[1] * .75))
        
    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        sizer.Add(item = self.notebook, proportion = 1,
                  flag = wx.EXPAND)
        
        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        sizer.Fit(self)
        
        self.Layout()

    def _addEvent(self, item):
        """!Add event to item"""
        evthandler = ModelEvtHandler(self.statusbar,
                                     self)
        evthandler.SetShape(item)
        evthandler.SetPreviousHandler(item.GetEventHandler())
        item.SetEventHandler(evthandler)

    def GetCanvas(self):
        """!Get canvas"""
        return self.canvas
    
    def GetModel(self):
        """!Get model"""
        return self.model
    
    def ModelChanged(self, changed = True):
        """!Update window title"""
        self.modelChanged = changed
        
        if self.modelFile:
            if self.modelChanged:
                self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.modelFile) + '*')
            else:
                self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.modelFile))
        else:
            self.SetTitle(self.baseTitle)
        
    def OnVariables(self, event):
        """!Switch to variables page"""
        self.notebook.SetSelectionByName('variables')
        
    def OnRemoveItem(self, event):
        """!Remove shape
        """
        self.GetCanvas().RemoveSelected()
        
    def OnCanvasRefresh(self, event):
        """!Refresh canvas"""
        self.SetStatusText(_("Redrawing model..."), 0)
        self.GetCanvas().Refresh()
        self.SetStatusText("", 0)

    def OnCmdRun(self, event):
        """!Run command"""
        try:
            action = self.GetModel().GetItems()[event.pid]
            if hasattr(action, "task"):
                action.Update(running = True)
        except IndexError:
            pass
        
    def OnCmdPrepare(self, event):
        """!Prepare for running command"""
        event.onPrepare(item = event.userData['item'],
                        params = event.userData['params'])
        
    def OnCmdDone(self, event):
        """!Command done (or aborted)"""
        try:
            action = self.GetModel().GetItems()[event.pid]
            if hasattr(action, "task"):
                action.Update(running = True)
        except IndexError:
            pass
        
    def OnCloseWindow(self, event):
        """!Close window"""
        if self.modelChanged and \
                UserSettings.Get(group='manager', key='askOnQuit', subkey='enabled'):
            if self.modelFile:
                message = _("Do you want to save changes in the model?")
            else:
                message = _("Do you want to store current model settings "
                            "to model file?")
            
            # ask user to save current settings
            dlg = wx.MessageDialog(self,
                                   message = message,
                                   caption=_("Quit Graphical Modeler"),
                                   style = wx.YES_NO | wx.YES_DEFAULT |
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
        """!Open preferences dialog"""
        dlg = PreferencesDialog(parent = self)
        dlg.CenterOnParent()
        
        dlg.ShowModal()
        self.canvas.Refresh()
        
    def OnHelp(self, event):
        """!Show help"""
        if self.parent and self.parent.GetName() == 'LayerManager':
            log = self.parent.GetLogWindow()
            log.RunCmd(['g.manual',
                        'entry=wxGUI.Modeler'])
        else:
            RunCommand('g.manual',
                       quiet = True,
                       entry = 'wxGUI.Modeler')
        
    def OnModelProperties(self, event):
        """!Model properties dialog"""
        dlg = PropertiesDialog(parent = self)
        dlg.CentreOnParent()
        properties = self.model.GetProperties()
        dlg.Init(properties)
        if dlg.ShowModal() == wx.ID_OK:
            self.ModelChanged()
            for key, value in dlg.GetValues().iteritems():
                properties[key] = value
            for action in self.model.GetItems(objType = ModelAction):
                action.GetTask().set_flag('overwrite', properties['overwrite'])
        
        dlg.Destroy()
        
    def OnDeleteData(self, event):
        """!Delete intermediate data"""
        rast, vect, rast3d, msg = self.model.GetIntermediateData()
        
        if not rast and not vect and not rast3d:
            GMessage(parent = self,
                     message = _('Nothing to delete.'))
            return
        
        dlg = wx.MessageDialog(parent = self,
                               message= _("Do you want to permanently delete data?%s" % msg),
                               caption=_("Delete intermediate data?"),
                               style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
        
        ret = dlg.ShowModal()
        if ret == wx.ID_YES:
            dlg.Destroy()
            
            if rast:
                self.goutput.RunCmd(['g.remove', 'rast=%s' %','.join(rast)])
            if rast3d:
                self.goutput.RunCmd(['g.remove', 'rast3d=%s' %','.join(rast3d)])
            if vect:
                self.goutput.RunCmd(['g.remove', 'vect=%s' %','.join(vect)])
            
            self.SetStatusText(_("%d maps deleted from current mapset") % \
                                 int(len(rast) + len(rast3d) + len(vect)))
            return
        
        dlg.Destroy()
                
    def OnModelNew(self, event):
        """!Create new model"""
        Debug.msg(4, "ModelFrame.OnModelNew():")
        
        # ask user to save current model
        if self.modelFile and self.modelChanged:
            self.OnModelSave()
        elif self.modelFile is None and \
                (self.model.GetNumItems() > 0 or len(self.model.GetData()) > 0):
            dlg = wx.MessageDialog(self, message=_("Current model is not empty. "
                                                   "Do you want to store current settings "
                                                   "to model file?"),
                                   caption=_("Create new model?"),
                                   style=wx.YES_NO | wx.YES_DEFAULT |
                                   wx.CANCEL | wx.ICON_QUESTION)
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
        
    def OnModelOpen(self, event):
        """!Load model from file"""
        filename = ''
        dlg = wx.FileDialog(parent = self, message=_("Choose model file"),
                            defaultDir = os.getcwd(),
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
        self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.modelFile))
        self.SetStatusText(_('%(items)d items (%(actions)d actions) loaded into model') % \
                               { 'items' : self.model.GetNumItems(),
                                 'actions' : self.model.GetNumItems(actionOnly = True) }, 0)
        
    def OnModelSave(self, event = None):
        """!Save model to file"""
        if self.modelFile and self.modelChanged:
            dlg = wx.MessageDialog(self, message=_("Model file <%s> already exists. "
                                                   "Do you want to overwrite this file?") % \
                                       self.modelFile,
                                   caption=_("Save model"),
                                   style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
            else:
                Debug.msg(4, "ModelFrame.OnModelSave(): filename=%s" % self.modelFile)
                self.WriteModelFile(self.modelFile)
                self.SetStatusText(_('File <%s> saved') % self.modelFile, 0)
                self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.modelFile))
        elif not self.modelFile:
            self.OnModelSaveAs(None)
        
    def OnModelSaveAs(self, event):
        """!Create model to file as"""
        filename = ''
        dlg = wx.FileDialog(parent = self,
                            message = _("Choose file to save current model"),
                            defaultDir = os.getcwd(),
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
            dlg = wx.MessageDialog(parent = self,
                                   message=_("Model file <%s> already exists. "
                                             "Do you want to overwrite this file?") % filename,
                                   caption=_("File already exists"),
                                   style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() != wx.ID_YES:
                dlg.Destroy()
                return
        
        Debug.msg(4, "GMFrame.OnModelSaveAs(): filename=%s" % filename)
        
        self.WriteModelFile(filename)
        self.modelFile = filename
        self.SetTitle(self.baseTitle + " - " + os.path.basename(self.modelFile))
        self.SetStatusText(_('File <%s> saved') % self.modelFile, 0)

    def OnModelClose(self, event = None):
        """!Close model file"""
        Debug.msg(4, "ModelFrame.OnModelClose(): file=%s" % self.modelFile)
        # ask user to save current model
        if self.modelFile and self.modelChanged:
            self.OnModelSave()
        elif self.modelFile is None and \
                (self.model.GetNumItems() > 0 or len(self.model.GetData()) > 0):
            dlg = wx.MessageDialog(self, message=_("Current model is not empty. "
                                                   "Do you want to store current settings "
                                                   "to model file?"),
                                   caption=_("Create new model?"),
                                   style=wx.YES_NO | wx.YES_DEFAULT |
                                   wx.CANCEL | wx.ICON_QUESTION)
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
        """!Run entire model"""
        self.model.Run(self.goutput, self.OnDone, parent = self)
        
    def OnDone(self, cmd, returncode):
        """!Computation finished"""
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
        
    def OnValidateModel(self, event, showMsg = True):
        """!Validate entire model"""
        if self.model.GetNumItems() < 1:
            GMessage(parent = self, 
                     message = _('Model is empty. Nothing to validate.'))
            return
        
        
        self.SetStatusText(_('Validating model...'), 0)
        errList = self.model.Validate()
        self.SetStatusText('', 0)
        
        if errList:
            GWarning(parent = self,
                     message = _('Model is not valid.\n\n%s') % '\n'.join(errList))
        else:
            GMessage(parent = self,
                     message = _('Model is valid.'))
    
    def OnExportImage(self, event):
        """!Export model to image (default image)
        """
        xminImg = 0
        xmaxImg = 0
        yminImg = 0
        ymaxImg = 0
        # get current size of canvas
        for shape in self.canvas.GetDiagram().GetShapeList():
            w, h = shape.GetBoundingBoxMax()
            x    = shape.GetX()
            y    = shape.GetY()
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
        bitmap = wx.EmptyBitmap(width = size.width, height = size.height)
        
        filetype, ltype = GetImageHandlers(wx.ImageFromBitmap(bitmap))
        
        dlg = wx.FileDialog(parent = self,
                            message = _("Choose a file name to save the image (no need to add extension)"),
                            defaultDir = "",
                            defaultFile = "",
                            wildcard = filetype,
                            style=wx.SAVE | wx.FD_OVERWRITE_PROMPT)
        
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            if not path:
                dlg.Destroy()
                return
            
            base, ext = os.path.splitext(path)
            fileType = ltype[dlg.GetFilterIndex()]['type']
            extType  = ltype[dlg.GetFilterIndex()]['ext']
            if ext != extType:
                path = base + '.' + extType
            
            dc = wx.MemoryDC(bitmap)
            dc.SetBackground(wx.WHITE_BRUSH)
            dc.SetBackgroundMode(wx.SOLID)
            
            dc.BeginDrawing()
            self.canvas.GetDiagram().Clear(dc)
            self.canvas.GetDiagram().Redraw(dc)
            dc.EndDrawing()
            
            bitmap.SaveFile(path, fileType)
            self.SetStatusText(_("Model exported to <%s>") % path)
        
        dlg.Destroy()
        
    def OnExportPython(self, event):
        """!Export model to Python script"""
        filename = ''
        dlg = wx.FileDialog(parent = self,
                            message = _("Choose file to save"),
                            defaultDir = os.getcwd(),
                            wildcard=_("Python script (*.py)|*.py"),
                            style=wx.FD_SAVE)
        
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
        
        if not filename:
            return
        
        # check for extension
        if filename[-3:] != ".py":
            filename += ".py"
        
        if os.path.exists(filename):
            dlg = wx.MessageDialog(self, message=_("File <%s> already exists. "
                                                   "Do you want to overwrite this file?") % filename,
                                   caption=_("Save file"),
                                   style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
                return
            
            dlg.Destroy()
        
        fd = open(filename, "w")
        try:
            WritePythonFile(fd, self.model)
        finally:
            fd.close()
        
        # executable file
        os.chmod(filename, stat.S_IRWXU | stat.S_IWUSR)
        
        self.SetStatusText(_("Model exported to <%s>") % filename)

    def OnDefineRelation(self, event):
        """!Define relation between data and action items"""
        self.canvas.SetCursor(self.cursors["cross"])
        self.defineRelation = { 'from' : None,
                                'to'   : None }
        
    def OnDefineLoop(self, event):
        """!Define new loop in the model"""
        self.ModelChanged()
        
        width, height = self.canvas.GetSize()
        loop = ModelLoop(self, x = width/2, y = height/2,
                         id = self.model.GetNumItems() + 1)
        self.canvas.diagram.AddShape(loop)
        loop.Show(True)
        
        self._addEvent(loop)
        self.model.AddItem(loop)
        
        self.canvas.Refresh()
        
    def OnDefineCondition(self, event):
        """!Define new condition in the model"""
        self.ModelChanged()
        
        width, height = self.canvas.GetSize()
        cond = ModelCondition(self, x = width/2, y = height/2,
                              id = self.model.GetNumItems() + 1)
        self.canvas.diagram.AddShape(cond)
        cond.Show(True)
        
        self._addEvent(cond)
        self.model.AddItem(cond)
        
        self.canvas.Refresh()
    
    def OnAddAction(self, event):
        """!Add action to model"""
        if self.searchDialog is None:
            self.searchDialog = ModelSearchDialog(self)
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
        width, height = self.canvas.GetSize()
        action = ModelAction(self.model, cmd = cmd, x = width/2, y = height/2,
                             id = self.model.GetNextId())
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
            if action.IsValid():
                self.GetOptData(dcmd = action.GetLog(string = False), layer = action,
                                params = action.GetParams(), propwin = None)
            else:
                GUI(parent = self, show = True).ParseCommand(action.GetLog(string = False),
                                                             completed = (self.GetOptData, action, action.GetParams()))
        elif win and not win.IsShown():
            win.Show()
        
        if win:
            win.Raise()
        
    def OnAddData(self, event):
        """!Add data item to model
        """
        # add action to canvas
        width, height = self.canvas.GetSize()
        data = ModelData(self, x = width/2, y = height/2)
       
        dlg = ModelDataDialog(parent = self, shape = data)
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
        
        
    def OnHelp(self, event):
        """!Display manual page"""
        grass.run_command('g.manual',
                          entry = 'wxGUI.Modeler')

    def OnAbout(self, event):
        """!Display About window"""
        info = wx.AboutDialogInfo()

        info.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        info.SetName(_('wxGUI Graphical Modeler'))
        info.SetWebSite('http://grass.osgeo.org')
        year = grass.version()['date']
        info.SetDescription(_('(C) 2010-%s by the GRASS Development Team\n\n') % year + 
                            '\n'.join(textwrap.wrap(_('This program is free software under the GNU General Public License'
                                                      '(>=v2). Read the file COPYING that comes with GRASS for details.'), 75)))
        
        wx.AboutBox(info)
        
    def GetOptData(self, dcmd, layer, params, propwin):
        """!Process action data"""
        if params: # add data items
            width, height = self.canvas.GetSize()
            x = [width/2 + 200, width/2 - 200]
            for p in params['params']:
                if p.get('prompt', '') in ('raster', 'vector', 'raster3d') and \
                        (p.get('value', None) or \
                             (p.get('age', 'old') != 'old' and p.get('required', 'no') == 'yes')):
                    data = layer.FindData(p.get('name', ''))
                    if data:
                        data.SetValue(p.get('value', ''))
                        data.Update()
                        continue
                    
                    data = self.model.FindData(p.get('value', ''),
                                               p.get('prompt', ''))
                    if data:
                        if p.get('age', 'old') == 'old':
                            rel = ModelRelation(parent = self, fromShape = data,
                                                toShape = layer, param = p.get('name', ''))
                        else:
                            rel = ModelRelation(parent = self, fromShape = layer,
                                                toShape = data, param = p.get('name', ''))
                        layer.AddRelation(rel)
                        data.AddRelation(rel)
                        self.AddLine(rel)
                        data.Update()
                        continue
                    
                    data = ModelData(self, value = p.get('value', ''),
                                     prompt = p.get('prompt', ''),
                                     x = x.pop(), y = height/2)
                    self._addEvent(data)
                    self.canvas.diagram.AddShape(data)
                    data.Show(True)
                                                            
                    if p.get('age', 'old') == 'old':
                        rel = ModelRelation(parent = self, fromShape = data,
                                            toShape = layer, param = p.get('name', ''))
                    else:
                        rel = ModelRelation(parent = self, fromShape = layer,
                                            toShape = data, param = p.get('name', ''))
                    layer.AddRelation(rel)
                    data.AddRelation(rel)
                    self.AddLine(rel)
                    data.Update()
            
            # valid / parameterized ?
            layer.SetValid(params)
            
            self.canvas.Refresh()
        
        if dcmd:
            layer.SetProperties(params, propwin)
            
        self.SetStatusText(layer.GetLog(), 0)
        
    def AddLine(self, rel):
        """!Add connection between model objects
        
        @param rel relation
        """
        fromShape = rel.GetFrom()
        toShape   = rel.GetTo()
        
        rel.SetCanvas(self)
        rel.SetPen(wx.BLACK_PEN)
        rel.SetBrush(wx.BLACK_BRUSH)
        rel.AddArrow(ogl.ARROW_ARROW)
        points = rel.GetControlPoints()
        rel.MakeLineControlPoints(2)
        if points:
            for x, y in points:
                rel.InsertLineControlPoint(point = wx.RealPoint(x, y))
        
        self._addEvent(rel)
        try:
            fromShape.AddLine(rel, toShape)
        except TypeError:
            pass # bug when connecting ModelCondition and ModelLoop - to be fixed
        
        self.canvas.diagram.AddShape(rel)
        rel.Show(True)
        
    def LoadModelFile(self, filename):
        """!Load model definition stored in GRASS Model XML file (gxm)
        """
        try:
            self.model.LoadModel(filename)
        except GException, e:
            GError(parent = self,
                   message = _("Reading model file <%s> failed.\n"
                               "Invalid file, unable to parse XML document.") % filename)
        
        self.modelFile = filename
        self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.modelFile))
        
        self.SetStatusText(_("Please wait, loading model..."), 0)
        
        # load actions
        for item in self.model.GetItems(objType = ModelAction):
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
        for item in self.model.GetItems(objType = ModelLoop):
            self._addEvent(item)
            self.canvas.diagram.AddShape(item)
            item.Show(True)
            
            # connect items in the loop
            self.DefineLoop(item)

        # load conditions
        for item in self.model.GetItems(objType = ModelCondition):
            self._addEvent(item)
            self.canvas.diagram.AddShape(item)
            item.Show(True)
            
            # connect items in the condition
            self.DefineCondition(item)
        
        # load variables
        self.variablePanel.Update()
        self.itemPanel.Update()
        self.SetStatusText('', 0)
        
        # final updates
        for action in self.model.GetItems(objType = ModelAction):
            action.SetValid(action.GetParams())
            action.Update()
        
        self.canvas.Refresh(True)
        
    def WriteModelFile(self, filename):
        """!Save model to model file, recover original file on error.
        
        @return True on success
        @return False on failure
        """
        self.ModelChanged(False)
        tmpfile = tempfile.TemporaryFile(mode='w+b')
        try:
            WriteModelFile(fd = tmpfile, model = self.model)
        except StandardError:
            GError(parent = self,
                   message = _("Writing current settings to model file failed."))
            return False
        
        try:
            mfile = open(filename, "w")
            tmpfile.seek(0)
            for line in tmpfile.readlines():
                mfile.write(line)
        except IOError:
            wx.MessageBox(parent = self,
                          message = _("Unable to open file <%s> for writing.") % filename,
                          caption = _("Error"),
                          style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return False
        
        mfile.close()
        
        return True
    
    def DefineLoop(self, loop):
        """!Define loop with given list of items"""
        parent = loop
        items = loop.GetItems()
        if not items:
            return
        
        # remove defined relations first
        for rel in loop.GetRelations():
            self.canvas.GetDiagram().RemoveShape(rel)
        loop.Clear()
        
        for item in items:
            rel = ModelRelation(parent = self, fromShape = parent, toShape = item)
            dx = item.GetX() - parent.GetX()
            dy = item.GetY() - parent.GetY()
            loop.AddRelation(rel)
            if dx != 0:
                rel.SetControlPoints(((parent.GetX(), parent.GetY() + dy / 2),
                                      (parent.GetX() + dx, parent.GetY() + dy / 2)))
            self.AddLine(rel)
            parent = item
        
        # close loop
        item = loop.GetItems()[-1]
        rel = ModelRelation(parent = self, fromShape = item, toShape = loop)
        loop.AddRelation(rel)
        self.AddLine(rel)
        dx = (item.GetX() - loop.GetX()) + loop.GetWidth() / 2 + 50
        dy = item.GetHeight() / 2 + 50
        rel.MakeLineControlPoints(0)
        rel.InsertLineControlPoint(point = wx.RealPoint(loop.GetX() - loop.GetWidth() / 2 ,
                                                        loop.GetY()))
        rel.InsertLineControlPoint(point = wx.RealPoint(item.GetX(),
                                                        item.GetY() + item.GetHeight() / 2))
        rel.InsertLineControlPoint(point = wx.RealPoint(item.GetX(),
                                                        item.GetY() + dy))
        rel.InsertLineControlPoint(point = wx.RealPoint(item.GetX() - dx,
                                                        item.GetY() + dy))
        rel.InsertLineControlPoint(point = wx.RealPoint(item.GetX() - dx,
                                                        loop.GetY()))
        
        self.canvas.Refresh()

    def DefineCondition(self, condition):
        """!Define if-else statement with given list of items"""
        parent = condition
        items = condition.GetItems()
        if not items['if'] and not items['else']:
            return
        
        # remove defined relations first
        for rel in condition.GetRelations():
            self.canvas.GetDiagram().RemoveShape(rel)
        condition.Clear()
        dxIf   = condition.GetX() + condition.GetWidth() / 2
        dxElse = condition.GetX() - condition.GetWidth() / 2
        dy     = condition.GetY()
        for branch in items.keys():
            for item in items[branch]:
                rel = ModelRelation(parent = self, fromShape = parent,
                                    toShape = item)
                condition.AddRelation(rel)
                self.AddLine(rel)
                rel.MakeLineControlPoints(0)
                if branch == 'if':
                    rel.InsertLineControlPoint(point = wx.RealPoint(item.GetX() - item.GetWidth() / 2, item.GetY()))
                    rel.InsertLineControlPoint(point = wx.RealPoint(dxIf, dy))
                else:
                    rel.InsertLineControlPoint(point = wx.RealPoint(dxElse, dy))
                    rel.InsertLineControlPoint(point = wx.RealPoint(item.GetX() - item.GetWidth() / 2, item.GetY()))
                parent = item
        
        self.canvas.Refresh()
        
class ModelCanvas(ogl.ShapeCanvas):
    """!Canvas where model is drawn"""
    def __init__(self, parent):
        self.parent = parent
        ogl.OGLInitialize()
        ogl.ShapeCanvas.__init__(self, parent)
        
        self.diagram = ogl.Diagram()
        self.SetDiagram(self.diagram)
        self.diagram.SetCanvas(self)
        
        self.SetScrollbars(20, 20, 1000/20, 1000/20)
        
        self.Bind(wx.EVT_CHAR,  self.OnChar)
        
    def OnChar(self, event):
        """!Key pressed"""
        kc = event.GetKeyCode()
        diagram = self.GetDiagram()
        if kc == wx.WXK_DELETE:
            self.RemoveSelected()
        
    def RemoveSelected(self):
        """!Remove selected shapes"""
        self.parent.ModelChanged()
        
        diagram = self.GetDiagram()
        for shape in diagram.GetShapeList():
            if not shape.Selected():
                continue
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
  
class ModelEvtHandler(ogl.ShapeEvtHandler):
    """!Model event handler class"""
    def __init__(self, log, frame):
        ogl.ShapeEvtHandler.__init__(self)
        self.log = log
        self.frame = frame
        self.x = self.y = None
        
    def OnLeftClick(self, x, y, keys = 0, attachment = 0):
        """!Left mouse button pressed -> select item & update statusbar"""
        shape = self.GetShape()
        canvas = shape.GetCanvas()
        dc = wx.ClientDC(canvas)
        canvas.PrepareDC(dc)
        
        if hasattr(self.frame, 'defineRelation'):
            drel = self.frame.defineRelation
            if drel['from'] is None:
                drel['from'] = shape
            elif drel['to'] is None:
                drel['to'] = shape
                rel = ModelRelation(parent = self.frame, fromShape = drel['from'],
                                    toShape = drel['to'])
                dlg = ModelRelationDialog(parent = self.frame,
                                          shape = rel)
                if dlg.IsValid():
                    ret = dlg.ShowModal()
                    if ret == wx.ID_OK:
                        option = dlg.GetOption()
                        rel.SetName(option)
                        drel['from'].AddRelation(rel)
                        drel['to'].AddRelation(rel)
                        drel['from'].Update()
                        params = { 'params' : [{ 'name' : option,
                                                 'value' : drel['from'].GetValue()}] }
                        drel['to'].MergeParams(params)
                        self.frame.AddLine(rel)
                
                    dlg.Destroy()
                del self.frame.defineRelation
        
        if shape.Selected():
            shape.Select(False, dc)
        else:
            redraw = False
            shapeList = canvas.GetDiagram().GetShapeList()
            toUnselect = list()
            
            for s in shapeList:
                if s.Selected():
                    toUnselect.append(s)
            
            shape.Select(True, dc)
            
            for s in toUnselect:
                s.Select(False, dc)
                
        canvas.Refresh(False)

        if hasattr(shape, "GetLog"):
            self.log.SetStatusText(shape.GetLog(), 0)
        else:
            self.log.SetStatusText('', 0)
        
    def OnLeftDoubleClick(self, x, y, keys = 0, attachment = 0):
        """!Left mouse button pressed (double-click) -> show properties"""
        self.OnProperties()
        
    def OnProperties(self, event = None):
        """!Show properties dialog"""
        self.frame.ModelChanged()
        shape = self.GetShape()
        if isinstance(shape, ModelAction):
            module = GUI(parent = self.frame, show = True).ParseCommand(shape.GetLog(string = False),
                                                                        completed = (self.frame.GetOptData, shape, shape.GetParams()))
        
        elif isinstance(shape, ModelData):
            dlg = ModelDataDialog(parent = self.frame, shape = shape)
            shape.SetPropDialog(dlg)
            dlg.CentreOnParent()
            dlg.Show()
        
        elif isinstance(shape, ModelLoop):
            dlg = ModelLoopDialog(parent = self.frame, shape = shape)
            dlg.CentreOnParent()
            if dlg.ShowModal() == wx.ID_OK:
                shape.SetText(dlg.GetCondition())
                alist = list()
                ids = dlg.GetItems()
                for aId in ids['unchecked']:
                    action = self.frame.GetModel().GetItem(aId)
                    action.UnSetBlock(shape)
                for aId in ids['checked']:
                    action = self.frame.GetModel().GetItem(aId)
                    action.SetBlock(shape)
                    if action:
                        alist.append(action)
                shape.SetItems(alist)
                self.frame.DefineLoop(shape)
                self.frame.SetStatusText(shape.GetLog(), 0)
            self.frame.GetCanvas().Refresh()
            
            dlg.Destroy()
        
        elif isinstance(shape, ModelCondition):
            dlg = ModelConditionDialog(parent = self.frame, shape = shape)
            dlg.CentreOnParent()
            if dlg.ShowModal() == wx.ID_OK:
                shape.SetText(dlg.GetCondition())
                ids = dlg.GetItems()
                for b in ids.keys():
                    alist = list()
                    for aId in ids[b]['unchecked']:
                        action = self.frame.GetModel().GetItem(aId)
                        action.UnSetBlock(shape)
                    for aId in ids[b]['checked']:
                        action = self.frame.GetModel().GetItem(aId)
                        action.SetBlock(shape)
                        if action:
                            alist.append(action)
                    shape.SetItems(alist, branch = b)
                self.frame.DefineCondition(shape)
            self.frame.GetCanvas().Refresh()
            
            dlg.Destroy()
                   
    def OnBeginDragLeft(self, x, y, keys = 0, attachment = 0):
        """!Drag shape (begining)"""
        self.frame.ModelChanged()
        if self._previousHandler:
            self._previousHandler.OnBeginDragLeft(x, y, keys, attachment)
        
    def OnEndDragLeft(self, x, y, keys = 0, attachment = 0):
        """!Drag shape (end)"""
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
        
    def OnEndSize(self, x, y):
        """!Resize shape"""
        self.frame.ModelChanged()
        if self._previousHandler:
            self._previousHandler.OnEndSize(x, y)
        
    def OnRightClick(self, x, y, keys = 0, attachment = 0):
        """!Right click -> pop-up menu"""
        if not hasattr (self, "popupID"):
            self.popupID = dict()
            for key in ('remove', 'enable', 'addPoint',
                        'delPoint', 'intermediate', 'props', 'id'):
                self.popupID[key] = wx.NewId()
        
        # record coordinates
        self.x = x
        self.y = y
        
        shape = self.GetShape()
        popupMenu = wx.Menu()
        popupMenu.Append(self.popupID['remove'], text=_('Remove'))
        self.frame.Bind(wx.EVT_MENU, self.OnRemove, id = self.popupID['remove'])
        if isinstance(shape, ModelAction) or isinstance(shape, ModelLoop):
            if shape.IsEnabled():
                popupMenu.Append(self.popupID['enable'], text=_('Disable'))
                self.frame.Bind(wx.EVT_MENU, self.OnDisable, id = self.popupID['enable'])
            else:
                popupMenu.Append(self.popupID['enable'], text=_('Enable'))
                self.frame.Bind(wx.EVT_MENU, self.OnEnable, id = self.popupID['enable'])
        
        if isinstance(shape, ModelRelation):
            popupMenu.AppendSeparator()
            popupMenu.Append(self.popupID['addPoint'], text=_('Add control point'))
            self.frame.Bind(wx.EVT_MENU, self.OnAddPoint, id = self.popupID['addPoint'])
            popupMenu.Append(self.popupID['delPoint'], text=_('Remove control point'))
            self.frame.Bind(wx.EVT_MENU, self.OnRemovePoint, id = self.popupID['delPoint'])
            if len(shape.GetLineControlPoints()) == 2:
                popupMenu.Enable(self.popupID['delPoint'], False)
        
        if isinstance(shape, ModelData) and '@' not in shape.GetValue():
            popupMenu.AppendSeparator()
            popupMenu.Append(self.popupID['intermediate'], text=_('Intermediate'),
                             kind = wx.ITEM_CHECK)
            if self.GetShape().IsIntermediate():
                popupMenu.Check(self.popupID['intermediate'], True)
            
            self.frame.Bind(wx.EVT_MENU, self.OnIntermediate, id = self.popupID['intermediate'])
            
        if isinstance(shape, ModelData) or \
                isinstance(shape, ModelAction) or \
                isinstance(shape, ModelLoop):
            popupMenu.AppendSeparator()
            popupMenu.Append(self.popupID['props'], text=_('Properties'))
            self.frame.Bind(wx.EVT_MENU, self.OnProperties, id = self.popupID['props'])
        
        self.frame.PopupMenu(popupMenu)
        popupMenu.Destroy()

    def OnDisable(self, event):
        """!Disable action"""
        self._onEnable(False)
        
    def OnEnable(self, event):
        """!Disable action"""
        self._onEnable(True)
        
    def _onEnable(self, enable):
        shape = self.GetShape()
        shape.Enable(enable)
        self.frame.ModelChanged()
        self.frame.canvas.Refresh()
        
    def OnAddPoint(self, event):
        """!Add control point"""
        shape = self.GetShape()
        shape.InsertLineControlPoint(point = wx.RealPoint(self.x, self.y))
        shape.ResetShapes()
        shape.Select(True)
        self.frame.ModelChanged()
        self.frame.canvas.Refresh()
        
    def OnRemovePoint(self, event):
        """!Remove control point"""
        shape = self.GetShape()
        shape.DeleteLineControlPoint()
        shape.Select(False)
        shape.Select(True)
        self.frame.ModelChanged()
        self.frame.canvas.Refresh()
        
    def OnIntermediate(self, event):
        """!Mark data as intermediate"""
        self.frame.ModelChanged()
        shape = self.GetShape()
        shape.SetIntermediate(event.IsChecked())
        self.frame.canvas.Refresh()

    def OnRemove(self, event):
        """!Remove shape
        """
        self.frame.GetCanvas().RemoveSelected()
        self.frame.itemPanel.Update()
       
class ModelListCtrl(wx.ListCtrl,
                    listmix.ListCtrlAutoWidthMixin,
                    listmix.TextEditMixin,
                    listmix.ColumnSorterMixin):
    def __init__(self, parent, columns, id = wx.ID_ANY,
                 style = wx.LC_REPORT | wx.BORDER_NONE |
                 wx.LC_SORT_ASCENDING |wx.LC_HRULES |
                 wx.LC_VRULES, **kwargs):
        """!List of model variables"""
        self.parent = parent
        self.columns = columns
        self.shape = None
        try:
            self.frame  = parent.parent
        except AttributeError:
            self.frame = None
        
        wx.ListCtrl.__init__(self, parent, id = id, style = style, **kwargs)
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.TextEditMixin.__init__(self)
        listmix.ColumnSorterMixin.__init__(self, 4)
        
        i = 0
        for col in columns:
            self.InsertColumn(i, col)
            self.SetColumnWidth(i, wx.LIST_AUTOSIZE_USEHEADER)
            i += 1
        
        self.itemDataMap = {} # requested by sorter
        self.itemCount   = 0
        
        self.Bind(wx.EVT_LIST_BEGIN_LABEL_EDIT, self.OnBeginEdit)
        self.Bind(wx.EVT_LIST_END_LABEL_EDIT, self.OnEndEdit)
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColClick)
        self.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnRightUp) #wxMSW
        self.Bind(wx.EVT_RIGHT_UP, self.OnRightUp)            #wxGTK
                
    def OnBeginEdit(self, event):
        """!Editing of item started"""
        event.Allow()

    def OnEndEdit(self, event):
        """!Finish editing of item"""
        pass
    
    def OnColClick(self, event):
        """!Click on column header (order by)"""
        event.Skip()

class VariablePanel(wx.Panel):
    def __init__(self, parent, id = wx.ID_ANY,
                 **kwargs):
        """!Manage model variables panel
        """
        self.parent = parent
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self.listBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                    label=" %s " % _("List of variables - right-click to delete"))
        
        self.list = VariableListCtrl(parent = self,
                                     columns = [_("Name"), _("Data type"),
                                                _("Default value"), _("Description")])
        
        # add new category
        self.addBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                   label = " %s " % _("Add new variable"))
        self.name = wx.TextCtrl(parent = self, id = wx.ID_ANY)
        wx.CallAfter(self.name.SetFocus)
        self.type = wx.Choice(parent = self, id = wx.ID_ANY,
                              choices = [_("integer"),
                                         _("float"),
                                         _("string"),
                                         _("raster"),
                                         _("vector"),
                                         _("mapset"),
                                         _("file")])
        self.type.SetSelection(2) # string
        self.value = wx.TextCtrl(parent = self, id = wx.ID_ANY)
        self.desc = wx.TextCtrl(parent = self, id = wx.ID_ANY)
        
        # buttons
        self.btnAdd = wx.Button(parent = self, id = wx.ID_ADD)
        self.btnAdd.SetToolTipString(_("Add new variable to the model"))
        self.btnAdd.Enable(False)
        
        # bindings
        self.name.Bind(wx.EVT_TEXT, self.OnText)
        self.value.Bind(wx.EVT_TEXT, self.OnText)
        self.desc.Bind(wx.EVT_TEXT, self.OnText)
        self.btnAdd.Bind(wx.EVT_BUTTON, self.OnAdd)
        
        self._layout()

    def _layout(self):
        """!Layout dialog"""
        listSizer = wx.StaticBoxSizer(self.listBox, wx.VERTICAL)
        listSizer.Add(item = self.list, proportion = 1,
                      flag = wx.EXPAND)
        
        addSizer = wx.StaticBoxSizer(self.addBox, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        gridSizer.AddGrowableCol(1)
        gridSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                           label = "%s:" % _("Name")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 0))
        gridSizer.Add(item = self.name,
                      pos = (0, 1),
                      flag = wx.EXPAND)
        gridSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                           label = "%s:" % _("Data type")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (0, 2))
        gridSizer.Add(item = self.type,
                      pos = (0, 3))
        gridSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                           label = "%s:" % _("Default value")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (1, 0))
        gridSizer.Add(item = self.value,
                      pos = (1, 1), span = (1, 3),
                      flag = wx.EXPAND)
        gridSizer.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                           label = "%s:" % _("Description")),
                      flag = wx.ALIGN_CENTER_VERTICAL,
                      pos = (2, 0))
        gridSizer.Add(item = self.desc,
                      pos = (2, 1), span = (1, 3),
                      flag = wx.EXPAND)
        addSizer.Add(item = gridSizer,
                     flag = wx.EXPAND)
        addSizer.Add(item = self.btnAdd, proportion = 0,
                     flag = wx.TOP | wx.ALIGN_RIGHT, border = 5)
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = listSizer, proportion = 1,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        mainSizer.Add(item = addSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALIGN_CENTER |
                      wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        
    def OnText(self, event):
        """!Text entered"""
        if self.name.GetValue():
            self.btnAdd.Enable()
        else:
            self.btnAdd.Enable(False)
    
    def OnAdd(self, event):
        """!Add new variable to the list"""
        msg = self.list.Append(self.name.GetValue(),
                               self.type.GetStringSelection(),
                               self.value.GetValue(),
                               self.desc.GetValue())
        self.name.SetValue('')
        self.name.SetFocus()
        
        if msg:
            GError(parent = self,
                   message = msg)
        else:
            self.type.SetSelection(2) # string
            self.value.SetValue('')
            self.desc.SetValue('')
            self.UpdateModelVariables()
        
    def UpdateModelVariables(self):
        """!Update model variables"""
        variables = dict()
        for values in self.list.GetData().itervalues():
            name = values[0]
            variables[name] = { 'type' : str(values[1]) }
            if values[2]:
                variables[name]['value'] = values[2]
            if values[3]:
                variables[name]['description'] = values[3]
        
        self.parent.GetModel().SetVariables(variables)
        self.parent.ModelChanged()

    def Update(self):
        """!Reload list of variables"""
        self.list.OnReload(None)
        
    def Reset(self):
        """!Remove all variables"""
        self.list.DeleteAllItems()
        self.parent.GetModel().SetVariables([])
        
class VariableListCtrl(ModelListCtrl):
    def __init__(self, parent, columns, **kwargs):
        """!List of model variables"""
        ModelListCtrl.__init__(self, parent, columns, **kwargs)

        self.SetColumnWidth(2, 200) # default value

    def GetListCtrl(self):
        """!Used by ColumnSorterMixin"""
        return self
    
    def GetData(self):
        """!Get list data"""
        return self.itemDataMap
    
    def Populate(self, data):
        """!Populate the list"""
        self.itemDataMap = dict()
        i = 0
        for name, values in data.iteritems():
            self.itemDataMap[i] = [name, values['type'],
                                   values.get('value', ''),
                                   values.get('description', '')]
            i += 1
        
        self.itemCount = len(self.itemDataMap.keys())
        self.DeleteAllItems()
        i = 0
        for name, vtype, value, desc in self.itemDataMap.itervalues():
            index = self.InsertStringItem(sys.maxint, name)
            self.SetStringItem(index, 0, name)
            self.SetStringItem(index, 1, vtype)
            self.SetStringItem(index, 2, value)
            self.SetStringItem(index, 3, desc)
            self.SetItemData(index, i)
            i += 1
        
    def Append(self, name, vtype, value, desc):
        """!Append new item to the list

        @return None on success
        @return error string
        """
        for iname, ivtype, ivalue, idesc in self.itemDataMap.itervalues():
            if iname == name:
                return _("Variable <%s> already exists in the model. "
                         "Adding variable failed.") % name
        
        index = self.InsertStringItem(sys.maxint, name)
        self.SetStringItem(index, 0, name)
        self.SetStringItem(index, 1, vtype)
        self.SetStringItem(index, 2, value)
        self.SetStringItem(index, 3, desc)
        self.SetItemData(index, self.itemCount)
        
        self.itemDataMap[self.itemCount] = [name, vtype, value, desc]
        self.itemCount += 1
        
        return None

    def OnRemove(self, event):
        """!Remove selected variable(s) from the model"""
        item = self.GetFirstSelected()
        while item != -1:
            self.DeleteItem(item)
            del self.itemDataMap[item]
            item = self.GetFirstSelected()
        self.parent.UpdateModelVariables()
        
        event.Skip()
        
    def OnRemoveAll(self, event):
        """!Remove all variable(s) from the model"""
        dlg = wx.MessageBox(parent=self,
                            message=_("Do you want to delete all variables from "
                                      "the model?"),
                            caption=_("Delete variables"),
                            style=wx.YES_NO | wx.CENTRE)
        if dlg != wx.YES:
            return
        
        self.DeleteAllItems()
        self.itemDataMap = dict()
        
        self.parent.UpdateModelVariables()
        
    def OnEndEdit(self, event):
        """!Finish editing of item"""
        itemIndex = event.GetIndex()
        columnIndex = event.GetColumn()
        nameOld = self.GetItem(itemIndex, 0).GetText()

        if columnIndex == 0: # TODO
            event.Veto()
        
        self.itemDataMap[itemIndex][columnIndex] = event.GetText()
        
        self.parent.UpdateModelVariables()

    def OnReload(self, event):
        """!Reload list of variables"""
        self.Populate(self.parent.parent.GetModel().GetVariables())

    def OnRightUp(self, event):
        """!Mouse right button up"""
        if not hasattr(self, "popupID1"):
            self.popupID1 = wx.NewId()
            self.popupID2 = wx.NewId()
            self.popupID3 = wx.NewId()
            self.Bind(wx.EVT_MENU, self.OnRemove,    id = self.popupID1)
            self.Bind(wx.EVT_MENU, self.OnRemoveAll, id = self.popupID2)
            self.Bind(wx.EVT_MENU, self.OnReload,    id = self.popupID3)
        
        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupID1, _("Delete selected"))
        menu.Append(self.popupID2, _("Delete all"))
        if self.GetFirstSelected() == -1:
            menu.Enable(self.popupID1, False)
            menu.Enable(self.popupID2, False)
        
        menu.AppendSeparator()
        menu.Append(self.popupID3, _("Reload"))
        
        self.PopupMenu(menu)
        menu.Destroy()

class ItemPanel(wx.Panel):
    def __init__(self, parent, id = wx.ID_ANY,
                 **kwargs):
        """!Manage model items
        """
        self.parent = parent
        
        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)
        
        self.listBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                    label=" %s " % _("List of items - right-click to delete"))
        
        self.list = ItemListCtrl(parent = self,
                                 columns = [_("ID"), _("Name"), _("In block"),
                                            _("Command / Condition")])
        
        self._layout()

    def _layout(self):
        """!Layout dialog"""
        listSizer = wx.StaticBoxSizer(self.listBox, wx.VERTICAL)
        listSizer.Add(item = self.list, proportion = 1,
                      flag = wx.EXPAND)
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = listSizer, proportion = 1,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        
    def Update(self):
        """!Reload list of variables"""
        self.list.OnReload(None)
        
class ItemListCtrl(ModelListCtrl):
    def __init__(self, parent, columns, disablePopup = False, **kwargs):
        """!List of model actions"""
        self.disablePopup = disablePopup
                
        ModelListCtrl.__init__(self, parent, columns, **kwargs)
        self.SetColumnWidth(1, 100)
        self.SetColumnWidth(2, 65)
        
    def GetListCtrl(self):
        """!Used by ColumnSorterMixin"""
        return self
    
    def GetData(self):
        """!Get list data"""
        return self.itemDataMap
    
    def Populate(self, data):
        """!Populate the list"""
        self.itemDataMap = dict()
        
        if self.shape:
            if isinstance(self.shape, ModelCondition):
                if self.GetName() == 'ElseBlockList':
                    shapeItems = map(lambda x: x.GetId(), self.shape.GetItems()['else'])
                else:
                    shapeItems = map(lambda x: x.GetId(), self.shape.GetItems()['if'])
            else:
                shapeItems = map(lambda x: x.GetId(), self.shape.GetItems())
        else:
            shapeItems = list()
        
        i = 0
        if len(self.columns) == 3: # ItemCheckList
            checked = list()
        for action in data:
            if isinstance(action, ModelData) or \
                    action == self.shape:
                continue
            
            if len(self.columns) == 3:
                self.itemDataMap[i] = [str(action.GetId()),
                                       action.GetName(),
                                       action.GetLog()]
                aId = action.GetBlockId()
                if action.GetId() in shapeItems:
                    checked.append(aId)
                else:
                    checked.append(None)
            else:
                bId = action.GetBlockId()
                if not bId:
                    bId = ''
                self.itemDataMap[i] = [str(action.GetId()),
                                       action.GetName(),
                                       ','.join(map(str, bId)),
                                       action.GetLog()]
            
            i += 1
        
        self.itemCount = len(self.itemDataMap.keys())
        self.DeleteAllItems()
        i = 0
        if len(self.columns) == 3:
            for aid, name, desc in self.itemDataMap.itervalues():
                index = self.InsertStringItem(sys.maxint, aid)
                self.SetStringItem(index, 0, aid)
                self.SetStringItem(index, 1, name)
                self.SetStringItem(index, 2, desc)
                self.SetItemData(index, i)
                if checked[i]:
                    self.CheckItem(index, True)
                i += 1
        else:
            for aid, name, inloop, desc in self.itemDataMap.itervalues():
                index = self.InsertStringItem(sys.maxint, aid)
                self.SetStringItem(index, 0, aid)
                self.SetStringItem(index, 1, name)
                self.SetStringItem(index, 2, inloop)
                self.SetStringItem(index, 3, desc)
                self.SetItemData(index, i)
                i += 1
                
    def OnRemove(self, event):
        """!Remove selected action(s) from the model"""
        model = self.frame.GetModel()
        canvas = self.frame.GetCanvas()
        
        item = self.GetFirstSelected()
        while item != -1:
            self.DeleteItem(item)
            del self.itemDataMap[item]
            
            aId = self.GetItem(item, 0).GetText()
            action = model.GetItem(int(aId))
            if not action:
                item = self.GetFirstSelected()
                continue
            
            model.RemoveItem(action)
            canvas.GetDiagram().RemoveShape(action)
            self.frame.ModelChanged()
            
            item = self.GetFirstSelected()
        
        canvas.Refresh()
        
        event.Skip()
    
    def OnRemoveAll(self, event):
        """!Remove all variable(s) from the model"""
        deleteDialog = wx.MessageBox(parent=self,
                                     message=_("Selected data records (%d) will permanently deleted "
                                               "from table. Do you want to delete them?") % \
                                         (len(self.listOfSQLStatements)),
                                     caption=_("Delete records"),
                                     style=wx.YES_NO | wx.CENTRE)
        if deleteDialog != wx.YES:
            return False
        
        self.DeleteAllItems()
        self.itemDataMap = dict()

        self.parent.UpdateModelVariables()

    def OnEndEdit(self, event):
        """!Finish editing of item"""
        itemIndex = event.GetIndex()
        columnIndex = event.GetColumn()
        
        self.itemDataMap[itemIndex][columnIndex] = event.GetText()
        
        aId = int(self.GetItem(itemIndex, 0).GetText())
        action = self.frame.GetModel().GetItem(aId)
        if not action:
            event.Veto()
        if columnIndex == 0:
            action.SetId(int(event.GetText()))
        
        self.frame.ModelChanged()

    def OnReload(self, event = None):
        """!Reload list of actions"""
        self.Populate(self.frame.GetModel().GetItems())

    def OnRightUp(self, event):
        """!Mouse right button up"""
        if self.disablePopup:
            return
        
        if not hasattr(self, "popupID1"):
            self.popupID1 = wx.NewId()
            self.popupID2 = wx.NewId()
            self.popupID3 = wx.NewId()
            self.popupID4 = wx.NewId()
            self.Bind(wx.EVT_MENU, self.OnRemove,    id = self.popupID1)
            self.Bind(wx.EVT_MENU, self.OnRemoveAll, id = self.popupID2)
            self.Bind(wx.EVT_MENU, self.OnReload,    id = self.popupID3)
            self.Bind(wx.EVT_MENU, self.OnNormalize, id = self.popupID4)

        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupID1, _("Delete selected"))
        menu.Append(self.popupID2, _("Delete all"))
        if self.GetFirstSelected() == -1:
            menu.Enable(self.popupID1, False)
            menu.Enable(self.popupID2, False)
        
        menu.AppendSeparator()
        menu.Append(self.popupID4, _("Normalize"))
        menu.Append(self.popupID3, _("Reload"))
        
        self.PopupMenu(menu)
        menu.Destroy()
    
    def OnNormalize(self, event):
        """!Update id of actions"""
        model = self.frame.GetModel()
        
        aId = 1
        for item in model.GetItems():
            item.SetId(aId)
            aId += 1
        
        self.OnReload(None)
        self.frame.GetCanvas().Refresh()
        self.frame.ModelChanged()

class ItemCheckListCtrl(ItemListCtrl, listmix.CheckListCtrlMixin):
    def __init__(self, parent, shape, columns, window = None, **kwargs):
        self.parent = parent
        self.window = window
        
        ItemListCtrl.__init__(self, parent, columns, disablePopup = True, **kwargs)
        listmix.CheckListCtrlMixin.__init__(self)
        self.SetColumnWidth(0, 50)
        
        self.shape  = shape
        
    def OnBeginEdit(self, event):
        """!Disable editing"""
        event.Veto()
        
    def OnCheckItem(self, index, flag):
        """!Item checked/unchecked"""
        name = self.GetName()
        if name == 'IfBlockList' and self.window:
            self.window.OnCheckItemIf(index, flag)
        elif name == 'ElseBlockList' and self.window:
            self.window.OnCheckItemElse(index, flag)
        
    def GetItems(self):
        """!Get list of selected actions"""
        ids = { 'checked'   : list(),
                'unchecked' : list() }
        for i in range(self.GetItemCount()):
            iId = int(self.GetItem(i, 0).GetText())
            if self.IsChecked(i):
                ids['checked'].append(iId)
            else:
                ids['unchecked'].append(iId)
            
        return ids

    def CheckItemById(self, aId, flag):
        """!Check/uncheck given item by id"""
        for i in range(self.GetItemCount()):
            iId = int(self.GetItem(i, 0).GetText())
            if iId == aId:
                self.CheckItem(i, flag)
                break
        
def main():
    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    
    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()
    frame = ModelFrame(parent = None)
    if len(sys.argv) > 1:
        frame.LoadModelFile(sys.argv[1])
    frame.Show()
    
    app.MainLoop()
    
if __name__ == "__main__":
    main()
