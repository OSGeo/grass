"""!
@package gcp.statusbar

@brief Classes for statusbar management in GCP Manager

Classes:
 - statusbar::SbRMSError
 - statusbar::SbGoToGCP

(C) 2012-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Vaclav Petras <wenzeslaus gmail.com> (statusbar refactoring)
@author Anna Kratochvilova <kratochanna gmail.com> (statusbar refactoring)
"""


import wx

from core.gcmd import GMessage
from core.utils import _
from mapdisp.statusbar import SbItem, SbTextItem


class SbGoToGCP(SbItem):
    """!SpinCtrl to select GCP to focus on

    Requires MapFrame.GetSrcWindow, MapFrame.GetTgtWindow,
    MapFrame.GetListCtrl, MapFrame.GetMapCoordList.
    """
    def __init__(self, mapframe, statusbar, position=0):
        SbItem.__init__(self, mapframe, statusbar, position)
        self.name = 'gotoGCP'
        self.label = _("Go to GCP No.")

        self.widget = wx.SpinCtrl(parent=self.statusbar, id=wx.ID_ANY,
                                  value="", min=0)
        self.widget.Hide()

        self.widget.Bind(wx.EVT_TEXT_ENTER, self.OnGoToGCP)
        self.widget.Bind(wx.EVT_SPINCTRL, self.OnGoToGCP)

    def OnGoToGCP(self, event):
        """!Zooms to given GCP."""
        gcpNumber = self.GetValue()
        mapCoords = self.mapFrame.GetMapCoordList()

        # always false, spin checks it
        if gcpNumber < 0 or gcpNumber > len(mapCoords):
            GMessage(parent=self,
                     message="%s 1 - %s." % (_("Valid Range:"),
                                             len(mapCoords)))
            return

        if gcpNumber == 0:
            return

        listCtrl = self.mapFrame.GetListCtrl()

        listCtrl.selectedkey = gcpNumber
        listCtrl.selected = listCtrl.FindItemData(-1, gcpNumber)
        listCtrl.render = False
        listCtrl.SetItemState(listCtrl.selected,
                              wx.LIST_STATE_SELECTED,
                              wx.LIST_STATE_SELECTED)
        listCtrl.render = True

        listCtrl.EnsureVisible(listCtrl.selected)

        srcWin = self.mapFrame.GetSrcWindow()
        tgtWin = self.mapFrame.GetTgtWindow()

        # Source MapWindow:
        begin = (mapCoords[gcpNumber][1], mapCoords[gcpNumber][2])
        begin = srcWin.Cell2Pixel(begin)
        end = begin
        srcWin.Zoom(begin, end, 0)

        # redraw map
        srcWin.UpdateMap()

        if self.mapFrame.GetShowTarget():
            # Target MapWindow:
            begin = (mapCoords[gcpNumber][3], mapCoords[gcpNumber][4])
            begin = tgtWin.Cell2Pixel(begin)
            end = begin
            tgtWin.Zoom(begin, end, 0)

            # redraw map
            tgtWin.UpdateMap()

        self.GetWidget().SetFocus()

    def Update(self):
        """Checks the number of items in the gcp list
        and sets the spin limits accordingly."""
        self.statusbar.SetStatusText("")
        maximum = self.mapFrame.GetListCtrl().GetItemCount()
        if maximum < 1:
            maximum = 1
        self.widget.SetRange(0, maximum)
        self.Show()

        # disable long help
        self.mapFrame.StatusbarEnableLongHelp(False)


class SbRMSError(SbTextItem):
    """!Shows RMS error.

    Requires MapFrame.GetFwdError, MapFrame.GetBkwError.
    """
    def __init__(self, mapframe, statusbar, position=0):
        SbTextItem.__init__(self, mapframe, statusbar, position)
        self.name = 'RMSError'
        self.label = _("RMS error")

    def Show(self):
        """Shows the RMS errors."""
        self.SetValue(_("Forward: %(forw)s, Backward: %(back)s") %
                      {'forw': self.mapFrame.GetFwdError(),
                       'back': self.mapFrame.GetBkwError()})
        SbTextItem.Show(self)
