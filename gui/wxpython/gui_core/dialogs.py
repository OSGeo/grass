"""
@package gui_core.dialogs

@brief Various dialogs used in wxGUI.

List of classes:
 - :class:`SimpleDialog`
 - :class:`LocationDialog`
 - :class:`MapsetDialog`
 - :class:`VectorDialog`
 - :class:`NewVectorDialog`
 - :class:`SavedRegion`
 - :class:`GroupDialog`
 - :class:`MapLayersDialog`
 - :class:`SetOpacityDialog`
 - :class:`ImageSizeDialog`
 - :class:`SqlQueryFrame`
 - :class:`SymbolDialog`
 - :class:`QuitDialog`
 - :class:`DefaultFontDialog`

(C) 2008-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com> (GroupDialog, SymbolDialog)
"""

import os
import re
from pathlib import Path

import wx

from grass.script import core as grass
from grass.script.utils import naturally_sorted, try_remove

from grass.pydispatch.signal import Signal
from core import globalvar
from core.gcmd import GError, RunCommand, GMessage
from gui_core.gselect import (
    LocationSelect,
    MapsetSelect,
    Select,
    OgrTypeSelect,
    SubGroupSelect,
)
from gui_core.widgets import SingleSymbolPanel, SimpleValidator, MapValidator
from core.settings import UserSettings
from core.debug import Debug
from core.utils import is_shell_running
from gui_core.wrap import (
    Button,
    CheckListBox,
    EmptyBitmap,
    HyperlinkCtrl,
    Menu,
    NewId,
    Slider,
    SpinCtrl,
    StaticBox,
    StaticText,
    TextCtrl,
    ListBox,
)


class SimpleDialog(wx.Dialog):
    def __init__(
        self,
        parent,
        title,
        id=wx.ID_ANY,
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        """General dialog to choose given element (location, mapset, vector map, etc.)

        :param parent: window
        :param title: window title
        """
        wx.Dialog.__init__(self, parent, id, title, style=style, **kwargs)
        self.SetExtraStyle(wx.WS_EX_VALIDATE_RECURSIVELY)
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.btnCancel = Button(parent=self.panel, id=wx.ID_CANCEL)
        self.btnOK = Button(parent=self.panel, id=wx.ID_OK)
        self.btnOK.SetDefault()

        self.__layout()
        self.warning = _("Required item is not set.")

    def __layout(self):
        """Do layout"""
        self.sizer = wx.BoxSizer(wx.VERTICAL)

        self.dataSizer = wx.BoxSizer(wx.VERTICAL)

        # self.informLabel = wx.StaticText(self.panel, id = wx.ID_ANY)
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()

        self.sizer.Add(self.dataSizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)

        # self.sizer.Add(item = self.informLabel, proportion = 0,
        # flag = wx.ALL, border = 5)
        self.sizer.Add(btnSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

    def ValidatorCallback(self, win):
        GMessage(parent=self, message=self.warning)
        # self.informLabel.SetForegroundColour(wx.Colour(255, 0, 0))
        # self.informLabel.SetLabel(self.warning)


class LocationDialog(SimpleDialog):
    """Dialog used to select location"""

    def __init__(self, parent, title=_("Select GRASS project and mapset")):
        SimpleDialog.__init__(self, parent, title)

        self.element1 = LocationSelect(
            parent=self.panel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            validator=SimpleValidator(callback=self.ValidatorCallback),
        )
        self.element1.Bind(wx.EVT_TEXT, self.OnLocation)
        self.element1.Bind(wx.EVT_COMBOBOX, self.OnLocation)
        self.element2 = MapsetSelect(
            parent=self.panel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            setItems=False,
            skipCurrent=True,
            validator=SimpleValidator(callback=self.ValidatorCallback),
        )
        self.element1.SetFocus()
        self.warning = _("Project or mapset is not defined.")
        self._layout()
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """Do layout"""
        self.dataSizer.Add(
            StaticText(
                parent=self.panel, id=wx.ID_ANY, label=_("Name of GRASS project:")
            ),
            proportion=0,
            flag=wx.ALL,
            border=1,
        )
        self.dataSizer.Add(
            self.element1, proportion=0, flag=wx.EXPAND | wx.ALL, border=1
        )

        self.dataSizer.Add(
            StaticText(parent=self.panel, id=wx.ID_ANY, label=_("Name of mapset:")),
            proportion=0,
            flag=wx.EXPAND | wx.ALL,
            border=1,
        )

        self.dataSizer.Add(
            self.element2, proportion=0, flag=wx.EXPAND | wx.ALL, border=1
        )

        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def OnLocation(self, event):
        """Select mapset given location name"""
        location = event.GetString()

        if location:
            dbase = grass.gisenv()["GISDBASE"]
            self.element2.UpdateItems(dbase=dbase, location=location)
            self.element2.SetSelection(0)

    def GetValues(self):
        """Get location, mapset"""
        return (self.element1.GetValue(), self.element2.GetValue())


class MapsetDialog(SimpleDialog):
    """Dialog used to select mapset"""

    def __init__(
        self, parent, title=_("Select mapset in GRASS project"), location=None
    ):
        SimpleDialog.__init__(self, parent, title)

        if location:
            self.SetTitle(self.GetTitle() + " <%s>" % location)
        else:
            self.SetTitle(self.GetTitle() + " <%s>" % grass.gisenv()["LOCATION_NAME"])

        self.element = MapsetSelect(
            parent=self.panel,
            id=wx.ID_ANY,
            skipCurrent=True,
            size=globalvar.DIALOG_GSELECT_SIZE,
            validator=SimpleValidator(callback=self.ValidatorCallback),
        )

        self.element.SetFocus()
        self.warning = _("Name of mapset is missing.")

        self._layout()
        self.SetMinSize(self.GetSize())

    def _layout(self):
        """Do layout"""
        self.dataSizer.Add(
            StaticText(parent=self.panel, id=wx.ID_ANY, label=_("Name of mapset:")),
            proportion=0,
            flag=wx.ALL,
            border=1,
        )
        self.dataSizer.Add(
            self.element, proportion=0, flag=wx.EXPAND | wx.ALL, border=1
        )
        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetMapset(self):
        return self.element.GetValue()


class VectorDialog(SimpleDialog):
    def __init__(self, parent, title=_("Select vector map"), layerTree=None):
        """Dialog for selecting existing vector map

        :param parent: parent window
        :param title: window title
        :param layerTree: show only vector maps in given layer tree if not None

        :return: dialog instance
        """
        SimpleDialog.__init__(self, parent, title)

        self.element = self._selection_widget(layerTree)
        self.element.SetFocus()

        self.warning = _("Name of vector map is missing.")
        wx.CallAfter(self._layout)

    def _selection_widget(self, layerTree):
        return Select(
            parent=self.panel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            type="vector",
            layerTree=layerTree,
            fullyQualified=True,
        )

    def _layout(self):
        """Do layout"""
        self.dataSizer.Add(
            StaticText(parent=self.panel, id=wx.ID_ANY, label=_("Name of vector map:")),
            proportion=0,
            flag=wx.ALL,
            border=1,
        )
        self.dataSizer.Add(
            self.element, proportion=0, flag=wx.EXPAND | wx.ALL, border=1
        )

        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)

    def GetName(self, full=False):
        """Get name of vector map to be created

        :param full: True to get fully qualified name
        """
        name = self.element.GetValue()
        if not full:
            return name.split("@", 1)[0]
        if "@" in name:
            return name
        return name + "@" + grass.gisenv()["MAPSET"]


class NewVectorDialog(VectorDialog):
    def __init__(
        self,
        parent,
        title=_("Create new vector map"),
        disableAdd=False,
        disableTable=False,
        showType=False,
    ):
        """Dialog for creating new vector map

        :param parent: parent window
        :param title: window title
        :param disableAdd: disable 'add layer' checkbox
        :param disableTable: disable 'create table' checkbox
        :param showType: True to show feature type selector (used for creating new
                         empty OGR layers)

        :return: dialog instance
        """
        VectorDialog.__init__(self, parent, title)

        # determine output format
        if showType:
            self.ftype = OgrTypeSelect(parent=self, panel=self.panel)
        else:
            self.ftype = None

        # create attribute table
        self.table = wx.CheckBox(
            parent=self.panel, id=wx.ID_ANY, label=_("Create attribute table")
        )
        self.table.SetValue(True)
        if disableTable:
            self.table.Enable(False)

        if showType:
            self.keycol = None
        else:
            self.keycol = TextCtrl(
                parent=self.panel, id=wx.ID_ANY, size=globalvar.DIALOG_SPIN_SIZE
            )
            self.keycol.SetValue(
                UserSettings.Get(group="atm", key="keycolumn", subkey="value")
            )
            if disableTable:
                self.keycol.Enable(False)

        self.addbox = wx.CheckBox(
            parent=self.panel,
            label=_("Add created map into layer tree"),
            style=wx.NO_BORDER,
        )
        if disableAdd:
            self.addbox.SetValue(True)
            self.addbox.Enable(False)
        else:
            self.addbox.SetValue(
                UserSettings.Get(group="cmd", key="addNewLayer", subkey="enabled")
            )

        self.table.Bind(wx.EVT_CHECKBOX, self.OnTable)

        self.warning = _("Name of new vector map is missing.")

    def _selection_widget(self, layerTree):
        return Select(
            parent=self.panel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            type="vector",
            layerTree=layerTree,
            fullyQualified=False,
            validator=MapValidator(),
        )

    def OnTable(self, event):
        if self.keycol:
            self.keycol.Enable(event.IsChecked())

    def _layout(self):
        """Do layout"""
        self.dataSizer.Add(
            StaticText(
                parent=self.panel, id=wx.ID_ANY, label=_("Name for new vector map:")
            ),
            proportion=0,
            flag=wx.ALL,
            border=1,
        )
        self.dataSizer.Add(
            self.element, proportion=0, flag=wx.EXPAND | wx.ALL, border=1
        )
        if self.ftype:
            self.dataSizer.AddSpacer(1)
            self.dataSizer.Add(
                self.ftype, proportion=0, flag=wx.EXPAND | wx.ALL, border=1
            )

        self.dataSizer.Add(self.table, proportion=0, flag=wx.EXPAND | wx.ALL, border=1)

        if self.keycol:
            keySizer = wx.BoxSizer(wx.HORIZONTAL)
            keySizer.Add(
                StaticText(parent=self.panel, label=_("Key column:")),
                proportion=0,
                flag=wx.ALIGN_CENTER_VERTICAL,
            )
            keySizer.AddSpacer(10)
            keySizer.Add(self.keycol, proportion=0)
            self.dataSizer.Add(
                keySizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=1
            )

        self.dataSizer.AddSpacer(5)

        self.dataSizer.Add(self.addbox, proportion=0, flag=wx.EXPAND | wx.ALL, border=1)

        self.panel.SetSizer(self.sizer)
        self.sizer.Fit(self)
        self.SetMinSize(self.GetSize())

    def GetKey(self):
        """Get key column name"""
        if self.keycol:
            return self.keycol.GetValue()
        return UserSettings.Get(group="atm", key="keycolumn", subkey="value")

    def IsChecked(self, key):
        """Get dialog properties

        :param key: window key ('add', 'table')

        :return: True/False
        :return: None on error
        """
        if key == "add":
            return self.addbox.IsChecked()
        if key == "table":
            return self.table.IsChecked()

        return None

    def GetFeatureType(self):
        """Get feature type for OGR

        :return: feature type as string
        :return: None for native format
        """
        if self.ftype:
            return self.ftype.GetType()

        return None


def CreateNewVector(
    parent,
    cmd,
    title=_("Create new vector map"),
    exceptMap=None,
    giface=None,
    disableAdd=False,
    disableTable=False,
):
    r"""Create new vector map layer

    :param cmd: (prog, \*\*kwargs)
    :param title: window title
    :param exceptMap: list of maps to be excepted
    :param log:
    :param disableAdd: disable 'add layer' checkbox
    :param disableTable: disable 'create table' checkbox

    :return: dialog instance
    :return: None on error
    """
    vExternalOut = grass.parse_command("v.external.out", flags="g")
    isNative = vExternalOut["format"] == "native"
    showType = bool(cmd[0] == "v.edit" and not isNative)
    dlg = NewVectorDialog(
        parent,
        title=title,
        disableAdd=disableAdd,
        disableTable=disableTable,
        showType=showType,
    )

    if dlg.ShowModal() != wx.ID_OK:
        dlg.Destroy()
        return None

    outmap = dlg.GetName()
    key = dlg.GetKey()
    if outmap == exceptMap:
        GError(parent=parent, message=_("Unable to create vector map <%s>.") % outmap)
        dlg.Destroy()
        return None
    if dlg.table.IsEnabled() and not key:
        GError(
            parent=parent,
            message=_("Invalid or empty key column.\nUnable to create vector map <%s>.")
            % outmap,
        )
        dlg.Destroy()
        return None

    if outmap == "":  # should not happen
        dlg.Destroy()
        return None

    # update cmd -> output name defined
    cmd[1][cmd[2]] = outmap
    if showType:
        cmd[1]["type"] = dlg.GetFeatureType()

    curMapset = grass.gisenv()["MAPSET"]
    if isNative:
        listOfVectors = grass.list_grouped("vector")[curMapset]
    else:
        listOfVectors = RunCommand(
            "v.external",
            quiet=True,
            parent=parent,
            read=True,
            flags="l",
            input=vExternalOut["dsn"],
        ).splitlines()

    overwrite = False
    if (
        not UserSettings.Get(group="cmd", key="overwrite", subkey="enabled")
        and outmap in listOfVectors
    ):
        dlgOw = wx.MessageDialog(
            parent,
            message=_(
                "Vector map <%s> already exists "
                "in the current mapset. "
                "Do you want to overwrite it?"
            )
            % outmap,
            caption=_("Overwrite?"),
            style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
        )
        if dlgOw.ShowModal() != wx.ID_YES:
            dlgOw.Destroy()
            dlg.Destroy()
            return None
        overwrite = True

    if UserSettings.Get(group="cmd", key="overwrite", subkey="enabled"):
        overwrite = True

    ret = RunCommand(prog=cmd[0], parent=parent, overwrite=overwrite, **cmd[1])
    if ret != 0:
        dlg.Destroy()
        return None

    if (
        not isNative
        and not grass.find_file(outmap, element="vector", mapset=curMapset)["fullname"]
    ):
        # create link for OGR layers
        RunCommand(
            "v.external",
            overwrite=overwrite,
            parent=parent,
            input=vExternalOut["dsn"],
            layer=outmap,
        )

    # create attribute table
    if dlg.table.IsEnabled() and dlg.table.IsChecked():
        if isNative:
            sql = "CREATE TABLE %s (%s INTEGER)" % (outmap, key)

            RunCommand("db.connect", flags="c")

            Debug.msg(1, "SQL: %s" % sql)
            RunCommand("db.execute", quiet=True, parent=parent, input="-", stdin=sql)

            RunCommand(
                "v.db.connect",
                quiet=True,
                parent=parent,
                map=outmap,
                table=outmap,
                key=key,
                layer="1",
            )
        # TODO: how to deal with attribute tables for OGR layers?

    # return fully qualified map name
    if "@" not in outmap:
        outmap += "@" + grass.gisenv()["MAPSET"]

    # if giface:
    #     giface.WriteLog(_("New vector map <%s> created") % outmap)

    return dlg


class SavedRegion(wx.Dialog):
    def __init__(self, parent, title, id=wx.ID_ANY, loadsave="load", **kwargs):
        """Loading or saving of display extents to saved region file

        :param loadsave: load or save region?
        """
        wx.Dialog.__init__(self, parent, id, title, **kwargs)

        self.loadsave = loadsave
        self.wind = ""

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.BoxSizer(wx.HORIZONTAL)
        label = StaticText(parent=self, id=wx.ID_ANY)
        box.Add(label, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
        if loadsave == "load":
            label.SetLabel(_("Load region:"))
            self._selection = Select(
                parent=self, size=globalvar.DIALOG_GSELECT_SIZE, type="windows"
            )
        elif loadsave == "save":
            label.SetLabel(_("Save region:"))
            self._selection = Select(
                parent=self,
                size=globalvar.DIALOG_GSELECT_SIZE,
                type="windows",
                mapsets=[grass.gisenv()["MAPSET"]],
                fullyQualified=False,
            )

        box.Add(self._selection, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=5)
        self._selection.SetFocus()
        self._selection.Bind(wx.EVT_TEXT, self.OnRegion)

        sizer.Add(
            box,
            proportion=0,
            flag=wx.GROW | wx.ALL,
            border=5,
        )

        line = wx.StaticLine(
            parent=self, id=wx.ID_ANY, size=(20, -1), style=wx.LI_HORIZONTAL
        )
        sizer.Add(
            line,
            proportion=0,
            flag=wx.GROW | wx.LEFT | wx.RIGHT,
            border=5,
        )

        btnsizer = wx.StdDialogButtonSizer()

        btn = Button(parent=self, id=wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = Button(parent=self, id=wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(btnsizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()

    def OnRegion(self, event):
        value = self._selection.GetValue()
        if "@" in value:
            value = value.rsplit("@", 1)[0]
        if not grass.legal_name(value):
            GMessage(
                parent=self,
                message=_(
                    "Name cannot begin with '.' "
                    "and must not contain space, quotes, "
                    "'/', ''', '@', ',', '=', '*', "
                    "and all other non-alphanumeric characters."
                ),
            )
        else:
            self.wind = value

    def GetName(self):
        """Return region name"""
        return self.wind


class GroupDialog(wx.Dialog):
    """Dialog for creating/editing groups"""

    def __init__(
        self,
        parent=None,
        defaultGroup=None,
        defaultSubgroup=None,
        title=_("Create or edit imagery groups"),
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        wx.Dialog.__init__(
            self, parent=parent, id=wx.ID_ANY, title=title, style=style, **kwargs
        )

        self.parent = parent
        self.defaultGroup = defaultGroup
        self.defaultSubgroup = defaultSubgroup
        self.currentGroup = self.defaultGroup
        self.currentSubgroup = self.defaultGroup

        self.dataChanged = False

        # signaling edit subgroup / group mode
        self.edit_subg = False

        # sungroup maps dict value - ischecked
        self.subgmaps = {}

        # list of group maps
        self.gmaps = []

        # pattern chosen for filtering
        self.flt_pattern = ""

        self.bodySizer = self._createDialogBody()

        # buttons
        btnOk = Button(parent=self, id=wx.ID_OK)
        btnApply = Button(parent=self, id=wx.ID_APPLY)
        btnClose = Button(parent=self, id=wx.ID_CANCEL)

        btnOk.SetToolTip(_("Apply changes to selected group and close dialog"))
        btnApply.SetToolTip(_("Apply changes to selected group"))
        btnClose.SetToolTip(_("Close dialog, changes are not applied"))

        # btnOk.SetDefault()

        # sizers & do layout
        # btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        # btnSizer.Add(item = btnClose, proportion = 0,
        #              flag = wx.RIGHT | wx.ALIGN_RIGHT | wx.EXPAND, border = 5)
        # btnSizer.Add(item = btnApply, proportion = 0,
        #              flag = wx.LEFT, border = 5)
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnOk)
        btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnClose)
        btnSizer.Realize()

        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(
            self.bodySizer, proportion=1, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=10
        )
        mainSizer.Add(
            wx.StaticLine(parent=self, id=wx.ID_ANY, style=wx.LI_HORIZONTAL),
            proportion=0,
            flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
            border=10,
        )

        mainSizer.Add(
            btnSizer,
            proportion=0,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.ALIGN_RIGHT,
            border=10,
        )

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

        btnOk.Bind(wx.EVT_BUTTON, self.OnOk)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnClose.Bind(wx.EVT_BUTTON, self.OnClose)

        # set dialog min size
        self.SetMinSize(self.GetSize())
        self.SetSize((-1, 400))

    def _createDialogBody(self):
        bodySizer = wx.BoxSizer(wx.VERTICAL)
        # TODO same text in MapLayersDialogBase

        filter_tooltip = _(
            "Put here a regular expression."
            " Characters '.*' stand for anything,"
            " character '^' stands for the beginning"
            " and '$' for the end."
        )

        # group selection
        bodySizer.Add(
            StaticText(
                parent=self,
                id=wx.ID_ANY,
                label=_("Select existing group or enter name of new group:"),
            ),
            flag=wx.TOP,
            border=10,
        )
        self.groupSelect = Select(
            parent=self,
            type="group",
            mapsets=[grass.gisenv()["MAPSET"]],
            size=globalvar.DIALOG_GSELECT_SIZE,
            fullyQualified=False,
        )  # searchpath?

        bodySizer.Add(self.groupSelect, flag=wx.TOP | wx.EXPAND, border=5)

        self.subg_chbox = wx.CheckBox(
            parent=self, id=wx.ID_ANY, label=_("Edit/create subgroup")
        )

        bodySizer.Add(self.subg_chbox, flag=wx.TOP, border=10)

        self.subg_panel = wx.Panel(self)
        subg_sizer = wx.BoxSizer(wx.VERTICAL)

        subg_sizer.Add(
            StaticText(
                parent=self.subg_panel,
                id=wx.ID_ANY,
                label=_("Select existing subgroup or enter name of new subgroup:"),
            )
        )

        self.subGroupSelect = SubGroupSelect(parent=self.subg_panel)

        subg_sizer.Add(self.subGroupSelect, flag=wx.EXPAND | wx.TOP, border=5)

        self.subg_panel.SetSizer(subg_sizer)

        bodySizer.Add(self.subg_panel, flag=wx.TOP | wx.EXPAND, border=5)

        bodySizer.AddSpacer(10)

        buttonSizer = wx.BoxSizer(wx.VERTICAL)

        # layers in group
        self.gListPanel = wx.Panel(self)

        gListSizer = wx.GridBagSizer(vgap=3, hgap=2)

        self.g_sel_all = wx.CheckBox(
            parent=self.gListPanel, id=wx.ID_ANY, label=_("Select all")
        )

        gListSizer.Add(self.g_sel_all, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 1))

        gListSizer.Add(
            StaticText(parent=self.gListPanel, label=_("Pattern:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(1, 0),
        )

        self.gfilter = TextCtrl(
            parent=self.gListPanel, id=wx.ID_ANY, value="", size=(250, -1)
        )
        self.gfilter.SetToolTip(filter_tooltip)

        gListSizer.Add(self.gfilter, flag=wx.EXPAND, pos=(1, 1))

        gListSizer.Add(
            StaticText(parent=self.gListPanel, label=_("List of maps:")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.BOTTOM,
            border=5,
            pos=(2, 0),
        )

        sizer = wx.BoxSizer(wx.HORIZONTAL)

        self.gLayerBox = ListBox(
            parent=self.gListPanel,
            id=wx.ID_ANY,
            size=(-1, 150),
            style=wx.LB_MULTIPLE | wx.LB_NEEDED_SB,
        )
        sizer.Add(self.gLayerBox, proportion=1, flag=wx.EXPAND)

        self.addLayer = Button(self.gListPanel, id=wx.ID_ADD)
        self.addLayer.SetToolTip(_("Select map layers and add them to the list."))
        buttonSizer.Add(self.addLayer, flag=wx.BOTTOM, border=10)

        self.removeLayer = Button(self.gListPanel, id=wx.ID_REMOVE)
        self.removeLayer.SetToolTip(_("Remove selected layer(s) from list."))
        buttonSizer.Add(self.removeLayer)
        sizer.Add(buttonSizer, flag=wx.LEFT, border=5)

        gListSizer.Add(sizer, flag=wx.EXPAND, pos=(2, 1))
        gListSizer.AddGrowableCol(1)
        gListSizer.AddGrowableRow(2)

        self.gListPanel.SetSizer(gListSizer)
        bodySizer.Add(self.gListPanel, proportion=1, flag=wx.EXPAND)

        # layers in subgroup
        self.subgListPanel = wx.Panel(self)

        subgListSizer = wx.GridBagSizer(vgap=3, hgap=2)

        # select toggle
        self.subg_sel_all = wx.CheckBox(
            parent=self.subgListPanel, id=wx.ID_ANY, label=_("Select all")
        )

        subgListSizer.Add(self.subg_sel_all, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 1))

        subgListSizer.Add(
            StaticText(parent=self.subgListPanel, label=_("Pattern:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(1, 0),
        )

        self.subgfilter = TextCtrl(
            parent=self.subgListPanel, id=wx.ID_ANY, value="", size=(250, -1)
        )
        self.subgfilter.SetToolTip(filter_tooltip)

        subgListSizer.Add(self.subgfilter, flag=wx.EXPAND, pos=(1, 1))

        subgListSizer.Add(
            StaticText(parent=self.subgListPanel, label=_("List of maps:")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.BOTTOM,
            border=5,
            pos=(2, 0),
        )

        self.subgListBox = CheckListBox(
            parent=self.subgListPanel, id=wx.ID_ANY, size=(250, 100)
        )
        self.subgListBox.SetToolTip(
            _("Check maps from group to be included into subgroup.")
        )

        subgListSizer.Add(self.subgListBox, flag=wx.EXPAND, pos=(2, 1))
        subgListSizer.AddGrowableCol(1)
        subgListSizer.AddGrowableRow(2)

        self.subgListPanel.SetSizer(subgListSizer)
        bodySizer.Add(self.subgListPanel, proportion=1, flag=wx.EXPAND)

        self.infoLabel = StaticText(parent=self, id=wx.ID_ANY)
        bodySizer.Add(self.infoLabel, flag=wx.TOP | wx.BOTTOM, border=5)

        # bindings
        self.gfilter.Bind(wx.EVT_TEXT, self.OnGroupFilter)
        self.subgfilter.Bind(wx.EVT_TEXT, self.OnSubgroupFilter)
        self.gLayerBox.Bind(wx.EVT_LISTBOX, self.OnGLayerCheck)
        self.subgListBox.Bind(wx.EVT_CHECKLISTBOX, self.OnSubgLayerCheck)
        self.groupSelect.GetTextCtrl().Bind(wx.EVT_TEXT, self.OnGroupSelected)
        self.addLayer.Bind(wx.EVT_BUTTON, self.OnAddLayer)
        self.removeLayer.Bind(wx.EVT_BUTTON, self.OnRemoveLayer)
        self.subg_chbox.Bind(wx.EVT_CHECKBOX, self.OnSubgChbox)
        self.subGroupSelect.Bind(wx.EVT_TEXT, lambda event: self.SubGroupSelected())
        self.subg_sel_all.Bind(wx.EVT_CHECKBOX, self.OnSubgSelAll)
        self.g_sel_all.Bind(wx.EVT_CHECKBOX, self.OnGSelAll)

        if self.defaultGroup:
            self.groupSelect.SetValue(self.defaultGroup)

        if self.defaultSubgroup is not None:
            self.subGroupSelect.SetValue(self.defaultSubgroup)
            self.subg_chbox.SetValue(1)
            self.SubgChbox(True)
        else:
            self.subg_chbox.SetValue(0)
            self.SubgChbox(False)

        return bodySizer

    def OnGLayerCheck(self, event):
        self._checkGSellAll()

    def OnSubgSelAll(self, event):
        check = event.IsChecked()
        for item in range(self.subgListBox.GetCount()):
            self.CheckSubgItem(item, check)
            self.dataChanged = True

        event.Skip()

    def OnGSelAll(self, event):
        check = event.IsChecked()
        if not check:
            self.gLayerBox.DeselectAll()
        else:
            self.gLayerBox.SelectAll()

        event.Skip()

    def _checkGSellAll(self):
        check = False

        nsel = len(self.gLayerBox.GetSelections())
        if self.gLayerBox.GetCount() == nsel and self.gLayerBox.GetCount() != 0:
            check = True

        self.g_sel_all.SetValue(check)

    def _checkSubGSellAll(self):
        not_all_checked = False
        if self.subgListBox.GetCount() == 0:
            not_all_checked = True
        else:
            for item in range(self.subgListBox.GetCount()):
                if not self.subgListBox.IsChecked(item):
                    not_all_checked = True

        self.subg_sel_all.SetValue(not not_all_checked)

    def OnSubgroupFilter(self, event):
        text = event.GetString()
        self.gfilter.ChangeValue(text)
        self.flt_pattern = text

        self.FilterGroup()
        self.FilterSubgroup()

        event.Skip()

    def OnGroupFilter(self, event):
        text = event.GetString()
        self.subgfilter.ChangeValue(text)
        self.flt_pattern = text

        self.FilterGroup()
        self.FilterSubgroup()

        event.Skip()

    def OnSubgLayerCheck(self, event):
        idx = event.GetInt()
        m = self.subgListBox.GetString(idx)
        self.subgmaps[m] = self.subgListBox.IsChecked(idx)
        self.dataChanged = True
        self._checkSubGSellAll()

    def CheckSubgItem(self, idx, val):
        m = self.subgListBox.GetString(idx)
        self.subgListBox.Check(idx, val)
        self.subgmaps[m] = val
        self.dataChanged = val

    def DisableSubgroupEdit(self):
        """Disable editation of subgroups in the dialog

        .. todo::
            used by gcp manager, maybe the gcp m should also support subgroups
        """
        self.edit_subg = False
        self.subg_panel.Hide()
        self.subg_chbox.Hide()
        self.subgListBox.Hide()

        self.Layout()

    def OnSubgChbox(self, event):
        edit_subg = self.subg_chbox.GetValue()
        self.SubgChbox(edit_subg)

    def SubgChbox(self, edit_subg):
        self._checkChange()
        if edit_subg:
            self.edit_subg = edit_subg

            self.SubGroupSelected()
            self._subgroupLayout()
        else:
            self.edit_subg = edit_subg

            self.GroupSelected()
            self._groupLayout()

        self.SetMinSize(self.GetBestSize())

    def _groupLayout(self):
        self.subg_panel.Hide()
        self.subgListPanel.Hide()
        self.gListPanel.Show()
        self.Layout()

    def _subgroupLayout(self):
        self.subg_panel.Show()
        self.subgListPanel.Show()
        self.gListPanel.Hide()
        self.Layout()

    def OnAddLayer(self, event):
        """Add new layer to listbox"""
        dlg = MapLayersDialogForGroups(
            parent=self, title=_("Add selected map layers into group")
        )

        if dlg.ShowModal() != wx.ID_OK:
            dlg.Destroy()
            return

        layers = dlg.GetMapLayers()
        for layer in layers:
            if layer not in self.gmaps:
                self.gLayerBox.Append(layer)
                self.gmaps.append(layer)
                self.dataChanged = True

    def OnRemoveLayer(self, event):
        """Remove layer from listbox"""
        # After removal of last selected item by .Delete,
        # ListBox selects the last of remaining items in the list
        # and thus adds a new item to GetSelections
        # Items are removed in reverse order to maintain positional number
        # of other selected items (ListBox is dynamic!)
        selections = sorted(self.gLayerBox.GetSelections(), reverse=True)
        for sel in selections:
            m = self.gLayerBox.GetString(sel)
            self.gLayerBox.Delete(sel)
            self.gmaps.remove(m)
            self.dataChanged = True

    def GetLayers(self):
        """Get layers"""
        if self.edit_subg:
            layers = [maps for maps, sel in self.subgmaps.items() if sel]
        else:
            layers = self.gmaps[:]

        return layers

    def OnGroupSelected(self, event):
        """Text changed in group selector"""
        # callAfter must be called to close popup before other actions
        wx.CallAfter(self.GroupSelected)

    def GroupSelected(self):
        """Group was selected, check if changes were applied"""
        self._checkChange()
        group, s = self.GetSelectedGroup()
        maps = []
        groups = self.GetExistGroups()
        if group in groups:
            maps = self.GetGroupLayers(group)

        self.subGroupSelect.Insert(group)

        self.gmaps = maps
        maps = self._filter(maps)

        self.ShowGroupLayers(maps)
        self.currentGroup = group

        self.SubGroupSelected()
        self.ClearNotification()

        self._checkGSellAll()

    def FilterGroup(self):
        maps = self._filter(self.gmaps)
        self.ShowGroupLayers(maps)
        self._checkGSellAll()

    def FilterSubgroup(self):
        maps = self._filter(self.gmaps)
        self.subgListBox.Set(maps)

        for i, m in enumerate(maps):
            if m in self.subgmaps.keys() and self.subgmaps[m]:
                self.subgListBox.Check(i)

        self._checkSubGSellAll()

    def SubGroupSelected(self):
        """Subgroup was selected, check if changes were applied"""
        self._checkChange()

        subgroup = self.subGroupSelect.GetValue().strip()
        group = self.currentGroup

        gmaps = []
        groups = self.GetExistGroups()

        self.subgmaps = {}
        if group in groups:
            gmaps = self.GetGroupLayers(group)
            if subgroup:
                maps = self.GetGroupLayers(group, subgroup)
                for m in maps:
                    if m in gmaps:
                        self.subgmaps[m] = True
                    else:
                        self.subgmaps[m] = False

        gmaps = self._filter(gmaps)
        self.subgListBox.Set(gmaps)

        for i, m in enumerate(gmaps):
            if m in self.subgmaps:
                self.subgListBox.Check(i)
            else:
                self.subgListBox.Check(i, False)

        self._checkSubGSellAll()
        self.currentSubgroup = subgroup
        self.ClearNotification()

    def _filter(self, data):
        """Apply filter for strings in data list"""
        flt_data = []
        if len(self.flt_pattern) == 0:
            return data[:]

        for dt in data:
            try:
                if re.compile(self.flt_pattern).search(dt):
                    flt_data.append(dt)
            except re.error:
                pass

        return flt_data

    def _checkChange(self):
        if self.edit_subg:
            self._checkSubgroupChange()
        else:
            self._checkGroupChange()

    def _checkGroupChange(self):
        if self.currentGroup and self.dataChanged:
            dlg = wx.MessageDialog(
                self,
                message=_("Group <%s> was changed, do you want to apply changes?")
                % self.currentGroup,
                caption=_("Unapplied changes"),
                style=wx.YES_NO | wx.ICON_QUESTION | wx.YES_DEFAULT,
            )
            if dlg.ShowModal() == wx.ID_YES:
                self.ApplyChanges()

            dlg.Destroy()
        self.dataChanged = False

    def _checkSubgroupChange(self):
        if self.currentSubgroup and self.dataChanged:
            dlg = wx.MessageDialog(
                self,
                message=_("Subgroup <%s> was changed, do you want to apply changes?")
                % self.currentSubgroup,
                caption=_("Unapplied changes"),
                style=wx.YES_NO | wx.ICON_QUESTION | wx.YES_DEFAULT,
            )
            if dlg.ShowModal() == wx.ID_YES:
                self.ApplyChanges()

            dlg.Destroy()
        self.dataChanged = False

    def ShowGroupLayers(self, mapList):
        """Show map layers in currently selected group"""
        self.gLayerBox.Set(mapList)

    def EditGroup(self, group, subgroup=None):
        """Edit selected group"""
        layersNew = self.GetLayers()
        layersOld = self.GetGroupLayers(group, subgroup)

        add = []
        remove = []
        for layerNew in layersNew:
            if layerNew not in layersOld:
                add.append(layerNew)

        for layerOld in layersOld:
            if layerOld not in layersNew:
                remove.append(layerOld)

        kwargs = {}
        if subgroup:
            kwargs["subgroup"] = subgroup

        ret = None
        if remove:
            ret = RunCommand(
                "i.group",
                parent=self,
                group=group,
                flags="r",
                input=",".join(remove),
                **kwargs,
            )

        if add:
            ret = RunCommand(
                "i.group", parent=self, group=group, input=",".join(add), **kwargs
            )

        return ret

    def CreateNewGroup(self, group, subgroup):
        """Create new group"""
        layers = self.GetLayers()
        if not layers:
            GMessage(parent=self, message=_("No raster maps selected."))
            return 1

        kwargs = {}
        if subgroup:
            kwargs["subgroup"] = subgroup

        ret = RunCommand("i.group", parent=self, group=group, input=layers, **kwargs)
        # update subgroup select
        self.SubGroupSelected()
        return ret

    def GetExistGroups(self):
        """Returns existing groups in current mapset"""
        return grass.list_grouped("group")[grass.gisenv()["MAPSET"]]

    def GetExistSubgroups(self, group):
        """Returns existing subgroups in a group"""
        return RunCommand("i.group", group=group, read=True, flags="sg").splitlines()

    def ShowResult(self, group, returnCode, create):
        """Show if operation was successful."""
        group += "@" + grass.gisenv()["MAPSET"]
        if returnCode is None:
            label = _("No changes to apply in group <%s>.") % group
        elif returnCode == 0:
            if create:
                label = _("Group <%s> was successfully created.") % group
            else:
                label = _("Group <%s> was successfully changed.") % group
        else:  # noqa: PLR5501
            if create:
                label = _("Creating of new group <%s> failed.") % group
            else:
                label = _("Changing of group <%s> failed.") % group

        self.infoLabel.SetLabel(label)
        wx.CallLater(4000, self.ClearNotification)

    def GetSelectedGroup(self):
        """Return currently selected group (without mapset)"""
        g = self.groupSelect.GetValue().split("@")[0]
        s = self.subGroupSelect.GetValue() if self.edit_subg else None
        return g, s

    def GetGroupLayers(self, group, subgroup=None):
        """Get layers in group"""
        kwargs = {"group": group}
        if subgroup:
            kwargs["subgroup"] = subgroup

        res = RunCommand("i.group", parent=self, flags="g", read=True, **kwargs)
        if not res:
            return []
        return res.splitlines()

    def ClearNotification(self):
        """Clear notification string"""
        if self.infoLabel:
            self.infoLabel.SetLabel("")

    def ApplyChanges(self):
        """Create or edit group"""
        group = self.currentGroup
        if not group:
            GMessage(parent=self, message=_("No group selected."))
            return False

        if self.edit_subg and not self.currentSubgroup:
            GMessage(parent=self, message=_("No subgroup selected."))
            return 0

        subgroup = self.currentSubgroup if self.edit_subg else None

        groups = self.GetExistGroups()
        if group in groups:
            ret = self.EditGroup(group, subgroup)
            self.ShowResult(group=group, returnCode=ret, create=False)

        else:
            ret = self.CreateNewGroup(group, subgroup)
            self.ShowResult(group=group, returnCode=ret, create=True)

        self.dataChanged = False

        return True

    def OnApply(self, event):
        """Apply changes"""
        self.ApplyChanges()

    def OnOk(self, event):
        """Apply changes and close dialog"""
        if self.ApplyChanges():
            self.OnClose(event)

    def OnClose(self, event):
        """Close dialog"""
        if not self.IsModal():
            self.Destroy()
        event.Skip()


class MapLayersDialogBase(wx.Dialog):
    """Base dialog for selecting map layers (raster, vector).

    There are 3 subclasses: MapLayersDialogForGroups, MapLayersDialogForModeler,
    MapLayersDialog. Base class contains core functionality.
    """

    def __init__(
        self, parent, title, style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, **kwargs
    ):
        wx.Dialog.__init__(
            self, parent=parent, id=wx.ID_ANY, title=title, style=style, **kwargs
        )

        self.parent = parent  # GMFrame or ?

        self.applyAddingMapLayers = Signal("MapLayersDialogBase.applyAddingMapLayers")

        self.mainSizer = wx.BoxSizer(wx.VERTICAL)

        # dialog body
        self.bodySizer = self._createDialogBody()
        self.mainSizer.Add(
            self.bodySizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=5
        )

        # update list of layer to be loaded
        self.map_layers = []  # list of map layers (full list type/mapset)
        self.LoadMapLayers(
            self.GetLayerType(cmd=True), self.mapset.GetStringSelection()
        )

        self._fullyQualifiedNames()
        self._modelerDSeries()

        # buttons
        btnCancel = Button(parent=self, id=wx.ID_CANCEL)
        btnOk = Button(parent=self, id=wx.ID_OK)
        btnOk.SetDefault()

        # sizers & do layout
        self.btnSizer = wx.StdDialogButtonSizer()
        self.btnSizer.AddButton(btnCancel)
        self.btnSizer.AddButton(btnOk)
        self._addApplyButton()
        self.btnSizer.Realize()

        self.mainSizer.Add(
            self.btnSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5
        )

        self.SetSizer(self.mainSizer)
        self.mainSizer.Fit(self)

        # set dialog min size
        self.SetMinSize(self.GetSize())

    def _modelerDSeries(self):
        """Method used only by MapLayersDialogForModeler,
        for other subclasses does nothing.
        """

    def _addApplyButton(self):
        """Method used only by MapLayersDialog,
        for other subclasses does nothing.
        """

    def _fullyQualifiedNames(self):
        """Adds CheckBox which determines if fully qualified names are returned."""
        self.fullyQualified = wx.CheckBox(
            parent=self, id=wx.ID_ANY, label=_("Use fully-qualified map names")
        )
        self.fullyQualified.SetValue(True)
        self.mainSizer.Add(
            self.fullyQualified,
            proportion=0,
            flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
            border=5,
        )

    def _useFullyQualifiedNames(self):
        return self.fullyQualified.IsChecked()

    def _layerTypes(self):
        """Determines which layer types can be chosen.

        Valid values:
        - raster
        - raster3d
        - vector
        """
        return [_("raster"), _("3D raster"), _("vector")]

    def _selectAll(self):
        """Check all layers by default"""
        return True

    def _createDialogBody(self):
        bodySizer = wx.GridBagSizer(vgap=3, hgap=3)

        # layer type
        bodySizer.Add(
            StaticText(parent=self, label=_("Map type:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(0, 0),
        )

        self.layerType = wx.Choice(
            parent=self, id=wx.ID_ANY, choices=self._layerTypes(), size=(100, -1)
        )

        self.layerType.SetSelection(0)

        bodySizer.Add(self.layerType, pos=(0, 1))
        self.layerType.Bind(wx.EVT_CHOICE, self.OnChangeParams)

        # select toggle
        self.toggle = wx.CheckBox(parent=self, id=wx.ID_ANY, label=_("Select toggle"))
        self.toggle.SetValue(self._selectAll())
        bodySizer.Add(self.toggle, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 2))

        # mapset filter
        bodySizer.Add(
            StaticText(parent=self, label=_("Mapset:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(1, 0),
        )

        self.mapset = MapsetSelect(parent=self, searchPath=True)
        self.mapset.SetStringSelection(grass.gisenv()["MAPSET"])
        bodySizer.Add(self.mapset, pos=(1, 1), span=(1, 2))

        # map name filter
        bodySizer.Add(
            StaticText(parent=self, label=_("Pattern:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(2, 0),
        )

        self.filter = TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(250, -1))
        bodySizer.Add(self.filter, flag=wx.EXPAND, pos=(2, 1), span=(1, 2))

        self.filter.SetFocus()
        # TODO same text in GroupDialog
        self.filter.SetToolTip(
            _(
                "Put here a regular expression."
                " Characters '.*' stand for anything,"
                " character '^' stands for the beginning"
                " and '$' for the end."
            )
        )

        # layer list
        bodySizer.Add(
            StaticText(parent=self, label=_("List of maps:")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_TOP,
            pos=(3, 0),
        )
        self.layers = CheckListBox(
            parent=self, id=wx.ID_ANY, size=(250, 100), choices=[]
        )
        bodySizer.Add(self.layers, flag=wx.EXPAND, pos=(3, 1), span=(1, 2))

        bodySizer.AddGrowableCol(1)
        bodySizer.AddGrowableRow(3)

        # bindings
        self.mapset.Bind(wx.EVT_TEXT, self.OnChangeParams)
        self.mapset.Bind(wx.EVT_COMBOBOX, self.OnChangeParams)
        self.layers.Bind(wx.EVT_RIGHT_DOWN, self.OnMenu)
        self.filter.Bind(wx.EVT_TEXT, self.OnFilter)
        self.toggle.Bind(wx.EVT_CHECKBOX, self.OnToggle)

        return bodySizer

    def LoadMapLayers(self, type, mapset):
        """Load list of map layers

        :param str type: layer type ('raster' or 'vector')
        :param str mapset: mapset name
        """
        self.map_layers = grass.list_grouped(type=type)[mapset]
        self.layers.Set(naturally_sorted(self.map_layers))

        # check all items by default
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, check=self._selectAll())

    def OnChangeParams(self, event):
        """Filter parameters changed by user"""
        # update list of layer to be loaded
        self.LoadMapLayers(
            self.GetLayerType(cmd=True), self.mapset.GetStringSelection()
        )

        event.Skip()

    def OnMenu(self, event):
        """Table description area, context menu"""
        if not hasattr(self, "popupID1"):
            self.popupDataID1 = NewId()
            self.popupDataID2 = NewId()
            self.popupDataID3 = NewId()

            self.Bind(wx.EVT_MENU, self.OnSelectAll, id=self.popupDataID1)
            self.Bind(wx.EVT_MENU, self.OnSelectInvert, id=self.popupDataID2)
            self.Bind(wx.EVT_MENU, self.OnDeselectAll, id=self.popupDataID3)

        # generate popup-menu
        menu = Menu()
        menu.Append(self.popupDataID1, _("Select all"))
        menu.Append(self.popupDataID2, _("Invert selection"))
        menu.Append(self.popupDataID3, _("Deselect all"))

        self.PopupMenu(menu)
        menu.Destroy()

    def OnSelectAll(self, event):
        """Select all map layer from list"""
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, True)

    def OnSelectInvert(self, event):
        """Invert current selection"""
        for item in range(self.layers.GetCount()):
            if self.layers.IsChecked(item):
                self.layers.Check(item, False)
            else:
                self.layers.Check(item, True)

    def OnDeselectAll(self, event):
        """Select all map layer from list"""
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, False)

    def OnFilter(self, event):
        """Apply filter for map names"""
        if len(event.GetString()) == 0:
            self.layers.Set(self.map_layers)
            return

        list = []
        for layer in self.map_layers:
            try:
                if re.compile(event.GetString()).search(layer):
                    list.append(layer)
            except re.error:
                pass
        list = naturally_sorted(list)

        self.layers.Set(list)
        self.OnSelectAll(None)

        event.Skip()

    def OnToggle(self, event):
        """Select toggle (check or uncheck all layers)"""
        check = event.IsChecked()
        for item in range(self.layers.GetCount()):
            self.layers.Check(item, check)

        event.Skip()

    def GetMapLayers(self):
        """Return list of checked map layers"""
        layerNames = []
        for indx in self.layers.GetSelections():
            # layers.append(self.layers.GetStringSelec(indx))
            pass

        mapset = self.mapset.GetStringSelection()
        for item in range(self.layers.GetCount()):
            if not self.layers.IsChecked(item):
                continue
            if self._useFullyQualifiedNames():
                layerNames.append(self.layers.GetString(item) + "@" + mapset)
            else:
                layerNames.append(self.layers.GetString(item))

        return layerNames

    def GetLayerType(self, cmd=False):
        """Get selected layer type

        :param bool cmd: True for g.list
        """
        if not cmd:
            return self.layerType.GetStringSelection()

        sel = self.layerType.GetSelection()
        if sel == 0:
            ltype = "raster"
        elif sel == 1:
            ltype = "raster_3d"
        else:
            ltype = "vector"

        return ltype


class MapLayersDialog(MapLayersDialogBase):
    """Subclass of MapLayersDialogBase used in Layer Manager.

    Contains apply button, which sends wxApplyMapLayers event.
    """

    def __init__(self, parent, title, **kwargs):
        MapLayersDialogBase.__init__(self, parent=parent, title=title, **kwargs)

    def _addApplyButton(self):
        btnApply = Button(parent=self, id=wx.ID_APPLY)
        self.btnSizer.AddButton(btnApply)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)

    def OnApply(self, event):
        self.applyAddingMapLayers.emit(
            mapLayers=self.GetMapLayers(), ltype=self.GetLayerType(cmd=True)
        )


class MapLayersDialogForGroups(MapLayersDialogBase):
    """Subclass of MapLayersDialogBase used for specifying maps in an imagery group.

    Shows only raster maps.
    """

    def __init__(self, parent, title, **kwargs):
        MapLayersDialogBase.__init__(self, parent=parent, title=title, **kwargs)

    def _layerTypes(self):
        return [
            _("raster"),
        ]

    def _selectAll(self):
        """Could be overridden"""
        return False

    def _fullyQualifiedNames(self):
        pass

    def _useFullyQualifiedNames(self):
        return True


class MapLayersDialogForModeler(MapLayersDialogBase):
    """Subclass of MapLayersDialogBase used in Modeler."""

    def __init__(self, parent, title, **kwargs):
        MapLayersDialogBase.__init__(self, parent=parent, title=title, **kwargs)

    def _modelerDSeries(self):
        self.dseries = wx.CheckBox(
            parent=self, id=wx.ID_ANY, label=_("Dynamic series (%s)") % "g.list"
        )
        self.dseries.SetValue(False)
        self.mainSizer.Add(
            self.dseries, proportion=0, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5
        )

    def GetDSeries(self):
        """Used by modeler only

        :return: g.list command
        """
        if not self.dseries or not self.dseries.IsChecked():
            return ""

        cond = "map in `g.list type=%s " % self.GetLayerType(cmd=True)
        patt = self.filter.GetValue()
        if patt:
            cond += "pattern=%s " % patt
        cond += "mapset=%s`" % self.mapset.GetStringSelection()

        return cond


class SetOpacityDialog(wx.Dialog):
    """Set opacity of map layers.
    Dialog expects opacity between 0 and 1 and returns this range, too.
    """

    def __init__(
        self,
        parent,
        id=wx.ID_ANY,
        title=_("Set Map Layer Opacity"),
        size=wx.DefaultSize,
        pos=wx.DefaultPosition,
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        opacity=1,
    ):
        self.parent = parent  # GMFrame
        self.opacity = opacity  # current opacity

        super().__init__(parent, id=id, pos=pos, size=size, style=style, title=title)

        self.applyOpacity = Signal("SetOpacityDialog.applyOpacity")
        panel = wx.Panel(parent=self, id=wx.ID_ANY)

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.GridBagSizer(vgap=5, hgap=5)
        box.AddGrowableCol(0)
        self.value = Slider(
            panel,
            id=wx.ID_ANY,
            value=int(self.opacity * 100),
            style=wx.SL_HORIZONTAL | wx.SL_AUTOTICKS | wx.SL_TOP | wx.SL_LABELS,
            minValue=0,
            maxValue=100,
        )

        box.Add(self.value, flag=wx.EXPAND, pos=(0, 0), span=(1, 2))
        box.Add(
            StaticText(parent=panel, id=wx.ID_ANY, label=_("transparent")), pos=(1, 0)
        )
        box.Add(
            StaticText(parent=panel, id=wx.ID_ANY, label=_("opaque")),
            flag=wx.ALIGN_RIGHT,
            pos=(1, 1),
        )

        sizer.Add(box, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        line = wx.StaticLine(parent=panel, id=wx.ID_ANY, style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        # buttons
        btnsizer = wx.StdDialogButtonSizer()

        btnOK = Button(parent=panel, id=wx.ID_OK)
        btnOK.SetDefault()
        btnsizer.AddButton(btnOK)

        btnCancel = Button(parent=panel, id=wx.ID_CANCEL)
        btnsizer.AddButton(btnCancel)

        btnApply = Button(parent=panel, id=wx.ID_APPLY)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnsizer.AddButton(btnApply)
        btnsizer.Realize()

        sizer.Add(btnsizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        panel.SetSizer(sizer)
        sizer.Fit(panel)

        w, h = self.GetBestSize()
        self.SetSize(wx.Size(w, h))
        self.SetMaxSize(wx.Size(-1, h))
        self.SetMinSize(wx.Size(w, h))

        self.Layout()

    def GetOpacity(self):
        """Button 'OK' pressed"""
        # return opacity value
        return float(self.value.GetValue()) / 100

    def OnApply(self, event):
        self.applyOpacity.emit(value=self.GetOpacity())


def GetImageHandlers(image):
    """Get list of supported image handlers"""
    lext = []
    ltype = []
    try:
        for h in image.GetHandlers():
            lext.append(h.GetExtension())
    except AttributeError:
        lext = {"png", "gif", "jpg", "pcx", "pnm", "tif", "xpm"}

    filetype = ""
    if "png" in lext:
        filetype += "PNG file (*.png)|*.png|"
        ltype.append({"type": wx.BITMAP_TYPE_PNG, "ext": "png"})
    filetype += "BMP file (*.bmp)|*.bmp|"
    ltype.append({"type": wx.BITMAP_TYPE_BMP, "ext": "bmp"})
    if "gif" in lext:
        filetype += "GIF file (*.gif)|*.gif|"
        ltype.append({"type": wx.BITMAP_TYPE_GIF, "ext": "gif"})

    if "jpg" in lext:
        filetype += "JPG file (*.jpg)|*.jpg|"
        ltype.append({"type": wx.BITMAP_TYPE_JPEG, "ext": "jpg"})

    if "pcx" in lext:
        filetype += "PCX file (*.pcx)|*.pcx|"
        ltype.append({"type": wx.BITMAP_TYPE_PCX, "ext": "pcx"})

    if "pnm" in lext:
        filetype += "PNM file (*.pnm)|*.pnm|"
        ltype.append({"type": wx.BITMAP_TYPE_PNM, "ext": "pnm"})

    if "tif" in lext:
        filetype += "TIF file (*.tif)|*.tif|"
        ltype.append({"type": wx.BITMAP_TYPE_TIF, "ext": "tif"})

    if "xpm" in lext:
        filetype += "XPM file (*.xpm)|*.xpm"
        ltype.append({"type": wx.BITMAP_TYPE_XPM, "ext": "xpm"})

    return filetype, ltype


class ImageSizeDialog(wx.Dialog):
    """Set size for saved graphic file"""

    def __init__(
        self,
        parent,
        id=wx.ID_ANY,
        title=_("Set image size"),
        style=wx.DEFAULT_DIALOG_STYLE,
        **kwargs,
    ):
        img_size = kwargs.pop("img_size", None)
        self.parent = parent

        wx.Dialog.__init__(self, parent, id=id, style=style, title=title, **kwargs)

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.box = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=" % s" % _("Image size")
        )

        size = self.parent.GetWindow().GetClientSize()
        self.width = SpinCtrl(parent=self.panel, id=wx.ID_ANY, style=wx.SP_ARROW_KEYS)
        self.width.SetRange(20, 1e6)
        self.width.SetValue(img_size[0] if img_size else size.width)
        wx.CallAfter(self.width.SetFocus)
        self.height = SpinCtrl(parent=self.panel, id=wx.ID_ANY, style=wx.SP_ARROW_KEYS)
        self.height.SetRange(20, 1e6)
        self.height.SetValue(img_size[1] if img_size else size.height)
        self.template = wx.Choice(
            parent=self.panel,
            id=wx.ID_ANY,
            size=(125, -1),
            choices=[
                "",
                "640x480",
                "800x600",
                "1024x768",
                "1280x960",
                "1600x1200",
                "1920x1440",
            ],
        )

        self.btnOK = Button(parent=self.panel, id=wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnCancel = Button(parent=self.panel, id=wx.ID_CANCEL)

        self.template.Bind(wx.EVT_CHOICE, self.OnTemplate)

        self._layout()
        self.SetSize(self.GetBestSize())

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        # body
        box = wx.StaticBoxSizer(self.box, wx.HORIZONTAL)
        fbox = wx.FlexGridSizer(cols=2, vgap=5, hgap=5)
        fbox.Add(
            StaticText(parent=self.panel, id=wx.ID_ANY, label=_("Width:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
        )
        fbox.Add(self.width)
        fbox.Add(
            StaticText(parent=self.panel, id=wx.ID_ANY, label=_("Height:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
        )
        fbox.Add(self.height)
        fbox.Add(
            StaticText(parent=self.panel, id=wx.ID_ANY, label=_("Template:")),
            flag=wx.ALIGN_CENTER_VERTICAL,
        )
        fbox.Add(self.template)

        box.Add(fbox, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        sizer.Add(box, proportion=1, flag=wx.EXPAND | wx.ALL, border=3)

        # buttons
        btnsizer = wx.StdDialogButtonSizer()
        btnsizer.AddButton(self.btnOK)
        btnsizer.AddButton(self.btnCancel)
        btnsizer.Realize()

        sizer.Add(btnsizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        self.panel.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()

    def GetValues(self):
        """Get width/height values"""
        return self.width.GetValue(), self.height.GetValue()

    def OnTemplate(self, event):
        """Template selected"""
        sel = event.GetString()
        if not sel:
            width, height = self.parent.GetWindow().GetClientSize()
        else:
            width, height = map(int, sel.split("x"))
        self.width.SetValue(width)
        self.height.SetValue(height)


class SqlQueryFrame(wx.Frame):
    def __init__(self, parent, id=wx.ID_ANY, title=_("SQL Query Utility"), *kwargs):
        """SQL Query Utility window"""
        self.parent = parent

        wx.Frame.__init__(self, parent=parent, id=id, title=title, *kwargs)
        self.SetIcon(
            wx.Icon(
                os.path.join(globalvar.ICONDIR, "grass_sql.ico"), wx.BITMAP_TYPE_ICO
            )
        )
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.sqlBox = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=_(" SQL statement ")
        )
        self.sql = TextCtrl(parent=self.panel, id=wx.ID_ANY, style=wx.TE_MULTILINE)

        self.btnApply = Button(parent=self.panel, id=wx.ID_APPLY)
        self.btnCancel = Button(parent=self.panel, id=wx.ID_CANCEL)
        self.Bind(wx.EVT_BUTTON, self.OnCloseWindow, self.btnCancel)

        self._layout()

        self.SetMinSize(wx.Size(300, 150))
        self.SetSize(wx.Size(500, 200))

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        sqlSizer = wx.StaticBoxSizer(self.sqlBox, wx.HORIZONTAL)
        sqlSizer.Add(self.sql, proportion=1, flag=wx.EXPAND)

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnApply)
        btnSizer.AddButton(self.btnCancel)
        btnSizer.Realize()

        sizer.Add(sqlSizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        sizer.Add(
            btnSizer,
            proportion=0,
            flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
            border=5,
        )

        self.panel.SetSizer(sizer)

        self.Layout()

    def OnCloseWindow(self, event):
        """Close window"""
        self.Close()


class SymbolDialog(wx.Dialog):
    """Dialog for GRASS symbols selection.

    Dialog is called in gui_core::forms module.
    """

    def __init__(self, parent, symbolPath, currentSymbol=None, title=_("Symbols")):
        """Dialog constructor.

        It is assumed that symbolPath contains folders with symbols.

        :param parent: dialog parent
        :param symbolPath: absolute path to symbols
        :param currentSymbol: currently selected symbol (e.g. 'basic/x')
        :param title: dialog title
        """
        wx.Dialog.__init__(self, parent=parent, title=title, id=wx.ID_ANY)

        self.symbolPath = Path(symbolPath)
        self.currentSymbol = currentSymbol  # default basic/x
        self.selected = None
        self.selectedDir = None

        self._layout()

    def _layout(self):
        mainPanel = wx.Panel(self, id=wx.ID_ANY)
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        vSizer = wx.BoxSizer(wx.VERTICAL)
        fgSizer = wx.FlexGridSizer(rows=2, cols=2, vgap=5, hgap=5)
        self.folderChoice = wx.Choice(
            mainPanel, id=wx.ID_ANY, choices=[p.name for p in self.symbolPath.iterdir()]
        )
        self.folderChoice.Bind(wx.EVT_CHOICE, self.OnFolderSelect)

        fgSizer.Add(
            StaticText(mainPanel, id=wx.ID_ANY, label=_("Symbol directory:")),
            proportion=0,
            flag=wx.ALIGN_CENTER_VERTICAL,
        )

        fgSizer.Add(self.folderChoice, proportion=0, flag=wx.ALIGN_CENTER, border=0)

        self.infoLabel = StaticText(mainPanel, id=wx.ID_ANY)
        fgSizer.Add(
            StaticText(mainPanel, id=wx.ID_ANY, label=_("Symbol name:")),
            flag=wx.ALIGN_CENTRE_VERTICAL,
        )
        fgSizer.Add(self.infoLabel, proportion=0, flag=wx.ALIGN_CENTRE_VERTICAL)
        vSizer.Add(fgSizer, proportion=0, flag=wx.ALL, border=5)

        self.panels = self._createSymbolPanels(mainPanel)
        for panel in self.panels:
            vSizer.Add(panel, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)

        mainSizer.Add(vSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=5)
        self.btnCancel = Button(parent=mainPanel, id=wx.ID_CANCEL)
        self.btnOK = Button(parent=mainPanel, id=wx.ID_OK)
        self.btnOK.SetDefault()
        self.btnOK.Enable(False)

        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()
        mainSizer.Add(btnSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        # show panel with the largest number of images and fit size
        count = [
            len(list((self.symbolPath / folder).iterdir()))
            for folder in [p.name for p in self.symbolPath.iterdir()]
        ]

        index = count.index(max(count))
        self.folderChoice.SetSelection(index)
        self.OnFolderSelect(None)
        self.infoLabel.Show()

        mainPanel.SetSizerAndFit(mainSizer)
        self.SetSize(self.GetBestSize())

        # show currently selected symbol
        if self.currentSymbol:
            # set directory
            self.selectedDir, self.selected = os.path.split(self.currentSymbol)
            self.folderChoice.SetStringSelection(self.selectedDir)
            # select symbol
            panelIdx = self.folderChoice.GetSelection()
            for panel in self.symbolPanels[panelIdx]:
                if panel.GetName() == self.selected:
                    panel.Select()
        else:
            self.folderChoice.SetSelection(0)

        self.OnFolderSelect(None)

    def _createSymbolPanels(self, parent):
        """Creates multiple panels with symbols.

        Panels are shown/hidden according to selected folder."""
        folders = [p.name for p in self.symbolPath.iterdir()]

        panels = []
        self.symbolPanels = []

        for folder in folders:
            panel = wx.Panel(parent, style=wx.BORDER_RAISED)
            sizer = wx.GridSizer(cols=6, vgap=3, hgap=3)
            images = self._getSymbols(path=self.symbolPath / folder)

            symbolPanels = []
            for img in images:
                iP = SingleSymbolPanel(parent=panel, symbolPath=img)
                iP.symbolSelectionChanged.connect(self.SelectionChanged)
                sizer.Add(iP, proportion=0, flag=wx.ALIGN_CENTER)
                symbolPanels.append(iP)

            panel.SetSizerAndFit(sizer)
            panel.Hide()
            panels.append(panel)
            self.symbolPanels.append(symbolPanels)

        return panels

    def _getSymbols(self, path):
        # we assume that images are in subfolders (1 level only)
        imageList = [str(p) for p in path.iterdir()]

        return sorted(imageList)

    def OnFolderSelect(self, event):
        """Selected folder with symbols changed."""
        idx = self.folderChoice.GetSelection()
        for i in range(len(self.panels)):
            sizer = self.panels[i].GetContainingSizer()
            sizer.Show(self.panels[i], i == idx, recursive=True)
            sizer.Layout()

        if self.selectedDir == self.folderChoice.GetStringSelection():
            self.btnOK.Enable()
            self.infoLabel.SetLabel(self.selected)
        else:
            self.btnOK.Disable()
            self.infoLabel.SetLabel("")

    def SelectionChanged(self, name, doubleClick):
        """Selected symbol changed."""
        if doubleClick:
            self.EndModal(wx.ID_OK)
        # deselect all
        for i in range(len(self.panels)):
            for panel in self.symbolPanels[i]:
                if panel.GetName() != name:
                    panel.Deselect()

        self.btnOK.Enable()

        self.selected = name
        self.selectedDir = self.folderChoice.GetStringSelection()

        self.infoLabel.SetLabel(name)

    def GetSelectedSymbolName(self):
        """Returns currently selected symbol name (e.g. 'basic/x')."""
        # separator must be '/' and not dependent on OS
        return self.selectedDir + "/" + self.selected

    def GetSelectedSymbolPath(self):
        """Returns currently selected symbol full path."""
        return os.path.join(self.symbolPath, self.selectedDir, self.selected)


class TextEntryDialog(wx.Dialog):
    """Simple dialog with text field.

    It differs from wx.TextEntryDialog because it allows adding validator.
    """

    def __init__(
        self,
        parent,
        message,
        caption="",
        defaultValue="",
        validator=wx.DefaultValidator,
        style=wx.OK | wx.CANCEL | wx.CENTRE,
        textStyle=0,
        textSize=(300, -1),
        **kwargs,
    ):
        wx.Dialog.__init__(self, parent=parent, id=wx.ID_ANY, title=caption, **kwargs)

        vbox = wx.BoxSizer(wx.VERTICAL)

        stline = StaticText(self, id=wx.ID_ANY, label=message)
        vbox.Add(stline, proportion=0, flag=wx.EXPAND | wx.ALL, border=10)

        self._textCtrl = TextCtrl(
            self, id=wx.ID_ANY, value=defaultValue, validator=validator, style=textStyle
        )
        self._textCtrl.SetInitialSize(textSize)
        wx.CallAfter(self._textCtrl.SetFocus)

        vbox.Add(
            self._textCtrl, proportion=0, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=10
        )
        self._textCtrl.SetFocus()

        sizer = self.CreateSeparatedButtonSizer(style)
        vbox.Add(sizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=10)

        self.SetSizerAndFit(vbox)

    def GetValue(self):
        return self._textCtrl.GetValue()

    def SetValue(self, value):
        self._textCtrl.SetValue(value)


class HyperlinkDialog(wx.Dialog):
    """Dialog for displaying message with hyperlink."""

    def __init__(
        self, parent, title, message, hyperlink, hyperlinkLabel=None, style=wx.OK
    ):
        """Constructor

        :param parent: gui parent
        :param title: dialog title
        :param message: message
        :param hyperlink: url
        :param hyperlinkLabel: label shown instead of url
        :param style: button style
        """
        wx.Dialog.__init__(
            self,
            parent=parent,
            id=wx.ID_ANY,
            title=title,
            style=wx.DEFAULT_DIALOG_STYLE,
        )

        sizer = wx.BoxSizer(wx.VERTICAL)

        label = StaticText(self, label=message)
        sizer.Add(label, proportion=0, flag=wx.ALIGN_CENTRE | wx.ALL, border=10)
        hyperlinkLabel = hyperlinkLabel or hyperlink
        hyperlinkCtrl = HyperlinkCtrl(
            self,
            id=wx.ID_ANY,
            label=hyperlinkLabel,
            url=hyperlink,
            style=HyperlinkCtrl.HL_ALIGN_LEFT | HyperlinkCtrl.HL_CONTEXTMENU,
        )
        sizer.Add(hyperlinkCtrl, proportion=0, flag=wx.EXPAND | wx.ALL, border=10)

        btnsizer = self.CreateSeparatedButtonSizer(style)
        sizer.Add(btnsizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=10)

        self.SetSizer(sizer)
        sizer.Fit(self)


class QuitDialog(wx.Dialog):
    def __init__(
        self,
        parent,
        title=_("Quit GRASS GIS"),
        id=wx.ID_ANY,
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        **kwargs,
    ):
        """Dialog to quit GRASS

        :param parent: window
        """
        wx.Dialog.__init__(self, parent, id, title, style=style, **kwargs)
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self._icon = wx.StaticBitmap(
            self.panel,
            wx.ID_ANY,
            wx.ArtProvider().GetBitmap(wx.ART_QUESTION, client=wx.ART_MESSAGE_BOX),
        )

        self._shell_running = is_shell_running()

        if self._shell_running:
            text = _(
                "Do you want to quit GRASS GIS including shell or just close the GUI?"
            )
        else:
            text = _("Do you want to quit GRASS GIS?")
        self.informLabel = StaticText(parent=self.panel, id=wx.ID_ANY, label=text)
        self.btnCancel = Button(parent=self.panel, id=wx.ID_CANCEL)
        if self._shell_running:
            self.btnClose = Button(parent=self.panel, id=wx.ID_NO, label=_("Close GUI"))
            self.btnClose.Bind(wx.EVT_BUTTON, self.OnClose)
        self.btnQuit = Button(
            parent=self.panel, id=wx.ID_YES, label=_("Quit GRASS GIS")
        )
        self.btnQuit.SetFocus()
        self.btnQuit.Bind(wx.EVT_BUTTON, self.OnQuit)

        self.__layout()

    def __layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnCancel, flag=wx.RIGHT, border=5)
        if self._shell_running:
            btnSizer.Add(self.btnClose, flag=wx.RIGHT, border=5)
        btnSizer.Add(self.btnQuit, flag=wx.RIGHT, border=5)

        bodySizer = wx.BoxSizer(wx.HORIZONTAL)
        bodySizer.Add(self._icon, flag=wx.RIGHT, border=10)
        bodySizer.Add(self.informLabel, proportion=1, flag=wx.EXPAND)

        sizer.Add(bodySizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=15)
        sizer.Add(btnSizer, proportion=0, flag=wx.ALL | wx.ALIGN_RIGHT, border=5)

        self.panel.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()

    def OnClose(self, event):
        self.EndModal(wx.ID_NO)

    def OnQuit(self, event):
        self.EndModal(wx.ID_YES)


class DefaultFontDialog(wx.Dialog):
    """
    Opens a file selection dialog to select default font
    to use in all GRASS displays
    """

    def __init__(
        self,
        parent,
        title,
        id=wx.ID_ANY,
        style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER,
        settings=UserSettings,
        type="font",
    ):
        self.settings = settings
        self.type = type

        wx.Dialog.__init__(self, parent, id, title, style=style)

        panel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.tmp_file = grass.tempfile(False) + ".png"

        self.fontdict, fontdict_reverse, self.fontlist = self.GetFonts()

        border = wx.BoxSizer(wx.VERTICAL)
        box = StaticBox(parent=panel, id=wx.ID_ANY, label=" %s " % _("Font settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer(hgap=5, vgap=5)

        label = StaticText(parent=panel, id=wx.ID_ANY, label=_("Select font:"))
        gridSizer.Add(label, flag=wx.ALIGN_TOP, pos=(0, 0))

        self.fontlb = wx.ListBox(
            parent=panel,
            id=wx.ID_ANY,
            pos=wx.DefaultPosition,
            choices=self.fontlist,
            style=wx.LB_SINGLE,
        )
        self.Bind(wx.EVT_LISTBOX, self.EvtListBox, self.fontlb)
        self.Bind(wx.EVT_LISTBOX_DCLICK, self.EvtListBoxDClick, self.fontlb)

        gridSizer.Add(self.fontlb, flag=wx.EXPAND, pos=(1, 0))

        self.renderfont = wx.StaticBitmap(
            panel, -1, wx.Bitmap.FromRGBA(100, 50, 255, 255, 255)
        )
        gridSizer.Add(self.renderfont, flag=wx.EXPAND, pos=(2, 0))

        if self.type == "font":
            if "GRASS_FONT" in os.environ:
                self.font = os.environ["GRASS_FONT"]
            else:
                self.font = self.settings.Get(
                    group="display", key="font", subkey="type"
                )
            self.encoding = self.settings.Get(
                group="display", key="font", subkey="encoding"
            )

            label = StaticText(
                parent=panel, id=wx.ID_ANY, label=_("Character encoding:")
            )
            gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 0))

            self.textentry = TextCtrl(parent=panel, id=wx.ID_ANY, value=self.encoding)
            gridSizer.Add(self.textentry, flag=wx.EXPAND, pos=(4, 0))

            self.textentry.Bind(wx.EVT_TEXT, self.OnEncoding)

        elif self.type == "outputfont":
            self.font = self.settings.Get(
                group="appearance", key="outputfont", subkey="type"
            )
            self.fontsize = self.settings.Get(
                group="appearance", key="outputfont", subkey="size"
            )
            label = StaticText(parent=panel, id=wx.ID_ANY, label=_("Font size:"))
            gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 0))

            self.spin = SpinCtrl(parent=panel, id=wx.ID_ANY)
            if self.fontsize:
                self.spin.SetValue(int(self.fontsize))
            self.spin.Bind(wx.EVT_SPINCTRL, self.OnSizeSpin)
            self.spin.Bind(wx.EVT_TEXT, self.OnSizeSpin)
            gridSizer.Add(self.spin, flag=wx.ALIGN_CENTER_VERTICAL, pos=(4, 0))

        else:
            return

        if self.font:
            long_name = fontdict_reverse.get(self.font, None)
            if long_name:
                self.fontlb.SetStringSelection(long_name, True)
            else:
                # font is not in the list of GRASS recognized fonts
                self.font = None

        gridSizer.AddGrowableCol(0)
        sizer.Add(gridSizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)

        border.Add(sizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=3)

        btnsizer = wx.StdDialogButtonSizer()

        btn = Button(parent=panel, id=wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = Button(parent=panel, id=wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        border.Add(btnsizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)

        panel.SetAutoLayout(True)
        panel.SetSizer(border)
        border.Fit(self)
        row, col = gridSizer.GetItemPosition(self.renderfont)
        self.renderfont.SetSize(gridSizer.GetCellSize(row, col))
        if self.font:
            self.RenderText(self.font, _("Example"), size=self.renderfont.GetSize())

        self.Layout()

    def OnEncoding(self, event):
        self.encoding = event.GetString()

    def EvtListBox(self, event):
        self.font = self.fontdict[event.GetString()]
        self.RenderText(self.font, "Example", size=self.renderfont.GetSize())
        event.Skip()

    def EvtListBoxDClick(self, event):
        self.font = self.fontdict[event.GetString()]
        event.Skip()

    def OnSizeSpin(self, event):
        self.fontsize = self.spin.GetValue()
        event.Skip()

    def GetFonts(self):
        """
        parses fonts directory or fretypecap file to get a list of fonts
        for the listbox
        """
        fontlist = []
        fontdict = {}
        fontdict_reverse = {}
        env = os.environ.copy()
        driver = UserSettings.Get(group="display", key="driver", subkey="type")
        if driver == "png":
            env["GRASS_RENDER_IMMEDIATE"] = "png"
        else:
            env["GRASS_RENDER_IMMEDIATE"] = "cairo"
        ret = RunCommand("d.fontlist", flags="v", read=True, env=env)
        if not ret:
            return fontlist

        dfonts = ret.splitlines()
        for line in dfonts:
            shortname = line.split("|")[0]
            longname = line.split("|")[1]
            # not sure when this happens?
            if shortname.startswith("#"):
                continue
            fontlist.append(longname)
            fontdict[longname] = shortname
            fontdict_reverse[shortname] = longname
        fontlist = naturally_sorted(list(set(fontlist)))

        return fontdict, fontdict_reverse, fontlist

    def RenderText(self, font, text, size):
        """Renders an example text with the selected font and resets the bitmap
        widget"""
        env = os.environ.copy()
        driver = UserSettings.Get(group="display", key="driver", subkey="type")
        if driver == "png":
            env["GRASS_RENDER_IMMEDIATE"] = "png"
        else:
            env["GRASS_RENDER_IMMEDIATE"] = "cairo"
        env["GRASS_RENDER_WIDTH"] = str(size[0])
        env["GRASS_RENDER_HEIGHT"] = str(size[1])
        env["GRASS_RENDER_FILE"] = self.tmp_file
        env["GRASS_REGION"] = grass.region_env(s=0, n=size[1], w=0, e=size[0])
        ret = RunCommand(
            "d.text",
            text=text,
            font=font,
            align="cc",
            at="50,60",
            size=80,
            color="black",
            env=env,
        )
        if ret == 0:
            self.renderfont.SetBitmap(wx.Bitmap(self.tmp_file))
        else:
            self.renderfont.SetBitmap(EmptyBitmap(size[0], size[1]))
        try_remove(self.tmp_file)
