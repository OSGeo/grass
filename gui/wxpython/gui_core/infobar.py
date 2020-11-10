
import wx
import wx.aui
try:
    import wx.lib.agw.infobar as IB
except ImportError:
    import wx.lib.infobar as IB

from grass.pydispatch.signal import Signal


class NotificationManager(IB.InfoBar):
    """
    Creates structure for notifications
    """
    def __init__(self, parent):
        IB.InfoBar.__init__(self, parent)

        self.displayInfoBar = Signal('NotificationManager.displayInfoBar')
        self.displayInfoBar.connect(
            lambda text, flag, button_dict: self.SetNotification(text,
                                                                 flag,
                                                                 button_dict))

        self.SetBackgroundColour(wx.Colour(255, 248, 220))
        self.SetOwnForegroundColour(wx.Colour(10, 10, 10))

        self.buttons_ids = []

    def SetNotification(self, text, flag, button_dict=None):
        """ Set text and buttons for notification.
        Parameter *text* is a message string.
        Parameter *flag* is define which type of InfoBar should be displayed
        Parameter *button* is dictionary or button names and their events:{button_name, event}
        """
        if button_dict:
            button_id = wx.NewId()
            for button_name in button_dict:
                self.AddButton(button_id, button_name)
                self.Bind(wx.EVT_BUTTON, button_dict[button_name])
                self.buttons_ids.append(button_id)
        self.ShowMessage(text, flag)

    def OnButton(self):
        self.DoHide()
        if self.buttons_ids:
            for i in self.buttons_ids:
                self.RemoveButton(i)
