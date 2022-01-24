"""
@package frame.statusbar

@brief Classes for main window statusbar management

Classes:
 - statusbar::SbMask

(C) 2022 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
@author Linda Kladivova <lindakladivova gmail.com>
"""

import wx

from core.gcmd import RunCommand
from gui_core.wrap import Button
from grass.script import core as grass


class SbMask:
    """Button to show whether mask is activated and remove mask with
    left mouse click
    """

    def __init__(self, parent, giface):
        self.name = "mask"
        self.mask_layer = "MASK"
        self.parent = parent
        self.giface = giface
        self.widget = Button(
            parent=parent, id=wx.ID_ANY, label=_(self.mask_layer), style=wx.NO_BORDER
        )
        self.widget.Bind(wx.EVT_BUTTON, self.OnRemoveMask)
        self.widget.SetForegroundColour(wx.Colour(255, 0, 0))
        self.widget.SetToolTip(tip=_("Left mouse click to remove the MASK"))
        self.giface.updateMap.connect(self.Refresh)
        self.giface.currentMapsetChanged.connect(self.Refresh)
        self.giface.grassdbChanged.connect(self._dbChanged)
        self.Refresh()

    def _dbChanged(self, map=None, newname=None):
        if map == self.mask_layer or newname == self.mask_layer:
            self.Refresh()

    def Show(self):
        """Invokes showing of underlying widget.

        In derived classes it can do what is appropriate for it,
        e.g. showing text on statusbar (only).
        """
        self.widget.Show()

    def Hide(self):
        self.widget.Hide()

    def SetValue(self, value):
        self.widget.SetValue(value)

    def GetValue(self):
        return self.widget.GetValue()

    def GetWidget(self):
        """Returns underlying widget.

        :return: widget or None if doesn't exist
        """
        return self.widget

    def Refresh(self):
        """Show mask in the statusbar if mask file found"""
        if grass.find_file(
            name=self.mask_layer, element="cell", mapset=grass.gisenv()["MAPSET"]
        )["name"]:
            self.Show()
        else:
            self.Hide()

    def OnRemoveMask(self, event):
        dlg = wx.MessageDialog(
            self.parent,
            message=_("Are you sure that you want to remove the MASK?"),
            caption=_("Remove MASK"),
            style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
        )
        if dlg.ShowModal() != wx.ID_YES:
            dlg.Destroy()
            return
        RunCommand("r.mask", flags="r")
        self.giface.updateMap.emit(render=False)
        self.giface.grassdbChanged.emit(
            grassdb=grass.gisenv()["GISDBASE"],
            location=grass.gisenv()["LOCATION_NAME"],
            mapset=grass.gisenv()["MAPSET"],
            map=self.mask_layer,
            action="delete",
            element="raster",
        )
