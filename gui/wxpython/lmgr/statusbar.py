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

import json
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
        self.widget.SetFieldsCount(3)
        self.widget.SetStatusWidths([-1, 100, 100])
        self.mask = SbMask(self.widget, self.giface)
        self.crs = SbCRS(self.widget, self.giface)
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

        rect2 = self.GetWidget().GetFieldRect(2)
        rect2.x += 1
        rect2.y += 1
        self.crs.GetWidget().SetRect(rect2)

    def Refresh(self):
        """Refresh statusbar"""
        self.mask.Refresh()
        self.crs.Refresh()

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
        self.parent = parent
        self.giface = giface
        self.widget = Button(
            parent=parent, id=wx.ID_ANY, label=_("Mask"), style=wx.NO_BORDER
        )
        self.widget.Bind(wx.EVT_BUTTON, self.OnRemoveMask)
        self.widget.SetForegroundColour(wx.Colour(255, 0, 0))
        self.widget.SetToolTip(tip=_("Left mouse click to remove the raster mask"))
        self.giface.currentMapsetChanged.connect(self.Refresh)
        if not watchdog_used:
            self.giface.grassdbChanged.connect(self.dbChanged)
        self.Refresh()

    def dbChanged(self, map=None, newname=None):
        """Mapset files changed

        :param str map: map that is changed
        :param str newname: new map
        """
        mask_layer = gs.parse_command("r.mask.status", format="json")["name"].split(
            "@", maxsplit=1
        )[0]
        # This assumes mask is always in the current mapset (or the event is triggered
        # only for the current mapset).
        if mask_layer in {map, newname}:
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
        if gs.parse_command("r.mask.status", format="json")["present"]:
            self.Show()
        else:
            self.Hide()

    def OnRemoveMask(self, event):
        dlg = wx.MessageDialog(
            self.parent,
            message=_("Are you sure that you want to remove the current raster mask?"),
            caption=_("Remove raster mask"),
            style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
        )
        if dlg.ShowModal() != wx.ID_YES:
            dlg.Destroy()
            return
        RunCommand("r.mask", flags="r")
        mask_full_name = gs.parse_command("r.mask.status", format="json")["name"]
        mask_name, mask_mapset = mask_full_name.split("@", maxsplit=1)
        gisenv = gs.gisenv()
        self.giface.grassdbChanged.emit(
            grassdb=gisenv["GISDBASE"],
            location=gisenv["LOCATION_NAME"],
            mapset=mask_mapset,
            map=mask_name,
            action="delete",
            element="raster",
        )


class SbCRS:
    """Button to show the EPSG code if available, otherwise 'Custom CRS'.
    Clicking it prints Coordinate Reference System (CRS) information for
    the current project to the console.
    """

    def __init__(self, parent, giface):
        self.name = "crs"
        self.giface = giface
        self.widget = Button(
            parent=parent, id=wx.ID_ANY, label=_("Custom CRS"), style=wx.NO_BORDER
        )
        self.widget.Bind(wx.EVT_BUTTON, self.OnShowProjection)
        self.widget.SetToolTip(
            tip=_("Left mouse click to print detailed projection info")
        )

        self.giface.currentMapsetChanged.connect(self.Refresh)
        self.Refresh()

    def Show(self):
        self.widget.Show()

    def Hide(self):
        self.widget.Hide()

    def GetWidget(self):
        """Returns underlying widget.

        :return: widget or None if doesn't exist
        """
        return self.widget

    def Refresh(self):
        """Fetch the EPSG code, or display 'Custom CRS' if not found"""

        label = _("Custom CRS")

        try:
            proj_str = gs.read_command(
                "g.proj", flags="p", format="projjson", quiet=True
            )
            if proj_str:
                proj_info = json.loads(proj_str)
                proj_id = proj_info.get("id", {})
                if proj_id.get("authority") == "EPSG" and proj_id.get("code"):
                    label = f"EPSG: {proj_id['code']}"
        except (TypeError, KeyError, json.JSONDecodeError):
            pass

        self.widget.SetLabel(label)
        self.Show()

    def OnShowProjection(self, event):
        self.giface.RunCmd(["g.proj", "-p"])
