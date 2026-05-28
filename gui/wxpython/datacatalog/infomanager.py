"""
@package datacatalog.infomanager

@brief Class for managing info messages
in Data Catalog

Classes:
- infomanager::DataCatalogInfoManager

(C) 2020 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Kladivova
@author Anna Petrasova <kratochanna gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
"""

import wx

from grass.script import gisenv
from grass.grassdb.checks import get_mapset_owner


class DataCatalogInfoManager:
    """Manager for all things related to info bar in Data Catalog"""

    def __init__(self, infobar, giface):
        self.infoBar = infobar
        self._giface = giface

    def ShowDataStructureInfo(self, onCreateLocationHandler):
        """Show info about the data hierarchy focused on the first-time user"""
        buttons = [
            (_("Create new project"), onCreateLocationHandler),
            (_("Learn more"), self._onLearnMore),
        ]
        message = _(
            "GRASS helps you organize your data using projects (locations) "
            "which contain mapsets (subprojects). All data in one project is "
            "in the same coordinate reference system (CRS).\n\n"
            "You are currently in mapset PERMANENT in default project {loc} "
            "which uses WGS 84 (EPSG:4326). "
            "Consider creating a new project with a CRS "
            "specific to your area. You can do it now or anytime later from "
            "the toolbar above."
        ).format(loc=gisenv()["LOCATION_NAME"])
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION, buttons)

    def ShowImportDataInfo(self, OnImportOgrLayersHandler, OnImportGdalLayersHandler):
        """Show info about the data import focused on the first-time user"""
        buttons = [
            (_("Import vector data"), OnImportOgrLayersHandler),
            (_("Import raster data"), OnImportGdalLayersHandler),
        ]
        message = _(
            "You have successfully created a new project {loc}. "
            "Currently you are in its PERMANENT mapset which is used for "
            "storing your base maps to make them readily available in other "
            "mapsets. You can create new mapsets for different tasks by right "
            "clicking on the project name.\n\n"
            "To import data, go to the toolbar above or use the buttons below."
        ).format(loc=gisenv()["LOCATION_NAME"])
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION, buttons)

    def ShowLazyLoadingOn(self, setLazyLoadingOnHandler, doNotAskHandler):
        """Show info about lazy loading"""
        message = _(
            "Loading of Data catalog content took rather long. "
            "To prevent delay, you can enable loading of current mapset only. "
            "You can change that later in GUI Settings, General tab."
        )
        buttons = [
            (_("Enable loading current mapset only"), setLazyLoadingOnHandler),
            (_("No change, don't ask me again"), doNotAskHandler),
        ]
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION, buttons)

    def ShowFallbackSessionInfo(self, reason_id):
        """Show info when last used mapset is not usable"""
        string = self._text_from_reason_id(reason_id)
        message = _(
            "{string} GRASS has started in a temporary project. "
            "To continue, use Data Catalog below to switch to a different project."
        ).format(
            string=string,
        )
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION)

    def ShowLockedMapsetInfo(self, OnSwitchMapsetHandler):
        """Show info when last used mapset is locked"""
        last_used_mapset_path = gisenv()["LAST_MAPSET_PATH"]
        buttons = [(_("Switch to last used mapset"), OnSwitchMapsetHandler)]
        message = _(
            "Last used mapset in path '{mapsetpath}' is currently in use. "
            "GRASS has started in a temporary project. "
            "To continue, use Data Catalog below to switch to a different project "
            "or remove lock file and switch to the last used mapset."
        ).format(mapsetpath=last_used_mapset_path)
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION, buttons)

    def _text_from_reason_id(self, reason_id):
        """Get string for infobar message based on the reason."""
        last_used_mapset_path = gisenv()["LAST_MAPSET_PATH"]
        reason = None
        if reason_id == "non-existent":
            reason = _(
                "Last used mapset in path '{mapsetpath}' does not exist."
            ).format(mapsetpath=last_used_mapset_path)
        elif reason_id == "invalid":
            reason = _("Last used mapset in path '{mapsetpath}' is invalid.").format(
                mapsetpath=last_used_mapset_path
            )
        elif reason_id == "different-owner":
            owner = get_mapset_owner(last_used_mapset_path)
            reason = _(
                "Last used mapset in path '{mapsetpath}' has different owner {owner}."
            ).format(owner=owner, mapsetpath=last_used_mapset_path)
        return reason

    def _onLearnMore(self, event):
        self._giface.Help(entry="grass_projects")
