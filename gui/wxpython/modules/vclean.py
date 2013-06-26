"""
@package modules.vclean

@brief Dialog for interactive construction of vector cleaning
operations

Classes:
 - vclean::VectorCleaningFrame

(C) 2010-2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Markus Metz
"""

import os
import sys

import wx
import wx.lib.scrolledpanel as scrolled

if __name__ == '__main__':
    gui_wx_path = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if gui_wx_path not in sys.path:
        sys.path.append(gui_wx_path)

from core.gcmd        import RunCommand, GError
from core             import globalvar
from gui_core.gselect import Select
from core.settings    import UserSettings
from grass.script import core as grass


class VectorCleaningFrame(wx.Frame):
    def __init__(self, parent, id=wx.ID_ANY, title=_('Set up vector cleaning tools'),
                 style=wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER,
                 **kwargs):
        """!
        Dialog for interactively defining vector cleaning tools
        """
        wx.Frame.__init__(self, parent, id, title, style=style, **kwargs)

        self.parent = parent  # GMFrame
        if self.parent:
            self.log = self.parent.GetLogWindow()
        else:
            self.log = None

        # grass command
        self.cmd = 'v.clean'

        # statusbar
        self.CreateStatusBar()

        # icon
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        # input map to clean
        self.inmap = ''

        # cleaned output map
        self.outmap = ''

        self.ftype = ''

        # cleaning tools
        self.toolslines = {}

        self.tool_desc_list = [
            _('break lines/boundaries'),
            _('remove duplicates'),
            _('remove dangles'),
            _('change boundary dangles to lines'),
            _('remove bridges'),
            _('change bridges to lines'),
            _('snap lines/boundaries'),
            _('remove duplicate area centroids'),
            _('break polygons'),
            _('prune lines/boundaries'),
            _('remove small areas'),
            _('remove lines/boundaries of zero length'),
            _('remove small angles at nodes')
            ]

        self.tool_list = [
            'break',
            'rmdupl',
            'rmdangle',
            'chdangle',
            'rmbridge',
            'chbridge',
            'snap',
            'rmdac',
            'bpol',
            'prune',
            'rmarea',
            'rmline',
            'rmsa'
            ]

        self.ftype = [
            'point',
            'line',
            'boundary',
            'centroid',
            'area',
            'face']

        self.n_ftypes = len(self.ftype)

        self.tools_string = ''
        self.thresh_string = ''
        self.ftype_string = ''

        self.SetStatusText(_("Set up vector cleaning tools"))
        self.elem = 'vector'
        self.ctlabel = _('Choose cleaning tools and set thresholds')

        # top controls
        self.inmaplabel = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                         label=_('Select input vector map:'))
        self.selectionInput = Select(parent=self.panel, id=wx.ID_ANY,
                                     size=globalvar.DIALOG_GSELECT_SIZE,
                                     type='vector')
        self.ftype_check = {}
        ftypeBox = wx.StaticBox(parent=self.panel, id=wx.ID_ANY,
                                label=_(' Feature type: '))
        self.ftypeSizer = wx.StaticBoxSizer(ftypeBox, wx.HORIZONTAL)

        self.outmaplabel = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                         label=_('Select output vector map:'))
        self.selectionOutput = Select(parent=self.panel, id=wx.ID_ANY,
                                      size=globalvar.DIALOG_GSELECT_SIZE,
                                      mapsets=[grass.gisenv()['MAPSET'],],
                                      fullyQualified = False,
                                      type='vector')

        self.overwrite = wx.CheckBox(parent=self.panel, id=wx.ID_ANY,
                                       label=_('Allow output files to overwrite existing files'))
        self.overwrite.SetValue(UserSettings.Get(group='cmd', key='overwrite', subkey='enabled'))

        # cleaning tools
        self.ct_label = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                      label=self.ctlabel)

        self.ct_panel = self._toolsPanel()

        # buttons to manage cleaning tools
        self.btn_add = wx.Button(parent=self.panel, id=wx.ID_ADD)
        self.btn_remove = wx.Button(parent=self.panel, id=wx.ID_REMOVE)
        self.btn_moveup = wx.Button(parent=self.panel, id=wx.ID_UP)
        self.btn_movedown = wx.Button(parent=self.panel, id=wx.ID_DOWN)

        # add one tool as default
        self.AddTool()
        self.selected = -1

        # Buttons
        self.btn_close = wx.Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btn_run = wx.Button(parent=self.panel, id=wx.ID_ANY, label=_("&Run"))
        self.btn_run.SetDefault()
        self.btn_clipboard = wx.Button(parent=self.panel, id=wx.ID_COPY)
        self.btn_clipboard.SetToolTipString(_("Copy the current command string to the clipboard (Ctrl+C)"))
        self.btn_help = wx.Button(parent=self.panel, id=wx.ID_HELP)

        # bindings
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnCleaningRun)
        self.btn_clipboard.Bind(wx.EVT_BUTTON, self.OnCopy)
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)

        self.btn_add.Bind(wx.EVT_BUTTON, self.OnAddTool)
        self.btn_remove.Bind(wx.EVT_BUTTON, self.OnClearTool)
        self.btn_moveup.Bind(wx.EVT_BUTTON, self.OnMoveToolUp)
        self.btn_movedown.Bind(wx.EVT_BUTTON, self.OnMoveToolDown)

        # layout
        self._layout()

        self.SetMinSize(self.GetBestSize())

        self.CentreOnScreen()

    def _layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)

        #
        # input output
        #
        inSizer = wx.GridBagSizer(hgap=5, vgap=5)

        inSizer.Add(item=self.inmaplabel, pos=(0, 0),
                       flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border=1)
        inSizer.Add(item=self.selectionInput, pos=(1, 0),
                       flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border=1)

        self.ftype_check = [
            wx.CheckBox(parent=self.panel, id=wx.ID_ANY, label=_('point')),
            wx.CheckBox(parent=self.panel, id=wx.ID_ANY, label=_('line')),
            wx.CheckBox(parent=self.panel, id=wx.ID_ANY, label=_('boundary')),
            wx.CheckBox(parent=self.panel, id=wx.ID_ANY, label=_('centroid')),
            wx.CheckBox(parent=self.panel, id=wx.ID_ANY, label=_('area')),
            wx.CheckBox(parent=self.panel, id=wx.ID_ANY, label=_('face'))
            ]

        typeoptSizer = wx.BoxSizer(wx.HORIZONTAL)
        for num in range(0, self.n_ftypes):
            type_box = self.ftype_check[num]
            if self.ftype[num] in ('point', 'line', 'area'):
                type_box.SetValue(True);
            typeoptSizer.Add(item=type_box, flag=wx.ALIGN_LEFT, border=1)

        self.ftypeSizer.Add(item=typeoptSizer,
                            flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=2)

        outSizer = wx.GridBagSizer(hgap=5, vgap=5)

        outSizer.Add(item=self.outmaplabel, pos=(0, 0),
                       flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border=1)
        outSizer.Add(item=self.selectionOutput, pos=(1, 0),
                       flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND, border=1)
        replaceSizer = wx.BoxSizer(wx.HORIZONTAL)
        replaceSizer.Add(item=self.overwrite, proportion=1,
                         flag=wx.ALL | wx.EXPAND, border=1)

        outSizer.Add(item=replaceSizer, pos=(2, 0),
                     flag=wx.ALL | wx.EXPAND, border=1)

        #
        # tools selection
        #
        bodySizer = wx.GridBagSizer(hgap=5, vgap=5)

        bodySizer.Add(item=self.ct_label, pos=(0, 0), span=(1, 2),
                      flag=wx.ALL, border=5)

        bodySizer.Add(item=self.ct_panel, pos=(1, 0), span=(1, 2))

        manageBoxSizer = wx.GridBagSizer(hgap=10, vgap=1)
        # start with row 1 for nicer layout
        manageBoxSizer.Add(item=self.btn_add, pos=(1, 0), border=2, flag=wx.ALL | wx.EXPAND)
        manageBoxSizer.Add(item=self.btn_remove, pos=(2, 0), border=2, flag=wx.ALL | wx.EXPAND)
        manageBoxSizer.Add(item=self.btn_moveup, pos=(3, 0), border=2, flag=wx.ALL | wx.EXPAND)
        manageBoxSizer.Add(item=self.btn_movedown, pos=(4, 0), border=2, flag=wx.ALL | wx.EXPAND)

        bodySizer.Add(item=manageBoxSizer, pos=(1, 2),
                      flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5)

        bodySizer.AddGrowableCol(2)

        #
        # standard buttons
        #
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btn_close,
                     flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btn_run,
                     flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btn_clipboard,
                     flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btn_help,
                     flag=wx.LEFT | wx.RIGHT, border=5)

        #
        # put it all together
        #
        sizer.Add(item=inSizer, proportion=0,
                  flag=wx.ALL | wx.EXPAND, border=5)

        sizer.Add(item=self.ftypeSizer, proportion=0,
                  flag=wx.ALL | wx.EXPAND, border=5)

        sizer.Add(item=outSizer, proportion=0,
                  flag=wx.ALL | wx.EXPAND, border=5)

        sizer.Add(item=wx.StaticLine(parent=self, id=wx.ID_ANY,
                  style=wx.LI_HORIZONTAL), proportion=0,
                  flag=wx.EXPAND | wx.ALL, border=5)

        sizer.Add(item=bodySizer, proportion=1,
                  flag=wx.ALL | wx.EXPAND, border=5)

        sizer.Add(item=wx.StaticLine(parent=self, id=wx.ID_ANY,
                  style=wx.LI_HORIZONTAL), proportion=0,
                  flag=wx.EXPAND | wx.ALL, border=5)

        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.ALL | wx.ALIGN_RIGHT, border=5)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

        self.Layout()

    def _toolsPanel(self):
        ct_panel = scrolled.ScrolledPanel(parent=self.panel, id=wx.ID_ANY,
                                          size=(500, 240),
                                          style=wx.SUNKEN_BORDER)

        self.ct_sizer = wx.GridBagSizer(vgap=2, hgap=4)

        ct_panel.SetSizer(self.ct_sizer)
        ct_panel.SetAutoLayout(True)

        return ct_panel

    def OnAddTool(self, event):
        """!Add tool button pressed"""
        self.AddTool()

    def AddTool(self):
        snum = len(self.toolslines.keys())
        num = snum + 1
        # tool
        tool_cbox = wx.ComboBox(parent=self.ct_panel, id=1000 + num,
                                size=(300, -1), choices=self.tool_desc_list,
                                style=wx.CB_DROPDOWN |
                                wx.CB_READONLY | wx.TE_PROCESS_ENTER)
        self.Bind(wx.EVT_COMBOBOX, self.OnSetTool, tool_cbox)
        # threshold
        txt_ctrl = wx.TextCtrl(parent=self.ct_panel, id=2000 + num, value='0.00',
                               size=(100, -1),
                               style=wx.TE_NOHIDESEL)
        self.Bind(wx.EVT_TEXT, self.OnThreshValue, txt_ctrl)

        # select with tool number
        select = wx.CheckBox(parent=self.ct_panel, id=num, label=str(num) + '.')
        select.SetValue(False)
        self.Bind(wx.EVT_CHECKBOX, self.OnSelect, select)

        # start with row 1 and col 1 for nicer layout
        self.ct_sizer.Add(item=select, pos=(num, 1),
                          flag=wx.ALIGN_CENTER | wx.RIGHT)
        self.ct_sizer.Add(item=tool_cbox, pos=(num, 2),
                          flag=wx.ALIGN_CENTER | wx.RIGHT, border=5)
        self.ct_sizer.Add(item=txt_ctrl, pos=(num, 3),
                          flag=wx.ALIGN_CENTER | wx.RIGHT, border=5)

        self.toolslines[num] = {'tool_desc': '',
                                'tool': '',
                                'thresh': '0.00'}

        self.ct_panel.Layout()
        self.ct_panel.SetupScrolling()

    def OnClearTool(self, event):
        """!Remove tool button pressed"""
        id = self.selected

        if id > 0:
            self.FindWindowById(id + 1000).SetValue('')
            self.toolslines[id]['tool_desc'] = ''
            self.toolslines[id]['tool'] = ''
            self.SetStatusText(_("%s. cleaning tool removed, will be ignored") % id)
        else:
            self.SetStatusText(_("Please select a cleaning tool to remove"))

    def OnMoveToolUp(self, event):
        """!Move up tool button pressed"""
        id = self.selected

        if id > 1:
            id_up = id - 1
            this_toolline = self.toolslines[id]
            up_toolline = self.toolslines[id_up]

            self.FindWindowById(id_up).SetValue(True)
            self.FindWindowById(id_up + 1000).SetValue(this_toolline['tool_desc'])
            self.FindWindowById(id_up + 2000).SetValue(this_toolline['thresh'])
            self.toolslines[id_up] = this_toolline

            self.FindWindowById(id).SetValue(False)
            self.FindWindowById(id + 1000).SetValue(up_toolline['tool_desc'])
            self.FindWindowById(id + 2000).SetValue(up_toolline['thresh'])
            self.toolslines[id] = up_toolline
            self.selected = id_up
            self.SetStatusText(_("%s. cleaning tool moved up") % id)
        elif id == 1:
            self.SetStatusText(_("1. cleaning tool can not be moved up "))
        elif id == -1:
            self.SetStatusText(_("Please select a cleaning tool to move up"))

    def OnMoveToolDown(self, event):
        """!Move down tool button pressed"""
        id = self.selected
        snum = len(self.toolslines.keys())

        if id > 0 and id < snum:
            id_down = id + 1
            this_toolline = self.toolslines[id]
            down_toolline = self.toolslines[id_down]

            self.FindWindowById(id_down).SetValue(True)
            self.FindWindowById(id_down + 1000).SetValue(this_toolline['tool_desc'])
            self.FindWindowById(id_down + 2000).SetValue(this_toolline['thresh'])
            self.toolslines[id_down] = this_toolline

            self.FindWindowById(id).SetValue(False)
            self.FindWindowById(id + 1000).SetValue(down_toolline['tool_desc'])
            self.FindWindowById(id + 2000).SetValue(down_toolline['thresh'])
            self.toolslines[id] = down_toolline
            self.selected = id_down
            self.SetStatusText(_("%s. cleaning tool moved down") % id)
        elif id == snum:
            self.SetStatusText(_("Last cleaning tool can not be moved down "))
        elif id == -1:
            self.SetStatusText(_("Please select a cleaning tool to move down"))

    def OnSetTool(self, event):
        """!Tool was defined"""
        id = event.GetId()
        tool_no = id - 1000
        num = self.FindWindowById(id).GetCurrentSelection()

        self.toolslines[tool_no]['tool_desc'] = self.tool_desc_list[num]
        self.toolslines[tool_no]['tool'] = self.tool_list[num]

        self.SetStatusText(str(tool_no) + '. ' + _("cleaning tool: '%s'") % (self.tool_list[num]))

    def OnThreshValue(self, event):
        """!Threshold value was entered"""
        id = event.GetId()
        num = id - 2000
        self.toolslines[num]['thresh'] = self.FindWindowById(id).GetValue()

        self.SetStatusText(_("Threshold for %(num)s. tool '%(tool)s': %(thresh)s") % \
                           {'num': num,
                            'tool': self.toolslines[num]['tool'],
                            'thresh': self.toolslines[num]['thresh']})

    def OnSelect(self, event):
        """!Tool was selected"""
        id = event.GetId()

        if self.selected > -1 and self.selected != id:
            win = self.FindWindowById(self.selected)
            win.SetValue(False)

        if self.selected != id:
            self.selected = id
        else:
            self.selected = -1

    def OnDone(self, cmd, returncode):
        """!Command done"""
        self.SetStatusText('')

    def OnCleaningRun(self, event):
        """!Builds options and runs v.clean
        """
        self.GetCmdStrings()

        err = list()
        for p, name in ((self.inmap, _('Name of input vector map')),
                        (self.outmap, _('Name for output vector map')),
                        (self.tools_string, _('Tools')),
                        (self.thresh_string, _('Threshold'))):
            if not p:
                err.append(_("'%s' not defined") % name)
        if err:
            GError(_("Some parameters not defined. Operation "
                     "canceled.\n\n%s") % '\n'.join(err),
                   parent=self)
            return

        self.SetStatusText(_("Executing selected cleaning operations..."))
        snum = len(self.toolslines.keys())

        if self.log:
            cmd = [self.cmd,
                  'input=%s' % self.inmap,
                  'output=%s' % self.outmap,
                  'tool=%s' % self.tools_string,
                  'thres=%s' % self.thresh_string]
            if self.ftype_string:
                cmd.append('type=%s' % self.ftype_string)
            if self.overwrite.IsChecked():
                cmd.append('--overwrite')

            self.log.RunCmd(cmd, onDone=self.OnDone)
            self.parent.Raise()
        else:
            if self.overwrite.IsChecked():
                overwrite = True
            else:
                overwrite = False

            RunCommand(self.cmd,
                       input=self.inmap,
                       output=self.outmap,
                       type=self.ftype_string,
                       tool=self.tools_string,
                       thresh=self.thresh_string,
                       overwrite=overwrite)

    def OnClose(self, event):
        self.Destroy()

    def OnHelp(self, event):
        """!Show GRASS manual page"""
        RunCommand('g.manual',
                   quiet=True,
                   parent=self,
                   entry=self.cmd)

    def OnCopy(self, event):
        """!Copy the command"""
        cmddata = wx.TextDataObject()
        # get tool and thresh strings
        self.GetCmdStrings()
        cmdstring = '%s' % (self.cmd)
        # list -> string
        cmdstring += ' input=%s output=%s type=%s tool=%s thres=%s' % \
                     (self.inmap, self.outmap, self.ftype_string, self.tools_string, self.thresh_string)
        if self.overwrite.IsChecked():
            cmdstring += ' --overwrite'

        cmddata.SetText(cmdstring)
        if wx.TheClipboard.Open():
            wx.TheClipboard.SetData(cmddata)
            wx.TheClipboard.Close()
            self.SetStatusText(_("Vector cleaning command copied to clipboard"))

    def GetCmdStrings(self):
        self.tools_string = ''
        self.thresh_string = ''
        self.ftype_string = ''
        # feature types
        first = 1
        for num in range(0, self.n_ftypes - 1):
            if self.ftype_check[num].IsChecked():
                if first:
                    self.ftype_string = '%s' % self.ftype[num]
                    first = 0
                else:
                    self.ftype_string += ',%s' % self.ftype[num]

        # cleaning tools
        first = 1
        snum = len(self.toolslines.keys())
        for num in range(1, snum + 1):
            if self.toolslines[num]['tool']:
                if first:
                    self.tools_string = '%s' % self.toolslines[num]['tool']
                    self.thresh_string = '%s' % self.toolslines[num]['thresh']
                    first = 0
                else:
                    self.tools_string += ',%s' % self.toolslines[num]['tool']
                    self.thresh_string += ',%s' % self.toolslines[num]['thresh']

        self.inmap = self.selectionInput.GetValue()
        self.outmap = self.selectionOutput.GetValue()

if __name__ == '__main__':
    app = wx.App()
    frame = VectorCleaningFrame(parent=None)
    frame.Show()
    app.MainLoop()
