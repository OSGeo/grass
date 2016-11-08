"""
@package gui_core.wrap

@brief Core wrapped wxpython widgets 

Classes:
 - wrap::GSpinCtrl


(C) 2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import wx

from core.globalvar import gtk3


class GSpinCtrl(wx.SpinCtrl):
    """Wrapper around wx.SpinCtrl to have more control
    over the widget on different platforms"""

    gtk3MinSize = 130

    def __init__(self, *args, **kwargs):
        if gtk3:
            if 'size' in kwargs:
                kwargs['size'] = wx.Size(max(self.gtk3MinSize, kwargs['size'][0]), kwargs['size'][1])
            else:
                kwargs['size'] = wx.Size(self.gtk3MinSize, -1)
                
        wx.SpinCtrl.__init__(self, *args, **kwargs)