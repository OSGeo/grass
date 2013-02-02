"""!
@package mapswipe.dialogs

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
from gui_core.dialogs   import SimpleDialog
from gui_core           import gselect
from gui_core.widgets   import SimpleValidator
from core.gcmd          import GMessage

class SwipeMapDialog(SimpleDialog):
    """!Dialog used to select raster maps"""
    def __init__(self, parent, title = _("Select raster maps"), first = None, second = None):
        SimpleDialog.__init__(self, parent, title)

        self.element1 = gselect.Select(parent = self.panel, type = 'raster',
                                      size = globalvar.DIALOG_GSELECT_SIZE,
                                      validator = SimpleValidator(callback = self.ValidatorCallback))
        
        self.element2 = gselect.Select(parent = self.panel, type = 'raster',
                                       size = globalvar.DIALOG_GSELECT_SIZE,
                                       validator = SimpleValidator(callback = self.ValidatorCallback))
        
        self.element1.SetFocus()
        if first:
            self.element1.SetValue(first)
        if second:
            self.element2.SetValue(second)
            
        self._layout()
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """!Do layout"""
        self.dataSizer.Add(wx.StaticText(self.panel, id = wx.ID_ANY,
                                         label = _("Name of top/left raster map:")),
                           proportion = 0, flag = wx.EXPAND | wx.ALL, border = 5)
        self.dataSizer.Add(self.element1, proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)
 
        self.dataSizer.Add(wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                         label = _("Name of bottom/right raster map:")), proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)

        self.dataSizer.Add(self.element2, proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 1)
       
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def ValidatorCallback(self, win):
        if win == self.element1.GetTextCtrl():
            GMessage(parent = self, message = _("Name of the first map is missing."))
        else:
            GMessage(parent = self, message = _("Name of the second map is missing."))

    def GetValues(self):
        """!Get raster maps"""
        return (self.element1.GetValue(), self.element2.GetValue())

