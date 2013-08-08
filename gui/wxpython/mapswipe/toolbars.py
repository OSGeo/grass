"""!
@package swipe.toolbars

@brief Map Swipe toolbars and icons.

Classes:
 - toolbars::SwipeMapToolbar
 - toolbars::SwipeMainToolbar
 - toolbars::SwipeMiscToolbar

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import wx

from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon
from core.utils import _


swipeIcons = {
        'tools': MetaIcon(img = 'tools', label = _("Tools")),
        'quit' : BaseIcons['quit'].SetLabel(_("Quit Map Swipe")),
        'addRast' : BaseIcons['addRast'].SetLabel(_("Select raster maps")),
        }

class SwipeMapToolbar(BaseToolbar):
    """!Map toolbar (to control map zoom and rendering)
    """
    def __init__(self, parent, toolSwitcher):
        """!Map toolbar constructor
        """
        BaseToolbar.__init__(self, parent, toolSwitcher)
        
        self.InitToolbar(self._toolbarData())
        self._default = self.pan

        # realize the toolbar
        self.Realize()
        
        for tool in (self.pointer, self.pan, self.zoomIn, self.zoomOut):
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
                                     (None, ), # creates separator
                                     ("pointer", icons["pointer"],
                                      self.parent.OnPointer,
                                      wx.ITEM_CHECK),
                                     ("pan", icons["pan"],
                                      self.parent.OnPan,
                                      wx.ITEM_CHECK), # toggle tool
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
                                     (None, ),
                                     ('saveFile', icons['saveFile'],
                                      self.parent.SaveToFile),
                                    ))
                                    
    def SetActiveMap(self, index):
        """!Set currently selected map.
        Unused, needed because of DoubleMapFrame API.
        """
        pass

class SwipeMainToolbar(BaseToolbar):
    """!Toolbar with tools related to application functionality
    """
    def __init__(self, parent):
        """!Toolbar constructor
        """
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())

        # add tool to toggle active map window
        self.toggleModeId = wx.NewId()
        self.toggleMode = wx.Choice(parent = self, id = self.toggleModeId)
        for label, cdata in zip([_('Swipe mode'), _('Mirror mode')], ['swipe', 'mirror']):
            self.toggleMode.Append(label, cdata)
        self.toggleMode.SetSelection(0)
        self.toggleMode.SetSize(self.toggleMode.GetBestSize())
        self.toggleMode.Bind(wx.EVT_CHOICE,
                             lambda event: 
                             self.parent.SetViewMode(self.toggleMode.GetClientData(event.GetSelection())))
        self.InsertControl(3, self.toggleMode)

        help = _("Choose view mode")
        self.SetToolShortHelp(self.toggleModeId, help)
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        return self._getToolbarData((("addRaster", swipeIcons['addRast'],
                                      self.parent.OnSelectRasters),
                                     (None, ),
                                     ("tools", swipeIcons['tools'],
                                      self.OnToolMenu)
                                    ))

    def SetMode(self, mode):
        for i in range(self.toggleMode.GetCount()):
            if mode == self.toggleMode.GetClientData(i):
                self.toggleMode.SetSelection(i)

    def OnToolMenu(self, event):
        """!Menu for additional tools"""
        toolMenu = wx.Menu()
        
        for label, itype, handler, desc in (
            (_("Switch orientation"),
             wx.ITEM_NORMAL, self.parent.OnSwitchOrientation, "switchOrientation"),
            (_("Switch maps"),
             wx.ITEM_NORMAL, self.parent.OnSwitchWindows, "switchMaps")):
            # Add items to the menu
            item = wx.MenuItem(parentMenu = toolMenu, id = wx.ID_ANY,
                               text = label,
                               kind = itype)
            toolMenu.AppendItem(item)
            self.parent.GetWindow().Bind(wx.EVT_MENU, handler, item)
        
        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.parent.GetWindow().PopupMenu(toolMenu)
        toolMenu.Destroy()


class SwipeMiscToolbar(BaseToolbar):
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
        return self._getToolbarData((("help", BaseIcons['help'],
                                      self.parent.OnHelp),
                                    ("quit", swipeIcons['quit'],
                                      self.parent.OnCloseWindow),
                                     ))
