"""
@package gui_core.infobar

@brief Wrapper around wx.InfoBar

Classes:
- gui_core::InfoBar

(C) 2020 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Linda Kladivova
@author Anna Petrasova <kratochanna gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
"""

import wx
import wx.aui
try:
    import wx.lib.agw.infobar as IB
except ImportError:
    import wx.lib.infobar as IB


def GetCloseButtonBitmap(win, size, colBg, flags=0):
    """Workaround for missing DrawTitleBarBitmap method
    of wx.RendererNative in certain wx versions.
    See https://github.com/wxWidgets/Phoenix/issues/1425."""
    renderer = wx.RendererNative.Get()
    if hasattr(renderer, 'DrawTitleBarBitmap'):
        bmp = wx.Bitmap(*size)
        dc = wx.MemoryDC()
        dc.SelectObject(bmp)
        dc.SetBackground(wx.Brush(colBg))
        dc.Clear()

        wx.RendererNative.Get().DrawTitleBarBitmap(win, dc, wx.Rect(size), wx.TITLEBAR_BUTTON_CLOSE, flags)
        dc.SelectObject(wx.NullBitmap)
    else:
        bmp = wx.ArtProvider.GetBitmap(wx.ART_CLOSE, wx.ART_BUTTON)

    return bmp


IB.GetCloseButtonBitmap = GetCloseButtonBitmap


class InfoBar(IB.InfoBar):
    """A customized and specialized info bar to used by default"""
    def __init__(self, parent):
        IB.InfoBar.__init__(self, parent)

        self.button_ids = []

        # some system themes have alpha, remove it
        self._background_color = wx.SystemSettings.GetColour(
            wx.SYS_COLOUR_HIGHLIGHT
        ).Get(False)
        self._foreground_color = wx.SystemSettings.GetColour(
            wx.SYS_COLOUR_HIGHLIGHTTEXT
        ).Get(False)
        self.SetBackgroundColour(self._background_color)
        self.SetForegroundColour(self._foreground_color)
        self._text.SetBackgroundColour(self._background_color)
        self._text.SetForegroundColour(self._foreground_color)
        self._button.SetBackgroundColour(self._background_color)

        # layout
        self._unInitLayout()
        sizer = wx.BoxSizer(wx.VERTICAL)
        subSizerText = wx.BoxSizer(wx.HORIZONTAL)
        self.subSizerButtons = wx.BoxSizer(wx.HORIZONTAL)
        subSizerText.Add(self._icon, wx.SizerFlags().Centre().Border())
        subSizerText.Add(self._text, 1, wx.ALIGN_CENTER_VERTICAL)
        self.subSizerButtons.AddStretchSpacer()
        self.subSizerButtons.Add(self._button, wx.SizerFlags().Centre().Border())
        sizer.Add(subSizerText, wx.SizerFlags().Expand())
        sizer.Add(self.subSizerButtons, wx.SizerFlags().Expand())
        self.SetSizer(sizer)

    def _unInitLayout(self):
        sizer = self.GetSizer()
        children = sizer.GetChildren()
        for i in reversed(range(len(children))):
            if children[i].IsSpacer():
                sizer.Remove(i)
            else:
                sizer.Detach(i)

    def ShowMessage(self, message, icon, buttons=None):
        """Show message with buttons (optional).
        Buttons are list of tuples (label, handler)"""
        self.Hide()
        self.RemoveButtons()
        if buttons:
            self.SetButtons(buttons)
        super().ShowMessage(message, icon)

    def AddButton(self, btnid, label):
        """
        Adds a button to be shown in the info bar.
        """
        sizer = self.GetSizer()

        assert sizer is not None, "Sizer must be created first"

        # user-added buttons replace the standard close button so remove it if we
        # hadn't done it yet
        if sizer.Detach(self._button):
            self._button.Hide()

        button = wx.Button(self, btnid, label)
        button.SetBackgroundColour(self._background_color)
        button.SetForegroundColour(self._foreground_color)

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
        for button_name, evt_handler in buttons:
            button_id = wx.NewId()
            self.button_ids.append(button_id)
            self.AddButton(button_id, button_name)
            self.Bind(wx.EVT_BUTTON, evt_handler, id=button_id)

    def RemoveButtons(self):
        """
        Removes buttons from info bar.
        """
        items = self.subSizerButtons.GetChildren()
        for item in reversed(items):
            if not item.IsSpacer():
                window = item.GetWindow()
                if window.GetId() in self.button_ids:
                    self.subSizerButtons.Detach(window)
                    window.Destroy()
