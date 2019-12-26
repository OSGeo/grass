"""
@package rdigit.dialogs

@brief rdigit dialog for craeting new map.

Classes:
 - rdigit:NewRasterDialog

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import wx

from gui_core.gselect import Select
from gui_core.wrap import Button, StaticText
from core.gcmd import GWarning

import grass.script.core as gcore
import grass.script.raster as grast
from grass.exceptions import CalledModuleError


class NewRasterDialog(wx.Dialog):
    """Dialog for new raster map name and type selection
    and selection of optional background map."""

    def __init__(self, parent):
        wx.Dialog.__init__(self, parent)
        self.SetTitle(_("Create new raster map"))
        self._name = None
        self._type = None

        # create widgets
        self._mapSelect = Select(parent=self, type='raster')
        self._backgroundSelect = Select(parent=self, type='raster')
        self._typeChoice = wx.Choice(self, choices=['CELL', 'FCELL', 'DCELL'])
        self._typeChoice.SetSelection(0)
        self._mapSelect.SetFocus()

        btnCancel = Button(parent=self, id=wx.ID_CANCEL)
        btnOK = Button(parent=self, id=wx.ID_OK)
        btnOK.SetDefault()
        btnOK.Bind(wx.EVT_BUTTON, self.OnOK)

        # do layout
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        sizer = wx.GridBagSizer(hgap=10, vgap=10)
        sizer.Add(StaticText(self, label=_("Name for new raster map:")),
                  pos=(0, 0), span=(1, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self._mapSelect, pos=(1, 0), span=(1, 2))
        sizer.Add(
            StaticText(
                self, label=_("Optionally select background raster map:")), pos=(
                2, 0), span=(
                1, 2), flag=wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self._backgroundSelect, pos=(3, 0), span=(1, 2))
        sizer.Add(StaticText(self, label=_("New raster map type:")),
                  pos=(4, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
        sizer.Add(self._typeChoice, pos=(4, 1), flag=wx.EXPAND)

        mainSizer.Add(sizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=10)

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        btnSizer.AddButton(btnOK)
        btnSizer.Realize()

        mainSizer.Add(btnSizer, flag=wx.EXPAND | wx.ALL, border=10)

        self._backgroundSelect.Bind(wx.EVT_TEXT, self.OnBackgroundMap)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

    def OnBackgroundMap(self, event):
        value = self._backgroundSelect.GetValue()
        try:
            ret = grast.raster_info(value)
            self._typeChoice.SetStringSelection(ret['datatype'])
        except CalledModuleError:
            return

    def OnOK(self, event):
        mapName = self.GetMapName()
        if not mapName:
            GWarning(parent=self.GetParent(), message=_(
                "Please specify name for a new raster map"))
        else:
            found = gcore.find_file(
                name=mapName, mapset=gcore.gisenv()['MAPSET'])
            if found and found['mapset'] == gcore.gisenv()['MAPSET']:
                dlgOverwrite = wx.MessageDialog(
                    self.GetParent(),
                    message=_(
                        "Raster map <%s> already exists "
                        "in the current mapset. "
                        "Do you want to overwrite it?") %
                    mapName,
                    caption=_("Overwrite?"),
                    style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
                if not dlgOverwrite.ShowModal() == wx.ID_YES:
                    dlgOverwrite.Destroy()
                    return
                else:
                    dlgOverwrite.Destroy()
                    self.EndModal(wx.ID_OK)
            else:
                self.EndModal(wx.ID_OK)

    def GetMapName(self):
        return self._mapSelect.GetValue()

    def GetBackgroundMapName(self):
        return self._backgroundSelect.GetValue()

    def GetMapType(self):
        return self._typeChoice.GetStringSelection()


if __name__ == '__main__':
    app = wx.App()
    dlg = NewRasterDialog(None)
    dlg.Show()
    app.MainLoop()
