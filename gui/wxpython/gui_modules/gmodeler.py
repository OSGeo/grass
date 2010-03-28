"""!
@package gmodeler.py

@brief Graphical modeler to create edit, and manage models

Classes:
 - ModelFrame
 - ModelCanvas
 - ModelAction
 - ModelSearchDialog
 - ModelData

(C) 2010 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import shlex
import time

import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()
import wx
import wx.lib.ogl as ogl

import menu
import menudata
import toolbars
import menuform
import prompt

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
        
        wx.Frame.__init__(self, parent = parent, id = id, title = title, **kwargs)
        self.SetName("Modeler")
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self.menubar = menu.Menu(parent = self, data = menudata.ModelerData())
        
        self.SetMenuBar(self.menubar)
        
        self.toolbar = toolbars.ModelToolbar(parent = self)
        self.SetToolBar(self.toolbar)

        self.statusbar = self.CreateStatusBar(number = 1)
        
        self.canvas = ModelCanvas(self)
        self.canvas.SetBackgroundColour(wx.WHITE)
        
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        
        self._layout()
        self.SetMinSize((640, 480))
        
    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        sizer.Add(item = self.canvas, proportion = 1,
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
        pass

    def OnModelSave(self, event):
        """!Save model to file"""
        pass

    def OnModelSaveAs(self, event):
        """!Create model to file as"""
        pass

    def OnRunModel(self, event):
        """!Run entire model"""
        pass

    def OnValidateModel(self, event):
        """!Validate entire model"""
        for s in self.actions:
            print s
        
    def OnRemoveItem(self, event):
        """!Remove item from model"""
        pass

    def OnAddAction(self, event):
        """!Add action to model"""
        debug = False
        if debug == False:
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
        else:
            cmd = ['r.buffer']
        
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
                    
                    # connect with action
                    line = ogl.LineShape()
                    line.SetCanvas(self)
                    line.SetPen(wx.BLACK_PEN)
                    line.SetBrush(wx.BLACK_BRUSH)
                    line.AddArrow(ogl.ARROW_ARROW)
                    line.MakeLineControlPoints(2)
                    if p.get('age', 'old') == 'old':
                        data.AddLine(line, layer)
                    else:
                        layer.AddLine(line, data)
                    self.canvas.diagram.AddShape(line)
                    line.Show(True)
            
            self.canvas.Refresh()
        
        self.SetStatusText(layer.GetLog(), 0)
        
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
        self.cmd     = dcmd
        self.params  = params
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

class ModelData(ogl.EllipseShape):
    """!Data item class"""
    def __init__(self, parent, x, y, name = '', value = '', prompt = '', width = 175, height = 50):
        self.parent  = parent
        self.name    = name
        self.value   = value
        self.prompt  = prompt
        
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
        if not win:
            module = menuform.GUI().ParseCommand(shape.cmd,
                                                 completed = (self.frame.GetOptData, shape, None),
                                                 parentframe = self.frame, show = True)
        
        elif not win.IsShown():
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
        
def main():
    app = wx.PySimpleApp()
    frame = ModelFrame(parent = None)
    # frame.CentreOnScreen()
    frame.Show()
    
    app.MainLoop()
    
if __name__ == "__main__":
    main()
