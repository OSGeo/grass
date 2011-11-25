"""!
@package gmodeler.dialogs

@brief wxGUI Graphical Modeler - dialogs

Classes:
 - ModelDataDialog
 - ModelSearchDialog
 - ModelRelationDialog
 - ModelParamDialog
 - ModelItemDialog
 - ModelLoopDialog
 - ModelConditionDialog

(C) 2010-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os

import wx

from core                 import globalvar
from core                 import utils
from gui_core.widgets     import GNotebook
from core.gcmd            import GError, EncodeString
from gui_core.dialogs     import ElementDialog, MapLayersDialog
from gui_core.ghelp       import SearchModuleWindow
from gui_core.prompt      import GPromptSTC

from grass.script import task as gtask

class ModelDataDialog(ElementDialog):
    """!Data item properties dialog"""
    def __init__(self, parent, shape, id = wx.ID_ANY, title = _("Data properties"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent
        self.shape = shape
        
        label, etype = self._getLabel()
        ElementDialog.__init__(self, parent, title, label = label, etype = etype)
                
        self.element = gselect.Select(parent = self.panel,
                                      type = prompt)
        self.element.SetValue(shape.GetValue())
        
        self.Bind(wx.EVT_BUTTON, self.OnOK,     self.btnOK)
        self.Bind(wx.EVT_BUTTON, self.OnCancel, self.btnCancel)
        
        self.PostInit()
        
        if shape.GetValue():
            self.btnOK.Enable()
        
        self._layout()
        self.SetMinSize(self.GetSize())
        
    def _getLabel(self):
        etype = False
        prompt = self.shape.GetPrompt()
        if prompt == 'raster':
            label = _('Name of raster map:')
        elif prompt == 'vector':
            label = _('Name of vector map:')
        else:
            etype = True
            label = _('Name of element:')

        return label, etype
    
    def _layout(self):
        """!Do layout"""
        self.dataSizer.Add(self.element, proportion=0,
                      flag=wx.EXPAND | wx.ALL, border=1)
        
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def OnOK(self, event):
        """!Ok pressed"""
        self.shape.SetValue(self.GetElement())
        if self.etype:
            elem = self.GetType()
            if elem == 'rast':
                self.shape.SetPrompt('raster')
            elif elem == 'vect':
                self.shape.SetPrompt('raster')
        
        self.parent.canvas.Refresh()
        self.parent.SetStatusText('', 0)
        self.shape.SetPropDialog(None)
        
        if self.IsModal():
            event.Skip() 
        else:
            self.Destroy()
    
    def OnCancel(self, event):
        """!Cancel pressed"""
        self.shape.SetPropDialog(None)
        if self.IsModal():
            event.Skip()
        else:
            self.Destroy()
class ModelSearchDialog(wx.Dialog):
    def __init__(self, parent, id = wx.ID_ANY, title = _("Add new GRASS module to the model"),
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
        
        self.cmdBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                   label=" %s " % _("Command"))
        
        self.cmd_prompt = GPromptSTC(parent = self)
        self.search = SearchModuleWindow(parent = self.panel, cmdPrompt = self.cmd_prompt, showTip = True)
        wx.CallAfter(self.cmd_prompt.SetFocus)
        
        # get commands
        items = self.cmd_prompt.GetCommandItems()
        
        self.btnCancel = wx.Button(self.panel, wx.ID_CANCEL)
        self.btnOk     = wx.Button(self.panel, wx.ID_OK)
        self.btnOk.SetDefault()
        self.btnOk.Enable(False)

        self.cmd_prompt.Bind(wx.EVT_KEY_UP, self.OnText)
        self.search.searchChoice.Bind(wx.EVT_CHOICE, self.OnText)
        self.Bind(wx.EVT_BUTTON, self.OnOk, self.btnOk)
        
        self._layout()
        
        self.SetSize((500, 275))
        
    def _layout(self):
        cmdSizer = wx.StaticBoxSizer(self.cmdBox, wx.VERTICAL)
        cmdSizer.Add(item = self.cmd_prompt, proportion = 1,
                     flag = wx.EXPAND)
        
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = self.search, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 3)
        mainSizer.Add(item = cmdSizer, proportion = 1,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, border = 3)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.panel.SetSizer(mainSizer)
        mainSizer.Fit(self.panel)
        
        self.Layout()

    def GetPanel(self):
        """!Get dialog panel"""
        return self.panel

    def GetCmd(self):
        """!Get command"""
        line = self.cmd_prompt.GetCurLine()[0].strip()
        if len(line) == 0:
            list()
        
        try:
            cmd = utils.split(str(line))
        except UnicodeError:
            cmd = utils.split(EncodeString((line)))
            
        return cmd
    
    def OnOk(self, event):
        """!Button 'OK' pressed"""
        self.btnOk.SetFocus()
        cmd = self.GetCmd()
        
        if len(cmd) < 1:
            GError(parent = self,
                   message = _("Command not defined.\n\n"
                               "Unable to add new action to the model."))
            return
        
        if cmd[0] not in globalvar.grassCmd['all']:
            GError(parent = self,
                   message = _("'%s' is not a GRASS module.\n\n"
                               "Unable to add new action to the model.") % cmd[0])
            return
        
        self.EndModal(wx.ID_OK)
        
    def OnText(self, event):
        """!Text in prompt changed"""
        if self.cmd_prompt.AutoCompActive():
            event.Skip()
            return
        
        if isinstance(event, wx.KeyEvent):
            entry = self.cmd_prompt.GetTextLeft()
        elif isinstance(event, wx.stc.StyledTextEvent):
            entry = event.GetText()
        else:
            entry = event.GetString()
        
        if entry:
            self.btnOk.Enable()
        else:
            self.btnOk.Enable(False)
            
        event.Skip()
        
    def Reset(self):
        """!Reset dialog"""
        self.search.Reset()
        self.cmd_prompt.OnCmdErase(None)
        self.btnOk.Enable(False)
        self.cmd_prompt.SetFocus()

class ModelRelationDialog(wx.Dialog):
    """!Relation properties dialog"""
    def __init__(self, parent, shape, id = wx.ID_ANY, title = _("Relation properties"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        self.parent = parent
        self.shape = shape
        
        options = self._getOptions()
        if not options:
            self.valid = False
            return
        
        self.valid = True
        wx.Dialog.__init__(self, parent, id, title, style = style, **kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.fromBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                    label = " %s " % _("From"))
        self.toBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                  label = " %s " % _("To"))
        
        self.option = wx.ComboBox(parent = self.panel, id = wx.ID_ANY,
                                  style = wx.CB_READONLY,
                                  choices = options)
        self.option.Bind(wx.EVT_COMBOBOX, self.OnOption)
        
        self.btnCancel = wx.Button(self.panel, wx.ID_CANCEL)
        self.btnOk     = wx.Button(self.panel, wx.ID_OK)
        self.btnOk.Enable(False)
        
        self._layout()

    def _layout(self):
        mainSizer = wx.BoxSizer(wx.VERTICAL)

        fromSizer = wx.StaticBoxSizer(self.fromBox, wx.VERTICAL)
        self._layoutShape(shape = self.shape.GetFrom(), sizer = fromSizer)
        toSizer = wx.StaticBoxSizer(self.toBox, wx.VERTICAL)
        self._layoutShape(shape = self.shape.GetTo(), sizer = toSizer)

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()
        
        mainSizer.Add(item = fromSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = toSizer, proportion = 0,
                      flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.panel.SetSizer(mainSizer)
        mainSizer.Fit(self.panel)
        
        self.Layout()
        self.SetSize(self.GetBestSize())
        
    def _layoutShape(self, shape, sizer):
        if isinstance(shape, ModelData):
            sizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                           label = _("Data: %s") % shape.GetLog()),
                      proportion = 1, flag = wx.EXPAND | wx.ALL,
                      border = 5)
        elif isinstance(shape, ModelAction):
            gridSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
            gridSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                               label = _("Command:")),
                          pos = (0, 0))
            gridSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                               label = shape.GetName()),
                          pos = (0, 1))
            gridSizer.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                               label = _("Option:")),
                          flag = wx.ALIGN_CENTER_VERTICAL,
                          pos = (1, 0))
            gridSizer.Add(item = self.option,
                          pos = (1, 1))
            sizer.Add(item = gridSizer,
                      proportion = 1, flag = wx.EXPAND | wx.ALL,
                      border = 5)
            
    def _getOptions(self):
        """!Get relevant options"""
        items = []
        fromShape = self.shape.GetFrom()
        if not isinstance(fromShape, ModelData):
            GError(parent = self.parent,
                   message = _("Relation doesn't start with data item.\n"
                               "Unable to add relation."))
            return items
        
        toShape = self.shape.GetTo()
        if not isinstance(toShape, ModelAction):
            GError(parent = self.parent,
                   message = _("Relation doesn't point to GRASS command.\n"
                               "Unable to add relation."))
            return items
        
        prompt = fromShape.GetPrompt()
        task = toShape.GetTask()
        for p in task.get_options()['params']:
            if p.get('prompt', '') == prompt and \
                    'name' in p:
                items.append(p['name'])
        
        if not items:
            GError(parent = self.parent,
                   message = _("No relevant option found.\n"
                               "Unable to add relation."))
        return items
    
    def GetOption(self):
        """!Get selected option"""
        return self.option.GetStringSelection()
    
    def IsValid(self):
        """!Check if relation is valid"""
        return self.valid
    
    def OnOption(self, event):
        """!Set option"""
        if event.GetString():
            self.btnOk.Enable()
        else:
            self.btnOk.Enable(False)

class ModelParamDialog(wx.Dialog):
    def __init__(self, parent, params, id = wx.ID_ANY, title = _("Model parameters"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        """!Model parameters dialog
        """
        self.parent = parent
        self.params = params
        self.tasks  = list() # list of tasks/pages
        
        wx.Dialog.__init__(self, parent = parent, id = id, title = title, style = style, **kwargs)
        
        self.notebook = GNotebook(parent = self, 
                                  style = globalvar.FNPageDStyle)
        
        panel = self._createPages()
        wx.CallAfter(self.notebook.SetSelection, 0)
        
        self.btnCancel = wx.Button(parent = self, id = wx.ID_CANCEL)
        self.btnRun    = wx.Button(parent = self, id = wx.ID_OK,
                                   label = _("&Run"))
        self.btnRun.SetDefault()
        
        self._layout()
        
        size = self.GetBestSize()
        self.SetMinSize(size)
        self.SetSize((size.width, size.height +
                      panel.constrained_size[1] -
                      panel.panelMinHeight))
                
    def _layout(self):
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnRun)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = self.notebook, proportion = 1,
                      flag = wx.EXPAND)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        
    def _createPages(self):
        """!Create for each parameterized module its own page"""
        nameOrdered = [''] * len(self.params.keys())
        for name, params in self.params.iteritems():
            nameOrdered[params['idx']] = name
        for name in nameOrdered:
            params = self.params[name]
            panel = self._createPage(name, params)
            if name == 'variables':
                name = _('Variables')
            self.notebook.AddPage(page = panel, text = name)
        
        return panel
    
    def _createPage(self, name, params):
        """!Define notebook page"""
        if name in globalvar.grassCmd['all']:
            task = gtask.grassTask(name)
        else:
            task = gtask.grassTask()
        task.flags  = params['flags']
        task.params = params['params']
        
        panel = menuform.cmdPanel(parent = self, id = wx.ID_ANY, task = task)
        self.tasks.append(task)
        
        return panel

    def GetErrors(self):
        """!Check for errors, get list of messages"""
        errList = list()
        for task in self.tasks:
            errList += task.get_cmd_error()
        
        return errList

class ModelItemDialog(wx.Dialog):
    """!Abstract item properties dialog"""
    def __init__(self, parent, shape, title, id = wx.ID_ANY,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        self.parent = parent
        self.shape = shape
        
        wx.Dialog.__init__(self, parent, id, title = title, style = style, **kwargs)
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.condBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                    label=" %s " % _("Condition"))
        self.condText = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY,
                                    value = shape.GetText())
        
        self.itemList = ItemCheckListCtrl(parent = self.panel,
                                          window = self,
                                          columns = [_("ID"), _("Name"),
                                                     _("Command")],
                                          shape = shape)
        self.itemList.Populate(self.parent.GetModel().GetItems())
        
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btnOk     = wx.Button(parent = self.panel, id = wx.ID_OK)
        self.btnOk.SetDefault()
        
    def _layout(self):
        """!Do layout (virtual method)"""
        pass
    
    def GetCondition(self):
        """!Get loop condition"""
        return self.condText.GetValue()

class ModelLoopDialog(ModelItemDialog):
    """!Loop properties dialog"""
    def __init__(self, parent, shape, id = wx.ID_ANY, title = _("Loop properties"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        ModelItemDialog.__init__(self, parent, shape, title,
                                 style = style, **kwargs)
        
        self.listBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                    label=" %s " % _("List of items in loop"))
        
        self.btnSeries = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                   label = _("Series"))
        self.btnSeries.SetToolTipString(_("Define map series as condition for the loop"))
        self.btnSeries.Bind(wx.EVT_BUTTON, self.OnSeries)
        
        self._layout()
        self.SetMinSize(self.GetSize())
        self.SetSize((500, 400))
        
    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        condSizer = wx.StaticBoxSizer(self.condBox, wx.HORIZONTAL)
        condSizer.Add(item = self.condText, proportion = 1,
                      flag = wx.ALL, border = 3)
        condSizer.Add(item = self.btnSeries, proportion = 0,
                      flag = wx.EXPAND)

        listSizer = wx.StaticBoxSizer(self.listBox, wx.VERTICAL)
        listSizer.Add(item = self.itemList, proportion = 1,
                      flag = wx.EXPAND | wx.ALL, border = 3)
        
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()

        sizer.Add(item = condSizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALL, border = 3)
        sizer.Add(item = listSizer, proportion = 1,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 3)
        sizer.Add(item = btnSizer, proportion=0,
                  flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        
        self.Layout()
        
    def GetItems(self):
        """!Get list of selected actions"""
        return self.itemList.GetItems()

    def OnSeries(self, event):
        """!Define map series as condition"""
        dialog = MapLayersDialog(parent = self, title = _("Define series of maps"), modeler = True)
        if dialog.ShowModal() != wx.ID_OK:
            dialog.Destroy()
            return
        
        cond = dialog.GetDSeries()
        if not cond:
            cond = 'map in %s' % map(lambda x: str(x), dialog.GetMapLayers())
        
        self.condText.SetValue(cond)
                               
        dialog.Destroy()

class ModelConditionDialog(ModelItemDialog):
    """!Condition properties dialog"""
    def __init__(self, parent, shape, id = wx.ID_ANY, title = _("If-else properties"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs):
        ModelItemDialog.__init__(self, parent, shape, title,
                                 style = style, **kwargs)
        
        self.listBoxIf = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                      label=" %s " % _("List of items in 'if' block"))
        self.itemListIf = self.itemList
        self.itemListIf.SetName('IfBlockList')
        
        self.listBoxElse = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                        label=" %s " % _("List of items in 'else' block"))
        self.itemListElse = ItemCheckListCtrl(parent = self.panel,
                                              window = self,
                                              columns = [_("ID"), _("Name"),
                                                         _("Command")],
                                              shape = shape)
        self.itemListElse.SetName('ElseBlockList')
        self.itemListElse.Populate(self.parent.GetModel().GetItems())
        
        self._layout()
        self.SetMinSize(self.GetSize())
        self.SetSize((500, 400))
        
    def _layout(self):
        """!Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        condSizer = wx.StaticBoxSizer(self.condBox, wx.VERTICAL)
        condSizer.Add(item = self.condText, proportion = 1,
                      flag = wx.EXPAND)
        
        listIfSizer = wx.StaticBoxSizer(self.listBoxIf, wx.VERTICAL)
        listIfSizer.Add(item = self.itemListIf, proportion = 1,
                        flag = wx.EXPAND)
        listElseSizer = wx.StaticBoxSizer(self.listBoxElse, wx.VERTICAL)
        listElseSizer.Add(item = self.itemListElse, proportion = 1,
                          flag = wx.EXPAND)
        
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()

        sizer.Add(item = condSizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALL, border = 3)
        sizer.Add(item = listIfSizer, proportion = 1,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 3)
        sizer.Add(item = listElseSizer, proportion = 1,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 3)
        sizer.Add(item = btnSizer, proportion=0,
                  flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)
        
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        
        self.Layout()

    def OnCheckItemIf(self, index, flag):
        """!Item in if-block checked/unchecked"""
        if flag is False:
            return
        
        aId = int(self.itemListIf.GetItem(index, 0).GetText())
        if aId in self.itemListElse.GetItems()['checked']:
            self.itemListElse.CheckItemById(aId, False)
            
    def OnCheckItemElse(self, index, flag):
        """!Item in else-block checked/unchecked"""
        if flag is False:
            return
        
        aId = int(self.itemListElse.GetItem(index, 0).GetText())
        if aId in self.itemListIf.GetItems()['checked']:
            self.itemListIf.CheckItemById(aId, False)
        
    def GetItems(self):
        """!Get items"""
        return { 'if'   : self.itemListIf.GetItems(),
                 'else' : self.itemListElse.GetItems() }
