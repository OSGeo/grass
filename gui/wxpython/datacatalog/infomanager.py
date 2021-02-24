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
from startup.guiutils import read_gisrc


class DataCatalogInfoManager:
    """Manager for all things related to info bar in Data Catalog"""

    def __init__(self, infobar, giface):
        self.infoBar = infobar
        self._giface = giface

    def ShowDataStructureInfo(self, onCreateLocationHandler):
        """Show info about the data hierarchy focused on the first-time user"""
        buttons = [("Create new Location", onCreateLocationHandler),
                   ("Learn More", self._onLearnMore)]
        message = _(
            "GRASS GIS helps you organize your data using Locations (projects) "
            "which contain Mapsets (subprojects). All data in one Location is "
            "in the same coordinate reference system (CRS).\n\n"
            "You are currently in Mapset PERMANENT in default Location {loc} "
            "which uses WGS 84 (EPSG:4326). Consider creating a new Location with a CRS "
            "specific to your area. You can do it now or anytime later from "
            "the toolbar above."
        ).format(loc=gisenv()['LOCATION_NAME'])
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION, buttons)

    def ShowImportDataInfo(self, OnImportOgrLayersHandler, OnImportGdalLayersHandler):
        """Show info about the data import focused on the first-time user"""
        buttons = [("Import vector data", OnImportOgrLayersHandler),
                   ("Import raster data", OnImportGdalLayersHandler)]
        message = _(
            "You have successfully created a new Location {loc}. "
            "Currently you are in its PERMANENT Mapset which is used for "
            "storing your base maps to make them readily available in other "
            "Mapsets. You can create new Mapsets for different tasks by right "
            "clicking on the Location name.\n\n"
            "To import data, go to the toolbar above or use the buttons below."
        ).format(loc=gisenv()['LOCATION_NAME'])
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION, buttons)

    def ShowLockedMapsetInfo(self, OnSwitchMapsetHandler):
        """Show info when last used mapset is locked"""
        grassrc = read_gisrc()
        buttons = [("Switch to last used mapset", OnSwitchMapsetHandler)]
        message = _(
            "Last used mapset in path '{lastdb}/{lastloc}/{lastmapset}' is locked."
            "GRASS GIS has started in the default Location {loc} which uses "
            "WGS 84 (EPSG:4326). To continue, set usable mapset through Data "
            "Catalog below, or remove .gislock and switch to last usable mapset."
        ).format(lastdb=grassrc['UNUSED_GISDBASE'],
                 lastloc=grassrc['UNUSED_LOCATION_NAME'],
                 lastmapset=grassrc['UNUSED_MAPSET'],
                 loc=gisenv()['LOCATION_NAME'])
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION, buttons)

    def ShowBasicDemolocationInfo(self):
        """Show info when last used mapset is invalid or owned by different user"""
        grassrc = read_gisrc()
        message = _(
            "Last used mapset in path '{lastdb}/{lastloc}/{lastmapset}' is {reason}."
            "GRASS GIS has started in the default Location {loc} which uses "
            "WGS 84 (EPSG:4326). To continue, find or create usable mapset through "
            "Data catalog below."
        ).format(lastdb=grassrc['UNUSED_GISDBASE'],
                 lastmapset=grassrc['UNUSED_MAPSET'],
                 lastloc=grassrc['UNUSED_LOCATION_NAME'],
                 reason=grassrc['REASON'],
                 loc=gisenv()['LOCATION_NAME'])
        self.infoBar.ShowMessage(message, wx.ICON_INFORMATION)

    def _onLearnMore(self, event):
        self._giface.Help(entry="grass_database")
