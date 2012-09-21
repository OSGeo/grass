#!/usr/bin/env python

import os
import sys
import pwd

from grass.script import core as grass

import wx
import wx.lib.mixins.listctrl as listmix

class MapsetsFrame(wx.Frame):
    def __init__(self):
        wx.Frame.__init__(self, parent = None, id = wx.ID_ANY,
                          title = "Check mapsets to access")
        
        
        self.SetMinSize((350, 400))

        sizer = wx.BoxSizer(wx.VERTICAL)
        
        # list of mapsets
        self.mapsetlb = CheckListMapset(parent = self)
        self.mapsetlb.LoadData()
        
        sizer.Add(item = self.mapsetlb, proportion = 1,
                  flag = wx.ALL | wx.EXPAND, border = 5)

        
        # dialog buttons
        line = wx.StaticLine(parent = self, id = wx.ID_ANY,
                             style = wx.LI_HORIZONTAL)
        sizer.Add(item = line, proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_CENTRE | wx.ALL, border = 5)
        
        btnsizer = wx.StdDialogButtonSizer()
        btnOK = wx.Button(self, wx.ID_OK)
        btnOK.Bind(wx.EVT_BUTTON, self.OnOK)
        btnOK.SetToolTipString("Close dialog and apply changes")
        btnOK.SetDefault()
        btnsizer.AddButton(btnOK)
        
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        btnCancel.SetToolTipString("Close dialog and ignore changes")
        btnsizer.AddButton(btnCancel)
        btnsizer.Realize()
        
        sizer.Add(item = btnsizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALIGN_RIGHT | wx.ALL, border = 5)
        
        # do layout
        self.Layout()
        self.SetSizer(sizer)
        sizer.Fit(self)

    def GetMapsets(self):
        """Get list of checked mapsets"""
        ms = []
        i = 0
        for mset in self.mapsetlb.mapsets:
            if self.mapsetlb.IsChecked(i):
                ms.append(mset)
            i += 1

        return ms

    def OnCancel(self, event):
        """Button 'Cancel' pressed"""
        self.Close()
        
    def OnOK(self, event):
        """Button 'OK' pressed"""
        mapsets = ','.join(self.GetMapsets())

        grass.run_command('g.mapsets',
                          quiet = True,
                          mapset = mapsets,
                          operation = 'set')
        
        self.Close()
        
class CheckListMapset(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.CheckListCtrlMixin):
    """List of mapset/owner/group"""
    def __init__(self, parent, pos = wx.DefaultPosition,
                 log = None):
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, wx.ID_ANY,
                             style = wx.LC_REPORT)
        listmix.CheckListCtrlMixin.__init__(self)
        self.log = log
        
        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)
    
    def LoadData(self):
        """Load data into list"""
        self.InsertColumn(0, 'Mapset')
        self.InsertColumn(1, 'Owner')
        ### self.InsertColumn(2, 'Group')

        gisenv = grass.gisenv()
        locationPath = os.path.join(gisenv['GISDBASE'], gisenv['LOCATION_NAME'])
        self.curr_mapset = gisenv['MAPSET']
        
        ret = grass.read_command('g.mapsets',
                                 quiet = True,
                                 flags = 'l',
                                 sep = 'newline')
        self.mapsets = []
        if ret:
            self.mapsets = ret.splitlines()
            

        ret = grass.read_command('g.mapsets',
                                 quiet = True,
                                 flags = 'p',
                                 sep = 'newline')
        mapsets_access = []
        if ret:
            mapsets_access = ret.splitlines()
            
        for mapset in self.mapsets:
            index = self.InsertStringItem(sys.maxint, mapset)
            mapsetPath = os.path.join(locationPath,
                                      mapset)
            stat_info = os.stat(mapsetPath)
            if os.name in ('posix', 'mac'):
                self.SetStringItem(index, 1, "%s" % pwd.getpwuid(stat_info.st_uid)[0])
                # FIXME: get group name
                ### self.SetStringItem(index, 2, "%-8s" % stat_info.st_gid) 
            else:
                # FIXME: no pwd under MS Windows (owner: 0, group: 0)
                self.SetStringItem(index, 1, "%-8s" % stat_info.st_uid)
                ### self.SetStringItem(index, 2, "%-8s" % stat_info.st_gid)

            if mapset in mapsets_access:
                self.CheckItem(self.mapsets.index(mapset), True)
            
        self.SetColumnWidth(col = 0, width = wx.LIST_AUTOSIZE)
        self.SetColumnWidth(col = 1, width = wx.LIST_AUTOSIZE)

    def OnCheckItem(self, index, flag):
        """Mapset checked/unchecked"""
        mapset = self.mapsets[index]
        if mapset == 'PERMANENT' or mapset == self.curr_mapset:
            self.CheckItem(index, True)
    
class MyApp(wx.App):
    def OnInit(self):
        frame = MapsetsFrame()
        
        frame.CentreOnScreen()
        frame.Show()
        
        self.SetTopWindow(frame)
        
        return True

if __name__ == "__main__":
    app = MyApp(0)
    app.MainLoop()
