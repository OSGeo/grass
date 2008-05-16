"""
MODULE:     rules.py

CLASSES:
    * RulesText

PURPOSE:    Dialog for interactive entry of rules for r.colors,
            r.reclass, r.recode, and v.reclass

AUTHORS:    The GRASS Development Team
            Michael Barton (Arizona State University)

COPYRIGHT:  (C) 2007 by the GRASS Development Team
            This program is free software under the GNU General Public
            License (>=v2). Read the file COPYING that comes with GRASS
            for details.

"""

import os
import sys

import wx

import gcmd
import gselect
import globalvar

class RulesText(wx.Dialog):
    def __init__(self, parent, id=wx.ID_ANY, title=_("Enter rules"),
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE,
                 cmd=None):
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)

        """
        Dialog for interactively entering rules
        for map management commands

        @param cmd command (given as list)
        """
        self.parent = parent
        self.cmd = cmd # map management command
        self.inmap = '' # input map to change
        self.outmap = '' # output map for reclass/recode
        self.rules = '' # rules for changing
        self.overwrite = False

        if self.cmd[0] == 'r.colors':
            label1 = _('Create new color table using color rules')
            label2 = _('Raster map:')
            label3 = None
            label4 = _('Enter color rules')
            seltype = 'cell'
        elif self.cmd[0] == 'r.reclass':
            label1 = _('Reclassify raster map using rules')
            label2 = _('Raster map to reclassify:')
            label3 = _('Reclassified raster map:')
            label4 = _('Enter reclassification rules')
            seltype = 'cell'
        elif self.cmd[0] == 'r.recode':
            label1 = _('Recode raster map using rules')
            label2 = _('Raster map to recode:')
            label3 = _('Recoded raster map:')
            label4 = _('Enter recoding rules')
            seltype = 'cell'
        elif self.cmd[0] == 'v.reclass':
            label1 = _('Reclassify vector map using SQL rules')
            label2 = _('Vector map to reclassify:')
            label3 = _('Reclassified vector map:')
            label4 = _('Enter reclassification rules')
            seltype = 'vector'

        # set window frame title
        self.SetTitle(label1)

        sizer = wx.BoxSizer(wx.VERTICAL)
        boxSizer =  wx.GridBagSizer(hgap=5, vgap=5)
        boxSizer.AddGrowableCol(0)

        row = 0
        boxSizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=label2),
                     flag=wx.ALIGN_CENTER_VERTICAL,
                     pos=(row,0))

        self.selectionInput = gselect.Select(parent=self, id=wx.ID_ANY, size=globalvar.DIALOG_GSELECT_SIZE,
                                             type=seltype)
        boxSizer.Add(item=self.selectionInput,
                     pos=(row,1))
        row += 1

        if self.cmd[0] != 'r.colors':
            boxSizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=label3),
                         flag=wx.ALIGN_CENTER_VERTICAL,
                         pos=(row, 0))
            self.selectionOutput = gselect.Select(parent=self, id=wx.ID_ANY, size=(300,-1),
                                                  type=seltype)
            self.selectionOutput.Bind(wx.EVT_TEXT, self.OnSelectionOutput)
            boxSizer.Add(item=self.selectionOutput,
                         pos=(row,1))
            row += 1
            # TODO remove (see Preferences dialog)
            self.ovrwrtcheck = wx.CheckBox(parent=self, id=wx.ID_ANY, label=_('overwrite existing file'))
            self.ovrwrtcheck.SetValue(self.overwrite)
            boxSizer.Add(item=self.ovrwrtcheck,
                         pos=(row, 1))
            self.Bind(wx.EVT_CHECKBOX, self.OnOverwrite,   self.ovrwrtcheck)
            row += 1

        boxSizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=label4),
                     flag=wx.ALIGN_CENTER_VERTICAL,
                     pos=(row, 0))
        helpbtn = wx.Button(parent=self, id=wx.ID_HELP)
        boxSizer.Add(item=helpbtn, flag=wx.ALIGN_RIGHT, pos=(row, 1))
        row += 1 

        self.rulestxt = wx.TextCtrl(parent=self, id=wx.ID_ANY, value='',
                                    pos=wx.DefaultPosition, size=(400,150),
                                    style=wx.TE_MULTILINE |
                                    wx.HSCROLL |
                                    wx.TE_NOHIDESEL)
        self.rulestxt.SetFont(wx.Font(10, wx.FONTFAMILY_MODERN, wx.NORMAL, wx.NORMAL, 0, ''))
        boxSizer.Add(item=self.rulestxt, pos=(row, 0), flag=wx.EXPAND, span=(1, 2))

        sizer.Add(item=boxSizer, proportion=1,
                  flag=wx.ALL ,border=10)

        line = wx.StaticLine(parent=self, id=wx.ID_ANY, size=(20,-1), style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.GROW | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(self, wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(self, wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(item=btnsizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)

        # bindings
        self.Bind(wx.EVT_BUTTON, self.OnHelp, helpbtn)
        self.selectionInput.Bind(wx.EVT_TEXT, self.OnSelectionInput)
        self.Bind(wx.EVT_TEXT, self.OnRules,   self.rulestxt)

    def OnSelectionInput(self, event):
        self.inmap = event.GetString()

    def OnSelectionOutput(self, event):
        self.outmap = event.GetString()

    def OnRules(self, event):
        self.rules = event.GetString().strip()
        if self.cmd[0] == 'r.recode':
            self.rules = self.rules + '%s' % os.linesep

    def OnHelp(self, event):
        gcmd.Command(['g.manual',
                      '--quiet', 
                      '%s' % self.cmd[0]])

    def OnOverwrite(self, event):
        self.overwrite = event.IsChecked()
