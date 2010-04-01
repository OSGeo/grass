"""!
@package gmodeler.py

@brief Graphical modeler to create edit, and manage models

Classes:
 - ModelFrame
 - ModelCanvas
 - ModelAction
 - ModelSearchDialog
 - ModelData
 - ProcessModelFile
 - WriteModelFile

(C) 2010 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import shlex
import time
import traceback

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()
import wx
import wx.lib.ogl as ogl
import wx.lib.flatnotebook as FN

import menu
import menudata
import toolbars
import menuform
import prompt
import utils
import goutput
from   debug import Debug
from   gcmd import GMessage

from grass.script import core as grass

class ModelFrame(wx.Frame):
    def __init__(self, parent, id = wx.ID_ANY, title = _("Graphical modeler (under development)"), **kwargs):
        """!Graphical modeler main window
        
        @param parent parent window
        @param id window id
        @param title window title

        @param kwargs wx.Frames' arguments
        """
        self.parent = parent
        self.searchDialog = None # module search dialog
        self.actions = list()    # list of recorded actions
        self.data    = list()    # list of recorded data items
        self.baseTitle = title
        self.modelFile = None    # loaded model
        
        wx.Frame.__init__(self, parent = parent, id = id, title = title, **kwargs)
        self.SetName("Modeler")
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self.menubar = menu.Menu(parent = self, data = menudata.ModelerData())
        
        self.SetMenuBar(self.menubar)
        
        self.toolbar = toolbars.ModelToolbar(parent = self)
        self.SetToolBar(self.toolbar)

        self.statusbar = self.CreateStatusBar(number = 1)

        self.notebook = FN.FlatNotebook(parent = self, id = wx.ID_ANY,
                                        style = FN.FNB_FANCY_TABS | FN.FNB_BOTTOM |
                                        FN.FNB_NO_NAV_BUTTONS | FN.FNB_NO_X_BUTTON)
        
        self.canvas = ModelCanvas(self)
        self.canvas.SetBackgroundColour(wx.WHITE)

        self.goutput = goutput.GMConsole(parent = self, pageid = 1)
                
        self.modelPage   = self.notebook.AddPage(self.canvas, text=_('Model'))
        self.commandPage = self.notebook.AddPage(self.goutput, text=_('Command output'))
        
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        
        self._layout()
        self.SetMinSize((640, 480))
        
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
        
    def OnCloseWindow(self, event):
        """!Close window"""
        self.Destroy()

    def OnModelNew(self, event):
        """!Create new model"""
        pass

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
        ### self.OnModelClose()
        
        self.LoadModelFile(filename)
        
        self.modelFile = filename
        self.SetTitle(self.baseTitle + " - " +  os.path.basename(self.modelFile))
        self.SetStatusText(_('%d actions loaded into model') % len(self.actions), 0)
        
    def OnModelSave(self, event):
        """!Save model to file"""
        if self.modelFile:
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
        else:
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
        
    def OnRunModel(self, event):
        """!Run entire model"""
        for action in self.actions:
            self.SetStatusText(_('Running model...'), 0)
            self.goutput.RunCmd(command = action.GetLog(string = False),
                                onDone = self.OnDone)
        
    def OnDone(self, returncode):
        """!Computation finished"""
        self.SetStatusText('', 0)
        
    def OnValidateModel(self, event):
        """!Validate entire model"""
        for s in self.actions:
            print s
        
    def OnRemoveItem(self, event):
        """!Remove item from model"""
        pass

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
        
        # add action to canvas
        width, height = self.canvas.GetSize()
        action = ModelAction(self, cmd = cmd, x = width/2, y = height/2)
        self.canvas.diagram.AddShape(action)
        action.Show(True)

        self._addEvent(action)
        self.actions.append(action)
        
        self.canvas.Refresh()
        time.sleep(.1)
        
        # show properties dialog
        win = action.GetPropDialog()
        if not win:
            module = menuform.GUI().ParseCommand(action.GetLog(string = False),
                                                 completed = (self.GetOptData, action, None),
                                                 parentframe = self, show = True)
        elif not win.IsShown():
            win.Show()
        if win:
            win.Raise()

    def OnAddData(self, event):
        """!Add data item to model"""
        # add action to canvas
        width, height = self.canvas.GetSize()
        data = ModelData(self, x = width/2, y = height/2)
        self.canvas.diagram.AddShape(data)
        data.Show(True)
        
        self._addEvent(data)
        self.data.append(data)
        
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
        info.SetDescription(_('(C) 2010 by the GRASS Development Team\n\n'
                              'This program is free software under the GNU General Public License'
                              '(>=v2). Read the file COPYING that comes with GRASS for details.'))
        
        wx.AboutBox(info)
        
    def GetOptData(self, dcmd, layer, params, propwin):
        """!Process action data"""
        layer.SetProperties(dcmd, params, propwin)
            
        if params: # add data items
            width, height = self.canvas.GetSize()
            x = [width/2 + 200, width/2 - 200]
            for p in params['params']:
                if p.get('value', None) and \
                        p.get('prompt', '') in ('raster', 'vector', 'raster3d'):
                    # create data item
                    data = ModelData(self, name = p.get('name', ''),
                                     value = p.get('value', ''),
                                     prompt = p.get('prompt', ''),
                                     x = x.pop(), y = height/2)
                    self.canvas.diagram.AddShape(data)
                    data.Show(True)
                    
                    self._addEvent(data)                    
                    self.data.append(data)
                    
                    if p.get('age', 'old') == 'old':
                        self._addLine(data, layer)
                        data.AddAction(layer, direction = 'from')
                    else:
                        self._addLine(layer, data)
                        data.AddAction(layer, direction = 'to')
            
            self.canvas.Refresh()
        
        self.SetStatusText(layer.GetLog(), 0)

    def _addLine(self, fromShape, toShape):
        """!Add connection

        @param fromShape from
        @param toShape to
        """
        line = ogl.LineShape()
        line.SetCanvas(self)
        line.SetPen(wx.BLACK_PEN)
        line.SetBrush(wx.BLACK_BRUSH)
        line.AddArrow(ogl.ARROW_ARROW)
        line.MakeLineControlPoints(2)
        
        fromShape.AddLine(line, toShape)
        
        self.canvas.diagram.AddShape(line)
        line.Show(True)
        
    def LoadModelFile(self, filename):
        """!Load model definition stored in GRASS Model XML file (gxm)

        @todo Validate against DTD

        Raise exception on error.
        """
        ### dtdFilename = os.path.join(globalvar.ETCWXDIR, "xml", "grass-gxm.dtd")
        
        # parse workspace file
        try:
            gxmXml = ProcessModelFile(etree.parse(filename))
        except:
            GMessage(parent = self,
                     message = _("Reading model file <%s> failed.\n"
                                 "Invalid file, unable to parse XML document.") % filename)
            return
        
        busy = wx.BusyInfo(message=_("Please wait, loading model..."),
                           parent=self)
        wx.Yield()
        
        # load actions
        for action in gxmXml.actions:
            actionShape = ModelAction(parent = self, 
                                      x = action['pos'][0],
                                      y = action['pos'][1],
                                      width = action['size'][0],
                                      height = action['size'][1],
                                      cmd = action['cmd'])
            self.canvas.diagram.AddShape(actionShape)
            actionShape.Show(True)
            
            self._addEvent(actionShape)
            self.actions.append(actionShape)
        
        # load data & connections
        for data in gxmXml.data:
            dataShape = ModelData(parent = self, 
                                  x = data['pos'][0],
                                  y = data['pos'][1],
                                  width = data['size'][0],
                                  height = data['size'][1],
                                  name = data['name'],
                                  prompt = data['prompt'],
                                  value = data['value'])
            self.canvas.diagram.AddShape(dataShape)
            dataShape.Show(True)
            
            self._addEvent(dataShape)
            self.data.append(dataShape)
            
            actionShape = self.actions[0]
            if data['from'] is True:
                self._addLine(dataShape, actionShape)
            elif data['from'] is False:
                self._addLine(actionShape, dataShape)
            
        self.canvas.Refresh(True)
        
    def WriteModelFile(self, filename):
        """!Save model to model file
        
        @return True on success
        @return False on failure
        """
        try:
            file = open(filename, "w")
        except IOError:
            wx.MessageBox(parent = self,
                          message = _("Unable to open file <%s> for writing.") % filename,
                          caption = _("Error"),
                          style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return False
        
        try:
            WriteModelFile(fd = file, actions = self.actions, data = self.data)
        except StandardError:
            file.close()
            
            GMessage(parent = self,
                     message = _("Writing current settings to model file failed."))
            
            return False
        
        file.close()
        
        return True
    
class ModelCanvas(ogl.ShapeCanvas):
    """!Canvas where model is drawn"""
    def __init__(self, parent):
        ogl.OGLInitialize()
        ogl.ShapeCanvas.__init__(self, parent)
        
        self.diagram = ogl.Diagram()
        self.SetDiagram(self.diagram)
        self.diagram.SetCanvas(self)
        
        self.SetScrollbars(20, 20, 1000/20, 1000/20)
        
class ModelAction(ogl.RectangleShape):
    """!Action class (GRASS module)"""
    def __init__(self, parent, x, y, cmd = None, width = 100, height = 50):
        self.parent  = parent
        self.cmd     = cmd
        self.params  = None
        self.propWin = None

        ogl.RectangleShape.__init__(self, width, height)
        
        # self.Draggable(True)
        self.SetCanvas(self.parent)
        self.SetX(x)
        self.SetY(y)
        self.SetPen(wx.BLACK_PEN)
        self.SetBrush(wx.LIGHT_GREY_BRUSH)
        if self.cmd and len(self.cmd) > 0:
            self.AddText(self.cmd[0])
        else:
            self.AddText('<<module>>')

    def SetProperties(self, dcmd, params, propwin):
        """!Record properties dialog"""
        self.cmd = dcmd
        self.params = params
        self.propWin = propwin

    def GetPropDialog(self):
        """!Get properties dialog"""
        return self.propWin

    def GetLog(self, string = True):
        """!Get logging info"""
        if string:
            if self.cmd is None:
                return ''
            else:
                return ' '.join(self.cmd)
        
        return self.cmd
    
    def GetName(self):
        """!Get name"""
        if self.cmd and len(self.cmd) > 0:
            return self.cmd[0]
        
        return _('unknown')

    def GetParams(self):
        """!Get dictionary of parameters"""
        return self.params
    
class ModelData(ogl.EllipseShape):
    """!Data item class"""
    def __init__(self, parent, x, y, name = '', value = '', prompt = '', width = 175, height = 50):
        self.parent  = parent
        self.name    = name
        self.value   = value
        self.prompt  = prompt
        
        self.actions = { 'from' : list(), 'to' : list() }
        
        ogl.EllipseShape.__init__(self, width, height)
        
        # self.Draggable(True)
        self.SetCanvas(self.parent)
        self.SetX(x)
        self.SetY(y)
        self.SetPen(wx.BLACK_PEN)
        if self.prompt == 'raster':
            self.SetBrush(wx.Brush(wx.Colour(215, 215, 248)))
        elif self.prompt == 'vector':
            self.SetBrush(wx.Brush(wx.Colour(248, 215, 215)))
        else:
            self.SetBrush(wx.LIGHT_GREY_BRUSH)
        
        if name:
            self.AddText(name)
            self.AddText(value)
        else:
            self.AddText(_('unknown'))

    def GetLog(self, string = True):
        """!Get logging info"""
        if self.name:
            return self.name + '=' + self.value + ' (' + self.prompt + ')'
        else:
            return _('unknown')

    def GetName(self):
        """!Get name"""
        return self.name

    def GetPrompt(self):
        """!Get prompt"""
        return self.prompt

    def GetValue(self):
        """!Get value"""
        return self.value
    
    def GetActions(self, direction):
        """!Get related actions

        @param direction direction - 'from' or 'to'
        """
        return self.actions[direction]

    def AddAction(self, action, direction):
        """!Record related actions

        @param action action to be recoreded
        @param direction direction of relation
        """
        self.actions[direction].append(action)

    def GetPropDialog(self):
        """!Get properties dialog"""
        return None
    
class ModelEvtHandler(ogl.ShapeEvtHandler):
    """!Model event handler class"""
    def __init__(self, log, frame):
        ogl.ShapeEvtHandler.__init__(self)
        self.log = log
        self.frame = frame
        
    def OnLeftClick(self, x, y, keys = 0, attachment = 0):
        """!Left mouse button pressed -> select item & update statusbar"""
        shape = self.GetShape()
        canvas = shape.GetCanvas()
        dc = wx.ClientDC(canvas)
        canvas.PrepareDC(dc)
        
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
        
        self.log.SetStatusText(shape.GetLog(), 0)
        
    def OnLeftDoubleClick(self, x, y, keys = 0, attachment = 0):
        """!Left mouse button pressed (double-click) -> show properties"""
        shape = self.GetShape()
        win = shape.GetPropDialog()
        if isinstance(shape, ModelAction) and not win:
            module = menuform.GUI().ParseCommand(shape.cmd,
                                                 completed = (self.frame.GetOptData, shape, None),
                                                 parentframe = self.frame, show = True)
        
        elif win and not win.IsShown():
            win.Show()
        
        if win:
            win.Raise()
            
class ModelSearchDialog(wx.Dialog):
    def __init__(self, parent, id = wx.ID_ANY, title = _("Find GRASS module"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        """!Graphical modeler module search window
        
        @param parent parent window
        @param id window id
        @param title window title

        @param kwargs wx.Dialogs' arguments
        """
        self.parent = parent
        
        wx.Dialog.__init__(self, parent = parent, id = id, title = title, **kwargs)
        self.SetName("ModelerDialog")
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.searchBy = wx.Choice(parent = self.panel, id = wx.ID_ANY,
                                  choices = [_("description"),
                                             _("keywords")])
        self.search = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY,
                                  value = "", size = (-1, 25))
        self.cmd_prompt = prompt.GPromptSTC(parent = self)
        
        self.btnCancel = wx.Button(self.panel, wx.ID_CANCEL)
        self.btnOk     = wx.Button(self.panel, wx.ID_OK)
        self.btnOk.SetDefault()

        self._layout()
        
    def _layout(self):
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()

        bodyBox = wx.StaticBox(parent=self.panel, id=wx.ID_ANY,
                               label=" %s " % _("Find GRASS module"))
        bodySizer = wx.StaticBoxSizer(bodyBox, wx.VERTICAL)
        searchSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        searchSizer.Add(item = self.searchBy,
                        proportion = 0, flag = wx.LEFT, border = 3)
        searchSizer.Add(item = self.search,
                        proportion = 1, flag = wx.LEFT | wx.EXPAND, border = 3)
        
        bodySizer.Add(item=searchSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=1)
        bodySizer.Add(item=self.cmd_prompt, proportion=1,
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, border=3)
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=bodySizer, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(item=btnSizer, proportion=0,
                      flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
        
        self.panel.SetSizer(mainSizer)
        mainSizer.Fit(self.panel)
        
    def GetPanel(self):
        """!Get dialog panel"""
        return self.panel

    def GetCmd(self):
        """!Get command"""
        line = self.cmd_prompt.GetCurLine()[0].strip()
        if len(line) == 0:
            list()
        
        try:
            cmd = shlex.split(str(line))
        except UnicodeError:
            cmd = shlex.split(utils.EncodeString((line)))
            
        return cmd

    def OnOk(self, event):
        self.btnOk.SetFocus()
        
    def Reset(self):
        """!Reset dialog"""
        self.searchBy.SetSelection(0)
        self.search.SetValue('')
        self.cmd_prompt.OnCmdErase(None)

class ProcessModelFile:
    """!Process GRASS model file (gxm)"""
    def __init__(self, tree):
        """!A ElementTree handler for the GXM XML file, as defined in
        grass-gxm.dtd.
        """
        self.tree = tree
        self.root = self.tree.getroot()

        # list of actions, data
        self.actions = list()
        self.data    = list()
        
        self._processActions()
        self._processData()
        
    def _filterValue(self, value):
        """!Filter value
        
        @param value
        """
        value = value.replace('&lt;', '<')
        value = value.replace('&gt;', '>')
        
        return value
        
    def _getNodeText(self, node, tag, default = ''):
        """!Get node text"""
        p = node.find(tag)
        if p is not None:
            return utils.normalize_whitespace(p.text)
        
        return default
    
    def _processActions(self):
        """!Process model file"""
        for action in self.root.findall('action'):
            pos, size = self._getDim(action)
            
            task = action.find('task')
            if task:
                cmd = self._processTask(task)
            else:
                cmd = None
            
            self.actions.append({ 'pos' : pos,
                                  'size': size,
                                  'cmd' : cmd })
            
    def _getDim(self, node):
        """!Get position and size of shape"""
        pos = size = None
        posAttr = node.get('pos', None)
        if posAttr:
            posVal = map(int, posAttr.split(','))
            try:
                pos = (posVal[0], posVal[1])
            except:
                pos = None
        
        sizeAttr = node.get('size', None)
        if sizeAttr:
            sizeVal = map(int, sizeAttr.split(','))
            try:
                size = (sizeVal[0], sizeVal[1])
            except:
                size = None
        
        return pos, size        
    
    def _processData(self):
        """!Process model file"""
        for data in self.root.findall('data'):
            pos, size = self._getDim(data)
            param = data.find('parameter')
            name = prompt = value = None
            if param is not None:
                name = param.get('name', None)
                prompt = param.get('prompt', None)
                value = self._filterValue(self._getNodeText(param, 'value'))
                
            action = data.find('action')
            aId = fromDir = None
            if action is not None:
                aId = int(action.get('id', None))
                if action.get('dir', 'to') == 'to':
                    fromDir = False
                else:
                    fromDir = True
            
            self.data.append({ 'pos' : pos,
                               'size': size,
                               'name' : name,
                               'prompt' : prompt,
                               'value' : value,
                               'id' : aId,
                               'from' : fromDir })
        
    def _processTask(self, node):
        """!Process task"""
        cmd = list()
        name = node.get('name', None)
        if not name:
            return cmd
        cmd.append(name)
        
        # flags
        for p in node.findall('flag'):
            flag = p.get('name', '')
            if len(flag) > 1:
                cmd.append('--' + flag)
            else:
                cmd.append('-' + flag)
        # parameters
        for p in node.findall('parameter'):
            cmd.append('%s=%s' % (p.get('name', ''),
                                  self._filterValue(self._getNodeText(p, 'value'))))
        return cmd
    
class WriteModelFile:
    """!Generic class for writing model file"""
    def __init__(self, fd, actions, data):
        self.fd      = fd
        self.actions = actions
        self.data    = data
        
        self.indent = 0
        
        self._header()
        
        self._actions()
        self._data()
        
        self._footer()

    def _filterValue(self, value):
        """!Make value XML-valid"""
        value = value.replace('<', '&lt;')
        value = value.replace('>', '&gt;')
        
        return value
        
    def _header(self):
        """!Write header"""
        self.fd.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        self.fd.write('<!DOCTYPE gxm SYSTEM "grass-gxm.dtd">\n')
        self.fd.write('%s<gxm>\n' % (' ' * self.indent))
                
    def _footer(self):
        """!Write footer"""
        self.fd.write('%s</gxm>\n' % (' ' * self.indent))
        
    def _actions(self):
        """!Write actions"""
        id = 1
        self.indent += 4
        for action in self.actions:
            self.fd.write('%s<action id="%d" name="%s" pos="%d,%d" size="%d,%d">\n' % \
                              (' ' * self.indent, id, action.GetName(), action.GetX(), action.GetY(),
                               action.GetWidth(), action.GetHeight()))
            self.indent += 4
            self.fd.write('%s<task name="%s">\n' % (' ' * self.indent, action.GetLog(string = False)[0]))
            self.indent += 4
            for key, val in action.GetParams().iteritems():
                if key == 'flags':
                    for f in val:
                        if f.get('value', False):
                            self.fd.write('%s<flag name="%s" />\n' %
                                          (' ' * self.indent, f.get('name', '')))
                else: # parameter
                    for p in val:
                        if not p.get('value', ''):
                            continue
                        self.fd.write('%s<parameter name="%s">\n' %
                                      (' ' * self.indent, p.get('name', '')))
                        self.indent += 4
                        self.fd.write('%s<value>%s</value>\n' %
                                      (' ' * self.indent, self._filterValue(p.get('value', ''))))
                        self.indent -= 4
                        self.fd.write('%s</parameter>\n' % (' ' * self.indent))
            self.indent -= 4
            self.fd.write('%s</task>\n' % (' ' * self.indent))
            self.indent -= 4
            self.fd.write('%s</action>\n' % (' ' * self.indent))
            id += 1
        
        self.indent -= 4
        
    def _data(self):
        """!Write data"""
        self.indent += 4
        for data in self.data:
            self.fd.write('%s<data pos="%d,%d" size="%d,%d">\n' % \
                              (' ' * self.indent, data.GetX(), data.GetY(),
                               data.GetWidth(), data.GetHeight()))
            self.indent += 4
            self.fd.write('%s<parameter name="%s" prompt="%s">\n' % \
                              (' ' * self.indent, data.GetName(), data.GetPrompt()))
            self.indent += 4
            self.fd.write('%s<value>%s</value>\n' %
                          (' ' * self.indent, self._filterValue(data.GetValue())))
            self.indent -= 4
            self.fd.write('%s</parameter>\n' % (' ' * self.indent))
            self.indent -= 4
            for action in data.GetActions('from'):
                self.fd.write('%s<action id="1" dir="from" />\n' % \
                                  (' ' * self.indent))
            for action in data.GetActions('to'):
                self.fd.write('%s<action id="1" dir="to" />\n' % \
                                  (' ' * self.indent))
            
            self.fd.write('%s</data>\n' % (' ' * self.indent))
            
        self.indent -= 4
        
def main():
    app = wx.PySimpleApp()
    frame = ModelFrame(parent = None)
    if len(sys.argv) > 1:
        frame.LoadModelFile(sys.argv[1])
    # frame.CentreOnScreen()
    frame.Show()
    
    app.MainLoop()
    
if __name__ == "__main__":
    main()
