"""!
@package example.toolbars

@brief Example toolbars and icons.

Classes:
 - toolbars::ExampleMapToolbar
 - toolbars::ExampleMainToolbar
 - toolbars::ExampleMiscToolbar

(C) 2011-2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import wx

from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon


class ExampleMapToolbar(BaseToolbar):
    """!Map toolbar (to control map zoom and rendering)
    """
    def __init__(self, parent, toolSwitcher):
        """!Map toolbar constructor
        """
        BaseToolbar.__init__(self, parent, toolSwitcher)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

        self._default = self.pan

        for tool in (self.pan, self.zoomIn, self.zoomOut):
            self.toolSwitcher.AddToolToGroup(group='mouseUse', toolbar=self, tool=tool)

        self.EnableTool(self.zoomBack, False)

    def _toolbarData(self):
        """!Returns toolbar data (name, icon, handler)"""
        # BaseIcons are a set of often used icons. It is possible
        # to reuse icons in ./trunk/gui/icons/grass or add new ones there.
        icons = BaseIcons
        return self._getToolbarData((("displaymap", icons["display"],
                                      self.parent.OnDraw),
                                     ("rendermap", icons["render"],
                                      self.parent.OnRender),
                                     ("erase", icons["erase"],
                                      self.parent.OnErase),
                                     (None, ),  # creates separator
                                     ("pan", icons["pan"],
                                      self.parent.OnPan,
                                      wx.ITEM_CHECK),  # toggle tool
                                     ("zoomIn", icons["zoomIn"],
                                      self.parent.OnZoomIn,
                                      wx.ITEM_CHECK),
                                     ("zoomOut", icons["zoomOut"],
                                      self.parent.OnZoomOut,
                                      wx.ITEM_CHECK),
                                     (None, ),
                                     ("zoomBack", icons["zoomBack"],
                                      self.parent.OnZoomBack),
                                     ("zoomToMap", icons["zoomExtent"],
                                      self.parent.OnZoomToMap),
                                     ))


class ExampleMainToolbar(BaseToolbar):
    """!Toolbar with tools related to application functionality
    """
    def __init__(self, parent):
        """!Toolbar constructor
        """
        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data"""
        return self._getToolbarData((("addRaster", BaseIcons['addRast'],
                                      self.parent.OnSelectRaster),
                                     ))


class ExampleMiscToolbar(BaseToolbar):
    """!Toolbar with miscellaneous tools related to app
    """
    def __init__(self, parent):
        """!Toolbar constructor
        """
        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data"""
        icons = BaseIcons
        return self._getToolbarData((("help", icons['help'],
                                      self.parent.OnHelp),
                                     ("quit", icons['quit'],
                                      self.parent.OnCloseWindow),
                                     ))
