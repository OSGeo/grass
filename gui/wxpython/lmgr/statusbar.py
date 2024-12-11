"""
@package frame.statusbar

@brief Classes for main window statusbar management

Classes:
 - statusbar::SbMain
 - statusbar::SbMask

(C) 2022 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Kladivova <lindakladivova gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
"""

import wx

import grass.script as gs

from core.gcmd import RunCommand
from core.watchdog import watchdog_used
from gui_core.wrap import Button


class SbMain:
    """Statusbar for main window."""

    def __init__(self, parent, giface):
        self.parent = parent
        self.giface = giface
        self.widget = wx.StatusBar(self.parent, id=wx.ID_ANY)
        self.widget.SetMinHeight(24)
        self.widget.SetFieldsCount(2)
        self.widget.SetStatusWidths([-1, 100])
        self.mask = SbMask(self.widget, self.giface)
        self.widget.Bind(wx.EVT_SIZE, self.OnSize)
        self._repositionStatusbar()

    def GetWidget(self):
        """Returns underlying widget.

        :return: widget or None if doesn't exist
        """
        return self.widget

    def _repositionStatusbar(self):
        """Reposition widgets in main window statusbar"""
        rect1 = self.GetWidget().GetFieldRect(1)
        rect1.x += 1
        rect1.y += 1
        self.mask.GetWidget().SetRect(rect1)

    def Refresh(self):
        """Refresh statusbar. So far it refreshes just a mask."""
        self.mask.Refresh()

    def OnSize(self, event):
        """Adjust main window statusbar on changing size"""
        self._repositionStatusbar()

    def SetStatusText(self, *args):
        """Override wx.StatusBar method"""
        self.GetWidget().SetStatusText(*args)


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
        self.giface.currentMapsetChanged.connect(self.Refresh)
        if not watchdog_used:
            self.giface.grassdbChanged.connect(self.dbChanged)
        self.Refresh()

    def dbChanged(self, map=None, newname=None):
        """Mapset files changed

        :param str map: map that is changed
        :param str newname: new map
        """
        if self.mask_layer in {map, newname}:
            self.Refresh()
            self.giface.updateMap.emit()

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
        if gs.find_file(
            name=self.mask_layer, element="cell", mapset=gs.gisenv()["MAPSET"]
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
        gisenv = gs.gisenv()
        self.giface.grassdbChanged.emit(
            grassdb=gisenv["GISDBASE"],
            location=gisenv["LOCATION_NAME"],
            mapset=gisenv["MAPSET"],
            map=self.mask_layer,
            action="delete",
            element="raster",
        )
