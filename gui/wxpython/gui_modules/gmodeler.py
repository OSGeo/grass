"""!
@package gmodeler.py

@brief Graphical modeler to create edit, and manage models

Classes:
 - ModelFrame
 - ModelCanvas
 - ModelAction
 - ModelSearchDialog

(C) 2010 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import shlex

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
        
        self.actions = list() # list of recoreded actions

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

    def OnAddAction(self, event):
        """!Add action to model"""
        dlg = ModelSearchDialog(self)
        dlg.CentreOnParent()
        
        if dlg.ShowModal() == wx.CANCEL:
            dlg.Destroy()
            return
        
        cmd = dlg.GetCmd()
        dlg.Destroy()
        
        action = ModelAction(self, cmd = cmd, x = 100, y = 100)
        self.canvas.diagram.AddShape(action)
        action.Show(True)
        
        evthandler = ModelEvtHandler(self.statusbar,
                                     self)
        evthandler.SetShape(action)
        evthandler.SetPreviousHandler(action.GetEventHandler())
        action.SetEventHandler(evthandler)
        
        self.actions.append(action)
        
        self.canvas.Refresh()
        
    def OnHelp(self, event):
        """!Display manual page"""
        grass.run_command('g.manual',
                          entry = 'wxGUI.Modeler')
        
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
        self.parent = parent
        
        ogl.RectangleShape.__init__(self, width, height)
        
        # self.Draggable(True)
        self.SetCanvas(self.parent)
        self.SetX(x)
        self.SetY(y)
        self.SetPen(wx.BLACK_PEN)
        self.SetBrush(wx.LIGHT_GREY_BRUSH)
        if cmd and len(cmd) > 0:
            self.AddText(cmd[0])
        else:
            self.AddText('<<module>>')
       
class ModelEvtHandler(ogl.ShapeEvtHandler):
    """!Model event handler class"""
    def __init__(self, log, frame):
        ogl.ShapeEvtHandler.__init__(self)
        self.log = log
        self.frame = frame
        
    def OnLeftClick(self, x, y, keys = 0, attachment = 0):
        """!Left mouse button pressed -> update statusbar"""
        shape = self.GetShape()
        
    def OnLeftDoubleClick(self, x, y, keys = 0, attachment = 0):
        """!Left mouse button pressed (double-click) -> show properties"""
        shape = self.GetShape()
        module = menuform.GUI()
        # module.ParseCommand(['r.buffer'],
        # completed = (None , None, None),
        # parentframe = self.frame, show = True)

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
        self.Close()
    
def main():
    app = wx.PySimpleApp()
    frame = ModelFrame(parent = None)
    # frame.CentreOnScreen()
    frame.Show()
    
    app.MainLoop()
    
if __name__ == "__main__":
    main()
