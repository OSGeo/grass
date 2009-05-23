"""
@package prompt.py

@brief GRASS prompt

Classes:
 - GPrompt

(C) 2008-2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import shlex

import wx

class GPrompt:
    """Interactive GRASS prompt"""
    def __init__(self, parent):
        self.parent = parent
                
        self.panel, self.input = self.__create()
        
    def __create(self):
        """Create widget"""
        cmdprompt = wx.Panel(self.parent)
        
        label = wx.Button(parent = cmdprompt, id = wx.ID_ANY,
                          label = _("Cmd >"), size = (-1, 25))
        label.SetToolTipString(_("Click for erasing command prompt"))
        
        cmdinput = wx.TextCtrl(parent = cmdprompt, id = wx.ID_ANY,
                               value = "",
                               style = wx.TE_LINEWRAP | wx.TE_PROCESS_ENTER,
                               size = (-1, 25))
        
        cmdinput.SetFont(wx.Font(10, wx.FONTFAMILY_MODERN, wx.NORMAL, wx.NORMAL, 0, ''))
        
        wx.CallAfter(cmdinput.SetInsertionPoint, 0)
        
        label.Bind(wx.EVT_BUTTON,        self.OnCmdErase)
        cmdinput.Bind(wx.EVT_TEXT_ENTER, self.OnRunCmd)
        cmdinput.Bind(wx.EVT_TEXT,       self.OnUpdateStatusBar)
        
        # layout
        sizer = wx.BoxSizer(wx.HORIZONTAL)
        sizer.Add(item = label, proportion = 0,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT |
                  wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER,
                  border = 3)
        sizer.Add(item = cmdinput, proportion = 1,
                  flag = wx.EXPAND | wx.ALL,
                  border = 1)
        
        cmdprompt.SetSizer(sizer)
        sizer.Fit(cmdprompt)
        cmdprompt.Layout()
        
        return cmdprompt, cmdinput

    def GetPanel(self):
        """Get main widget panel"""
        return self.panel
    
    def OnCmdErase(self, event):
        """Erase command prompt"""
        self.input.SetValue('')
        
    def OnRunCmd(self, event):
        """Run command"""
        cmdString = event.GetString()
        
        if self.parent.GetName() != "LayerManager":
            return
        
        if cmdString[:2] == 'd.' and not self.parent.curr_page:
            self.parent.NewDisplay(show=True)
        
        cmd = shlex.split(str(cmdString))
        if len(cmd) > 1:
            self.parent.goutput.RunCmd(cmd, switchPage = True)
        else:
            self.parent.goutput.RunCmd(cmd, switchPage = False)
        
        self.OnUpdateStatusBar(None)
        
    def OnUpdateStatusBar(self, event):
        """Update Layer Manager status bar"""
        if self.parent.GetName() != "LayerManager":
            return
        
        if event is None:
            self.parent.statusbar.SetStatusText("")
        else:
            self.parent.statusbar.SetStatusText(_("Type GRASS command and run by pressing ENTER"))
