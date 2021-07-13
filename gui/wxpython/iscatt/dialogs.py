"""
@package iscatt.dialogs

@brief Dialogs widgets.

Classes:
 - dialogs::AddScattPlotDialog
 - dialogs::ExportCategoryRaster
 - dialogs::SettingsDialog
 - dialogs::ManageBusyCursorMixin
 - dialogs::RenameClassDialog

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Stepan Turek <stepan.turek seznam.cz> (mentor: Martin Landa)
"""

import six

import wx
from gui_core.gselect import Select
import wx.lib.colourselect as csel

import grass.script as grass

from core import globalvar
from core.gcmd import GMessage
from core.settings import UserSettings
from gui_core.dialogs import SimpleDialog
from gui_core.wrap import SpinCtrl, Button, StaticText, StaticBox, TextCtrl


class AddScattPlotDialog(wx.Dialog):
    def __init__(self, parent, bands, check_bands_callback, id=wx.ID_ANY):
        wx.Dialog.__init__(self, parent, title=_("Add scatter plots"), id=id)

        self.bands = bands

        self.x_band = None
        self.y_band = None

        self.chb_callback = check_bands_callback

        self.added_bands_ids = []
        self.sel_bands_ids = []
        self._createWidgets()

    def _createWidgets(self):

        self.labels = {}
        self.params = {}

        self.band_1_label = StaticText(parent=self, id=wx.ID_ANY, label=_("x axis:"))

        self.band_1_ch = wx.ComboBox(
            parent=self,
            id=wx.ID_ANY,
            choices=self.bands,
            style=wx.CB_READONLY,
            size=(350, 30),
        )

        self.band_2_label = StaticText(parent=self, id=wx.ID_ANY, label=_("y axis:"))

        self.band_2_ch = wx.ComboBox(
            parent=self,
            id=wx.ID_ANY,
            choices=self.bands,
            style=wx.CB_READONLY,
            size=(350, 30),
        )

        self.scattsBox = wx.ListBox(
            parent=self,
            id=wx.ID_ANY,
            size=(-1, 150),
            style=wx.LB_MULTIPLE | wx.LB_NEEDED_SB,
        )

        # buttons
        self.btn_add = Button(parent=self, id=wx.ID_ADD)
        self.btn_remove = Button(parent=self, id=wx.ID_REMOVE)

        self.btn_close = Button(parent=self, id=wx.ID_CANCEL)
        self.btn_ok = Button(parent=self, id=wx.ID_OK)

        self._layout()

    def _layout(self):

        border = wx.BoxSizer(wx.VERTICAL)
        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        regionSizer = wx.BoxSizer(wx.HORIZONTAL)

        dialogSizer.Add(
            self._addSelectSizer(title=self.band_1_label, sel=self.band_1_ch)
        )

        dialogSizer.Add(
            self._addSelectSizer(title=self.band_2_label, sel=self.band_2_ch)
        )

        dialogSizer.Add(
            self.btn_add, proportion=0, flag=wx.TOP | wx.ALIGN_RIGHT, border=5
        )

        box = StaticBox(
            self,
            id=wx.ID_ANY,
            label=" %s " % _("Bands of scatter plots to be added (x y):"),
        )
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        sizer.Add(self.scattsBox, proportion=1, flag=wx.EXPAND | wx.TOP, border=5)
        sizer.Add(self.btn_remove, proportion=0, flag=wx.TOP | wx.ALIGN_RIGHT, border=5)

        dialogSizer.Add(sizer, proportion=1, flag=wx.EXPAND | wx.TOP, border=5)

        # buttons
        self.btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)

        self.btnsizer.Add(
            self.btn_close,
            proportion=0,
            flag=wx.RIGHT | wx.LEFT | wx.ALIGN_CENTER,
            border=10,
        )

        self.btnsizer.Add(
            self.btn_ok,
            proportion=0,
            flag=wx.RIGHT | wx.LEFT | wx.ALIGN_CENTER,
            border=10,
        )

        dialogSizer.Add(
            self.btnsizer, proportion=0, flag=wx.ALIGN_CENTER | wx.TOP, border=5
        )

        border.Add(
            dialogSizer, proportion=0, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM, border=10
        )

        self.SetSizer(border)
        self.Layout()
        self.Fit()

        # bindings
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btn_ok.Bind(wx.EVT_BUTTON, self.OnOk)
        self.btn_add.Bind(wx.EVT_BUTTON, self.OnAdd)
        self.btn_remove.Bind(wx.EVT_BUTTON, self.OnRemoveLayer)

    def OnOk(self, event):

        if not self.GetBands():
            GMessage(parent=self, message=_("No scatter plots selected."))
            return

        event.Skip()

    def _addSelectSizer(self, title, sel):
        """Helper layout function."""
        selSizer = wx.BoxSizer(orient=wx.VERTICAL)

        selTitleSizer = wx.BoxSizer(wx.HORIZONTAL)
        selTitleSizer.Add(title, proportion=1, flag=wx.TOP | wx.EXPAND, border=5)

        selSizer.Add(selTitleSizer, proportion=0, flag=wx.EXPAND)

        selSizer.Add(sel, proportion=1, flag=wx.EXPAND | wx.TOP, border=5)

        return selSizer

    def GetBands(self):
        """Get layers"""
        return self.sel_bands_ids

    def OnClose(self, event):
        """Close dialog"""
        if not self.IsModal():
            self.Destroy()
        event.Skip()

    def OnRemoveLayer(self, event):
        """Remove layer from listbox"""
        while self.scattsBox.GetSelections():
            sel = self.scattsBox.GetSelections()[0]
            self.scattsBox.Delete(sel)
            self.sel_bands_ids.pop(sel)

    def OnAdd(self, event):
        b_x = self.band_1_ch.GetSelection()
        b_y = self.band_2_ch.GetSelection()

        if b_x < 0 or b_y < 0:
            GMessage(parent=self, message=_("Select both x and y bands."))
            return
        if b_y == b_x:
            GMessage(parent=self, message=_("Selected bands must be different."))
            return

        if [b_x, b_y] in self.sel_bands_ids or [b_y, b_x] in self.sel_bands_ids:
            GMessage(
                parent=self,
                message=_(
                    "Scatter plot with same bands combination (regardless x y order) "
                    "has been already added into the list."
                ),
            )
            return

        if not self.chb_callback(b_x, b_y):
            return

        self.sel_bands_ids.append([b_x, b_y])

        b_x_str = self.band_1_ch.GetStringSelection()
        b_y_str = self.band_2_ch.GetStringSelection()

        text = b_x_str + " " + b_y_str

        self.scattsBox.Append(text)
        event.Skip()


class ExportCategoryRaster(wx.Dialog):
    def __init__(
        self,
        parent,
        title,
        rasterName=None,
        id=wx.ID_ANY,
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        """Dialog for export of category raster.

        :param parent: window
        :param str rasterName name of vector layer for export
        :param title: window title
        """
        wx.Dialog.__init__(self, parent, id, title, style=style, **kwargs)

        self.rasterName = rasterName
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.btnCancel = Button(parent=self.panel, id=wx.ID_CANCEL)
        self.btnOK = Button(parent=self.panel, id=wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)
        self.btnOK.Bind(wx.EVT_BUTTON, self.OnOK)

        self.__layout()

        self.vectorNameCtrl.Bind(wx.EVT_TEXT, self.OnTextChanged)
        self.OnTextChanged(None)
        wx.CallAfter(self.vectorNameCtrl.SetFocus)

    def OnTextChanged(self, event):
        """Name of new vector map given.

        Enable/diable OK button.
        """
        file = self.vectorNameCtrl.GetValue()
        if len(file) > 0:
            self.btnOK.Enable(True)
        else:
            self.btnOK.Enable(False)

    def __layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        dataSizer = wx.BoxSizer(wx.VERTICAL)

        dataSizer.Add(
            StaticText(
                parent=self.panel,
                id=wx.ID_ANY,
                label=_("Enter name of new vector map:"),
            ),
            proportion=0,
            flag=wx.ALL,
            border=3,
        )
        self.vectorNameCtrl = Select(
            parent=self.panel,
            type="raster",
            mapsets=[grass.gisenv()["MAPSET"]],
            size=globalvar.DIALOG_GSELECT_SIZE,
        )
        if self.rasterName:
            self.vectorNameCtrl.SetValue(self.rasterName)
        dataSizer.Add(
            self.vectorNameCtrl, proportion=0, flag=wx.ALL | wx.EXPAND, border=3
        )

        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()

        sizer.Add(
            dataSizer, proportion=1, flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5
        )

        sizer.Add(
            btnSizer, proportion=0, flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5
        )

        self.panel.SetSizer(sizer)
        sizer.Fit(self)

        self.SetMinSize(self.GetSize())

    def GetRasterName(self):
        """Returns vector name"""
        return self.vectorNameCtrl.GetValue()

    def OnOK(self, event):
        """Checks if map exists and can be overwritten."""
        overwrite = UserSettings.Get(group="cmd", key="overwrite", subkey="enabled")
        rast_name = self.GetRasterName()
        res = grass.find_file(rast_name, element="cell")
        if res["fullname"] and overwrite is False:
            qdlg = wx.MessageDialog(
                parent=self,
                message=_(
                    "Raster map <%s> already exists."
                    " Do you want to overwrite it?" % rast_name
                ),
                caption=_("Raster <%s> exists" % rast_name),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE,
            )
            if qdlg.ShowModal() == wx.ID_YES:
                event.Skip()
            qdlg.Destroy()
        else:
            event.Skip()


class SettingsDialog(wx.Dialog):
    def __init__(
        self,
        parent,
        id,
        title,
        scatt_mgr,
        pos=wx.DefaultPosition,
        size=wx.DefaultSize,
        style=wx.DEFAULT_DIALOG_STYLE,
    ):
        """Settings dialog"""
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)

        self.scatt_mgr = scatt_mgr

        maxValue = 1e8
        self.parent = parent
        self.settings = {}

        settsLabels = {}

        self.settings["show_ellips"] = wx.CheckBox(
            parent=self, id=wx.ID_ANY, label=_("Show confidence ellipses")
        )
        show_ellips = UserSettings.Get(
            group="scatt", key="ellipses", subkey="show_ellips"
        )
        self.settings["show_ellips"].SetValue(show_ellips)

        self.colorsSetts = {
            "sel_pol": ["selection", _("Selection polygon color:")],
            "sel_pol_vertex": ["selection", _("Color of selection polygon vertex:")],
            "sel_area": ["selection", _("Selected area color:")],
        }

        for settKey, sett in six.iteritems(self.colorsSetts):
            settsLabels[settKey] = StaticText(parent=self, id=wx.ID_ANY, label=sett[1])
            col = UserSettings.Get(group="scatt", key=sett[0], subkey=settKey)
            self.settings[settKey] = csel.ColourSelect(
                parent=self, id=wx.ID_ANY, colour=wx.Colour(col[0], col[1], col[2], 255)
            )

        self.sizeSetts = {
            "snap_tresh": ["selection", _("Snapping threshold in pixels:")],
            "sel_area_opacty": ["selection", _("Selected area opacity:")],
        }

        for settKey, sett in six.iteritems(self.sizeSetts):
            settsLabels[settKey] = StaticText(parent=self, id=wx.ID_ANY, label=sett[1])
            self.settings[settKey] = SpinCtrl(parent=self, id=wx.ID_ANY, min=0, max=100)
            size = int(UserSettings.Get(group="scatt", key=sett[0], subkey=settKey))
            self.settings[settKey].SetValue(size)

        # buttons
        self.btnSave = Button(self, wx.ID_SAVE)
        self.btnApply = Button(self, wx.ID_APPLY)
        self.btnClose = Button(self, wx.ID_CLOSE)
        self.btnApply.SetDefault()

        # bindings
        self.btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        self.btnApply.SetToolTip(_("Apply changes for the current session"))
        self.btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        self.btnSave.SetToolTip(
            _(
                "Apply and save changes to user settings file (default for next sessions)"
            )
        )
        self.btnClose.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btnClose.SetToolTip(_("Close dialog"))

        # Layout

        # Analysis result style layout
        self.SetMinSize(self.GetBestSize())

        sizer = wx.BoxSizer(wx.VERTICAL)

        sel_pol_box = StaticBox(
            parent=self, id=wx.ID_ANY, label=" %s " % _("Selection style:")
        )
        selPolBoxSizer = wx.StaticBoxSizer(sel_pol_box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer(vgap=1, hgap=1)

        row = 0
        setts = dict()
        setts.update(self.colorsSetts)
        setts.update(self.sizeSetts)

        settsOrder = [
            "sel_pol",
            "sel_pol_vertex",
            "sel_area",
            "sel_area_opacty",
            "snap_tresh",
        ]
        for settKey in settsOrder:
            sett = setts[settKey]
            gridSizer.Add(
                settsLabels[settKey], flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0)
            )
            gridSizer.Add(
                self.settings[settKey],
                flag=wx.ALIGN_RIGHT | wx.ALL,
                border=5,
                pos=(row, 1),
            )
            row += 1

        gridSizer.AddGrowableCol(1)
        selPolBoxSizer.Add(gridSizer, flag=wx.EXPAND)

        ell_box = StaticBox(
            parent=self, id=wx.ID_ANY, label=" %s " % _("Ellipses settings:")
        )
        ellPolBoxSizer = wx.StaticBoxSizer(ell_box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=1, hgap=1)

        sett = setts[settKey]

        row = 0
        gridSizer.Add(
            self.settings["show_ellips"], flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0)
        )

        gridSizer.AddGrowableCol(0)
        ellPolBoxSizer.Add(gridSizer, flag=wx.EXPAND)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnApply, flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btnSave, flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(self.btnClose, flag=wx.LEFT | wx.RIGHT, border=5)

        sizer.Add(selPolBoxSizer, flag=wx.EXPAND | wx.ALL, border=5)
        sizer.Add(ellPolBoxSizer, flag=wx.EXPAND | wx.ALL, border=5)
        sizer.Add(btnSizer, flag=wx.EXPAND | wx.ALL, border=5, proportion=0)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnSave(self, event):
        """Button 'Save' pressed"""
        self.UpdateSettings()

        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings["scatt"] = UserSettings.Get(group="scatt")
        UserSettings.SaveToFile(fileSettings)

        self.Close()

    def UpdateSettings(self):

        chanaged_setts = []
        for settKey, sett in six.iteritems(self.colorsSetts):
            col = tuple(self.settings[settKey].GetColour())
            col_s = UserSettings.Get(group="scatt", key=sett[0], subkey=settKey)
            if col_s != col:
                UserSettings.Set(group="scatt", key=sett[0], subkey=settKey, value=col)
                chanaged_setts.append([settKey, sett[0]])

        for settKey, sett in six.iteritems(self.sizeSetts):
            val = self.settings[settKey].GetValue()
            val_s = UserSettings.Get(group="scatt", key=sett[0], subkey=settKey)

            if val_s != val:
                UserSettings.Set(group="scatt", key=sett[0], subkey=settKey, value=val)
                chanaged_setts.append([settKey, sett[0]])

        val = self.settings["show_ellips"].IsChecked()
        val_s = UserSettings.Get(group="scatt", key="ellipses", subkey="show_ellips")

        if val != val_s:
            UserSettings.Set(
                group="scatt", key="ellipses", subkey="show_ellips", value=val
            )
            chanaged_setts.append(["ellipses", "show_ellips"])

        if chanaged_setts:
            self.scatt_mgr.SettingsUpdated(chanaged_setts)

    def OnApply(self, event):
        """Button 'Apply' pressed"""
        self.UpdateSettings()
        # self.Close()

    def OnClose(self, event):
        """Button 'Cancel' pressed"""
        self.Close()


class ManageBusyCursorMixin:
    def __init__(self, window):
        self.win = window

        self.is_busy = False
        self.cur_inside = False

        self.win.Bind(wx.EVT_ENTER_WINDOW, self.OnEnter)
        self.win.Bind(wx.EVT_LEAVE_WINDOW, self.OnLeave)

    def OnLeave(self, event):
        self.cur_inside = False
        self.busy_cur = None

    def OnEnter(self, event):
        self.cur_inside = True
        if self.is_busy:
            self.busy_cur = wx.BusyCursor()

    def UpdateCur(self, busy):
        self.is_busy = busy
        if self.cur_inside and self.is_busy:
            self.busy_cur = wx.BusyCursor()
            return

        self.busy_cur = None


class RenameClassDialog(SimpleDialog):
    def __init__(self, parent, old_name, title=("Change class name")):
        SimpleDialog.__init__(self, parent, title)

        self.name = TextCtrl(self.panel, id=wx.ID_ANY)
        self.name.SetValue(old_name)

        self.dataSizer.Add(self.name, proportion=0, flag=wx.EXPAND | wx.ALL, border=1)

        self.panel.SetSizer(self.sizer)
        self.name.SetMinSize((200, -1))
        self.sizer.Fit(self)

    def GetNewName(self):
        return self.name.GetValue()
