#!/usr/bin/env python

import os
import sys
import pwd

from grass.script import core as grass

import wx
import wx.lib.mixins.listctrl as listmix

from core.gcmd import RunCommand 
from core.utils import _, GuiModuleMain
from gui_core.preferences import MapsetAccess

def main():
    app = wx.App()

    dlg = MapsetAccess(parent = None)
    dlg.CenterOnScreen()
        
    if dlg.ShowModal() == wx.ID_OK:
        ms = dlg.GetMapsets()
        RunCommand('g.mapsets',
                   parent = None,
                   mapset = '%s' % ','.join(ms),
                   operation = 'set')
    
    app.MainLoop()

if __name__ == "__main__":
    GuiModuleMain(main)

