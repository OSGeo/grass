
import wx

from grass.script import gisenv


class DataCatalogInfoManager:
    """Manager for all things info bar in Data Catalog"""
    def __init__(self, parent, infobar, sizer, giface=None):
        self.infoBar = infobar
        self.sizer = sizer
        self._giface = giface

    def ShowDataStructureInfo(self, buttons):
        """Show info about the data hierarchy focused on the first-time user"""
        self.sizer.Add(self.infoBar, wx.SizerFlags().Expand())
        self.infoBar.SetButtons(buttons)
        self.infoBar.ShowMessage(_(
            "GRASS GIS helps you organize your data using Locations (projects) "
            "which contain Mapsets (subprojects). All data in one Location is "
            "in the same coordinate reference system (CRS).\n\n"
            "You are currently in Mapset PERMANENT in Location {loc} which uses "
            "WGS 84 (EPSG:4326). Consider creating a new Location with a CRS "
            "specific to your area. You can do it now or anytime later from "
            "the toolbar above."
        ).format(loc=gisenv()['LOCATION_NAME']), wx.ICON_INFORMATION)

    def _onLearnMore(self, event):
        self._giface.Help(entry="grass_database")
