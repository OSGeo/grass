
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
        infoBar.ShowMessage(_(
            "GRASS is using the following structure of data organization:\n"
            "The Database {db} has the character of a working directory.\n"
            "The Location (Project) defines in which coordinate system you will work.\n"
            "The Mapset (Subproject) contains GIS data related to one project task."
        ).format(db=gisenv()['GISDBASE']), wx.ICON_INFORMATION)

    def ShowInfoBar2(self, buttons):
        infoBar = InfoBar(self.guiparent)
        self.sizer.Add(infoBar, wx.SizerFlags().Expand())
        infoBar.SetButtons(buttons)
        infoBar.ShowMessage(_(
            "In GRASS you can be sure that your data will not be displayed in a "
            "different coordinate system than the one you defined. "
            "Now, you are in a world wide, latitude-longitude system in degrees. "
            "If you are planning to work with data in a different system, "
            "please, create new Location. You can do it through right click "
            "on the Database node in Data tab or simply click here:"
        ), wx.ICON_INFORMATION)

    def _onLearnMore(self, event):
        webbrowser.open("https://grass.osgeo.org/grass79/manuals/grass_database.html")
        event.Skip()
