"""
@package photo2image.ip2i_toolbars

@brief Georectification module - toolbars

Classes:
 - toolbars::GCPMapToolbar
 - toolbars::GCPDisplayToolbar

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Markus Metz
"""

import wx

from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon


class GCPManToolbar(BaseToolbar):
    """Toolbar for managing ground control points

    :param parent: reference to GCP widget
    """

    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        icons = {
            "gcpAdd": MetaIcon(img="gcp-add", label=_("Add new GCP to the list")),
            "gcpDelete": MetaIcon(img="gcp-delete", label=_("Delete selected GCP")),
            "gcpClear": MetaIcon(img="gcp-remove", label=_("Clear selected GCP")),
            "gcpRms": MetaIcon(img="gcp-rms", label=_("Recalculate RMS error")),
            "georectify": MetaIcon(img="georectify", label=_("Georectify")),
            "gcpSave": MetaIcon(img="gcp-save", label=_("Save GCPs to POINTS file")),
            "gcpReload": MetaIcon(
                img="reload", label=_("Reload GCPs from POINTS file")
            ),
        }

        return self._getToolbarData(
            (
                (
                    ("gcpAdd", icons["gcpAdd"].label),
                    icons["gcpAdd"],
                    self.parent.AddGCPm,
                ),
                (
                    ("gcpDelete", icons["gcpDelete"].label),
                    icons["gcpDelete"],
                    self.parent.DeleteGCP,
                ),
                (
                    ("gcpClear", icons["gcpDelete"].label),
                    icons["gcpClear"],
                    self.parent.ClearGCP,
                ),
                (None,),
                (
                    ("rms", icons["gcpRms"].label),
                    icons["gcpRms"],
                    self.parent.OnRMS,
                ),
                (
                    ("georect", icons["georectify"].label),
                    icons["georectify"],
                    self.parent.OnGeorect,
                ),
                (None,),
                (
                    ("gcpSave", icons["gcpSave"].label),
                    icons["gcpSave"],
                    self.parent.SaveGCPs,
                ),
                (
                    ("gcpReload", icons["gcpReload"].label),
                    icons["gcpReload"],
                    self.parent.ReloadGCPs,
                ),
            )
        )


class GCPDisplayToolbar(BaseToolbar):
    """GCP Display toolbar"""

    def __init__(self, parent, toolSwitcher):
        """GCP Display toolbar constructor"""
        BaseToolbar.__init__(self, parent, toolSwitcher)

        self.InitToolbar(self._toolbarData())
        self._default = self.gcpset

        # add tool to toggle active map window
        self.togglemap = wx.Choice(
            parent=self, id=wx.ID_ANY, choices=[_("source"), _("target")]
        )

        self.InsertControl(10, self.togglemap)

        self.SetToolShortHelp(
            self.togglemap.GetId(),
            "%s %s %s"
            % (
                _("Set map canvas for "),
                BaseIcons["zoomBack"].GetLabel(),
                _(" / Zoom to map"),
            ),
        )

        for tool in (self.gcpset, self.pan, self.zoomin, self.zoomout):
            self.toolSwitcher.AddToolToGroup(group="mouseUse", toolbar=self, tool=tool)

        # realize the toolbar
        self.Realize()

        self.EnableTool(self.zoomback, False)

    def _toolbarData(self):
        """Toolbar data"""
        icons = {
            "gcpSet": MetaIcon(
                img="gcp-create",
                label=_("Update GCP coordinates"),
                desc=_("Update GCP coordinates)"),
            ),
            "quit": BaseIcons["quit"],
            "settings": BaseIcons["settings"],
            "help": BaseIcons["help"],
        }

        return self._getToolbarData(
            (
                (
                    ("displaymap", BaseIcons["display"].label),
                    BaseIcons["display"],
                    self.parent.OnDraw,
                ),
                (
                    ("rendermap", BaseIcons["render"].label),
                    BaseIcons["render"],
                    self.parent.OnRender,
                ),
                (
                    ("erase", BaseIcons["erase"].label),
                    BaseIcons["erase"],
                    self.parent.OnErase,
                ),
                (None,),
                (
                    ("gcpset", icons["gcpSet"].label),
                    icons["gcpSet"],
                    self.parent.OnPointer,
                    wx.ITEM_CHECK,
                ),
                (
                    ("pan", BaseIcons["pan"].label),
                    BaseIcons["pan"],
                    self.parent.OnPan,
                    wx.ITEM_CHECK,
                ),
                (
                    ("zoomin", BaseIcons["zoomIn"].label),
                    BaseIcons["zoomIn"],
                    self.parent.OnZoomIn,
                    wx.ITEM_CHECK,
                ),
                (
                    ("zoomout", BaseIcons["zoomOut"].label),
                    BaseIcons["zoomOut"],
                    self.parent.OnZoomOut,
                    wx.ITEM_CHECK,
                ),
                (
                    ("zoommenu", BaseIcons["zoomMenu"].label),
                    BaseIcons["zoomMenu"],
                    self.parent.OnZoomMenuGCP,
                ),
                (None,),
                (
                    ("zoomback", BaseIcons["zoomBack"].label),
                    BaseIcons["zoomBack"],
                    self.parent.OnZoomBack,
                ),
                (
                    ("zoomtomap", BaseIcons["zoomExtent"].label),
                    BaseIcons["zoomExtent"],
                    self.parent.OnZoomToMap,
                ),
                (None,),
                (
                    ("mapDispSettings", BaseIcons["mapDispSettings"].label),
                    BaseIcons["mapDispSettings"],
                    self.parent.OnMapDisplayProperties,
                ),
                (None,),
                (
                    ("settings", icons["settings"].label),
                    icons["settings"],
                    self.parent.OnSettings,
                ),
                (
                    ("help", icons["help"].label),
                    icons["help"],
                    self.parent.OnHelp,
                ),
                (None,),
                (
                    ("quit", icons["quit"].label),
                    icons["quit"],
                    self.parent.OnQuit,
                ),
            )
        )
