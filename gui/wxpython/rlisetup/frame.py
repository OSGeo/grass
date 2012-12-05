# -*- coding: utf-8 -*-
"""
Created on Mon Nov 26 11:57:54 2012

@author: lucadelu
"""

import wx
import os

from core import globalvar, gcmd
from grass.script import core as grass
from rlisetup.functions import retRLiPath
from rlisetup.wizard import RLIWizard


class RLiSetupFrame(wx.Frame):

    def __init__(self, parent, giface = None, id=wx.ID_ANY, title=_("GRASS" \
                 " GIS Setup for r.li modules"), 
                 style=wx.DEFAULT_FRAME_STYLE | wx.RESIZE_BORDER, **kwargs):
        ###VARIABLES
        self.parent = parent
        self.cmd = "r.li.setup"
        self.rlipath = retRLiPath()
        self.listfiles = self.ListFiles()
        ###END VARIABLES
        #init of frame
        wx.Frame.__init__(self, parent=parent, id=id, title=title,
                          **kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'),
                             wx.BITMAP_TYPE_ICO))
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)
        #box for select configuration file
        self.confilesBox = wx.StaticBox(parent=self.panel, id=wx.ID_ANY,
                                        label=_('Available sampling area configuration files'))
        self.listfileBox = wx.ListBox(parent=self.panel,  id=wx.ID_ANY,
                    choices=self.listfiles)
        ###BUTTONS
        #definition
        self.btn_close = wx.Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btn_help = wx.Button(parent=self.panel, id=wx.ID_HELP)
        self.btn_remove = wx.Button(parent=self.panel, id=wx.ID_ANY,
                                    label=_("Remove"))
        self.btn_remove.SetToolTipString(_('Remove a configuration file'))
        self.btn_new = wx.Button(parent=self.panel, id=wx.ID_ANY,
                                 label=_("Create"))
        self.btn_new.SetToolTipString(_('Create a new configuration file'))
        self.btn_rename = wx.Button(parent=self.panel, id=wx.ID_ANY,
                                    label=_("Rename"))
        self.btn_rename.SetToolTipString(_('Rename a configuration file'))
        #set action for button
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.btn_remove.Bind(wx.EVT_BUTTON, self.OnRemove)
        self.btn_new.Bind(wx.EVT_BUTTON, self.OnNew)
        self.btn_rename.Bind(wx.EVT_BUTTON, self.OnRename)
        self._layout()
        ###END BUTTONS

        ###SIZE FRAME
        self.SetMinSize(self.GetBestSize())
        ##Please check this because without this the size it is not the min
        self.SetClientSize(self.GetBestSize())
        ###END SIZE

    def _layout(self):
        """Set the layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        ###CONFILES
        confilesSizer = wx.StaticBoxSizer(self.confilesBox, wx.HORIZONTAL)
        confilesSizer.Add(item=self.listfileBox, proportion=1,
                         flag=wx.EXPAND)
        ###END CONFILES
        ###BUTTONS
        buttonSizer = wx.BoxSizer(wx.HORIZONTAL)
        buttonSizer.Add(item=self.btn_new, flag=wx.ALL, border=5)
        buttonSizer.Add(item=self.btn_rename, flag=wx.ALL, border=5)
        buttonSizer.Add(item=self.btn_remove, flag=wx.ALL, border=5)
        buttonSizer.Add(item=self.btn_help, flag=wx.ALL, border=5)
        buttonSizer.Add(item=self.btn_close, flag=wx.ALL, border=5)
        ###END BUTTONS
        #add to sizer
        sizer.Add(item=confilesSizer, proportion=0,
                  flag=wx.ALIGN_LEFT | wx.EXPAND | wx.ALL, border=3)
        sizer.Add(item=buttonSizer, proportion=0,
                  flag=wx.ALIGN_RIGHT | wx.ALL, border=3)
        #set dimension
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        self.Layout()

    def ListFiles(self):
        """!Check the configuration files inside the path"""
        # list of configuration file
        listfiles = []
        #return all the configuration files in self.rlipath, check if there are
        #link or directory and doesn't add them
        for l in os.listdir(self.rlipath):
            if os.path.isfile(os.path.join(self.rlipath, l)):
                listfiles.append(l)
        return listfiles

    def OnClose(self, event):
        """!Close window"""
        self.Destroy()

    def OnHelp(self, event):
        """!Launches help"""
        gcmd.RunCommand('g.manual', parent = self, entry = 'wxGUI.rlisetup')

    def OnRemove(self, event):
        """!Remove configuration file from path and update the list"""
        confile = self.listfiles[self.listfileBox.GetSelections()[0]]
        self.listfileBox.Delete(self.listfileBox.GetSelections()[0])
        grass.try_remove(os.path.join(self.rlipath, confile))
        self.listfiles = self.ListFiles()
        return

    def OnNew(self, event):
        """!Remove configuration file from path and update the list"""
        RLIWizard(self)
        self.listfiles = self.ListFiles()
        self.listfileBox.Clear()
        self.listfileBox.Set(self.listfiles)

    def OnRename(self, event):
        """!Rename an existing configuration file"""
        try:
            confile = self.listfiles[self.listfileBox.GetSelections()[0]]
        except:
            gcmd.GMessage(parent=self,
                          message=_("You have to select a configuration file"))
            return
        dlg = wx.TextEntryDialog(parent=self.parent,
                                 message=_('Set te new name for %s " \
                                           "configuration file') % confile,
                                 caption=_('Rename configuration file'))
        if dlg.ShowModal() == wx.ID_OK:
            res = dlg.GetValue()
            newname = "%s%s%s" % (self.rlipath, os.sep, res)
            os.rename(os.path.join(self.rlipath, confile), newname)
            self.listfiles = self.ListFiles()
            self.listfileBox.Clear()
            self.listfileBox.Set(self.listfiles)
