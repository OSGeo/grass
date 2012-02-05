"""!
@package example.dialogs

@brief Dialogs used in Example tool

Classes:
 - dialogs::ExampleMapDialog

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

from core               import globalvar
from gui_core.dialogs   import ElementDialog, GroupDialog
from gui_core           import gselect

class ExampleMapDialog(ElementDialog):
    """!Dialog for adding raster map.
    
    Dialog can be easily changed to enable to choose vector, imagery groups or other elements.
    """
    def __init__(self, parent, title = _("Choose raster map"), id = wx.ID_ANY):
        """!Calls super class constructor.
        
        @param parent gui parent
        @param title dialog window title
        @param id id
        """
        ElementDialog.__init__(self, parent, title, label = _("Name of raster map:"))
        
        # here is the place to determine element type
        self.element = gselect.Select(parent = self.panel, type = 'raster',
                                      size = globalvar.DIALOG_GSELECT_SIZE)
        
        self.PostInit()
        
        self.__Layout()
        
        self.SetMinSize(self.GetSize())

    def __Layout(self):
        """!Do layout"""
        self.dataSizer.Add(item = self.element, proportion = 0,
                           flag = wx.EXPAND | wx.ALL, border = 5)
                            
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)
        
    def GetRasterMap(self):
        """!Returns selected raster map"""
        return self.GetElement()

