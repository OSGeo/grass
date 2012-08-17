"""!
@package swipe.dialogs

@brief Dialogs used in Map Swipe

Classes:
 - dialogs::SwipeMapDialog

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

from core               import globalvar
from gui_core.dialogs   import ElementDialog
from gui_core           import gselect

class SwipeMapDialog(ElementDialog):
    """!Dialog used to select raster maps"""
    def __init__(self, parent, title = _("Select raster maps"), id =  wx.ID_ANY, first = None, second = None):
        ElementDialog.__init__(self, parent, title, id = id, label = _("Name of top/left raster map:"))

        self.element = gselect.Select(parent = self.panel, type = 'raster',
                                      size = globalvar.DIALOG_GSELECT_SIZE)
        
        self.element2 = gselect.Select(parent = self.panel, type = 'raster',
                                       size = globalvar.DIALOG_GSELECT_SIZE)
        
        self.PostInit()
        
        if first:
            self.element.SetValue(first)
        if second:
            self.element2.SetValue(second)
            
        self._layout()
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """!Do layout"""
        self.dataSizer.Add(self.element, proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)
 
        self.dataSizer.Add(wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                         label = _("Name of bottom/right raster map:")), proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)

        self.dataSizer.Add(self.element2, proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)
       
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetValues(self):
        """!Get raster maps"""
        return (self.GetElement(), self.element2.GetValue())

