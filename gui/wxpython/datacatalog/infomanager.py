
import wx
import webbrowser

from gui_core.infobar import InfoBar
from grass.script import gisenv


class InfoManagerDataCatalog:
    """
    Infobar Manager
    """
    def __init__(self, guiparent, sizer):
        self.guiparent = guiparent
        self.sizer = sizer

    def ShowInfoBar1(self, buttons):
        infoBar = InfoBar(self.guiparent)
        self.sizer.Add(infoBar, wx.SizerFlags().Expand())
        infoBar.SetButtons(buttons)
        genv = gisenv()
        infoBar.ShowMessage(_(
            "GRASS helps you organize your data using Locations (projects) "
            "which contain Mapsets (subprojects). All data in one Location is "
            "in the same coordinate reference system (CRS).\n\n"
            "You are currently in Mapset PERMANENT in Location {loc} which uses "
            "WGS 84 (EPSG:4326). Consider creating a new Location with a CRS "
            "specific to your area. You can do it now or anytime later from "
            "the toolbar above."
        ).format(loc=genv['LOCATION_NAME']
        ), wx.ICON_INFORMATION)

    def _onLearnMore(self, event):
        webbrowser.open("https://grass.osgeo.org/grass79/manuals/grass_database.html")
        event.Skip()
