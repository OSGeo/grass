
import wx
import wx.aui
try:
    import wx.lib.agw.infobar as IB
except ImportError:
    import wx.lib.infobar as IB

from grass.script import gisenv


class InfoBar(IB.InfoBar):
    """
    Custom Info bar
    """
    def __init__(self, parent):
        IB.InfoBar.__init__(self, parent)

        self.SetBackgroundColour(wx.Colour(255, 248, 220))
        self._text.SetOwnForegroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT))

        self.buttons_ids = []

        # LAYOUT
        # icon, text, close button
        sizer = wx.BoxSizer(wx.HORIZONTAL)
        self.buttonBoxSizer = wx.BoxSizer(wx.VERTICAL)
        self.buttonBoxSizer.Add(self._button, wx.SizerFlags().Top().Border(wx.ALL,5))

        sizer.Add(self._icon, wx.SizerFlags().Top().Border(wx.ALL,5))
        sizer.Add(self._text, proportion=1,
                      flag=wx.EXPAND | wx.ALL, border=5)
        sizer.Add(self.buttonBoxSizer, wx.SizerFlags().Top().Border(wx.ALL,5))

        self.SetSizer(sizer)


    def AddButton(self, btnid, label, bitmap=wx.NullBitmap):
        """
        Adds a button to be shown in the info bar and
        does not remove Close button.
        """
        sizer = self.GetSizer()

        if not sizer:
            raise Exception("must be created first")

        button = wx.Button(self, btnid, label)

        # Add another button under close button
        self.buttonBoxSizer.Add(button, wx.SizerFlags().Top().Border(wx.ALL,5))


        if self.IsShown():
            self.UpdateParent()

    def CreateButton(self, button_dict):
        """
        Sets buttons for notification.
        Parameter *button_dict* is dictionary or button names and their events:
        {button_name, event}
        """
        if button_dict:
            for button_name in button_dict:
                button_id = wx.NewId()
                event = button_dict[button_name]
                self.AddButton(button_id, button_name)
                if event:
                    self.Bind(wx.EVT_BUTTON, event)
                self.buttons_ids.append(button_id)

    def OnButton(self, event):
        """
        Hides infobar and removes buttons.
        """
        self.DoHide()
        if self.buttons_ids:
            for i in self.buttons_ids:
                self.RemoveButton(i)


class InfoManager:
    """
    Infobar Manager
    """
    def __init__(self, guiparent, sizer):
        self.guiparent = guiparent
        self.sizer = sizer

    def ShowInfoBar1(self, button_dict):
        infoBar = InfoBar(self.guiparent)
        self.sizer.Add(infoBar, wx.SizerFlags().Expand())
        self._fitLayout()
        infoBar.CreateButton(button_dict)
        infoBar.ShowMessage(_(
            "GRASS GIS is a geodata analysis system which keeps its own data hierarchy. The database {db} has "
            "the character of a working directory. "
            "It contains Locations (Projects) that can have different coordinate systems (CRS). "
            "In Location we can find Mapsets that always "
            "have the same CRS as given Location. To store general spatial data, "
            "you can use PERMANENT Mapset which is created automatically when creating a new Location. "
            "To learn more about GRASS GIS data hierarchy, "
            "please look to the documentation."
        ).format(db=gisenv()['GISDBASE']), wx.ICON_INFORMATION)

    def ShowInfoBar2(self):
        infoBar = InfoBar(self.guiparent)
        self.sizer.Add(infoBar, wx.SizerFlags().Expand())
        self._fitLayout()
        infoBar.ShowMessage(_(
            "GRASS GIS has opened in a world wide, latitude-longitude system in degrees. "
            "To import your own data, first, define its coordinate "
            "system through creating a new location. Then you can create "
            "a mapset, switch to it (make it current) and import data."
        ), wx.ICON_INFORMATION)

    def ShowInfoBar3(self):
        infoBar = InfoBar(self.guiparent)
        self.sizer.Add(infoBar, wx.SizerFlags().Expand())
        self._fitLayout()
        infoBar.ShowMessage(_(
            "GRASS GIS has started in default world wide location"
            "because the last used mapset is used by another process."
        ), wx.ICON_WARNING)

    def ShowInfoBar4(self):
        infoBar = InfoBar(self.guiparent)
        self.sizer.Add(infoBar, wx.SizerFlags().Expand())
        self._fitLayout()
        infoBar.ShowMessage(_(
            "GRASS GIS has started in default world wide location"
            "because the last used mapset was not found."
        ), wx.ICON_WARNING)

    def ShowInfoBar5(self):
        infoBar = InfoBar(self.guiparent)
        self.sizer.Add(infoBar, wx.SizerFlags().Expand())
        self._fitLayout()
        infoBar.ShowMessage(_("You have created new location. Would you like to create"
            "a new mapset within this location?"
        ), wx.ICON_INFORMATION)

    def _fitLayout(self):
        self.guiparent.SetAutoLayout(True)
        self.guiparent.SetSizer(self.sizer)
        self.guiparent.Fit()
