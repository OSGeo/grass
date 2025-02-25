"""
@package location_wizard.wizard

@brief Location wizard - creates a new GRASS Location. User can choose
from multiple methods.

Classes:
 - wizard::TitledPage
 - wizard::GridBagSizerTitledPage
 - wizard::DatabasePage
 - wizard::CoordinateSystemPage
 - wizard::ProjectionsPage
 - wizard::ItemList
 - wizard::ProjParamsPage
 - wizard::DatumPage
 - wizard::EllipsePage
 - wizard::GeoreferencedFilePage
 - wizard::WKTPage
 - wizard::EPSGPage
 - wizard::IAUPage
 - wizard::CustomPage
 - wizard::SummaryPage
 - wizard::LocationWizard
 - wizard::WizardWithHelpButton

(C) 2007-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Hamish Bowman (planetary ellipsoids)
"""

from __future__ import annotations

import functools
import locale
import os
from pathlib import Path
from typing import TYPE_CHECKING

import wx
import wx.lib.mixins.listctrl as listmix
from core import globalvar

if globalvar.wxPythonPhoenix or TYPE_CHECKING:
    from wx import adv as wiz
    from wx.adv import Wizard, WizardPageSimple
else:
    from wx import wizard as wiz
    from wx.wizard import Wizard
    from wx.wizard import WizardPageSimple

import wx.lib.scrolledpanel as scrolled
from core import utils
from core.gcmd import GError, GWarning, RunCommand
from core.utils import cmp
from gui_core.widgets import GenericMultiValidator
from gui_core.wrap import (
    Button,
    CheckBox,
    HyperlinkCtrl,
    ListCtrl,
    NewId,
    SearchCtrl,
    SpinCtrl,
    StaticBox,
    StaticText,
    TextCtrl,
)
from location_wizard.dialogs import SelectTransformDialog

from grass.exceptions import OpenError, ScriptError
from grass.grassdb.checks import location_exists
from grass.script import core as grass
from grass.script import decode

global coordsys, north, south, east, west, resolution, wizerror, translist

if globalvar.CheckWxVersion(version=[4, 1, 0]):
    search_cancel_evt = wx.EVT_SEARCH_CANCEL
else:
    search_cancel_evt = wx.EVT_SEARCHCTRL_CANCEL_BTN

if TYPE_CHECKING:
    from wx.adv import WizardEvent


class TitledPage(WizardPageSimple):
    """Class to make wizard pages. Generic methods to make labels,
    text entries, and buttons.
    """

    def __init__(self, parent, title):
        self.page = WizardPageSimple.__init__(self, parent)

        font = wx.Font(13, wx.SWISS, wx.NORMAL, wx.BOLD)
        font_height = font.GetPixelSize()[1]

        # page title
        self.title = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=title,
            style=wx.ALIGN_CENTRE_HORIZONTAL,
            size=(-1, font_height),
        )
        self.title.SetFont(font)
        # main sizers
        self.pagesizer = wx.BoxSizer(wx.VERTICAL)

    def DoLayout(self):
        """Do page layout"""
        self.pagesizer.Add(self.title, proportion=0, flag=wx.EXPAND | wx.ALL, border=5)
        self.pagesizer.Add(
            wx.StaticLine(self, -1), proportion=0, flag=wx.EXPAND | wx.ALL, border=0
        )
        self.pagesizer.Add(self.sizer, proportion=1, flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(self.pagesizer)
        self.Layout()

    def MakeLabel(self, text="", style=wx.ALIGN_LEFT, parent=None, tooltip=None):
        """Make aligned label"""
        if not parent:
            parent = self
        label = StaticText(parent=parent, id=wx.ID_ANY, label=text, style=style)
        if tooltip:
            label.SetToolTip(tooltip)
        return label

    def MakeTextCtrl(self, text="", size=(100, -1), style=0, parent=None, tooltip=None):
        """Generic text control"""
        if not parent:
            parent = self
        textCtrl = TextCtrl(
            parent=parent, id=wx.ID_ANY, value=text, size=size, style=style
        )
        if tooltip:
            textCtrl.SetToolTip(tooltip)
        return textCtrl

    def MakeButton(self, text, id=wx.ID_ANY, size=(-1, -1), parent=None, tooltip=None):
        """Generic button"""
        if not parent:
            parent = self
        button = Button(parent=parent, id=id, label=text, size=size)
        if tooltip:
            button.SetToolTip(tooltip)
        return button

    def MakeCheckBox(
        self, text, id=wx.ID_ANY, size=(-1, -1), parent=None, tooltip=None
    ):
        """Generic checkbox"""
        if not parent:
            parent = self
        chbox = CheckBox(parent=parent, id=id, label=text, size=size)
        if tooltip:
            chbox.SetToolTip(tooltip)
        return chbox


class GridBagSizerTitledPage(TitledPage):
    """GridBagSizer declaration for TitledPage class"""

    def __init__(self, parent, title):
        super().__init__(parent, title)

        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)


class DatabasePage(TitledPage):
    """Wizard page for setting GIS data directory and location name"""

    def __init__(self, wizard, parent, grassdatabase):
        TitledPage.__init__(self, wizard, _("Define new GRASS project"))

        # grid definition
        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)
        self.sizer.AddGrowableCol(1)

        # definition of variables
        self.grassdatabase = grassdatabase
        self.location = ""
        self.locTitle = ""

        # browse button
        self.bbrowse = self.MakeButton(_("Change"))

        # text controls
        self.tgisdbase = self.MakeLabel(grassdatabase)
        self.tlocation = self.MakeTextCtrl("newProject")
        self.tlocation.SetFocus()

        checks = [
            (grass.legal_name, self._nameValidationFailed),
            (self._checkLocationNotExists, self._locationAlreadyExists),
        ]
        self.tlocation.SetValidator(GenericMultiValidator(checks))
        self.tlocTitle = self.MakeTextCtrl()

        # text for required options
        required_txt = self.MakeLabel("*")
        required_txt.SetForegroundColour(wx.RED)
        required_txt.SetToolTip(_("This option is required"))

        # text for optional options
        optional_txt = self.MakeLabel(_("(optional)"))
        italics = wx.Font(10, wx.DEFAULT, wx.ITALIC, wx.NORMAL)
        optional_txt.SetFont(italics)
        optional_txt.SetForegroundColour(
            wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT)
        )

        # layout
        self.sizer.Add(
            self.MakeLabel(
                "%s:" % _("Name"),
                tooltip=_("Name of project directory in GIS Data Directory"),
            ),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 1),
        )
        self.sizer.Add(
            self.tlocation,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND,
            border=5,
            pos=(2, 1),
        )
        self.sizer.Add(
            required_txt,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(2, 2),
        )

        self.sizer.Add(
            self.MakeLabel(
                "%s:" % _("Description"),
                tooltip=_("Description of project directory in GIS Data Directory"),
            ),
            flag=wx.ALIGN_LEFT | wx.ALIGN_TOP | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(3, 1),
        )
        self.sizer.Add(
            self.tlocTitle,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL | wx.EXPAND,
            border=5,
            pos=(4, 1),
        )
        self.sizer.Add(
            optional_txt,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(4, 2),
        )

        self.sizer.Add(
            self.MakeLabel(_("Project will be created in GRASS database:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=2,
            pos=(5, 1),
        )
        self.sizer.Add(
            self.tgisdbase,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(6, 1),
        )
        self.sizer.Add(
            self.bbrowse,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(6, 2),
        )

        # bindings
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.tgisdbase.Bind(wx.EVT_TEXT, self.OnChangeName)
        self.tlocation.Bind(wx.EVT_TEXT, self.OnChangeName)
        self.Bind(wx.EVT_BUTTON, self.OnBrowse, self.bbrowse)

    def _nameValidationFailed(self, ctrl):
        message = _(
            "Name '{}' is not a valid name for project. "
            "Please use only ASCII characters excluding characters {} "
            "and space."
        ).format(ctrl.GetValue(), "/\"'@,=*~")
        GError(parent=self, message=message, caption=_("Invalid name"))

    def _checkLocationNotExists(self, text) -> bool:
        """Check whether user's input location exists or not."""
        return not location_exists(self.tgisdbase.GetLabel(), text)

    def _locationAlreadyExists(self, ctrl):
        message = _(
            "Project '{}' already exists. Please consider using "
            "another name for your project."
        ).format(ctrl.GetValue())
        GError(parent=self, message=message, caption=_("Existing project path"))

    def OnChangeName(self, event):
        """Name for new location was changed"""
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(event.GetString()) > 0:
            if not nextButton.IsEnabled():
                nextButton.Enable()
        else:
            nextButton.Disable()

        event.Skip()

    def OnBrowse(self, event):
        """Choose GRASS data directory"""
        dlg = wx.DirDialog(
            self,
            _("Choose GRASS data directory:"),
            str(Path.cwd()),
            wx.DD_DEFAULT_STYLE,
        )
        if dlg.ShowModal() == wx.ID_OK:
            self.grassdatabase = dlg.GetPath()
            self.tgisdbase.SetLabel(self.grassdatabase)

        dlg.Destroy()

    def OnPageChanging(self, event: WizardEvent | None = None) -> None:
        self.location = self.tlocation.GetValue()
        self.grassdatabase = self.tgisdbase.GetLabel()
        self.locTitle = self.tlocTitle.GetValue()
        if os.linesep in self.locTitle or len(self.locTitle) > 255:
            GWarning(
                parent=self,
                message=_(
                    "Title of the project is limited only to one line and "
                    "256 characters. The rest of the text will be ignored."
                ),
            )
            self.locTitle = self.locTitle.split(os.linesep)[0][:255]


class CoordinateSystemPage(TitledPage):
    """Wizard page for choosing method for location creation"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select Coordinate Reference System (CRS)"))

        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        self.parent = parent
        global coordsys

        # toggles
        self.radioEpsg = wx.RadioButton(
            parent=self,
            id=wx.ID_ANY,
            label=_("Select CRS from a list by EPSG or description"),
            style=wx.RB_GROUP,
        )
        # self.radioIau = wx.RadioButton(
        #    parent=self, id=wx.ID_ANY,
        #    label=_("Select IAU code of spatial reference system"))
        self.radioFile = wx.RadioButton(
            parent=self,
            id=wx.ID_ANY,
            label=_("Read CRS from a georeferenced data file"),
        )
        self.radioXy = wx.RadioButton(
            parent=self,
            id=wx.ID_ANY,
            label=_("Create a generic cartesian coordinate system (XY)"),
        )
        self.radioWkt = wx.RadioButton(
            parent=self, id=wx.ID_ANY, label=_("Specify CRS using WKT string")
        )
        self.radioProj = wx.RadioButton(
            parent=self, id=wx.ID_ANY, label=_("Specify CRS using PROJ.4 string")
        )
        self.radioSrs = wx.RadioButton(
            parent=self, id=wx.ID_ANY, label=_("Define custom CRS")
        )

        # layout
        self.sizer.SetVGap(10)
        self.sizer.Add(self.radioEpsg, flag=wx.ALIGN_LEFT, pos=(1, 1))
        # self.sizer.Add(self.radioIau,
        #               flag=wx.ALIGN_LEFT, pos=(1, 1))
        self.sizer.Add(self.radioFile, flag=wx.ALIGN_LEFT, pos=(2, 1))
        self.sizer.Add(self.radioXy, flag=wx.ALIGN_LEFT, pos=(3, 1))
        self.sizer.Add(
            StaticText(parent=self, label=_("Additional methods:")),
            flag=wx.ALIGN_LEFT,
            pos=(4, 1),
        )
        self.sizer.Add(self.radioWkt, flag=wx.ALIGN_LEFT, pos=(5, 1))
        self.sizer.Add(self.radioProj, flag=wx.ALIGN_LEFT, pos=(6, 1))
        self.sizer.Add(self.radioSrs, flag=wx.ALIGN_LEFT, pos=(7, 1))
        self.sizer.AddGrowableCol(1)

        # bindings
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioEpsg.GetId())
        # self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioIau.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioFile.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioWkt.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioSrs.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioProj.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioXy.GetId())
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event: WizardEvent) -> None:
        global coordsys

        if not coordsys:
            coordsys = "epsg"
            self.radioEpsg.SetValue(True)
        else:
            if coordsys == "proj":
                self.radioSrs.SetValue(True)
            if coordsys == "epsg":
                self.radioEpsg.SetValue(True)
            # if coordsys == "iau":
            #    self.radioIau.SetValue(True)
            if coordsys == "file":
                self.radioFile.SetValue(True)
            if coordsys == "wkt":
                self.radioWkt.SetValue(True)
            if coordsys == "custom":
                self.radioProj.SetValue(True)
            if coordsys == "xy":
                self.radioXy.SetValue(True)

        if event.GetDirection():
            if coordsys == "proj":
                self.SetNext(self.parent.projpage)
                self.parent.sumpage.SetPrev(self.parent.datumpage)
            if coordsys == "epsg":
                self.SetNext(self.parent.epsgpage)
                self.parent.sumpage.SetPrev(self.parent.epsgpage)
            # if coordsys == "iau":
            #    self.SetNext(self.parent.iaupage)
            #    self.parent.sumpage.SetPrev(self.parent.iaupage)
            if coordsys == "file":
                self.SetNext(self.parent.filepage)
                self.parent.sumpage.SetPrev(self.parent.filepage)
            if coordsys == "wkt":
                self.SetNext(self.parent.wktpage)
                self.parent.sumpage.SetPrev(self.parent.wktpage)
            if coordsys == "custom":
                self.SetNext(self.parent.custompage)
                self.parent.sumpage.SetPrev(self.parent.custompage)
            if coordsys == "xy":
                self.SetNext(self.parent.sumpage)
                self.parent.sumpage.SetPrev(self.parent.csystemspage)

        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable()

    def SetVal(self, event):
        """Choose method"""
        global coordsys
        if event.GetId() == self.radioSrs.GetId():
            coordsys = "proj"
            self.SetNext(self.parent.projpage)
            self.parent.sumpage.SetPrev(self.parent.datumpage)
        # elif event.GetId() == self.radioIau.GetId():
        #    coordsys = "iau"
        #    self.SetNext(self.parent.iaupage)
        #    self.parent.sumpage.SetPrev(self.parent.iaupage)
        elif event.GetId() == self.radioEpsg.GetId():
            coordsys = "epsg"
            self.SetNext(self.parent.epsgpage)
            self.parent.sumpage.SetPrev(self.parent.epsgpage)
        elif event.GetId() == self.radioFile.GetId():
            coordsys = "file"
            self.SetNext(self.parent.filepage)
            self.parent.sumpage.SetPrev(self.parent.filepage)
        elif event.GetId() == self.radioWkt.GetId():
            coordsys = "wkt"
            self.SetNext(self.parent.wktpage)
            self.parent.sumpage.SetPrev(self.parent.wktpage)
        elif event.GetId() == self.radioProj.GetId():
            coordsys = "custom"
            self.SetNext(self.parent.custompage)
            self.parent.sumpage.SetPrev(self.parent.custompage)
        elif event.GetId() == self.radioXy.GetId():
            coordsys = "xy"
            self.SetNext(self.parent.sumpage)
            self.parent.sumpage.SetPrev(self.parent.csystemspage)


class ProjectionsPage(TitledPage):
    """Wizard page for defining custom CRS"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Define custom CRS"))

        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        self.parent = parent
        self.proj = ""
        self.projdesc = ""
        self.p4proj = ""

        # text input
        self.tproj = self.MakeTextCtrl("", size=(200, -1))

        # search box
        self.searchb = SearchCtrl(self, size=(200, -1), style=wx.TE_PROCESS_ENTER)
        self.searchb.ShowCancelButton(True)

        # projection list
        self.projlist = ItemList(
            self,
            data=list(self.parent.projdesc.items()),
            columns=[_("Code"), _("Description")],
        )
        self.projlist.resizeLastColumn(30)

        # layout
        self.sizer.Add(
            self.MakeLabel(_("Projection code:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 1),
        )
        self.sizer.Add(
            self.tproj, flag=wx.ALIGN_RIGHT | wx.EXPAND | wx.ALL, border=5, pos=(1, 2)
        )

        self.sizer.Add(
            self.MakeLabel(_("Search in description:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(2, 1),
        )
        self.sizer.Add(
            self.searchb, flag=wx.ALIGN_RIGHT | wx.EXPAND | wx.ALL, border=5, pos=(2, 2)
        )

        self.sizer.Add(
            self.projlist,
            flag=wx.EXPAND | wx.ALIGN_LEFT | wx.ALL,
            border=5,
            pos=(3, 1),
            span=(1, 3),
        )
        self.sizer.AddGrowableCol(3)
        self.sizer.AddGrowableRow(3)

        # events
        self.tproj.Bind(wx.EVT_TEXT, self.OnText)
        self.searchb.Bind(wx.EVT_TEXT, self.OnSearch)
        self.searchb.Bind(search_cancel_evt, self.OnSearchCancel)
        self.projlist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnPageChanging(self, event: WizardEvent) -> None:
        if event.GetDirection() and self.proj not in self.parent.projections.keys():
            event.Veto()

    def OnText(self, event):
        """Projection name changed"""
        self.proj = event.GetString().lower()
        self.p4proj = ""
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if self.proj not in self.parent.projections.keys() and nextButton.IsEnabled():
            nextButton.Enable(False)

        if self.proj in self.parent.projections.keys():
            if self.proj == "stp":
                wx.MessageBox(
                    "Currently State Plane projections must be selected using the "
                    "text-based setup (g.setproj), or entered by EPSG code or "
                    "custom PROJ.4 terms.",
                    "Warning",
                    wx.ICON_WARNING,
                )
                self.proj = ""
                self.tproj.SetValue(self.proj)
                nextButton.Enable(False)
                return
            if self.proj.lower() == "ll":
                self.p4proj = "+proj=longlat"
            else:
                self.p4proj = "+proj=" + self.proj.lower()
            self.projdesc = self.parent.projections[self.proj][0]
            nextButton.Enable()

    def OnEnterPage(self, event: WizardEvent) -> None:
        if len(self.proj) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnSearch(self, event):
        """Search projection by desc"""
        search_str = event.GetString()
        try:
            self.proj, self.projdesc = self.projlist.Search(
                index=[0, 1], pattern=search_str
            )
        except (IndexError, ValueError):
            self.proj = self.projdesc = ""

        event.Skip()

    def OnSearchCancel(self, event):
        self.projlist.Search(index=None, pattern="")
        event.Skip()

    def OnItemSelected(self, event):
        """Projection selected"""
        index = event.GetIndex()

        # set values
        self.proj = self.projlist.GetItem(index, 0).GetText().lower()
        self.tproj.SetValue(self.proj)

        event.Skip()


class ItemList(ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.ColumnSorterMixin):
    """Generic list (for projections, ellipsoids, etc.)"""

    def __init__(self, parent, columns, data=None):
        ListCtrl.__init__(
            self,
            parent=parent,
            id=wx.ID_ANY,
            style=wx.LC_REPORT
            | wx.LC_VIRTUAL
            | wx.LC_HRULES
            | wx.LC_VRULES
            | wx.LC_SINGLE_SEL
            | wx.LC_SORT_ASCENDING,
            size=(550, 125),
        )

        # original data or None
        self.sourceData = data

        #
        # insert columns
        #
        i = 0
        for column in columns:
            self.InsertColumn(i, column)
            i += 1

        self.EnableAlternateRowColours()

        if self.sourceData:
            self.Populate()

        for i in range(self.GetColumnCount()):
            self.SetColumnWidth(i, wx.LIST_AUTOSIZE_USEHEADER)
            if self.GetColumnWidth(i) < 80:
                self.SetColumnWidth(i, 80)

        #
        # listmix
        #
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        listmix.ColumnSorterMixin.__init__(self, self.GetColumnCount())

        self.il = wx.ImageList(16, 16)
        self.sm_up = self.il.Add(
            wx.ArtProvider.GetBitmap(wx.ART_GO_UP, wx.ART_TOOLBAR, (16, 16))
        )
        self.sm_dn = self.il.Add(
            wx.ArtProvider.GetBitmap(wx.ART_GO_DOWN, wx.ART_TOOLBAR, (16, 16))
        )
        self.SetImageList(self.il, wx.IMAGE_LIST_SMALL)

        #
        # sort by first column
        #
        if self.sourceData:
            self.SortListItems(col=0, ascending=True)

    def Populate(self, data=None, update=False):
        """Populate and sort list.
        Returns sorted list."""
        self.itemDataMap = {}
        self.itemIndexMap = []

        if data is None:
            data = self.sourceData
        elif update:
            self.sourceData = data

        try:
            data = sorted(data)
            self.DeleteAllItems()
            row = 0
            for value in data:
                self.itemDataMap[row] = [value[0]]
                for i in range(1, len(value)):
                    self.itemDataMap[row].append(value[i])
                self.itemIndexMap.append(row)
                row += 1

            self.SetItemCount(row)

            # set column width
            self.SetColumnWidth(0, 80)
            self.SetColumnWidth(1, 300)

            self.SendSizeEvent()
            return data

        except Exception as e:
            wx.MessageBox(
                parent=self,
                message=_("Unable to read list: %s") % e,
                caption=_("Error"),
                style=wx.OK | wx.ICON_ERROR,
            )

    def GetSortImages(self):
        """Used by the ColumnSorterMixin, see wx/lib/mixins/listctrl.py"""
        return (self.sm_dn, self.sm_up)

    def OnGetItemText(self, item, col):
        """Get item text"""
        index = self.itemIndexMap[item]
        return str(self.itemDataMap[index][col])

    def OnGetItemImage(self, item):
        return -1

    def SortItems(self, sorter=cmp):
        """Sort items"""
        items = list(self.itemDataMap.keys())
        items.sort(key=functools.cmp_to_key(self.Sorter))
        self.itemIndexMap = items

        # redraw the list
        self.Refresh()

    def Sorter(self, key1, key2):
        ascending = self._colSortFlag[self._col]
        # convert always string
        item1 = self.itemDataMap[key1][self._col]
        item2 = self.itemDataMap[key2][self._col]

        if isinstance(item1, str) or isinstance(item2, str):
            cmpVal = locale.strcoll(str(item1), str(item2))
        else:
            cmpVal = cmp(item1, item2)

        # If the items are equal then pick something else to make the sort
        # value unique
        if cmpVal == 0:
            cmpVal = cmp(*self.GetSecondarySortValues(self._col, key1, key2))

        if ascending:
            return cmpVal
        return -cmpVal

    def GetListCtrl(self):
        """Used by listmix.ColumnSorterMixin"""
        return self

    def Search(self, index, pattern, firstOnly=True):
        """Search projection by description
        Return first found item (or None) if firstOnly is True,
        all data (or empty list) if False
        """
        if pattern == "":
            self.Populate(self.sourceData)
            return None if firstOnly else []

        data = []
        pattern = pattern.lower()
        for i in range(len(self.sourceData)):
            for idx in index:
                try:
                    value = str(self.sourceData[i][idx]).lower()
                    if pattern in value:
                        data.append(self.sourceData[i])
                        break
                except UnicodeDecodeError:
                    # osgeo4w problem (should be fixed)
                    pass

        data = self.Populate(data)
        if len(data) > 0:
            if firstOnly:
                return data[0]
            return data
        if firstOnly:
            return None
        return []


class ProjParamsPage(TitledPage):
    """Wizard page for selecting method of setting coordinate system
    parameters (select coordinate system option)
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose projection parameters"))
        global coordsys

        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        self.parent = parent
        self.panel = None
        self.prjParamSizer = None

        self.pparam = {}

        self.p4projparams = ""
        self.projdesc = ""

        radioSBox = StaticBox(
            parent=self,
            id=wx.ID_ANY,
            label=" %s " % _("Select datum or ellipsoid (next page)"),
        )
        radioSBSizer = wx.StaticBoxSizer(radioSBox)
        self.sizer.Add(
            radioSBSizer, pos=(0, 1), flag=wx.EXPAND | wx.ALIGN_TOP | wx.TOP, border=10
        )
        self.sizer.AddGrowableCol(1)

        self.radio1 = wx.RadioButton(
            parent=self,
            id=wx.ID_ANY,
            label=_("Datum with associated ellipsoid"),
            style=wx.RB_GROUP,
        )
        self.radioEpsg = wx.RadioButton(
            parent=self, id=wx.ID_ANY, label=_("Ellipsoid only")
        )

        # default button setting
        if self.radio1.GetValue() is False and self.radioEpsg.GetValue() is False:
            self.radio1.SetValue(True)
            self.SetNext(self.parent.datumpage)
            #            self.parent.sumpage.SetPrev(self.parent.datumpage)

        radioSBSizer.Add(self.radio1, flag=wx.ALIGN_LEFT | wx.RIGHT, border=20)
        radioSBSizer.Add(self.radioEpsg, flag=wx.ALIGN_LEFT)

        # bindings
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio1.GetId())
        self.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioEpsg.GetId())
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChange)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnParamEntry(self, event):
        """Parameter value changed"""
        id = event.GetId()
        val = event.GetString()

        if id not in self.pparam:
            event.Skip()
            return

        param = self.pparam[id]
        win = self.FindWindowById(id)
        if param["type"] == "zone":
            val = self.FindWindowById(id).GetValue()
            if val < 1:
                win.SetValue(1)
            elif val > 60:
                win.SetValue(60)

        if param["type"] == "bool":
            param["value"] = event.GetSelection()
        else:
            param["value"] = val

        event.Skip()

    def OnPageChange(self, event: WizardEvent | None = None) -> None:
        """Go to next page"""
        if event.GetDirection():
            self.p4projparams = ""
            for param in self.pparam.values():
                if param["type"] == "bool":
                    if param["value"] is False:
                        continue
                    self.p4projparams += " +" + param["proj4"]
                elif param["value"] is None:
                    wx.MessageBox(
                        parent=self,
                        message=_("You must enter a value for %s") % param["desc"],
                        caption=_("Error"),
                        style=wx.ICON_ERROR | wx.CENTRE,
                    )
                    event.Veto()
                else:
                    self.p4projparams += (
                        " +" + param["proj4"] + "=" + str(param["value"])
                    )

    def OnEnterPage(self, event: WizardEvent) -> None:
        """Page entered"""
        self.projdesc = self.parent.projections[self.parent.projpage.proj][0]
        if self.prjParamSizer is None:
            # entering page for the first time
            self.paramSBox = StaticBox(
                parent=self,
                id=wx.ID_ANY,
                label=_(" Enter parameters for %s projection ") % self.projdesc,
            )
            paramSBSizer = wx.StaticBoxSizer(self.paramSBox)

            self.panel = scrolled.ScrolledPanel(parent=self, id=wx.ID_ANY)
            self.panel.SetupScrolling()

            self.prjParamSizer = wx.GridBagSizer(vgap=0, hgap=0)

            self.sizer.Add(paramSBSizer, pos=(1, 1), flag=wx.EXPAND)
            self.sizer.AddGrowableRow(1)
            paramSBSizer.Add(self.panel, proportion=1, flag=wx.EXPAND)

            paramSBSizer.Fit(self.panel)
            self.panel.SetSizer(self.prjParamSizer)

        if event.GetDirection():
            self.prjParamSizer.Clear(True)
            self.paramSBox.SetLabel(
                _(" Enter parameters for %s projection ") % self.projdesc
            )
            self.pparam = {}
            row = 0
            for paramgrp in self.parent.projections[self.parent.projpage.proj][1]:
                # get parameters
                id = NewId()
                param = self.pparam[id] = {
                    "type": self.parent.paramdesc[paramgrp[0]][0],
                    "proj4": self.parent.paramdesc[paramgrp[0]][1],
                    "desc": self.parent.paramdesc[paramgrp[0]][2],
                }

                # default values
                if param["type"] == "bool":
                    param["value"] = 0
                elif param["type"] == "zone":
                    param["value"] = 30
                    param["desc"] += " (1-60)"
                else:
                    param["value"] = paramgrp[2]

                label = StaticText(
                    parent=self.panel,
                    id=wx.ID_ANY,
                    label=param["desc"],
                    style=wx.ALIGN_RIGHT | wx.ST_NO_AUTORESIZE,
                )
                if param["type"] == "bool":
                    win = wx.Choice(
                        parent=self.panel,
                        id=id,
                        size=(100, -1),
                        choices=[_("No"), _("Yes")],
                    )
                    win.SetSelection(param["value"])
                    win.Bind(wx.EVT_CHOICE, self.OnParamEntry)
                elif param["type"] == "zone":
                    win = SpinCtrl(
                        parent=self.panel,
                        id=id,
                        size=(100, -1),
                        style=wx.SP_ARROW_KEYS | wx.SP_WRAP,
                        min=1,
                        max=60,
                    )
                    win.SetValue(param["value"])
                    win.Bind(wx.EVT_SPINCTRL, self.OnParamEntry)
                    win.Bind(wx.EVT_TEXT, self.OnParamEntry)
                else:
                    win = TextCtrl(
                        parent=self.panel, id=id, value=param["value"], size=(100, -1)
                    )
                    win.Bind(wx.EVT_TEXT, self.OnParamEntry)
                    if paramgrp[1] == "noask":
                        win.Enable(False)

                self.prjParamSizer.Add(
                    label,
                    pos=(row, 1),
                    flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.RIGHT,
                    border=5,
                )
                self.prjParamSizer.Add(
                    win,
                    pos=(row, 2),
                    flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
                    border=5,
                )
                row += 1

        self.panel.SetSize(self.panel.GetBestSize())
        self.panel.Layout()
        self.Layout()
        self.Update()

        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable()

        event.Skip()

    def SetVal(self, event):
        """Set value"""
        if event.GetId() == self.radio1.GetId():
            self.SetNext(self.parent.datumpage)
            self.parent.sumpage.SetPrev(self.parent.datumpage)
        elif event.GetId() == self.radioEpsg.GetId():
            self.SetNext(self.parent.ellipsepage)
            self.parent.sumpage.SetPrev(self.parent.ellipsepage)


class DatumPage(TitledPage):
    """Wizard page for selecting datum (with associated ellipsoid)
    and datum transformation parameters (select coordinate system option)
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Specify geodetic datum"))

        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        self.parent = parent
        self.datum = ""
        self.datumdesc = ""
        self.ellipse = ""
        self.datumparams = ""
        self.proj4params = ""

        # text input
        self.tdatum = self.MakeTextCtrl("", size=(200, -1))

        # search box
        self.searchb = SearchCtrl(self, size=(200, -1), style=wx.TE_PROCESS_ENTER)
        self.searchb.ShowCancelButton(True)

        # create list control for datum/elipsoid list
        data = [
            [key, datum[0], datum[1]] for (key, datum) in self.parent.datums.items()
        ]
        self.datumlist = ItemList(
            self, data=data, columns=[_("Code"), _("Ellipsoid"), _("Description")]
        )
        self.datumlist.resizeLastColumn(10)

        # layout
        self.sizer.Add(
            self.MakeLabel(_("Datum code:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 1),
        )
        self.sizer.Add(
            self.tdatum,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 2),
        )

        self.sizer.Add(
            self.MakeLabel(_("Search in description:")),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(2, 1),
        )
        self.sizer.Add(
            self.searchb,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(2, 2),
        )

        self.sizer.Add(
            self.datumlist,
            flag=wx.EXPAND | wx.ALIGN_LEFT | wx.ALL,
            border=5,
            pos=(3, 1),
            span=(1, 4),
        )
        self.sizer.AddGrowableCol(4)
        self.sizer.AddGrowableRow(3)

        # events
        self.datumlist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnDatumSelected)
        self.searchb.Bind(wx.EVT_TEXT, self.OnDSearch)
        self.searchb.Bind(search_cancel_evt, self.OnSearchCancel)
        self.tdatum.Bind(wx.EVT_TEXT, self.OnDText)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnPageChanging(self, event: WizardEvent):
        self.proj4params = ""
        proj = self.parent.projpage.p4proj

        if event.GetDirection():
            if self.datum not in self.parent.datums:
                event.Veto()
            else:
                # check for datum transforms
                #                proj4string = self.parent.CreateProj4String()
                #                + ' +datum=%s' % self.datum
                ret = RunCommand(
                    "g.proj",
                    read=True,
                    proj4="%s" % proj,
                    datum="%s" % self.datum,
                    datum_trans="-1",
                    flags="t",
                )
                #                wx.Messagebox('here')
                if ret != "":
                    dtrans = ""
                    # open a dialog to select datum transform number
                    dlg = SelectTransformDialog(self.parent.parent, transforms=ret)

                    if dlg.ShowModal() == wx.ID_OK:
                        dtrans = dlg.GetTransform()
                        if dtrans == "":
                            dlg.Destroy()
                            event.Veto()
                            return "Datum transform is required."
                    else:
                        dlg.Destroy()
                        event.Veto()
                        return "Datum transform is required."

                    self.parent.datum_trans = dtrans

            self.GetNext().SetPrev(self)
            self.parent.ellipsepage.ellipse = self.ellipse
            self.parent.ellipsepage.ellipseparams = self.parent.ellipsoids[
                self.ellipse
            ][1]

    def OnEnterPage(self, event: WizardEvent) -> None:
        self.parent.datum_trans = None
        if event.GetDirection():
            if len(self.datum) == 0:
                # disable 'next' button by default when entering from previous
                # page
                wx.FindWindowById(wx.ID_FORWARD).Enable(False)
            else:
                wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnDText(self, event):
        """Datum code changed"""
        self.datum = event.GetString()

        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.datum) == 0 or self.datum not in self.parent.datums:
            nextButton.Enable(False)
        else:
            self.ellipse = self.parent.datums[self.datum][0]
            self.datumdesc = self.parent.datums[self.datum][1]
            self.datumparams = self.parent.datums[self.datum][2]
            try:
                self.datumparams.remove("dx=0.0")
            except ValueError:
                pass
            try:
                self.datumparams.remove("dy=0.0")
            except ValueError:
                pass
            try:
                self.datumparams.remove("dz=0.0")
            except ValueError:
                pass

            nextButton.Enable(True)

        self.Update()
        event.Skip()

    def OnDSearch(self, event):
        """Search geodetic datum by desc"""
        search_str = self.searchb.GetValue()
        try:
            self.datum, self.ellipsoid, self.datumdesc = self.datumlist.Search(
                index=[0, 1, 2], pattern=search_str
            )
        except (IndexError, ValueError):
            self.datum = self.datumdesc = self.ellipsoid = ""

        event.Skip()

    def OnSearchCancel(self, event):
        self.datumlist.Search(index=None, pattern="")
        event.Skip()

    def OnDatumSelected(self, event):
        """Datum selected"""
        index = event.GetIndex()

        self.datum = self.datumlist.GetItem(index, 0).GetText()
        self.tdatum.SetValue(self.datum)

        event.Skip()


class EllipsePage(TitledPage):
    """Wizard page for selecting ellipsoid (select coordinate system option)"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Specify ellipsoid"))

        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        self.parent = parent

        self.ellipse = ""
        self.ellipsedesc = ""
        self.ellipseparams = ""
        self.proj4params = ""

        # text input
        self.tellipse = self.MakeTextCtrl("", size=(200, -1))

        # search box
        self.searchb = SearchCtrl(self, size=(200, -1), style=wx.TE_PROCESS_ENTER)
        self.searchb.ShowCancelButton(True)
        # radio buttons
        self.radio1 = wx.RadioButton(
            parent=self, id=wx.ID_ANY, label=_("Earth based"), style=wx.RB_GROUP
        )
        self.radioEpsg = wx.RadioButton(
            parent=self, id=wx.ID_ANY, label=_("Planetary bodies")
        )

        # create list control for ellipse list
        # extract code, desc
        data = [
            [key, ellipsoid[0]] for (key, ellipsoid) in self.parent.ellipsoids.items()
        ]

        self.ellipselist = ItemList(
            self, data=data, columns=[_("Code"), _("Description")]
        )
        self.ellipselist.resizeLastColumn(30)

        # layout

        self.sizer.Add(
            self.MakeLabel(_("Ellipsoid code:")),
            flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 1),
        )
        self.sizer.Add(
            self.tellipse,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 2),
        )
        self.sizer.Add(
            self.radio1,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
            border=25,
            pos=(1, 3),
        )

        self.sizer.Add(
            self.MakeLabel(_("Search in description:")),
            flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(2, 1),
        )
        self.sizer.Add(
            self.searchb,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(2, 2),
        )
        self.sizer.Add(
            self.radioEpsg,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.LEFT,
            border=25,
            pos=(2, 3),
        )

        self.sizer.Add(
            self.ellipselist,
            flag=wx.EXPAND | wx.ALIGN_LEFT | wx.ALL,
            border=5,
            pos=(3, 1),
            span=(1, 4),
        )
        self.sizer.AddGrowableCol(4)
        self.sizer.AddGrowableRow(3)

        # events
        self.ellipselist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.tellipse.Bind(wx.EVT_TEXT, self.OnText)
        self.searchb.Bind(wx.EVT_TEXT, self.OnSearch)
        self.searchb.Bind(search_cancel_evt, self.OnSearchCancel)

        self.radio1.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radio1.GetId())
        self.radioEpsg.Bind(wx.EVT_RADIOBUTTON, self.SetVal, id=self.radioEpsg.GetId())

        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)

    def OnEnterPage(self, event: WizardEvent) -> None:
        if len(self.ellipse) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        self.scope = "earth"
        event.Skip()

    def OnPageChanging(self, event: WizardEvent) -> None:
        if (
            event.GetDirection()
            and self.ellipse not in self.parent.ellipsoids
            and self.ellipse not in self.parent.planetary_ellipsoids
        ):
            event.Veto()

        # print self.ellipse, self.ellipsedesc, self.ellipseparams

        self.proj4params = ""
        self.GetNext().SetPrev(self)
        self.parent.datumpage.datumparams = ""
        # self.GetNext().SetPrev(self) (???)

    # FIXME: index number doesn't translate when you've given a valid name
    # from the other list
    def OnText(self, event):
        """Ellipsoid code changed"""
        self.ellipse = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.ellipse) == 0 or (
            self.ellipse not in self.parent.ellipsoids
            and self.ellipse not in self.parent.planetary_ellipsoids
        ):
            nextButton.Enable(False)
            self.ellipsedesc = ""
            self.ellipseparams = ""
            self.proj4params = ""
        elif self.ellipse in self.parent.ellipsoids:
            self.ellipsedesc = self.parent.ellipsoids[self.ellipse][0]
            self.ellipseparams = self.parent.ellipsoids[self.ellipse][1]
            nextButton.Enable(True)
        elif self.ellipse in self.parent.planetary_ellipsoids:
            self.ellipsedesc = self.parent.planetary_ellipsoids[self.ellipse][0]
            self.ellipseparams = self.parent.planetary_ellipsoids[self.ellipse][1]
            nextButton.Enable(True)
        # print self.ellipse, self.ellipsedesc, self.ellipseparams

    def OnSearch(self, event):
        """Search ellipsoid by desc"""
        try:
            self.ellipse, self.ellipsedesc = self.ellipselist.Search(
                index=[0, 1], pattern=event.GetString()
            )
            if self.scope == "earth":
                self.ellipseparams = self.parent.ellipsoids[self.ellipse][1]
            else:
                self.ellipseparams = self.parent.planetary_ellipsoids[self.ellipse][1]
        except (IndexError, ValueError, KeyError):
            self.ellipse = self.ellipsedesc = self.ellipseparams = ""

        event.Skip()

    def OnSearchCancel(self, event):
        self.ellipselist.Search(index=None, pattern="")
        event.Skip()

    def OnItemSelected(self, event):
        """Ellipsoid selected"""
        index = event.GetIndex()

        self.ellipse = self.ellipselist.GetItem(index, 0).GetText()
        self.tellipse.SetValue(self.ellipse)

        event.Skip()

    def SetVal(self, event):
        """Choose table to use"""
        self.ellipselist.DeleteAllItems()
        data = []
        if event.GetId() == self.radio1.GetId():
            self.scope = "earth"
            for key in self.parent.ellipsoids.keys():
                data.append([key, self.parent.ellipsoids[key][0]])
        elif event.GetId() == self.radioEpsg.GetId():
            self.scope = "planetary"
            for key in self.parent.planetary_ellipsoids.keys():
                data.append([key, self.parent.planetary_ellipsoids[key][0]])

        self.ellipselist.Populate(data=data, update=True)


class GeoreferencedFilePage(TitledPage):
    """Wizard page for selecting georeferenced file to use
    for setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Read CRS from a georeferenced data file"))

        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        # create variables
        self.georeffile = ""

        # create controls
        self.lfile = self.MakeLabel(_("Georeferenced file:"))
        self.tfile = self.MakeTextCtrl(size=(300, -1))
        self.bbrowse = self.MakeButton(_("Browse"))

        # do layout
        self.sizer.Add(
            self.lfile,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTRE_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 1),
        )
        self.sizer.Add(
            self.tfile,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTRE_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 2),
        )
        self.sizer.Add(self.bbrowse, flag=wx.ALIGN_LEFT | wx.ALL, border=5, pos=(1, 3))
        self.sizer.AddGrowableCol(3)

        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.tfile.Bind(wx.EVT_TEXT, self.OnText)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do page layout
        # self.DoLayout()

    def OnEnterPage(self, event: WizardEvent) -> None:
        if len(self.georeffile) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnPageChanging(self, event: WizardEvent) -> None:
        if event.GetDirection() and not os.path.isfile(self.georeffile):
            event.Veto()
        self.GetNext().SetPrev(self)

    def OnText(self, event):
        """File changed"""
        self.georeffile = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.georeffile) > 0 and os.path.isfile(self.georeffile):
            if not nextButton.IsEnabled():
                nextButton.Enable(True)
        elif nextButton.IsEnabled():
            nextButton.Enable(False)

        event.Skip()

    def OnBrowse(self, event):
        """Choose file"""
        dlg = wx.FileDialog(
            self, _("Select georeferenced file"), str(Path.cwd()), "", "*.*", wx.FD_OPEN
        )
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.tfile.SetValue(path)
        dlg.Destroy()

        event.Skip()


class WKTPage(TitledPage):
    """Wizard page for selecting WKT file to use
    for setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Specify CRS using WKT string"))

        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        self.wktstring = ""
        self.parent = parent

        # widgets
        self.text_wkt = self.MakeTextCtrl(size=(400, 200), style=wx.TE_MULTILINE)
        self.label_wkt = self.MakeLabel(_("Enter WKT parameters string:"))

        # layout
        self.sizer.Add(
            self.label_wkt, flag=wx.ALIGN_LEFT | wx.ALL, border=5, pos=(1, 1)
        )
        self.sizer.Add(
            self.text_wkt,
            flag=wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
            border=5,
            pos=(2, 1),
            span=(1, 2),
        )
        self.sizer.AddGrowableRow(2)
        self.sizer.AddGrowableCol(2)

        self.text_wkt.Bind(wx.EVT_TEXT, self.OnText)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event: WizardEvent) -> None:
        if len(self.wktstring) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        event.Skip()

    def OnPageChanging(self, event: WizardEvent) -> None:
        if event.GetDirection() and not self.wktstring.strip():
            event.Veto()
        self.GetNext().SetPrev(self)

    def OnText(self, event):
        """Change WKT string"""
        # TODO: check WKT syntax
        self.wktstring = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.wktstring) == 0:
            if nextButton.IsEnabled():
                nextButton.Enable(False)
        elif not nextButton.IsEnabled():
            nextButton.Enable()


class EPSGPage(TitledPage):
    """Wizard page for selecting EPSG code for
    setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select CRS from a list"))

        self.sizer = wx.BoxSizer(wx.VERTICAL)
        searchBoxSizer = wx.BoxSizer(wx.HORIZONTAL)
        epsglistBoxSizer = wx.BoxSizer(wx.HORIZONTAL)
        informationBoxSizer = wx.BoxSizer(wx.HORIZONTAL)

        # definition of variables
        self.parent = parent
        self.epsgCodeDict = {}
        self.epsgcode = None
        self.epsgdesc = ""
        self.epsgparams = ""

        # labels
        self.lcode = self.MakeLabel(
            _("Filter by EPSG code or description:"),
            style=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
        )
        self.llink = self.MakeLabel(
            _("Find more information at:"),
            style=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
        )

        # search box
        self.searchb = SearchCtrl(self, size=(-1, 30), style=wx.TE_PROCESS_ENTER)
        self.searchb.ShowCancelButton(True)

        self.epsglist = ItemList(
            self, data=None, columns=[_("Code"), _("Description"), _("Parameters")]
        )

        # A hyperlink to a CRS database (PROJ related)
        self.tlink = HyperlinkCtrl(
            self,
            id=wx.ID_ANY,
            label="spatialreference.org",
            url="https://spatialreference.org/",
        )
        self.tlink.SetNormalColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT))
        self.tlink.SetVisitedColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT))

        # layout
        searchBoxSizer.Add(
            self.lcode, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5
        )
        searchBoxSizer.Add(
            self.searchb, proportion=1, flag=wx.ALL | wx.EXPAND, border=5
        )
        epsglistBoxSizer.Add(
            self.epsglist, proportion=1, flag=wx.ALL | wx.EXPAND, border=5
        )
        informationBoxSizer.AddStretchSpacer(1)
        informationBoxSizer.Add(
            self.llink, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL | wx.RIGHT, border=5
        )
        informationBoxSizer.Add(self.tlink, proportion=0, flag=wx.ALIGN_CENTER_VERTICAL)
        self.sizer.Add(searchBoxSizer, proportion=0, flag=wx.EXPAND)
        self.sizer.Add(epsglistBoxSizer, proportion=1, flag=wx.EXPAND)
        self.sizer.Add(
            informationBoxSizer, proportion=0, flag=wx.EXPAND | wx.TOP, border=5
        )

        # events
        self.searchb.Bind(wx.EVT_TEXT, self.OnTextChange)
        self.epsglist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.searchb.Bind(search_cancel_evt, self.OnSearchCancel)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event: WizardEvent) -> None:
        self.parent.datum_trans = None
        if event.GetDirection():
            if not self.epsgcode:
                # disable 'next' button by default
                self.EnableNext(False)
                # load default epsg database file
                self.OnBrowseCodes(None)
            else:
                self.EnableNext(True)

        event.Skip()

    def OnPageChanging(self, event: WizardEvent):
        if event.GetDirection():
            if not self.epsgcode:
                event.Veto()
                return None
            # check for datum transforms
            ret = RunCommand(
                "g.proj", read=True, epsg=self.epsgcode, datum_trans="-1", flags="t"
            )

            if ret != "":
                dtrans = ""
                # open a dialog to select datum transform number
                dlg = SelectTransformDialog(self.parent.parent, transforms=ret)

                if dlg.ShowModal() == wx.ID_OK:
                    dtrans = dlg.GetTransform()
                    if dtrans == "":
                        dlg.Destroy()
                        event.Veto()
                        return "Datum transform is required."
                else:
                    dlg.Destroy()
                    event.Veto()
                    return "Datum transform is required."

                self.parent.datum_trans = dtrans
            self.GetNext().SetPrev(self)

    def EnableNext(self, enable=True):
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        nextButton.Enable(enable)

    def OnTextChange(self, event):
        value = self.searchb.GetValue()
        if value == "":
            self.tlink.SetURL("https://spatialreference.org/")
            self.epsgcode = None
            self.epsgdesc = self.epsgparams = ""
            self.searchb.ChangeValue("")
            self.OnBrowseCodes(None)
            self.EnableNext(False)
        else:
            self.tlink.SetURL(f"https://spatialreference.org/ref/?&search={value}")
            data = self.epsglist.Search(index=[0, 1, 2], pattern=value, firstOnly=False)
            if data:
                index = 0
                # search for the exact epsg code match
                # otherwise just select first item
                try:
                    epsg = int(value)
                    for i, (code, desc, params) in enumerate(data):
                        if code == epsg:
                            index = i
                            break
                except ValueError:
                    pass
                self.epsgcode, self.epsgdesc, self.epsgparams = data[index]
                self.epsglist.Select(index)
                self.epsglist.Focus(index)
                self.EnableNext()
            else:
                self.epsgcode = None
                self.epsgdesc = self.epsgparams = ""
                self.EnableNext(False)

        event.Skip()

    def OnItemSelected(self, event):
        """EPSG code selected from the list"""
        index = event.GetIndex()

        self.epsgcode = int(self.epsglist.GetItem(index, 0).GetText())
        self.epsgdesc = self.epsglist.GetItem(index, 1).GetText()
        self.EnableNext(True)

        event.Skip()

    def OnSearchCancel(self, event):
        self.epsglist.Search(index=None, pattern="")
        event.Skip()

    def OnBrowseCodes(self, event, search=None):
        """Browse EPSG codes"""
        try:
            self.epsgCodeDict = utils.ReadEpsgCodes()
        except OpenError as e:
            GError(
                parent=self,
                message=_("Unable to read EPGS codes: {0}").format(e),
                showTraceback=False,
            )
            self.epsglist.Populate([], update=True)
            return

        data = [
            (code, val[0], val[1])
            for code, val in self.epsgCodeDict.items()
            if code is not None
        ]

        self.epsglist.Populate(data, update=True)


class IAUPage(TitledPage):
    """Wizard page for selecting IAU code/WKT for
    setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Choose IAU Code"))

        # grid definition
        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        # definition of variables
        self.parent = parent
        self.epsgCodeDict = {}
        self.epsgcode = None
        self.epsgdesc = ""
        self.epsgparams = ""

        # labels
        self.lfile = self.MakeLabel(
            _("Path to the IAU-codes file:"),
            style=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL,
        )
        self.lcode = self.MakeLabel(
            _("IAU code:"), style=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL
        )

        # text input
        epsgdir = utils.PathJoin(globalvar.ETCDIR, "proj", "ogr_csv", "iau2009.csv")
        self.tfile = self.MakeTextCtrl(
            text=epsgdir, size=(200, -1), style=wx.TE_PROCESS_ENTER
        )
        self.tcode = self.MakeTextCtrl(size=(200, -1))

        # buttons
        self.bbrowse = self.MakeButton(_("Browse"))

        # search box
        self.searchb = SearchCtrl(self, size=(200, -1), style=wx.TE_PROCESS_ENTER)
        self.searchb.ShowCancelButton(True)

        self.epsglist = ItemList(
            self, data=None, columns=[_("Code"), _("Description"), _("Parameters")]
        )

        # layout
        self.sizer.Add(
            self.lfile,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 1),
            span=(1, 2),
        )
        self.sizer.Add(
            self.tfile,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 3),
        )
        self.sizer.Add(
            self.bbrowse,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(1, 4),
        )
        self.sizer.Add(
            self.lcode,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(2, 1),
            span=(1, 2),
        )
        self.sizer.Add(
            self.tcode,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(2, 3),
        )
        self.sizer.Add(
            self.searchb,
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL,
            border=5,
            pos=(3, 3),
        )

        self.sizer.Add(
            self.epsglist, flag=wx.ALIGN_LEFT | wx.EXPAND, pos=(4, 1), span=(1, 4)
        )
        self.sizer.AddGrowableCol(3)
        self.sizer.AddGrowableRow(4)

        # events
        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.tfile.Bind(wx.EVT_TEXT_ENTER, self.OnBrowseCodes)
        self.tcode.Bind(wx.EVT_TEXT, self.OnText)
        self.epsglist.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.searchb.Bind(wx.EVT_TEXT, self.OnSearch)
        self.searchb.Bind(search_cancel_evt, self.OnSearchCancel)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event: WizardEvent) -> None:
        self.parent.datum_trans = None
        if event.GetDirection():
            if not self.epsgcode:
                # disable 'next' button by default
                wx.FindWindowById(wx.ID_FORWARD).Enable(False)
            else:
                wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        # load default epsg database file
        self.OnBrowseCodes(None)

        event.Skip()

    def OnPageChanging(self, event: WizardEvent):
        if event.GetDirection():
            if not self.epsgcode:
                event.Veto()
                return None
            # check for datum transforms
            ret = RunCommand(
                "g.proj",
                read=True,
                proj4=self.epsgparams,
                datum_trans="-1",
                flags="t",
            )

            if ret != "":
                dtrans = ""
                # open a dialog to select datum transform number
                dlg = SelectTransformDialog(self.parent.parent, transforms=ret)

                if dlg.ShowModal() == wx.ID_OK:
                    dtrans = dlg.GetTransform()
                    if dtrans == "":
                        dlg.Destroy()
                        event.Veto()
                        return "Datum transform is required."
                else:
                    dlg.Destroy()
                    event.Veto()
                    return "Datum transform is required."

                self.parent.datum_trans = dtrans
                self.parent.epsgcode = self.epsgcode
                self.parent.epsgdesc = self.epsgdesc

            # prepare +nadgrids or +towgs84 terms for Summary page. first
            # convert them:
            ret, projlabel, err = RunCommand(
                "g.proj",
                flags="jft",
                proj4=self.epsgparams,
                datum_trans=self.parent.datum_trans,
                getErrorMsg=True,
                read=True,
            )
            # splitting on space alone would break for grid files with
            # space in pathname
            for projterm in projlabel.split(" +"):
                if projterm.find("towgs84=") != -1 or projterm.find("nadgrids=") != -1:
                    self.custom_dtrans_string = " +%s" % projterm
                    break

            self.GetNext().SetPrev(self)

    def OnText(self, event):
        self.epsgcode = event.GetString()
        try:
            self.epsgcode = int(self.epsgcode)
        except ValueError:
            self.epsgcode = None

        nextButton = wx.FindWindowById(wx.ID_FORWARD)

        # if self.epsgcode and self.epsgcode in self.epsgCodeDict.keys():
        if self.epsgcode:
            self.epsgdesc = self.epsgCodeDict[self.epsgcode][0]
            self.epsgparams = self.epsgCodeDict[self.epsgcode][1]
            if not nextButton.IsEnabled():
                nextButton.Enable(True)
        else:
            self.epsgcode = None  # not found
            if nextButton.IsEnabled():
                nextButton.Enable(False)
            self.epsgdesc = self.epsgparams = ""

    def OnSearch(self, event):
        value = self.searchb.GetValue()

        if value == "":
            self.epsgcode = None
            self.epsgdesc = self.epsgparams = ""
            self.tcode.SetValue("")
            self.searchb.SetValue("")
            self.OnBrowseCodes(None)
        else:
            try:
                self.epsgcode, self.epsgdesc, self.epsgparams = self.epsglist.Search(
                    index=[0, 1, 2], pattern=value
                )
            except (IndexError, ValueError):  # -> no item found
                self.epsgcode = None
                self.epsgdesc = self.epsgparams = ""
                self.tcode.SetValue("")

        event.Skip()

    def OnSearchCancel(self, event):
        self.epsglist.Search(index=None, pattern="")
        event.Skip()

    def OnBrowse(self, event):
        """Define path for IAU code file"""
        path = os.path.dirname(self.tfile.GetValue())
        if not path:
            path = str(Path.cwd())

        dlg = wx.FileDialog(
            parent=self,
            message=_("Choose IAU codes file"),
            defaultDir=path,
            defaultFile="",
            wildcard="*",
            style=wx.FD_OPEN,
        )

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            self.tfile.SetValue(path)
            self.OnBrowseCodes(None)

        dlg.Destroy()

        event.Skip()

    def OnItemSelected(self, event):
        """IAU code selected from the list"""
        index = event.GetIndex()

        self.epsgcode = int(self.epsglist.GetItem(index, 0).GetText())
        # This is here that the index 2 (aka WKT) should be loaded in a
        # variable
        self.epsgdesc = self.epsglist.GetItem(index, 1).GetText()
        self.tcode.SetValue(str(self.epsgcode))

        event.Skip()

    def OnBrowseCodes(self, event, search=None):
        """Browse IAU codes"""
        try:
            self.epsgCodeDict = utils.ReadEpsgCodes()
        except OpenError as e:
            GError(
                parent=self,
                message=_("Unable to read IAU codes: {0}").format(e),
                showTraceback=False,
            )
            self.epsglist.Populate([], update=True)
            return

        data = [
            (code, val[0], val[1])
            for code, val in self.epsgCodeDict.items()
            if code is not None
        ]

        self.epsglist.Populate(data, update=True)


class CustomPage(TitledPage):
    """Wizard page for entering custom PROJ.4 string
    for setting coordinate system parameters"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Specify CRS using PROJ.4 string"))
        global coordsys

        # grid definition
        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        # definition of variables
        self.customstring = ""
        self.parent = parent

        # widgets
        self.text_proj4string = self.MakeTextCtrl(
            size=(400, 200), style=wx.TE_MULTILINE
        )
        self.label_proj4string = self.MakeLabel(_("Enter PROJ.4 parameters string:"))

        # layout
        self.sizer.Add(
            self.label_proj4string, flag=wx.ALIGN_LEFT | wx.ALL, border=5, pos=(1, 1)
        )
        self.sizer.Add(
            self.text_proj4string,
            flag=wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
            border=5,
            pos=(2, 1),
            span=(1, 2),
        )
        self.sizer.AddGrowableRow(2)
        self.sizer.AddGrowableCol(2)

        self.text_proj4string.Bind(wx.EVT_TEXT, self.GetProjstring)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event: WizardEvent) -> None:
        if len(self.customstring) == 0:
            # disable 'next' button by default
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

    def OnPageChanging(self, event: WizardEvent):
        if event.GetDirection():
            self.custom_dtrans_string = ""

            if self.customstring.find("+datum=") < 0:
                self.GetNext().SetPrev(self)
                return None

            # check for datum transforms
            # FIXME: -t flag is a hack-around for trac bug #1849
            ret, out, err = RunCommand(
                "g.proj",
                read=True,
                getErrorMsg=True,
                proj4=self.customstring,
                datum_trans="-1",
                flags="t",
            )
            if ret != 0:
                wx.MessageBox(
                    parent=self,
                    message=err,
                    caption=_("Error"),
                    style=wx.OK | wx.ICON_ERROR | wx.CENTRE,
                )
                event.Veto()
                return None

            if out:
                dtrans = ""
                # open a dialog to select datum transform number
                dlg = SelectTransformDialog(self.parent.parent, transforms=out)

                if dlg.ShowModal() == wx.ID_OK:
                    dtrans = dlg.GetTransform()
                    if dtrans == "":
                        dlg.Destroy()
                        event.Veto()
                        return _("Datum transform is required.")
                else:
                    dlg.Destroy()
                    event.Veto()
                    return _("Datum transform is required.")

                self.parent.datum_trans = dtrans

                # prepare +nadgrids or +towgs84 terms for Summary page. first
                # convert them:
                ret, projlabel, err = RunCommand(
                    "g.proj",
                    flags="jft",
                    proj4=self.customstring,
                    datum_trans=dtrans,
                    getErrorMsg=True,
                    read=True,
                )
                # splitting on space alone would break for grid files with
                # space in pathname
                for projterm in projlabel.split(" +"):
                    if (
                        projterm.find("towgs84=") != -1
                        or projterm.find("nadgrids=") != -1
                    ):
                        self.custom_dtrans_string = " +%s" % projterm
                        break

        self.GetNext().SetPrev(self)

    def GetProjstring(self, event):
        """Change proj string"""
        # TODO: check PROJ.4 syntax
        self.customstring = event.GetString()
        nextButton = wx.FindWindowById(wx.ID_FORWARD)
        if len(self.customstring) == 0:
            if nextButton.IsEnabled():
                nextButton.Enable(False)
        elif not nextButton.IsEnabled():
            nextButton.Enable()


class SummaryPage(TitledPage):
    """Shows summary result of choosing coordinate system parameters
    prior to creating location"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Summary"))

        # grid definition
        self.sizer = wx.GridBagSizer(vgap=0, hgap=0)
        self.sizer.SetCols(5)
        self.sizer.SetRows(8)

        # definition of variables
        self.parent = parent

        # panels
        self.panelTitle = scrolled.ScrolledPanel(parent=self, id=wx.ID_ANY)
        self.panelProj4string = scrolled.ScrolledPanel(parent=self, id=wx.ID_ANY)
        self.panelProj = scrolled.ScrolledPanel(parent=self, id=wx.ID_ANY)

        # labels
        self.ldatabase = self.MakeLabel()
        self.llocation = self.MakeLabel()
        self.llocTitle = self.MakeLabel(parent=self.panelTitle)
        self.lprojection = self.MakeLabel(parent=self.panelProj)
        self.lproj4string = self.MakeLabel(parent=self.panelProj4string)

        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        # do sub-page layout
        self._doLayout()

    def _doLayout(self):
        """Do page layout"""

        titleSizer = wx.BoxSizer(wx.VERTICAL)
        titleSizer.Add(self.llocTitle, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        self.panelTitle.SetSizer(titleSizer)

        projSizer = wx.BoxSizer(wx.VERTICAL)
        projSizer.Add(self.lprojection, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        self.panelProj.SetSizer(projSizer)

        proj4stringSizer = wx.BoxSizer(wx.VERTICAL)
        proj4stringSizer.Add(
            self.lproj4string, proportion=1, flag=wx.EXPAND | wx.ALL, border=5
        )
        self.panelProj4string.SetSizer(proj4stringSizer)

        self.panelProj4string.SetupScrolling()
        self.panelProj.SetupScrolling()
        self.panelTitle.SetupScrolling(scroll_y=False)

        self.sizer.Add(
            self.MakeLabel(_("GRASS Database:")),
            flag=wx.ALIGN_LEFT | wx.ALL,
            border=5,
            pos=(1, 0),
        )
        self.sizer.Add(
            self.ldatabase, flag=wx.ALIGN_LEFT | wx.ALL, border=5, pos=(1, 1)
        )
        self.sizer.Add(
            self.MakeLabel(_("Project Name:")),
            flag=wx.ALIGN_LEFT | wx.ALL,
            border=5,
            pos=(2, 0),
        )
        self.sizer.Add(
            self.llocation, flag=wx.ALIGN_LEFT | wx.ALL, border=5, pos=(2, 1)
        )
        self.sizer.Add(
            self.MakeLabel(_("Description:")),
            flag=wx.ALIGN_LEFT | wx.ALL,
            border=5,
            pos=(3, 0),
        )
        self.sizer.Add(
            self.panelTitle,
            flag=wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
            border=0,
            pos=(3, 1),
        )
        self.sizer.Add(
            self.MakeLabel(_("Projection:")),
            flag=wx.ALIGN_LEFT | wx.ALL,
            border=5,
            pos=(4, 0),
        )
        self.sizer.Add(
            self.panelProj,
            flag=wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
            border=0,
            pos=(4, 1),
        )
        self.sizer.Add(
            self.MakeLabel(_("PROJ.4 definition:\n (non-definitive)")),
            flag=wx.ALIGN_LEFT | wx.ALL,
            border=5,
            pos=(5, 0),
        )
        self.sizer.Add(
            self.panelProj4string,
            flag=wx.ALIGN_LEFT | wx.ALL | wx.EXPAND,
            border=0,
            pos=(5, 1),
        )
        self.sizer.AddGrowableCol(1)
        self.sizer.AddGrowableRow(4, 1)
        self.sizer.AddGrowableRow(5, 5)

    def OnEnterPage(self, event: WizardEvent) -> None:
        """Insert values into text controls for summary of location
        creation options
        """
        database = self.parent.startpage.grassdatabase
        location = self.parent.startpage.location
        proj4string = self.parent.CreateProj4String()
        iauproj4string = self.parent.iaupage.epsgparams
        epsgcode = self.parent.epsgpage.epsgcode
        datum = self.parent.datumpage.datum
        dtrans = self.parent.datum_trans
        global coordsys

        # print coordsys,proj4string
        if coordsys in {"proj", "epsg", "iau", "wkt", "file"}:
            extra_opts = {"project": "project", "getErrorMsg": True, "read": True}

            if coordsys == "proj":
                if len(datum) > 0:
                    extra_opts["datum"] = "%s" % datum
                    extra_opts["datum_trans"] = dtrans

                ret, projlabel, err = RunCommand(
                    "g.proj", flags="jf", proj4=proj4string, **extra_opts
                )
            elif coordsys == "iau":
                if len(datum) > 0:
                    extra_opts["datum"] = "%s" % datum
                    extra_opts["datum_trans"] = dtrans

                ret, projlabel, err = RunCommand(
                    "g.proj", flags="jf", proj4=iauproj4string, **extra_opts
                )
            elif coordsys == "epsg":
                ret, projlabel, err = RunCommand(
                    "g.proj",
                    flags="jft",
                    epsg=epsgcode,
                    datum_trans=dtrans,
                    **extra_opts,
                )
            elif coordsys == "file":
                ret, projlabel, err = RunCommand(
                    "g.proj",
                    flags="jft",
                    georef=self.parent.filepage.georeffile,
                    **extra_opts,
                )
            elif coordsys == "wkt":
                ret, projlabel, err = RunCommand(
                    "g.proj",
                    flags="jft",
                    wkt="-",
                    stdin=self.parent.wktpage.wktstring,
                    **extra_opts,
                )

            finishButton = wx.FindWindowById(wx.ID_FORWARD)
            if ret == 0:
                if datum != "":
                    projlabel += "+datum=%s" % datum
                self.lproj4string.SetLabel(projlabel.replace(" +", os.linesep + "+"))
                finishButton.Enable(True)
            else:
                GError(err, parent=self)
                self.lproj4string.SetLabel("")
                finishButton.Enable(False)

        projdesc = self.parent.projpage.projdesc
        ellipsedesc = self.parent.ellipsepage.ellipsedesc
        datumdesc = self.parent.datumpage.datumdesc
        # print projdesc,ellipsedesc,datumdesc
        self.ldatabase.SetLabel(database)
        self.llocation.SetLabel(location)
        self.llocTitle.SetLabel(self.parent.startpage.locTitle)

        label = ""
        if coordsys == "epsg":
            label = "EPSG code %s (%s)" % (
                self.parent.epsgpage.epsgcode,
                self.parent.epsgpage.epsgdesc,
            )
        elif coordsys == "iau":
            label = "IAU code %s (%s)" % (
                self.parent.iaupage.epsgcode,
                self.parent.iaupage.epsgdesc,
            )
        elif coordsys == "file":
            label = "matches file %s" % self.parent.filepage.georeffile

        elif coordsys == "wkt":
            label = "matches WKT string %s" % self.parent.wktpage.wktstring

        elif coordsys == "proj":
            label = "%s, %s %s" % (projdesc, datumdesc, ellipsedesc)

        elif coordsys == "xy":
            label = "XY coordinate system (not projected)."
            self.lproj4string.SetLabel("")

        elif coordsys == "custom":
            label = _("custom")
            combo_str = (
                self.parent.custompage.customstring
                + self.parent.custompage.custom_dtrans_string
            )
            self.lproj4string.SetLabel("%s" % combo_str.replace(" +", os.linesep + "+"))

        self.lprojection.SetLabel(label)


class LocationWizard(wx.Object):
    """Start wizard here and finish wizard here"""

    def __init__(self, parent, grassdatabase):
        self.__cleanUp()

        global coordsys
        self.parent = parent

        #
        # get georeferencing information from tables in $GISBASE/etc
        #
        self.__readData()

        #
        # datum transform number and list of datum transforms
        #
        self.datum_trans = None
        self.proj4string = ""

        # file from which new location is created
        self.georeffile = None

        # additional settings
        self.default_region = False
        self.user_mapset = False

        #
        # define wizard pages
        #
        self.wizard = WizardWithHelpButton(
            parent, id=wx.ID_ANY, title=_("Define new GRASS project")
        )
        self.wizard.Bind(wiz.EVT_WIZARD_HELP, self.OnHelp)

        self.startpage = DatabasePage(self.wizard, self, grassdatabase)
        self.csystemspage = CoordinateSystemPage(self.wizard, self)
        self.projpage = ProjectionsPage(self.wizard, self)
        self.datumpage = DatumPage(self.wizard, self)
        self.paramspage = ProjParamsPage(self.wizard, self)
        self.epsgpage = EPSGPage(self.wizard, self)
        self.iaupage = IAUPage(self.wizard, self)
        self.filepage = GeoreferencedFilePage(self.wizard, self)
        self.wktpage = WKTPage(self.wizard, self)
        self.ellipsepage = EllipsePage(self.wizard, self)
        self.custompage = CustomPage(self.wizard, self)
        self.sumpage = SummaryPage(self.wizard, self)

        #
        # set the initial order of the pages
        # (should follow the epsg line)
        #
        self.startpage.SetNext(self.csystemspage)

        self.csystemspage.SetPrev(self.startpage)
        self.csystemspage.SetNext(self.sumpage)

        self.projpage.SetPrev(self.csystemspage)
        self.projpage.SetNext(self.paramspage)

        self.paramspage.SetPrev(self.projpage)
        self.paramspage.SetNext(self.datumpage)

        self.datumpage.SetPrev(self.paramspage)
        self.datumpage.SetNext(self.sumpage)

        self.ellipsepage.SetPrev(self.paramspage)
        self.ellipsepage.SetNext(self.sumpage)

        self.epsgpage.SetPrev(self.csystemspage)
        self.epsgpage.SetNext(self.sumpage)

        self.iaupage.SetPrev(self.csystemspage)
        self.iaupage.SetNext(self.sumpage)

        self.filepage.SetPrev(self.csystemspage)
        self.filepage.SetNext(self.sumpage)

        self.wktpage.SetPrev(self.csystemspage)
        self.wktpage.SetNext(self.sumpage)

        self.custompage.SetPrev(self.csystemspage)
        self.custompage.SetNext(self.sumpage)

        self.sumpage.SetPrev(self.csystemspage)

        #
        # do pages layout
        #
        self.startpage.DoLayout()
        self.csystemspage.DoLayout()
        self.projpage.DoLayout()
        self.datumpage.DoLayout()
        self.paramspage.DoLayout()
        self.epsgpage.DoLayout()
        self.iaupage.DoLayout()
        self.filepage.DoLayout()
        self.wktpage.DoLayout()
        self.ellipsepage.DoLayout()
        self.custompage.DoLayout()
        self.sumpage.DoLayout()
        self.wizard.FitToPage(self.datumpage)
        size = self.wizard.GetPageSize()
        self.wizard.SetPageSize((size[0], size[1] + 75))

        # new location created?
        self.location = None

        # location created in different GIS database?
        self.altdb = False

        #
        # run wizard...
        #
        if self.wizard.RunWizard(self.startpage):
            msg = self.OnWizFinished()
            if not msg:
                self.wizard.Destroy()
                self.location = self.startpage.location
                self.grassdatabase = self.startpage.grassdatabase
                self.georeffile = self.filepage.georeffile
                # FIXME here was code for setting default region, what for is this if:
                # if self.altdb is False:

            else:  # -> error
                self.wizard.Destroy()
                GError(
                    parent=self.parent,
                    message="%s"
                    % _(
                        "Unable to create new project. "
                        "Project <%(loc)s> not created.\n\n"
                        "Details: %(err)s"
                    )
                    % {"loc": self.startpage.location, "err": msg},
                )
        else:  # -> canceled
            self.wizard.Destroy()

        self.__cleanUp()

    def __cleanUp(self):
        global coordsys, north, south, east, west, resolution, wizerror, translist

        coordsys = None
        north = None
        south = None
        east = None
        west = None
        resolution = None

    def __readData(self):
        """Get georeferencing information from tables in $GISBASE/etc/proj"""

        # read projection and parameters
        self.projections = {}
        self.projdesc = {}
        with open(os.path.join(globalvar.ETCDIR, "proj", "parms.table")) as f:
            for line in f:
                line = line.strip()
                try:
                    proj, projdesc, params = line.split(":")
                    paramslist = params.split(";")
                    plist = []
                    for p in paramslist:
                        if p == "":
                            continue
                        p1, pdefault = p.split(",")
                        pterm, pask = p1.split("=")
                        p = [pterm.strip(), pask.strip(), pdefault.strip()]
                        plist.append(p)
                    self.projections[proj.lower().strip()] = (projdesc.strip(), plist)
                    self.projdesc[proj.lower().strip()] = projdesc.strip()
                except (ValueError, IndexError):
                    continue

        # read datum definitions
        self.datums = {}
        paramslist = []
        with open(os.path.join(globalvar.ETCDIR, "proj", "datum.table")) as f:
            for line in f:
                line = line.expandtabs(1)
                line = line.strip()
                if line == "" or line[0] == "#":
                    continue
                datum, info = line.split(" ", 1)
                info = info.strip()
                datumdesc, params = info.split(" ", 1)
                datumdesc = datumdesc.strip('"')
                paramlist = params.split()
                ellipsoid = paramlist.pop(0)
                self.datums[datum] = (ellipsoid, datumdesc.replace("_", " "), paramlist)

        # read Earth-based ellipsiod definitions
        self.ellipsoids = {}
        with open(os.path.join(globalvar.ETCDIR, "proj", "ellipse.table")) as f:
            for line in f:
                line = line.expandtabs(1)
                line = line.strip()
                if line == "" or line[0] == "#":
                    continue
                ellipse, rest = line.split(" ", 1)
                rest = rest.strip('" ')
                desc, params = rest.split('"', 1)
                desc = desc.strip('" ')
                paramslist = params.split()
                self.ellipsoids[ellipse] = (desc, paramslist)

        # read Planetary ellipsiod definitions
        self.planetary_ellipsoids = {}
        with open(
            os.path.join(globalvar.ETCDIR, "proj", "ellipse.table.solar.system")
        ) as f:
            for line in f:
                line = line.expandtabs(1)
                line = line.strip()
                if line == "" or line[0] == "#":
                    continue
                ellipse, rest = line.split(" ", 1)
                rest = rest.strip('" ')
                desc, params = rest.split('"', 1)
                desc = desc.strip('" ')
                paramslist = params.split()
                self.planetary_ellipsoids[ellipse] = (desc, paramslist)

        # read projection parameter description and parsing table
        self.paramdesc = {}
        with open(os.path.join(globalvar.ETCDIR, "proj", "desc.table")) as f:
            for line in f:
                line = line.strip()
                try:
                    pparam, datatype, proj4term, desc = line.split(":")
                    self.paramdesc[pparam] = (datatype, proj4term, desc)
                except ValueError:
                    continue

    def OnWizFinished(self):
        """Wizard finished, create new location

        :return: error message on error
        :return: None on success
        """
        database = self.startpage.grassdatabase
        location = self.startpage.location

        # location already exists?
        if os.path.isdir(os.path.join(database, location)):
            GError(
                parent=self.wizard,
                message="%s <%s>: %s"
                % (
                    _("Unable to create new project"),
                    os.path.join(database, location),
                    _("Project already exists in GRASS Database."),
                ),
            )
            return None

        # current GISDbase or a new one?
        current_gdb = decode(grass.gisenv()["GISDBASE"])
        if current_gdb != database:
            # change to new GISDbase or create new one
            if not os.path.isdir(database):
                # create new directory
                try:
                    os.mkdir(database)
                except OSError as error:
                    GError(
                        parent=self.wizard,
                        message=_(
                            "Unable to create new GRASS Database <{path}>: {error}"
                        ).format(path=database, error=error),
                    )
                    return None

            # location created in alternate GISDbase
            self.altdb = True

        global coordsys
        try:
            if coordsys == "xy":
                grass.create_location(
                    dbase=self.startpage.grassdatabase,
                    location=self.startpage.location,
                    desc=self.startpage.locTitle,
                )
            elif coordsys == "proj":
                grass.create_location(
                    dbase=self.startpage.grassdatabase,
                    location=self.startpage.location,
                    proj4=self.CreateProj4String(),
                    datum=self.datumpage.datum,
                    datum_trans=self.datum_trans,
                    desc=self.startpage.locTitle,
                )
            elif coordsys == "custom":
                addl_opts = {}
                if self.datum_trans is not None:
                    addl_opts["datum_trans"] = self.datum_trans

                grass.create_location(
                    dbase=self.startpage.grassdatabase,
                    location=self.startpage.location,
                    proj4=self.custompage.customstring,
                    desc=self.startpage.locTitle,
                    **addl_opts,
                )
            elif coordsys == "epsg":
                if not self.epsgpage.epsgcode:
                    return _("EPSG code missing.")

                grass.create_location(
                    dbase=self.startpage.grassdatabase,
                    location=self.startpage.location,
                    epsg=self.epsgpage.epsgcode,
                    datum=self.datumpage.datum,
                    datum_trans=self.datum_trans,
                    desc=self.startpage.locTitle,
                )
            elif coordsys == "iau":
                if not self.iaupage.epsgcode:
                    return _("IAU code missing.")

                grass.create_location(
                    dbase=self.startpage.grassdatabase,
                    location=self.startpage.location,
                    proj4=self.iaupage.epsgparams,
                    datum=self.datumpage.datum,
                    datum_trans=self.datum_trans,
                    desc=self.startpage.locTitle,
                )
            elif coordsys == "file":
                if not self.filepage.georeffile or not os.path.isfile(
                    self.filepage.georeffile
                ):
                    return _("File <%s> not found.") % self.filepage.georeffile

                grass.create_location(
                    dbase=self.startpage.grassdatabase,
                    location=self.startpage.location,
                    filename=self.filepage.georeffile,
                    desc=self.startpage.locTitle,
                )
            elif coordsys == "wkt":
                if not self.wktpage.wktstring:
                    return _("WKT string missing.")

                grass.create_location(
                    dbase=self.startpage.grassdatabase,
                    location=self.startpage.location,
                    wkt=self.wktpage.wktstring,
                    desc=self.startpage.locTitle,
                )

        except ScriptError as e:
            return e.value

        return None

    def CreateProj4String(self):
        """Construct PROJ.4 string"""
        proj = self.projpage.p4proj
        proj4params = self.paramspage.p4projparams

        datumparams = self.datumpage.datumparams
        ellipseparams = self.ellipsepage.ellipseparams

        #
        # creating PROJ.4 string
        #
        proj4string = "%s %s" % (proj, proj4params)

        # set ellipsoid parameters
        for item in ellipseparams:
            item = " +rf=" + item[4:] if item[:4] == "f=1/" else " +" + item
            proj4string = "%s %s" % (proj4string, item)

        # set datum transform parameters if relevant
        if datumparams:
            for item in datumparams:
                proj4string = "%s +%s" % (proj4string, item)

        return "%s +no_defs" % proj4string

    def OnHelp(self, event: WizardEvent) -> None:
        """'Help' button clicked"""

        # help text in lib/init/helptext.html
        RunCommand("g.manual", entry="helptext")


class WizardWithHelpButton(Wizard):
    def __init__(self, parent, id, title):
        if globalvar.wxPythonPhoenix:
            Wizard.__init__(self)
            self.SetExtraStyle(wx.adv.WIZARD_EX_HELPBUTTON)
            self.Create(parent=parent, id=id, title=title)
        else:
            pre = wiz.PreWizard()
            pre.SetExtraStyle(wx.wizard.WIZARD_EX_HELPBUTTON)
            pre.Create(parent=parent, id=id, title=title)
            self.PostCreate(pre)
