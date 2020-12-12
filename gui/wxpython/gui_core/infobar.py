
import wx
import wx.aui
try:
    import wx.lib.agw.infobar as IB
except ImportError:
    import wx.lib.infobar as IB


class InfoBar(IB.InfoBar):
    """
    Custom Info bar
    """
    def __init__(self, parent):
        IB.InfoBar.__init__(self, parent)

        colBg = wx.SystemSettings.GetColour(wx.SYS_COLOUR_HIGHLIGHT)
        self.SetBackgroundColour(colBg)
        self.SetOwnForegroundColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_HIGHLIGHTTEXT))
        self._text.SetBackgroundColour(colBg)
        self._button.SetBackgroundColour(colBg)

        # LAYOUT
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.subSizerText = wx.BoxSizer(wx.HORIZONTAL)
        self.subSizerButtons = wx.BoxSizer(wx.HORIZONTAL)
        self.subSizerText.Add(self._icon, wx.SizerFlags().Centre().Border())
        self.subSizerText.Add(self._text, 200, wx.ALIGN_CENTER_VERTICAL)
        self.subSizerText.AddStretchSpacer()
        self.subSizerText.Add(self._button, wx.SizerFlags().Centre().Border())
        self.subSizerButtons.AddStretchSpacer()
        sizer.Add(self.subSizerText, wx.SizerFlags().Expand())
        sizer.Add(self.subSizerButtons, wx.SizerFlags().Expand())
        self.SetSizer(sizer)

    def AddButton(self, btnid, label, bitmap=wx.NullBitmap):
        """
        Adds a button to be shown in the info bar.
        """
        sizer = self.GetSizer()

        if not sizer:
            raise Exception("must be created first")

        # user-added buttons replace the standard close button so remove it if we
        # hadn't done it yet
        if sizer.Detach(self._button):
            self._button.Hide()

        button = wx.Button(self, btnid, label)

        if bitmap.IsOk():
            # Add the bitmap to the button
            button.SetBitmap(bitmap, wx.LEFT)
            button.SetBitmapMargins((2, 2))  # default is 4 but that seems too big to me.
            button.Show()

        if wx.Platform == '__WXMAC__':
            # smaller buttons look better in the(narrow)info bar under OS X
            button.SetWindowVariant(wx.WINDOW_VARIANT_SMALL)

        num_items = self.subSizerButtons.GetItemCount()
        if num_items == 1:
            self.subSizerButtons.Add(button, wx.SizerFlags().Centre().Border())
            self.subSizerButtons.Add(self._button, wx.SizerFlags().Centre().Border())
            self._button.Show()
        else:
            self.subSizerButtons.Insert(num_items - 1, button, wx.SizerFlags().Centre().Border())

        if self.IsShown():
            self.UpdateParent()

    def SetButtons(self, buttons):
        """
        Sets buttons for notification.
        Parameter *buttons* is a list of tuples (button_name, event)
        """
        for button_name, evt_handler, bitmap in buttons:
            button_id = wx.NewId()
            if bitmap:
                self.AddButton(button_id, button_name, bitmap)
            else:
                self.AddButton(button_id, button_name)
            self.Bind(wx.EVT_BUTTON, evt_handler, id=button_id)

    def OnButton(self, event):
        """
        Hides infobar.
        """
        self.DoHide()
